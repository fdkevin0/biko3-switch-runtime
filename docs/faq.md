# 常见问题与排障

先确认运行的是桌面窗口、主机离屏入口、Switch NRO 还是 NSP。它们共享游戏逻辑，但数据目录、输出位置和驱动入口不同。[文档索引](README.md)提供各平台完整说明。

## 启动时找不到 Data

游戏根目录应包含 `Data/`，可选 `patch.pp` 与它同级。桌面可用 `BIKO3_GAME=/path/to/game` 或第一个命令行参数指定；macOS 还可通过首次启动文件夹选择器保存路径。目录示例见 [macOS](macos-native.md#数据目录) 或 [Linux](linux-native.md#数据目录)。

主机 `build/biko3-preview` 是有帧数上限的离屏工具；需要可玩窗口时构建 `build-macos/biko3` 或 `build-native/biko3`。

## macOS 编译时报 u_int / u_char 未定义

Cocoa 菜单头文件需要 Darwin 扩展类型。只定义 `_POSIX_C_SOURCE` 时会隐藏这些类型；当前 CMake 已在 macOS 桌面平台目标上补 `_DARWIN_C_SOURCE`。更新源码后重新运行 `./build-macos.sh`，不要在 SDK 中手工补 typedef。

## MoltenVK 或 Metal 初始化失败

确认 Homebrew 的 `sdl3`、`vulkan-loader` 和 `molten-vk` 已安装。窗口构建需要 loader 与 SDL 使用同一套 Vulkan 分发；不能把它直接替换成离屏探针的 MoltenVK 链接方式。

ICD 查找失败时，给当前启动设置实际清单路径，例如：

```sh
VK_ICD_FILENAMES="$(brew --prefix molten-vk)/etc/vulkan/icd.d/MoltenVK_icd.json" \
  ./build-macos/biko3
```

清单也可能位于安装前缀的 `share/vulkan/icd.d/`。离屏检查若看不到任何 Metal 设备，先确认运行环境能访问 GPU，再解释该检查结果。

## 中文存档槽的日期、时间或区域乱码

旧标签使用 Shift-JIS，而汉化 FTT 在重叠码位存放 GBK 字形，例如 `82 4F` 会把「０」显示成「侽」。当前源码已按汉化标志切换标签编码，重新构建并重启程序即可使用。[修复报告](../reports/save-session.md#2026-10-19汉化存档槽文字乱码)记录实际补丁和界面验证；既有存档无需转换。

其他中文问题先区分图片、脚本文字和程序标签，再核对实际加载的补丁与字库，见[编码说明](patches.md#编码与-ftt-字库)。

## 黑边、窗口尺寸或 Retina 鼠标偏移

游戏内容固定 4:3，宽屏两侧留黑是预期行为。macOS 鼠标事件使用窗口 points，游戏使用 drawable pixels，平台按两者比例转换。使用 `BIKO3_INPUT_DEBUG=1` 查看窗口/像素尺寸与缩放日志；`BIKO3_SHOW_SYSTEM_CURSOR=1` 可临时显示系统箭头。设置和快捷键见 [macOS 说明](macos-native.md)。

## 手柄或 Steam 启动后没有输入

标准 USB/蓝牙手柄由 SDL3 识别。部分 Linux 掌机内置手柄需要 InputPlumber 将设备暴露为标准 uinput 手柄；对应启动器会尝试 `xbox-elite` 目标。先按 [Linux 说明](linux-native.md#手柄与启动器)核对设备路径，macOS 不依赖 InputPlumber。

## 存档或照片没有出现在另一种运行方式中

macOS、Linux、Switch NRO 和 NSP 的输出根目录不同；NSP 还按当前用户和 TitleID 区分 HOS SaveData。原 PC 存档、旧 TitleID 与 SD 照片不自动迁移。先查看对应平台的数据目录及 [Switch 存储说明](switch.md#存档与照片)，保留原文件后再决定是否需要迁移工具。

## 旧报告说某功能未实现

报告描述当次实现与测试，保留它是为了追溯证据。[当前状态](roadmap.md)说明现有接入；[报告索引](../reports/README.md)按主题关联修复。更晚的一次构建也不会自动继承旧二进制的全部测试结论。
