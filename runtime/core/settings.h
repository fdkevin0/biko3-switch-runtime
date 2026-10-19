#ifndef BK_SETTINGS_H
#define BK_SETTINGS_H
#include <stddef.h>

#define BK_SETTINGS_PATH_MAX 1024

/* Small user-facing settings file. It lives beside saves/photos, not inside
 * the read-only game data directory. Values are deliberately simple so the
 * same parser works on macOS, Linux and Switch. */
typedef struct {
  int windowed;           /* 1 = 1280x720-style window, 0 = fullscreen */
  int width, height;      /* logical window size in points */
  int show_help;          /* optional control-help side panels */
  int show_fps;           /* original FPS overlay */
  int show_dev_overlay;   /* diagnostic banner at the bottom */
  int invert_look;        /* invert camera Y input */
  char game_path[BK_SETTINGS_PATH_MAX]; /* remembered game root, empty = default */
} BkSettings;

void bk_settings_defaults(BkSettings *settings);
/* Returns 1 on success. A missing file is success with defaults. */
int bk_settings_load(BkSettings *settings, const char *root, char error[256]);
/* Atomically replaces <root>/settings.cfg. */
int bk_settings_save(const BkSettings *settings, const char *root,
                     char error[256]);

#endif
