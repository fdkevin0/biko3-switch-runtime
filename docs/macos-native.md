# macOS 原生运行

SDL3 提供窗口、键盘/鼠标/触屏/手柄和 CoreAudio 输出；Vulkan renderer 通过 MoltenVK 映射到 Metal。运行时直接读取自有游戏资源，无需 Wine 或虚拟机。当前以本机 macOS 和 Homebrew 环境维护。

## 构建与运行

```sh
brew install cmake sdl3 molten-vk vulkan-loader vulkan-headers glslang
./build-macos.sh
./build-macos/biko3
```

产物为 `build-macos/biko3`，CMake 目标名仍为 `biko3-preview`。构建参数、离屏工具和 app 打包统一见[构建说明](build.md)。

首次启动找不到默认游戏目录时，系统文件夹选择器可选择包含 `Data/` 的目录，也接受 `Data` 本身；路径保存到 `settings.cfg`。也可直接传入路径：

```sh
./build-macos/biko3 /path/to/MAINDIR
```

## 数据目录

默认游戏目录：

```text
~/Library/Application Support/Biko3/game/
├── Data/
└── patch.pp
```

设置和玩家输出位于 app 外：

```text
~/Library/Application Support/Biko3/
├── settings.cfg
├── save/
├── captures/
└── album/
```

`patch.pp` 可选，启用方法见[汉化与去码](patches.md)。`BIKO3_GAME` 覆盖游戏输入目录，`BIKO3_DATA_DIR` 覆盖可写输出根目录；它们是不同路径。

## 窗口与设置

默认 960×720 窗口，支持拖动缩放与运行时全屏。内容保持 4:3，宽屏两侧黑边是预期布局。窗口尺寸变化会重建 swapchain/帧缓冲，失去焦点时暂停模拟并降低轮询频率。

菜单与 HUD 使用固定逻辑画布，窗口变化只调整显示视口和输入映射，保留当前选择、动画及命中区域。暂停截图会按当前 drawable 更新缓冲。本机 Retina 窗口、真实资源流程与回归范围见[引擎边界修复验证](../reports/engine-boundaries-2026-10-19.json)；早期标题/人物选择页复现见[窗口缩放验证](../reports/macos-window-resize.json)。

设置文件 `~/Library/Application Support/Biko3/settings.cfg` 示例：

```ini
windowed=1
width=960
height=720
show_help=0
show_fps=0
show_dev_overlay=0
fps_cap=60
invert_look=0
game_path=/path/to/MAINDIR
```

`windowed=0` 为全屏；`show_help`、`show_fps` 和 `invert_look` 分别控制操作说明、原版 FPS 和反转视角。`fps_cap=0` 跟随显示器，其余可从菜单选择 30/60/120。关闭时保存窗口尺寸；菜单与 F1/F2 的设置会同步持久化。

### 菜单栏

| 「显示」菜单项 | 快捷键 | 行为 |
| --- | --- | --- |
| 显示 FPS | Cmd+1 / F1 | 切换原版 FPS 覆盖层 |
| 操作说明 | Cmd+2 / F2 | 切换左侧帮助 |
| 反转视角 | Cmd+3 | 反转右摇杆上下视角 |
| 全屏 | Cmd+Ctrl+F / F11 | 窗口与全屏切换 |
| 帧率上限 | 菜单选择 | 30 / 60 / 120 / 跟随显示器 |

游戏内操作见[操作说明](controls.md)。原生菜单勾选与快捷键状态同步；菜单实现和历史验证范围见[菜单报告](../reports/macos-menu.md)。

## 启动参数与环境变量

| 参数 / 变量 | 作用 |
| --- | --- |
| 第一个命令行参数、`BIKO3_GAME` | 游戏目录，包含 `Data/` |
| `BIKO3_DATA_DIR` | 可写用户数据根目录 |
| `BIKO3_WINDOWED=1` | 本次启动强制窗口模式 |
| `BIKO3_FULLSCREEN=1` | 本次启动强制全屏，同时指定时优先于窗口覆盖 |
| `BK_SHOW_FPS=1` | 启动显示 FPS |
| `BIKO3_SHOW_SYSTEM_CURSOR=1` | 保留系统箭头，便于排查游戏光标 |
| `BIKO3_INPUT_DEBUG=1` | 打印前几次鼠标事件的 points / drawable 尺寸及比例 |
| `VK_ICD_FILENAMES` | 指定 MoltenVK ICD 清单 |

窗口/全屏启动覆盖只对本次生效；永久选择通过菜单或设置文件保存。例如：

```sh
BIKO3_FULLSCREEN=1 ./build-macos/biko3
```

SDL3 鼠标事件使用窗口 points，平台输入使用 drawable 像素；2× Retina 的 960×720 窗口通常对应 1920×1440 drawable。平台按实际尺寸比例转换，scene 再映射到 4:3 内容对应的 UI 逻辑画布。排障见[常见问题](faq.md)。

## app 与验证范围

`./tools/package_macos_app.sh` 默认生成 `dist/Biko3.app`，重建会替换指定 app。游戏数据留在外部目录；本机构建依赖 Homebrew dylib。输出路径、图标与签名范围见[app 打包](build.md#macos-app)。

已在本机完成实际编译链接及 MoltenVK 中日存读档界面检查，修复汉化日期/区域乱码；普通与 ASan/UBSan 单测也通过，见[存档报告](../reports/save-session.md)。这些结果不覆盖完整剧情或所有原生菜单点击，平台整体验证范围见[状态页](roadmap.md)。
