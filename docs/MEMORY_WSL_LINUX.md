# Cubium 内存实测报告（WSL2 / Linux / 区块生成）

> 测量日期：2026-10-07　构建：`linux-relwithdebinfo`（clang 22.1.8，Tracy **OFF**、Perfetto ON，Commit `5cceb8b90`）
> 测量主机：WSL2 Ubuntu，16 vCPU，62 GB RAM
> 测量方法：`heaptrack`（LD_PRELOAD 全量 malloc 钩子 + 调用栈归因）为主，`/usr/bin/time -v` 峰值 RSS 与 `mc_benchmark` 内置 `MemoryProfiler` 为辅
> 对照文档：`docs/MEMORY.md`（Windows / viewDistance=16 / 满负载，2026-09-25）、`docs/MEMORY_IDLE_MACOS.md`（macOS / 空载，2026-09-29）

---

## 零、本次测量要回答的问题

目标是**把程序内存砍半**。为此必须先回答：**内存在哪里，生成中与生成后各是多少**。
本报告用 heaptrack 的**按分配点归因**（不是尺寸直方图）给出逐项字节数，并区分两个口径：

| 口径 | 含义 | 8/32 实测 |
|---|---|---:|
| **生成峰值** | 1024 区块并行生成过程中进程堆的最高点 | **159.4 MB**（优化前 207.6） |
| **生成后稳态** | 同一批区块全部 FULL 后、仍驻留内存时的堆 | **104.8 MB**（优化前 116.9） |
| 服务端空载（不生成区块） | `--benchmark-exit-after-shell_init` 路径的堆峰值 | 27.8 MB |
| 服务端进主循环（含出生区区块） | 堆峰值 / 稳态 | 42.1 / 39.7 MB |

**一句话结论**：生成峰值里最大的单项曾是密度函数区块级实例的 Op 序列深拷贝
（86.0 MB / 41%），代码注释已自证该拷贝冗余——**已落地修复，峰值降 23%**（见 §五靶点 1）。
稳态侧的**每区块边际成本约 68.5 KB**（由 64/1024 区块两次冻结差分实测），
其中 49% 是 `ChunkPrimer` ctor 与 `PalettedContainer` 的**多次独立堆分配**——
这是稳态进一步压缩的主攻方向（见 §五靶点 8）。

---

## 一、基准吞吐结果（顺带产出，供对照）

`mc_benchmark --benchmark_filter=ChunkGeneration`，`--benchmark_min_time=0.3s`，每档 1 次迭代：

| 档位 | 区块数 | 墙钟 | chunks/s | 峰值 RSS |
|---|---:|---:|---:|---:|
| ChunkGeneration/1/8 | 64 | 2022 ms | 32.3 | — |
| ChunkGeneration/2/8 | 64 | 1110 ms | 59.8 | — |
| ChunkGeneration/4/8 | 64 | 705 ms | 96.6 | — |
| ChunkGeneration/8/8 | 64 | 498 ms | 144.9 | 311 MB |
| ChunkGeneration/1/16 | 256 | 5973 ms | 43.4 | — |
| ChunkGeneration/8/16 | 256 | 1591 ms | 172.3 | — |
| ChunkGeneration/1/32 | 1024 | 20620 ms | 50.0 | — |
| **ChunkGeneration/8/32** | **1024** | **5609 ms** | **190.0** | **318 MB** |

单进程跑完整 12 档套件峰值 RSS = 728 MB（含每档 Setup/Teardown 重建世界、RocksDB 开关 12 次的开销）。

> **注意**：`mc_benchmark` 注册了 `MemoryProfiler`（全局 `operator new/delete` 钩子），google/benchmark 会为**每次重复额外跑一遍**基准函数采集内存指标。因此单档实际执行 2 遍生成 + 2 遍卸载。

---

## 二、生成峰值归因（8/32，207.6 MB）

由 `heaptrack_print -p 1 -n 400 -s 1`（未合并调用栈，取进程堆峰值时刻的存活分配）聚合到**最深 Cubium 帧**：

| # | 分配点 | MB | 占比 | 次数 | 性质 |
|---:|---|---:|---:|---:|---|
| 1 | `CompiledDensityFunction::newInstance` @ `CompiledDensityFunction.cpp:106`（`std::vector<Op> newOps = m_ops` 深拷贝） | **86.0** | **41.4%** | 291 688 | **纯冗余，可归零** |
| 2 | `ChunkPrimer::ChunkPrimer` @ `ChunkPrimer.cpp:164/168/169`（`make_shared<ChunkData>` + `BiomeContainer` + `array<Heightmap,7>`） | 32.4 | 15.6% | 5 832 | 半可削 |
| 3 | `NoiseInterpolator::NoiseInterpolator` @ `NoiseChunk.cpp:103/104`（两个 `m_slice0/m_slice1` 扁平缓冲） | 24.8 | 11.9% | 46 208 | 可削 |
| 4 | `FlatCache::FlatCache` @ `DensityFunctions.hpp:998`（构造期整张预计算表） | 10.9 | 5.2% | 199 272 | 需评估 |
| 5 | `TracyTrackingAlloc::allocate` @ `MemoryTracking.hpp:283`（PalettedContainer 存储 vector） | 7.4 | 3.6% | 119 806 | 本体必要；**注意 Tracy 已 OFF**（该分配器在关闭态退化为 `std::allocator`，只是类型名仍叫 `TracyTrackingAlloc`） |
| 6 | `NoiseBasedAquifer::NoiseBasedAquifer` @ `NoiseBasedAquifer.cpp:76/77` | 5.1 | 2.5% | 5 184 | — |
| 7 | `StaticChunkCache2D<>::StaticChunkCache2D` @ `StaticChunkCache2D.hpp:76` | 5.0 | 2.4% | 24 256 | — |
| 8 | `NoiseChunk::samplePreliminarySurfaceLevel` @ `NoiseChunk.cpp:648`（`unordered_map` 增长） | 4.7 | 2.3% | 534 976 | 可削 |
| 9 | `CompiledDensityFunction::newInstance` @ `:107/:111/:132/:150/:157/:164`（`newObjects`/`newSubEvaluators`/`Adapter`/缓存对象） | 17.9 | 8.6% | — | 部分可削 |
| 10 | `ChunkProgressionTask::executeEmptyLoad` @ `:164` | 1.9 | 0.9% | 5 832 | — |
| 11 | `ReentrantAreaLock::lock` | 1.7 | 0.8% | — | — |
| 12 | `BytecodeGen::compile` 及其 `emit*` 子项（维度级编译，仅一次） | 1.7 | 0.8% | — | 启动期固定 |
| 13 | 其余（生命周期管理器、RTree、RocksDB、方块注册表等） | ~14 | 6.8% | — | 启动期/小项 |

> 按分配点聚合时，`newInstance` 的多行相加为 86.0 + 17.9 = **103.9 MB（50.0%）**——
> 即**密度函数区块级实例的构建占生成峰值的一半**。

---

## 三、生成后稳态归因（8/32，104.8 MB）

### 3.1 归因方法：稳态冻结快照

heaptrack 的 massif 稳态堆树在本版本（1.5.0）不可用（见 §七 第 2 条），
故改用**稳态冻结快照**：在基准完成一批生成、拿到全部区块的快照后（此时所有
worker 已停），直接 `std::_Exit(0)` 跳过全部析构与 atexit——**heaptrack 报出的
"未释放"即稳态存活分配的真实构成**。

驱动方式：环境变量 `MC_BENCH_FREEZE_STEADY=1`（`benchmark/cases/ChunkGenerationBenchmark.cpp`，
TODO(临时诊断)）。

用 **64 区块与 1024 区块两次冻结的差分**消除进程固定开销（RocksDB、方块注册表、
数据包资源、命令树等约 **39.9 MB**），得到**每区块的边际成本**：

```
(104.0 − 39.9) MB / (1024 − 64) 区块 = 70 102 B/区块 ≈ 68.5 KB/区块
```

### 3.2 每区块边际成本构成（已归一到 1024 区块）

| 项 | MB | 占比 | 性质 |
|---|---:|---:|---|
| `ChunkPrimer::ChunkPrimer`（`ChunkData` + `BiomeContainer` + `Heightmap[7]`） | 22.8 | 33% | 见 §3.4 |
| `PalettedContainer` 存储（storage/palette/hashMap） | 11.0 | 16% | 三次独立堆分配 |
| `ServerTickList` 的 `ScheduledTick`（流体 tick） | 6.8 | 10% | **基准口径产物，见 §3.5** |
| `SWMRNibbleArray` 缓冲（天空光/方块光） | 6.7 | 10% | 见 §五靶点 7 |
| `SingleChunkLifecycleManager` 邻居集 | 4.5 | 7% | — |
| `CompiledDensityFunction::newInstance`（区块级求值器） | 4.4 | 6% | 见 §五靶点 5 |
| `NoiseInterpolator` 双 slice | 3.1 | 5% | 对齐原版，不可削 |
| `ChunkProgressionTask::executeEmptyLoad` | 1.7 | 2% | — |
| `FlatCache` 预计算表 | 1.3 | 2% | 见 §五靶点 5 |
| `ChunkData::_setBlockStateUnlockedGen`、`ReentrantAreaLock`、其余 | ~9 | 13% | — |
| **合计** | **≈ 68.5** | | |

### 3.4 `ChunkPrimer` ctor 的 22.8 MB：primers 数量是目标区块的 2.85 倍

`ChunkPrimer` ctor 的边际成本实测 **23.36 KB/区块**，而 `ChunkData` 本体按成员推算只有
约 8.4 KB（`BiomeContainer` 3 072 B + `Heightmap[7]` 2 072 B + nibble 表 2 080 B + 其余约 1.2 KB）。
差额来自 **primers 数量远多于目标区块数**。

**primer 状态普查**（新增 `dumpPrimerStats` 诊断，`benchmark_results/primer_stats/`）：

| gen_status | primer 数 | 相对中心距离 | 持有 localBiomes | 持有 noiseChunk |
|---|---:|---|---:|---:|
| `structure_starts` | 1 472 | 19–27 | 1 472 | 0 |
| `full` | 1 024 | 0–16 | **0** | 0 |
| `biomes` | 148 | 18–19 | 148 | 148 |
| `carvers` | 140 | 17–18 | 140 | 0 |
| `initialize_light` | 132 | 16–17 | 132 | 0 |
| **合计** | **2 916** | | **1 892** | 148 |

- **1024 个 FULL primer**（目标区域 32×32）已正确释放本地副本（`toChunkData` 的 `reset` 生效）。
- **1892 个非 FULL primer 是"光环"**：目标区域外 16–27 格，各停在某个中间状态，
  **全部仍持有 `BiomeContainer` + `Heightmap[7]`**（`reset` 只发生在 FULL 路径）。
- 8.4 KB × 2.85 primer/区块 = 23.9 KB/区块，与实测 23.36 KB 吻合——**这解释了 22.8 MB 的构成**。

**可削项（靶点 9）**：对非 FULL primer 提前释放 `m_biomes`/`m_heightmaps`，
可省 **1892 × (3 072 + 2 072) B = 9.28 MB（稳态 −8.9%）**。

> **但要注意原版对照**：MC 1.21.11 的 `ProtoChunk` 同样持有 heightmaps
> （`ChunkAccess.heightmaps` 是 `Map<Types, Heightmap>`，`computeIfAbsent` 惰性创建）
> 与 biomes（存在 `LevelChunkSection.biomes` 里，section 随区块分配）。
> 原版**不为"光环"primer 单独保留一份 biomes 副本**——biomes 是 section 的一部分。
> 所以 Cubium 的"primer 侧独立副本"确实是额外开销，但**能否安全释放取决于这些
> 光环 primer 在后续状态推进中是否还需要读自己的 `m_biomes`**（`biomes` 状态用
> `fillBiomesFromNoise` 写入；`surface`/`carvers` 经 `getBiomeAtBlock` 读）。
> **实现前必须先确认：光环 primer 推进到下一状态时，其 `m_biomes` 是否已被写入。**
> 若已写入且后续只读，则应改为**写入后即转入 `ChunkData`**（而非等 FULL 才转），
> 这样 `m_biomes` 从 `biomes` 状态之后就不再需要。

### 3.5 `ServerTickList` 的 6.8 MB 已确认为基准口径产物

初版判断"这 7.1 MB 是基准口径产物"**已由实测证实**（新增 `MC_BENCH_DRAIN_TICK_LISTS=1`
诊断，用 `TickManager::tick` 推进 1000 游戏刻排空 tick list）：

```
drain_tick_lists: ticks=1000  blockPending 0 -> 0  fluidPending 54638 -> 8
```

排空后稳态存活量 **104.0 MB → 100.5 MB（−3.5 MB）**，逐项归因显示消失的正是：

| 项 | 排空前 | 排空后 | 差 |
|---|---:|---:|---:|
| `_Rb_tree::_M_insert_unique<ScheduledTick>` | 3.99 MB | **0.00** | −3.99 |
| `_Hashtable::_M_insert_unique<ScheduledTick>` | 3.44 MB | 0.65 MB | −2.79 |
| **合计** | **7.43 MB** | 0.65 MB | **−6.78** |

（总量只降 3.5 MB 而非 6.8 MB，因为 tick 执行时会在 `StarLightEngine` 等路径产生新的
稳态分配，部分抵消。）

**结论**：这 6.8 MB **不应计入常规稳态**。真实服务端下 `ServerWorld::tick()` 每刻排空
tick list，条目不会累积。修正后的真实稳态约为 **104.8 − 6.8 ≈ 98 MB**。

**与初版（本节旧版）的差异**：初版按结构性推算给出"`ChunkData` 外壳 4.5 MB +
`Heightmap` 7.2 MB + `BiomeContainer` 3.1 MB"，其中 **`Heightmap` 的 7.2 MB 是错的**——
位压缩早已落地（`sizeof(Heightmap)=296 B`，7 类型仅 2.0 MB，见 §五靶点 3），
初版沿用了 `docs/MEMORY.md`（2026-09-25）的过时数据。实测的 22.8 MB（`ChunkPrimer` ctor
三项合计）与初版推算的 14.8 MB 相差 8 MB，差额即来自此。

### 3.3 段调色板位宽实测分布

`benchmark_results/palette_bits/chunk_palette_bits_threads=8_batch=32.csv`
（含新增的 `container_bytes` 列 = `PalettedContainer::estimatedMemoryUsage()`）：

| bits | 段数 | 位存储字节 | 容器总字节 | 小计 |
|---:|---:|---:|---:|---:|
| 1 | 2 025 | 512 | 680 | 1.31 MB |
| 2 | 6 407 | 1 024 | 1 200 | 7.33 MB |
| 3 | 1 257 | 1 536 | 1 728 | 2.07 MB |
| **合计** | **9 689** | 均摊 983 | **均摊 1 160** | **10.72 MB** |

**关键观察**：
- **最高位宽只有 3 bit**，66% 的段是 2 bit——位存储本身没有浪费。
- 但**容器总字节比位存储多 1.63 MB**（每段多约 180 B）：这是 palette 数组 + 开放寻址
  哈希表 + `Data` 头。`bits=1` 时段均 680 B 而位存储只有 512 B，**外壳占 25%**。
- **1024 区块 × 24 段 = 24 576 个段槽位，只有 9 689 个（39.4%）被填充**；
  空段的 `unique_ptr<ChunkSection>` 已为 `nullptr`，不占位存储。

---

## 四、服务端空载内存（不生成区块 / 进主循环）

### 4.1 `--benchmark-exit-after-shell_init`（子系统初始化 + 网络监听就绪即退出）

- 堆峰值 **27.8 MB** @ t=16.6s，随后 `server.stop()` 释放至 0。
- 峰值归因（`-p 1 -n 300`）：

| 项 | MB | 说明 |
|---|---:|---|
| `BytecodeGen::emitGenericShiftedNoise` + `asReg` + `emitBinary` + `makeConstantEvaluator` | 3.5 | 维度级密度函数**编译**（一次性，非区块级）|
| `RocksDBDatabase::open` + `createDBOptions` | 2.2 | 16 列族 |
| `RTree<>::create` + `OverworldBiomeBuilder::addSurfaceBiome` | 1.9 | 生物群系气候参数树 |
| `AdvancementLoader::loadFromDataPackRepository` + `Advancement::fromJson` | 1.3 | 1 584 个进度 |
| `WallBlock` lambda | 0.8 | 26 个墙方块的状态生成 |
| `FolderResourcePack::initialize` | 0.9 | 数据包文件索引 |
| `RecipeSerializers` + `Ingredient` | 0.9 | 1 470 个配方 |
| `LootSerializers` 系列 | 0.9 | 战利品表 |
| `StateContainer::generateStates` | 0.4 | 方块状态空间 |
| 其余 | ~13 | 各类注册表 / 命令树 / 脚本系统 |

> 与 `docs/MEMORY_IDLE_MACOS.md`（macOS 空载 49 MB footprint / 31.4 MB heap）**同量级**：
> Linux 侧 shell-init 稳态堆约 **17.7 MB**（heaptrack 归因总量），峰值 27.8 MB（含编译期瞬时缓冲）。
> 差异来自 macOS 侧统计口径（`footprint` 含 malloc 碎片与 guard page）与 Linux 侧 glibc malloc 的行为差异。

### 4.2 进入主循环（含出生点 SPAWN_CHUNK_RADIUS=11 区块生成）

- **RSS 稳定在 87.4 MB**（t=2s 起即稳定，不随时间增长——无泄漏式增长）。
- 堆峰值 **42.1 MB** @ t=17.4s，稳态 **39.7 MB**。
- 峰值归因中区块相关项（`ChunkPrimer` 4.4+1.6+1.1=7.1 MB、`newInstance` 2.3 MB、
  `NoiseInterpolator` 1.3 MB、`FlatCache` 0.4 MB）合计约 **11 MB**，对应出生区 (2×11+1)² = 529 个区块。

**结论**：**空载基线（不含任何区块）在 Linux 上约 17–28 MB 堆 / 87 MB RSS**，
其中不可压的系统/运行时部分（RocksDB 16 列族、方块注册表、数据包资源、命令树、脚本引擎）占大头，
与 `MEMORY_IDLE_MACOS.md` 的结论一致：**空载侧可削空间有限，真正的战场在区块数据侧**。

---

## 五、优化靶点（按「收益 ÷ 风险」排序）

### 靶点 1 · 密度函数区块级实例共享维度级 Op 序列 —— **✅ 已落地 `a2f98b278`（峰值 −48.2 MB）**

**证据来自代码自身的注释。**

`CompiledDensityFunction::newInstance`（`src/server/world/gen/density/ast/CompiledDensityFunction.cpp:104`）
在每个区块创建时执行：

```cpp
std::vector<Op> newOps = m_ops;                    // ← 86.0 MB 的来源（深拷贝整个字节码）
std::vector<RuntimeObject> newObjects = m_objects; // ← 另一份深拷贝
for (auto& op : newOps) {
    if (op.code != OpCode::Marker) continue;
    ...
    newObjects[op.objIdx].densityFunction = ...;   // 只改 newObjects，从不写 op
}
```

而同一函数的注释（`:185`）明确写道：

> **「区块级 ops 与维度级字节相同（newInstance 只改 newObjects 缓存对象，不改 Op），
> 故直接复用维度级 `m_jitFn`」**

即：**`newOps` 与维度级 `m_ops` 逐字节相同**，这份深拷贝没有任何消费者。
`Op` 实测 `sizeof = 80 B`（4×f64 + 9×u32 + 1×u8，对齐到 8），
峰值时存活的 Op 缓冲合计约 **86 MB ≈ 110 万个 Op**。

**削减路径**：把 `m_ops` 从值成员改为 `std::shared_ptr<const std::vector<Op>>`，
区块级实例拷贝指针即共享维度级的 Op 缓冲。`m_jitFn` 本来就是共享的，Op 共享与之自洽。

**代价与前置**：
- `Op` 序列必须**真正不可变**——`const` 由编译器强制这一不变量。
- `m_evalCtx` 已指向本实例的 `m_objects/m_subEvaluators/m_splines`，与 Op 缓冲无关，不受影响。

> **已落地（`a2f98b278`）**：`m_ops` 改为 `shared_ptr<const std::vector<Op>>`，
> `GenContext::compile` / `makeConstantEvaluator` 建缓冲后转移所有权，
> `newInstance` 直接拷贝 shared_ptr（零字节复制）。实测：
>
> | 指标 | 改动前 | 改动后 | 差值 |
> |---|---:|---:|---:|
> | 生成峰值堆 | 207.6 MB | **159.4 MB** | **−48.2 MB（−23%）** |
> | `CompiledDensityFunction.cpp:106` 分配点 | 86.0 MB | **0**（条目消失） | −86.0 MB |
> | `newInstance` 子树合计 | 88.3 MB | 34.1 MB | −54.2 MB |
> | 生成后稳态 | 116.9 MB | **104.8 MB** | −12.1 MB |
> | 吞吐（8/32） | 190.0 chunks/s | 193.5 chunks/s | 无回归 |
>
> 降幅（48.2 MB）小于该分配点的 86.0 MB，因为 `newInstance` 子树的其余分配
> （`make_shared` 16.6 MB、`newObjects` 3.4 MB、`newSubEvaluators` 3.5 MB、
> Adapter/CacheOnce/Cache2D/FlatCache 约 11.4 MB）仍在；它们随区块数量线性增长，
> 见靶点 5/7 与 §六。
>
> **注意**：实际收益比预估的"−86 MB"小，因为估算是按"该分配点单独归零"算的，
> 而实测受"峰值时刻并非所有区块都持有 Op 缓冲"影响——共享后峰值时刻的存活量下降
> 不等于分配总量的下降。

### 靶点 2 · `NoiseInterpolator` 双 slice 缓冲 —— 收益 **不可削（已核实对齐原版）**，风险：—

`NoiseInterpolator`（`NoiseChunk.cpp:103-104`）构造时无条件分配两个扁平缓冲：

```cpp
m_slice0.assign(zPoints * yPoints, 0.0);   // zPoints=cellCountZ+1=42, yPoints=cellCountY+1=25
m_slice1.assign(zPoints * yPoints, 0.0);   // → 2 × 42 × 25 × 8 B = 16.8 KB / 个
```

8/32 实测 **46 208 次分配 / 24.8 MB**（`:103` 与 `:104` 各 23 104 次，即约 23 104 个 interpolator）。
注意基准因 `MemoryProfiler` 注册会**跑两遍**，故单轮约 11 552 个 interpolator / 1024 区块 ≈ **11.3 个/区块**。

**核实结论：两个 slice 都是必要的，且与原版一致。**
- `NoiseChunk::advanceCellX` 对**每一个** interpolator 调 `fillSlice(*this, false, ...)` 填充 `m_slice1`
  （`NoiseChunk.cpp:527`），`initializeForFirstCellX` 则填 `m_slice0`——**两个缓冲在每区块的
  每个 cellX 迭代中都被完整写入**，不存在"未使用的 slice"。
- MC 1.21.11 `NoiseChunk.NoiseInterpolator` 构造同样是
  `slice0 = allocateSlice(cellCountY, cellCountXZ); slice1 = allocateSlice(...)` 两份都分配
  （`NoiseChunk.java:703-704`），且 `allocateSlice` 用 `double[][]` 逐行 `new double[j]`
  （Java 侧是 `(cellCountXZ+1)` 个独立数组，**比 Cubium 的扁平单块更差**）。

**因此这一项不是缺陷，不应削减**（初版把它列为"−25 MB 可削"是误判，已作废）。
真正可评估的是 `m_cellCountZ` 参数的语义：Cubium 的 `NoiseInterpolator(filler, cellCountZ, cellCountY)`
第一个参数是 Z 方向点数，调用处传 `cellCfg.cellCountXZ`（`CompiledDensityFunction.cpp:137`），
两者在区块生成下同为 4，属**命名与语义错位**（原版 `allocateSlice(cellCountY, cellCountXZ)`
的第一个参数是 cellCountY）。若未来支持非方形 XZ/Y cell 配置，此处会出错——建议改名为
`cellCountXZ` 并加断言。**这是正确性问题，不是内存问题。**

### 靶点 3 · Heightmap —— **已实现，无剩余收益**

`ChunkData::m_heightmaps = std::array<Heightmap, HEIGHTMAP_TYPE_COUNT>`，实测
`sizeof(Heightmap) = 296 B`（`BITS=9`、`VALUES_PER_LONG=7`、`WORD_COUNT=37` → 37×8 B），
7 类型 = **2 072 B/区块** → 1024 区块共 **2.02 MB**。

**位压缩已经落地**：`Heightmap.hpp:120-127` 的 `BITS = ceilLog2(MAX_BUILD_HEIGHT - NO_BLOCK_SENTINEL + 1) = 9`、
`m_words` 为 `std::array<u64, WORD_COUNT>`——这正是原版 `SimpleBitStorage(9, 256)` 的等价实现。
`docs/MEMORY.md`（Windows，2026-09-25）记录的"`array<BlockCoord,256>` = 7 196 B/区块"
是**过时数据**，该报告写作时位压缩尚未落地。**本项无剩余收益，已作废。**

### 靶点 4 · `samplePreliminarySurfaceLevel` 缓存 —— 收益 **−4.7 MB（峰值 −2.3%）**，风险：低

`NoiseChunk::samplePreliminarySurfaceLevel`（`NoiseChunk.cpp:648`）用
`std::unordered_map<i64, i32> m_preliminarySurfaceLevelCache` 缓存预表面高度，
峰值实测 **4.7 MB / 534 976 次分配**。

**原版对照**：MC 1.21.11 用 fastutil 的 `Long2IntOpenHashMap`（`NoiseChunk.java:35`）——
开放寻址、无节点分配、`computeIfAbsent`。Cubium 的 `std::unordered_map` 每节点 32 B + 桶数组，
是这 4.7 MB 的主因。

**削减路径（已排除定长数组）**：初版曾建议改"定长数组（键为相对坐标的稠密索引）"。
**该方案不成立**——查询键**并非**只落在本区块的 4 方块网格内：
- `NoiseBasedAquifer::computeSubstance` 按 `SURFACE_SAMPLING_OFFSETS_IN_CHUNKS`
  查询 `x + offset[0]*16`（offset ∈ [-3, 1]），即**区块外 ±3 区块**（`NoiseBasedAquifer.cpp:362-366`）；
- `maxPreliminarySurfaceLevel` 在网格范围 `[minGridX, maxGridX]` 上迭代，网格按
  `gridX(minBlockX - 5)` / `gridX(maxBlockX - 5) + 1` 构造（对齐原版 `Aquifer.java:124`），
  同样越出本区块。

（实现过程中曾按"5×5 定长数组"改过一版，边界推导错误，已回退并改为此处记录的结论。）

**正确做法**：换成开放寻址的 `i64 → i32` 扁平表（线性探测，容量取 2 的幂，
对齐原版 `Long2IntOpenHashMap` 的语义），保留任意坐标键的通用性。收益约 −4.7 MB。

### 靶点 5 · `FlatCache` 预计算表 —— 收益 **待评估（峰值 −11.3 MB 上限）**，风险：中

`FlatCache::FlatCache(..., precompute=true)`（`DensityFunctions.hpp:998`）在构造期
双 for 填满 `m_values`，尺寸 `(sizeXZ+1)²`。峰值实测 **11.3 MB / 199 272 次**。

**对齐原版**：MC 1.21.11 `NoiseChunk.FlatCache` 同样在构造期预计算
（`NoiseChunk.java:619-637`，`sizeXZ = noiseSizeXZ + 1`、`values = new double[sizeXZ * sizeXZ]`）
——**这是对齐原版的行为**，不能简单删除。

**但要核对数量**：原版 `FlatCache` 只由 `wrapNew` 在遇到 `Marker.Type.FlatCache` 时按需创建
（`NoiseChunk.java:378`），且原版**没有"维度级编译产物"这一层**——每个区块的 NoiseChunk
各自持有一份 FlatCache。Cubium 的路径相同（`CompiledDensityFunction::newInstance` 的
`MarkerType::FlatCache` 分支），但需确认：数据包里 16 处 `flat_cache` 引用在编译后的
router 树上展开为**多少个**独立 Marker。实测 199 272 次分配 / 5832 primer ≈ **34 个/区块**，
若原版实际只有 6–10 个，说明有重复编译。

**此项需独立评估**（先统计原版每个 NoiseChunk 实际创建多少个 FlatCache），不宜与靶点 1–4 同期做。

### 靶点 6 · `PalettedContainer` 段外壳与空段 —— 收益 **需实测**，风险：中

实测 **24 576 个段槽位只有 9 689 个（39.4%）非空**，位存储仅 9.09 MB（均摊 983 B/段），
但每个 `ChunkSection` 对象 + `PalettedContainer` 的 palette 数组/哈希表本身有固定外壳。
**方向**：空段的 `unique_ptr<ChunkSection>` 已为 `nullptr`（不占位存储），
可评估的是**非空段的外壳开销**（`bits=1/2/3` 时 palette 数组仅 2–6 项，哈希表可能偏大）。

### 靶点 7 · 光照 nibble 的常量态 —— 实测收益 **−2.6 MB（稳态 −2.5%）**，风险：中

**这是初版报告严重高估的一项，实测后从"−20~40 MB"修正为 −2.6 MB。**

`SWMRNibbleArray` 每段 2 048 B，`LIGHT_SECTIONS = CHUNK_SECTIONS + 2 = 26`，
天空光 + 方块光共 52 槽位/区块 → 理论满配 106 KB/区块。初版据此推算"1024 区块满配 104 MB"，
并断言这是稳态最大单项。**实测推翻了这个推算**——nibble 是延迟分配的，绝大多数槽位根本没物化。

**实测（新增 `dumpNibbleStats` 诊断，`benchmark_results/nibble_stats/`）**：

| 项 | 8/32（1024 区块） | 占比 |
|---|---:|---:|
| 槽位总数 | 53 248（1024 × 52） | — |
| **已物化槽位** | **3 457** | **6.5%** |
| 物化总字节 | **6.75 MB** | — |
| 其中「全 15」（天空光全亮） | 1 323 槽位 / **2.58 MB** | 38% of 物化 |
| 其中「全 0」 | 32 槽位 / **0.06 MB** | 1% |
| 其余（有真实光照梯度） | 2 102 槽位 / 4.11 MB | 61% |

**关键数字**：nibble 的**边际成本只有 6.7 MB / 1024 区块 ≈ 6.7 KB/区块**
（由"1024 区块 − 64 区块"的稳态差分得出，见 §三）。

**原版确实有常量态、Cubium 确实没有**（这一点初版判断正确）：
- MC 1.21.11 `SkyLightSectionStorage.createDataLayer`（`:92-111`）返回
  `lightOnInSection(sec) ? new DataLayer(15) : new DataLayer()` —— `DataLayer(int)` 只设
  `defaultValue`，`data` 保持 null，**零字节分配**；`DataLayer.get()` 在 `data == null` 时
  直接返回 `defaultValue`（`DataLayer.java:38-43`）。
- Cubium 的 `SWMRNibbleArray::setFull()`（`SWMRNibbleArray.cpp:165-190`）**立即
  `_allocateBytes()` 分配 2 048 B 并 `std::fill(0xFF)`**。

**但收益只有 2.6 MB**，因为只有 38% 的物化槽位是全常量态，其余 61% 是真实的光照梯度
（不可常量化）。若把 1 323 个全 15 + 32 个全 0 槽位改为常量态，可省
**约 2.6 MB**（稳态 104.8 → 约 102 MB），另有减少 1 355 次 2 KB 分配带来的碎片收益（未量化）。

> **注意**：本项是"对齐原版"的改动（原版确实不物化常量段），但**收益远小于初版预期**。
> 优先级应低于靶点 8（arena 池化）与靶点 5（FlatCache 数量核实）。

**改动面**：`SWMRNibbleArray` 的 `State` 枚举扩展"全满常量态"，`get()`/`getUpdating()` 在
常量态直接返回 15（或 0），首次 `set()` 时才物化实体缓冲。需与 thread_local 池
（`POOL_CAPACITY_PER_THREAD`）的分配/释放路径协同，并覆盖 `updateVisible`/`toByteArray`/
`copyVisibleTo`/`getSaveState` 等导出路径。

### 靶点 8 · 段/区块的**预分配池化** —— 收益 **潜在 −10~20 MB**，风险：中-高

由"1024 − 64 区块"的稳态差分得出的**每区块边际成本约 68.5 KB**，构成见 §三 3.2。

**前两项（33.8 MB）都是"每区块多次独立堆分配"**：`ChunkPrimer` ctor 的
`make_shared<ChunkData>` + `BiomeContainer` + `array<Heightmap,7>` 是三次独立分配，
`PalettedContainer` 的 storage/palette/hashMap 又是三次。按 `MEMORY_IDLE_MACOS.md` §二的
推论——**malloc 碎片与小对象基数正相关**——改为**单次 arena 分配**（一个区块一块连续内存）
可同时降低分配次数与碎片。

> **但本项的前提在 Linux 侧未经验证**：本报告全部数据在 WSL2/glibc 下测得，
> 而 macOS 报告的核心论据（碎片与节点数正相关）是 **macOS small zone 特有**的行为。
> glibc 的 arena/tcache 碎片机制不同，**arena 池化在 Linux 上的收益可能远小于 macOS**。
> 要判断它值不值，须先在 Windows 或 macOS 上重测同一负载。
> 故本项虽理论收益大，**性价比存疑，需先量化再决策**。

### 靶点 9 · 非 FULL primer 的 `biomes`/`heightmaps` 提前释放 —— 收益 **−9.3 MB（稳态 −8.9%）**，风险：中

见 §三 3.4 的 primer 状态普查：**1892 个非 FULL primer 全部仍持有
`BiomeContainer`（3 072 B）+ `Heightmap[7]`（2 072 B）**，因为 `m_biomes`/`m_heightmaps`
的 `reset` 只发生在 FULL 的 `toChunkData()` 里。

**收益**：1892 × 5 144 B = **9.28 MB**。

**这是本轮排查中收益最大的稳态可削项**（高于 nibble 常量态的 2.6 MB）。
但**实现前必须确认一个前提**：

- 光环 primer 停在 `biomes`/`carvers`/`initialize_light` 状态，**后续推进时是否还需读自己的 `m_biomes`**？
  `biomes` 状态由 `fillBiomesFromNoise` 写入；`surface`/`carvers` 经 `getBiomeAtBlock` 读。
- 若写入后即转入 `ChunkData`（而不是等 FULL），则 `m_biomes` 从 `biomes` 状态之后就不再需要。
- 但 `m_heightmaps` 不同：生成期高度图读取（`ChunkPrimer::getTopBlockY`）**依赖 primer 侧副本**
  （`ChunkData::m_heightmaps` 在生成期不被维护，见 `ChunkData.hpp` 的 `_setBlockStateUnlockedGen` 注释），
  故 `m_heightmaps` 只能等 `primeHeightmaps(POST_FEATURES)` + 后续全量重建之后才可释放——
  即 **FEATURES 之后**，而不是 `biomes` 之后。

**结论**：`m_biomes` 的提前释放可行且收益 5.5 MB；`m_heightmaps` 需推到 FEATURES 之后
（收益 3.7 MB）。**两者都需先做一次"光环 primer 的读写审计"确认无遗漏读取点。**

### 已排除 / 需注意

- **`ChunkPrimer` 与 `NoiseChunk` 的释放时机已是正确实现**：`releaseGenOnlyData` 在
  CARVERS 之后 `m_noiseChunk.reset()`（`ChunkPrimer.cpp:566`），生成期数据不残留到稳态。
  **不要**误以为"生成期内存常驻"而去改这里。
- **稳态 104.8 MB 中不可削的部分**：1024 个区块的 `ChunkData` 本体是"区块已加载"的
  语义要求，只能通过靶点 6/7/8/9 的结构性压缩来削，不能靠懒加载消除。
- **`ChunkPrimer` 在 FULL 后被刻意保留**（`ChunkProgressionTask.cpp:263-267`：邻居
  `getChunkIfPresentUnchecked` 仍可返回有效指针），直到 holder 卸载。这不是缺陷。
- **`ServerTickList` 的 6.8 MB 已确认是基准口径产物**（§三 3.5 实测），不计入常规稳态。

---

## 六、"内存砍半"的路径评估

以 8/32 为口径（**靶点 1 已落地**）：

```
生成峰值  207.6 → 159.4 MB（已落地 −48.2 MB）
├── 靶点 1（Op 共享）        −48.2 MB  ✅ 已落地 a2f98b278
├── 靶点 4（surface 缓存）    −4.7 MB   ← 改开放寻址扁平表
├── 靶点 5（FlatCache 数量）  −? MB     ← 先核实原版数量，上限 −11.3 MB
└── 靶点 8（arena 池化）     −10~20 MB ← Linux 侧收益存疑，需先量化
                     合计   约 −75~84 MB → 约 76~84 MB（−60~64%）
```

```
生成后稳态  104.8 MB（含 6.8 MB 基准口径的 tick 条目；真实稳态约 98 MB）
每区块边际成本 ≈ 68.5 KB，其中：
├── 靶点 9（非 FULL primer 提前释放） −9.3 MB ← 本轮新增，收益最大的稳态可削项
├── 靶点 7（光照常量态）             −2.6 MB ← 实测（初版高估为 −20~40 MB）
├── 靶点 6（段外壳）                待实测
└── 靶点 8（arena 池化）            −? MB   ← Linux 侧收益未验证，需先量化
                     合计（确定项）  约 −12 MB → 约 93 MB（−11%）
```

**结论**：
1. **生成峰值已降 23%**（靶点 1）。要到 −50% 需再叠加靶点 4 + 5。
2. **生成后稳态的瓶颈是"primers 数量 × 每 primer 的独立堆分配"**，不是光照 nibble：
   - `ChunkPrimer` ctor 22.8 MB（33%）——由 **primers 数是目标区块的 2.85 倍** 与
     **非 FULL primer 仍持有 biomes/heightmaps** 共同造成（§三 3.4）；
   - `PalettedContainer` 存储 11.0 MB（16%）——位存储本体，必需；
   - 光照 nibble 只有 6.7 MB（10%），初版把它当稳态最大单项是误判（靶点 7）。
3. **本轮排查新增的最大稳态可削项是靶点 9（非 FULL primer 提前释放）9.3 MB**，
   其次是靶点 7（nibble 常量态）2.6 MB。
4. **靶点 8（arena 池化）的收益在 Linux 侧未经验证**——macOS 报告的核心论据
   （碎片与节点数正相关）是 macOS small zone 特有，glibc 下可能不成立，需先量化。
5. **空载基线（17–28 MB 堆）不是主战场**：构成以数据包资源、方块/物品注册表、命令树、
   脚本引擎为主，多为"启动即可查"的原版语义要求，可削空间有限
   （与 `docs/MEMORY_IDLE_MACOS.md` §5.3 的结论一致）。

---

## 七、测量方法学与陷阱（复用要点）

1. **heaptrack 是 Linux 侧的首选工具**：`heaptrack --record-only -o X.raw <cmd>` 记录，
   `heaptrack_print -f X.raw.gz -p 1 -n N -s 1` 输出**按调用栈归因**的峰值消费。
   - `-p` 按峰值字节、`-a` 按调用次数、`-l` 按泄漏、`-T` 按临时分配。
   - **`-m 0`（不合并调用栈）是归因的前提**：默认合并会把不同分配点折叠成一行，
     无法定位到具体代码行。
2. **`heaptrack` 的 `-M`（massif 输出）在 1.5.0 上有 bug**：`heap_tree=detailed` 块只有
   `n1` 根节点 + `n0: 0 in N places, all below threshold`，**不输出真实子树**，
   无论 `--massif-threshold` 调到多低。**不要依赖 massif 做稳态归因**。
   **稳态归因的正确做法是"冻结快照"**（见 §三 3.1）：在稳态时刻 `std::_Exit(0)`
   跳过全部析构，再用 `--flamegraph-cost-type leaked` 导出，此时"未释放"即稳态存活。
   配合 64 / 1024 区块两次冻结的**差分**可消除进程固定开销，得到每区块边际成本。
3. **区分"峰值"与"稳态"**：massif 时间序列的 `mem_heap_B` 可以直接读出两者
   （8/32：t=14.8s 峰值 203 MB，t=20.3s 稳态 116 MB）。峰值含生成中间态
   （NoiseChunk + 密度求值器），稳态只有 `ChunkData`。
4. **基准进程的 `MemoryProfiler` 口径 ≠ 进程堆口径**：`mc_benchmark` 的
   `max_bytes_used`/`net_heap_growth` 是"Start/Stop 区间内累计分配 − 累计释放"，
   8/32 报 30.9 MB；而 heaptrack 报进程堆峰值 207.6 MB。**两者相差近 7 倍**，
   因为前者是**单次迭代的净增长**（每次迭代末都卸载了区块），后者是**进程总堆**。
   比较不同来源的数字前必须先确认口径。
5. **构建必须显式关 Tracy**：`MC_ENABLE_TRACY` 默认 OFF（`CMakePresets.json` 的 `base` preset），
   Linux 上 Tracy 若开启会引入 rpmalloc（按 4 MiB 为单位 `VirtualAlloc`），污染内存口径。
   本报告全部数据在 **Tracy OFF** 下测得。
6. **WSL2 的 git fetch 走 Windows 代理会失败**（GnuTLS/HTTP2 断流），
   但 `curl https://github.com` 正常。需要更新 WSL 仓库时，改用
   `git remote add localwin /mnt/e/dev/minecraft-reborn-branch-1 && git fetch localwin main`
   从 Windows 侧本地仓库直取，绕开网络。
7. **基准的"跑两遍"会放大 heaptrack 的计数类数字**：`mc_benchmark` 注册了 `MemoryProfiler`，
   google/benchmark 会为每次重复额外跑一遍基准函数采集指标。因此 heaptrack 报出的
   **调用次数**（如 `NoiseInterpolator` 的 23 104 次）约为单轮的 2 倍，
   据此推算"每区块个数"时须**先除以 2**。**字节数不受影响**——两遍的分配在时间上不重叠，
   峰值时刻只统计当时存活的分配。
8. **"每区块 KB"用 64/1024 两次冻结的差分实测，不再靠结构性推算**：早期版本的
   §三 用 `sizeof` 推导逐项字节（并因此沿用了 `docs/MEMORY.md` 的过时 `Heightmap`
   数据，误报 7.2 MB）。现改用稳态冻结 + 差分（见 §三 3.1），得到的是**分配点级**的
   实测归属，不再需要具名分配器。

---

## 八、复现命令

```bash
cd ~/dev/Cubium/branch-1
export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh"          # 构建期烘焙脚本需要 node

# 配置 + 构建（linux-relwithdebinfo：Tracy OFF、Perfetto ON）
./scripts/configure.sh
cmake --build --preset linux-relwithdebinfo -- -j14 mc_benchmark minecraft-server

# 基准吞吐 + 内置内存指标
./build/bin/RelWithDebInfo/mc_benchmark --benchmark_filter=ChunkGeneration --benchmark_min_time=0.3s

# 生成峰值归因（heaptrack）
heaptrack --record-only -o /tmp/ht.raw \
  build/bin/RelWithDebInfo/mc_benchmark --benchmark_filter="ChunkGeneration/8/32" --benchmark_min_time=0.5s
heaptrack_print -f /tmp/ht.raw.gz -m 0 -p 1 -n 400 -s 1 > /tmp/peak.txt

# 时间序列（峰值 vs 稳态）
heaptrack_print -f /tmp/ht.raw.gz -p 0 -a 0 -T 0 -l 0 -n 0 \
  -M /tmp/massif.txt --massif-detailed-freq 1 > /dev/null

# 稳态归因（冻结快照 + 64/1024 差分）—— massif 子树在本版本不可用，用这个代替
MC_BENCH_FREEZE_STEADY=1 heaptrack --record-only -o /tmp/frz1024.raw \
  build/bin/RelWithDebInfo/mc_benchmark --benchmark_filter="ChunkGeneration/8/32" --benchmark_min_time=0.3s
MC_BENCH_FREEZE_STEADY=1 heaptrack --record-only -o /tmp/frz64.raw \
  build/bin/RelWithDebInfo/mc_benchmark --benchmark_filter="ChunkGeneration/8/8" --benchmark_min_time=0.3s
for f in /tmp/frz1024 /tmp/frz64; do
  heaptrack_print -f $f.raw.gz -p 0 -a 0 -T 0 -l 0 -n 0 \
    --flamegraph-cost-type leaked -F $f.leak.txt > /dev/null
done
# 两者差分即每区块边际成本（脚本见本报告 §三 3.2 的推导）

# 逐段诊断（基准自带 CSV）：palette_bits 与 nibble_stats
# 输出在 benchmark_results/{palette_bits,nibble_stats}/

# 服务端空载（不生成区块 / 进主循环）
heaptrack --record-only -o /tmp/srv.raw \
  build/bin/RelWithDebInfo/minecraft-server --profiler_enabled=false --benchmark_exit_after_shell_init
heaptrack --record-only -o /tmp/srv2.raw \
  build/bin/RelWithDebInfo/minecraft-server --profiler_enabled=false   # 45s 后 SIGTERM
```
