#ifndef BK_UI_CANVAS_H
#define BK_UI_CANVAS_H
#include "core/input.h"
#include <stdio.h>
/* Original menus author coordinates in 1280x960. Physical pixels belong to
 * the viewport; retaining this canvas avoids resizing live animation state. */
#define BK_UI_WIDTH 1280u
#define BK_UI_HEIGHT 960u
static inline BkViewport bk_ui_canvas(void) {
  return (BkViewport){0, 0, BK_UI_WIDTH, BK_UI_HEIGHT};
}
/* Projection extents are logical; they may exceed a small drawable. */
static inline int bk_ui_extent(unsigned width, unsigned height) {
  return width && height && width <= 16384 && height <= 16384;
}
static inline int bk_ui_input(BkInput *out, const BkInput *in,
                               const BkViewport *display, char error[256]) {
  if (!out || !in || !display || !display->width || !display->height) {
    snprintf(error, 256, "UI canvas: invalid input/display viewport");
    return 0;
  }
  BkInput mapped = *in;
  float x = (float)BK_UI_WIDTH / display->width;
  float y = (float)BK_UI_HEIGHT / display->height;
  mapped.pointer_x = (in->pointer_x - display->x) * x;
  mapped.pointer_y = (in->pointer_y - display->y) * y;
  mapped.pointer_motion_x *= x;
  mapped.pointer_motion_y *= y;
  *out = mapped;
  return 1;
}
#endif
