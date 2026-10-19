# macOS 原生运行（SDL3 + MoltenVK）

同一套运行时代码新增了 macOS 桌面后端，直接读取 `Data/` 与可选的
`patch.pp`，不经过 Wine、Proton 或虚拟机。渲染仍由 runtime 自己的 Vulkan
渲染器完成，macOS 上通过 [MoltenVK](https://github.com/KhronosGroup/MoltenVK)
把 Vulkan 映射到 Metal；窗口、输入和音频使用 SDL3（CoreAudio）。

平台层实现：`runtime/platform/darwin.c`（窗口/输入/时钟/数据目录），
`runtime/platform/audio_sdl.c`（与 Linux 后端共用 SDL3 音频推流）。

> 状态说明：构建脚本、平台层和打包脚本已加入仓库；Linux 端已用真实游戏
> 数据跑通资源/模型/视频探测、标题/办公室/角色场景和完整会话回放。
> macOS 窗口路径仍需在真机上首次构建/运行确认，首次反馈请附
> `cmake` 与运行日志。

## 依赖

需要 macOS 12+、Xcode Command Line Tools 和 Homebrew：

```sh
brew install cmake sdl3 molten-vk vulkan-loader vulkan-headers glslang
```

- `molten-vk`：Vulkan → Metal。
- `vulkan-loader`：SDL3 通过它加载 MoltenVK（运行时需要）。
- `vulkan-headers`：编译期的 `vulkan/vulkan.h`。
- `glslang`：构建时把 `runtime/render/shaders/*` 编译成内嵌 SPIR-V。

## 构建

```sh
./build-macos.sh
```

脚本会：

1. 校验 Homebrew 依赖，自动取 `brew --prefix`（Apple Silicon 通常是
   `/opt/homebrew`，Intel 是 `/usr/local`）；
2. 以 `-DBK_NATIVE_WINDOW=ON -DBK_BUILD_TESTS=OFF` 配置 `build-macos/`；
3. 链接 Vulkan loader（运行时经 MoltenVK ICD 分发到 Metal），输出 `build-macos/biko3`（即 `biko3-preview` 目标）。

可选环境变量：

| 变量 | 作用 |
| --- | --- |
| `BK_BUILD_DIR` | 构建目录，默认 `build-macos` |
| `BK_BUILD_JOBS` | 并行任务数，默认 `sysctl -n hw.ncpu` |
| `BK_BUILD_TYPE` | 默认 `RelWithDebInfo` |
| `BK_MACOS_MIN` | 最低系统版本，默认 `12.0` |
| `BK_MACOS_ARCH` | 例如 `arm64` 或 `arm64;x86_64`（通用二进制；Homebrew MoltenVK 通常为单架构，需配套） |
| `MOLTENVK_PREFIX` | 覆盖 MoltenVK 前缀（CMake 也会自动用 `brew --prefix molten-vk`） |

手动构建等价命令：

```sh
cmake -S . -B build-macos \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBK_NATIVE_WINDOW=ON \
  -DBK_BUILD_TESTS=OFF \
  -DCMAKE_PREFIX_PATH="$(brew --prefix)" \
  -DMOLTENVK_PREFIX="$(brew --prefix molten-vk)" \
  -DMESA_SWITCH_SDK="$(brew --prefix vulkan-headers)"
cmake --build build-macos --parallel 8 --target biko3-preview
```

## 运行

```sh
# 窗口模式（首次排错建议）
BIKO3_WINDOWED=1 ./build-macos/biko3

# 全屏（默认）
./build-macos/biko3
```

默认数据目录（`Data/` 的上一级，即原来的 `MAINDIR`）：

```text
~/Library/Application Support/Biko3/game/
├── Data/
└── patch.pp        # 可选：汉化/去码外挂
```

存档、照片和音量配置写入：

```text
~/Library/Application Support/Biko3/
├── save/
├── captures/
└── album/
```

环境变量：

| 变量 | 作用 |
| --- | --- |
| `BIKO3_GAME` | 游戏目录（包含 `Data/`）；也可作为第一个命令行参数传入 |
| `BIKO3_DATA_DIR` | 覆盖用户数据根目录（save/captures/album 的父目录） |
| `BIKO3_WINDOWED=1` | 1280×720 窗口模式 |
| `BK_SHOW_FPS=1` | 启动时显示原版 FPS |
| `BIKO3_SHOW_SYSTEM_CURSOR=1` | 保留 macOS 系统箭头（默认隐藏，游戏绘制自己的光标） |
| `BIKO3_INPUT_DEBUG=1` | 打印前 12 次鼠标事件的窗口/像素尺寸与缩放结果 |
| `VK_ICD_FILENAMES` | 指定 MoltenVK ICD 清单；通常平台层会自动探测 Homebrew 路径 |

按键与 Linux 桌面一致（键盘/鼠标/最多 4 个 SDL 手柄），见
[Linux 原生运行](linux-native.md) 的按键表。

### 鼠标与 Retina 坐标

SDL3 的鼠标事件使用**窗口坐标（points）**，而游戏内部指针/视口使用
**渲染像素**。在 2x Retina 屏幕上，1280×720 窗口的实际 drawable 是
2560×1440；`darwin.c` 用 `SDL_GetWindowSize()` 与当前 drawable 像素尺寸的
比例把事件换算到渲染像素，避免光标只覆盖屏幕一半或整体偏移。启动日志会
打印：

```text
Input: window 1280x720 points, drawable 2560x1440 pixels, pointer scale 2.00x/2.00x
```

游戏默认隐藏 macOS 系统箭头（原版通过 `ShowCursor(FALSE)` 隐藏并绘制
自己的菜单光标）。保留系统箭头可用于排查：
`BIKO3_SHOW_SYSTEM_CURSOR=1`。若仍有偏移，用 `BIKO3_INPUT_DEBUG=1` 运行
并把前几行 `Pointer:` 日志发回即可定位是窗口比例还是渲染视口问题。

## 打包 .app

```sh
./tools/package_macos_app.sh
# 可选：--build build-macos --app dist/Biko3.app --icon /path/to/icon.png
```

生成 `dist/Biko3.app`，包含：

- `Contents/MacOS/biko3`（运行时二进制）；
- `Info.plist`（高分屏、最低系统版本、版本号取自
  `config/dependencies.lock.json`）；
- `THIRD_PARTY.md` 与 `licenses/` 的第三方许可说明；
- 可选 `.icns` 图标（脚本用 `sips`/`iconutil` 从 PNG 生成）；
- ad-hoc 签名（`codesign -s -`，仅本地使用；公开分发需要 Developer ID
  和 notarization）。

**游戏数据不打进 app**：放到 `~/Library/Application Support/Biko3/game`，
或用 `BIKO3_GAME` 指定。这样可以避免 App Translocation / 只读 bundle 导致
存档写不进去。

当前 `.app` 通过绝对路径链接 Homebrew 的 `SDL3`、`vulkan-loader` 和
`MoltenVK`，适用于本机构建/自用。若要分发给没有 Homebrew 的机器，需要把
这些 dylib 打包进 `Contents/Frameworks`、设置 `@rpath`/`VK_ICD_FILENAMES`，
并做 Developer ID 签名与 notarization；这一部分尚未自动化。

## MoltenVK 排障

如果启动时报 `SDL_Vulkan_LoadLibrary` 或 `vkCreateInstance` 失败：

1. 确认 `vulkan-loader` 与 `molten-vk` 都已安装：
   `brew list --versions vulkan-loader molten-vk`；
2. 让 loader 找到 MoltenVK 的 ICD 清单：
   ```sh
   export VK_ICD_FILENAMES="$(brew --prefix molten-vk)/etc/vulkan/icd.d/MoltenVK_icd.json"
   ```
   `runtime/platform/darwin.c` 会先尝试常见的 Homebrew 路径，但自定义前缀
   或 app 从 Finder 启动时建议显式设置（或在启动脚本里设置）；
3. 用 `vulkaninfo --summary` 检查设备是否为 `MoltenVK` / `Apple M*`；
4. 如果使用通用二进制，确认 MoltenVK 也包含目标架构。

## 已知限制 / 待验证

- **macOS 真机尚未验证**（当前验证在 Linux + lavapipe 软件 Vulkan 上完成）。
  首次在 Mac 上构建/运行时请保留日志。
- 默认构建为当前机器架构；通用二进制需要匹配的 MoltenVK。
- 全屏使用 macOS 的独立 Space（`SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES`），
  切换 Space 可能触发窗口像素尺寸变化；如遇画面尺寸异常请反馈。
- 窗口模式固定 1280×720；全屏分辨率取桌面当前模式，运行中不切换分辨率。
- 照片/存档格式是 runtime 自己的格式，不自动导入原版 `.ckp` 存档。
- 提交或分发前请遵守仓库/作者的授权约定；游戏数据、补丁、字体等素材
  不随源码分发。

## 验证记录

在本机（Linux x86_64、Mesa 26 lavapipe）使用英文 2004 数据完成的验证：

- `asset-probe bk3_00.pp --verify`：575 项 / 574 张图片解码；
- `model-probe bk3_03.pp m00_10.x`：58 meshes / 281 submeshes / 36,967
  顶点 / 24,089 三角形 / 260 材质 / 121 纹理 / 93 帧；
- `avi-probe bk3_18.pp`：`poi.avi`、`D_moza.avi` 解码 PASS；
- 离屏渲染：标题、办公室、角色预览；
- `tests/check_game_application.py --replay-hz 15`：标题→开场→游戏→拍照→
  暂停/恢复→回标题 PASS（2 张 BMP）。
