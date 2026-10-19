#include "core/settings.h"
#include "core/file.h"
#include "platform/platform.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHECK(expr)                                                            \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr);                \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

static const char *selected_game;
static unsigned choices;
static unsigned commits;
static int commit(const char *path, char error[256]) {
  (void)path;
  (void)error;
  ++commits;
  return 1;
}
bool bk_test_sdl_init(SDL_InitFlags flags) {
  (void)flags;
  return true;
}
bool bk_test_load_vulkan(const char *path) {
  (void)path;
  SDL_SetError("startup settings probe stop");
  return false;
}
void bk_test_choose_folder(SDL_DialogFileCallback callback, void *userdata,
                           SDL_Window *window, const char *location,
                           bool allow_many) {
  (void)window;
  (void)location;
  (void)allow_many;
  ++choices;
  const char *files[] = {selected_game, NULL};
  callback(userdata, files, 0);
}

static void startup(int argc, char **argv, unsigned expected_choices) {
  BkLaunchConfig config = {0};
  char error[256] = {0};
  choices = 0;
  commits = 0;
  bk_file_set_commit(commit);
  CHECK(!bk_platform_open(argc, argv, &config, error));
  bk_file_set_commit(NULL);
  CHECK(strstr(error, "startup settings probe stop") != NULL);
  CHECK(choices == expected_choices);
  /* A failed startup can remember a chosen directory, not window geometry. */
  CHECK(commits == expected_choices);
}

int main(void) {
  char root[] = "/tmp/bk-platform-settings-XXXXXX";
  CHECK(mkdtemp(root));
  char game[1024], data[1040], path[1040], error[256];
  CHECK(snprintf(game, sizeof(game), "%s/external-game", root) < (int)sizeof(game));
  CHECK(snprintf(data, sizeof(data), "%s/Data", game) < (int)sizeof(data));
  CHECK(!mkdir(game, 0700) && !mkdir(data, 0700));
  selected_game = game;
  CHECK(!setenv("BIKO3_DATA_DIR", root, 1));
  CHECK(!unsetenv("BIKO3_GAME") && !unsetenv("BIKO3_WINDOWED") &&
        !unsetenv("BIKO3_FULLSCREEN"));
  BkSettings settings, loaded;
  bk_settings_defaults(&settings);
  settings.width = 1111;
  settings.height = 777;
  settings.show_help = 1;
  settings.invert_look = 1;
  settings.fps_cap = 120;
  snprintf(settings.game_path, sizeof(settings.game_path), "%s", game);
  CHECK(bk_settings_save(&settings, root, error));
  char *argv[] = {"settings-test", NULL};
  startup(1, argv, 0);

  /* A replacement game directory changes only the remembered path. */
  snprintf(settings.game_path, sizeof(settings.game_path), "/missing-game");
  CHECK(bk_settings_save(&settings, root, error));
  startup(1, argv, 1);
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(!strcmp(loaded.game_path, game));
  CHECK(loaded.width == 1111 && loaded.height == 777 && loaded.show_help &&
        loaded.invert_look && loaded.fps_cap == 120);

  snprintf(settings.game_path, sizeof(settings.game_path), "/missing-game");
  CHECK(bk_settings_save(&settings, root, error));
  char *explicit_argv[] = {"settings-test", game, NULL};
  startup(2, explicit_argv, 0);
  CHECK(!setenv("BIKO3_GAME", game, 1));
  startup(1, argv, 0);
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(!strcmp(loaded.game_path, "/missing-game"));

  CHECK(snprintf(path, sizeof(path), "%s/settings.cfg", root) < (int)sizeof(path));
  CHECK(!unlink(path));
  CHECK(!rmdir(data) && !rmdir(game));
  CHECK(snprintf(path, sizeof(path), "%s/captures", root) < (int)sizeof(path));
  CHECK(!rmdir(path));
  CHECK(snprintf(path, sizeof(path), "%s/save", root) < (int)sizeof(path));
  CHECK(!rmdir(path) && !rmdir(root));
  puts("PASS platform settings: remembered path, picker preserves preferences, explicit overrides");
}
