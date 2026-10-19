#include "core/settings.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHECK(expr)                                                            \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr);               \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

static void write_file(const char *path, const char *text) {
  FILE *stream = fopen(path, "wb");
  CHECK(stream != NULL);
  CHECK(fputs(text, stream) >= 0);
  CHECK(fclose(stream) == 0);
}

int main(void) {
  char root[] = "/tmp/biko3-settings-XXXXXX";
  CHECK(mkdtemp(root) != NULL);
  BkSettings settings;
  char error[256] = {0};

  bk_settings_defaults(&settings);
  CHECK(settings.windowed == 1 && settings.width == 1280 &&
        settings.height == 720 && !settings.show_help &&
        !settings.show_dev_overlay && !settings.invert_look);

  CHECK(bk_settings_load(&settings, root, error));
  CHECK(settings.windowed == 1 && settings.width == 1280);

  settings.windowed = 0;
  settings.width = 1600;
  settings.height = 900;
  settings.show_help = 1;
  settings.show_fps = 1;
  settings.show_dev_overlay = 1;
  settings.invert_look = 1;
  snprintf(settings.game_path, sizeof(settings.game_path), "/tmp/example-game");
  CHECK(bk_settings_save(&settings, root, error));

  BkSettings loaded;
  memset(&loaded, 0, sizeof(loaded));
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(!loaded.windowed && loaded.width == 1600 && loaded.height == 900);
  CHECK(loaded.show_help && loaded.show_fps && loaded.show_dev_overlay);
  CHECK(loaded.invert_look);
  CHECK(!strcmp(loaded.game_path, "/tmp/example-game"));

  /* Unknown keys and out-of-range values are handled deterministically. */
  char path[512];
  CHECK(snprintf(path, sizeof(path), "%s/settings.cfg", root) <
        (int)sizeof(path));
  write_file(path,
             "windowed=2\n"
             "width=99999\n"
             "height=1\n"
             "show_help=yes\n"
             "future_key=whatever\n"
             "invert_look=false\n"
             "game_path=/tmp/another-game\n");
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(loaded.width == 3840 && loaded.height == 480);
  CHECK(!loaded.invert_look);
  CHECK(!strcmp(loaded.game_path, "/tmp/another-game"));
  CHECK(!loaded.show_help);

  /* A malformed boolean stays on the default instead of breaking startup. */
  write_file(path, "show_help=maybe\n");
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(!loaded.show_help);

  /* Missing file returns defaults; path with no root fails. */
  char missing[512];
  CHECK(snprintf(missing, sizeof(missing), "%s/none", root) <
        (int)sizeof(missing));
  CHECK(bk_settings_load(&loaded, missing, error));
  CHECK(loaded.width == 1280);
  CHECK(!bk_settings_load(&loaded, "", error));

  CHECK(remove(path) == 0);
  CHECK(rmdir(root) == 0);
  puts("settings-file ok");
  return 0;
}
