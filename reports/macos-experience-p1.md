# macOS 体验优化第二批（P1）

本轮在第一轮设置/首次启动/overlay 清理之上，补上真正影响桌面体验的窗口与
帧节奏能力。

## 已实现

### 1. 运行时全屏/窗口切换

- `F11` 或 `Cmd+Ctrl+F` 在运行中切换全屏/窗口；
- 窗口模式可拖动缩放；
- 切换后窗口 drawable 尺寸变化由平台通知应用，应用调用新的
  `bk_renderer_resize()` 重建 Vulkan swapchain、image view、framebuffer、
  depth/readback 和 semaphore，不重启进程；
- 窗口尺寸/全屏状态写回 `settings.cfg`。

### 2. 渲染器 resize

[`runtime/render/vulkan/renderer.c`](../runtime/render/vulkan/renderer.c) 新增：

```c
int bk_renderer_resize(BkRenderer *, unsigned width, unsigned height,
                       char error[256]);
```

- 窗口路径重建 swapchain 和 presentation 附件；
- offscreen host 路径重建离屏颜色/深度/readback 目标，供测试；
- 重建前 `vkDeviceWaitIdle`，要求不在 active frame；
- 保留 render pass、pipeline、texture、mesh 等与 extent 无关的对象。
- 修复：`end()` 后帧仍处于 submitted 状态，下一次 begin 前收到 resize 不应被误报
  “active frame”；现在只拒绝真正的 active frame，submitted 由 device idle 收尾。
- 第二次修复：resize 后不仅重建 renderer，还通过 `bk_scene_resize()` 通知当前
  scene/session 重新计算缓存的 4:3 viewport。此前旧 viewport 大于缩小后的
  drawable 会触发 `invalid or inactive viewport` 并退出。

### 3. 帧率上限

`settings.cfg` 新增 `fps_cap`，默认 **60**：

```ini
fps_cap=60   # 0 = 跟随显示器刷新率
```

macOS 主循环在呈现后按目标帧预算 idle；ProMotion 120Hz 屏不再无条件以
120 次/秒运行 60Hz 原版逻辑。该上限只作用于 macOS 桌面循环，Linux 主机
离屏测试路径不受影响。

## 验证

- 新增 `tests/test_renderer_resize.c`：
  - 离屏 renderer 64×64 → 128×96 → 96×128；
  - 在 end() 之后、下一次 begin 之前再次 resize（submitted 帧仍在飞行）；
  - 每次 resize 后重新 begin/draw/end/readback 通过；
  - `renderer-resize ok`；
- `tests/test_settings.c` 覆盖 `fps_cap` 默认 60、120 保存/读取；
- Linux 主机构建 `biko3-preview` 通过；
- macOS `darwin.c` 以 SDL3/Vulkan 头文件 `-Wall -Wextra -Werror -fsyntax-only`
  通过；
- 未在真实 macOS 窗口上测试 F11/拖拽 resize；swapchain 和 scene callback
  路径只做了编译/offscreen 验证，不能宣称实机通过。

## 仍未完成

- 持久化 `VkPipelineCache`（当前 pipeline 创建仍无磁盘缓存）；
- 音频输出设备选择、缓冲/热插拔；
- universal2、依赖打包、codesign/notarization、DMG；
- 真实 macOS 首次运行和性能数据。

这些是后续 P1/P2 的明确工作，不把本轮当作完整 macOS 验收。
