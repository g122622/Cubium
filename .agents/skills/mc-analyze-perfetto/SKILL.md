---
name: mc-analyze-perfetto
description: 用 Perfetto trace 排查 Cubium 的性能异常（长切片、启动慢、卡顿、阶段占比异常），从复现录制到切片归因、补追踪、同种子 A/B 验证的完整 SOP
---

# Perfetto 性能异常排查 SOP

本 skill 把「用户给一张 Perfetto 截图或一句『某处很慢』」收敛成可复现、可量化、可验证的结论。
补追踪是其中的一环，具体宏用法见子 skill `.agents/skills/mc-add-trace/SKILL.md`（需要插桩时先加载它）。

## 适用 / 不适用

适用：

- 用户给出 `.perfetto-trace` 文件或 Perfetto UI 截图，问「为什么这么慢 / 找原因」
- 某个 `MC_TRACE_*` 切片时长异常、启动某阶段占比异常、chunk 生成卡顿、tick 超时
- 需要判断「是 CPU 密集、锁竞争，还是未插桩代码里的黑盒」

不适用：

- 内存分配/泄漏（走 Tracy：`mc-analyze-memory` 场景或 `docs/MEMORY.md`）
- 只有代码没有 trace 的纯猜测式优化（先按本文复现出 trace 再下结论）

## 前置条件

| 项 | 值 |
|---|---|
| 查询解释器 | `C:\Users\Administrator\AppData\Local\Programs\Python\Python310\python.exe`（**只有它装了 `perfetto`**；DSH 自带 3.12 会 `ModuleNotFoundError`） |
| 查询脚本 | `.agents/skills/mc-analyze-perfetto/scripts/perfetto_query.py` |
| 编码 | PowerShell 里先 `$env:PYTHONIOENCODING='utf-8'`，否则中文列名/提示会乱码 |
| 产物 | 服务端 trace 默认写到进程 CWD 的 `server_trace.perfetto-trace`（`MC_TRACE_SERVER_OUTPUT`） |

首次在新机器上跑 `perfetto` 会下载 `trace_processor_shell`；离线环境改用
`TraceProcessorConfig(bin_path=...)` 指向已缓存的二进制。

## 标准流程（六步，别跳步）

### 1. 把问题钉到具体切片

从截图/描述里取出：切片名、时长、线程名、所属阶段。截图只能当线索，**结论必须来自 trace 数据**。
先跑概览，确认 trace 覆盖了问题发生的时间窗：

```powershell
$py = 'C:\Users\Administrator\AppData\Local\Programs\Python\Python310\python.exe'
$q  = '.agents\skills\mc-analyze-perfetto\scripts\perfetto_query.py'
& $py $q server_trace.perfetto-trace info
& $py $q server_trace.perfetto-trace longest --min-ms 100 --limit 30
& $py $q server_trace.perfetto-trace stats _selectStructureStart GenerateStructureStarts
```

### 2. 看清调用栈与子切片

```powershell
& $py $q <trace> tree <sliceId> --depth 8      # 子切片树 + 「未被子切片覆盖的自身耗时」
& $py $q <trace> ancestors <sliceId>           # 从根到父
```

- 子切片占满 → 继续往下钻（第 2 步递归）。
- 子切片几乎为空但切片很长 → 时间在**未插桩代码**里，或线程被阻塞（第 4 步）。

### 3. 用「计数」反推黑盒里发生了什么

未插桩的函数往往有可观测的相邻事件。本例：`_selectStructureStart` 的子切片只有
`NoiseChunk::ctor`，而它出现了 852 次 —— 直接点出「这段时间在反复做单列噪声查询」：

```sql
select count(*) from slice where name='NoiseChunk::ctor'
  and ts >= <sliceTs> and ts < <sliceTs+sliceDur>;
```

同类线索：`StorageTask::execute`、`ChunkSourceLoader::drain`、`RTree::build` 等低频事件的计数
能反映黑盒内部的调用次数。

### 4. 区分「CPU 密集」与「等待」

本项目 trace 是进程内 Perfetto SDK 录制，**没有 sched/线程时钟**，`slice.thread_dur` / `thread_ts`
恒为 `NULL`，不能用它算 CPU 占用。改用两条证据：

1. **同窗口其它线程在干什么**：`perfetto_query.py context <sliceId>`。若其它 worker 全空闲、
   只有这一个线程占着，基本是 CPU 密集（锁等待需要有人持锁）。
2. **同窗口该线程的切片是否连续**：连续的子切片/事件说明在跑；大段空白且其它线程也没动，
   说明在等 I/O 或等锁。

同时看 `busy`（各线程 depth=0 忙碌时长）判断并行度是否被人为串行化。

### 5. 补追踪（子 skill）

时间钉到某个函数/循环但还看不清内部构成时，按 `mc-add-trace` 的规范补 1 层
`MC_TRACE_SCOPED_EVENT`（类别取 `TraceEvents.World.ChunkGen` 等已注册叶子），重建、重录、重解析。

- 分段粒度：先按「准备 / 主循环 / 收尾」切 2~4 段，不要一上来就 per-block 插桩。
- 事件名用 `类名::函数名[::子段]`，父子切片不要重复带同一组坐标参数。
- 每次只加一层，加完立刻重录验证；多数问题 1~2 轮就能定位。

### 6. 验证并清理追踪

- 修复前后必须**同种子 A/B**（见「复现」），报告「每列/每区块平均」这类稳定指标，不要只报总时长。
- 高频追踪（循环内、逐方块、逐位置的）诊断完**必须删除**：每个事件 ~0.1µs 且会撑大 trace
  （本项目一次 30s 启动 trace 已 24~28MB、38 万切片）。
- 低频追踪（每任务 / 每列 / 每区块 / 每结构一次，量级 10³ 以内）**可以保留并提交**：
  它们是下次排障的现成入口。判断标准是触发频率，不是「是否临时」。

## 复现（服务端启动期问题）

```powershell
$dir = Join-Path (Get-Location) '.tmp_trace\world_seed'
if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
New-Item -ItemType Directory -Path $dir | Out-Null
# 固定种子！不设 levelSeed 时每次新建世界都是随机种子，跨 run 数字不可比
Set-Content (Join-Path $dir 'server_options.json') '{"world":{"levelSeed":"114514"}}' -Encoding UTF8

& build\bin\RelWithDebInfo\minecraft-server.exe `
    --benchmark_exit_after_world_init `
    --config (Join-Path $dir 'server_options.json') `
    --profiler_enabled=true *> (Join-Path $dir 'run.log')
```

- `--benchmark_exit_after_world_init`：初始化 + 出生点 `SPAWN_CHUNK_RADIUS` 全部生成到 FULL 后
  **干净退出**，`stopTracing()` 才会把 trace 落盘（强杀进程可能一个字都不写）。
- 每次都要用**全新世界目录**：复用目录会走存档加载，压根不生成，指标全失真。
- 想把 trace 留在仓库根之外：先 `--config` 指定目录，再把 `server_trace.perfetto-trace` 拷到
  `.tmp_trace/` 下分析（`.tmp_trace/` 不要提交）。
- 不要给服务端 stdout 接 `| Select-Object -First N` 这类会提前断管道的命令：PowerShell 会留下
  未退出的服务端进程，下一次运行报 `World is locked by another process`。重定向到日志文件。

其它场景的复现入口：`--gametest`（集成测试）、`mc_tests --gtest_filter=...`（单测）、
长期运行问题则常驻服务器后手工操作再用 Ctrl+C 正常停服。

## 常见坑（按踩坑频率排序）

1. **`thread_dur` 为 NULL**：进程内 trace 没有线程时钟，不能算 CPU 时间；用第 4 步的并发证据。
2. **时间戳是增量时钟且起点是大偏移**：包内 `timestamp_delta_us` 需按序列累加；`ts` 形如
   `614305110909100`。永远相对 `trace_bounds.start_ts` 换算成 ms，别直接当秒。
3. **`slice.dur = -1`**：trace 结束时未闭合的切片。所有统计都要 `dur >= 0` 过滤。
4. **只 join `thread_track` 会漏切片**：async/global track、counter 都挂别的 track。统计总量时
   用 `LEFT JOIN`，或先 `select * from track` 看清类型。
5. **`trace_processor` 版本差异**：不同版本 `slice` 表列不同（例如没有 `thread_inst_end_ts`）。
   写 SQL 前先 `select name from pragma_table_info('slice')` 核对，别照抄旧脚本。
6. **变量名歧义**：`slice` 与 `thread` 都有 `name`，多表查询一律 `s.name` / `t.name` 限定。
7. **截图会骗人**：Perfetto UI 里从视口左边缘开始的长条，实际 `ts` 在窗口之外、`dur` 可能更长；
   窗口裁剪还会让父切片看起来包含不存在的子切片。以 trace 数据为准。
8. **多进程 trace**：e2e 的 trace 同时含 client 与 server，先按 `process.name` 过滤再统计。
9. **世界种子不可比**：见「复现」。另外区块生成受调度影响有 ±10~20% 方差，结论落在
   「每次调用的平均成本」上才稳。
10. **追踪本身改变被测量对象**：循环内插桩会显著抬高该函数耗时并撑大 trace；先粗后细，
    细粒度插桩只用于定位、定位完即删。
11. **高频/低频的取舍**：per-block、per-position、per-cell 的追踪 = 必须删；per-task / per-column /
    per-chunk 的追踪 = 可保留提交（见第 6 步）。
12. **trace 正在被写**：Perfetto 在 `stopTracing()` 时才落盘，进程运行中文件可能不存在或不完整；
    崩溃路径可能只有半截 trace，先 `info` 看 `trace_bounds` 是否覆盖到问题时间窗。
13. **同一函数被按维度重复调用时，必须按参数分组**：总次数会把"每个区块各调一次"和
    "某个参数下多调 400 次"混在一起。给切片打上 `"set", xxx.path()` 这类字符串参数，
    再用 `args` 的字符串取值表（或 `group by a.string_value`）分开统计，才能看出谁贵。
14. **`slice` 总数正常不代表被测量对象正常**：先查 `stats` 里 `traced_buf_*dropped/overwrite/loss`
    全为 0，确认 trace 完整，再去下"并发度只有 1"这类结论；反之若确有丢包，并发度、次数的
    统计全部失效。

## 实战案例（本项目，勿重复踩）

**案例一：结构起点装配过早**（`_selectStructureStart` 单次 4.2s）

**现象**：`_selectStructureStart` 单次 4.2s，占启动期 chunk 生成的绝大部分。

**归因链**（全部由 trace 数字支撑）：

1. `tree` 显示该切片只有 `NoiseChunk::ctor` 子切片，计数 852 → 时间是 852 次单列噪声查询。
2. 代码：`_selectStructureStart` → `JigsawAssembler` 地形匹配每个接点调
   `IChunkGenerator::getHeight` → `getBaseColumn` 新建单列 `NoiseChunk` + `NoiseBasedAquifer`。
3. 补 3 层低频追踪（`getHeight`、`getBaseColumn::Prepare`、`::SliceFill`、
   `NoiseBasedAquifer::ctor::MaxPreliminarySurface`）后重录，得到每列 2.75ms 的构成：
   Prepare 1.70ms（其中含水层 `maxPreliminarySurfaceLevel` 对区块外扩一圈的 4 方块网格
   逐列采样，主世界 121 列）/ SliceFill 0.81ms（三线性插值的角点噪声采样）/
   Cell 填充 0.18ms / 逐方块 0.14ms。
4. 修复：`preliminarySurfaceLevel` 加维度级记忆（`RandomState::preliminarySurfaceLevelCache()`），
   纯函数结果按 4 方块列跨调用复用。
5. 同种子 A/B（两次修复后运行取范围）：每列 2752.9µs → 1312.5~1412.9µs（−49~52%），
   最长 `_selectStructureStart` 2781.1ms → 1413.3~1525.1ms（−45~49%），
   `initializeWorld` 5868.7ms → 4441.9~4581.2ms（−22~24%）。

**结论模板**（报告必须给全这五项）：

```
谁慢：<切片名> @ <线程>（<阶段>）
多慢：<dur>；同层占比 <%>；全 trace 内该切片 n 次 / 合计 <ms>
为什么：<调用次数 × 单次成本>，单次成本拆成 <子段 a/b/c>
改了什么：<文件:函数 + 一句话机制>
快了多少：同种子 A/B <改前> → <改后>（−<%>）
```

**案例二：一次性同心环搜索被逐区块重算**（`isPlacementChunk` 单次 235ms）

**现象**：`initializeWorld` 3.0s，其中 2.7s 落在要塞（strongholds）的
`StructurePlacementState::isPlacementChunk` 里，最长单次 2.19s。

**归因链**：

1. 给 `isStructureChunk` 的三个子步骤各加一条低频追踪（`debug.set` 打结构集名），
   按 `debug.set` 分组后一眼看出：strongholds 一个结构集 238ms，其余 16 个合计 ~16ms。
2. 强塞的 `isPlacementChunk` 走查表，表本身是一次性 128 次「半径 112 方块、step=1」的
   群系搜索（每次 57×57=3249 次气候采样，合计 ~42 万次）。
3. 该表缓存挂在放置规则上（`resolveRingPositions` 单飞），所以只在第一个区块上付一次钱；
   剩余 2.5ms/2025 次 = 单纯查 128 个坐标，无需优化。
4. 修复：128 个要塞的搜索彼此独立（各自 `fork()` 出来的随机流、群系源只读），用
   `util::parallelFor` 按要塞并行；缓存改用"持锁计算"的单飞语义，避免并发区块任务各算一遍。
5. 同种子 A/B：`GenerateRingPositions` 2716ms → **236ms（−91%）**，
   `initializeWorld` 3007ms → **476ms（−84%）**，差分能力 structure_starts 845/845 仍一致。

**教训**：判断"能不能并行"先看三件事——数据依赖（各自独立随机流）、输入是否只读、
写目标是否按元素互斥。三者都满足就不必引入锁，也不必经过任务池（池内阻塞等待有自锁风险，
见 `util/parallelFor` 的说明）。

## 交付前自检

- [ ] 结论每一项都能对应到一条 SQL 或代码行；没有「大概/可能」措辞
- [ ] 修复有同种子 A/B 数字；未修复则明确列出「已定位但未改」的原因
- [ ] 高频追踪已删除，保留的低频追踪在报告里列出事件名与触发频率
- [ ] 改动的代码已 clang-format；受影响的 README 已同步（含「容易踩的坑」）
- [ ] `.tmp_trace/`、trace 文件、日志未进入提交。`.tmp_trace/`和日志等需要清理。
