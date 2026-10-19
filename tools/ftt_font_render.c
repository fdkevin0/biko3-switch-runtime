/* Rebuild a Biko3 FTT (32-pixel fixed-cell bitmap font) from a TTF/OTF face.
 *
 * The game indexes GBK/Shift-JIS bytes directly, so this tool maps each
 * non-empty FTT code through GBK to Unicode, rasterizes the glyph with
 * FreeType, and writes a new FTT with the original advance widths retained.
 * Codes the source font cannot map keep their original bitmap.
 *
 * Build:
 *   cc -O2 -o ftt-font tools/ftt_font_render.c $(pkg-config --cflags --libs freetype2)
 * Run:
 *   ./ftt-font Type_S.FTT Type_S.pretty.FTT NotoSansCJK-Medium.ttc 2 30
 */
#include <errno.h>
#include <iconv.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define TABLE_BYTES 0x60000u
#define BITMAP_START (TABLE_BYTES + 4u)
#define GLYPHS 32768u

static uint16_t u16(const uint8_t *p) {
  return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}
static uint32_t u32(const uint8_t *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
         (uint32_t)p[3] << 24;
}
static void put16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}
static void put32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}
static uint8_t *read_file(const char *path, size_t *size) {
  FILE *stream = fopen(path, "rb");
  if (!stream) {
    fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
    return NULL;
  }
  if (fseek(stream, 0, SEEK_END) || (*size = (size_t)ftell(stream)) == 0 ||
      fseek(stream, 0, SEEK_SET)) {
    fclose(stream);
    fprintf(stderr, "cannot size %s\n", path);
    return NULL;
  }
  uint8_t *data = malloc(*size);
  if (!data || fread(data, 1, *size, stream) != *size) {
    free(data);
    fclose(stream);
    fprintf(stderr, "cannot read %s\n", path);
    return NULL;
  }
  fclose(stream);
  return data;
}
static int append(uint8_t **buffer, size_t *used, size_t *capacity,
                  const uint8_t *data, size_t size) {
  if (size > *capacity - *used) {
    size_t next = *capacity ? *capacity : 65536;
    while (next < *used + size)
      next *= 2;
    uint8_t *grown = realloc(*buffer, next);
    if (!grown)
      return 0;
    *buffer = grown;
    *capacity = next;
  }
  memcpy(*buffer + *used, data, size);
  *used += size;
  return 1;
}
static uint32_t gbk_unicode(uint16_t code) {
  iconv_t converter = iconv_open("UTF-32LE", "GBK");
  if (converter == (iconv_t)-1)
    return 0;
  char input[2] = {(char)(code >> 8), (char)code};
  char output[8] = {0};
  char *in = input, *out = output;
  size_t in_left = 2, out_left = 4;
  size_t result = iconv(converter, &in, &in_left, &out, &out_left);
  iconv_close(converter);
  if (result == (size_t)-1 || in_left || out_left != 0)
    return 0;
  return (uint32_t)(uint8_t)output[0] | (uint32_t)(uint8_t)output[1] << 8 |
         (uint32_t)(uint8_t)output[2] << 16 |
         (uint32_t)(uint8_t)output[3] << 24;
}
int main(int argc, char **argv) {
  if (argc != 6) {
    fprintf(stderr,
            "usage: %s source.ftt output.ftt font.ttc face-index pixel-height\n",
            argv[0]);
    return 2;
  }
  size_t source_size = 0;
  uint8_t *source = read_file(argv[1], &source_size);
  if (!source || source_size < BITMAP_START) {
    free(source);
    return 1;
  }
  uint32_t cell = u32(source);
  if (!cell || cell > 4096) {
    fprintf(stderr, "unsupported cell size %u\n", cell);
    free(source);
    return 1;
  }
  int face_index = atoi(argv[4]);
  int pixel_height = atoi(argv[5]);
  if (pixel_height < 8 || pixel_height > 64 || pixel_height > (int)cell) {
    fprintf(stderr, "pixel height must fit the FTT cell\n");
    free(source);
    return 1;
  }
  FT_Library library;
  FT_Face face;
  if (FT_Init_FreeType(&library)) {
    fprintf(stderr, "FreeType init failed\n");
    free(source);
    return 1;
  }
  if (FT_New_Face(library, argv[3], face_index, &face)) {
    fprintf(stderr, "cannot open face %s\n", argv[3]);
    FT_Done_FreeType(library);
    free(source);
    return 1;
  }
  if (FT_Set_Pixel_Sizes(face, 0, (FT_UInt)pixel_height)) {
    fprintf(stderr, "cannot set pixel size\n");
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    free(source);
    return 1;
  }
  size_t capacity = BITMAP_START + (1u << 20);
  uint8_t *out = calloc(capacity, 1);
  if (!out) {
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    free(source);
    return 1;
  }
  put32(out, cell);
  size_t used = BITMAP_START;
  unsigned replaced = 0, kept = 0;
  size_t row_bytes = (cell + 7) / 8;
  uint8_t *mask = calloc(pixel_height, row_bytes);
  if (!mask) {
    free(out);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    free(source);
    return 1;
  }
  for (uint32_t code = 0x8000; code <= 0xffff; code++) {
    const uint8_t *record = source + 4 + (code - 0x8000u) * 12;
    uint32_t old_offset = u32(record);
    uint16_t old_pitch = u16(record + 4);
    uint16_t old_width = u16(record + 6);
    uint16_t old_height = u16(record + 8);
    if (!old_pitch || !old_height || old_offset > source_size - BITMAP_START ||
        (size_t)old_pitch * old_height > source_size - BITMAP_START - old_offset) {
      kept++;
      continue;
    }
    const uint8_t *old_bits = source + BITMAP_START + old_offset;
    uint32_t unicode = gbk_unicode((uint16_t)code);
    int rendered = 0;
    if (unicode) {
      FT_UInt index = FT_Get_Char_Index(face, unicode);
      if (index && !FT_Load_Glyph(face, index, FT_LOAD_RENDER) &&
          face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY &&
          face->glyph->bitmap.width && face->glyph->bitmap.rows) {
        const FT_Bitmap *bitmap = &face->glyph->bitmap;
        memset(mask, 0, (size_t)pixel_height * row_bytes);
        int x0 = ((int)cell - (int)bitmap->width) / 2;
        int y0 = (pixel_height - (int)bitmap->rows) / 2;
        for (unsigned y = 0; y < bitmap->rows; y++)
          for (unsigned x = 0; x < bitmap->width; x++) {
            int px = x0 + (int)x, py = y0 + (int)y;
            if (px < 0 || py < 0 || px >= (int)cell ||
                py >= pixel_height)
              continue;
            if (bitmap->buffer[y * bitmap->pitch + x] >= 96)
              mask[(size_t)py * row_bytes + (size_t)px / 8] |=
                  (uint8_t)(0x80u >> (px & 7));
          }
        if (append(&out, &used, &capacity, mask,
                   (size_t)pixel_height * row_bytes)) {
          uint8_t *target = out + 4 + (code - 0x8000u) * 12;
          put32(target, (uint32_t)(used - (size_t)pixel_height * row_bytes -
                                   (size_t)BITMAP_START));
          put16(target + 4, (uint16_t)row_bytes);
          put16(target + 6, old_width ? old_width : (uint16_t)cell);
          put16(target + 8, (uint16_t)pixel_height);
          put16(target + 10, 0);
          rendered = 1;
          replaced++;
        }
      }
    }
    if (!rendered) {
      if (!append(&out, &used, &capacity, old_bits,
                  (size_t)old_pitch * old_height)) {
        fprintf(stderr, "out of memory\n");
        free(mask);
        free(out);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        free(source);
        return 1;
      }
      uint8_t *target = out + 4 + (code - 0x8000u) * 12;
      put32(target, (uint32_t)(used - (size_t)old_pitch * old_height -
                               (size_t)BITMAP_START));
      put16(target + 4, old_pitch);
      put16(target + 6, old_width);
      put16(target + 8, old_height);
      put16(target + 10, 0);
      kept++;
    }
  }
  free(mask);
  FILE *stream = fopen(argv[2], "wb");
  if (!stream || fwrite(out, 1, used, stream) != used || fclose(stream)) {
    fprintf(stderr, "cannot write %s\n", argv[2]);
    free(out);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    free(source);
    return 1;
  }
  printf("wrote %s: %zu bytes, %u glyphs replaced, %u kept\n", argv[2], used,
         replaced, kept);
  free(out);
  FT_Done_Face(face);
  FT_Done_FreeType(library);
  free(source);
  return 0;
}
