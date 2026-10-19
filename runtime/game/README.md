# game：流程与控制

恢复游戏的入口、玩家/NPC 行为、任务状态、结局控制和有序服务请求。CPU 代码依赖 world 及其下层能力；真实图片、声音、GPU 和文件所有者由 scene/app 提供，game 不直接访问 libnx 或 Vulkan。

控制器保留原版阶段、共享字节、RNG 消耗与调用顺序。服务没有实现或状态非法时明确失败；合法的原版无更新分支保留其行为。生产兼容适配与严格 CPU 入口分开，不能因一个已知哨兵放宽其他索引校验。

按路径查阅：

- 入口与任务帧：[entry.h](entry.h)、[frame_dispatch.h](frame_dispatch.h)，证据见[游戏帧](../../reports/game-frame.md)和[应用入口](../../reports/play-session.md)。
- 玩家与 NPC：[player_control.h](player_control.h)、[npc_ai.h](npc_ai.h)，空间查询由 world 提供，实际表现和音频由 scene 装配。
- 结局与回放：[ending_control.h](ending_control.h)、[ending_record.h](ending_record.h)，共享记录由 app 持有；[侧栏修复](../../reports/ending-sidebar.md)说明当前事件适配边界。
- 天气与绘制策略：[rain.h](rain.h)、[draw_dispatch.h](draw_dispatch.h)，只产生策略和服务调用，GPU 提交属于 scene/render。

当前能力见[状态页](../../docs/roadmap.md)，跨模块约定见[架构](../../docs/architecture.md)。
