# macOS 平台后端（SDL3 + MoltenVK）

日期：2026-10-19。范围：为现有运行时新增 macOS 桌面平台层与构建/打包入口。
Switch 分支、Linux 分支、渲染器、资源/模型/游戏逻辑未改动。

## 改动

| 文件 | 内容 |
| --- | --- |
| `runtime/platform/darwin.c` | 新增 macOS 平台后端：Cocoa/SDL3 窗口、Metal 后端的 `VkSurfaceKHR`（经 MoltenVK）、键鼠/手柄/触屏输入、单调时钟、`~/Library/Application Support/Biko3` 数据根、`fsync`/`F_FULLFSYNC` 存档提交、MoltenVK ICD 自动探测 |
| `CMakeLists.txt` | `BK_NATIVE_WINDOW` 在 `APPLE` 上选择 `darwin.c`；Vulkan 头文件支持系统前缀回退；macOS 可玩构建链接 Vulkan loader（经 MoltenVK ICD 分发），离屏 host 仍可直接链接 MoltenVK；MoltenVK 前缀支持 `brew --prefix` 自动探测 |
| `build-macos.sh` | Homebrew 依赖检查、构建 `build-macos/biko3`、打印运行/ICD 提示 |
| `tools/package_macos_app.sh` | 生成 `dist/Biko3.app`（Info.plist、可选 icns、第三方许可、ad-hoc 签名），数据不打包 |
| `docs/macos-native.md` | 依赖、构建、数据目录、运行、打包、MoltenVK 排障、限制 |
| `README.md` / `.gitignore` | macOS 入口与构建产物忽略 |

设计边界：

- 平台层只依赖 SDL3 + POSIX；不把 Vulkan/Metal 类型引入 `core/resource/model`。
- 数据根与游戏目录分离：游戏数据在 `~/Library/Application Support/Biko3/game`，
  存档/照片/音量在 `~/Library/Application Support/Biko3/{save,captures,album}`。
- `BIKO3_GAME`、`BIKO3_DATA_DIR`、`BIKO3_WINDOWED` 与既有主机变量保持一致。

## 已验证（Linux 主机，非 macOS）

macOS 的 Cocoa/Metal 路径还无法在本机验证，但平台代码与构建集成本身做了以下检查：

1. `darwin.c` 以 `-Wall -Wextra -Werror` 语法检查通过（Linux + SDL3 3.4.18）。
2. 用 `-DAPPLE=TRUE` 在 Linux 上模拟 CMake 的 `APPLE` 分支：配置、编译
   `runtime/platform/darwin.c` 并链接 `biko3-preview` 全部通过。
3. 用该二进制在 Xvfb + lavapipe（Mesa 26 软件 Vulkan）启动真实数据：
   - `Display: 1280x720 fullscreen-flag=0`
   - `Renderer ready: 1280x720, 3 images, explicit Vulkan graphics pipeline`
   - `Audio: SDL3 stream, 48000Hz PCM16 stereo`
   - `Original menu scene loaded`，SEC 持续运行至超时；
   - `BIKO3_DATA_DIR` 下自动创建 `save/`、`captures/`、`album/`。
4. `tools/package_macos_app.sh` 在 Linux 上走通打包：生成
   `Biko3.app/Contents/{MacOS/biko3,Info.plist,Resources/...}`，Info.plist
   版本/标识/最低系统版本解析正确；`codesign` 在 Linux 上按预期跳过。

这些证明平台层能编译、链接、初始化窗口/Vulkan/音频并加载真实数据；
**不能替代 macOS 真机的 Cocoa/Metal 验证**。

## 2026-10-19 指针修复（Mac 反馈）

Mac 真机运行后报告“鼠标位置与 macOS 不同步”。原因与修复：

- SDL3 的鼠标事件/`SDL_GetMouseState()` 返回窗口坐标（points），而游戏
  `BkInput.pointer_*`/菜单视口使用渲染器像素（`bk_camera_fit` 基于
  `bk_renderer_extent`）。原实现用 `SDL_GetWindowSizeInPixels()` 作为
  “窗口尺寸”去缩放，等价于 scale=1；在 2x Retina 上指针只能覆盖一半屏幕。
- `darwin.c` 新增 `pointer_scale()`：用 `SDL_GetWindowSize()`（points）和
  平台启动时的 drawable 像素尺寸（`p->width/height`，与 Vulkan surface
  extent 一致）计算 x/y 缩放；鼠标移动事件与 `SDL_GetMouseState()` 轮询
  共用该比例。
- 默认调用 `SDL_HideCursor()`，与 Windows 原版 `ShowCursor(FALSE)` 一致，
  避免 macOS 箭头和游戏自绘菜单光标同时出现；`BIKO3_SHOW_SYSTEM_CURSOR=1`
  可恢复系统箭头，`BIKO3_INPUT_DEBUG=1` 打印前 12 次鼠标事件的
  window/drawable/scale/target。
- 启动日志增加 `Input: window ... points, drawable ... pixels, pointer
  scale ...`，便于区分 Retina 缩放和 4:3 视口偏移。

验证：Linux 冒烟测试（Xvfb + lavapipe，scale=1）重新构建/启动/渲染通过；
本机无法模拟 macOS Retina，需 Mac 真机按 `docs/macos-native.md` 的
`BIKO3_WINDOWED=1` 与 `BIKO3_INPUT_DEBUG=1` 确认。Linux 后端也有相同的
point/pixel 假设（HiDPI Linux），本提交未改动它。

## 未验证 / 已知限制

- macOS 真机上的 MoltenVK 设备创建、swapchain、Retina、全屏 Space、
  CoreAudio 与手柄路径均未实测。
- `.app` 绝对路径依赖 Homebrew 的 SDL3/vulkan-loader/MoltenVK；分发给
  无 Homebrew 的机器需要捆绑 dylib/ICD 并签名、公证。
- 通用二进制需要与 MoltenVK 架构匹配。
- 英文 2004 数据在 Linux 上已通过资源/模型/AVI/完整会话回放；macOS
  上仍建议先抽查标题、第一关、存档、拍照。
- 游戏数据、补丁与字体素材不随源码/安装包分发。

## 下一步

1. 在 Apple Silicon 与 Intel Mac 上各跑一次 `./build-macos.sh`，记录
   `cmake` 与运行日志。
2. 若 `SDL_Vulkan_LoadLibrary` 失败，按 `docs/macos-native.md` 设置
   `VK_ICD_FILENAMES`，并确认 `vulkaninfo --summary` 使用 MoltenVK。
3. 真机通过后，再决定是否把 dylib/ICD 打包进 `.app`、做 Developer ID
   签名与 notarization。
