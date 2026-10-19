#include "core/settings.h"
#include "core/file.h"
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

static unsigned commits;
static int fail_commit;
static int commit(const char *path, char error[256]) {
  FILE *stream = fopen(path, "rb");
  CHECK(stream != NULL && fgetc(stream) != EOF);
  CHECK(fclose(stream) == 0);
  ++commits;
  if (fail_commit) {
    snprintf(error, 256, "injected settings commit failure");
    return 0;
  }
  return 1;
}

int main(void) {
  char root[] = "/tmp/biko3-settings-XXXXXX";
  CHECK(mkdtemp(root) != NULL);
  BkSettings settings;
  char error[256] = {0};

  bk_settings_defaults(&settings);
  CHECK(settings.windowed == 1 && settings.width == 960 &&
        settings.height == 720 && !settings.show_help &&
        !settings.show_dev_overlay && settings.fps_cap == 60 &&
        !settings.invert_look);

  CHECK(bk_settings_load(&settings, root, error));
  CHECK(settings.windowed == 1 && settings.width == 960);
  bk_file_set_commit(commit);

  settings.windowed = 0;
  settings.width = 1600;
  settings.height = 900;
  settings.show_help = 1;
  settings.show_fps = 1;
  settings.show_dev_overlay = 1;
  settings.fps_cap = 120;
  settings.invert_look = 1;
  snprintf(settings.game_path, sizeof(settings.game_path), "/tmp/example-game");
  CHECK(bk_settings_save(&settings, root, error));
  CHECK(commits == 1);

  BkSettings loaded;
  memset(&loaded, 0, sizeof(loaded));
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(!loaded.windowed && loaded.width == 1600 && loaded.height == 900);
  CHECK(loaded.show_help && loaded.show_fps && loaded.show_dev_overlay);
  CHECK(loaded.fps_cap == 120);
  CHECK(loaded.invert_look);
  CHECK(!strcmp(loaded.game_path, "/tmp/example-game"));

  /* Replacing an existing file also works on fsdev's no-overwrite rename. */
  settings.fps_cap = 90;
  CHECK(bk_settings_save(&settings, root, error));
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(loaded.fps_cap == 90);
  CHECK(commits == 2);

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

  /* Invalid integers must not wrap into valid settings (notably cap=0). */
  write_file(path,
             "fps_cap=4294967296\n"
             "width=4294967297\n"
             "height=-999999999999999999999999\n");
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(loaded.fps_cap == 60 && loaded.width == 960 && loaded.height == 720);

  /* Every path accepted by the public field can be saved and read back. */
  bk_settings_defaults(&settings);
  memset(settings.game_path, 'x', sizeof(settings.game_path) - 1);
  settings.game_path[sizeof(settings.game_path) - 1] = 0;
  CHECK(bk_settings_save(&settings, root, error));
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(!strcmp(loaded.game_path, settings.game_path));

  /* The same recovery/commit contract covers interrupted settings writes. */
  char backup[520];
  CHECK(snprintf(backup, sizeof(backup), "%s.bak", path) < (int)sizeof(backup));
  CHECK(rename(path, backup) == 0);
  unsigned previous_commits = commits;
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(!strcmp(loaded.game_path, settings.game_path));
  CHECK(commits == previous_commits + 1 && access(backup, F_OK) != 0);

  fail_commit = 1;
  settings.fps_cap = 45;
  CHECK(!bk_settings_save(&settings, root, error));
  CHECK(strstr(error, "settings commit failure") != NULL);
  fail_commit = 0;
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(loaded.fps_cap == 45);

  char oversized[2048];
  memset(oversized, 'x', sizeof(oversized) - 1);
  oversized[sizeof(oversized) - 1] = 0;
  write_file(path, oversized);
  CHECK(!bk_settings_load(&loaded, root, error));
  CHECK(strstr(error, "too long") != NULL);

  /* A malformed boolean stays on the default instead of breaking startup. */
  write_file(path, "show_help=maybe\n");
  CHECK(bk_settings_load(&loaded, root, error));
  CHECK(!loaded.show_help);

  /* Missing file returns defaults; path with no root fails. */
  char missing[512];
  CHECK(snprintf(missing, sizeof(missing), "%s/none", root) <
        (int)sizeof(missing));
  CHECK(bk_settings_load(&loaded, missing, error));
  CHECK(loaded.width == 960);
  CHECK(!bk_settings_load(&loaded, "", error));

  CHECK(remove(path) == 0);
  CHECK(rmdir(root) == 0);
  bk_file_set_commit(NULL);
  puts("settings-file ok");
  return 0;
}
