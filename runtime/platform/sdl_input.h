#ifndef BK_SDL_INPUT_H
#define BK_SDL_INPUT_H
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
#endif
