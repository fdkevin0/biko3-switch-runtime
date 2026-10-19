#include "scene/save_menu_labels.h"
#include <stdio.h>
#include <string.h>
static void full(uint8_t *out, size_t *n, uint8_t ascii, int chinese) {
  uint16_t code;
  if (chinese)
    code = ascii >= '0' && ascii <= '9' ? 0xa3b0 + ascii - '0'
           : ascii == '/'               ? 0xa3af
           : ascii == ':'               ? 0xa3ba
                                        : 0xa3ad;
  else
    code = ascii >= '0' && ascii <= '9' ? 0x824f + ascii - '0'
           : ascii == '/'               ? 0x815e
           : ascii == ':'               ? 0x8146
                                        : 0x817c;
  out[(*n)++] = (uint8_t)(code >> 8);
  out[(*n)++] = (uint8_t)code;
}
int bk_save_menu_labels(const BkSaveMenuLabel records[10], int chinese,
                        uint8_t out[512], size_t *size, char e[256]) {
  if (!records || !out || !size) {
    snprintf(e, 256, "save labels: invalid input/output");
    return 0;
  }
  uint8_t tmp[512] = {0};
  size_t n = 0;
  for (unsigned i = 0; i < 10; ++i) {
    const BkSaveMenuLabel *r = &records[i];
    full(tmp, &n, (uint8_t)('0' + (i + 1) / 10), chinese);
    full(tmp, &n, (uint8_t)('0' + (i + 1) % 10), chinese);
    full(tmp, &n, '-', chinese);
    if (!r->stamp[0]) {
      for (unsigned k = 0; k < 19; ++k)
        full(tmp, &n, '-', chinese);
      continue;
    }
    if (r->area > 8) {
      snprintf(e, 256, "save labels: invalid occupied area");
      return 0;
    }
    /* Both locales keep the original three-glyph prefix width. */
    static const uint8_t prefix[2][6] = {
        {0x83, 0x47, 0x83, 0x8a, 0x83, 0x41}, /* エリア (Shift-JIS) */
        {0xb3, 0xa1, 0xbe, 0xb0, 0xba, 0xc5}}; /* 场景号 (GBK) */
    memcpy(tmp + n, prefix[chinese != 0], sizeof(prefix[0]));
    n += sizeof(prefix[0]);
    full(tmp, &n, (uint8_t)('1' + r->area), chinese);
    full(tmp, &n, '-', chinese);
    for (unsigned k = 2; k < 16; ++k) {
      uint8_t ch = (uint8_t)r->stamp[k];
      if ((ch >= '0' && ch <= '9') || ch == '/' || ch == '-' || ch == ':')
        full(tmp, &n, ch, chinese);
    }
  }
  memcpy(out, tmp, sizeof(tmp));
  *size = n;
  return 1;
}
