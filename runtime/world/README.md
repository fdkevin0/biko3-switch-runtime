# world：姿态、路线与空间查询

提供节点树、演员姿态、路线推进、跟随/交接镜头、地面/墙面/可见性和碰撞查询。依赖 core/model，不直接访问资源文件、libnx 或 Vulkan，也不判断任务成败。

## 发布与缓存

[actor_pose.h](actor_pose.h)、[actor_forest.h](actor_forest.h) 和 [frame_tree.h](frame_tree.h) 将局部姿态更新与世界矩阵发布分开。根放置、动画提交和节点遍历是不同操作；原版阶段可能读取上次 publish 的缓存，不应为了“刷新”在任意边界重算整棵树。调用方负责资产及实例寿命，见[节点发布记录](../../reports/frame-tree.md)。

## 路线与镜头

[route.h](route.h) 区分活动运动节点、保留终点和固定 1024 槽 flag 查询。跨区域保留的索引可用于 flag 槽读取，不代表允许访问无效运动节点；相关修复见[存档后交接](../../reports/checkpoint-next-story.md)。[route_motion.h](route_motion.h) 只负责运动与边界，跨点动作和任务结果归 game。

[follow_camera.h](follow_camera.h)、[player_camera.h](player_camera.h) 和 [ending_camera.h](ending_camera.h) 消费调用方提供的演员、节点及时间状态。保持已恢复的混合精度、过渡判据与查询顺序；真实场景障碍接入见[跟随障碍记录](../../reports/follow-obstacle.md)。

[collision.h](collision.h)、[ground.h](ground.h) 与 [visibility.h](visibility.h) 处理 CPU 几何；加载、动态实例和帧内调用次序由 scene/game 组合。[架构](../../docs/architecture.md)描述上层所有权。
