# 单元测试指南

项目的单元测试基于 **GoogleTest**，通过 **CTest** 编排运行。本文说明如何运行测试、如何对单个用例限时、如何排查崩溃，以及测试体系的组成。

## 快速开始

构建后，所有测试可执行文件位于 `build/bin/RelWithDebInfo/`（多配置生成器，配置名 `RelWithDebInfo`）。

```bash
# 方式 A（推荐）：通过 CTest 运行，支持单用例限时、并行、筛选。强烈推荐使用ctest进行测试以便发挥cpu的并行能力。
cd build
ctest --build-config RelWithDebInfo --output-on-failure -j8

# 方式 B：直接运行可执行文件（不经过 CTest，无单用例限时）
# 不推荐，由于测试用例众多（数万），直接运行大概率会严重超时
./build/bin/RelWithDebInfo/mc_tests
```

> Windows 下必须带 `--build-config RelWithDebInfo`：本项目用 Ninja Multi-Config 多配置生成器，CTest 默认不知道当前配置，需显式指定。Linux/macOS 单配置生成器可省略。

## 测试 target

测试源码位于 `tests/`，共 2 个测试可执行文件，全部注册到 CTest（`tests/CMakeLists.txt` 中 `mc_register_gtests`）：

| target | 范围 | 注册位置 |
|---|---|---|
| `mc_tests` | 主测试套件，覆盖 common/server/client 大部分模块（含命令系统、村庄系统、资源包/纹理/图集等原独立 target 的全部用例） | `tests/CMakeLists.txt:2435` |
| `mc_trident_tests` | Trident 渲染引擎核心组件（需 `MC_BUILD_CLIENT=ON`） | `tests/CMakeLists.txt:2430` |

> 历史：`mc_command_tests`/`mc_village_tests`/`mc_resource_tests` 三个独立 target 已合并入 `mc_tests`，以减少构建产物数量。合并后命令/村庄测试随之带上 Vulkan/OpenAL/asio 依赖（mc_tests 现链这些）——纯服务端 CI、无显卡环境将无法运行这些用例。

构建开关 `MC_BUILD_TESTS`（默认 `ON`，根 `CMakeLists.txt:30`）控制是否构建测试。

## CTest 常用命令

```bash
cd build

# 全量运行
ctest --build-config RelWithDebInfo --output-on-failure -j8

# 按用例名筛选（正则匹配 TestSuite.TestCase）
ctest --build-config RelWithDebInfo -R 'ServerChunkManagerTest' --output-on-failure
ctest --build-config RelWithDebInfo -R 'GetChunkSync_MultipleChunks' -V   # -V 详细输出

# 排除某些用例
ctest --build-config RelWithDebInfo -E 'WorldGen|EndDragon' --output-on-failure -j8

# 只跑单个 target 的所有用例（用 -R 匹配 target 名前缀不可靠，建议按用例名筛）

# 临时覆盖超时（仅本次运行生效，不持久化）
ctest --build-config RelWithDebInfo --timeout 60 -R '某慢用例'

# 重跑上次失败的用例
ctest --build-config RelWithDebInfo --rerun-failed --output-on-failure
```

### Windows 下 `ctest -N` 列不出用例是正常的

Windows 采用 `DISCOVERY_MODE PRE_TEST`（见下文），用例列表在 CTest **实际运行时**才探测生成，预览（`ctest -N`）阶段看不到。直接去掉 `-N` 运行即可。

## 跨测试隔离模型

测试有两种运行形态，隔离强度与代价不同，需按场景选择：

| 形态 | 命令 | 隔离强度 | 代价 | 适用 |
|---|---|---|---|---|
| **CTest per-case** | `ctest -R '...'` | 强：每用例独立进程，物理隔离全部全局状态 | 慢：每进程重载原版数据包注册表（`WorldGenRegistryEnvironment::SetUp`） | 单用例/小批定位、CI |
| **全二进制直跑** | `mc_tests --gtest_filter='...'` | 弱：单进程内顺序跑，共享进程级全局状态 | 快：注册表只载一次 | 大批量本地回归、验顺序敏感性 |

全二进制直跑更快，但单进程内跨用例共享状态可能互相污染（典型表现：某用例隔离跑稳定、全二进制偶发 flaky）。已知的污染源与治理：

- **生产时序**：`ServerChunkManager::processTicketUpdatesSync()` 现已出队 `m_pendingLoadCompletes`（对齐 MC Java `ServerChunkCache.runDistanceManagerUpdates`），消除了“票据已推进但存档完成回调未出队”的 TOCTOU 窗口——这是 `GameEventServerTest` 全二进制 flaky 的根因。
- **thread_local 调度上下文**：`tests/main.cpp` 注册了 `TestIsolationListener`，每用例结束重置 `ChunkTaskScheduler` 的 thread_local `SyncSchedulingContext`（depth/pending），根除同 worker 线程跨用例的残留。

经验上**不是污染源**、无需重置的进程级状态：`VanillaBlocks`/`BlockTags`/`BlockRegistry`/`Items` 等原版基线注册表——它们幂等初始化，每个用例都期望以相同方式加载，重置反而要重解析数据包（全量 per-case 跑 ×27000 用例不可接受）。`::testing::Environment::TearDown` 在进程末尾才跑一次，对进程内隔离无帮助，故 `WorldGenRegistryEnvironment` 不设 TearDown。

### 顺序敏感性回归

验证某用例不受顺序影响（flaky 根治后必跑）：

```bash
# 重复 20 次（捕偶发）
./build/bin/RelWithDebInfo/mc_tests --gtest_filter='GameEventServerTest.*' --gtest_repeat=20

# 打乱顺序重复 10 次（捕顺序依赖）
./build/bin/RelWithDebInfo/mc_tests --gtest_filter='GameEventServerTest.*' --gtest_shuffle --gtest_repeat=10
```

`--gtest_shuffle` 默认以当前时间为种子；复现某次 shuffle 顺序可用 `--gtest_random_seed=N`（N 见上次输出末尾）。

## 单用例限时机制

每个 gtest 用例（`TestSuite.TestCase`）都被 `gtest_discover_tests` 拆成**独立的 CTest 条目**，并附带独立的 `TIMEOUT`。超时即判失败，用于及早暴露区块生成/光照等长耗时用例的 hang/flake。

核心配置在 `tests/CMakeLists.txt:2378-2399`：

```cmake
# 单个测试用例执行超时（秒）
set(MC_TEST_TIMEOUT 300 CACHE STRING "单个 gtest 用例的 ctest 超时秒数")

function(mc_register_gtests targetName)
    if(WIN32)
        # Windows + clang + 多配置生成器：构建期 DLL 未部署会导致探测失败，
        # 改用 PRE_TEST 在 CTest 运行期才探测（此时 DLL 已就位），
        # DL_PATHS 把目标目录加入 DLL 搜索路径。
        gtest_discover_tests(${targetName}
            DISCOVERY_MODE PRE_TEST
            DL_PATHS "$<TARGET_FILE_DIR:${targetName}>"
            PROPERTIES TIMEOUT ${MC_TEST_TIMEOUT}
        )
    else()
        gtest_discover_tests(${targetName}
            PROPERTIES TIMEOUT ${MC_TEST_TIMEOUT}
        )
    endif()
endfunction()
```

### 调整超时

`MC_TEST_TIMEOUT` 是 CACHE 变量，默认 300 秒（5 分钟）。两种改法：

```bash
# 1. 持久化：重新 configure（之后所有用例都用新值）
cmake -B build -DMC_TEST_TIMEOUT=120 <其余 configure 参数>

# 2. 临时：仅本次运行覆盖（不修改缓存）
ctest --build-config RelWithDebInfo --timeout 60 -j8
```

CI 侧另有全局兜底：`.github/workflows/ci.yml` 中 Linux/asan/tsan job 用 `ctest --output-on-failure --timeout 300 -j$(nproc)`，`--timeout` 是 CTest 进程级兜底，与单用例 `TIMEOUT` 取较小者生效。

## 测试崩溃时查看完整调用栈

`mc_tests` 在 `main` 中安装了 `mc::assert::CrashHandler`（参考 `src/client/main.cpp`、`src/server/main.cpp`），崩溃（SEH 访问违例、除零、栈溢出、纯虚调用、`std::terminate`、`MC_ASSERT_RELEASE` 触发的 `abort`）时输出调用栈和寄存器到 stderr。

但 GoogleTest 默认安装自己的 SEH 处理器（`--gtest_catch_exceptions=1`），会抢先捕获 SEH 并打印 `unknown file: error: SEH exception with code 0x... thrown in the test body.` **不带栈**，CrashHandler 拿不到崩溃现场。要看到完整栈，必须禁用 gtest 的 SEH 捕获，让 CrashHandler 接管：

```bash
# 禁用 gtest SEH 捕获，崩溃时由 CrashHandler 输出完整调用栈
./build/bin/RelWithDebInfo/mc_tests --gtest_filter='ServerChunkManagerTest.GetChunkSync_MultipleChunks' --gtest_catch_exceptions=0
```

- `--gtest_catch_exceptions=0`：关闭 gtest 的 SEH 捕获，SEH 直接传给 CrashHandler。
- 崩溃时输出 `Reason: ACCESS_VIOLATION - Read/Write access at address 0x...`、寄存器转储、`Stack trace:`（帧序号 + 函数名 + 文件:行号）。
- 仅 Windows 需要 `--gtest_catch_exceptions=0`（Linux/macOS 用信号处理，gtest 默认不捕获 SIGSEGV）。
- 配合 `--gtest_filter` 定位到单个用例，栈最干净。`--gtest_break_on_failure` 会让 gtest 抛 `BREAKPOINT`（被 CrashHandler 当作断点，不是原始崩溃点），调试时不要混用。

> 注意：通过 CTest 运行时，gtest 默认仍带 `--gtest_catch_exceptions=1`。需要看栈时请直接运行可执行文件并加 `--gtest_catch_exceptions=0`，或给 CTest 传参：`ctest --build-config RelWithDebInfo -R '用例名' --output-on-failure --gtest_catch_exceptions=0`（CTest 会把额外参数透传给测试命令）。

## 测试入口与全局环境

`tests/main.cpp` 是 `mc_tests` 的入口：安装 `CrashHandler` 后运行所有用例，并注册全局 `WorldGenRegistryEnvironment`——在所有用例运行前一次性从原版数据包加载 `noise_settings` / `density_function` / `noise` / `flat_preset` / `world_preset` 等数据驱动注册表。任何调用 `RandomState::create()` 的测试都依赖这些注册表已加载。数据包目录缺失时（非开发机）静默跳过，相关测试会因 registry 为空而断言失败（属预期）。

> 合并前 `mc_command_tests` 曾用独立的 `tests/command_main.cpp` 入口（与 `main.cpp` 等价地安装 CrashHandler + 注册 `WorldGenRegistryEnvironment`）；该 target 并入 `mc_tests` 后 `command_main.cpp` 已删除，命令测试改用 `main.cpp` 的同一全局环境。`mc_village_tests`/`mc_resource_tests` 原本用 gtest 默认 main，并入后同样由 `main.cpp` 接管。

数据包路径见 `CLAUDE.md`「重要的外部路径」一节（Windows: `C:\Users\Administrator\minecraft_reborn\datapacks\Vanilla`）。

## 编写新测试

- 测试文件命名 `test_*.cpp` 或 `*Test.cpp`，放在 `tests/` 下对应子目录。
- 新增测试源文件后，需在 `tests/CMakeLists.txt` 的对应 `add_executable` 列表里登记（如 `mc_tests` 列表位于 `tests/CMakeLists.txt:17`）。**未登记的源文件不会编译**。绝大多数新测试应加入 `mc_tests`；仅当被测代码有特殊依赖隔离需求（如 `mc_trident_tests` 仅在 `MC_BUILD_CLIENT=ON` 下构建）时才新建 target。
- 新增测试 target 需自行调用 `mc_register_gtests(<target>)` 注册到 CTest，否则不参与 `ctest` 运行、也无单用例限时。
- **TestSuite 名在 target 内必须唯一**：合并后 `mc_tests` 单进程内运行全部用例，若两个文件注册同名 TestSuite（`TEST(SuiteName, ...)`/`TEST_F(SuiteName, ...)` 的第一个参数），gtest 会重复注册报错。新增测试前先 `grep -rE 'TEST(_F)?\(\s*YourSuiteName' tests/` 确认无重名。
- 复用被测代码基建时遵循 `docs/PROJECT_CONVENTIONS.md` 与 `docs/CODE_CONVENTIONS.md`。
- 断言使用 `MC_ASSERT_RELEASE` / `MC_ASSERT_RELEASE_MSG`（见 `docs/PROJECT_CONVENTIONS.md`「断言库」），不要手写防御性检查掩盖问题。

## 容易踩的坑

1. **Windows `ctest -N` 看不到用例**：`PRE_TEST` 探测机制所致，正常现象，直接运行即可。
2. **忘记 `--build-config RelWithDebInfo`**：多配置生成器下 CTest 默认空配置，会报 "No tests found"。Linux/macOS 单配置可省略。
3. **新增测试源文件没登记到 `tests/CMakeLists.txt`**：编译期不会报错，但用例不会出现。排查时先 `grep` 确认文件名在对应 target 的源列表里。
4. **想看崩溃栈却拿到 "SEH exception ... 不带栈"**：gtest 抢了 SEH，加 `--gtest_catch_exceptions=0` 交给 CrashHandler。
5. **`--gtest_break_on_failure` 与崩溃调试混用**：它抛的是 `BREAKPOINT`，不是原始崩溃点，定位根因时不要用。
6. **慢用例 hang 看不到失败**：单用例 `TIMEOUT` 默认 300s，超时会判失败并输出；若整体卡住可加 `ctest --timeout` 全局兜底。
7. **用例隔离跑通过、全二进制偶发 flaky**：典型跨用例共享状态污染。先用 `--gtest_repeat=20`/`--gtest_shuffle` 复现，再排查 thread_local 残留或生产时序窗口（见「跨测试隔离模型」）。已知污染源已治理，新增疑似污染源时优先补 `TestIsolationListener` 的 `OnTestEnd` 清理而非给原版基线注册表加重置。
