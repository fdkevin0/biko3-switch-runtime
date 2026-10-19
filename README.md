# 尾行3 / Biko3 — 原生运行时

《尾行3》的 C11 原生移植工程。Switch 使用 libnx 和指定 Mesa NVK；macOS 使用 SDL3 + MoltenVK；Linux 使用 SDL3 + Vulkan。同一套游戏逻辑直接读取原版 `Data/`，支持可选汉化与去码 `patch.pp`。

[文档索引](docs/README.md) · [操作说明](docs/controls.md) · [常见问题](docs/faq.md) · [开发指南](CONTRIBUTING.md) · [当前状态](docs/roadmap.md)

## 快速开始

macOS 安装依赖、构建并运行：

```sh
brew install cmake sdl3 molten-vk vulkan-loader vulkan-headers glslang
./build-macos.sh
./build-macos/biko3
```

首次运行可选择包含 `Data/` 的游戏目录。默认路径为 `~/Library/Application Support/Biko3/game`，`patch.pp` 与 `Data/` 同级。游戏数据和补丁素材由用户自行准备，不随源码提供。

| 平台 | 构建 / 启动 | 详细说明 |
| --- | --- | --- |
| macOS | `./build-macos.sh` → `build-macos/biko3` | [窗口、设置、数据目录与 app 打包](docs/macos-native.md) |
| Linux | `./build-linux.sh` → `build-native/biko3` | [运行与 Steam 安装](docs/linux-native.md) |
| Switch | `./build-switch.sh` → `build-switch/biko3-runtime.nro` | [NRO、NSP 与存档](docs/switch.md) |

Linux 的 Steam 安装入口是 `./install-steam.sh`。Switch 的 SDK 准备和交叉构建见[构建说明](docs/build.md#switch)。

## 功能与验证范围

已接入标题、五人选择、剧情与跟踪、失败/重试、暂停、结局、特殊场景、鉴赏回放、音量、相册，以及进度、动作记录、解锁和照片持久化。画面保持原版 4:3，宽屏输出居中显示。

当前以实际反馈维护，功能接入和主机检查不等于完整剧情或最新 Switch 实机验收。各平台进展与限制统一见[当前状态](docs/roadmap.md)，具体修复和数据见[报告索引](reports/README.md)。版本及依赖取自[锁定文件](config/dependencies.lock.json)。

## 更多文档

- [汉化、去码与字体编码](docs/patches.md)：启用补丁或从自有资源生成外挂。
- [架构与所有权](docs/architecture.md)：模块边界、共享状态和渲染生命周期。
- [构建与打包](docs/build.md)、[测试与探针](docs/testing.md)：开发环境和验证入口。
- [第三方许可](THIRD_PARTY.md)：依赖及素材分发边界。
- [llms.txt](llms.txt)：供代理按需查阅的文档入口。
