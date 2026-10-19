#ifndef BK_SCENE_ENDING_ACTOR_H
#define BK_SCENE_ENDING_ACTOR_H
#include "model/material_animation.h"
#include "model/morph_group.h"
#include "resource/store.h"
#include "world/actor_pose.h"

typedef struct {
  BkModel *model;
  BkClipSet *clips;
  BkActorPose *pose;
  BkMaterialPose *materials;
  BkMaterialAnimation *animation;
  BkMorphGroup *morph;
  uint32_t root;
} BkEndingActor;

/* Load into a fresh zero-initialized owner; destroy also releases a failed
 * load's acquired prefix. Borrowed copies must never be destroyed. No forest
 * attachment, clip request, world publication or shared-state reset. */
int bk_ending_actor_load(BkEndingActor *, BkResourceStore *, const char *pack,
                         const char *name, int effects, char error[256]);
void bk_ending_actor_destroy(BkEndingActor *);
/* Preserve the original pose -> morph blend -> material -> morph sample order. */
int bk_ending_actor_advance(const BkEndingActor *, float seconds, char error[256]);
int bk_ending_actor_advance_plain(const BkEndingActor *, float seconds,
                                 BkClipPlainMode, char error[256]);
#endif
