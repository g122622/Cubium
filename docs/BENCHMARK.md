# 基准测试指南（mc_benchmark）

Cubium 的性能基准基于 [google/benchmark](https://github.com/google/benchmark) 1.9.5（vcpkg 引入），可执行目标 `mc_benchmark`。框架细节与用例实现在 [benchmark/README.md](../benchmark/README.md)，本篇是使用指南。

## 构建

```bash
# macOS
cmake --build --preset macos-relwithdebinfo -- -j{核心数-2} mc_benchmark
# Windows（仅支持唯一命令）
./scripts/configure.sh build
```

依赖说明：`vcpkg.json` 中的 `benchmark` port 首次配置会多编译一个库；`mc_benchmark` 链接 `mc_gen/mc_server_storage/mc_common/mc_bedrock_addon` 并直接编译一批 server 源文件（源闭包参照 mc_tests 的登记方式），因此构建时间与 mc_tests 同量级。

## 运行

**必须在仓库根目录运行**（server_initialize 用例以相对路径定位被测服务端，结果目录基于 CWD）：

```bash
./build/bin/RelWithDebInfo/mc_benchmark                          # 全部用例
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_list_tests   # 列出用例
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_filter="ChunkGeneration/8/16"
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_filter=Lighting --benchmark_min_time=3s
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_filter=serverInitialize
```

结果输出到 `benchmark_results/<时间戳>/`（已 gitignore）：

| 文件 | 内容 |
|---|---|
| `results.json` | 全部结果，含 mean/median/stddev 聚合与内存指标 |
| `<case>.perfetto-trace` | 每用例一个 trace（ui.perfetto.dev 分析火焰图） |

命令行 flag 为 google/benchmark 原生（`--benchmark_filter/--benchmark_min_time/--benchmark_repetitions/--benchmark_out` 等），完整列表见 `--help`。

## 现有用例

### ChunkGeneration（区块生成系统吞吐）

测生产级并行生成系统：`ServerChunkManager` → `ChunkTaskScheduler`（区域锁调度）→ `UniversalWorkerPool` → 逐状态推进到 FULL。

- 参数矩阵：`threads`（内部线程池大小 1/2/4/8）× `batch`（每迭代 n×n 区块批，8/16/32）
- 口径：提交一批 FULL 请求，主线程 `tick()` 泵送驱动至全部完成；报告 `chunks_per_second`
- 迭代间卸载本批区块（暂停计时），保证每轮测真实生成

解读示例（macOS arm64 实测，batch=16）：

| threads | 耗时/迭代 | chunks/s | 加速比 |
|---|---|---|---|
| 1 | 4975 ms | 51.9 | 1.0× |
| 2 | 2711 ms | 96.3 | 1.9× |
| 4 | 417 ms | 632.7 | 12.2× |
| 8 | 253 ms | 1056.3 | 20.4× |

注意 threads=1 档仍走完整生成系统（含调度器/区域锁开销），与"裸生成器性能"不同口径；跨档对比反映的是系统的并行扩展性。

### Lighting（光照引擎本体）

TLS 方块光引擎批量更新：每迭代对一个 16×16 截面层做 256 个方块光变更，GLOWSTONE/AIR 交替翻转保证传播工作量稳定；报告 `blocks_per_second`。不经过 tick 级调度（那属于系统吞吐，由 ChunkGeneration 间接覆盖）。

### serverInitializeShell / serverInitializeWorld（服务端启动）

每次重复启动外部 `minecraft-server`（5 次重复），统计 mean/median/stddev：

- **Shell**：`--benchmark-exit-after-shell-init`——子系统初始化 + 网络监听就绪即退出（实测 macOS ~2.05s）
- **World**：`--benchmark-exit-after-world-init`——再加世界创建 + 出生区域（`SPAWN_CHUNK_RADIUS`）全部生成 FULL（实测 macOS ~12.3s）

每次启动使用全新临时世界目录（`$TMPDIR/mc_benchmark_server_init/`，退出即删），保证冷启动口径；被测进程 `--profiler_enabled=false`，trace 只录基准进程侧。**须先构建 `minecraft-server`。**

## 指标解读

### 时间与吞吐

- `Time`：每迭代墙钟时间（默认毫秒，`--benchmark_min_time` 控制自动迭代总量）
- `chunks_per_second` / `blocks_per_second`：User Counter 吞吐（`kIsIterationInvariantRate`，自动迭代下统计正确）
- `Repetitions(5)` 用例输出 `_mean/_median/_stddev/_cv` 聚合行；`_cv`（变异系数）< 5% 视为稳定

### 内存指标（MemoryProfiler）

注册了 `benchmark::MemoryManager` 实现（全局 operator new/delete 钩子），JSON 自动携带：

| 指标 | 含义 |
|---|---|
| `num_allocs` / `allocs_per_iter` | 区间内分配次数（总 / 每迭代） |
| `total_allocated_bytes` | 累计分配字节 |
| `max_bytes_used` / `net_heap_growth` | 累计分配 - 累计释放（**近似在用，非精确峰值**——无 size 的 delete 只计次数，是下界口径） |

用于跨提交对比分配行为回归（如某改动让每迭代分配次数翻倍）。跨平台行为一致（C++ 全局替换），无平台 API 依赖。

### CPU 缓存命中率等硬件计数器

`--benchmark_perf_counters` 依赖 Linux PMU + libpfm4（`BENCHMARK_ENABLE_LIBPFM`）。**macOS/Windows 不可用**（Apple Silicon PMU 不暴露用户态）。macOS 替代：`xctrace` 对 `mc_benchmark` 外部采样（CPU Counters 模板）。vcpkg port 默认未开启 libpfm，Linux 上使用需定制 port 或 bazel 构建。

## 添加新用例

1. `cases/` 新建 `.cpp`：`void MyBench(::benchmark::State&)` + `BENCHMARK(MyBench)->...`
2. 用例模型选择：
   - 计算型：默认自动迭代（每用例跑 ≥0.5s）
   - 一次性事件（进程启动等）：`->Repetitions(N)->Iterations(1)`，**禁止**让它自动校准（会启动几百个进程）
3. 每参数独立装配用 `->Setup(...)/->Teardown(...)`；参数用 `->Arg/->Args` 编码，`state.range(i)` 读取
4. 需要独立 trace 文件：循环首行 `PerfettoProfilerAdapter::setCaseName("名称")`
5. `benchmark/CMakeLists.txt` 登记源文件；引用 server 符号时按链接错误补齐源闭包（参照 `tests/unit/CMakeLists.txt`）
6. 有迭代间状态时在 `PauseTiming()/ResumeTiming()` 区间内清理；复用 `ServerChunkManager` 必须主线程 tick 泵送（见 benchmark/README.md 容易踩的坑）
7. 更新 `benchmark/README.md` 的用例清单

## 性能回归工作流

```bash
# 1. 基线（改动前）
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_out=baseline.json --benchmark_filter="ChunkGeneration|Lighting"

# 2. 改动后
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_out=current.json --benchmark_filter="ChunkGeneration|Lighting"

# 3. 对比（tools/compare.py 来自 google/benchmark 仓库，vcpkg 的 tools feature 亦有安装）
python3 tools/compare.py benchmarks baseline.json current.json
```

机器空闲度对结果影响显著（ChunkGeneration threads=8 档尤其敏感）；跨机器只对比相对比值，不对比绝对值。
