# Benchmark 框架（google/benchmark）

本目录包含 Cubium 项目的基准测试框架，基于 [google/benchmark](https://github.com/google/benchmark)（经 vcpkg 引入，目标名 `mc_benchmark`）。

## 目录结构

```
benchmark/
├── main.cpp                          # 入口：benchmark::Initialize/RunSpecifiedBenchmarks + 时间戳归档目录
├── MemoryProfiler.hpp/.cpp           # MemoryManager 实现（全局 operator new/delete 钩子统计内存指标）
├── PerfettoProfilerAdapter.hpp/.cpp  # ProfilerManager 适配器（每用例一个 .perfetto-trace）
├── CMakeLists.txt                    # mc_benchmark 目标（源闭包参照 mc_tests 的登记方式）
└── cases/
    ├── ChunkGenerationBenchmark.cpp  # 区块生成吞吐（生产级生成系统，线程数×批量多档）
    ├── LightingBenchmark.cpp         # 光照引擎本体（16×16 整层批量方块光更新）
    └── ServerInitializeBenchmark.cpp # 服务端启动耗时（外部进程测量，shell/world 两个用例）
```

## 用例说明

### ChunkGeneration（区块生成吞吐）

走**生产级区块生成系统**完整链路：`ServerChunkManager` 请求聚合 → `ChunkTaskScheduler` 邻居依赖调度（ReentrantAreaLock）→ `UniversalWorkerPool` 区域互斥执行 → `ChunkProgressionTask` 逐状态推进 → FULL 完成回收。

- 参数：`threads`（生成系统内部线程池大小，1/2/4/8）× `batch`（每迭代生成的 n×n 区块批，8/16/32）
- 口径：一次迭代 = 提交一批 FULL 请求 → 主线程 `tick()` 泵送驱动 → 全部完成。报告 `chunks_per_second`
- 迭代间经 `unloadChunkSync` 卸载本批区块（PauseTiming 区间内），保证每轮都测真实生成而非缓存命中
- 存储为临时目录 RocksDB（`$TMPDIR/mc_bench_chunkgen`），seed 固定 12345，中心区块 (0,0)

**关键约束**：`ServerChunkManager` 的线程模型要求主线程 tick 驱动调度推进（存档解析完成回调等只在 `tick()` 出队）。异步批量提交后只等 future 不 tick 会**死锁**——基准内已复现生产主循环节拍（泵 `tick()`），新增类似用例务必遵守。

### Lighting（光照引擎本体）

直接驱动 TLS 方块光引擎（`acquireBlockLightEngine` → `blocksChangedInChunk` → `release`），不经过 ServerLightQueue/RuntimeLightTask 等 tick 级调度。

- 口径：一次迭代 = 对 (0,0) 区块一个 16×16 截面层批量方块光更新（256 方块），GLOWSTONE/AIR 交替翻转保证每轮传播工作量稳定。报告 `blocks_per_second`

### serverInitializeShell / serverInitializeWorld（服务端启动耗时）

每次重复启动一个外部 `minecraft-server` 进程并阻塞等待其退出，共 5 次重复取 mean/median/stddev。

- `serverInitializeShell`：服务端带 `--benchmark-exit-after-shell-init`，子系统初始化 + 网络监听就绪即退出
- `serverInitializeWorld`：服务端带 `--benchmark-exit-after-world-init`，再加世界创建 + 出生区域（`SPAWN_CHUNK_RADIUS`）区块全部生成 FULL 后退出
- 每次启动使用全新临时世界目录（`$TMPDIR/mc_benchmark_server_init/<case>_<n>`，内含空配置指定独立 worldName 与随机端口），退出后删除，保证冷启动口径一致
- 被测服务端进程显式传 `--profiler_enabled=false`，trace 只录基准进程侧
- 依赖：须先构建 `minecraft-server`（路径 `build/bin/RelWithDebInfo/minecraft-server` 硬编码，相对仓库根目录运行）

## 构建与运行

```bash
# 构建（也可只构建目标：cmake --build --preset macos-relwithdebinfo -- -j16 mc_benchmark）
cmake --build --preset macos-relwithdebinfo

# 在仓库根目录运行（结果输出到 benchmark_results/<时间戳>/）
./build/bin/RelWithDebInfo/mc_benchmark

# 常用 flag（google/benchmark 原生）
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_list_tests
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_filter="ChunkGeneration/8/16"
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_filter=Lighting --benchmark_min_time=3s
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_filter=serverInitializeShell
```

### 输出

每次运行输出到 `benchmark_results/yyyy-MM-dd_HH-mm-ss/`（已 gitignore）：

- `results.json`：全部用例的 JSON 结果（含聚合统计与内存指标）
- `<case>.perfetto-trace`：每用例一个 trace 文件（ui.perfetto.dev 分析）；同一用例多次重复为 `<case>.rep2.perfetto-trace` 等

### 内存指标

main 注册了 `MemoryProfiler`（`benchmark::MemoryManager` 实现），JSON 结果自动携带：

- `num_allocs` / `allocs_per_iter`：区间内分配次数（总次数 / 每迭代）
- `total_allocated_bytes`：累计分配字节
- `max_bytes_used` / `net_heap_growth`：累计分配 - 累计释放（近似在用，非精确峰值——纯 new/delete 钩子无法逐块跟踪）

实现为全局 `operator new/delete` 替换（本 TU 参与链接即生效，跨平台行为一致）。注意 `Start/Stop` 之间未捕获 sized-delete 的字节（无 size 的 delete 变体只计次数），`net_heap_growth` 是下界近似口径。

### CPU 硬件计数器（缓存命中率等）

google/benchmark 经 libpfm4 支持 `--benchmark_perf_counters=CYCLES,INSTRUCTIONS,...`，**仅 Linux**（依赖 PMU + `BENCHMARK_ENABLE_LIBPFM`，vcpkg port 未开启该选项，需自行定制 port 或 bazel 构建）。macOS（Apple Silicon PMU 不暴露用户态）与 Windows 均不可用。macOS 上如需缓存分析可用 `xctrace` 对 `mc_benchmark` 进程做外部采样。

## 添加新用例

1. 在 `cases/` 新建 `.cpp`，写 `void MyBench(::benchmark::State& state)` 函数 + `BENCHMARK(MyBench)->...` 注册
2. 计算型用例默认自动迭代即可；一次性事件用 `->Repetitions(N)->Iterations(1)`（**禁止**让进程启动类用例自动校准迭代，会启动几百个进程）
3. 需要每参数独立装配时用 `->Setup(...)/->Teardown(...)` 回调
4. 需要每用例独立 trace 文件时，在循环首行调 `PerfettoProfilerAdapter::setCaseName("名称")`
5. 在 `benchmark/CMakeLists.txt` 登记源文件；若引用 server 模块符号，按链接错误补齐源闭包（参照 `tests/unit/CMakeLists.txt` 对 mc_tests 的登记）

## 容易踩的坑

### ServerChunkManager 异步请求必须由主线程 tick 驱动

批量 `requestChunkAsync` 后只 `future.get()` 而不调 `tick()` 会**永久死锁**：存档解析完成回调入队 `m_pendingLoadCompletes`，只有 `tick()`（或 `requestChunkSync` 内部的主动 pump）出队。基准用"非阻塞收割 + tick 泵送"复现生产主循环；新用例若复用 ServerChunkManager，必须遵守此模型。

### 迭代间必须卸载区块

`ChunkGeneration` 若不卸载，第二轮起全部命中内存缓存，测的是缓存查询而非生成（实测 78ms → 0.046ms 的假象差异）。卸载 + 排空必须放在 `PauseTiming()/ResumeTiming()` 之间。

### worldgen 数据驱动注册表须显式加载

`RandomState::create` 查 `NoiseSettingsRegistry`，数据包未加载时断言崩溃。进程内首次使用前须经 `ensureWorldGenRegistriesLoaded()` 同类逻辑按依赖拓扑加载：noise → density_function → noise_settings → world_preset（参照 `tests/unit/main.cpp` 的 `WorldGenRegistryEnvironment`）。

### mc_benchmark 必须在仓库根目录运行

`serverInitialize*` 用例以相对路径 `build/bin/RelWithDebInfo/minecraft-server` 启动被测进程；结果目录也基于 CWD。

### benchmark 名大小写

注册名即函数名（如 `ChunkGeneration`、`Lighting`），`--benchmark_filter` 是正则且大小写敏感。

### 全局 new/delete 替换的影响范围

`MemoryProfiler.cpp` 的 operator new/delete 替换作用于整个进程（含库自身）。`Start/Stop` 边界外的分配不计入结果，但会轻微影响全局分配路径性能；若怀疑影响被测口径，可在 profiler 实现中加旁路开关。
