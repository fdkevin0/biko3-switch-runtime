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
static bool keys[SDL_SCANCODE_COUNT];
static SDL_MouseButtonFlags mouse_buttons;
static float mouse_x, mouse_y;
static unsigned enumerated, closed;
static struct {
  Sint16 axes[SDL_GAMEPAD_AXIS_COUNT];
  bool buttons[SDL_GAMEPAD_BUTTON_COUNT];
} pads[5];

SDL_JoystickID *SDLCALL SDL_GetGamepads(int *count) {
  *count = (int)enumerated;
  SDL_JoystickID *ids = SDL_malloc((enumerated + 1) * sizeof(*ids));
  assert(ids);
  for (unsigned i = 0; i < enumerated; ++i)
    ids[i] = i + 1;
  ids[enumerated] = 0;
  return ids;
}
bool SDLCALL SDL_IsGamepad(SDL_JoystickID id) { return id >= 1 && id <= 6; }
SDL_Gamepad *SDLCALL SDL_OpenGamepad(SDL_JoystickID id) {
  return id >= 1 && id <= 5 ? (SDL_Gamepad *)&pads[id - 1] : NULL;
}
void SDLCALL SDL_CloseGamepad(SDL_Gamepad *pad) { assert(pad); ++closed; }
const char *SDLCALL SDL_GetGamepadName(SDL_Gamepad *pad) {
  assert(pad);
  return "fixture";
}
bool SDLCALL SDL_GetGamepadButton(SDL_Gamepad *pad, SDL_GamepadButton button) {
  for (unsigned i = 0; i < 5; ++i)
    if (pad == (SDL_Gamepad *)&pads[i])
      return pads[i].buttons[button];
  abort();
}
const bool *SDLCALL SDL_GetKeyboardState(int *count) {
  *count = SDL_SCANCODE_COUNT;
  return keys;
}
SDL_MouseButtonFlags SDLCALL SDL_GetMouseState(float *x, float *y) {
  *x = mouse_x;
  *y = mouse_y;
  return mouse_buttons;
}
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
  for (unsigned i = 0; i < 5; ++i)
    if (pad == (SDL_Gamepad *)&pads[i])
      return pads[i].axes[axis];
  assert(pad && axis == SDL_GAMEPAD_AXIS_LEFTX);
  return raw_axis;
}

static void desktop_test(void) {
  BkSdlInput owner = {0};
  BkInput input;
  int fake_window;
  SDL_Window *window = (SDL_Window *)&fake_window;
  logical_w = 640;
  logical_h = 480;
  SDL_Event event = {.type = SDL_EVENT_GAMEPAD_ADDED};
  event.gdevice.which = 1;
  bk_sdl_input_event(&owner, window, 1280, 960, &event);
  bk_sdl_input_event(&owner, window, 1280, 960, &event);
  assert(owner.pad_count == 1);
  enumerated = 5;
  bk_sdl_input_open(&owner);
  assert(owner.pad_count == 4);
  event.type = SDL_EVENT_GAMEPAD_REMOVED;
  event.gdevice.which = 2;
  bk_sdl_input_event(&owner, window, 1280, 960, &event);
  assert(owner.pad_count == 3 && owner.pad_ids[1] == 3 && closed == 1);
  event.gdevice.which = 99;
  bk_sdl_input_event(&owner, window, 1280, 960, &event);
  assert(owner.pad_count == 3 && closed == 1);

  pads[0].buttons[SDL_GAMEPAD_BUTTON_SOUTH] = true;
  pads[0].axes[SDL_GAMEPAD_AXIS_LEFTX] = 16384;
  pads[0].axes[SDL_GAMEPAD_AXIS_RIGHTY] = -16384;
  pads[2].axes[SDL_GAMEPAD_AXIS_LEFTX] = -24576;
  pads[2].axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] = 16385;
  keys[SDL_SCANCODE_W] = true;
  bk_sdl_input_poll(&owner, window, 1280, 960, &input);
  assert(input.move_x == 0 && input.move_y == 1 && input.look_y == .5f);
  assert(input.pressed == (BK_BUTTON_CONFIRM | BK_BUTTON_INTERACT | BK_BUTTON_SLOW));
  bk_sdl_input_poll(&owner, window, 1280, 960, &input);
  assert(!input.pressed && !input.released);
  keys[SDL_SCANCODE_W] = false;
  pads[0].buttons[SDL_GAMEPAD_BUTTON_SOUTH] = false;
  pads[2].axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] = 16384;
  bk_sdl_input_poll(&owner, window, 1280, 960, &input);
  assert(input.move_x == -.75f && input.move_y == 0);
  assert(input.released == (BK_BUTTON_CONFIRM | BK_BUTTON_INTERACT | BK_BUTTON_SLOW));

  event.type = SDL_EVENT_MOUSE_MOTION;
  event.motion.x = 100;
  event.motion.y = 150;
  bk_sdl_input_event(&owner, window, 1280, 960, &event);
  bk_sdl_input_poll(&owner, window, 1280, 960, &input);
  assert(input.pointer_active && input.pointer_x == 200 && input.pointer_y == 300);
  bk_sdl_input_poll(&owner, window, 1280, 960, &input);
  assert(!input.pointer_active && input.pointer_x == 200);
  event.type = SDL_EVENT_FINGER_DOWN;
  event.tfinger.x = .75f;
  event.tfinger.y = .25f;
  bk_sdl_input_event(&owner, window, 1280, 960, &event);
  mouse_buttons = SDL_BUTTON_RMASK;
  mouse_x = 10;
  mouse_y = 20;
  bk_sdl_input_poll(&owner, window, 1280, 960, &input);
  assert(input.pointer_x == 960 && input.pointer_y == 240);
  assert(input.held == (BK_BUTTON_BACK | BK_BUTTON_STANCE));
  event.type = SDL_EVENT_FINGER_CANCELED;
  bk_sdl_input_event(&owner, window, 1280, 960, &event);
  bk_sdl_input_poll(&owner, window, 1280, 960, &input);
  assert(input.pointer_active && input.pointer_x == 20 && input.pointer_y == 40);
  mouse_buttons = 0;
  bk_sdl_input_close(&owner);
  assert(!owner.pad_count && closed == 4);
  enumerated = 0;
  event.type = SDL_EVENT_GAMEPAD_ADDED;
  event.gdevice.which = 6;
  bk_sdl_input_event(&owner, window, 1280, 960, &event);
  assert(!owner.pad_count);
  bk_sdl_input_close(&owner);
  assert(closed == 4);
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
  if (argc < 2 || !strcmp(argv[1], "desktop"))
    desktop_test();
  puts("PASS SDL input: HiDPI/axis, hotplug, keyboard/pad edges and touch/mouse priority");
  return 0;
}
