#include "scene/ending_auxiliary.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void word(uint8_t *p, uint32_t v) {
  for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8 * i));
}
static void number(uint8_t *p, float v) {
  uint32_t bits;
  memcpy(&bits, &v, 4);
  word(p, bits);
}
int main(void) {
  uint8_t xan[0x5190] = {0};
  strcpy((char *)xan, "fixture.x");
  strcpy((char *)xan + 256, "fixture.x");
  for (unsigned i = 0; i < 2; ++i) {
    uint8_t *p = xan + 0x390 + i * 0x9c;
    word(p + 0x50, 10);
    number(p + 0x54, 10.f + 20.f * i);
    number(p + 0x58, 20.f + 20.f * i);
  }
  char e[256] = {0};
  BkClipSet *clips = bk_clip_set_decode(xan, sizeof(xan), e);
  assert(clips);
  BkModelFrame root = {.parent_index = BK_MODEL_NONE};
  for (unsigned i = 0; i < 4; ++i) root.local[i * 5] = 1;
  BkModel model = {.frames = &root, .frame_count = 1};
  BkActorPose *actor = bk_actor_pose_create_loaded(&model, clips, 0,
      (float[3]){0}, 0, e);
  BkActorPose *sibling = bk_actor_pose_create_loaded(&model, clips, 0,
      (float[3]){0}, 0, e);
  assert(actor && sibling);
  BkClipEdit source = {.slot = 0, .fields = BK_CLIP_EDIT_SOURCE, .source = 18};
  assert(bk_actor_pose_edit_clips(actor, &source, 1, e));
  BkClipState before, after, other;
  assert(bk_actor_pose_state(actor, &before));
  assert(bk_actor_pose_state(sibling, &other));
  float world[16], local[16];
  memcpy(world, bk_actor_pose_frame(actor, 0), sizeof(world));
  memcpy(local, bk_actor_pose_local(actor, 0), sizeof(local));
  assert(bk_ending_clip_write(actor, 0, BK_ENDING_CLIP_NEXT, 1, e));
  assert(bk_ending_clip_write(actor, 0, BK_ENDING_CLIP_CHAIN, 1, e));
  assert(bk_ending_clip_write(actor, 0, BK_ENDING_CLIP_REWIND, -99, e));
  assert(bk_actor_pose_state(actor, &after) && after.source == 10);
  before.source = 10;
  assert(!memcmp(&before, &after, sizeof(before)));
  assert(!memcmp(world, bk_actor_pose_frame(actor, 0), sizeof(world)));
  assert(!memcmp(local, bk_actor_pose_local(actor, 0), sizeof(local)));
  assert(bk_clip_definition(clips, 0)->chain == 0 &&
         bk_clip_definition(clips, 0)->next == 0);
  assert(bk_actor_pose_state(sibling, &after) && !memcmp(&other, &after, sizeof(other)));
  assert(!bk_ending_clip_write(actor, 0, BK_ENDING_CLIP_CHAIN, 2, e));
  assert(!bk_ending_clip_write(actor, 0, BK_ENDING_CLIP_NEXT, 128, e));
  assert(!bk_ending_clip_write(actor, 128, BK_ENDING_CLIP_REWIND, 0, e));
  assert(!bk_ending_clip_write(actor, 0, (BkEndingClipWrite)99, 0, e) && *e);
  assert(!bk_ending_clip_write(NULL, 0, BK_ENDING_CLIP_CHAIN, 0, e));
  assert(bk_actor_pose_state(actor, &after) && !memcmp(&before, &after, sizeof(before)));
  /* Empty descriptors above the selected controller's 32-slot limit remain
   * valid for the general UI; these edits still do not select that slot. */
  assert(bk_ending_clip_write(actor, 127, BK_ENDING_CLIP_REWIND, 0, e));
  assert(bk_actor_pose_state(actor, &after) && !memcmp(&before, &after, sizeof(before)));
  /* The edits take effect only when the existing scheduler later advances. */
  for (unsigned i = 0; i < 3; ++i) {
    assert(bk_actor_pose_advance(actor, -1, 1, e));
    assert(bk_actor_pose_advance(sibling, -1, 1, e));
  }
  assert(bk_actor_pose_state(actor, &after) && after.slot == 1);
  assert(bk_actor_pose_state(sibling, &after) && after.slot == 0);
  bk_actor_pose_destroy(actor);
  bk_actor_pose_destroy(sibling);
  bk_clip_set_destroy(clips);
  puts("PASS ending descriptor writes preserve instance isolation and deferred scheduling");
  return 0;
}
