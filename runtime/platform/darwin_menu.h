#ifndef BK_PLATFORM_DARWIN_MENU_H
#define BK_PLATFORM_DARWIN_MENU_H
/* Native macOS menu bar for runtime UI toggles. Implemented in
 * darwin_menu.m; only built into the APPLE native-window backend. */
typedef void BkDarwinMenu;

typedef enum {
  BK_MENU_TOGGLE_FPS = 1,
  BK_MENU_TOGGLE_HELP,
  BK_MENU_TOGGLE_INVERT_LOOK,
  BK_MENU_TOGGLE_FULLSCREEN,
  BK_MENU_FPS_CAP_30,
  BK_MENU_FPS_CAP_60,
  BK_MENU_FPS_CAP_120,
  BK_MENU_FPS_CAP_DISPLAY,
} BkMenuAction;

typedef void (*BkDarwinMenuCallback)(void *context, BkMenuAction action);

BkDarwinMenu *bk_darwin_menu_create(void *context,
                                    BkDarwinMenuCallback callback);
/* Update checkmarks without invoking the callback. */
void bk_darwin_menu_sync(BkDarwinMenu *menu, int show_fps, int show_help,
                         int invert_look, int fullscreen, int fps_cap);
void bk_darwin_menu_destroy(BkDarwinMenu *menu);
#endif
