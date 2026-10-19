#include "platform/platform.h"
#include "platform/sdl_input.h"
#include "platform/darwin_menu.h"
#include "core/settings.h"
#include "render/window_surface.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_dialog.h>
#include <errno.h>
#include <fcntl.h>
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

/* Native macOS backend for the Biko3 runtime.
 *
 * The renderer was written for the Switch's NWindow plus libnx pad/touch.
 * This backend supplies the equivalent macOS pieces on top of SDL3:
 *   - a Cocoa window and Metal-backed VkSurfaceKHR (MoltenVK) factory,
 *   - keyboard/mouse/gamepad/touch translated to BkInput,
 *   - local wall and monotonic clocks,
 *   - a writable user data root under ~/Library/Application Support/Biko3
 *     for saves/photos/settings.
 * Audio lives in audio_sdl.c (CoreAudio through SDL3). It is optional: if SDL
 * cannot open a device the game still runs silently rather than refusing to
 * start.
 *
 * Kept free of Apple-only APIs where possible so it can be syntax-checked on
 * other hosts; CMake only compiles it on APPLE. */

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
  int focused;
  int window_override; /* BIKO3_WINDOWED/FULLSCREEN is for this launch only. */
  int resize_pending;
  unsigned pending_width, pending_height;
  unsigned debug_pointer_events;
  BkSettings settings;
  BkDarwinMenu *menu;
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
static void save_window_settings(BkPlatform *p);
static void toggle_fullscreen(BkPlatform *p);

static void menu_sync(BkPlatform *p) {
  if (!p || !p->menu)
    return;
  bk_darwin_menu_sync(p->menu, p->settings.show_fps, p->settings.show_help,
                      p->settings.invert_look, !p->settings.windowed,
                      p->settings.fps_cap);
}

static void save_menu_settings(BkPlatform *p) {
  if (!p || !p->root[0])
    return;
  BkSettings settings;
  char error[256] = {0};
  if (!bk_settings_load(&settings, p->root, error))
    bk_settings_defaults(&settings);
  settings.show_fps = p->settings.show_fps;
  settings.show_help = p->settings.show_help;
  settings.invert_look = p->settings.invert_look;
  settings.fps_cap = p->settings.fps_cap;
  if (!bk_settings_save(&settings, p->root, error))
    fprintf(stderr, "Settings: cannot save menu state: %s\n", error);
}

static void menu_action(void *context, BkMenuAction action) {
  BkPlatform *p = context;
  if (!p)
    return;
  switch (action) {
  case BK_MENU_TOGGLE_FPS:
    p->settings.show_fps = !p->settings.show_fps;
    break;
  case BK_MENU_TOGGLE_HELP:
    p->settings.show_help = !p->settings.show_help;
    break;
  case BK_MENU_TOGGLE_INVERT_LOOK:
    p->settings.invert_look = !p->settings.invert_look;
    break;
  case BK_MENU_TOGGLE_FULLSCREEN:
    toggle_fullscreen(p);
    menu_sync(p);
    return;
  case BK_MENU_FPS_CAP_30:
    p->settings.fps_cap = 30;
    break;
  case BK_MENU_FPS_CAP_60:
    p->settings.fps_cap = 60;
    break;
  case BK_MENU_FPS_CAP_120:
    p->settings.fps_cap = 120;
    break;
  case BK_MENU_FPS_CAP_DISPLAY:
    p->settings.fps_cap = 0;
    break;
  default:
    return;
  }
  save_menu_settings(p);
  menu_sync(p);
  fprintf(stderr, "Menu: fps=%d help=%d invert=%d cap=%d\n",
          p->settings.show_fps, p->settings.show_help,
          p->settings.invert_look, p->settings.fps_cap);
}

static void update_drawable(BkPlatform *p) {
  int width = 0, height = 0;
  if (!p->window ||
      !SDL_GetWindowSizeInPixels(p->window, &width, &height) ||
      width <= 0 || height <= 0)
    return;
  if ((unsigned)width != p->width || (unsigned)height != p->height) {
    p->width = (unsigned)width;
    p->height = (unsigned)height;
    p->surface.width = p->width;
    p->surface.height = p->height;
    p->pending_width = p->width;
    p->pending_height = p->height;
    p->resize_pending = 1;
  }
}

static void toggle_fullscreen(BkPlatform *p) {
  if (!p || !p->window)
    return;
  int fullscreen =
      (SDL_GetWindowFlags(p->window) & SDL_WINDOW_FULLSCREEN) != 0;
  if (fullscreen) {
    SDL_SetWindowFullscreen(p->window, false);
    SDL_SetWindowBordered(p->window, true);
    SDL_SetWindowSize(p->window, p->settings.width, p->settings.height);
    SDL_SetWindowPosition(p->window, SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);
    p->settings.windowed = 1;
  } else {
    int width = 0, height = 0;
    if (SDL_GetWindowSize(p->window, &width, &height) && width >= 640 &&
        height >= 480) {
      p->settings.width = width;
      p->settings.height = height;
    }
    SDL_SetWindowBordered(p->window, false);
    SDL_SetWindowFullscreen(p->window, true);
    p->settings.windowed = 0;
  }
  SDL_SyncWindow(p->window);
  /* The macOS fullscreen Space transition can complete after SyncWindow.
   * Wait until pixel size actually changes so the renderer/scene viewport
   * does not stay at the previous window extent (which presents in the
   * top-left of the fullscreen surface). */
  unsigned before_width = p->width, before_height = p->height;
  for (unsigned attempt = 0; attempt < 60; ++attempt) {
    update_drawable(p);
    if (p->width != before_width || p->height != before_height)
      break;
    SDL_PumpEvents();
    SDL_Delay(16);
  }
  static int last_width = -1, last_height = -1;
  if (last_width != (int)p->width || last_height != (int)p->height) {
    fprintf(stderr, "Display: fullscreen %s drawable=%ux%u\n",
            p->settings.windowed ? "off" : "on", p->width, p->height);
    last_width = (int)p->width;
    last_height = (int)p->height;
  }
  p->window_override = 0; /* An explicit F11 choice becomes the saved state. */
  save_window_settings(p);
  menu_sync(p);
}

static void poll_events(BkPlatform *p) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      p->quit = 1;
      break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
      p->focused = 1;
      break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
      p->focused = 0;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
    case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
    case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
      update_drawable(p);
      break;
    case SDL_EVENT_KEY_DOWN: {
      bool shortcut = event.key.key == SDLK_F11;
#ifdef SDL_KMOD_GUI
      if (event.key.key == SDLK_F &&
          (event.key.mod & SDL_KMOD_GUI) &&
          (event.key.mod & SDL_KMOD_CTRL))
        shortcut = true;
#endif
      if (shortcut)
        toggle_fullscreen(p);
      break;
    }
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
      if (getenv("BIKO3_INPUT_DEBUG") && p->debug_pointer_events < 12) {
        int logical_w = 0, logical_h = 0, drawable_w = 0, drawable_h = 0;
        SDL_GetWindowSize(p->window, &logical_w, &logical_h);
        SDL_GetWindowSizeInPixels(p->window, &drawable_w, &drawable_h);
        fprintf(stderr,
                "Pointer: event=(%.1f,%.1f) window=%dx%d drawable=%dx%d "
                "scale=%.2fx/%.2fx target=(%.1f,%.1f)\n",
                event.motion.x, event.motion.y, logical_w, logical_h,
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

static int directory_exists(const char *path) {
  struct stat info;
  return path && *path && stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

static int game_root_valid(const char *root) {
  char data[PATH_MAX];
  if (!root || !*root ||
      snprintf(data, sizeof(data), "%s/Data", root) >= (int)sizeof(data))
    return 0;
  return directory_exists(data);
}

typedef struct {
  int done;
  char path[PATH_MAX];
} BkFolderChoice;

static void folder_choice_callback(void *userdata, const char *const *files,
                                   int filter) {
  (void)filter;
  BkFolderChoice *choice = userdata;
  if (!choice)
    return;
  if (files && files[0] && *files[0])
    snprintf(choice->path, sizeof(choice->path), "%s", files[0]);
  choice->done = 1;
}

static int choose_game_folder(BkPlatform *p, char error[256]) {
  BkFolderChoice choice = {0};
  SDL_ShowOpenFolderDialog(folder_choice_callback, &choice, NULL,
                           p->game[0] ? p->game : NULL, false);
  for (unsigned waited = 0; !choice.done && waited < 120000; ++waited) {
    SDL_PumpEvents();
    SDL_Delay(16);
  }
  if (!choice.done || !choice.path[0]) {
    snprintf(error, 256,
             "game data directory not found; select the folder containing Data/");
    return 0;
  }
  char selected[PATH_MAX];
  snprintf(selected, sizeof(selected), "%s", choice.path);
  if (game_root_valid(selected)) {
    snprintf(p->game, sizeof(p->game), "%s", selected);
  } else {
    /* Some users select the Data folder itself; accept its parent. */
    char parent[PATH_MAX];
    snprintf(parent, sizeof(parent), "%s", selected);
    char *slash = strrchr(parent, '/');
    if (!slash || slash == parent) {
      snprintf(error, 256, "selected folder does not contain Data/: %.200s",
               selected);
      return 0;
    }
    *slash = 0;
    if (!game_root_valid(parent)) {
      snprintf(error, 256, "selected folder does not contain Data/: %.200s",
               selected);
      return 0;
    }
    snprintf(p->game, sizeof(p->game), "%s", parent);
  }
  snprintf(p->settings.game_path, sizeof(p->settings.game_path), "%s", p->game);
  if (!bk_settings_save(&p->settings, p->root, error)) {
    fprintf(stderr, "Settings: cannot remember game path: %s\n", error);
    error[0] = 0;
  }
  return 1;
}

/* MoltenVK ships an ICD manifest; make sure the Vulkan loader can find it
 * even when the app is launched from Finder with a minimal environment.
 * Homebrew installs it under one of these prefixes (Apple Silicon first). */
static void configure_vulkan_icd(void) {
  if (getenv("VK_ICD_FILENAMES") || getenv("VK_DRIVER_FILES"))
    return;
  const char *candidates[] = {
      "/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json",
      "/usr/local/etc/vulkan/icd.d/MoltenVK_icd.json",
      "/opt/homebrew/opt/molten-vk/etc/vulkan/icd.d/MoltenVK_icd.json",
      "/usr/local/opt/molten-vk/etc/vulkan/icd.d/MoltenVK_icd.json",
      "/opt/homebrew/share/vulkan/icd.d/MoltenVK_icd.json",
      "/usr/local/share/vulkan/icd.d/MoltenVK_icd.json",
  };
  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    if (access(candidates[i], R_OK) == 0) {
      setenv("VK_ICD_FILENAMES", candidates[i], 1);
      return;
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
  const char *data_dir = getenv("BIKO3_DATA_DIR");
  if (data_dir && *data_dir) {
    if (snprintf(p->root, sizeof(p->root), "%s", data_dir) >=
        (int)sizeof(p->root)) {
      snprintf(error, 256, "data directory path too long");
      goto failed;
    }
  } else if (snprintf(p->root, sizeof(p->root),
                      "%s/Library/Application Support/Biko3", home) >=
             (int)sizeof(p->root)) {
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

  char settings_error[256] = {0};
  if (!bk_settings_load(&p->settings, p->root, settings_error)) {
    bk_settings_defaults(&p->settings);
    fprintf(stderr, "Settings: %s; using defaults\n", settings_error);
  }
  const char *game = (argc > 1 && argv[1] && argv[1][0] != '-') ? argv[1]
                                                                 : getenv("BIKO3_GAME");
  int explicit_game = game && *game;
  if (!game || !*game) {
    if (p->settings.game_path[0])
      game = p->settings.game_path;
  }
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
  if (!game_root_valid(p->game)) {
    if (explicit_game) {
      snprintf(error, 256, "game data directory does not contain Data/: %.200s",
               p->game);
      goto failed;
    }
    if (!choose_game_folder(p, error))
      goto failed;
  }

  if (getenv("BIKO3_WINDOWED")) {
    p->settings.windowed = 1;
    p->window_override = 1;
  }
  if (getenv("BIKO3_FULLSCREEN")) {
    p->settings.windowed = 0;
    p->window_override = 1;
  }
  configure_vulkan_icd();
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

  SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "1");
  int windowed = p->settings.windowed;
  int width = p->settings.width, height = p->settings.height;
  Uint64 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  if (windowed)
    flags |= SDL_WINDOW_RESIZABLE;
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
  /* The original build uses ShowCursor(FALSE) and draws its own pointer for
   * menus/pause. Keep the macOS arrow from sitting beside that software cursor.
   * BIKO3_SHOW_SYSTEM_CURSOR=1 restores it for input debugging. */
  if (!getenv("BIKO3_SHOW_SYSTEM_CURSOR"))
    SDL_HideCursor();
  if (!windowed) {
    /* Cocoa may ignore the create-time fullscreen flag; enter the desktop's
     * fullscreen Space explicitly and hide the title bar. */
    SDL_SetWindowBordered(p->window, false);
    SDL_SetWindowFullscreen(p->window, true);
    SDL_SetWindowPosition(p->window, 0, 0);
    SDL_SyncWindow(p->window);
  } else {
    SDL_SetWindowMinimumSize(p->window, 640, 480);
    SDL_SetWindowPosition(p->window, SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);
  }
  p->focused = 1;
  if (!SDL_DisableScreenSaver())
    fprintf(stderr, "Display: screen-saver suppression unavailable: %s\n",
            SDL_GetError());
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
  {
    int logical_w = 0, logical_h = 0;
    if (SDL_GetWindowSize(p->window, &logical_w, &logical_h))
      fprintf(stderr,
              "Input: window %dx%d points, drawable %ux%u pixels, "
              "pointer scale %.2fx/%.2fx\n",
              logical_w, logical_h, p->width, p->height,
              logical_w > 0 ? (double)p->width / logical_w : 1.0,
              logical_h > 0 ? (double)p->height / logical_h : 1.0);
  }
  open_gamepads(p);
  p->audio_ready = SDL_InitSubSystem(SDL_INIT_AUDIO);
  if (!p->audio_ready)
    fprintf(stderr, "Audio: SDL audio unavailable (%s); running silent\n",
            SDL_GetError());

  *config = (BkLaunchConfig){
      p->game, NULL, "game", p->audio_ready, p->root,
      p->settings.show_fps || getenv("BK_SHOW_FPS") != NULL};
  fprintf(stderr,
          "Display: %ux%u fullscreen-flag=%d; windowed=%d; settings: %s; "
          "game data: %s\n",
          p->width, p->height,
          (SDL_GetWindowFlags(p->window) & SDL_WINDOW_FULLSCREEN) != 0,
          windowed, settings_error[0] ? "defaults" : "settings.cfg", p->game);
  p->menu = bk_darwin_menu_create(p, menu_action);
  if (!p->menu)
    fprintf(stderr, "Menu: native menu bar unavailable\n");
  else
    menu_sync(p);
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
  if (keys[SDL_SCANCODE_F2])
    result |= BK_BUTTON_HELP;
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
  if (!p->focused) {
    p->previous_held = 0;
    *input = (BkInput){0};
    return 1;
  }
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
    if (p->settings.invert_look)
      ry = -ry;
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

int bk_platform_focused(const BkPlatform *p) {
  return p && p->focused;
}

void bk_platform_idle(BkPlatform *p, unsigned milliseconds) {
  (void)p;
  SDL_Delay(milliseconds);
}

int bk_platform_take_resize(BkPlatform *p, unsigned *width, unsigned *height) {
  if (!p || !width || !height)
    return 0;
  /* Fullscreen/DPI transitions can finish after the SDL event that triggered
   * them. Re-query the drawable on every frame so a delayed extent cannot
   * leave the game presented at the old window size in the top-left. */
  update_drawable(p);
  if (!p->resize_pending)
    return 0;
  *width = p->pending_width;
  *height = p->pending_height;
  p->resize_pending = 0;
  return 1;
}

void bk_platform_ui_state(const BkPlatform *p, BkPlatformUiState *out) {
  if (!out)
    return;
  *out = (BkPlatformUiState){0};
  if (!p)
    return;
  out->show_fps = p->settings.show_fps;
  out->show_help = p->settings.show_help;
  out->invert_look = p->settings.invert_look;
  out->fullscreen = !p->settings.windowed;
  out->fps_cap = p->settings.fps_cap;
}

void bk_platform_set_ui_state(BkPlatform *p, int show_fps, int show_help,
                              int fps_cap) {
  if (!p)
    return;
  p->settings.show_fps = show_fps != 0;
  p->settings.show_help = show_help != 0;
  if (fps_cap >= 30 || fps_cap == 0)
    p->settings.fps_cap = fps_cap;
  menu_sync(p);
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
  /* The save layer already replaced the destination atomically. Flush the
   * file's data before reporting success, matching the Switch backend's
   * explicit commit contract. A missing file is not a hard error here because
   * callers may commit volume/photo files that were removed just before. */
  if (!path || !*path) {
    snprintf(error, 256, "platform: empty save path");
    return 0;
  }
  int fd = open(path, O_RDWR);
  if (fd < 0)
    fd = open(path, O_RDONLY);
  if (fd < 0) {
    if (errno == ENOENT)
      return 1;
    snprintf(error, 256, "platform: cannot open %.200s", path);
    return 0;
  }
#ifdef F_FULLFSYNC
  int ok = fcntl(fd, F_FULLFSYNC) == 0;
  if (!ok)
    ok = fsync(fd) == 0;
#else
  int ok = fsync(fd) == 0;
#endif
  if (close(fd))
    ok = 0;
  if (!ok)
    snprintf(error, 256, "platform: cannot flush %.200s", path);
  return ok;
}

void bk_platform_report_error(BkPlatform *p, const char *error) {
  fprintf(stderr, "FAILED: %s\n", error);
  fflush(stderr);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "尾行3 / Biko3", error,
                           p ? p->window : NULL);
}

static void save_window_settings(BkPlatform *p) {
  if (!p || !p->window || !p->root[0] || p->window_override)
    return;
  BkSettings settings;
  char error[256] = {0};
  if (!bk_settings_load(&settings, p->root, error))
    bk_settings_defaults(&settings);
  settings.windowed = p->settings.windowed;
  if (p->window && settings.windowed) {
    int width = 0, height = 0;
    if (SDL_GetWindowSize(p->window, &width, &height) &&
        width >= 640 && height >= 480) {
      settings.width = width;
      settings.height = height;
    }
  }
  if (!bk_settings_save(&settings, p->root, error))
    fprintf(stderr, "Settings: cannot save window state: %s\n", error);
}

void bk_platform_close(BkPlatform *p) {
  if (!p)
    return;
  save_window_settings(p);
  bk_darwin_menu_destroy(p->menu);
  p->menu = NULL;
  SDL_EnableScreenSaver();
  close_gamepads(p);
  if (p->window)
    SDL_DestroyWindow(p->window);
  SDL_Quit();
  free(p);
}
