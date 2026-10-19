#include "game/flow_transition.h"
#include <stdio.h>
int bk_flow_is_front_end(uint8_t flow) {
  switch (flow) {
  case BK_FLOW_TITLE:
  case BK_FLOW_DIALOGUE:
  case BK_FLOW_GALLERY:
  case BK_FLOW_VOLUME:
  case BK_FLOW_SELECTION:
  case BK_FLOW_SPECIAL:
  case BK_FLOW_ALBUM:
    return 1;
  default:
    return 0;
  }
}
int bk_flow_transition_schedule(BkFlowTransition *s,
                                const BkFlowTransitionOps *o, uint8_t target,
                                uint8_t mode, char e[256]) {
  if (!s || !o || !o->release) {
    snprintf(e, 256, "flow transition: missing state/release service");
    return 0;
  }
  if (mode != 2 && !o->release(o->context, s->current, e))
    return 0;
  s->previous = s->current;
  s->target = target;
  s->current = BK_FLOW_LOADING;
  s->mode = mode;
  return 1;
}
