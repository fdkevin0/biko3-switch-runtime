# 报告索引

本页列出全部 Markdown 报告，按问题查阅；同目录 JSON 保存各次机器可读结果、生成元数据和验证摘要。当前能力看[状态页](../docs/roadmap.md)，操作方法看[文档索引](../docs/README.md)。

报告描述具体阶段、二进制和夹具，不是当前任务清单。旧“尚未接应用”、待办或一次性授权保留为证据；更晚构建的结果不能靠文件名或显示版本继承。

[测试与遗留代码审计（2026-10-19）](test-audit-2026-10-19.json)记录删除依据、真实缺陷修复、保留的覆盖及本机验证范围。

[macOS 标题/人物选择页窗口缩放验证](macos-window-resize.json)记录缓存布局修复、原问题复现、实际 Retina 窗口和输入回归范围。

[引擎边界修复（2026-10-19）](engine-boundaries-2026-10-19.json)记录公共 UI 画布、输入、设置存储、流程归属与结局角色绑定，以及缩放和跨帧截图的红绿验证。

[Data 与手写控制逻辑来源核查（2026-10-19）](data-logic-source-audit.md)记录全部 CKP/XAN 检查，以及选择结局中资产默认连接与 EXE 动态控制的分界。

[结局绑定精简试点（2026-10-19）](ending-bindings-2026-10-19.json)记录共享描述符写入、回调精简，以及 52,702 帧真实素材前后摘要对照和重载回归。

[代码精简与剧情导出验证（2026-10-19）](simplification-2026-10-19.json)记录后续共享回调、背景表零初始化、固定流程复用及现有解析器导出的范围；改动量以本轮开始时的工作区为基线。

## 主题导航

- [平台与交付](#平台与交付)（11）
- [补丁、字体与文字](#补丁字体与文字)（8）
- [持久化、菜单与前端](#持久化菜单与前端)（23）
- [模型、动画与节点](#模型动画与节点)（28）
- [渲染、灯光与性能](#渲染灯光与性能)（15）
- [世界、追踪与交互](#世界追踪与交互)（45）
- [结局界面与输入](#结局界面与输入)（17）
- [结局录制与回放](#结局录制与回放)（12）
- [结局状态与资源](#结局状态与资源)（41）
- [特殊剧情](#特殊剧情)（7）
- [音视频与时序](#音视频与时序)（10）

共 217 份报告；具体字段与范围以报告正文及其引用的数据文件为准。

## 平台与交付

- [macos-experience-p1.md](macos-experience-p1.md)：macOS 体验优化第二批（P1）
- [macos-experience.md](macos-experience.md)：macOS 体验优化第一批（P0）
- [macos-menu.md](macos-menu.md)：macOS 菜单栏开关
- [macos-platform.md](macos-platform.md)：macOS 平台后端（SDL3 + MoltenVK）
- [nsp-1.0.78.md](nsp-1.0.78.md)：原版 NSP 与 HOS 存储
- [nsp-update-1.0.78.md](nsp-update-1.0.78.md)：BKTR 更新包 1.0.78cn
- [nsp-update-1.0.79.md](nsp-update-1.0.79.md)：BKTR 更新包 1.0.79cn
- [nsp-update-diagnostic.md](nsp-update-diagnostic.md)：1.0.79cn 诊断更新 NSP
- [porting.md](porting.md)：尾行3 Switch / NVK 移植记录
- [switch-controls-hover.md](switch-controls-hover.md)：日文按键说明与结局悬停退出
- [switch-performance.md](switch-performance.md)：Switch 开发版：减速、负载波动与音频断流修复

## 补丁、字体与文字

- [chinese-dialogue.md](chinese-dialogue.md)：中文文字字号与滚动
- [chinese-patch.md](chinese-patch.md)：纯中文 BKPT2 外挂（基于 3DH 绿色版）
- [combined-patch.md](combined-patch.md)：汉化与去码合并外挂
- [dialogue-ui.md](dialogue-ui.md)：剧情文字与结束分派
- [dialogue.md](dialogue.md)：原版对话脚本状态
- [font.md](font.md)：FTT字体、原文字排版与字形遮罩
- [patch-loading.md](patch-loading.md)：补丁读取与压缩优化
- [text-display.md](text-display.md)：文字滚动、显示缓存与原多遍混合

## 持久化、菜单与前端

- [album-application.md](album-application.md)：相册应用入口
- [album-switch-fix.md](album-switch-fix.md)：Switch 相册与剧情推进修复
- [capture-lifecycle.md](capture-lifecycle.md)：跨流程截图与相册数量
- [checkpoint-next-story.md](checkpoint-next-story.md)：保存完成后进入下一段剧情退出（2026-10-01）
- [checkpoint-storage.md](checkpoint-storage.md)：区域入口存档数据层
- [flow-loading.md](flow-loading.md)：flow50加载控制与真实加载界面
- [menu-cursor.md](menu-cursor.md)：菜单鼠标常显与摇杆操作
- [menu-shortcuts.md](menu-shortcuts.md)：菜单返回与确认快捷键
- [original-front-end.md](original-front-end.md)：原版标题、五人选择与剧情接入应用
- [record-storage.md](record-storage.md)：结局动作记录持久化与重启回放
- [save-car-crash.md](save-car-crash.md)：存档与撞车退出排查（2026-09-28）
- [save-session.md](save-session.md)：存读档页面与真实会话
- [screenshot.md](screenshot.md)：暂停背景与拍照截图
- [selection-actor.md](selection-actor.md)：选择页面演员 CPU 装配
- [selection-audio.md](selection-audio.md)：选择页真实语音与口型
- [selection-session.md](selection-session.md)：正常人物选择页组合
- [selection-ui.md](selection-ui.md)：角色选择界面（flow38）
- [selection-world.md](selection-world.md)：人物选择世界与绘制组合
- [session-entry.md](session-entry.md)：外层入口目的地与进程初态
- [title-menu.md](title-menu.md)：日文标题菜单组件
- [unlock-storage.md](unlock-storage.md)：独立鉴赏进度的编码、保存与启动读取
- [volume-application.md](volume-application.md)：标题与暂停音量设置（2026-10-01）
- [volume-menu.md](volume-menu.md)：音量页控制、显示与试听组件（2026-10-01）

## 模型、动画与节点

- [actor-entry.md](actor-entry.md)：角色入口状态与保留字段
- [actor-forest.md](actor-forest.md)：真实姿态的全局节点装配
- [actor-pose.md](actor-pose.md)：0.3.5：角色姿态、根坐标与独立播放实例
- [actor-render.md](actor-render.md)：动态角色绘制与应用接入
- [actor-start.md](actor-start.md)：原始XAN加载后选片
- [bom-assets.md](bom-assets.md)：BOM 实际绑定与辅助模型生命周期
- [bom-deformation.md](bom-deformation.md)：BOM 节点对齐与顶点映射
- [bom-motion.md](bom-motion.md)：BOM 输入、回弹与逐帧节点跟随
- [bom-render.md](bom-render.md)：GPU 蒙皮后的顶点回调
- [bom-seconds.md](bom-seconds.md)：辅助模型按秒动画与 MORP 组混合
- [camera-animation.md](camera-animation.md)：M1.4 相机轨道采样与运行时检查入口 — 0.3.3
- [camera-clips.md](camera-clips.md)：M1.4 相机片段调度与朝向基础 — 0.3.4
- [clip-edits.md](clip-edits.md)：XAN实例动态片段修改
- [eye-assets.md](eye-assets.md)：眼部资源与纹理选择
- [eye-pose.md](eye-pose.md)：眼球朝向与角色矩阵缓存
- [face-assets.md](face-assets.md)：实际角色面部装配与控制（CPU工作树快照）
- [face-controller.md](face-controller.md)：音量包络与口型/眨眼控制（CPU工作树快照）
- [fixed-animation.md](fixed-animation.md)：固定调用动画与整模型 MORP 采样
- [frame-tree.md](frame-tree.md)：全局帧节点与缓存发布
- [material-animation.md](material-animation.md)：MATA 材质动画与固定调用组合
- [materials.md](materials.md)：M1.2：材质状态与纹理绑定
- [menu-camera-assets.md](menu-camera-assets.md)：角色选择双轨道与文本镜头姿态
- [menu-camera.md](menu-camera.md)：角色选择镜头数学
- [model-cpu.md](model-cpu.md)：M1.1：CPU 模型解析与节点层级
- [morph-data.md](morph-data.md)：MORP数据解码（CPU工作树快照）
- [morph-pose.md](morph-pose.md)：MORP顶点采样与过渡（CPU工作树快照）
- [skinning.md](skinning.md)：ENVL蒙皮数据与CPU变形
- [track-forest.md](track-forest.md)：入口实例与相机轨道装配

## 渲染、灯光与性能

- [cpu-static-review.md](cpu-static-review.md)：2026-09-28 CPU 热路径静态审查
- [draw-dispatch.md](draw-dispatch.md)：主绘制根分派与事件接管
- [draw-queues.md](draw-queues.md)：全局网格提交队列
- [ending-raster-origin.md](ending-raster-origin.md)：结局画面剩余差异：视口原点隔离
- [gpu-skin-performance.md](gpu-skin-performance.md)：GPU 蒙皮与 CPU 热路径优化（2026-09-28）
- [lighting-motion-depth.md](lighting-motion-depth.md)：画面移动时明暗闪烁：深度精度修复
- [lighting-motion-transform.md](lighting-motion-transform.md)：第一场景移动镜头后的灯光闪烁
- [lighting-pass.md](lighting-pass.md)：原版灯光注册与绘制分派
- [lighting-surface-layers.md](lighting-surface-layers.md)：第二场景物体表面的明暗跳变
- [lighting.md](lighting.md)：M1.4 点光照子步骤 — 0.3.1
- [neon-skinning.md](neon-skinning.md)：ARM64 NEON 矩阵与 ENVL 蒙皮
- [npc-occlusion-performance.md](npc-occlusion-performance.md)：NPC 无效遮挡检查优化（2026-10-01）
- [spot-lighting.md](spot-lighting.md)：人物选择聚光灯
- [static-scene.md](static-scene.md)：M1.3：原版办公室静态接入
- [vulkan-bindings.md](vulkan-bindings.md)：Vulkan 图形状态绑定缓存（2026-10-01）

## 世界、追踪与交互

- [ai-collision.md](ai-collision.md)：NPC AI、视线与静态碰撞（工作树快照）
- [area-boundary.md](area-boundary.md)：发现结算与区域边界/提示音
- [area-handover.md](area-handover.md)：区域结束保存询问与保留角色交接依赖
- [area-session.md](area-session.md)：区域交接应用快照
- [background-lifecycle.md](background-lifecycle.md)：背景生命周期与音频（CPU）
- [background-render.md](background-render.md)：背景GPU资源与雾状态
- [camera-probe-fix.md](camera-probe-fix.md)：镜头贴墙反复缩放修复
- [camera.md](camera.md)：M1.4 投影、视图矩阵与相机加载证据 — 0.3.2
- [common-hud.md](common-hud.md)：公共HUD与转场请求
- [dialogue-actor.md](dialogue-actor.md)：对话人物与固定镜头
- [dialogue-backdrop.md](dialogue-backdrop.md)：对话背景切换与独立黑幕
- [dialogue-entry.md](dialogue-entry.md)：原版剧情入口与首个文本页
- [dialogue-session.md](dialogue-session.md)：真实剧情场景装配
- [dynamic-collision.md](dynamic-collision.md)：每帧动态道具碰撞（0.3.5之后工作树）
- [entry-binding.md](entry-binding.md)：入口资源与镜头帧阶段（0.3.5之后工作树）
- [failure-session.md](failure-session.md)：失败场景与重试流程
- [failure-transition.md](failure-transition.md)：失败结算HUD与五种镜头依赖
- [follow-obstacle.md](follow-obstacle.md)：跟随镜头的真实场景障碍
- [gallery-application.md](gallery-application.md)：鉴赏菜单实际应用连接
- [gallery-menu.md](gallery-menu.md)：鉴赏菜单控制与真实资源绘制
- [game-frame.md](game-frame.md)：中央51a682/4ec78d更新装配
- [hiding-exit.md](hiding-exit.md)：藏身物退出卡住（2026-10-01）
- [item-feedback.md](item-feedback.md)：拾取提示文本与实际音效
- [item-notice.md](item-notice.md)：拾取提示面板、图标与计时
- [items.md](items.md)：物品实例、显示与拾取
- [movement-animation.md](movement-animation.md)：NPC路线更新与玩家动作提交（工作树快照）
- [npc-frame.md](npc-frame.md)：NPC 表现阶段组合
- [npc-presentation.md](npc-presentation.md)：NPC隐藏、动画暂停与淡入淡出（工作树快照）
- [npc-shadow.md](npc-shadow.md)：NPC 网格阴影
- [npc-spatial.md](npc-spatial.md)：NPC缓存头节点与空间更新（工作树快照）
- [opening.md](opening.md)：开场对话、面板与镜头交接
- [pause-background-fix.md](pause-background-fix.md)：暂停背景更新修正
- [play-session.md](play-session.md)：首关应用与持久流程装配
- [player-hotkeys.md](player-hotkeys.md)：游戏热键、照片计数与真实服务接入
- [player-hud.md](player-hud.md)：操作界面状态、绘制与会话绑定
- [player-idle.md](player-idle.md)：开场玩家更新与镜头距离衔接
- [player-interaction.md](player-interaction.md)：玩家交互与脚本空间更新（0.3.5之后工作树）
- [player-presentation.md](player-presentation.md)：玩家表现与动作音效（0.3.5之后工作树）
- [player-spatial.md](player-spatial.md)：玩家移动、墙面与普通空间更新
- [player-view.md](player-view.md)：玩家镜头（0.3.5之后工作树）
- [prop-interaction.md](prop-interaction.md)：道具交互与音频保位续播
- [prop-lifecycle.md](prop-lifecycle.md)：道具实例、路线、显示阶段（0.3.5之后工作树）
- [rain.md](rain.md)：原版雨幕精灵
- [snow-camera.md](snow-camera.md)：雪景相机父节点与连续渲染
- [weather-integration.md](weather-integration.md)：实际游戏雨雪接入与雪花零缩放修正

## 结局界面与输入

- [ending-camera-controls.md](ending-camera-controls.md)：结局开场画幅核对与 L/R 镜头操作
- [ending-camera-direction.md](ending-camera-direction.md)：结局摇杆镜头方向修复
- [ending-camera-six-icons.md](ending-camera-six-icons.md)：结局镜头六倍速与 L/R 提示图标
- [ending-confirmation.md](ending-confirmation.md)：结局阶段9确认、取消与真实UI绑定
- [ending-framing.md](ending-framing.md)：结局镜头目标、活动相机与可选命中节点
- [ending-node-bindings.md](ending-node-bindings.md)：结局交互锚点、显隐节点与辅助场景取景
- [ending-sidebar.md](ending-sidebar.md)：结局侧栏物品、姿势切换与摇杆动作
- [ending-stage-ui.md](ending-stage-ui.md)：结局分支精灵与实际图片绘制
- [ending-ui-frame.md](ending-ui-frame.md)：结局完整 UI 帧与跨重载图片保留
- [ending-ui-hints.md](ending-ui-hints.md)：结局分支提示、进度条与游标显示
- [ending-ui-integration.md](ending-ui-integration.md)：普通结局生产场景 UI 接入
- [ending-ui-pick.md](ending-ui-pick.md)：结局游标的镜头角度与目标命中
- [ending-ui-scheduling.md](ending-ui-scheduling.md)：结局 UI 无图片推进、指示线与公共工具栏
- [ending-ui-select.md](ending-ui-select.md)：结局实际游标选择与语音门限
- [ending-ui-tail.md](ending-ui-tail.md)：结局共享黑幕与 UI 尾部
- [ending-ui.md](ending-ui.md)：结局界面布局、动画和GPU资源
- [ending-viewport-input.md](ending-viewport-input.md)：最后界面画幅、光标与图标定位修复

## 结局录制与回放

- [ending-gallery-auxiliary.md](ending-gallery-auxiliary.md)：Gallery 第五类回放 488674
- [ending-gallery-control.md](ending-gallery-control.md)：phase8 回放父控制（2026-09-30）
- [ending-gallery-effect.md](ending-gallery-effect.md)：phase8 效果音与语音依赖（2026-09-30）
- [ending-gallery-normal.md](ending-gallery-normal.md)：phase8 普通动作回放控制（2026-09-30）
- [ending-gallery-presentation.md](ending-gallery-presentation.md)：Phase8 表现调度48BCBB
- [ending-gallery-secondary.md](ending-gallery-secondary.md)：phase8 第二类动作回放（2026-09-30）
- [ending-gallery-selected.md](ending-gallery-selected.md)：phase8 选择动作回放（2026-09-30）
- [ending-gallery-session.md](ending-gallery-session.md)：鉴赏回放的进程所有者与真实场景接入
- [ending-gallery-tertiary.md](ending-gallery-tertiary.md)：Gallery 第四类动作回放 48758C
- [ending-natural-record.md](ending-natural-record.md)：普通结局自然输入录制与连续选择回放
- [ending-record-input.md](ending-record-input.md)：结局实际输入录制与空记录入口检查
- [ending-record.md](ending-record.md)：结局记录与故事入口

## 结局状态与资源

- [ending-47dc79-state1.md](ending-47dc79-state1.md)：47DC79 state1/state2 控制器边界
- [ending-47dc79-state3-parent.md](ending-47dc79-state3-parent.md)：47DC79 state3 父控制器边界
- [ending-47dc79-state4.md](ending-47dc79-state4.md)：47DC79 state4 控制器边界
- [ending-47dc79-state567.md](ending-47dc79-state567.md)：47DC79 state5/6/7 结局序列
- [ending-47dc79-state8.md](ending-47dc79-state8.md)：47DC79 state8 前置控制器
- [ending-4d39e6-presentation.md](ending-4d39e6-presentation.md)：4D39E6 资源与 48181F 表现阶段
- [ending-auxiliary-cycle.md](ending-auxiliary-cycle.md)：自动片段切换与真实资源对照
- [ending-auxiliary.md](ending-auxiliary.md)：结局辅助动作、真实动画与六通道声音
- [ending-background.md](ending-background.md)：普通结局背景与共享灯光
- [ending-camera-assets.md](ending-camera-assets.md)：结局双镜头真实资源与加载顺序
- [ending-camera.md](ending-camera.md)：结局场景的四种镜头数学
- [ending-clock-camera.md](ending-clock-camera.md)：结局状态复位对照、独立计时与入场镜头依赖
- [ending-control.md](ending-control.md)：结局公共操作控制
- [ending-draw-events.md](ending-draw-events.md)：普通结局绘制事件、特殊视口与语音音量
- [ending-draw-stage.md](ending-draw-stage.md)：结局实际阶段绘制与视频推进
- [ending-entry-assets.md](ending-entry-assets.md)：结局入口分派与角色装配依赖
- [ending-exit.md](ending-exit.md)：结局确认后的返回标题、退出与共享黑幕
- [ending-final-image.md](ending-final-image.md)：结局最终图片的实际资源生命周期
- [ending-frame.md](ending-frame.md)：结局事件帧分派与工作进度
- [ending-inventory.md](ending-inventory.md)：结局背包共享、辅助 state1/2 与禁用目标悬停
- [ending-lifecycle.md](ending-lifecycle.md)：结局入口工作表与退出音频生命周期
- [ending-normal-assets.md](ending-normal-assets.md)：普通结局的真实 CPU 资源装配
- [ending-normal-render.md](ending-normal-render.md)：普通结局常规 GPU 绘制
- [ending-normal-session.md](ending-normal-session.md)：普通结算场景应用接入
- [ending-presentation.md](ending-presentation.md)：结局表现组件与语音采样验证记录
- [ending-preset.md](ending-preset.md)：结局预设镜头过渡
- [ending-random-state.md](ending-random-state.md)：结局随机状态所有权与跨场景验证
- [ending-reload.md](ending-reload.md)：结局黑幕重载控制与效果音释放
- [ending-secondary-assets.md](ending-secondary-assets.md)：第二类结局 4D00FA：独立 CPU 资源与镜头初始化
- [ending-selected-adapters.md](ending-selected-adapters.md)：选择阶段真实服务与结束序列
- [ending-selected-motion.md](ending-selected-motion.md)：结局选择阶段：输入与动画源时间
- [ending-selected-session.md](ending-selected-session.md)：选择阶段的实际入口、交互与重载
- [ending-skin-homogeneous.md](ending-skin-homogeneous.md)：结局交互中的齐次蒙皮修复
- [ending-sound.md](ending-sound.md)：结局入口声音与语音压低
- [ending-special-render.md](ending-special-render.md)：普通结局的双视角 GPU 绘制
- [ending-special-scene.md](ending-special-scene.md)：结局特殊镜头的真实场景适配
- [ending-special.md](ending-special.md)：结局特殊双镜头调度与局部深度清除
- [ending-stage-lifecycle.md](ending-stage-lifecycle.md)：结局阶段资源生命周期与自然往返
- [ending-state.md](ending-state.md)：结局入口状态复位与资源表绑定
- [ending-tertiary-cpu.md](ending-tertiary-cpu.md)：第三类结局 CPU 父控制与表现
- [ending-tertiary-session.md](ending-tertiary-session.md)：第三类结局的实际场景与应用接入

## 特殊剧情

- [special-camera-assets.md](special-camera-assets.md)：特殊剧情真实镜头与重复动画通道
- [special-event-cpu.md](special-event-cpu.md)：特殊剧情 flow48 的逐帧控制
- [special-media.md](special-media.md)：特殊剧情真实音频、视频纹理与绘制（2026-09-30）
- [special-session.md](special-session.md)：特殊剧情应用入口与资源生命周期
- [special-ui-media.md](special-ui-media.md)：特殊剧情界面的真实图片、声音和相册边界
- [special-ui.md](special-ui.md)：特殊剧情界面的原版控制与绘制快照
- [special-world-assets.md](special-world-assets.md)：特殊剧情真实 CPU 场景装配与背景绘制开关

## 音视频与时序

- [audio-defaults.md](audio-defaults.md)：音量默认值
- [audio-fps-controls.md](audio-fps-controls.md)：三类基准音量与 FPS 开关
- [avi-texture.md](avi-texture.md)：AVI视频纹理与选择页特殊外观
- [bgm-gauge.md](bgm-gauge.md)：BGM 基准与左上角能量条修复
- [dialogue-audio.md](dialogue-audio.md)：剧情语音与音乐
- [footsteps.md](footsteps.md)：NPC 脚步与空间音量
- [game-clock-fps.md](game-clock-fps.md)：原版游戏步长与 FPS 显示
- [npc-event-audio.md](npc-event-audio.md)：NPC警戒、等待和路点音效接线
- [pcm-audio.md](pcm-audio.md)：PCM、播放队列与 NPC 音频连接
- [prop-audio.md](prop-audio.md)：道具音效与可变播放频率（0.3.5之后工作树）
