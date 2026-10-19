# 测试与验证

按改动影响选择检查，测试输出写入 `build/` 或独立临时目录。真实素材只读；涉及写档的应用探针使用自建测试目录。平台与打包入口见[构建说明](build.md)。

## 相关单测

以下以存档菜单为例，仅配置和构建相关 CPU 目标：

```sh
cmake -S . -B build -DBK_BUILD_TESTS=ON -DBK_WITH_VULKAN=OFF
cmake --build build --target test-save-menu-control test-save-menu-view --parallel 8
ctest --test-dir build --output-on-failure -R '^save-menu-(control|view)$'
```

目标和 CTest 名称由 [CMakeLists.txt](../CMakeLists.txt) 定义。查找对应 `add_executable` / `add_test`，按实际依赖构建即可；`ctest` 不自动构建缺少的测试程序。

需要检查内存或未定义行为时，使用独立目录：

```sh
cmake -S . -B build/asan -DBK_BUILD_TESTS=ON -DBK_SANITIZE=ON
cmake --build build/asan --target test-save-menu-control test-save-menu-view --parallel 8
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/asan \
  --output-on-failure -R '^save-menu-(control|view)$'
```

该配置检查 ASan/UBSan，关闭了 LeakSanitizer，不将其结果表述为泄漏检查通过。

## 实际资源与 GPU 探针

Vulkan 探针需要真实素材、可用 GPU/驱动，以及相关场景目标。macOS 离屏检查通过 MoltenVK 使用 Metal。

```sh
cmake -S . -B build -DBK_BUILD_TESTS=ON -DBK_WITH_VULKAN=ON
cmake --build build --target save-menu-probe --parallel 8
mkdir -p build/validation/save-menu
build/save-menu-probe local/game/MAINDIR/Data \
  build/validation/save-menu/japanese.rgba
build/save-menu-probe local/game/MAINDIR/Data \
  build/validation/save-menu/chinese.rgba local/game/MAINDIR/patch.pp
```

这个探针走实际存读档界面、临时文件和 GPU；可选第三参数加载真实补丁。它使用 `/tmp/bk-save-menu-*` 测试存档，结束时清理，不使用玩家原有存档。输出是 640×480 RGBA，重绘检查只验证同一准备状态的一致性，不能单独证明字形正确；中文/日文编码另由标签单测检查，画面需查看。

其他资源工具包括 `asset-probe`、`model-probe`、`material-probe`、`scene-probe` 和各 `*-render-probe`。传入数据前先看对应 `tools/` 源文件的 usage 或报告；不同探针可能接收游戏根目录、Data 目录或单个 PP。

如果进程看不到 Metal 设备，GPU 初始化结果不能当作游戏回归失败；确认实际设备访问条件后再判断。CPU 单测和编码核对仍可独立运行。

## 原版指令对照

`tests/original_*_oracle.py` 使用固定 EXE 的隔离 x86 对照，不能换任意安装 EXE。固定摘要、保护壳限制和地址范围见[移植记录](../reports/porting.md)。

部分 Python 检查通过 `tests/model_binding.py` 加载 `libmodel-test.dylib` / `.so`，目录由 `BK3_BUILD_DIR` 选择；动态库通常由对应 `check_*.py` 驱动准备，并非每次 CMake 构建都会生成。测试依赖见 [tests/requirements.txt](../tests/requirements.txt)。缺少 EXE 或素材时，记录未执行范围，不制造通过结果。

## 完整回归入口

```sh
python3 -m venv local/venv
local/venv/bin/pip install -r tests/requirements.txt
./test-host.sh
# 需要真实资源与应用回归时：
./test-host.sh local/game/MAINDIR
```

`test-host.sh` 会构建并运行普通和 ASan/UBSan 的多组检查；提供游戏根目录后继续真实资源/应用检查。`BK3_ORIGINAL_EXE` 选择固定 EXE，`BK3_TEST_PYTHON` 选择测试 Python。这是广泛回归入口，不是每次文档或局部代码编辑的前置步骤，也不等于完整自然剧情遍历。

ASan/UBSan 使用完整默认构建，避免手写目标清单漏掉新注册的 CTest；UBSan 遇错终止，LeakSanitizer 关闭。Python discovery 还使用 `asset-probe`、`pcm-probe` 和两份测试动态库，定向运行时须先构建这些依赖。资源审计拒绝空目录、空归档集合或没有被测条目的输入，不能用零工作量覆盖已有报告。

合成纹理更新、GPU 传输、局部深度清除和共享角色视图探针均已注册为 `vulkan-*` CTest，与其他 GPU 单测一起执行。

## 记录结果

| 证据 | 可以说明什么 |
| --- | --- |
| CPU 单测、独立编码器或原版指令对照 | 被测输入、数值、边界及调用顺序 |
| 实际素材 / GPU 回读 | 指定场景、帧和资源生命周期 |
| 显式入口、库存或阶段夹具 | 夹具之后实际运行的链路 |
| 连续真实输入、保存与独立进程读取 | 记录范围内的自然输入及持久化行为 |
| 平台实机反馈 | 所用二进制、设备和操作条件内的行为 |

报告写清二进制或资源版本、夹具、实际运行范围和限制。旧报告的“尚未接应用”属于历史状态；最新能力看[状态页](roadmap.md)，具体证据从[报告索引](../reports/README.md)进入。
