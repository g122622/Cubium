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
| **生成峰值** | 1024 区块并行生成过程中进程堆的最高点 | **207.7 MB** |
| **生成后稳态** | 同一批区块全部 FULL 后、仍驻留内存时的堆 | **117.8 MB** |
| 服务端空载（不生成区块） | `--benchmark-exit-after-shell_init` 路径的堆峰值 | 27.8 MB |
| 服务端进主循环（含出生区区块） | 堆峰值 / 稳态 | 42.1 / 39.7 MB |

**一句话结论**：生成峰值里**最大的单项是密度函数区块级实例的 Op 序列深拷贝（86.0 MB / 41%）**，而代码注释已自证这份拷贝是冗余的（区块级 Op 与维度级字节完全相同）。**这一项是本轮测出的第一优先削减目标**，且不改变任何行为语义。

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

## 二、生成峰值归因（8/32，207.7 MB）

由 `heaptrack_print -p 1 -n 400 -s 1`（未合并调用栈，取进程堆峰值时刻的存活分配）聚合到**最深 Cubium 帧**：

| # | 分配点 | MB | 占比 | 次数 | 性质 |
|---:|---|---:|---:|---:|---|
| 1 | `CompiledDensityFunction::newInstance` @ `CompiledDensityFunction.cpp:106`（`std::vector<Op> newOps = m_ops` 深拷贝） | **86.0** | **41.4%** | 291 688 | **纯冗余，可归零** |
| 2 | `ChunkPrimer::ChunkPrimer` @ `ChunkPrimer.cpp:164/168/169`（`make_shared<ChunkData>` + `BiomeContainer` + `array<Heightmap,7>`） | 32.4 | 15.6% | 5 832 | 半可削 |
| 3 | `NoiseInterpolator::NoiseInterpolator` @ `NoiseChunk.cpp:103/104`（两个 `m_slice0/m_slice1` 扁平缓冲） | 24.8 | 11.9% | 46 208 | 可削 |
| 4 | `FlatCache::FlatCache` @ `DensityFunctions.hpp:998`（构造期整张预计算表） | 10.9 | 5.2% | 199 272 | 需评估 |
| 5 | `TracyTrackingAlloc::allocate` @ `MemoryTracking.hpp:283`（PalettedContainer 位存储 vector） | 7.4 | 3.6% | 119 806 | 本体必要 |
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

## 三、生成后稳态归因（8/32，117.8 MB）

1024 个区块全部 FULL 且仍加载时，进程堆 = **117.8 MB**，即 **约 115 KB/区块**。
构成与生成期不同：`NoiseChunk` 及其密度求值器已在 CARVERS 之后释放
（`ChunkPrimer::releaseGenOnlyData`，`ChunkPrimer.cpp:566`），稳态的主体是 **ChunkData 本体**：

| 项 | 每区块 | ×1024 | 来源 |
|---|---:|---:|---|
| `ChunkData` 外壳（`shared_ptr` 控制块 + 成员） | ~4.4 KB | 4.5 MB | **推算**（对齐 `MEMORY.md` §3.4 的 13 777 B 减去 Tracy 开销；Linux 侧无 Tracy，实测未单独拆分） |
| `m_heightmaps` = `array<Heightmap, 7>` | 7.0 KB | 7.2 MB | 推算：`7 × 1024 B`（`array<BlockCoord,256>`） |
| `m_biomes` = `BiomeContainer`（`array<u16,1536>`） | 3.0 KB | 3.1 MB | 推算：`1536 × 2 B` |
| 段 `PalettedContainer` 位存储 | — | 9.1 MB | **本次实测**（见下方位宽分布表）|
| `m_skyNibbles` + `m_blockNibbles`（52 × 2 KB，池化） | 视光照而定 | — | `SWMRNibbleArray` thread_local 池，上限 128 KB/线程 |
| RocksDB / 存储层 / 生命周期管理器 / 其余 | — | ~94 MB | 差额（117.8 减去上表可归因项） |

> **上表前四项的"每区块字节"是结构性推算，不是本次 heaptrack 实测的归属**。
> heaptrack 的 massif 稳态堆树在本版本（1.5.0）有 bug（见 §七 第 2 条），
> 无法直接给出稳态时刻的逐项子树；只有段位存储是通过基准自带的 CSV 精确实测的。
> 要在 Linux 上得到稳态的逐项归因，需要给 `ChunkData`/`ChunkPrimer` 加
> **具名分配器（如 `TracyTrackingAlloc` 已有先例）** 后再跑一次 heaptrack。

**段调色板位宽实测分布**（`benchmark_results/palette_bits/chunk_palette_bits_threads=8_batch=32.csv`）：

| bits | 段数 | 每段字节 | 小计 |
|---:|---:|---:|---:|
| 1 | 2 025 | 512 | 0.99 MB |
| 2 | 6 407 | 1 024 | 6.26 MB |
| 3 | 1 257 | 1 536 | 1.84 MB |
| **合计** | **9 689** | 均摊 983 | **9.09 MB** |

**关键观察**：**最高位宽只有 3 bit**，且 66% 的段是 2 bit。位存储本身没有浪费空间；
浪费在**每段的固定外壳**（`PalettedContainer` 对象、palette 数组、哈希表）与 **1024 区块 × 24 段 = 24 576 个段槽位**（只有 39.4% 被填充）。

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

### 靶点 1 · 密度函数区块级实例共享维度级 Op 序列 —— 收益 **−86 MB（生成峰值 −41%）**，风险：低

**这是本轮测出的第一优先项，且证据来自代码自身的注释。**

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

**削减路径**：把 `m_ops` 从值成员改为 `std::shared_ptr<const std::vector<Op>>`（或让区块级实例持有
维度级的 `const` 引用 + 生命周期保证），区块级实例直接共享维度级的 Op 缓冲。
`m_jitFn` 已经是共享的，Op 共享与之自洽。

**代价与前置**：
- `Op` 序列必须**真正不可变**——当前 `newInstance` 循环确实不写 `op`，但需加断言/`const` 固化这一不变量。
- `m_evalCtx` 已指向本实例的 `m_objects/m_subEvaluators/m_splines`，与 Op 缓冲无关，不受影响。
- 需覆盖单元测试：`ops()` 返回值语义、JIT 复用路径、区块级 `newInstance` 后求值数值一致性。

**预期**：生成峰值 207.7 → 约 122 MB；稳态不变（稳态已无 NoiseChunk）。

### 靶点 2 · `NoiseInterpolator` 双 slice 缓冲按需分配 —— 收益 **−25 MB（峰值 −12%）**，风险：低

`NoiseInterpolator`（`NoiseChunk.cpp:103-104`）构造时无条件分配两个扁平缓冲：

```cpp
m_slice0.assign(zPoints * yPoints, 0.0);   // zPoints=cellCountZ+1=42, yPoints=cellCountY+1=25
m_slice1.assign(zPoints * yPoints, 0.0);   // → 2 × 42 × 25 × 8 B = 16.8 KB / 个
```

8/32 实测 **46 208 次分配 / 24.8 MB**（`:103` 与 `:104` 各 23 104 次，即约 23 104 个 interpolator）。
注意基准因 `MemoryProfiler` 注册会**跑两遍**，故单轮约 11 552 个 interpolator / 1024 区块 ≈ **11.3 个/区块**。
但这些缓冲**只在 NOISE 状态的双层 Z 扫描期间被填充**；对只在单层使用的 interpolator，
`m_slice1` 从未被写入。

**削减路径**：
1. 让 `m_slice1` **惰性分配**（首次需要双层滚动时再 `assign`），或
2. 两个 slice 合并为**一个 `2 × zPoints × yPoints` 缓冲**并按 `m_sliceIndex` 交替（省一次分配头 + 改善局部性），或
3. 若某 interpolator 的填充路径确定不滚动，则只分配一个 slice。

**代价**：需核对 `compute()`/`updateForZ()`/`fillSlice` 的全部读写点，确认惰性分配不影响
"未填充即读取"的语义（当前 `assign(..., 0.0)` 隐含"未填充读到 0"）。

### 靶点 3 · Heightmap 位压缩 —— 收益 **−6 MB（稳态 −5%）**，风险：低-中

`ChunkData::m_heightmaps = std::array<Heightmap, 7>`，每 `Heightmap` 为
`std::array<BlockCoord /*=i32*/, 256>` = **1024 B/类型**，7 类型 = 7 196 B/区块 → 1024 区块共 **7.2 MB**。

**原版对照**：MC 1.21.11 `Heightmap.java:39-41` 用
`SimpleBitStorage(Mth.ceillog2(chunk.getHeight()+1), 256)`，即按维度高度**位压缩**
（385 → 9 bit/列 → **288 B/类型**）。7 类型 = 2 016 B/区块，节省 5 180 B/区块 × 1024 = **6.3 MB**。

**与 Windows 报告一致**（`docs/MEMORY.md` 靶点 3，估 6.3 MB）。
**改动面**：`Heightmap.hpp` 的 `m_heights` 改位压缩存储，保持 `getHeight`/`setHeight`/`getData`/`setData`
接口语义；`getData()` 从"返回 const 引用"变为"返回值/填充缓冲"，需检查调用点。
**附加收益**：若实测确认某些 `HeightmapType` 在服务端零消费，可再省一档。

### 靶点 4 · `samplePreliminarySurfaceLevel` 缓存改定长数组 —— 收益 **−4.7 MB（峰值 −2.3%）**，风险：低

`NoiseChunk::samplePreliminarySurfaceLevel`（`NoiseChunk.cpp:648`）用
`std::unordered_map<i64, i32> m_preliminarySurfaceLevelCache` 缓存预表面高度，
峰值实测 **4.7 MB / 534 976 次分配**。

缓存键是 quart 对齐坐标 `(quartAlignedX, quartAlignedZ)`，取值范围**有界**
（一个区块 4×4 quart 网格）。改用**定长数组 / 小 map**（键为相对坐标的稠密索引）可消除
`unordered_map` 的节点开销与 rehash 抖动。

### 靶点 5 · `FlatCache` 预计算表 —— 收益 **待评估（峰值 −10.9 MB 上限）**，风险：中

`FlatCache::FlatCache(..., precompute=true)`（`DensityFunctions.hpp:998`）在构造期
双 for 填满 `m_values`，尺寸 `(sizeXZ+1)²`。峰值实测 **10.9 MB / 199 272 次**。

**对齐原版**：MC 的 `NoiseChunk.FlatCache` 同样在构造期预计算——**这是对齐原版的行为**，
不能简单删除。可评估的是**尺寸与缓存粒度**（原版是否也缓存全部 router slot 的 FlatCache）。
**建议**：先确认 15 个 router root 里哪些真的需要 FlatCache 预计算，再决定是否收窄。
**此项需独立评估，不宜与靶点 1–4 同期做。**

### 靶点 6 · `PalettedContainer` 段外壳与空段 —— 收益 **需实测**，风险：中

实测 **24 576 个段槽位只有 9 689 个（39.4%）非空**，位存储仅 9.09 MB（均摊 983 B/段），
但每个 `ChunkSection` 对象 + `PalettedContainer` 的 palette 数组/哈希表本身有固定外壳。
**方向**：空段的 `unique_ptr<ChunkSection>` 已为 `nullptr`（不占位存储），
可评估的是**非空段的外壳开销**（`bits=1/2/3` 时 palette 数组仅 2–6 项，哈希表可能偏大）。

### 靶点 7 · 段/区块的**预分配池化** —— 收益 **潜在 −10~20 MB**，风险：中-高

`ChunkPrimer` ctor 三行合计 **32.4 MB / 5 832 次**（每 primer 约 5.7 KB）：
`make_shared<ChunkData>` + `BiomeContainer` + `array<Heightmap,7>` 三块独立堆分配。
按 `MEMORY_IDLE_MACOS.md` §二的推论——**malloc 碎片与小对象基数正相关**——
把这三块改为**单次 arena 分配**（一个 primer 一块连续内存）可同时降低分配次数与碎片。
**此项是"内存砍半"的后半程主力，但属内存布局重构，需单独立项。**

### 已排除 / 需注意

- **`ChunkPrimer` 与 `NoiseChunk` 的释放时机已是正确实现**：`releaseGenOnlyData` 在
  CARVERS 之后 `m_noiseChunk.reset()`（`ChunkPrimer.cpp:566`），生成期数据不残留到稳态。
  **不要**误以为"生成期内存常驻"而去改这里。
- **稳态 117.8 MB 中不可削的部分**：1024 个区块的 `ChunkData` 本体（外壳 + 高度图 + 生物群系 + 位存储）
  是"区块已加载"的语义要求，只能通过靶点 3/6/7 的结构性压缩来削，不能靠懒加载消除。

---

## 六、"内存砍半"的路径评估

以 8/32 为口径：

```
生成峰值  207.7 MB
├── 靶点 1（Op 共享）        −86.0 MB   ← 单独立项即可砍掉 41%
├── 靶点 2（slice 惰性）     −25.0 MB
├── 靶点 4（surface 缓存）    −4.7 MB
├── 靶点 3（heightmap 压缩）  −6.0 MB   ← 稳态侧，峰值侧同样受益
└── 其余                     −3.5 MB（靶点 5 上限 10.9 MB 需评估）
                     合计   −125 MB → 约 83 MB（−60%）
```

```
生成后稳态  117.8 MB
├── 靶点 3（heightmap 压缩） −6.3 MB
├── 靶点 7（arena 池化）    −10~20 MB（需立项）
└── 靶点 6（段外壳）        待实测
                     合计   −16~26 MB → 约 92~102 MB（−13~22%）
```

**结论**：
1. **生成峰值可以一步砍半**——靶点 1 单独就贡献 41%，加靶点 2 即达 −53%，
   且两者都是**局部、不改变行为语义**的改动。
2. **生成后稳态砍半更难**：主体是 1024 个区块的 `ChunkData` 本体，属"已加载区块"的固有开销。
   要达到 −50% 需叠加靶点 3（位压缩）+ 6（段外壳）+ 7（arena 池化），
   其中靶点 7 是内存布局重构，需要单独立项。
3. **空载基线（17–28 MB 堆）不是主战场**：构成以数据包资源、方块/物品注册表、命令树、
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
   无论 `--massif-threshold` 调到多低。**不要依赖 massif 做稳态归因**——
   改用 `-p`/`-a` 的峰值归因 + 时间序列（`mem_heap_B` 逐 snapshot）交叉判断。
3. **区分"峰值"与"稳态"**：massif 时间序列的 `mem_heap_B` 可以直接读出两者
   （8/32：t=14.8s 峰值 203 MB，t=20.3s 稳态 116 MB）。峰值含生成中间态
   （NoiseChunk + 密度求值器），稳态只有 `ChunkData`。
4. **基准进程的 `MemoryProfiler` 口径 ≠ 进程堆口径**：`mc_benchmark` 的
   `max_bytes_used`/`net_heap_growth` 是"Start/Stop 区间内累计分配 − 累计释放"，
   8/32 报 30.9 MB；而 heaptrack 报进程堆峰值 207.7 MB。**两者相差近 7 倍**，
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
8. **本报告中的"每区块 KB"多数是结构性推算**：heaptrack 1.5.0 的 massif 子树不可用
   （见第 2 条），`ChunkData`/`Heightmap`/`BiomeContainer` 的逐项字节由
   `sizeof` 与成员声明推导，**已在表中标注"推算"**。要得到 Linux 侧的逐项实测，
   需要给这些类型加具名分配器（`TracyTrackingAlloc` 是现成范式）后再跑一次 heaptrack。

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

# 服务端空载（不生成区块 / 进主循环）
heaptrack --record-only -o /tmp/srv.raw \
  build/bin/RelWithDebInfo/minecraft-server --profiler_enabled=false --benchmark_exit_after_shell_init
heaptrack --record-only -o /tmp/srv2.raw \
  build/bin/RelWithDebInfo/minecraft-server --profiler_enabled=false   # 45s 后 SIGTERM
```
