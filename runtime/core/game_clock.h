#ifndef BK_GAME_CLOCK_H
#define BK_GAME_CLOCK_H
#include "core/timer.h"
/* Original4adc16/4adc96: the1ms rearming gate samples every other frame at
 * ordinary frame rates. Unsampled frames REUSE the preceding game step value,
 * but a presentation-only frame must not advance the simulation again: at a
 * 144Hz desktop that would run the game at double speed. `sampled` reports
 * whether this poll produced a fresh step; the application steps only then. */
typedef struct {
  BkTimer step_gate, fps_gate;
  float last_sample_ms, seconds;
  uint32_t frames, fps;
  double clamped_seconds;
  uint8_t sampled;
} BkGameClock;
/* Zero-initialize once. Milliseconds share the process-relative wall epoch.
 * sample_ms is the second native GetTickCount read, used only when due. */
void bk_game_clock_poll(BkGameClock *, uint32_t now_ms, uint32_t sample_ms);
#endif
