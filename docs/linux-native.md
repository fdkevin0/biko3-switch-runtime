# Linux 原生运行

SDL3 + Vulkan 桌面后端直接读取 `Data/` 和可选 `patch.pp`，不经过 Proton。已记录 KONKR Pocket FIT Elite / Snapdragon 8 Elite / Adreno 830，Armada OS、Mesa Turnip 26.2.3、KDE Wayland 下的标题、选人、手柄和全屏运行；这描述该设备与当时构建，不代表所有 Linux 环境。

## 构建与运行

需要 CMake 3.24+、GCC/Clang、SDL3 开发包、Vulkan 头文件/loader、zlib 和 `glslangValidator`：

```sh
./build-linux.sh
./build-native/biko3 /path/to/MAINDIR
```

桌面目标名为 `biko3-preview`，输出 `build-native/biko3`。脚本默认从 `/usr` 找头文件，链接系统 Vulkan；自定义前缀见[构建说明](build.md#linux)。不可变发行版可在匹配宿主依赖的容器中编译，再在宿主运行。

默认全屏采用当前桌面模式。窗口启动：

```sh
BIKO3_WINDOWED=1 ./build-native/biko3
```

## 数据目录

默认游戏根目录为 `~/.local/share/biko3/game`，包含 `Data/` 与可选 `patch.pp`。第一个命令行参数或 `BIKO3_GAME` 可指定其他游戏根目录。存档、照片和音量配置位于 `~/.local/share/biko3/` 下的 `save/`、`captures/`、`album/`。

| 变量 | 作用 |
| --- | --- |
| `BIKO3_GAME` | 游戏根目录 |
| `BIKO3_WINDOWED=1` | 以 1280×720 窗口启动 |
| `BIKO3_INPUT_DEBUG=1` | 打印输入按键边沿 |
| `BIKO3_CAPTURE=/tmp/frame.ppm` | 在指定帧输出调试截图，行为见应用入口 |
| `BK_SHOW_FPS=1` | 启动时显示原版 FPS |

## 手柄与启动器

标准 USB/蓝牙手柄由 SDL3 直接识别，最多 4 个；游戏按键和键盘映射见[操作说明](controls.md)。部分掌机内置 MCU 手柄需要 InputPlumber 将输入暴露为标准 uinput 设备。

安装后的 `~/.local/bin/biko3` 启动器尝试通过 D-Bus 切到 `xbox-elite` 目标，避免只暴露 `/dev/hidraw` 的配置无法被 SDL 当作手柄读取。直接运行构建产物不执行这一步。InputPlumber 不是所有 Linux 手柄的必要依赖。

## Steam

先准备游戏数据，然后执行：

```sh
./install-steam.sh
```

脚本安装或更新运行时、启动器、桌面入口，并写入 Steam 非 Steam 游戏快捷方式及可用封面。缺少 `build-native/biko3` 且没有已安装程序时先构建。它会修改当前用户的安装位置和 Steam 配置，适用于明确要求安装或更新的任务。

默认位置包括：

- 运行时：`~/.local/share/biko3/biko3`
- 启动器：`~/.local/bin/biko3`
- 桌面入口：`~/.local/share/applications/biko3.desktop`
- Steam 配置：`~/.local/share/Steam/userdata/<SteamID>/config/shortcuts.vdf`

`BIKO3_PREFIX`、`BIKO3_GAME`、`BIKO3_STEAM_NAME` 可调整安装根、数据目录和名称。关闭 Steam 后安装，再重启客户端，避免客户端退出时覆盖快捷方式文件。

## 实现与限制

`platform/linux.c` 提供 SDL 窗口、输入和 Vulkan surface 工厂，`platform/audio_sdl.c` 负责设备音频队列。renderer 仍拥有 VkInstance/device，平台只交付窗口扩展和 surface 创建服务。

分辨率使用当前桌面模式，内容仍按 4:3 居中；该后端没有 macOS 的完整运行时设置菜单。游戏时间沿用原版步长，`game/wall≈2` 是测量口径，不能据此减半逻辑步数，见[速率报告](../reports/game-clock-fps.md)。桌面构建不包含 Switch 打包步骤。
