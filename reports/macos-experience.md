# macOS 体验优化第一批（P0）

本轮目标不是继续补“能启动”，而是把 macOS 版本从开发预览推进到可日常使用的
桌面应用。第一批只处理不需要重写渲染器的项目：设置持久化、启动窗口、首次
数据目录选择、操作提示/FPS 开关、焦点暂停和开发 overlay 清理。

## 已实现

### 1. `settings.cfg` 持久化

新增 `runtime/core/settings.{c,h}`，文件位于数据根目录：

```text
~/Library/Application Support/Biko3/settings.cfg
```

默认值：

```ini
windowed=1
width=1280
height=720
show_help=0
show_fps=0
show_dev_overlay=0
invert_look=0
game_path=
```

- 原子写入 `settings.cfg.tmp` 后 rename；未知键和非法值忽略，旧版本配置不会阻止启动；
- 窗口大小在关闭时保存；F1/F2 切换后立即保存；
- `BIKO3_WINDOWED` / `BIKO3_FULLSCREEN` 仍可临时覆盖启动模式。

### 2. 启动窗口模式

`darwin.c` 在创建窗口前读取配置：

- 默认 **1280×720 窗口**，不再默认全屏；
- `windowed=0` 或 `BIKO3_FULLSCREEN=1` 可全屏启动；
- 窗口尺寸由 `width`/`height` 控制，逻辑点坐标，Retina 下仍使用 drawable 像素；
- 全屏切换/运行中改窗口大小目前需要重启（swapchain 重建属于下一批，不在本轮
  假装已支持）。

### 3. 首次启动数据目录选择

默认数据目录不存在 `Data/` 时，macOS 会弹出系统文件夹选择器：

- 选择包含 `Data/` 的 `MAINDIR`；
- 直接选择 `Data/` 时自动取父目录；
- 选择结果写入 `settings.cfg` 的 `game_path`，下次启动不再询问；
- 显式传入 `BIKO3_GAME` 或 argv 路径但无 `Data/` 时仍然直接报错，不会弹框掩盖配置错误。

### 4. 操作提示与 FPS

- 左侧操作说明默认关闭；`F2` 切换，设置持久化；
- 原版 FPS 默认关闭；`F1` 切换，设置持久化；
- 开发状态 overlay（`BIKO3 - NVK DEVELOPMENT PREVIEW` / `PORT IN DEVELOPMENT`）
  默认关闭，标题/办公室/角色检查场景不再显示；调试时可用
  `show_dev_overlay=1` 打开；
- 控制提示和 FPS 仍走原有渲染路径，不改变原作 UI 坐标。

### 5. 焦点与电源

- 窗口失焦后暂停模拟并降低轮询频率，切回后继续；
- 运行时禁用屏幕保护；关闭时恢复。

### 6. `invert_look`

为手柄右摇杆的 Y 轴提供配置项，默认不反转；键鼠光标映射保持原样。

## 验证

- `tests/test_settings.c`：默认值、往返保存、路径、非法值、未知键、缺失文件、空 root
  全部通过；
- Linux 主机构建 `biko3-preview` 通过；
- macOS `darwin.c` 以 SDL3/头文件做 `-fsyntax-only -Werror` 通过；
- 实际 offscreen 标题渲染：
  - 默认配置：无左提示、无底部开发 overlay、无 FPS；
  - `show_help=1 show_fps=1 show_dev_overlay=1`：三项按预期出现；
- 主机 replay 注入 `F1+F2`：日志输出 `Original FPS counter: on`、`Control help: on`，
  `settings.cfg` 写入 `show_help=1`、`show_fps=1`；
- 未在真实 macOS 窗口路径上运行（本机无 macOS 图形会话），也未验证全屏/窗口
  热切换；这两项仍属于下一批。

## 下一批（P1）

1. 渲染器 swapchain 重建，实现真正的运行中全屏/窗口/分辨率切换；
2. 帧率上限与 ProMotion/60Hz 帧节奏；
3. 持久化 `VkPipelineCache`，降低首次场景卡顿；
4. 音频输出设备选择、缓冲和热插拔；
5. universal2 + 依赖打包 + codesign/notarization/DMG。

以上没有在本轮声称完成，避免把“设置已存在”误报成完整 macOS 体验验收。
