#include "platform/sdl_input.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void add_gamepad(BkSdlInput *p, SDL_JoystickID id) {
  if (!SDL_IsGamepad(id) || p->pad_count >= 4)
    return;
  for (unsigned i = 0; i < p->pad_count; ++i)
    if (p->pad_ids[i] == id)
      return;
  SDL_Gamepad *pad = SDL_OpenGamepad(id);
  if (!pad)
    return;
  p->pads[p->pad_count] = pad;
  p->pad_ids[p->pad_count] = id;
  ++p->pad_count;
  const char *name = SDL_GetGamepadName(pad);
  fprintf(stderr, "Gamepad %u: %s\n", p->pad_count, name ? name : "unnamed");
}

static void remove_gamepad(BkSdlInput *p, SDL_JoystickID id) {
  for (unsigned i = 0; i < p->pad_count; ++i) {
    if (p->pad_ids[i] != id)
      continue;
    SDL_CloseGamepad(p->pads[i]);
    for (unsigned j = i + 1; j < p->pad_count; ++j) {
      p->pads[j - 1] = p->pads[j];
      p->pad_ids[j - 1] = p->pad_ids[j];
    }
    --p->pad_count;
    fprintf(stderr, "Gamepad removed: %u remaining\n", p->pad_count);
    return;
  }
}

void bk_sdl_input_open(BkSdlInput *p) {
  int count = 0;
  SDL_JoystickID *ids = SDL_GetGamepads(&count);
  if (!ids)
    return;
  for (int i = 0; i < count; ++i)
    add_gamepad(p, ids[i]);
  SDL_free(ids);
}

void bk_sdl_input_close(BkSdlInput *p) {
  for (unsigned i = 0; i < p->pad_count; ++i)
    SDL_CloseGamepad(p->pads[i]);
  p->pad_count = 0;
}

static uint32_t gamepad_buttons(SDL_Gamepad *pad) {
  uint32_t result = 0;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_SOUTH))
    result |= BK_BUTTON_CONFIRM | BK_BUTTON_INTERACT;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_EAST))
    result |= BK_BUTTON_BACK | BK_BUTTON_STANCE;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_WEST))
    result |= BK_BUTTON_CAMERA_TRACK | BK_BUTTON_GAME_CAMERA;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_NORTH))
    result |= BK_BUTTON_PHOTO;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_START))
    result |= BK_BUTTON_PAUSE;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_BACK))
    result |= BK_BUTTON_FPS;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_UP))
    result |= BK_BUTTON_UP;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
    result |= BK_BUTTON_DOWN;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
    result |= BK_BUTTON_LEFT;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
    result |= BK_BUTTON_RIGHT;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))
    result |= BK_BUTTON_CAMERA_ORBIT;
  if (SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
    result |= BK_BUTTON_CAMERA_ADJUST;
  return result;
}
static uint32_t keyboard_buttons(const bool *keys, float *move_x,
                                 float *move_y) {
  uint32_t result = 0;
  if (keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_SPACE] ||
      keys[SDL_SCANCODE_KP_ENTER])
    result |= BK_BUTTON_CONFIRM | BK_BUTTON_INTERACT;
  if (keys[SDL_SCANCODE_ESCAPE] || keys[SDL_SCANCODE_BACKSPACE])
    result |= BK_BUTTON_BACK | BK_BUTTON_STANCE;
  if (keys[SDL_SCANCODE_TAB])
    result |= BK_BUTTON_PAUSE;
  if (keys[SDL_SCANCODE_UP])
    result |= BK_BUTTON_UP;
  if (keys[SDL_SCANCODE_DOWN])
    result |= BK_BUTTON_DOWN;
  if (keys[SDL_SCANCODE_LEFT])
    result |= BK_BUTTON_LEFT;
  if (keys[SDL_SCANCODE_RIGHT])
    result |= BK_BUTTON_RIGHT;
  if (keys[SDL_SCANCODE_X])
    result |= BK_BUTTON_CAMERA_TRACK | BK_BUTTON_GAME_CAMERA;
  if (keys[SDL_SCANCODE_C])
    result |= BK_BUTTON_PHOTO;
  if (keys[SDL_SCANCODE_Q])
    result |= BK_BUTTON_CAMERA_ORBIT;
  if (keys[SDL_SCANCODE_E])
    result |= BK_BUTTON_CAMERA_ADJUST;
  if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT])
    result |= BK_BUTTON_SLOW;
  if (keys[SDL_SCANCODE_F1])
    result |= BK_BUTTON_FPS;
#ifdef __APPLE__
  if (keys[SDL_SCANCODE_F2])
    result |= BK_BUTTON_HELP;
#endif
  float x = (keys[SDL_SCANCODE_D] ? 1.0f : 0.0f) -
            (keys[SDL_SCANCODE_A] ? 1.0f : 0.0f);
  float y = (keys[SDL_SCANCODE_W] ? 1.0f : 0.0f) -
            (keys[SDL_SCANCODE_S] ? 1.0f : 0.0f);
  *move_x = x;
  *move_y = y;
  return result;
}

void bk_sdl_input_event(BkSdlInput *p, SDL_Window *window, unsigned width,
                        unsigned height, const SDL_Event *event) {
  switch (event->type) {
  case SDL_EVENT_GAMEPAD_ADDED:
    add_gamepad(p, event->gdevice.which);
    break;
  case SDL_EVENT_GAMEPAD_REMOVED:
    remove_gamepad(p, event->gdevice.which);
    break;
  case SDL_EVENT_MOUSE_MOTION: {
    float sx, sy;
    bk_sdl_pointer_scale(window, width, height, &sx, &sy);
    p->mouse_x = event->motion.x * sx;
    p->mouse_y = event->motion.y * sy;
    p->mouse_moved = 1;
    if (getenv("BIKO3_INPUT_DEBUG") && p->debug_pointer_events < 12) {
      int logical_w = 0, logical_h = 0, drawable_w = 0, drawable_h = 0;
      SDL_GetWindowSize(window, &logical_w, &logical_h);
      SDL_GetWindowSizeInPixels(window, &drawable_w, &drawable_h);
      fprintf(stderr,
              "Pointer: event=(%.1f,%.1f) window=%dx%d drawable=%dx%d "
              "scale=%.2fx/%.2fx target=(%.1f,%.1f)\n",
              event->motion.x, event->motion.y, logical_w, logical_h,
              drawable_w, drawable_h, sx, sy, p->mouse_x, p->mouse_y);
      ++p->debug_pointer_events;
    }
    break;
  }
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    p->mouse_moved = 1;
    break;
  case SDL_EVENT_FINGER_DOWN:
  case SDL_EVENT_FINGER_MOTION:
    p->touch_active = 1;
    p->pointer_x = event->tfinger.x * (float)width;
    p->pointer_y = event->tfinger.y * (float)height;
    break;
  case SDL_EVENT_FINGER_UP:
  case SDL_EVENT_FINGER_CANCELED:
    p->touch_active = 0;
    break;
  default:
    break;
  }
}

void bk_sdl_input_poll(BkSdlInput *p, SDL_Window *window, unsigned width,
                       unsigned height, BkInput *input) {
  *input = (BkInput){0};
  if (!p->pad_count)
    bk_sdl_input_open(p);

  int keys_count = 0;
  const bool *keys = SDL_GetKeyboardState(&keys_count);
  (void)keys_count;
  float keyboard_x = 0, keyboard_y = 0;
  uint32_t held = keys ? keyboard_buttons(keys, &keyboard_x, &keyboard_y) : 0;
  float move_x = keyboard_x, move_y = keyboard_y;
  float look_x = 0, look_y = 0;

  for (unsigned i = 0; i < p->pad_count; ++i) {
    SDL_Gamepad *pad = p->pads[i];
    held |= gamepad_buttons(pad);
    float lx = bk_sdl_axis_value(pad, SDL_GAMEPAD_AXIS_LEFTX);
    float ly = -bk_sdl_axis_value(pad, SDL_GAMEPAD_AXIS_LEFTY);
    float rx = bk_sdl_axis_value(pad, SDL_GAMEPAD_AXIS_RIGHTX);
    float ry = -bk_sdl_axis_value(pad, SDL_GAMEPAD_AXIS_RIGHTY);
    if (fabsf(lx) + fabsf(ly) > fabsf(move_x) + fabsf(move_y)) {
      move_x = lx;
      move_y = ly;
    }
    if (fabsf(rx) + fabsf(ry) > fabsf(look_x) + fabsf(look_y)) {
      look_x = rx;
      look_y = ry;
    }
    if (SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 16384)
      held |= BK_BUTTON_SLOW;
  }

  float mx = 0, my = 0;
  SDL_MouseButtonFlags mouse = SDL_GetMouseState(&mx, &my);
  float sx, sy;
  bk_sdl_pointer_scale(window, width, height, &sx, &sy);
  if (mouse & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK)) {
    p->mouse_x = mx * sx;
    p->mouse_y = my * sy;
    p->mouse_moved = 1;
  }
  if (mouse & SDL_BUTTON_LMASK)
    held |= BK_BUTTON_CONFIRM | BK_BUTTON_INTERACT;
  if (mouse & SDL_BUTTON_RMASK)
    held |= BK_BUTTON_BACK | BK_BUTTON_STANCE;

  int pointer_active = 0;
  if (p->touch_active) {
    pointer_active = 1;
  } else if (p->mouse_moved) {
    pointer_active = 1;
    p->pointer_x = p->mouse_x;
    p->pointer_y = p->mouse_y;
    p->mouse_moved = 0;
  }

  input->held = held;
  input->pressed = held & ~p->previous_held;
  input->released = p->previous_held & ~held;
  p->previous_held = held;
  input->move_x = move_x;
  input->move_y = move_y;
  input->look_x = look_x;
  input->look_y = look_y;
  input->pointer_active = (uint8_t)pointer_active;
  input->pointer_x = p->pointer_x;
  input->pointer_y = p->pointer_y;
  if (input->pressed && getenv("BIKO3_INPUT_DEBUG"))
    fprintf(stderr, "Input: pressed=0x%08x held=0x%08x pointer=%.0f,%.0f active=%d\n",
            input->pressed, input->held, input->pointer_x, input->pointer_y,
            pointer_active);
}
