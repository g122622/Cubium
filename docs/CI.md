## CI 与 nightly 构建

Cubium 的持续集成由 `.github/workflows/nightly.yml` 承担，**每晚 21:00（北京时间）自动运行一次**，
不随每次提交触发。目标有三：

1. 每晚跑一次完整测试（单元 / 集成 / e2e / fuzz / benchmark）；
2. 每晚产出可部署的服务端二进制（GitHub Release）；
3. 把失败结果归档为 artifact 并自动建一个 issue，供人工与 AI 助手排查。

> 历史：项目曾有一份 `ci.yml`（push/PR 触发，含 Windows/Linux 构建与 ASan/TSan/栈保护等 job）
> 与一份 `self-heal.yml`（CI 失败时用 GitHub Models 分析根因并派发给 @copilot）。`ci.yml` 的 5 个
> 构建/测试 job 于 2026-06 起被 `if: false` 静态关闭，只剩格式检查在跑，导致 self-heal 因「CI 永不
> 失败」而同步休眠。二者均已删除，由本工作流取代。

### 为什么只跑 Linux

客户端已停止维护，日常运行以服务端为主；Linux 是项目的 CI 目标平台，Windows/macOS 构建已移除。
这同时消除了 Windows 上 fuzz 工具链的两处历史包袱（libFuzzer 运行时的 /MT CRT 冲突、Sanitizer
运行时破坏 C++ 异常处理，见 [test/FUZZING.md](test/FUZZING.md) §7.7 / §8.1）。

### 为什么拆成多个 job

GitHub Actions 单个 job 有 **6 小时硬上限**。本项目百万行 C++ 的构建耗时长、单元测试用例数以万计、
fuzz 约 45 分钟，串行放在一个 job 里必然超时。因此拆为「1 个构建 job + 5 个测试 job + 1 个汇总 job」，
测试 job 从构建 job 的 artifact 取二进制，不各自重复构建。

### Job 拓扑

```
build ─┬─> unit-tests
       ├─> integrated-tests
       ├─> e2e-tests
       └─> benchmark
fuzz ──（独立，自行构建）

以上全部 ──> report（if: always()，汇总 + 建 issue）
```

| Job | 内容 | 是否阻塞 |
|---|---|---|
| `build` | `linux-relwithdebinfo` 构建，上传 artifact，建 nightly Release | 是 |
| `unit-tests` | `ctest` 全量单元测试 | **否**（仅归档） |
| `integrated-tests` | `scripts/test/run-gametests.ts`（含失败隔离重跑） | **否**（仅归档） |
| `e2e-tests` | `tests/e2e/bot` 的 `regress` 模式（仅 Cubium，不与 vanilla 双跑） | 是 |
| `fuzz` | 9 个 libFuzzer 目标各 `-max_total_time=300` | 是（崩溃/OOM 即失败） |
| `benchmark` | `mc_benchmark`，与上一次 nightly 对比 | **否**（仅记录 + 告警） |
| `report` | 汇总各 job 结果，有失败则建 issue | — |

**为什么单元/集成测试不阻塞**：项目存在大量历史遗留失败（`CLAUDE.md` 亦明确指出「很多测试错误是
以前留下的」）。把它们设成硬门禁会让 nightly 长期红灯、失去信号价值。因此这两类**全量跑、如实记录、
不 fail**，失败清单由 `report` job 汇总进 issue。

**为什么 benchmark 不阻塞**：nightly 跑在 GitHub 托管的 `ubuntu-latest` 上，**每次都是全新的机器
实例**，性能绝对值含实例间噪声。单晚差异不足以判定回归，故只输出对比报告并对超阈值项 `::warning::`。

### 触发

```bash
# 定时（无需操作，每晚自动运行）
# 手动触发（首次调试或按需重跑）
gh workflow run nightly.yml
```

cron 为 `0 13 * * *`（UTC）—— GitHub 的 cron 一律按 UTC 解释，13:00 UTC 即北京时间 21:00。

### 数据包依赖

Cubium 的世界生成 **100% 数据驱动、无硬编码兜底**：`StandaloneServer` 扫描
`GameDirectory::dataPacksDir()`（`~/minecraft_reborn/datapacks/`），列表为空时经
`DataPackRepository::ensureVanillaBuiltinPack` 注入 `~/minecraft_reborn/datapacks/Vanilla`；
单元测试的 `WorldGenRegistryEnvironment`（`tests/unit/main.cpp`）读同一目录。数据包缺失时各
worldgen loader 会加载 0 条目，`RandomState::create` 断言失败。

因此所有测试类 job 都先经 `.github/actions/setup-datapack`（composite action）安装数据包：

```
https://github.com/misode/mcmeta/archive/refs/tags/1.21.11-data.zip
```

> 注意：`misode/mcmeta` 的 `1.21.11-data` 是**标签**而非 Release（GitHub API 对该 tag 的
> releases 端点返回 404），只能用标签归档 zip。解压后 `mcmeta-1.21.11-data/` 的结构与本地
> `datapacks/Vanilla/` 完全一致（`data/minecraft/` + `pack.mcmeta` + `version.json`），
> 重命名为 `Vanilla` 即可。

本地复现 nightly 时，需自行准备同一份数据包（放到 `~/minecraft_reborn/datapacks/Vanilla`）。

### 性能基准的对比口径

`benchmark` job 用 `gh run list` 找到最近一次**成功**的 nightly 运行（排除本次），下载其
`benchmark-results` artifact，再由 `scripts/ci/compare_benchmark.py` 逐用例比对：

- **CPU**：`real_time` 归一化到纳秒后比较，阈值默认 10%；
- **内存**：`MemoryProfiler` 注入的计数器（`num_allocs` / `max_bytes_used` 等），阈值默认 20%。

超阈值的项在报告中列出并触发 `::warning::`，但**不 fail**。判定真实回归应结合连续多晚的趋势，
而非单晚的单点差异——托管 runner 的实例差异本身就足以造成超过阈值的抖动。

### 失败归档与 issue

- 各 job 的结果（日志、JUnit XML、benchmark 报告、fuzz 崩溃产物）都作为 artifact 上传，
  benchmark 结果保留 90 天，其余 30 天。
- `report` job 用 `scripts/ci/summarize_failures.py` 汇总为一个 Markdown 报告，
  同时写入 job summary（可在运行页直接查看）并作为 artifact 保留。
- 报告分别列出**失败和跳过**的单元、集成、e2e 用例，并保留原因。单元测试同时显示通过、
  失败、跳过数量；job 的执行成功不能解释为所有用例通过。集成测试清单含各轮重跑记录。
- `nightly-report.md` 用于 issue 正文，限制条数和长度；`nightly-report-full.md` 随
  `nightly-report` artifact 归档，**失败和跳过清单不截断**。跳过不计为通过或失败。
- JUnit 采用流式 XML 解析，日志完整读取，不限制输入长度。报告缺失、损坏或没有单测用例时
  按报告错误上报，不能判为 `clean`；日志兜底也识别段错误、超时和未运行用例。
- **是否需要建 issue 的判定集中在 `summarize_failures.py` 的 `has_failures()`**，它综合两类来源：
  1. job 层面的失败（build / e2e / fuzz / benchmark 任一非 success）；
  2. 非阻塞 job（unit-tests / integrated-tests）产物中的失败用例——这两个 job 内部吞掉了失败码、
     恒为 success，必须从产物（`ctest-results.xml`、gametest 的 JUnit XML、e2e 日志、fuzz 产物）里查。
- 判定结果经 `nightly-status.txt` 传出，`report` job 据此决定是否建 issue（标签 `nightly-ci`，
  标题 `Nightly CI 失败 (YYYY-MM-DD)`），正文即上述报告。**不派发给任何人**。
- 去重：同一 run 只建一个 issue。

### 本地复现各 job

```bash
# 构建（对应 build job）
./scripts/configure.sh build

# 单元测试（对应 unit-tests job；必须经 ctest 以发挥并行能力并启用单用例限时）
cd build && ctest --build-config RelWithDebInfo --output-on-failure -j8

# 集成测试（对应 integrated-tests job）
node scripts/test/run-gametests.ts

# e2e（对应 e2e-tests job）
cd tests/e2e/bot && npm ci && node run.ts

# fuzz（对应 fuzz job）
cmake --preset linux-clang-fuzz
cmake --build --preset linux-clang-fuzz -j$(nproc)
build-fuzz/bin/fuzz/RelWithDebInfo/fuzz_java_codec_sb \
  -max_total_time=300 tests/fuzz/corpus/fuzz_java_codec_sb

# benchmark（对应 benchmark job；须在仓库根目录运行）
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_repetitions=3
python3 scripts/ci/compare_benchmark.py \
  --current benchmark_results/<本次时间戳>/results.json \
  --previous <上次的 results.json>
```

### 相关脚本

| 文件 | 职责 |
|---|---|
| `scripts/ci/compare_benchmark.py` | benchmark 跨日对比，生成 Markdown 报告（仅记录 + 告警） |
| `scripts/ci/summarize_failures.py` | 汇总各测试结果，生成 issue 正文（永不抛异常、输出限长） |
| `.github/actions/setup-linux-build-env/action.yml` | 装系统依赖 + 恢复 ccache/vcpkg 缓存 + 安装 vcpkg（build / fuzz 共用） |
| `.github/actions/setup-datapack/action.yml` | 安装原版数据包到 `~/minecraft_reborn/datapacks/Vanilla` |

### 平台适配注意

`tests/e2e/bot/src/config.ts` 与 `scripts/test/run-gametests.ts` 中的服务端二进制路径按
`process.platform` 补后缀（Windows 为 `minecraft-server.exe`，Linux 为 `minecraft-server`）。
`benchmark/cases/ServerInitializeBenchmark.cpp` 同样已有 `#ifdef _WIN32` 分支。新增任何以
硬编码路径定位二进制产物（或按路径判平台）的脚本时，务必沿用该平台感知写法。

### 容易踩的坑

1. **cron 按 UTC 解释**：写 `0 21 * * *` 会得到北京时间凌晨 5 点。本项目用 `0 13 * * *`。
2. **托管 runner 的机器实例每次都不同**：benchmark 的绝对值不可跨夜直接比较「是否回归」，
   只看趋势与幅度。
3. **测试 job 与 build job 的 workspace 路径必须一致**：CTest 元数据里存的是绝对路径
   （`/home/runner/work/<repo>/<repo>`）。标准托管 runner 上两者一致，故可直接解压复用；
   若改用 self-hosted runner 且路径不同，unit-tests job 需改为在本 job 内重新 configure。
4. **vcpkg 依赖可能是动态库**：build job 会把 `installed/x64-linux/lib` 一并打进 artifact，
   测试 job 通过 `LD_LIBRARY_PATH` 引用。若某次构建链接方式改变（改静态链接），这段可以删掉。
5. **`report` job 必须 `if: always()`**：否则任一前置 job 失败时它不会运行，连 issue 都建不出来。
6. **行为包的 TS→JS 产物不入 git**：`tests/integrated/*/scripts/` 在 `.gitignore` 中，
   由 `build.mjs` 在构建期生成。因此任何**不构建**却要跑集成测试或 e2e 的 job，都必须先
   `node build.mjs` 自行编译一次（`build.mjs` 会在 `node_modules` 缺失时自动 `npm install`）。
7. **CTest 元数据是构建期产物**：`CTestTestfile.cmake` 与 gtest 的发现文件不在 `bin/` 下，
   故 build job 单独打包为 `ctest-meta.tar.gz` 供 unit-tests job 复用。**这些文件内含绝对路径**，
   两个 job 的 workspace 路径必须一致才能直接解压复用。
8. **fuzz 的语料目录不能传 `tests/fuzz/corpus` 根**：libFuzzer 会把新语料条目以 sha1 命名
   写进传入的目录，传仓库根语料目录会让工作区变脏。无专属语料的 target 用临时目录起跑。
9. **Linux 系统依赖不可只装编译器与 ninja**：vcpkg 的 `glfw3` port 依赖 X11 / Vulkan / Wayland
   开发包，缺失时 configure 会以 `vcpkg install failed` + glfw3 portfile 报错栈的形式失败，
   真实原因（`Could NOT find X11`）藏在 vcpkg 内部日志里，极难定位。完整清单见
   `.github/actions/setup-linux-build-env/action.yml`。
10. **clang 必须是 22，不能用 runner 预装的 18**：`cmake/CompilerWarnings.cmake` 使用了
    `-Wno-c2y-extensions`（Clang 20+ 引入，用于压制 `__COUNTER__` 在 C2y 下被 `-pedantic`
    判为扩展的警告）。clang 18 不认识该选项，配合 `-Werror` 会直接编译失败于
    `error: unknown warning option '-Wno-c2y-extensions'`，而该报错指向具体源文件、
    不会提示「编译器太老」。`setup-linux-build-env` 经 apt.llvm.org 的 llvm.sh 安装
    clang 22 并把 `/usr/lib/llvm-22/bin` 前置到 `PATH`，同时在 Verify toolchain 步骤里
    显式校验主版本 ≥ 20 提前暴露根因。
11. **CTest 用例清单的生成时机随平台不同**：Linux 走 `gtest_discover_tests` 的 POST_BUILD
    模式，清单是 `build/tests/**/*_tests-<Config>.cmake`（数千行 `add_test`）；
    Windows 才是 `DISCOVERY_MODE PRE_TEST`。打包元数据时必须把 `*_tests*.cmake` 一并带上，
    否则 `ctest` 会报「No tests were found」。
12. **composite action 内不能用 `env` 上下文**：`actions/cache` 的 key 里若需要 `VCPKG_COMMIT`，
    必须作为 `inputs` 传入（见 `setup-linux-build-env`），直接在 composite 里写 `env.X` 会解析为空。
13. **限制输出，不能截断输入**：全量单测 XML 可超过 35 MB，CTest 日志超过 7 MB。
    失败记录通常位于后段；只读前 400 万字符会同时漏掉 XML 失败项和日志末尾失败清单，
    导致带失败的运行被判为 `clean`。失败与跳过必须完整解析后再限制 issue 展示长度。
