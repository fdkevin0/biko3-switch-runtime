#include "render/renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr)                                                            \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr);               \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

int main(void) {
  char error[256] = {0};
  BkRenderer *renderer = bk_renderer_create(64, 64, stderr, error);
  if (!renderer) {
    fprintf(stderr, "renderer resize probe: %s\n", error);
    return 77; /* Vulkan unavailable: skip rather than fail a non-GPU host. */
  }
  unsigned width = 0, height = 0;
  bk_renderer_extent(renderer, &width, &height);
  CHECK(width == 64 && height == 64);
  CHECK(bk_renderer_resize(renderer, 128, 96, error));
  bk_renderer_extent(renderer, &width, &height);
  CHECK(width == 128 && height == 96);
  CHECK(bk_renderer_resize(renderer, 96, 128, error));
  bk_renderer_extent(renderer, &width, &height);
  CHECK(width == 96 && height == 128);

  uint8_t pixel[4] = {255, 80, 20, 255};
  BkImage image = {1, 1, pixel};
  BkTexture *texture = bk_texture_create(renderer, &image, error);
  CHECK(texture != NULL);
  BkVertex quad[6] = {
      {-1, -1, 0, 0, 0, 1, 1, 1, 1}, {1, -1, 0, 1, 0, 1, 1, 1, 1},
      {1, 1, 0, 1, 1, 1, 1, 1, 1},   {-1, -1, 0, 0, 0, 1, 1, 1, 1},
      {1, 1, 0, 1, 1, 1, 1, 1, 1},   {-1, 1, 0, 0, 1, 1, 1, 1, 1}};
  CHECK(bk_renderer_begin(renderer, error));
  CHECK(bk_renderer_draw(renderer, texture, quad, 6, bk_identity, error));
  CHECK(bk_renderer_end(renderer, error));
  /* The submitted frame is still in flight here; a window resize commonly
   * arrives exactly between end() and the next begin(). */
  CHECK(bk_renderer_resize(renderer, 80, 80, error));
  bk_renderer_extent(renderer, &width, &height);
  CHECK(width == 80 && height == 80);
  CHECK(bk_renderer_begin(renderer, error));
  CHECK(bk_renderer_draw(renderer, texture, quad, 6, bk_identity, error));
  CHECK(bk_renderer_end(renderer, error));
  uint8_t *rgba = calloc((size_t)width * height, 4);
  CHECK(rgba != NULL);
  CHECK(bk_renderer_readback(renderer, rgba, (size_t)width * height * 4, error));
  free(rgba);
  bk_texture_destroy(renderer, texture);
  bk_renderer_destroy(renderer);
  puts("renderer-resize ok");
  return 0;
}
