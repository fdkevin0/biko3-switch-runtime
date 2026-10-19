#include "scene/ending_actor.h"
#include <stdio.h>

static int fail(char e[256], const char *why) {
  snprintf(e, 256, "ending actor: %s", why);
  return 0;
}
void bk_ending_actor_destroy(BkEndingActor *p) {
  bk_morph_group_destroy(p->morph);
  bk_material_animation_destroy(p->animation);
  bk_material_pose_destroy(p->materials);
  bk_actor_pose_destroy(p->pose);
  bk_clip_set_destroy(p->clips);
  bk_model_destroy(p->model);
}
int bk_ending_actor_load(BkEndingActor *p, BkResourceStore *store,
                         const char *pack, const char *name,
                       int effects, char error[256]) {
  BkBlob blob = {0};
  int ok = 0;
  if (bk_resources_read(store, pack, name, &blob, error) != BK_RESOURCE_OK)
    goto done;
  p->clips = bk_clip_set_decode(blob.data, blob.size, error);
  bk_blob_free(&blob);
  if (!p->clips || bk_resources_read(store, pack, bk_clip_model_name(p->clips),
      &blob, error) != BK_RESOURCE_OK ||
      bk_model_decode(blob.data, blob.size, &p->model, error) != BK_MODEL_OK)
    goto done;
  if (bk_model_chunk(p->model, "LIGA") || bk_model_chunk(p->model, "TEXA")) {
    fail(error, "animated resource effect needs its own owner");
    goto done;
  }
  if (effects) {
    p->materials = bk_material_pose_create(p->model, error);
    if (!p->materials || (bk_model_chunk(p->model, "MATA") &&
        !(p->animation = bk_material_animation_create(p->model, error))) ||
        (bk_model_chunk(p->model, "MORP") &&
        !(p->morph = bk_morph_group_create(p->model, error))))
      goto done;
  }
  p->root = BK_MODEL_NONE;
  for (uint32_t i = 0; i < p->model->frame_count; ++i)
    if (p->model->frames[i].parent_index == BK_MODEL_NONE) {
      if (p->root != BK_MODEL_NONE) {
        fail(error, "multiple actor roots");
        goto done;
      }
      p->root = i;
    }
  if (p->root == BK_MODEL_NONE) {
    fail(error, "missing actor root");
    goto done;
  }
  p->pose = bk_actor_pose_create_loaded(p->model, p->clips, p->root,
                                         (float[3]){0}, 0, error);
  if (!p->pose || !bk_actor_pose_root_local(p->pose, p->model->frames[p->root].local, error))
    goto done;
  ok = 1;
done:
  bk_blob_free(&blob);
  return ok;
}
int bk_ending_actor_advance(const BkEndingActor *p, float seconds, char e[256]) {
  BkPlaybackEffects effects;
  int submitted;
  if (!bk_actor_pose_advance_effects(p->pose, seconds, &effects, &submitted, e)) return 0;
  if (!submitted) return 1;
  if (effects.pose.blend && p->morph &&
      !bk_morph_group_blend(p->morph, effects.pose.from, effects.pose.to,
                              effects.pose.weight, NULL, 0, e)) return 0;
  if (p->animation && !bk_material_animation_sample(
          p->animation, effects.source, p->materials, e)) return 0;
  return effects.pose.blend || !p->morph ||
         bk_morph_group_sample(p->morph, effects.source, NULL, 0, e);
}
int bk_ending_actor_advance_plain(const BkEndingActor *p, float seconds,
                                   BkClipPlainMode mode, char e[256]) {
  BkPlaybackEffects effects;
  int submitted;
  if (!bk_actor_pose_advance_plain(p->pose, seconds, mode,
                                    &effects, &submitted, e)) return 0;
  if (!submitted) return 1;
  if (p->animation && !bk_material_animation_sample(
          p->animation, effects.source, p->materials, e)) return 0;
  return !p->morph || bk_morph_group_sample(p->morph, effects.source, NULL, 0, e);
}
