#include "platform/sdl_input.h"
#include "core/input.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int logical_w = 640, logical_h = 480, window_available = 1;
static Sint16 raw_axis;
bool SDLCALL SDL_GetWindowSize(SDL_Window *window, int *w, int *h) {
  assert(window);
  if (!window_available)
    return false;
  *w = logical_w;
  *h = logical_h;
  return true;
}
bool SDLCALL SDL_GetWindowSizeInPixels(SDL_Window *window, int *w, int *h) {
  assert(window);
  *w = logical_w * 2;
  *h = logical_h * 2;
  return true;
}
Sint16 SDLCALL SDL_GetGamepadAxis(SDL_Gamepad *pad, SDL_GamepadAxis axis) {
  assert(pad && axis == SDL_GAMEPAD_AXIS_LEFTX);
  return raw_axis;
}

static void pointer_test(void) {
  int fake_window;
  SDL_Window *window = (SDL_Window *)&fake_window;
  float sx, sy;
  bk_sdl_pointer_scale(window, 1280, 960, &sx, &sy);
  assert(sx == 2 && sy == 2);
  BkViewport viewport;
  assert(bk_camera_fit(&viewport, 1280, 960, 4, 3));
  BkVirtualPointer pointer = {0};
  BkInput input = {.pointer_active = 1, .pointer_x = 320 * sx,
                  .pointer_y = 240 * sy};
  char error[256];
  assert(bk_virtual_pointer_step(&pointer, &viewport, &input, .1, error));
  assert(pointer.position[0] == 640 && pointer.position[1] == 480);
  /* Both a motion event and a held mouse sample use this same transform. */
  input.pointer_x = 500 * sx;
  input.pointer_y = 300 * sy;
  assert(bk_virtual_pointer_step(&pointer, &viewport, &input, .1, error));
  assert(pointer.position[0] == 1000 && pointer.position[1] == 600);
  assert(pointer.motion[0] == 360 && pointer.motion[1] == 120);

  /* Re-query after resize; rectangular windows may have nonuniform scale. */
  logical_w = 1000;
  logical_h = 600;
  bk_sdl_pointer_scale(window, 1600, 1200, &sx, &sy);
  assert(sx == 1.6f && sy == 2);
  assert(bk_camera_fit(&viewport, 1600, 1200, 4, 3));
  input.pointer_x = 500 * sx;
  input.pointer_y = 300 * sy;
  assert(bk_virtual_pointer_step(&pointer, &viewport, &input, .1, error));
  assert(pointer.position[0] == 800 && pointer.position[1] == 600);
  bk_sdl_pointer_scale(window, 1000, 600, &sx, &sy);
  assert(sx == 1 && sy == 1);
  assert(bk_camera_fit(&viewport, 1000, 600, 4, 3));
  assert(viewport.x == 100);
  input.pointer_x = 500 * sx;
  input.pointer_y = 300 * sy;
  assert(bk_virtual_pointer_step(&pointer, &viewport, &input, .1, error));
  assert(pointer.position[0] == 400 && pointer.position[1] == 300);
  window_available = 0;
  bk_sdl_pointer_scale(window, 1600, 1200, &sx, &sy);
  assert(sx == 1 && sy == 1);
  window_available = 1;
  logical_w = 0;
  bk_sdl_pointer_scale(window, 1600, 1200, &sx, &sy);
  assert(sx == 1 && sy == 1);
  bk_sdl_pointer_scale(NULL, 1600, 1200, &sx, &sy);
  assert(sx == 1 && sy == 1);
}

static void axis_test(void) {
  int fake_pad;
  SDL_Gamepad *pad = (SDL_Gamepad *)&fake_pad;
  BkViewport viewport = {0, 0, 640, 480};
  for (int raw = -32768; raw <= 32767; ++raw) {
    raw_axis = (Sint16)raw;
    float value = bk_sdl_axis_value(pad, SDL_GAMEPAD_AXIS_LEFTX);
    /* libnx and SDL supply the same signed 16-bit range before consumers
     * apply their pointer, movement or camera calibration. */
    assert(value == (float)raw / 32768.0f);
  }
  const Sint16 samples[] = {0, 3276, 5898, 6554, -6554, 32767, -32768};
  char error[256];
  for (unsigned i = 0; i < sizeof(samples) / sizeof(*samples); ++i) {
    raw_axis = samples[i];
    BkVirtualPointer desktop = {.position = {320, 240}}, console = desktop;
    BkInput input = {.move_x = bk_sdl_axis_value(pad, SDL_GAMEPAD_AXIS_LEFTX)};
    assert(bk_virtual_pointer_step(&desktop, &viewport, &input, .1, error));
    input.move_x = (float)raw_axis / 32768.0f;
    assert(bk_virtual_pointer_step(&console, &viewport, &input, .1, error));
    assert(!memcmp(&desktop, &console, sizeof(desktop)));
    if (abs(samples[i]) == 6554)
      assert(desktop.motion[0] != 0);
    if (abs(samples[i]) <= 5898)
      assert(desktop.motion[0] == 0);
  }
}

int main(int argc, char **argv) {
  if (argc < 2 || !strcmp(argv[1], "pointer"))
    pointer_test();
  if (argc < 2 || !strcmp(argv[1], "axis"))
    axis_test();
  puts("PASS SDL input: HiDPI/resize/fallback and shared raw-axis contract");
  return 0;
}
