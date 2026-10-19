#import <Cocoa/Cocoa.h>
#include "platform/darwin_menu.h"
#include <stddef.h>
#include <stdlib.h>

@interface BkMenuTarget : NSObject
@property(nonatomic, assign) void *context;
@property(nonatomic, assign) BkDarwinMenuCallback callback;
- (void)activate:(NSMenuItem *)sender;
@end

@implementation BkMenuTarget
- (void)activate:(NSMenuItem *)sender {
  if (self.callback)
    self.callback(self.context, (BkMenuAction)sender.tag);
}
@end

@interface BkDarwinMenuImpl : NSObject
@property(nonatomic, strong) BkMenuTarget *target;
@property(nonatomic, strong) NSMenu *main_menu;
@property(nonatomic, strong) NSMenuItem *fps_item;
@property(nonatomic, strong) NSMenuItem *help_item;
@property(nonatomic, strong) NSMenuItem *invert_item;
@property(nonatomic, strong) NSMenuItem *fullscreen_item;
@property(nonatomic, strong) NSArray<NSMenuItem *> *caps; /* 30,60,120,display */
@end

@implementation BkDarwinMenuImpl
@end

static NSMenuItem *add_item(NSMenu *menu, NSString *title, SEL action,
                            NSString *key, NSEventModifierFlags modifiers,
                            id target, NSInteger tag) {
  NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title
                                                action:action
                                         keyEquivalent:key];
  item.target = target;
  item.tag = tag;
  if (modifiers)
    item.keyEquivalentModifierMask = modifiers;
  [menu addItem:item];
  return item;
}

BkDarwinMenu *bk_darwin_menu_create(void *context,
                                    BkDarwinMenuCallback callback) {
#if !TARGET_OS_OSX
  (void)context;
  (void)callback;
  return NULL;
#else
  BkDarwinMenuImpl *menu = [[BkDarwinMenuImpl alloc] init];
  menu.target = [[BkMenuTarget alloc] init];
  menu.target.context = context;
  menu.target.callback = callback;

  NSMenu *main = [[NSMenu alloc] initWithTitle:@"Main Menu"];
  menu.main_menu = main;

  NSMenuItem *app_item = [[NSMenuItem alloc] initWithTitle:@""
                                                     action:NULL
                                              keyEquivalent:@""];
  [main addItem:app_item];
  NSMenu *app_menu = [[NSMenu alloc] initWithTitle:@"Biko3"];
  [app_menu addItemWithTitle:@"关于 Biko3"
                      action:@selector(orderFrontStandardAboutPanel:)
               keyEquivalent:@""];
  [app_menu addItem:[NSMenuItem separatorItem]];
  [app_menu addItemWithTitle:@"退出 Biko3"
                      action:@selector(terminate:)
               keyEquivalent:@"q"];
  app_item.submenu = app_menu;

  NSMenuItem *display_item = [[NSMenuItem alloc] initWithTitle:@"显示"
                                                         action:NULL
                                                  keyEquivalent:@""];
  [main addItem:display_item];
  NSMenu *display = [[NSMenu alloc] initWithTitle:@"显示"];
  display_item.submenu = display;
  menu.fps_item =
      add_item(display, @"显示 FPS", @selector(activate:), @"1",
               NSEventModifierFlagCommand, menu.target, BK_MENU_TOGGLE_FPS);
  menu.help_item =
      add_item(display, @"操作说明", @selector(activate:), @"2",
               NSEventModifierFlagCommand, menu.target, BK_MENU_TOGGLE_HELP);
  [display addItem:[NSMenuItem separatorItem]];
  menu.invert_item =
      add_item(display, @"反转视角", @selector(activate:), @"3",
               NSEventModifierFlagCommand, menu.target,
               BK_MENU_TOGGLE_INVERT_LOOK);
  menu.fullscreen_item =
      add_item(display, @"全屏", @selector(activate:), @"f",
               NSEventModifierFlagCommand | NSEventModifierFlagControl,
               menu.target, BK_MENU_TOGGLE_FULLSCREEN);
  [display addItem:[NSMenuItem separatorItem]];

  NSMenuItem *cap_item = [[NSMenuItem alloc] initWithTitle:@"帧率上限"
                                                     action:NULL
                                              keyEquivalent:@""];
  [display addItem:cap_item];
  NSMenu *caps = [[NSMenu alloc] initWithTitle:@"帧率上限"];
  cap_item.submenu = caps;
  menu.caps = @[
    add_item(caps, @"30 FPS", @selector(activate:), @"", 0, menu.target,
             BK_MENU_FPS_CAP_30),
    add_item(caps, @"60 FPS", @selector(activate:), @"", 0, menu.target,
             BK_MENU_FPS_CAP_60),
    add_item(caps, @"120 FPS", @selector(activate:), @"", 0, menu.target,
             BK_MENU_FPS_CAP_120),
    add_item(caps, @"跟随显示器", @selector(activate:), @"", 0, menu.target,
             BK_MENU_FPS_CAP_DISPLAY)
  ];

  [NSApp setMainMenu:main];
  return (__bridge_retained void *)menu;
#endif
}

void bk_darwin_menu_sync(BkDarwinMenu *handle, int show_fps, int show_help,
                         int invert_look, int fullscreen, int fps_cap) {
  BkDarwinMenuImpl *menu = (__bridge BkDarwinMenuImpl *)handle;
  if (!menu)
    return;
  menu.fps_item.state =
      show_fps ? NSControlStateValueOn : NSControlStateValueOff;
  menu.help_item.state =
      show_help ? NSControlStateValueOn : NSControlStateValueOff;
  menu.invert_item.state =
      invert_look ? NSControlStateValueOn : NSControlStateValueOff;
  menu.fullscreen_item.state =
      fullscreen ? NSControlStateValueOn : NSControlStateValueOff;
  unsigned index = fps_cap == 0    ? 3
                   : fps_cap >= 120 ? 2
                   : fps_cap >= 60  ? 1
                                    : 0;
  for (unsigned i = 0; i < menu.caps.count; ++i)
    menu.caps[i].state = i == index ? NSControlStateValueOn
                                    : NSControlStateValueOff;
}

void bk_darwin_menu_destroy(BkDarwinMenu *handle) {
  if (!handle)
    return;
  BkDarwinMenuImpl *menu = (__bridge_transfer BkDarwinMenuImpl *)handle;
  [NSApp setMainMenu:nil];
  menu.target = nil;
  menu.main_menu = nil;
}
