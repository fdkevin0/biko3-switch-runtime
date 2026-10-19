#include "scene/ending_background_assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct BkEndingBackgroundAssets {
  BkEndingBackgroundData data;
  uint32_t references;
  char name[64];
};
static int fail(char error[256], const char *why) {
  snprintf(error, 256, "ending background assets: %s", why);
  return 0;
}
BkEndingBackgroundAssets *bk_ending_background_assets_retain(
    BkEndingBackgroundAssets *a) {
  if (!a || !a->references || a->references == UINT32_MAX)
    return NULL;
  ++a->references;
  return a;
}
void bk_ending_background_assets_destroy(BkEndingBackgroundAssets *a) {
  if (!a || --a->references)
    return;
  bk_ending_actor_destroy(&a->data);
  free(a);
}
BkEndingBackgroundAssets *bk_ending_background_assets_create(
    BkResourceStore *store, const char *name, char error[256]) {
  if (!store || !name || !*name || strlen(name) >= 64) {
    fail(error, "invalid resource name");
    return NULL;
  }
  BkEndingBackgroundAssets *a = calloc(1, sizeof(*a));
  if (!a) {
    fail(error, "allocation failed");
    return NULL;
  }
  a->references = 1;
  memcpy(a->name, name, strlen(name) + 1);
  if (bk_ending_actor_load(&a->data, store, "bk3_03", name, 1, error))
    return a;
  bk_ending_background_assets_destroy(a);
  return NULL;
}
const BkEndingBackgroundData *bk_ending_background_assets_data(
    const BkEndingBackgroundAssets *a) {
  return a ? &a->data : NULL;
}
const char *bk_ending_background_assets_name(const BkEndingBackgroundAssets *a) {
  return a ? a->name : NULL;
}
int bk_ending_background_assets_advance(BkEndingBackgroundAssets *a,
                                        float seconds, char error[256]) {
  if (!a)
    return fail(error, "missing background owner");
  return bk_ending_actor_advance(&a->data, seconds, error);
}
