# model：CPU 资产、动画与变形

输入为资源层提供的只读字节，输出为模型、材质、节点、动画、蒙皮与变形数据。依赖 core/resource，不包含 Vulkan、libnx、存档或任务状态。GPU 上传由 scene/render 完成。

## 资产与运行实例

[model.h](model.h) 定义解码结果与所有权；解码模型拥有源数据副本，由 `bk_model_destroy` 释放。解析后资产保持不可变，播放实例借用资产，分别拥有时间、局部姿态及缓存；先销毁实例再释放资产。

[animation.h](animation.h)、[clip.h](clip.h)、[playback.h](playback.h) 分别处理轨道采样、片段配置和实例推进。普通采样缓存与混合采样的更新时间不同，重复动作请求也不应重启已播放片段；见[演员姿态](../../reports/actor-pose.md)、[运动动画](../../reports/movement-animation.md)和[固定调用](../../reports/fixed-animation.md)。

## 其他适配

- [material.h](material.h)、[material_pose.h](material_pose.h)、[material_animation.h](material_animation.h)：材质语义、实例状态和 MATA。
- [skin.h](skin.h)、[morph.h](morph.h)、[morph_pose.h](morph_pose.h)、[bom_deform.h](bom_deform.h)：ENVL、MORP 和 BOM 的 CPU 数据与数值规则。
- [static_model.h](static_model.h)、[environment.h](environment.h)：实例队列、LIGH/FOG 和节点关联；GPU 灯光与全局排序由 scene/render 接入。
- [x_pose.h](x_pose.h)：文本镜头姿态的显式入口；不等于通用文本 `.x` 绘制模型解析器。

未知格式和无效引用明确返回错误或不支持，保留原始 chunk 不等于已经消费它。原版字段、单位与浮点规则依据相关报告维护，不把 CPU 数值或 GPU 探针扩大为全游戏画面证明。入口见[架构](../../docs/architecture.md)及[模型报告索引](../../reports/README.md#模型动画与节点)。
