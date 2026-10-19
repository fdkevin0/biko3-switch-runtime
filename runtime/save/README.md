# save：持久化格式与文件提交

管理检查点、鉴赏解锁、动作记录、音量，以及暂停截图/相册文件。只接收数据或字节，不推断剧情、不解析游戏资源目录、不依赖 scene/game 或 GPU。应用负责连接用户目录、平台时钟和提交服务。

| 接口 | 数据与用途 |
| --- | --- |
| [checkpoint.h](checkpoint.h)、[checkpoint_file.h](checkpoint_file.h) | 五组各十槽的区域入口检查点、八字节库存和时间；不是任意运行帧快照 |
| [unlock.h](unlock.h)、[unlock_file.h](unlock_file.h) | 独立鉴赏解锁表 |
| [record.h](record.h)、[record_file.h](record_file.h) | 借用 app 的记录数组视图，保留路线、动作和计数 |
| [volume_file.h](volume_file.h) | 进程级语音、BGM、音效设置 |
| [capture_file.h](capture_file.h) | 独立输出目录中的暂停背景和相册 BMP |
| [file_commit.h](file_commit.h) | app 注入的平台事务提交回调，供 NSP HOS 存档使用 |

磁盘格式显式编解码字段，不转储 C 结构体、指针或 GPU 句柄。移植版检查点、解锁和动作记录带版本/维度/CRC 检查；读取完整校验后才发布。缺失、损坏和 I/O 失败分开处理，损坏旧文件不能当作空档覆盖。

写入使用同目录临时文件及已有替换/备份协议，失败保留可恢复数据。HOS 的写入/删除由提交回调显式 commit；多个文件分别提交，不声称跨文件事务或断电持久性。

原 `.b3f` 等格式的内存 codec 用于对照，不意味着自动导入旧 PC、SD 或旧 TitleID 存档。区域入口页和真实存读档已接入，中文标签编码属于 app/scene 显示适配，不改变存档格式。

相关证据：[检查点](../../reports/checkpoint-storage.md)、[存读档界面](../../reports/save-session.md)、[动作记录](../../reports/record-storage.md)、[解锁](../../reports/unlock-storage.md)、[HOS 接入](../../reports/nsp-1.0.78.md)。平台目录见[文档索引](../../docs/README.md)。
