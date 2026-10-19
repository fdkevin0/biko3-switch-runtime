#include "scene/save_menu_labels.h"
#include "scene/save_menu_view.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
  unsigned texts, motions, group;
  int fail;
} Calls;
static int text(void *p, unsigned group, float dt, char e[256]) {
  (void)dt;
  (void)e;
  Calls *c = p;
  ++c->texts;
  c->group = group;
  return !c->fail;
}
static int motion(void *p, float out[2], char e[256]) {
  (void)e;
  ++((Calls *)p)->motions;
  out[0] = out[1] = 0;
  return 1;
}
int main(void) {
  char e[256];
  unsigned frames = 0;
  BkSaveMenuRecord rows[5][10] = {0};
  for (unsigned g = 0; g < 5; ++g)
    for (unsigned k = 0; k < 10; ++k) {
      rows[g][k].area = k % 9;
      rows[g][k].occupied = k % 2;
      for (unsigned i = 0; i < 5; ++i)
        rows[g][k].inventory[i] = (uint8_t)((g + k + i) % 3);
    }
  for (unsigned mode = 0; mode < 2; ++mode)
    for (unsigned group = 0; group < 5; ++group) {
      BkSaveMenuView view = {.detail_slot = 9};
      BkSaveMenuControl control = {.tab = (int)group + 3};
      assert(bk_save_menu_view_initialize(&view, mode, group, 640));
      assert(view.detail_slot == 9);
      BkCommonHudState common = {.curtain = {0, 2, 0}};
      BkMenuCursor cursor = {0};
      assert(bk_menu_cursor_initialize(&cursor, 640, 480));
      BkPauseBindings bindings = {.common = &common, .cursor = &cursor};
      BkPauseInput input = {.scale = .5f, .seconds = .25f};
      Calls calls = {0};
      BkSaveMenuViewOps ops = {&calls, text, motion};
      BkSaveMenuFrame frame;
      for (unsigned k = 0; k < 10; ++k)
        for (unsigned page = 0; page < 2; ++page) {
          control.hover = 21 + (int)k;
          control.page = (int)page;
          assert(bk_save_menu_view_step(&view, &control, &bindings, &input,
                                        mode, rows, &ops, &frame, e));
          assert(frame.count && frame.count <= BK_SAVE_MENU_DRAWS &&
                 frame.text_after < frame.count);
          assert(frame.draws[frame.count - 1].slot == BK_SAVE_MENU_CURTAIN &&
                 calls.group == group);
          for (unsigned i = 0; i < frame.count; ++i) {
            BkPauseDraw *d = &frame.draws[i];
            assert(d->slot < 58 && isfinite(d->alpha) && d->alpha >= 0 &&
                   d->alpha <= 1);
          }
          ++frames;
        }
      BkSaveMenuView before = view;
      BkSaveMenuControl old = control;
      unsigned texts = calls.texts;
      input.seconds = NAN;
      assert(!bk_save_menu_view_step(&view, &control, &bindings, &input, mode,
                                     rows, &ops, &frame, e));
      assert(!memcmp(&view, &before, sizeof(view)) &&
             !memcmp(&control, &old, sizeof(control)) && calls.texts == texts);
      input.seconds = .25f;
      control.page = 0;
      control.hover = 21;
      rows[group][0].area = 9;
      assert(!bk_save_menu_view_step(&view, &control, &bindings, &input, mode,
                                     rows, &ops, &frame, e));
      assert(!memcmp(&view, &before, sizeof(view)) && calls.texts == texts);
      rows[group][0].area = 0;
      calls.fail = 1;
      assert(!bk_save_menu_view_step(&view, &control, &bindings, &input, mode,
                                     rows, &ops, &frame, e));
      assert(calls.texts == texts + 1);
      /* Native released snapshot returns before sprite/record accesses. */
      view.loaded = 0;
      control.skip_draw = 1;
      view.sprites[0].fade.alpha = NAN;
      unsigned motions = calls.motions;
      assert(bk_save_menu_view_step(&view, &control, &bindings, &input, mode,
                                    rows, &ops, &frame, e));
      assert(!control.skip_draw && !frame.count &&
             frame.text_after == UINT32_MAX && calls.motions == motions);
    }
  const char *expected[] = {
      /* ０１－エリア１－２６／０９／２８－１２：３４ (Shift-JIS) */
      "\x82\x4f\x82\x50\x81\x7c\x83\x47\x83\x8a\x83\x41\x82\x50\x81\x7c"
      "\x82\x51\x82\x55\x81\x5e\x82\x4f\x82\x58\x81\x5e\x82\x51\x82\x57"
      "\x81\x7c\x82\x50\x82\x51\x81\x46\x82\x52\x82\x53",
      /* ０１－场景号１－２６／０９／２８－１２：３４ (GBK) */
      "\xa3\xb0\xa3\xb1\xa3\xad\xb3\xa1\xbe\xb0\xba\xc5\xa3\xb1\xa3\xad"
      "\xa3\xb2\xa3\xb6\xa3\xaf\xa3\xb0\xa3\xb9\xa3\xaf\xa3\xb2\xa3\xb8"
      "\xa3\xad\xa3\xb1\xa3\xb2\xa3\xba\xa3\xb3\xa3\xb4"};
  for (int chinese = 0; chinese <= 1; ++chinese) {
    BkSaveMenuLabel labels[10] = {0};
    uint8_t out[512], before[512];
    size_t size = 0;
    assert(bk_save_menu_labels(labels, chinese, out, &size, e) &&
           size == 440 && !out[size]);
    assert(!memcmp(out, expected[chinese], 6));
    for (unsigned k = 6; k < 44; k += 2)
      assert(!memcmp(out + k, expected[chinese] + 4, 2));
    for (unsigned i = 0; i < 10; ++i) {
      labels[i].area = i % 9;
      strcpy(labels[i].stamp, "2026/09/28-12:34:56");
    }
    assert(bk_save_menu_labels(labels, chinese, out, &size, e) &&
           size == 440 && !out[size]);
    assert(!memcmp(out, expected[chinese], 44));
    assert(!memcmp(out + 9 * 44, expected[chinese] + 2, 2) &&
           !memcmp(out + 9 * 44 + 2, expected[chinese], 2));
    memcpy(before, out, sizeof(out));
    labels[0].area = UINT32_MAX;
    assert(!bk_save_menu_labels(labels, chinese, out, &size, e) &&
           size == 440 && !memcmp(before, out, sizeof(out)));
    labels[0].stamp[0] = 0;
    assert(bk_save_menu_labels(labels, chinese, out, &size, e));
  }
  assert(!bk_save_menu_image(58, 0, 0) && !bk_save_menu_image(0, 5, 0));
  printf("PASS save-menu-view %u snapshots; invalid indices/time/callback and "
         "released skip; Japanese/Chinese labels, bounds/atomicity\n",
         frames);
  return 0;
}
