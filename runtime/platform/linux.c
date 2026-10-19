#include "platform/platform.h"
#include "platform/sdl_input.h"
#include "render/window_surface.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* Native desktop backend for the Biko3 runtime.
 *
 * The renderer was written for the Switch's NWindow plus libnx pad/touch.
 * This backend supplies the equivalent desktop pieces:
 *   - an SDL3 Vulkan window and VkSurfaceKHR factory,
 *   - keyboard/mouse/gamepad/touch translated to BkInput,
 *   - local wall and monotonic clocks,
 *   - a writable user data root for saves/photos/settings.
 * Audio lives in audio_sdl.c. It is optional: if SDL cannot open a device the
 * game still runs silently rather than refusing to start. */

struct BkPlatform {
  SDL_Window *window;
  SDL_Gamepad *pads[4];
  SDL_JoystickID pad_ids[4];
  unsigned pad_count;
  BkWindowSurface surface;
  const char *extensions[8];
  unsigned width, height;
  uint32_t held, previous_held;
  float pointer_x, pointer_y;
  int mouse_moved;
  float mouse_x, mouse_y;
  int touch_active;
  int quit;
  int audio_ready;
  char root[PATH_MAX];
  char game[PATH_MAX];
};

static uint64_t runtime_origin_ms;
static int runtime_clock_started;

static int make_directory(const char *path, char error[256]) {
  if (!path || !*path) {
    snprintf(error, 256, "empty data directory");
    return 0;
  }
  char buffer[PATH_MAX];
  if (snprintf(buffer, sizeof(buffer), "%s", path) >= (int)sizeof(buffer)) {
    snprintf(error, 256, "data directory path too long");
    return 0;
  }
  for (char *p = buffer + 1; *p; ++p)
    if (*p == '/') {
      *p = 0;
      if (mkdir(buffer, 0777) && errno != EEXIST) {
        snprintf(error, 256, "cannot create %.200s", buffer);
        return 0;
      }
      *p = '/';
    }
  if (mkdir(buffer, 0777) && errno != EEXIST) {
    snprintf(error, 256, "cannot create %.200s", buffer);
    return 0;
  }
  return 1;
}

int bk_platform_runtime_clock(int timer, uint32_t *out, char error[256]) {
  if (!out || timer < 0 || timer > 2) {
    snprintf(error, 256, "platform: invalid runtime clock domain");
    return 0;
  }
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
    snprintf(error, 256, "platform: monotonic clock unavailable");
    return 0;
  }
  uint64_t now = (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
  if (!runtime_clock_started) {
    runtime_origin_ms = now;
    runtime_clock_started = 1;
  }
  *out = (uint32_t)(timer == 2 ? now - runtime_origin_ms : now);
  return 1;
}

static int create_surface(void *userdata, void *instance, void **surface,
                          char error[256]) {
  BkPlatform *p = userdata;
  VkSurfaceKHR native = NULL;
  if (!SDL_Vulkan_CreateSurface(p->window, (VkInstance)instance, NULL,
                                &native)) {
    snprintf(error, 256, "SDL_Vulkan_CreateSurface: %s", SDL_GetError());
    return 0;
  }
  *surface = (void *)native;
  return 1;
}

static void add_gamepad(BkPlatform *p, SDL_JoystickID id) {
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

static void remove_gamepad(BkPlatform *p, SDL_JoystickID id) {
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

static void open_gamepads(BkPlatform *p) {
  int count = 0;
  SDL_JoystickID *ids = SDL_GetGamepads(&count);
  if (!ids)
    return;
  for (int i = 0; i < count; ++i)
    add_gamepad(p, ids[i]);
  SDL_free(ids);
}

static void close_gamepads(BkPlatform *p) {
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
static void poll_events(BkPlatform *p) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      p->quit = 1;
      break;
    case SDL_EVENT_GAMEPAD_ADDED:
      add_gamepad(p, event.gdevice.which);
      break;
    case SDL_EVENT_GAMEPAD_REMOVED:
      remove_gamepad(p, event.gdevice.which);
      break;
    case SDL_EVENT_MOUSE_MOTION: {
      float sx, sy;
      bk_sdl_pointer_scale(p->window, p->width, p->height, &sx, &sy);
      p->mouse_x = event.motion.x * sx;
      p->mouse_y = event.motion.y * sy;
      p->mouse_moved = 1;
      break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      p->mouse_moved = 1;
      break;
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_MOTION:
      p->touch_active = 1;
      p->pointer_x = event.tfinger.x * (float)p->width;
      p->pointer_y = event.tfinger.y * (float)p->height;
      break;
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
      p->touch_active = 0;
      break;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      if (event.window.windowID == SDL_GetWindowID(p->window))
        p->quit = 1;
      break;
    default:
      break;
    }
  }
}

BkPlatform *bk_platform_open(int argc, char **argv, BkLaunchConfig *config,
                             char error[256]) {
  uint32_t ignored_clock;
  if (!bk_platform_runtime_clock(2, &ignored_clock, error))
    return NULL;
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    snprintf(error, 256, "SDL_Init(VIDEO|GAMEPAD): %s", SDL_GetError());
    return NULL;
  }
  BkPlatform *p = calloc(1, sizeof(*p));
  if (!p) {
    snprintf(error, 256, "platform allocation failed");
    SDL_Quit();
    return NULL;
  }
  const char *home = getenv("HOME");
  if (!home || !*home) {
    snprintf(error, 256, "HOME is not set");
    goto failed;
  }
  const char *xdg = getenv("XDG_DATA_HOME");
  if (xdg && *xdg) {
    if (snprintf(p->root, sizeof(p->root), "%s/biko3", xdg) >=
        (int)sizeof(p->root)) {
      snprintf(error, 256, "data directory path too long");
      goto failed;
    }
  } else if (snprintf(p->root, sizeof(p->root), "%s/.local/share/biko3",
                      home) >= (int)sizeof(p->root)) {
    snprintf(error, 256, "data directory path too long");
    goto failed;
  }
  if (!make_directory(p->root, error))
    goto failed;
  char capture[PATH_MAX];
  if (snprintf(capture, sizeof(capture), "%s/captures", p->root) >=
          (int)sizeof(capture) ||
      !make_directory(capture, error))
    goto failed;
  char save[PATH_MAX];
  if (snprintf(save, sizeof(save), "%s/save", p->root) >=
          (int)sizeof(save) ||
      !make_directory(save, error))
    goto failed;

  const char *game = argc > 1 ? argv[1] : getenv("BIKO3_GAME");
  if (!game || !*game) {
    if (snprintf(p->game, sizeof(p->game), "%s/game", p->root) >=
        (int)sizeof(p->game)) {
      snprintf(error, 256, "game directory path too long");
      goto failed;
    }
  } else if (snprintf(p->game, sizeof(p->game), "%s", game) >=
             (int)sizeof(p->game)) {
    snprintf(error, 256, "game directory path too long");
    goto failed;
  }
  struct stat game_stat;
  if (stat(p->game, &game_stat) || !S_ISDIR(game_stat.st_mode)) {
    snprintf(error, 256, "game data directory not found: %.200s", p->game);
    goto failed;
  }

  if (!SDL_Vulkan_LoadLibrary(NULL)) {
    snprintf(error, 256, "SDL_Vulkan_LoadLibrary: %s", SDL_GetError());
    goto failed;
  }
  Uint32 extension_count = 0;
  const char *const *names = SDL_Vulkan_GetInstanceExtensions(&extension_count);
  if (!names || !extension_count || extension_count > 5) {
    snprintf(error, 256, "SDL_Vulkan_GetInstanceExtensions failed");
    goto failed;
  }
  for (unsigned i = 0; i < extension_count; ++i)
    p->extensions[i] = names[i];
  p->surface.extensions = p->extensions;
  p->surface.extension_count = extension_count;
  p->surface.create = create_surface;
  p->surface.userdata = p;

  int windowed = getenv("BIKO3_WINDOWED") != NULL;
  int width = 1280, height = 720;
  Uint64 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  if (!windowed) {
    const SDL_DisplayMode *mode =
        SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    if (mode) {
      width = mode->w;
      height = mode->h;
    }
    flags |= SDL_WINDOW_FULLSCREEN;
  }
  p->window = SDL_CreateWindow("尾行3 / Biko3", width, height, flags);
  if (!p->window) {
    snprintf(error, 256, "SDL_CreateWindow: %s", SDL_GetError());
    goto failed;
  }
  if (!windowed) {
    /* The create flag alone is not reliable on every Wayland compositor:
     * mark the toplevel fullscreen explicitly and drop decorations. */
    SDL_SetWindowBordered(p->window, false);
    SDL_SetWindowFullscreen(p->window, true);
    SDL_SetWindowPosition(p->window, 0, 0);
    SDL_SyncWindow(p->window);
  }
  int pixel_w = 0, pixel_h = 0;
  if (!SDL_GetWindowSizeInPixels(p->window, &pixel_w, &pixel_h) ||
      pixel_w <= 0 || pixel_h <= 0) {
    snprintf(error, 256, "SDL_GetWindowSizeInPixels failed");
    goto failed;
  }
  p->width = (unsigned)pixel_w;
  p->height = (unsigned)pixel_h;
  p->surface.width = p->width;
  p->surface.height = p->height;
  open_gamepads(p);
  p->audio_ready = SDL_InitSubSystem(SDL_INIT_AUDIO);
  if (!p->audio_ready)
    fprintf(stderr, "Audio: SDL audio unavailable (%s); running silent\n",
            SDL_GetError());

  *config = (BkLaunchConfig){
      p->game, NULL, "game", p->audio_ready, p->root,
      getenv("BK_SHOW_FPS") != NULL};
  fprintf(stderr, "Display: %ux%u fullscreen-flag=%d; game data: %s\n",
          p->width, p->height,
          (SDL_GetWindowFlags(p->window) & SDL_WINDOW_FULLSCREEN) != 0,
          p->game);
  return p;
failed:
  bk_platform_close(p);
  return NULL;
}

const BkWindowSurface *bk_platform_window_surface(BkPlatform *p) {
  return p ? &p->surface : NULL;
}

FILE *bk_platform_log(BkPlatform *p) {
  (void)p;
  return stderr;
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
  float x = (keys[SDL_SCANCODE_D] ? 1.0f : 0.0f) -
            (keys[SDL_SCANCODE_A] ? 1.0f : 0.0f);
  float y = (keys[SDL_SCANCODE_W] ? 1.0f : 0.0f) -
            (keys[SDL_SCANCODE_S] ? 1.0f : 0.0f);
  *move_x = x;
  *move_y = y;
  return result;
}

int bk_platform_poll(BkPlatform *p, BkInput *input) {
  if (!p || !input)
    return 0;
  *input = (BkInput){0};
  poll_events(p);
  if (p->quit)
    return 0;
  if (!p->pad_count)
    open_gamepads(p);

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
  bk_sdl_pointer_scale(p->window, p->width, p->height, &sx, &sy);
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
  return 1;
}

double bk_platform_seconds(BkPlatform *p) {
  (void)p;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

double bk_platform_performance_seconds(BkPlatform *p) {
  return bk_platform_seconds(p);
}

int bk_platform_calendar_time(BkCalendarTime *out, char error[256]) {
  time_t now = time(NULL);
  struct tm local;
  struct timespec ticks;
  if (!out || now == (time_t)-1 || !localtime_r(&now, &local) ||
      clock_gettime(CLOCK_MONOTONIC, &ticks)) {
    snprintf(error, 256, "platform: capture clock unavailable");
    return 0;
  }
  *out = (BkCalendarTime){(unsigned)local.tm_year + 1900,
                          (unsigned)local.tm_mon + 1,
                          (unsigned)local.tm_mday,
                          (unsigned)local.tm_hour,
                          (unsigned)local.tm_min,
                          (unsigned)local.tm_sec,
                          (uint32_t)((uint64_t)ticks.tv_sec * 1000 +
                                     (uint64_t)ticks.tv_nsec / 1000000)};
  return 1;
}

int bk_platform_commit_save(const char *path, char error[256]) {
  (void)path;
  (void)error;
  return 1;
}

void bk_platform_report_error(BkPlatform *p, const char *error) {
  fprintf(stderr, "FAILED: %s\n", error);
  fflush(stderr);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "尾行3 / Biko3", error,
                           p ? p->window : NULL);
}

void bk_platform_close(BkPlatform *p) {
  if (!p)
    return;
  close_gamepads(p);
  if (p->window)
    SDL_DestroyWindow(p->window);
  SDL_Quit();
  free(p);
}
