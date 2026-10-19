# 纯中文 BKPT2 外挂（基于 3DH 绿色版）

2026-10-19：新增 `tools/prepare_chinese_patch.py`，从**原始日文 Data**与**已预打 3DH 汉化的绿色版 Data**直接生成标准 `patch.pp`。不运行 Windows EXE、不改写输入目录，也不把 3DH 的启动器、注册表、旧配置或存档带进外挂。

## 生成结果

```sh
python3 tools/prepare_chinese_patch.py \
  'work/installed/MAINDIR' \
  '尾行3绿色免安装简体中文版' \
  'dist/chinese-patch/patch.pp'
```

本次本地样本：

- 格式：inline PP 容器内的 BKPT2
- `patch.cfg`：`BKPT`、版本 2、flags **1**（仅中文；不含去码）
- 资源数：**234**
- 文件大小：**15,791,467 字节**
- SHA256：`3104970755de6c49470c07c8b8b8adb19791b25f7fd71b3f2c9e88413e9d3ea3`

资源分布：

| pack | 项数 | 最终载荷字节 |
| --- | ---: | ---: |
| `bk3_00` | 201 | 40,927,432 |
| `bk3_01` | 1 | 786,488 |
| `bk3_05` | 27 | 162,776 |
| `bk3_08` | 1 | 786,488 |
| `bk3_14` | 1 | 786,488 |
| `bk3_15` | 1 | 12,344 |
| `fonts` | 2 | 9,916,496 |

`bk3_01`/`bk3_08`/`bk3_14` 共用同一个 `f04_04.bmp` 替换；`fonts` 为 `Type_S.FTT`/`Type_G.FTT` 的日文基底 + 3DH 中文字形合并结果。`setting.cfg` 与 `bk3_Red.b3f` 的中文绿色版差异不属于翻译资源，已显式跳过并在 JSON 中列为 `ignored_non_localization_files`。

## 验证

- 生成器的 Python 校验逐项解码 234 条记录，与中文 Data 的对应资源逐字节一致；字体与 `font_overlay` 结果一致。
- 单独编译的 `bk_font_decode`/`bk_font_glyph` 探针：合并字库 `cell=32`，中文码 `B5C3/B5BD/C0FB/C4F2/BCC1/D1DB/BEB5` 均有字模，日文 `8148` 仍保留。
- 用生产版 `biko3-preview` 以原始 Data + 生成 `patch.pp` 启动标题：中文标题/菜单正常，左侧操作说明随 flags=1 切为中文；无补丁解析错误。
- 1200 帧 replay 冒烟运行完成，无资源/字体错误；该冒烟只到达标题/初始 flow01，**未**完成全部中文剧情路线审核。

## 使用

将 `patch.pp` 放在游戏数据根目录（与 `Data/` 同级）：

- macOS：`~/Library/Application Support/Biko3/game/patch.pp`
- Switch：`switch/biko3/game/patch.pp`
- Linux：`~/.local/share/biko3/game/patch.pp`

移除 `patch.pp` 即恢复日文。该外挂只包含中文资源；需要“汉化 + 去码”时使用下节的 `--uncensored` 生成 `patch-cn-uncensored.pp`。

## 限制

- 译文和图片来自 2004 年 3DH 中文绿色版，翻译质量与 3DH 水印仍随源数据保留；本工具只负责整理为 runtime 标准外挂，不重做翻译。
- 纯中文版未包含去码，因此 flags=1；需要去码时使用下节的 `--uncensored` 版本（flags=3）。
- `dist/` 下的 `patch.pp` 是本地生成物，不进入源码 Git；补丁素材及原补丁署名/授权仍按本地交付处理。

## 汉化 + 去码

在纯中文外挂的基础上加 `--uncensored`，指向 3DH 去马赛克包内的 `Patch.3dh`：

```sh
python3 tools/prepare_chinese_patch.py \
  'work/installed/MAINDIR' \
  '尾行3绿色免安装简体中文版' \
  'dist/chinese-patch/patch-cn-uncensored.pp' \
  --uncensored 'research/uncensor/Bk3msc_updatex_05_02_02/Patch.3dh'
```

生成结果：

- 格式：inline PP 内的 BKPT2
- `patch.cfg`：`BKPT`、版本 2、flags **3**（中文 + 去码）
- 资源数：**328**（234 项中文 + 94 项去码目标；去码操作 341 项）
- 文件大小：**33,556,071 字节**
- SHA256：`9a4a3819d5b56372b7e2e59a1d750b2b54b307e2bb7c99cc24f0cb4fd94bb670`
- 3DH 去码源 SHA256：`c6cd21d57f81bf6f121534b99dd7aa3f4d47f5850f0b9711250c92fbd779bdf2`
- 21 项原版 EXE 写入不进入 native 二进制；运行时仅在 flags=3 时省去对应视频/去码路径。

验证：

- 生成器逐项解码 328 条记录，与“中文 Data + 去码操作”的构建结果逐字节一致；
- 生产版 `biko3-preview` 以原始 Data + `patch-cn-uncensored.pp` 启动标题正常；
- `--scene office` 静态办公室成功加载 65 帧、93 instances、62 GPU parts，说明部分去码模型修改可被生产资源层应用；
- 1200 帧 replay 冒烟运行完成，无资源/字体/补丁错误；
- 完整剧情与去码实际画面仍需在 macOS/Switch 上做人工回归，本文不把冒烟当完整验收。
