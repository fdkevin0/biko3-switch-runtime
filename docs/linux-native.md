# Linux 原生运行（aarch64 / Wayland / Vulkan）

本分支在原 Switch NRO 工程上增加了桌面平台后端，让同一套运行时直接读取
SD 卡目录里的 `Data` 与 `patch.pp`，在 Linux 上以 Vulkan 窗口运行。

已在 KONKR Pocket FIT Elite（Snapdragon 8 Elite / Adreno 830，Armada OS，
Mesa Turnip 26.2.3，KDE Wayland，1920×1080@144Hz）实机跑通：标题、选人、
手柄按键与全屏显示均正常。

## 构建

依赖：CMake ≥ 3.24、GCC/Clang、Vulkan 头文件与 loader、glslangValidator、
SDL3 开发包、zlib。

```sh
cmake -S . -B build-native \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBK_NATIVE_WINDOW=ON \
  -DBK_BUILD_TESTS=OFF \
  -DMESA_SWITCH_SDK=/usr
cmake --build build-native --parallel 8 --target biko3-preview
```

产物为 `build-native/biko3`（`biko3-preview` 目标在桌面构建下改名为
`biko3`）。`MESA_SWITCH_SDK` 仅用于复用现有的 Vulkan 头文件路径检查；
桌面构建链接系统 `libvulkan`，不会使用 devkitPro 的 NVK 静态库。

如果系统没有 `-devel` 包（例如不可变发行版），可以在 Fedora 容器里构建，
再把二进制拿到宿主机运行；二进制只依赖宿主机的 `libSDL3.so.0`、
`libvulkan.so.1`、`libz.so.1` 与 glibc。

## 运行

```sh
~/.local/bin/biko3
```

或点击桌面上的「尾行3」。启动器会：

1. 通过 InputPlumber 的 D-Bus 接口把内置手柄切换成 `xbox-elite`
   （标准 uinput Xbox 手柄）。默认的 `deck-uhid` 目标在未绑定
   `hid-steam` 内核驱动时只暴露 `/dev/hidraw`，SDL 无法当作手柄使用。
2. 以全屏 1920×1080（当前显示器桌面分辨率）启动游戏。

游戏数据默认在 `~/.local/share/biko3/game`（`Data/` 与 `patch.pp`）。
可用 `BIKO3_GAME=/path/to/game` 或第一个命令行参数覆盖。

存档、照片、音量配置：
`~/.local/share/biko3/`（`save/`、`captures/`、`album/`）。

调试用环境变量：

| 变量 | 作用 |
| --- | --- |
| `BIKO3_WINDOWED=1` | 1280×720 窗口模式，便于调试 |
| `BIKO3_CAPTURE=/tmp/frame.ppm` | 第 120 帧写一张 PPM 截图 |
| `BIKO3_INPUT_DEBUG=1` | 打印按键边沿 |
| `BIKO3_GAME=/path` | 指定游戏数据目录 |
| `BK_SHOW_FPS=1` | 启动时显示原版 FPS |

## 按键

| 手柄 | 键盘 | 作用 |
| --- | --- | --- |
| A（下） | Enter / Space | 确认、交互、对白推进 |
| B（右） | Esc / Backspace | 返回、姿态切换 |
| X（左） | X | 镜头切换 |
| Y（上） | C | 拍照 |
| Start | Tab | 暂停菜单 |
| Select / Back | F1 | 原版 FPS 开关 |
| 十字键 | 方向键 | 菜单选择（标题/选人自动定位光标） |
| 左摇杆 | WASD | 移动 / 菜单光标 |
| 右摇杆 | — | 游戏视角 |
| LB | Q | 结局/特殊剧情镜头旋转（对应 Switch L） |
| RB | E | 镜头远近/高度（对应 Switch R） |
| LT | Shift | 慢走（对应 Switch ZL） |
| 触屏/鼠标 | — | 定位菜单光标，点击=确认 |

## 实现说明

- `runtime/platform/linux.c`：SDL3 窗口、Vulkan surface 工厂、键盘/鼠标/
  触屏/多手柄输入、时钟、存档目录。
- `runtime/platform/audio_sdl.c`：SDL3 推流音频（48kHz PCM16 立体声，
  4×10ms 队列，消费计数来自 SDL 队列）。
- `runtime/render/window_surface.h`、`bk_renderer_create_window()`：
  渲染器仍自建 VkInstance/device，平台只提供窗口系统实例扩展与
  `VkSurfaceKHR`。
- `runtime/app/application.c`：桌面构建使用 `BK_DESKTOP_PACING`，只在原版
  1ms 采样门真正采样时推进一帧。144Hz 下若每帧都复用两帧步长，游戏会以
  2 倍速运行；该宏不影响 Switch 构建。
- 渲染分辨率跟随 swapchain（当前 1920×1080），游戏内容仍按 4:3 居中，
  左侧黑边显示金色按键说明。

## 已知限制

- 手柄支持依赖 InputPlumber 服务；若它没有运行，USB/蓝牙标准手柄仍可被
  SDL 直接识别，但内置 MCU 手柄不会出现。
- 全屏分辨率取桌面当前模式，运行中不支持切换分辨率；窗口模式用
  `BIKO3_WINDOWED=1`。
- 桌面构建不包含 Switch 的 NRO/NSP 打包步骤。
