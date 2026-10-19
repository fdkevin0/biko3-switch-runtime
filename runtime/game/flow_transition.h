#ifndef BK_GAME_FLOW_TRANSITION_H
#define BK_GAME_FLOW_TRANSITION_H
#include <stdint.h>
/* Native flow bytes are part of the game protocol; names do not renumber them. */
typedef enum {
  BK_FLOW_TITLE = 1,
  BK_FLOW_GAME = 2,
  BK_FLOW_PAUSE = 4,
  BK_FLOW_DIALOGUE = 8,
  BK_FLOW_ENDING = 0x10,
  BK_FLOW_GALLERY = 0x18,
  BK_FLOW_CHECKPOINT = 0x20,
  BK_FLOW_SAVE = 0x28,
  BK_FLOW_VOLUME = 0x30,
  BK_FLOW_SELECTION = 0x38,
  BK_FLOW_FAILURE = 0x40,
  BK_FLOW_SPECIAL = 0x48,
  BK_FLOW_LOADING = 0x50,
  BK_FLOW_EXIT = 0x58,
  BK_FLOW_ALBUM = 0x60,
  BK_FLOW_RETRY = 0x68
} BkFlowId;
int bk_flow_is_front_end(uint8_t flow);
typedef struct {
  uint8_t current, previous, target, mode;
} BkFlowTransition;
typedef struct {
  void *context;
  int (*release)(void *, uint8_t flow, char error[256]);
} BkFlowTransitionOps;
/*51c47e schedules flow50. Unless mode==2, releases current scene first.
 * No loader execution or scene existence is fabricated. Failure preserves
 * these flow bytes, though release may have partial ownership effects. */
int bk_flow_transition_schedule(BkFlowTransition *, const BkFlowTransitionOps *,
                                uint8_t target, uint8_t mode, char error[256]);
#endif
