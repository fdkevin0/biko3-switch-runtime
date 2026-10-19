# 构建与打包

桌面使用同一套 CMake 目标；Switch 分支另选 libnx、NVK 和 NRO/NSO 工具链。平台运行与数据目录分别见 [macOS](macos-native.md)、[Linux](linux-native.md)、[Switch](switch.md)。

## 主机构建

基础依赖为 CMake 3.24+、C11 编译器、Python 3 和 zlib。Vulkan 路径还需要头文件、驱动/loader 和 `glslangValidator`，桌面窗口再需要 SDL3。

| 入口 | 产物与用途 |
| --- | --- |
| `./build-macos.sh` | `build-macos/biko3`，macOS 可玩窗口 |
| `./build-linux.sh` | `build-native/biko3`，Linux 可玩窗口 |
| `./build-host.sh` | `build/` 中的 CPU 单测和检查工具 |
| `./build-host.sh --vulkan` | 再加入离屏渲染、场景和应用探针 |

桌面 CMake 目标名仍是 `biko3-preview`，输出文件改名为 `biko3`。主机离屏入口保留 `build/biko3-preview`；它不是桌面可玩窗口。相关目标的定向构建方法见[测试说明](testing.md)。

### macOS

```sh
brew install cmake sdl3 molten-vk vulkan-loader vulkan-headers glslang
./build-macos.sh
```

`BK_BUILD_DIR`、`BK_BUILD_JOBS`、`BK_BUILD_TYPE` 可调整目录、并行度和构建类型。`BK_MACOS_ARCH` 传给 CMake 的架构列表；多架构构建需要配套架构的依赖。本项目按当前 macOS 环境维护，旧系统兼容性不纳入验收。

窗口版通过 Vulkan loader 调用 MoltenVK，离屏主机探针直接链接 MoltenVK。`MOLTENVK_PREFIX` 是 CMake/主机构建的查找参数；`build-macos.sh` 自己通过 Homebrew 取得前缀。SDK 头文件路径在桌面构建中仍沿用变量名 `MESA_SWITCH_SDK`，不表示桌面链接了 Switch NVK。

### Linux

```sh
./build-linux.sh
```

脚本使用 `BK_NATIVE_WINDOW=ON`、系统 Vulkan 和 SDL3。自定义头文件前缀可用 `MESA_SWITCH_SDK`，默认 `/usr`；更多运行条件见[Linux 说明](linux-native.md)。

## Switch

需要 devkitPro/devkitA64、libnx、Switch expat/zstd/zlib，以及 Python/Pillow 和 `glslangValidator`。依赖版本、Mesa 提交、Rust 工具链及下载摘要以[锁定文件](../config/dependencies.lock.json)为准。

```sh
python3 tools/fetch_sdk.py
./build-switch.sh
```

依赖下载到本地 `local/`；已校验的 SDK 归档可保留复用。若下载附件已过期，先处理下载来源，不把静默升级依赖作为修复。

产物为 `build-switch/biko3-runtime.nro`，名称/作者为 `biko3-runtime` / `ILLUSION`。图标由本地 `icon.jpg` 生成 256×256 baseline JPEG，源图不改写。CMake 可用 `BK_NRO_ICON` 指定图标、`Python3_EXECUTABLE` 指定带 Pillow 的 Python。

正式构建默认 `BK_SWITCH_FILE_LOG=OFF`。日志诊断包是单独的构建用途；历史诊断包不代表正式包的日志行为。

## macOS app

```sh
./tools/package_macos_app.sh
```

默认重建 `dist/Biko3.app`，其中包含二进制、Info.plist、第三方许可和本地 ad-hoc 签名；游戏素材留在 app 外。脚本会替换指定输出 app，需要保留现有包时使用新路径：

```sh
./tools/package_macos_app.sh --build build-macos --app dist/Biko3-test.app
```

可用 `--icon /path/to/icon.png` 生成图标。本机构建仍依赖 Homebrew dylib；公开分发所需的依赖打包和签名不在该脚本的当前范围内。

## SD 目录

```sh
python3 tools/package_sd.py local/game/MAINDIR --output '交付/新SD目录'
```

默认复制当前入口所需的 21 个 PP 及配套散文件；`--full-data` 复制全部 22 个归档，`--complete-data` 复制原版完整 Data 文件集合。复制按 SHA-256 检查，不包含 EXE、原存档或补丁。外挂需按[补丁说明](patches.md)单独配置。

显式指定新输出目录，避免把历史 SD 包当作当前构建的自动更新目标。工具检查见[打包记录](../reports/package-sd-verification.json)。

## NSP 本体与 BKTR 更新

本体打包需要本地 devkitPro 工具、hacBrewPack 和用户密钥：

```sh
python3 tools/package_nsp.py local/game/MAINDIR \
  --output '交付/biko3-01094F68D7330000.nsp'
```

现有输出不会被覆盖。工具默认从本地密钥路径读取，不下载或分发密钥。可选参数见脚本 `--help`。

BKTR 更新使用本地 hacPack，先应用本仓库的适配补丁；完整更新只作为生成差分包的中间输入：

```sh
git -C local/hacpack apply ../../tools/hacpack-bktr.patch
make -C local/hacpack -j8
python3 tools/package_nsp.py local/game/MAINDIR \
  --update-patch local/patch-speed/patch.pp --output local/full-update.nsp
python3 tools/package_bktr_update.py \
  '交付/biko3-01094F68D7330000.nsp' local/full-update.nsp \
  --display-version 1.0.79cn --content-version 196610 \
  --workdir local/bktr-build --output '交付/biko3-01094F68D7330800-update.nsp'
```

上例版本参数对应[已记录的 1.0.79cn 构建](../reports/nsp-update-1.0.79.md)，不是新发布自动递增规则。重打包时核对本体关联、内容版本及新输出路径；基础生成检查和独立提取校验分别记录。安装与 HOS 存档说明见[Switch 页面](switch.md)。
