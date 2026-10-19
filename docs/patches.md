# 汉化、去码与字体编码

运行时默认读取原版日文 Data。可选 `patch.pp` 覆盖资源，不改写原素材；各平台都使用相同补丁格式。

## 启用补丁

把 `patch.pp` 放到包含 `Data/` 的游戏根目录：

| 运行方式 | 默认位置 |
| --- | --- |
| macOS | `~/Library/Application Support/Biko3/game/patch.pp` |
| Linux | `~/.local/share/biko3/game/patch.pp` |
| Switch NRO | `sdmc:/switch/biko3/game/patch.pp` |
| Switch NSP | 安装关联本体的汉化更新包，见 [Switch 说明](switch.md#nsp) |

使用 `BIKO3_GAME` 或自选数据目录时，补丁跟随实际游戏根目录。下次启动时移除补丁可恢复原版资源。NSP 已装更新包时，更新内置的补丁属于该更新内容，不能靠删除 NRO 目录中的文件关闭。

当前生成器输出 BKPT2（`patch.cfg` 中版本为 2），资源逐项压缩；运行时仍支持旧 BKPT1。旧运行时未必支持 BKPT2，更新外挂时应配套更新程序。

| 标志 | 含义 |
| --- | --- |
| `BK_PATCH_CHINESE`，flags 位 1 | 中文资源、中文字库和中文操作说明 |
| `BK_PATCH_UNCENSORED`，flags 位 2 | 去码资源及对应原生视频策略 |
| flags = 3 | 同时启用以上两项 |

## 从中文绿色版生成

输入为自有的原始日文 Data 与已预打 3DH 汉化的中文 Data。生成器按资源比较，仅收录翻译内容，跳过旧设置、启动器和存档差异；两个输入目录均只读。

```sh
python3 tools/prepare_chinese_patch.py \
  local/game/MAINDIR \
  '尾行3绿色免安装简体中文版' \
  dist/chinese-patch/patch.pp
```

`--font /path/to/Type_S.FTT` 可选，用于替换中文位图字库。使用 Noto Sans CJK 等字体生成 FTT 的方法与具体样本摘要见[中文补丁报告](../reports/chinese-patch.md)。该字体来源有自己的许可，分发时一并保留。

合并去码时指定原 `Patch.3dh`：

```sh
python3 tools/prepare_chinese_patch.py \
  local/game/MAINDIR \
  '尾行3绿色免安装简体中文版' \
  dist/chinese-patch/patch-cn-uncensored.pp \
  --uncensored '路径/Bk3msc_updatex_05_02_02/Patch.3dh'
```

输出文件放入游戏目录时命名为 `patch.pp`。去码参数合并已识别的资源操作，Windows EXE 写入不会直接应用到原生二进制；行为依据见报告。

## 从早期汉化与去码补丁生成

保留此前交付使用的生成入口：

```sh
python3 tools/prepare_combined_patch.py \
  '尾行3 [汉化]/去码汉化补丁' local/game/MAINDIR \
  local/patch-speed/patch.pp
```

此入口与绿色版生成器的输入不同。资源数量、字体来源和输出摘要按对应报告区分，不能混用样本数字。[合并记录](../reports/combined-patch.md)与[读取优化](../reports/patch-loading.md)保存早期交付证据。

## 编码与 FTT 字库

FTT 按两个输入字节直接索引字形，不在绘制时自动转换 Unicode。日文使用 Shift-JIS/CP932，中文翻译使用 GBK；两种编码的码位会重叠。合并字体保留未覆盖的日文字形，不能在同一码位同时存放两种不同字符。

因此程序生成的标签也必须选择匹配编码：汉化时使用 GBK，原版时使用 Shift-JIS。`app/save_preview` 从 `BK_PATCH_CHINESE` 取得语言，交给 `scene/save_menu_labels` 生成数字、日期、标点及「场景号」/「エリア」。仅替换字体无法修正编码错配。[存档乱码修复](../reports/save-session.md#2026-10-19汉化存档槽文字乱码)记录了具体复现和验证。

中文剧情和世界提示还启用独立的字号与裁切适配，日文保持其原规则，见[中文文字报告](../reports/chinese-dialogue.md)。排查乱码时先区分图片资源、脚本文字和程序标签，再检查补丁标志与字库，避免把显示问题当作存档损坏。
