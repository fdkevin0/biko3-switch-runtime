#include "core/settings.h"
#include "core/file.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETTINGS_FILE "settings.cfg"
#define SETTINGS_MAX_LINE (BK_SETTINGS_PATH_MAX + sizeof("game_path=\r\n"))

static int clamp_int(int value, int low, int high) {
  return value < low ? low : value > high ? high : value;
}

static int parse_bool(const char *text, int *out) {
  if (!strcmp(text, "1") || !strcmp(text, "on") || !strcmp(text, "true")) {
    *out = 1;
    return 1;
  }
  if (!strcmp(text, "0") || !strcmp(text, "off") || !strcmp(text, "false")) {
    *out = 0;
    return 1;
  }
  return 0;
}

static int parse_int(const char *text, int *out) {
  char *end = NULL;
  errno = 0;
  long value = strtol(text, &end, 10);
  if (errno == ERANGE || !end || *end || end == text || value < INT_MIN ||
      value > INT_MAX)
    return 0;
  *out = (int)value;
  return 1;
}

static char *trim(char *text) {
  while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n')
    ++text;
  char *end = text + strlen(text);
  while (end > text && (end[-1] == ' ' || end[-1] == '\t' ||
                        end[-1] == '\r' || end[-1] == '\n'))
    *--end = 0;
  return text;
}

void bk_settings_defaults(BkSettings *settings) {
  if (!settings)
    return;
  *settings = (BkSettings){
      .windowed = 1,
      .width = 960,
      .height = 720,
      .show_help = 0,
      .show_fps = 0,
      .show_dev_overlay = 0,
      .fps_cap = 60,
      .invert_look = 0,
      .game_path = "",
  };
}

int bk_settings_load(BkSettings *settings, const char *root, char error[256]) {
  if (!settings || !root || !*root) {
    snprintf(error, 256, "settings: missing data root");
    return 0;
  }
  bk_settings_defaults(settings);
  char path[1024];
  if (snprintf(path, sizeof(path), "%s/%s", root, SETTINGS_FILE) >=
      (int)sizeof(path)) {
    snprintf(error, 256, "settings: path too long");
    return 0;
  }
  if (!bk_file_recover(path, error))
    return 0;
  FILE *stream = fopen(path, "rb");
  if (!stream) {
    if (errno == ENOENT)
      return 1;
    snprintf(error, 256, "settings: cannot open %.200s: %s", path,
             strerror(errno));
    return 0;
  }
  char line[SETTINGS_MAX_LINE];
  unsigned line_number = 0;
  while (fgets(line, sizeof(line), stream)) {
    ++line_number;
    if (!strchr(line, '\n') && !feof(stream)) {
      int next = fgetc(stream);
      if (next != EOF) {
        snprintf(error, 256, "settings: line %u too long", line_number);
        fclose(stream);
        return 0;
      }
    }
    char *text = trim(line);
    if (!*text || *text == '#' || *text == ';')
      continue;
    char *equals = strchr(text, '=');
    if (!equals) {
      snprintf(error, 256, "settings: malformed line %u", line_number);
      fclose(stream);
      return 0;
    }
    *equals = 0;
    char *key = trim(text), *value = trim(equals + 1);
    if (!strcmp(key, "windowed")) {
      (void)parse_bool(value, &settings->windowed);
    } else if (!strcmp(key, "width")) {
      if (parse_int(value, &settings->width))
        settings->width = clamp_int(settings->width, 640, 3840);
    } else if (!strcmp(key, "height")) {
      if (parse_int(value, &settings->height))
        settings->height = clamp_int(settings->height, 480, 2160);
    } else if (!strcmp(key, "show_help")) {
      (void)parse_bool(value, &settings->show_help);
    } else if (!strcmp(key, "show_fps")) {
      (void)parse_bool(value, &settings->show_fps);
    } else if (!strcmp(key, "show_dev_overlay")) {
      (void)parse_bool(value, &settings->show_dev_overlay);
    } else if (!strcmp(key, "fps_cap")) {
      if (parse_int(value, &settings->fps_cap)) {
        if (settings->fps_cap != 0)
          settings->fps_cap = clamp_int(settings->fps_cap, 30, 240);
      }
    } else if (!strcmp(key, "invert_look")) {
      (void)parse_bool(value, &settings->invert_look);
    } else if (!strcmp(key, "game_path")) {
      if (strlen(value) < sizeof(settings->game_path))
        snprintf(settings->game_path, sizeof(settings->game_path), "%s",
                 value);
    }
    /* Unknown keys and invalid values are intentionally ignored so a newer
     * settings file can still be read by an older build. */
  }
  if (ferror(stream)) {
    snprintf(error, 256, "settings: read failed: %s", strerror(errno));
    fclose(stream);
    return 0;
  }
  fclose(stream);
  return 1;
}

int bk_settings_save(const BkSettings *settings, const char *root,
                     char error[256]) {
  if (!settings || !root || !*root) {
    snprintf(error, 256, "settings: missing data root");
    return 0;
  }
  char path[1024], temporary[1024];
  if (snprintf(path, sizeof(path), "%s/%s", root, SETTINGS_FILE) >=
          (int)sizeof(path) ||
      snprintf(temporary, sizeof(temporary), "%s.tmp", path) >=
          (int)sizeof(temporary)) {
    snprintf(error, 256, "settings: path too long");
    return 0;
  }
  FILE *stream = fopen(temporary, "wb");
  if (!stream) {
    snprintf(error, 256, "settings: cannot create %.200s: %s", temporary,
             strerror(errno));
    return 0;
  }
  int ok = fprintf(stream,
                   "# Biko3 runtime settings; edited by the runtime and by hand.\n"
                   "windowed=%d\n"
                   "width=%d\n"
                   "height=%d\n"
                   "show_help=%d\n"
                   "show_fps=%d\n"
                   "show_dev_overlay=%d\n"
                   "fps_cap=%d\n"
                   "invert_look=%d\n"
                   "game_path=%s\n",
                   settings->windowed ? 1 : 0, settings->width,
                   settings->height, settings->show_help ? 1 : 0,
                   settings->show_fps ? 1 : 0,
                   settings->show_dev_overlay ? 1 : 0, settings->fps_cap,
                   settings->invert_look ? 1 : 0, settings->game_path) >= 0;
  if (fclose(stream) != 0)
    ok = 0;
  if (!ok) {
    snprintf(error, 256, "settings: write failed for %.200s", temporary);
    remove(temporary);
    return 0;
  }
  if (!bk_file_replace(temporary, path, error)) {
    remove(temporary);
    return 0;
  }
  return 1;
}
