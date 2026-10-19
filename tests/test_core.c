#include "core/clock.h"
#include "core/game_clock.h"
#include "core/input.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(expr)                                                            \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr);               \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)
int main(void) {
  BkClock clock;
  bk_clock_init(&clock);
  CHECK(bk_clock_advance(&clock, 100).steps == 0);
  CHECK(bk_clock_advance(&clock, 100 + 1.0 / 120).steps == 0);
  CHECK(bk_clock_advance(&clock, 100 + 1.0 / 60).steps == 1);
  CHECK(clock.ticks == 1);
  CHECK(bk_clock_advance(&clock, NAN).steps == 0);
  CHECK(bk_clock_advance(&clock, 90).steps == 0);
  BkClockFrame frame = bk_clock_advance(&clock, 110);
  CHECK(frame.steps == 8 && frame.alpha >= 0 && frame.alpha < 1);
  CHECK(clock.dropped_seconds > 9);
  /* 120 render samples over one second produce 60 simulation steps. */
  bk_clock_init(&clock);
  for (unsigned i = 0; i <= 120; i++)
    bk_clock_advance(&clock, i / 120.0);
  CHECK(clock.ticks == 60);
  /* Regression: game733700 is a held TWO-frame measurement. Supplying one
   * ordinary elapsed frame here silently halves native gameplay speed. */
  const unsigned rates[] = {15, 20, 30, 60, 120, 240};
  for (unsigned r = 0; r < 6; ++r) {
    BkGameClock native = {0};
    double total = 0;
    for (unsigned i = 0; i < rates[r] * 12; ++i) {
      unsigned now = (unsigned)lround(i * 1000.0 / rates[r]);
      float old = native.seconds;
      int was_armed = native.step_gate.armed;
      bk_game_clock_poll(&native, now, now);
      if (!was_armed)
        CHECK(native.seconds == old);
      if (i >= rates[r])
        total += native.seconds;
    }
    CHECK(fabs(total / 11 - 2) < .0005);
    CHECK(native.fps >= rates[r] - 1 && native.fps <= rates[r] + 1);
  }
  BkGameClock native = {0};
  bk_game_clock_poll(&native, 0, 0);
  bk_game_clock_poll(&native, 500, 500);
  CHECK(native.seconds == .22f && native.clamped_seconds > .279);
  bk_game_clock_poll(&native, 516, 516);
  CHECK(native.seconds == .22f);
  bk_game_clock_poll(&native, 533, 533);
  CHECK(native.seconds == .033f);
  BkInput pending = {0};
  BkInput press = {.held = BK_BUTTON_CONFIRM, .pressed = BK_BUTTON_CONFIRM};
  BkInput held = {.held = BK_BUTTON_CONFIRM, .move_x = .5f};
  bk_input_latch(&pending, &press);
  bk_input_latch(&pending, &held);
  BkInput tick = bk_input_consume(&pending);
  CHECK(tick.pressed == BK_BUTTON_CONFIRM && tick.move_x == .5f);
  tick = bk_input_consume(&pending);
  CHECK(tick.pressed == 0 && tick.held == BK_BUTTON_CONFIRM);
  BkInput release = {.released = BK_BUTTON_CONFIRM};
  bk_input_latch(&pending, &release);
  tick = bk_input_consume(&pending);
  CHECK(tick.held == 0 && tick.released == BK_BUTTON_CONFIRM);
  puts("PASS: fixed clock, pause/backward time, bounded catch-up, input edge "
       "lifetime");
  return 0;
}
