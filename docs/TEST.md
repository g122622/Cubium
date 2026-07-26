# 测试指南

项目测试基于 **GoogleTest**，通过 **CTest** 编排运行。本文说明如何运行测试、如何对单个用例限时、如何排查崩溃，以及测试体系的组成。

## 快速开始

构建后，所有测试可执行文件位于 `build/bin/RelWithDebInfo/`（多配置生成器，配置名 `RelWithDebInfo`）。

```bash
# 方式 A（推荐）：通过 CTest 运行，支持单用例限时、并行、筛选
cd build
ctest --build-config RelWithDebInfo --output-on-failure -j8

# 方式 B：直接运行可执行文件（不经过 CTest，无单用例限时）
# 不推荐，由于测试用例众多（数万），直接运行大概率会严重超时
./build/bin/RelWithDebInfo/mc_tests
```

> Windows 下必须带 `--build-config RelWithDebInfo`：本项目用 Ninja Multi-Config 多配置生成器，CTest 默认不知道当前配置，需显式指定。Linux/macOS 单配置生成器可省略。

## 测试 target

测试源码位于 `tests/`，共 5 个测试可执行文件，全部注册到 CTest（`tests/CMakeLists.txt` 中 `mc_register_gtests`）：

| target | 范围 | 注册位置 |
|---|---|---|
| `mc_tests` | 主测试套件，覆盖 common/server/client 大部分模块 | `tests/CMakeLists.txt:2401` |
| `mc_resource_tests` | 资源包/纹理/图集相关 | `:2402` |
| `mc_trident_tests` | Trident 渲染引擎核心组件（需 `MC_BUILD_CLIENT=ON`） | `:2517` |
| `mc_command_tests` | 命令系统 | `:2769` |
| `mc_village_tests` | 村庄系统 | `:2850` |

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

数据包路径见 `CLAUDE.md`「重要的外部路径」一节（Windows: `C:\Users\Administrator\minecraft_reborn\datapacks\Vanilla`）。

## 编写新测试

- 测试文件命名 `test_*.cpp` 或 `*Test.cpp`，放在 `tests/` 下对应子目录。
- 新增测试源文件后，需在 `tests/CMakeLists.txt` 的对应 `add_executable` 列表里登记（如 `mc_tests` 列表位于 `tests/CMakeLists.txt:17`）。**未登记的源文件不会编译**。
- 新增测试 target 需自行调用 `mc_register_gtests(<target>)` 注册到 CTest，否则不参与 `ctest` 运行、也无单用例限时。
- 复用被测代码基建时遵循 `docs/PROJECT_CONVENTIONS.md` 与 `docs/CODE_CONVENTIONS.md`。
- 断言使用 `MC_ASSERT_RELEASE` / `MC_ASSERT_RELEASE_MSG`（见 `docs/PROJECT_CONVENTIONS.md`「断言库」），不要手写防御性检查掩盖问题。

## 容易踩的坑

1. **Windows `ctest -N` 看不到用例**：`PRE_TEST` 探测机制所致，正常现象，直接运行即可。
2. **忘记 `--build-config RelWithDebInfo`**：多配置生成器下 CTest 默认空配置，会报 "No tests found"。Linux/macOS 单配置可省略。
3. **新增测试源文件没登记到 `tests/CMakeLists.txt`**：编译期不会报错，但用例不会出现。排查时先 `grep` 确认文件名在对应 target 的源列表里。
4. **想看崩溃栈却拿到 "SEH exception ... 不带栈"**：gtest 抢了 SEH，加 `--gtest_catch_exceptions=0` 交给 CrashHandler。
5. **`--gtest_break_on_failure` 与崩溃调试混用**：它抛的是 `BREAKPOINT`，不是原始崩溃点，定位根因时不要用。
6. **慢用例 hang 看不到失败**：单用例 `TIMEOUT` 默认 300s，超时会判失败并输出；若整体卡住可加 `ctest --timeout` 全局兜底。
