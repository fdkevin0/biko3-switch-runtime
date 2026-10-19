# macOS 菜单栏开关

## 实现

新增原生 Cocoa 菜单栏 `显示`，在 SDL3 窗口创建后安装：

| 菜单项 | 快捷键 | 行为 |
| --- | --- | --- |
| 显示 FPS | `Cmd+1` | 切换原版 FPS 覆盖层 |
| 操作说明 | `Cmd+2` | 切换左侧操作说明 |
| 反转视角 | `Cmd+3` | 反转鼠标/右摇杆上下视角 |
| 全屏 | `Cmd+Ctrl+F` | 运行时全屏/窗口切换 |
| 帧率上限 | — | 30 / 60 / 120 / 跟随显示器 |

菜单勾选状态来自平台设置；通过 `F1`/`F2` 或 `F11` 改变时，应用会同步菜单
勾选。菜单修改立即写入 `~/Library/Application Support/Biko3/settings.cfg`，
并作用于当前运行，不仅是下次启动。

## 文件

- `runtime/platform/darwin_menu.m`：Cocoa `NSMenu`/`NSMenuItem` 实现；
- `runtime/platform/darwin_menu.h`：无 Cocoa 头依赖的 C 接口；
- `runtime/platform/darwin.c`：菜单生命周期、动作、设置持久化；
- `runtime/app/application.c`：每帧读取菜单 UI 状态并应用到 FPS/帮助/帧率上限；
- `runtime/platform/platform.h`：`BkPlatformUiState` 和同步函数；
- `CMakeLists.txt`：APPLE 下启用 Objective-C、链接 Cocoa、仅对 `.m` 开启 ARC。

## 设计边界

- 菜单由平台层持有，应用只镜像 `show_fps`、`show_help`、`fps_cap`；
- `invert_look` 和全屏由平台自身立即应用；
- 没有把“开发者 overlay”放进菜单，因为 scene 创建时复制了该开关，运行时切换
  需要额外重建 scene；后续若需要可加 scene 级 setter；
- `NSMenu` 只替换 macOS 主菜单，游戏仍使用 SDL3 事件循环和现有 F1/F2/F11。

## 验证

- Linux 主机 `biko3-preview` 构建通过；
- `application.c`、`darwin.c` 以 macOS 目标头文件 `-Wall -Wextra -Werror` 语法通过；
- `darwin_menu.m` 使用最小 Cocoa 声明桩以 `clang -fobjc-arc` 语法通过；
- 未在真实 macOS AppKit 会话中点击菜单，Objective-C/Cocoa 链接和菜单行为仍需
  实机验证。
