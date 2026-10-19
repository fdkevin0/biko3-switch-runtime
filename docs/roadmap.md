# 当前状态与验证范围

更新：2026-10-19。主要游戏功能已接入，当前按实际反馈维护。源码版本取自[依赖锁](../config/dependencies.lock.json)；源码、某次二进制和已交付包是不同对象，不能凭同一显示版本合并测试结论。

## 功能接入

| 领域 | 当前能力 | 主要证据 |
| --- | --- | --- |
| 前端与任务 | 标题、五人选择、剧情、跟踪、区域交接、失败/重试和暂停 | [前端](../reports/original-front-end.md)、[应用入口](../reports/play-session.md)、[交接修复](../reports/checkpoint-next-story.md) |
| 动态世界 | 演员姿态、ENVL/MORP、表情、移动、碰撞、跟随镜头和天气 | [演员姿态](../reports/actor-pose.md)、[GPU 蒙皮](../reports/gpu-skin-performance.md)、[雨雪接入](../reports/weather-integration.md) |
| 结局与回放 | 普通/选择/第三类场景、动作记录、回放、特殊场景和鉴赏入口 | [自然输入记录](../reports/ending-natural-record.md)、[鉴赏回放](../reports/ending-gallery-session.md)、[特殊场景](../reports/special-session.md) |
| 菜单与输出 | 存读档、音量、相册、拍照与菜单快捷键 | [存读档](../reports/save-session.md)、[音量](../reports/volume-application.md)、[相册](../reports/album-application.md)、[菜单](../reports/menu-shortcuts.md) |
| 音视频与时序 | 语音/BGM/音效、设备供给、AVI 纹理与独立时钟 | [原版速率](../reports/game-clock-fps.md)、[音量默认值](../reports/audio-defaults.md)、[媒体](../reports/special-media.md) |
| 持久化 | 检查点、动作记录、解锁、照片和音量；NSP 按用户写 HOS 存档 | [记录存储](../reports/record-storage.md)、[解锁](../reports/unlock-storage.md)、[HOS 接入](../reports/nsp-1.0.78.md) |
| 汉化与去码 | BKPT2 外挂、中文资源与字库、中文文字适配，可选去码 | [合并补丁](../reports/combined-patch.md)、[绿色版外挂](../reports/chinese-patch.md)、[中文文字](../reports/chinese-dialogue.md) |

接入表示有生产应用路径，不表示全剧情或全部资源组合已遍历。具体入口、夹具和帧数以对应报告为准。

## 平台状态

| 平台 | 已记录的结果 | 仍未覆盖的范围 |
| --- | --- | --- |
| macOS | 原生构建与标题/人物选择页 Retina 缩放通过；UI 统一逻辑画布，真实资源暂停、存读档、重试及结局视图的缩放和截图边界已检查，见[引擎边界验证](../reports/engine-boundaries-2026-10-19.json) | 未做完整自然剧情和全部菜单点击审核；按当前系统维护，不做旧系统兼容验收 |
| Linux | 已在 Armada OS / Adreno / Turnip 的掌机上记录标题、选人、手柄与全屏运行，另有离屏资源和应用检查 | 不是本轮源码在全部 Linux 环境的验证，设备条件见 [Linux 说明](linux-native.md) |
| Switch | 2026-10-02 正式 NRO / 本体 / BKTR 更新已交付；指定 NVK 构建和专项主机回归完成 | 用户负责后续安装、性能和稳定性验证；最新更新只做生成检查，不能继承此前包的独立检测 |

## 最近维护

2026-10-19 的源码已修复两项 macOS 使用中暴露的问题：Cocoa 菜单编译时补齐 Darwin 类型可见性；汉化存档槽标签按当前语言输出 GBK，日期、时间与区域显示恢复。后者有普通及 ASan/UBSan 单测、中日实际界面和截图证据，见[存读档报告](../reports/save-session.md#2026-10-19汉化存档槽文字乱码)。这些源码改动没有自动重建或覆盖此前 Switch 交付包。

2026-10-02 的结局侧栏修复及人物动作适配仍保留，见[侧栏记录](../reports/ending-sidebar.md)。用户此前已确认雨天、存档/撞车和物体表面光影闪烁的相应修复；跨视口原点微小光栅差异继续保留为[单独诊断](../reports/ending-raster-origin.md)，没有改写为全面视觉通过。

## 已交付包的记录

- [原版 NSP 与存储](../reports/nsp-1.0.78.md)：本体 ID `01094F68D7330000`。
- [正式 1.0.79cn 更新](../reports/nsp-update-1.0.79.md)：更新 ID `01094F68D7330800`，最新记录内容版本 196610；具体摘要和检测范围在该报告维护。
- [补丁生成与样本](../reports/chinese-patch.md)：绿色版生成的纯中文、中文去码样本与早期 Switch 合并补丁分别记录。

安装方式见 [Switch 说明](switch.md)，重建方式见[构建与打包](build.md)。历史交付目录与 ZIP 不随源码自动刷新。

## 维护范围

后续围绕用户反馈、明确功能和可定位的性能问题推进。完整剧情路线、其他藏身点审核和最新 Switch 实机结果不作为自动扩展任务；需要这些工作时由用户指定范围。既有检查仅因本次改动、失败或未解决疑点扩展。
