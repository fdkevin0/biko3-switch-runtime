# Data 与手写控制逻辑的来源核查（2026-10-19）

本次只读核查运行时、原指令 oracle 与实际 Data；未修改运行时、原素材或存档。覆盖 PP 目录清单、模型块清单、全部 XAN 解码、全部松散 CKP 与一条选择结局调用链，不能外推为全游戏不存在其他脚本机制。

## 关键发现

**原版确有数据驱动行为，而且当前已经在解释执行；目前没有发现一份遗漏的脚本足以替换选择结局的原生控制器。** 把 CKP 称为“只有路线坐标”不准确；把所有 C 表格称为“手工编写业务逻辑”也不准确，已有从 EXE 生成的配置表。

### 1. 全部实际 CKP 的行为码均已覆盖

实际只读目录：`~/Library/Application Support/Biko3/game/Data`。逐文件按 `<4fB3s` 解包，首个四浮点和 flag 全零记录终止活动点，与 [route.c:17](../runtime/world/route.c#L17) 的格式一致。

| 检查项 | 本次实测 |
| --- | --- |
| 文件 | 53 个松散 `.ckp` |
| 有效点 | 9,086 |
| NPC 路线 flag | 0:8555、1:106、2:51、3:15、4:18、5:5、6:42、7:37、8:40、9:20 |
| `l93_` 道具路线 flag | 0:167、2:12、3:18 |
| 全部记录的 3 字节 padding | 48,128 条，全部 `000000` |
| 各文件首终止点后的非零 flag | 0 |

这里按文件名区分普通路线与 `l93_` 路线，仅用于统计。源集合摘要为 `c290b18d0e3d869198d68e75b5dea705484dccc259a095df81b212c54f384430`：按文件名排序，将每个 `name + NUL + SHA256(raw).digest()` 依次输入 SHA256。

实际消费链：

1. [entry_assets.c:98](../runtime/scene/entry_assets.c#L98) 从 `routes` 资源源读取所选 CKP；[route.c:28](../runtime/world/route.c#L28) 解码 flag、位置和 parameter。
2. [npc_route.c:24](../runtime/game/npc_route.c#L24) 在跨过实际路点时，把该点 flag 交给 `bk_npc_point_apply`。
3. [npc_point.c:20](../runtime/game/npc_point.c#L20) 的 0..9 分支解释走路、停留、背景等待、条件门、停止、跑步、特殊动作、显隐切换及音效。比如 flag2 等待 5 秒，flag8 切换淡出状态并请求路线音效。
4. 道具由 [prop_assets.c:274](../runtime/scene/prop_assets.c#L274) 把实际路点 flag 交给 [prop_motion.c:87](../runtime/game/prop_motion.c#L87)，其中 0、2、3 均有语义。

**已验证范围：** 本地实际 CKP 没有出现当前 NPC/道具解释器未处理的 flag，也没有 padding 中的额外字节码证据。这不证明所有资产格式均无脚本。

parameter 也没有整列丢弃：[area_entry.c:58](../runtime/game/area_entry.c#L58) 用首点 parameter 设置玩家视角；[prop_assets.c:125](../runtime/scene/prop_assets.c#L125)、[prop_assets.c:296](../runtime/scene/prop_assets.c#L296) 与 [prop_route.c:70](../runtime/world/prop_route.c#L70) 把它用于道具朝向。未对每一个非零 parameter 的游戏可见效果逐点验收。

### 2. 选择结局的一条代表性控制链来自原生 EXE

[ending_frame.c:97](../runtime/game/ending_frame.c#L97) 的 phase5/6 分派到 `48D8E9` 控制与 `494015` 表现；[ending_selected_control.c:167](../runtime/game/ending_selected_control.c#L167) 在 gate3 调用动作子控制器。

对应的原指令测试不是拿移植版状态机自身当标准：

- [original_ending_selected_control_oracle.py:196](../tests/original_ending_selected_control_oracle.py#L196) 把音频、镜头、输入、资源加载和子函数设为观察边界；[:332](../tests/original_ending_selected_control_oracle.py#L332) 从原 EXE 地址 `0x48D8E9` 执行 x86 指令。
- 同一测试 [:304](../tests/original_ending_selected_control_oracle.py#L304) 在到达原生 `0x48E75B` 时记录 action 调用；[独立 action oracle:424](../tests/original_ending_selected_action_oracle.py#L424) 又直接执行 `0x48E75B`，涵盖动作父控制器、部分数值子函数、片段描述符写入与记录复制。
- [ending_selected_control_tables.inc:1](../runtime/game/ending_selected_control_tables.inc#L1) 注明 210 个原始字来自 EXE 固定地址；[ending_selected_action_tables.inc:1](../runtime/game/ending_selected_action_tables.inc#L1) 明确注明由固定 EXE 生成。它们本来就是数据，而非必须逐条手工维护的 C 算法。

**本次验证方式：** 阅读 oracle 实现及原生入口/边界，未重新运行原指令对照。当前找到的 EXE 与上述 oracle 固定 SHA256 `a3c936...` 不匹配，不能据此宣称本次重新证实二进制等价。

**有依据的推断：** 这条选项、输入、声音等待、镜头与动作状态链的控制分支存在于 EXE 原生代码。没有看到先读取 Data 指令流、再由通用 VM 执行这些分支的证据。媒体函数仍为服务边界，故该 oracle 不能排除它们内部另有资源解释器。

### 3. 名称里的 script 不等于存在脚本文件

[player_script.c:16](../runtime/game/player_script.c#L16) 接收的是移动状态、交互状态、片段时间与输入，没有程序字节流、程序计数器或 opcode 表。它根据 `script_phase` 和动画源时间做预定移动；[player_control.c:29](../runtime/game/player_control.c#L29) 直接调用它。这是“预定动作控制”，不能因文件名就推断漏接了原版脚本。

真正的数据解释器可在 [dialogue.c:58](../runtime/resource/dialogue.c#L58) 与 [:116](../runtime/resource/dialogue.c#L116) 看到：它保留原始文本 cursor，扫描标签，并解释 `#bg/#SG/#g/#C/#F/#E/#M/#P/#se`。这类 C 分支是解释标签的语义；把分支改写成另一份配置，不会消除解释器本身。

### 4. 全部 XAN 已解码，实际资产与动态改写的边界已核实

本次继续追踪 **`phase5/6 → 48D8E9 → gate3 → 48E75B → 495469/4952C8 → XAN 描述符`**，并调用现有 CPU 库 `bk_clip_set_decode` 检查全部实际 XAN，而非只读历史报告：

| 检查项 | 本次实测 |
| --- | --- |
| XAN | 250 份，全部 20,880 字节（`0x5190`） |
| 现有解码器拒绝数 | 0 |
| 非零动画范围的描述符 | 2,165 个 |
| 全部 32,000 个描述符的 `+0x80..+0x98` 尾部字段 | 全零 |
| 选择结局默认主角资源 | 两个 variant × 五组 × 三个 selection，共 30 份 |

文件中描述符的 `+0x04..+0x43` 是可读动作名称，`+0x54/+0x58` 是动画范围，`+0x70/+0x74` 是 chain/next。首两段模型名与实例初态也由既有解码器检查或读取，见 [clip.c:45](../runtime/model/clip.c#L45)。这批 XAN 中没有发现遗漏在描述符尾部的指令流；没有将名称字段视为程序执行。

30 份选择结局主角资源依据 [ending_selected.c:28](../runtime/game/ending_selected.c#L28) 的默认命名规则选取：`bk3_10.pp` 中 `h01..05_03..05.xan`，以及 `bk3_13.pp` 中 `h01..05_13..15.xan`。这是进程初始资源集合，不声称覆盖任意运行时替换的路径。

这些资源的下列默认值完全一致：

| 片段 slot | 资产 chain | 资产 next | 资产 loop |
| --- | --- | --- | --- |
| 5 / 6 / 7 | 0 | 6 / 7 / 6 | 0 |
| 10 / 11 | 0 | 11 / 10 | 0 |
| 13 / 14 | 0 | 10 / 6 | 0 |
| 17 / 18 | 0 | 18 / 17 | 0 |

因此，**Data 已经表达动画的默认连接，但没有预先启用这些自动连接，也不能单靠静态 next 同时表达不同输入下的后继。** 当前 [ending_selected_action.c:562](../runtime/game/ending_selected_action.c#L562) 的 scheduled 分支根据按键把 slot7.next 从 6 改为 13，另一输入分支把它恢复为 6，并将 slot11.next 改为 14；[:607](../runtime/game/ending_selected_action.c#L607) 随后启用相关 chain。取消路径 [:201](../runtime/game/ending_selected_action.c#L201) 又恢复默认连接。

这不是把 XAN 动画帧逐条重写成 C：动画范围和默认连接仍来自 XAN，控制器负责输入条件、运行时改写与恢复。**某次写入等于资产默认值，也不能直接判定冗余**，因为此前分支可能已经改过同一实例。

`495469/4952C8` 的移植实现分别在 [ending_selected_motion.c:22](../runtime/game/ending_selected_motion.c#L22) 与 [:43](../runtime/game/ending_selected_motion.c#L43)，用指针距离或拖动量计算当前动画 source；[ending_selected_assets.c:82](../runtime/scene/ending_selected_assets.c#L82) 将结果写回实例。它们消费 XAN 的 start/end/duration，不读取额外 Data 程序。原指令 oracle 中这两个数值子函数保持执行，并未替换为移植版回调，见其 [:381](../tests/original_ending_selected_action_oracle.py#L381)。本次没有因而宣称重跑了该 oracle。

XAN 源集合摘要：`9aabf9ffb2f1caa6b5d70039c54257fadfbb8d2950d1867210e094790ce2c7c5`。按 PP 文件名排序，保留包内顺序，将每份 `archive + NUL + name + NUL + SHA256(raw).digest()` 依次输入 SHA256。扫描通过 `tools/bk3_assets.py` 的 `Archive` 只读解包，以 `tests/clip_binding.py` 绑定 `build/audit-python/libmodel-test.dylib`，逐个解码并销毁；脚本断言 XAN 总数 250、长度一致和选择结局样本数 30。

### 5. 文件清单没有揭示另一套剧情程序

原版 Data 的 22 个 PP 共 7,081 个条目：图片 4,162、WAV 2,239、模型 250、XAN 250、VIX 86、FAM 50、TXT 27、BOM 7、AVI 2，以及 8 个 Windows 缩略图数据库。扩展名只用于分类，不能单独证明无脚本。

进一步检查全部 250 个模型：247 个为 OBJM，3 个为 DirectX 文本；OBJM 块完整遍历并校验块范围，出现的类型仅有 MATE/TEXT/MESH/FRAM/ANIM/FOG/ENVL/MORP/LIGH/MATA/TEXA/PART。块名清单不能证明各块内部没有未知语义，本次也没有逐字段验收全部模型效果。

27 个 TXT 中，26 个带数字标签及 `#end`；命令词扫描见到 `#C/#F/#E/#M/#SG/#g/#bg/#bg_off/#P/#se` 及 `#end`。原始扫描的 `#PT/#PH` 实际属于 `#P` 后跟资源名称，不是额外操作码。剩余 `i00_10.txt` 为十行区域、日期和时间占位标签，没有控制命令；其 SHA256 为 `f25511b1b6bdc26b521701d9b1dc6b8a42d1dbe8b84663da3b5b7335dd275958`。这次是词汇与结构检查，没有重新执行全部对话或验证未知命令的游戏效果。

本地详细扫描元数据留在 `build/validation/data-logic-source/{inventory,formats,xan}.json`，不包含资源正文；这些是忽略的本地分析产物，不是源码依赖。

## 对工程方向的影响

**当前项目实际承担了“资源引擎 + 原版 EXE 游戏层的原生重建”。** [架构](../docs/architecture.md) 也明确将角色行为和阶段控制放在 `game/`，将真实资源和服务组合放在 `scene/`。若产品边界是“只做解释原版 Data 的通用引擎”，这条已核实的结局流程就缺少决定输入、状态转换和动态连接的程序来源，不能只靠接入现有 Data 补齐。

可直接从资产读取的默认值继续归资产；EXE 中的纯配置表优先自动提取；已确认的原生条件分支保留在游戏层；场景服务的重复装配继续合并。另写 Lua/JSON 剧情只是换一种语言维护同一套规则，尚无证据能减少原版行为重建的工作量。此次调查没有发现可安全整块删除的选择结局控制器，也没有因此新增一套脚本系统。

未做全剧情、逐帧真实素材或 Switch 验收；CKP/XAN 全覆盖仅指上述文件结构、解码和字段集合，不等于行为效果全部验收。找到的本地 EXE 与固定 oracle 不同，仍不能把历史原指令测试当作本次重新执行的结果。
