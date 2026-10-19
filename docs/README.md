# 文档索引

按当前任务选择专题。运行方法在本目录，当前能力在[状态页](roadmap.md)，实验与阶段证据在[报告索引](../reports/README.md)。代理入口见 [llms.txt](../llms.txt)。

## 使用与配置

| 需要做什么 | 文档 |
| --- | --- |
| 在 Mac 上构建、运行，调整窗口、菜单和数据目录 | [macOS 原生运行](macos-native.md) |
| 在 Linux 上运行、配置手柄或安装到 Steam | [Linux 原生运行](linux-native.md) |
| 使用 Switch NRO、安装 NSP 或确认存档位置 | [Switch 运行与安装](switch.md) |
| 查按键、菜单、拍照或镜头操作 | [操作说明](controls.md) |
| 启用汉化/去码，处理字库或生成外挂 | [补丁与编码](patches.md) |
| 定位启动、字体、输入或存档问题 | [常见问题](faq.md) |

## 开发与实现

| 需要做什么 | 文档 |
| --- | --- |
| 开始修改或提交文档 | [开发指南](../CONTRIBUTING.md) |
| 准备 SDK、交叉构建、生成 SD / NSP 包 | [构建与打包](build.md) |
| 确定模块接口、状态所有者和资源释放时机 | [架构](architecture.md) |
| 选择单测、原版对照或实际应用探针 | [测试与验证](testing.md) |
| 确认功能接入、平台验证和剩余范围 | [当前状态](roadmap.md) |
| 查第三方依赖许可 | [第三方许可](../THIRD_PARTY.md) |

模块局部约定：[game](../runtime/game/README.md)、[world](../runtime/world/README.md)、[model](../runtime/model/README.md)、[media](../runtime/media/README.md)、[save](../runtime/save/README.md)。依赖版本以[锁定文件](../config/dependencies.lock.json)为准。

## 证据与历史

[报告索引](../reports/README.md)按问题列出所有报告。报告中的计数、摘要和“尚未实现”只对应其记录阶段；具体成品以对应版本和内容版本的记录为准。

已被替代、仅重复现有说明的旧稿由 Git 历史追溯，不再复制整页快照。有独立验证价值的报告保留在报告索引中。

组织方式参考 [nerdctl](https://github.com/containerd/nerdctl#additional-documents)：README 提供上手路径，按用途链接专题文档；本项目另将实验与修复报告单独索引。
