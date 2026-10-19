#ifndef BK_UI_PROBE_INPUT_H
#define BK_UI_PROBE_INPUT_H
#include "ui/canvas.h"
/* Probes pick in the logical canvas and submit ordinary drawable input. */
static inline float probe_ui_x(const BkViewport *display, float x) {
  return display->x + x * display->width / BK_UI_WIDTH;
}
static inline float probe_ui_y(const BkViewport *display, float y) {
  return display->y + y * display->height / BK_UI_HEIGHT;
}
static inline float probe_ui_motion_x(const BkViewport *display, float x) {
  return x * display->width / BK_UI_WIDTH;
}
#endif
