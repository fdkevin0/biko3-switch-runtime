#ifndef BK_SCENE_ENDING_BACKGROUND_ASSETS_H
#define BK_SCENE_ENDING_BACKGROUND_ASSETS_H
#include "scene/ending_actor.h"

typedef struct BkEndingBackgroundAssets BkEndingBackgroundAssets;
/* Borrowed view; the enclosing background owner retains and releases it. */
typedef BkEndingActor BkEndingBackgroundData;

/* Decode an actual bk3_03 actor, without global attachment, orientation or
 * clip selection. Those operations belong to the original loader boundary. */
BkEndingBackgroundAssets *bk_ending_background_assets_create(
    BkResourceStore *, const char *name, char error[256]);
BkEndingBackgroundAssets *bk_ending_background_assets_retain(
    BkEndingBackgroundAssets *);
void bk_ending_background_assets_destroy(BkEndingBackgroundAssets *);
const BkEndingBackgroundData *bk_ending_background_assets_data(
    const BkEndingBackgroundAssets *);
const char *bk_ending_background_assets_name(const BkEndingBackgroundAssets *);
/*4026fe ordering, including hidden-clock retention and disjoint model
 * effects. No world publication, audio command, or material copy. */
int bk_ending_background_assets_advance(BkEndingBackgroundAssets *, float seconds,
                                        char error[256]);
#endif
