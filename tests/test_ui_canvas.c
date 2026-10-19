#ifdef NDEBUG
#undef NDEBUG
#endif
#include "ui/canvas.h"
#include <assert.h>
#include <math.h>
#include <string.h>
int main(void) {
  char error[256] = {0};
  const unsigned sizes[][2] = {{640,480},{1600,900},{1001,777},{800,800},{1280,720}};
  for (unsigned i = 0; i < sizeof(sizes)/sizeof(*sizes); ++i) {
    BkViewport display;
    assert(bk_camera_fit(&display,sizes[i][0],sizes[i][1],4,3));
    BkInput input = {.pointer_active=1,.pressed=BK_BUTTON_CONFIRM,
      .pointer_x=display.x+display.width*.75f,
      .pointer_y=display.y+display.height*.25f,
      .pointer_motion_x=display.width*.125f,
      .pointer_motion_y=-(float)display.height*.125f};
    BkInput mapped;
    assert(bk_ui_input(&mapped,&input,&display,error));
    assert(fabsf(mapped.pointer_x-960)<.001f);
    assert(fabsf(mapped.pointer_y-240)<.001f);
    assert(fabsf(mapped.pointer_motion_x-160)<.001f);
    assert(fabsf(mapped.pointer_motion_y+120)<.001f);
    assert(mapped.pressed==input.pressed && mapped.pointer_active);
    BkVirtualPointer cursor = {0};
    BkViewport logical = bk_ui_canvas();
    assert(bk_virtual_pointer_step(&cursor,&logical,&mapped,.016,error));
    assert(fabsf(cursor.position[0]-960)<.001f && fabsf(cursor.position[1]-240)<.001f);
  }
  BkInput untouched = {.pressed=7}, before=untouched;
  assert(!bk_ui_input(&untouched,&before,&(BkViewport){0},error));
  assert(!memcmp(&untouched,&before,sizeof(before)));
  assert(bk_ui_extent(1280,960) && !bk_ui_extent(0,960) && !bk_ui_extent(16385,960));
  return 0;
}
