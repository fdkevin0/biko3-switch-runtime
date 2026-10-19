#ifndef BK_SDL_INPUT_H
#define BK_SDL_INPUT_H
#include "core/input.h"
#include <SDL3/SDL.h>

static inline void bk_sdl_pointer_scale(SDL_Window *window, unsigned width,
                                        unsigned height, float *sx, float *sy) {
  /* SDL mouse coordinates are logical window points, not drawable pixels. */
  int w = 0, h = 0;
  *sx = *sy = 1;
  if (!window || !SDL_GetWindowSize(window, &w, &h) || w <= 0 || h <= 0)
    return;
  *sx = (float)width / (float)w;
  *sy = (float)height / (float)h;
}

static inline float bk_sdl_axis_value(SDL_Gamepad *pad, SDL_GamepadAxis axis) {
  return (float)SDL_GetGamepadAxis(pad, axis) / 32768.0f;
}
typedef struct {
  SDL_Gamepad *pads[4];
  SDL_JoystickID pad_ids[4];
  unsigned pad_count;
  uint32_t previous_held;
  float pointer_x, pointer_y, mouse_x, mouse_y;
  int mouse_moved, touch_active;
  unsigned debug_pointer_events;
} BkSdlInput;

/* Zero-initialized owner; platform retains window, focus and menu events. */
void bk_sdl_input_open(BkSdlInput *);
void bk_sdl_input_close(BkSdlInput *);
void bk_sdl_input_event(BkSdlInput *, SDL_Window *, unsigned width,
                        unsigned height, const SDL_Event *);
void bk_sdl_input_poll(BkSdlInput *, SDL_Window *, unsigned width,
                       unsigned height, BkInput *);
#endif
