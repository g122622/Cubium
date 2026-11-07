# tests/ci-probes

夜间 CI 单测耗时调查用的临时工具（**不是产品构建的一部分**，`scripts/ci/teststart_probe.py --clean` 可整体移除）。

## 目录结构树

```
tests/ci-probes/
├── src/
│   ├── launcher/probe_main.cpp   # 进程启动对照入口：按 PROBE_MODE 链接不同的重型静态库
│   └── teststart/test_main.cpp   # 分阶段装配 tests/unit/main.cpp 的入口（PROBE_LEVEL 0~4）
├── scripts/ci/teststart_probe.py # CI 作业内：链接、运行、汇总 PROBE_* 输出
└── README.md
```

## 内部模块关系

`teststart/test_main.cpp` 直接 `#include` `tests/unit/main.cpp`（后者以 `MC_TESTSTART_EMBED_MAIN` 跳过 `main`），因此复用同一份 `WorldGenRegistryEnvironment` 与 `TestIsolationListener`，两者的阶段时刻与真实测试进程可比。

`teststart_probe.py` 从构建目录的 `CMakeFiles/impl-<Config>.ninja` 解析 `mc_tests` 的链接输入：以测试自身的全部目标文件替换为 `main.cpp.o`（或无，取决于 level），并链接同一组库。因此本工具**不需要在构建期加入 CMake**。

## 上下游外部依赖关系

- 上游：`build/CMakeFiles/impl-<Config>.ninja`（解析链接输入）、`build/tests/unit/main.cpp.o`（编译配置取自 `build/compile_commands.json`）、nightly 的 build 作业产物（`mc_tests` 及其目标文件）。
- 下游：`.github/workflows/nightly.yml` 的 `Test Start Probe` 作业，只消费 `PROBE_*` 输出行。

## 容易踩的坑

1. **必须在与构建作业相同的路径下运行**：`main.cpp.o` 与各静态库里的调试路径、`MC_SOURCE_ROOT` 都是 `/home/runner/work/<repo>/<repo>` 的绝对路径；换路径后需要重新编译 `main.cpp`。
2. **不要用 `gtest_list_tests` 做进程启动基线**：`InitGoogleTest` 看到 `--gtest_list_tests` 会推后各层初始化，测得的启动时间不可与真实用例比较。
3. **进程耗时要在析构里取**：`main` 返回之后还有动态链接器卸载与 sanitizer 收尾，日志/`stdout` 关闭也在那段之后；`PROBE_DONE` 由静态对象析构发出，是"仍在运行"的最后一个可观测点。
4. **对照必须同机同次运行**：GitHub 托管 runner 每次都是新实例，跨运行的绝对耗时不具可比性；所有对照在同一次 CI 作业内背靠背执行。