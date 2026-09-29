# Cubium 服务端空载内存基线报告（macOS / 无玩家）

> 初次测量：2026-09-29（Commit 53d26f4c3）　修订：2026-09-29（Commit b69d8179f）
> 测量方法：`footprint` 取物理足迹 + `vmmap --summary` 分区域 + `heap <pid>` 全量节点归因
> （对照运行用 `MallocStackLogging=1` 让 `heap` 从分配回溯反推 `non-object` 的类型名）
> 对照文档：`docs/MEMORY.md`（Windows / viewDistance=16 / 满负载基线，2026-09-25）

---

## 零、修订记录：模板懒加载落地后的实测

`b69d8179f` 把 jigsaw 拼图块的模板解析从构造期推迟到首次访问（见 §3.2），
`73d105360` 把 spdlog 异步队列从 2048 条缩到 256 条（见 §3.9）。两项落地后重测：

| 指标 | 修订前 | 修订后 | 差值 |
|---|---:|---:|---:|
| 物理足迹（`--profiler_enabled=false`） | 67–74 MB | **49 MB** | **−20 MB** |
| `heap` 总量 | 44.1 MB | **31.4 MB** | **−12.7 MB** |
| 其中 `non-object`（裸缓冲） | 32.9 MB | 21.2 MB | −11.7 MB |
| 结构模板相关 | 9.1 MB | **0**（未触发解析） | −9.1 MB |
| NBT 标签 | 3.66 MB | **0**（随模板解析一并消失） | −3.66 MB |
| Jigsaw 装配 | 1.14 MB | 0.43 MB | −0.71 MB |
| spdlog 队列 | 0.81 MB | 0.10 MB | −0.71 MB |

**注意 −20 MB 的足迹降幅大于 −12.7 MB 的堆降幅**：消失的 1 100 余个模板对象与
6 万个 NBT 节点同时消去了它们推高的 malloc 碎片（macOS small zone 碎片率与节点数正相关，
见 §二）。这印证了 §五的结论——**减少小对象的"个数"比减少"总字节"更有效**。

**残留的三大项**（占修订后堆的 56%）：
`BlockState` 3.13 MB + 密度函数 7.52 MB + RocksDB 4.50 MB。其中密度函数与 RocksDB
见 §3.4/§3.7 的削减路径；`BlockState` 是原版方块状态空间的直接映射，不可削减。

---

## 一、结论摘要（修订前基线，保留供对照）

| 场景 | 物理足迹 | 说明 |
|---|---:|---|
| 空载，默认启动 | **95–99 MB** | 含 Perfetto 运行时 |
| 空载，`--profiler_enabled=false` | **67–74 MB** | 三次独立启动落在 67/68/74 MB |
| 空载，`--profiler_enabled=false` + `MallocStackLogging=1` | 99 MB | 供归因用，MSL 自身约 +4.3 MB 堆开销 |

**空载堆总量 44.2 MB**（`heap` 求和，不含 malloc 元数据与碎片）。堆之外的 30 MB 主要是
macOS malloc small zone 的碎片与保留页（15.9 MB 碎片 / 27–31% + 3.9 MB guard page），
这部分不随游戏数据增长，压不动。

**关于 40 MB 目标**：以修订前架构，`footprint < 40 MB` 等价于「堆降到约 15 MB 以下」，
即需砍掉现有堆的 65%。本文 §五 的逐项分析结论是**不可能**；§零 的实测证实了其中
最关键的一步（模板懒加载）的收益远超按字节的预估，因为它同时消去了碎片。
修订后 footprint 已到 **49 MB**，缺口 9 MB——继续推进 §3.4（密度函数）与 §3.7（RocksDB）
两项后有望进入 40 MB 区间。

---

## 二、物理足迹构成（修订前 74 MB）

| 区域 | Dirty | 占比 | 性质 |
|---|---:|---:|---|
| Malloc Small（heap 实际 + 碎片） | 60.5 MB | 82% | 其中实际分配 44.2 MB、碎片 15.9 MB（27%）、guard page 3.9 MB |
| App-Specific Tag 1 | 4.0 MB | 5% | dyld / 系统 VM region（非 malloc） |
| Malloc Large | 2.1 MB | 3% | 单块 ≥ 8 KB 的分配（RocksDB arena、大 vector） |
| `__DATA`（dirty） | 1.6 MB | 2% | 全局/静态变量写入过的页 |
| Malloc Metadata | 1.0 MB | 1% | 分配器自身元数据（349 031 个节点） |
| Stack | 0.9 MB | 1% | 49 个线程的栈已触达页 |
| page table | 0.6 MB | 1% | |
| Malloc Tiny | 0.3 MB | 0.4% | |
| `__DATA_DIRTY` / `__OBJC_RW` / 其余 | ~0.5 MB | 1% | |

```
74 MB footprint
├── 60.5 MB  malloc zone dirty
│   ├── 44.2 MB  实际分配（本文后续全部按此口径分析）
│   ├── 15.9 MB  malloc 碎片（31%，macOS small zone 固有；349 031 节点的高基数放大）
│   └──  3.9 MB  guard page（4 个 1 MB 保护区）
├──  4.0 MB  App-Specific Tag 1（dyld/系统，非 malloc）
└──  9.5 MB  元数据 + 栈 + 页表 + __DATA + Malloc Large
```

**关键推论**：`heap` 报出的 349 031 个 malloc 节点本身会推高碎片——碎片量与节点数正相关
（每节点带 16–48 B 元数据与对齐余量）。**减少小对象的"个数"比减少"总字节"更能压低 footprint**。

---

## 三、堆内构成（44.2 MB，按分配点归因）

归类由 `malloc in <函数>` 与 C++ 对象类型名共同判定；比例取自 MSL 运行（48.52 MB 总量，
含约 4.3 MB MSL 自身），按比例即适用于干净的 44.2 MB。

### 3.1 分类汇总

| # | 类别 | 字节 | 占比 | 节点数 | 削减潜力 |
|---|---|---:|---:|---:|---|
| A | **结构模板（Template）** | 8.40 MB | 17.5% | 11 242 | **高**（懒加载，−7 MB） |
| B | **密度函数（Density）** | 7.52 MB | 15.7% | 46 968 | 中（见 §3.4） |
| C | **方块注册表（Blocks）** | 6.96 MB | 14.5% | 76 512 | 低（核心数据，见 §3.5） |
| D | RocksDB（Storage） | 4.50 MB | 9.4% | 3 024 | 中（−2～3 MB） |
| E | NBT 标签（通用） | 3.66 MB | 7.6% | 61 765 | **高**（启动后应全部释放） |
| F | 生物群系（Biome） | 2.89 MB | 6.0% | 12 960 | 中（RTree 改扁平，−1.5 MB） |
| G | 进度（Advancement） | 1.81 MB | 3.8% | 15 049 | 中（懒加载） |
| H | 资源/数据包（Resource） | 1.51 MB | 3.1% | 21 075 | 低 |
| I | 配方（Recipe） | 1.35 MB | 2.8% | 6 879 | 中（懒加载） |
| J | `std::string`（通用） | 1.34 MB | 2.8% | 23 302 | 低（多为上述各项的附属） |
| K | 战利品表（Loot） | 1.18 MB | 2.5% | 15 136 | 中（懒加载） |
| L | Jigsaw 装配 | 1.14 MB | 2.4% | 11 151 | 中（见 §3.3） |
| M | 物品（Item） | 1.00 MB | 2.1% | 14 275 | 低 |
| N | 世界/区块（World） | 0.91 MB | 1.9% | 65 | 低（3 个维度各自的开销） |
| O | 日志（spdlog） | 0.82 MB | 1.7% | 45 | **高**（队列可缩容，−0.7 MB） |
| P | 噪声（Noise） | 0.71 MB | 1.5% | 2 363 | 低 |
| Q | 命令树（Command） | 0.43 MB | 0.9% | 3 277 | 低 |
| R | 其余 / 未归类 | 1.78 MB | 3.7% | 19 626 | — |
| | **合计** | **48.00 MB** | | 313 006 | |

### 3.2 A · 结构模板 8.40 MB —— 最大的单项

```
7.50 MB  2963 次  TemplateLoader::loadFromNbt        （模板 NBT 解析期缓冲）
1.48 MB 13233 次  vector<TemplateJigsawBlockInfo> / JigsawJoint
0.66 MB  4197 次  vector<TemplateJigsawBlockInfo>::__emplace_back_slow_path
0.14 MB  1972 次  TemplateManager::getTemplate        （unordered_map 节点）
```

**规模核实（三重印证）**：

| 量 | 值 | 来源 |
|---|---:|---|
| 数据包内 `.nbt` 结构文件 | 1 202 | 文件系统枚举 |
| `template_pool` JSON 实际引用的 distinct 模板 | 989 | 解析 188 个池文件 |
| `loadFromNbt` 分配次数 | 2 963 | `heap`（989 × 3，每模板 palettes/blocks/jigsaw 各一次） |
| `JigsawJoint` 拷贝后总量 | 5 755 | `heap` |
| 装配出的 `SingleJigsawPiece` + `LegacySingleJigsawPiece` | 1 225 | `heap`（622 + 603） |

**根因**：`TemplatePoolLoader` 在启动期遍历全部 188 个 `template_pool/*.json`，每个
`single_pool_element` 立即 `make_unique<SingleJigsawPiece>(...)`，其构造函数同步调用
`JigsawPiece::loadJointsFromTemplate` → `TemplateManager::getTemplate` → `loadFromNbt`，
把整个模板解析进内存并**永久缓存**（`m_templates` 无上限、无淘汰）。

**这是一次"启动期全量装配"**：与玩家是否走到那片结构无关，1 202 个结构文件里的 989 个
在服务端 accept 第一个连接之前就已全部驻留。

**原版对照**：MC 的 `SinglePoolElement` 只持有 `Either<Identifier, StructureTemplate>`，
**不预解析**；`getSize()` / `place()` 等每次经 `StructureTemplateManager.getOrCreate(id)`
**按需加载**（`StructureTemplateManager.java:94-101`）。原版的 `getOrCreate` 亦无上限淘汰，
但其**加载时机是首次访问**——正常游戏不会把 989 个模板全部触发。

**削减路径**：把 `SingleJigsawPiece` 的模板解析从构造函数推迟到首次真正需要
（`getSize()` / `place()`），`loadJointsFromTemplate` 随之下沉。直接收益 **−7.5 MB**，
且不改变任何行为语义（模板缓存在首次访问后仍长期有效）。
代价：`JigsawPiece::getJoints()` 从"恒可直接读"变为"可能需触发加载"，所有调用点需确认
其调用时机安全（装配期已进入生成 worker，需评估锁与 IO 路径）。

> **已落地（`b69d8179f`）**：实现方式是在 `JigsawPiece` 上加 `_ensureLoaded()` 虚钩子，
> `getJoints()`/`getSize()` 进入即调用；`SingleJigsawPiece` 覆写为"置位 m_loaded 后解析"，
> 模板缺失时也置位。实测 footprint −20 MB（远超按字节预估的 7.5 MB，因为它同时消去了
> 1 100 余个对象与 6 万个 NBT 节点推高的 malloc 碎片）。
>
> **数据包侧的并发安全性**：首次解析发生在区块生成 worker 内。`TemplateManager` 有互斥锁；
> `DataPackRepository` → `FolderResourcePack::readResource` 在 `initialize()` 建好内存索引后
> 只读打开文件，不触碰共享状态，故多 worker 并发触发安全。

### 3.3 B · Jigsaw 装配 1.14 MB

`JigsawJoint` 是**对 `TemplateJigsawBlockInfo` 的逐字段拷贝**：模板持有一份
（`Template::m_jigsawBlocks`），每个池元素又各持一份转写后的 `m_joints`
（`JigsawPiece.cpp:80-95`，12 个字段逐字段赋值）。5 755 个 joint 拷贝 + 989 个模板的
原始 `TemplateJigsawBlockInfo` 形成双份存储。

削减路径：让 `SingleJigsawPiece` 持有模板的 `ResourceLocation` + 索引区间，而非拷贝整份
joint 列表；或至少共享 `sourceName` / `targetPool` / `targetName` 三个 `std::string`
（当前每个 joint 都深拷贝一份，占该结构的大头）。收益约 −0.7 MB。

### 3.4 C · 密度函数 7.52 MB / 46 968 节点

```
3.31 MB   2287 次  vector<ast::Op>::push_back          （15 root × 3 维度的字节码）
1.63 MB   7461 个  shared_ptr_emplace<CompiledDensityFunction>
0.73 MB   3454 次  BytecodeGen::compile
0.44 MB   1136 次  vector<JigsawJoint>::reserve（见 §3.3）
0.43 MB   4164 次  GenContext::emitSpline
0.24 MB   5154 个  shared_ptr_pointer<DensityFunction*>
0.10 MB   3226 个  density::Marker
…（Marker/Constant/SharedTopology/TwoArgument/… 各 0.06–0.09 MB）
```

**7 461 个 `CompiledDensityFunction` 是每维度约 2 487 个**：15 个 router root，加上递归产生
的子求值器（样条每个常量值点一个 `makeConstantEvaluator`，`emitSpline` 为此产生 4 164 次
分配；嵌套子样条、`SharedSubtreeRef`、`FindTopSurface.density`、`Marker.delegate` 各自
独立编译）。

**注意**：这是**空载**数字——3 个维度（主世界/下界/末地）在启动时都已创建完整
`RandomState` 并 `compileRouter()`（`ServerDimensionManager.cpp:114/125/136`）。下界与末地
在没有玩家进入时也各持一份完整的密度函数编译产物。

**行 2287 次 `Op` push_back 的 3.31 MB 是字节码本体**，约 1 450 B/root。`Op` 结构本身
（`CompiledDensityFunction.hpp:232-247`）含 4 个 `f64` 立即数 + 9 个 `u32` + 1 个 `u8`，
按对齐后 56 字节：60 000 条指令左右。

**削减路径（需先量准）**：
1. 维度按需创建（下界/末地推迟到首个玩家进入）——若成立可省 2/3，约 −5 MB。**但这是行为
   改动**：维度未创建时 `dimension(NETHER)` 的语义、`getHeight` 等跨维度查询、末地龙战的
   初始化时机都需重新确认。
2. `Marker`/`Constant`/`SharedTopology` 等 AST 中间产物在编译完成后是否仍被引用：
   若 `CompiledDensityFunction` 已不依赖原始 AST，可在 `compileRouter()` 末尾释放
   `m_router` 的 AST 树，省下 `Marker` 0.10 MB + `Constant` 0.07 MB + `SharedTopology`
   0.06 MB + `TwoArgument` 0.09 MB + `CubicSpline` 0.08 MB + `ShiftNoise` 0.08 MB
   + `ShiftedNoise` 0.06 MB + `Mapped` 0.04 MB ≈ **0.6 MB**（含 shared_ptr 控制块共约 1 MB）。

### 3.5 D · 方块注册表 6.96 MB / 76 512 节点

```
3.13 MB   29294 个  mc::BlockState              （112 B/个）
1.95 MB   31925 次  StateContainer::generateStates
0.28 MB     127 次  BlockRegistry::registerBlock<StairsBlock>
0.18 MB    2222 次  BlockTags::initialize
0.16 MB      26 个  mc::blocks::WallBlock        （6144 B/个）
0.13 MB       1 次  Block::forEachBlockState
0.12 MB    1907 次  Block::forEachBlock
0.12 MB       1 次  StateContainer::Builder::create
```

**29 294 个 `BlockState` × 112 B = 3.13 MB 是方块系统的核心数据，不可削减**——这是原版
方块状态空间（含全部属性组合）的直接映射，`StateContainer::generateStates` 的 1.95 MB 是其
调色板/属性容器的配套开销。

**唯一可考虑项**：`WallBlock` 26 个各 6 144 B（合计 0.16 MB）异常偏大，值得单独看一眼
是否内联了本可下沉的数组。

### 3.6 E · NBT 标签 3.66 MB / 61 765 节点 —— 应当归零

```
2.17 MB  28382 次  nbt::tags::compound_tag::compound_tag(const&)   （复合标签深拷贝）
1.10 MB  24001 个  nbt::tags::string_tag                            （48 B/个）
0.25 MB   4868 个  nbt::tags::compound_tag                          （53 B/个）
0.11 MB   2101 个  numeric_tag<int>
```

**这 3.66 MB 是启动期解析数据包残留的 NBT 对象**。所有数据包资源（结构模板、进度、配方、
战利品表）解析完成后，原始 NBT 若只作为中间载体，应随解析器退出而释放。

值得专门核查的两处：
- `Template::Palette::m_nbt`（`NbtTable = vector<pair<u32, unique_ptr<CompoundTag>>>`）——
  每个带 NBT 的模板方块（箱子、刷怪笼、告示牌）都持有一份**深拷贝**的方块 NBT。
  这部分是**必要的**（放置时要写回去），但 `_cloneNbt` 的深拷贝路径产生了 28 382 次
  `compound_tag` 拷贝中的一部分。
- 若某条解析路径把原始 NBT 整个存进了长生命周期对象（而非只提取字段），那部分是纯浪费。

**削减路径**：先定位 28 382 次 `compound_tag` 拷贝的调用栈归属，区分"必要保留"与"可释放"。
预期可省 1–2 MB。

### 3.7 F · RocksDB 4.50 MB

```
2.09 MB       4  memalign  rocksdb::port::cacheline_aligned_alloc   （548 KB/块）
0.42 MB       3  malloc    rocksdb::AlignedBuffer::AllocateNewBuffer
0.31 MB      16  memalign  rocksdb::TableCache::TableCache
0.31 MB      16  memalign  rocksdb::BlobFileCache::BlobFileCache
0.29 MB      14  malloc    rocksdb::BlockFetcher::PrepareBufferForBlockFromFile
0.22 MB       2  malloc    rocksdb::FragmentedRangeTombstoneList::FragmentTombstones
0.12 MB      16  rocksdb::MemTable                                  （8 KB/个）
0.11 MB      16  rocksdb::HistogramImpl
```

16 个列族（每种存储对象一个）各自带 `TableCache` / `BlobFileCache` / `MemTable` /
`HistogramImpl`。空载无读写时这些是纯固定开销。

**削减路径**：
1. `cacheline_aligned_alloc` 4 块 × 548 KB = 2.09 MB 是 RocksDB 的 per-thread 缓存行缓冲，
   可由 `BlockBasedTableOptions` / `Env` 配置收紧（**需先确认 548 KB/块的来源**：
   疑似 `kNumCacheLineLocks` 或 `MemTable` 的 arena block）。
2. 列族数若能从 16 降到实际需要的数量，每减一个省约 0.03 MB（TableCache + BlobFileCache
   + MemTable + Histogram），但收益有限（16 个合计仅 0.85 MB）。
3. `MemTable` 的 `write_buffer_size` 下调：空载时每个 8 KB，合计仅 0.12 MB，收益可忽略。

综合预期 **−2～3 MB**（主要来自第 1 项）。

### 3.8 G · 生物群系 2.89 MB

```
1.16 MB  7598 个  RTreeLeaf<unsigned short>        （160 B/个）
0.23 MB  1520 个  RTreeSubTree<unsigned short>     （160 B/个）
0.10 MB  1266 次  RTree::bucketize
0.09 MB     1 次  vector<Biome>::__append（211 KB）
0.09 MB     1 次  OverworldBiomeBuilder::addSurfaceBiome（912 KB）
```

**11–12 万个 RTree 节点**（7 598 叶 + 1 520 子树 + 上级）承载的是原版 `ParameterPoint` 的
气候参数空间划分。3 个维度的 biome source 各自构造一份。

**削减路径**：RTree 的 11 万节点可改为**排序后的扁平数组 + 二分/线性扫描**。原版
`Climate.Sampler` 用的就是 `ParameterList`（扁平列表 + 逐项距离比较），并非树结构。
扁平化后每个 `ParameterPoint` 一条记录（约 40 B），总量约 0.5 MB，**省约 1.5 MB**。
代价：查询从 O(log n) 变为 O(n)（n ≈ 数千），但原版本就是线性扫描，且只在区块生成时按
cell 采样，实测热点不在此。

### 3.9 O · spdlog 日志队列 0.82 MB —— 一行配置

`spdlog::details::mpmc_blocking_queue<async_msg>::mpmc_blocking_queue` 单块 **832 KB**，
来自 `kQueueSize = 2048`（`LogManager.hpp:60`）。每个 `async_msg` 约 400 B。

空载时该队列深度基本为 0；2048 是为了应对突发日志的峰值缓冲。下调到 256 可省约 0.7 MB，
风险是极端日志风暴时 `overrun_oldest` 丢日志更早发生（当前策略本就是丢旧保新，不阻塞）。
**收益/风险比最高的单项**。

> **已落地（`73d105360`）**：改为 256 条，实测 0.81 → 0.10 MB。启动期日志共 191 行、
> 消费线程与生产同速，256 条足以吸收常规尖峰。

### 3.10 R · 世界/区块 0.91 MB

`ServerDimensionManager::_createServerWorld` 6 次分配共 916 KB。3 个维度各自的
`ServerWorld` + `ServerChunkManager` + 光照管理器等。空载无区块，这是纯骨架开销。

---

## 四、堆外的固定开销

| 项 | 字节 | 说明 |
|---|---:|---|
| `App-Specific Tag 1` | 4.0 MB | dyld 与系统 VM，非 malloc，不可控 |
| Malloc Metadata | 1.0 MB | 349 031 节点 × 约 3 B |
| Stack | 0.9 MB | 49 线程 |
| page table | 0.6 MB | |
| `__DATA` dirty | 1.6 MB | 全局对象写入页；含生成的 Java 映射表（`java_block_state_table.gen.cpp` 3.3 MB 源码展开） |

**`__TEXT` / `__DATA_CONST` / `__LINKEDIT` / `__OBJC_RO` 在 `vmmap` 中显示 22–60 MB，
但全部是 clean + shared**，不计入 physical footprint（`footprint` 工具已正确排除）。这是
`vmmap --summary` 与 `footprint` 读数差异的来源，比较两者时不要混用。

---

## 五、40 MB 目标的可行性评估

### 5.1 数学

```
当前 footprint                 74 MB
其中不可控（碎片+元数据+系统）  30 MB   ← 不随游戏数据变化
其中受控（堆实际分配）          44 MB
```

要达到 `footprint ≤ 40 MB`，需要 `44 → 10 MB`，即**砍掉 77%**。

### 5.2 逐项做满的终点

| 优化项 | 收益 | 风险 | 前提 |
|---|---:|---|---|
| 结构模板懒加载（§3.2） | −7.5 MB | 中 | 需重排 `SingleJigsawPiece` 的模板解析时机 |
| NBT 残留释放（§3.6） | −1.5 MB | 低 | 先定位拷贝栈 |
| spdlog 队列 2048→256（§3.9） | −0.7 MB | 低 | 一行常量 |
| RocksDB per-thread 缓冲（§3.7） | −2.0 MB | 中 | 先确认 548 KB 来源 |
| RTree 扁平化（§3.8） | −1.5 MB | 中 | 改查询结构，需 parity 回归 |
| AST 中间产物释放（§3.4） | −1.0 MB | 低 | 确认编译产物不依赖 AST |
| JigsawJoint 共享字符串（§3.3） | −0.7 MB | 低 | |
| 进度/配方/战利品懒加载（§3.1 G/I/K） | −4.0 MB | 中 | 三者均为启动期全量装配 |
| **合计** | **−18.9 MB** | | |

终点：`44 − 19 = 25 MB` 堆 → `footprint ≈ 55 MB`。**仍高于 40 MB 约 15 MB。**

### 5.3 结论

**40 MB 不可通过"优化现有结构"达到**。缺口来自两个架构性事实：

1. **启动期全量装配**。方块注册表（29 294 个 `BlockState`）、方块标签、物品、配方、战利品
   表、进度、结构模板、3 个维度的密度函数编译产物——全部在 `MinecraftServer::initialize()`
   期间构建完毕，与"玩家是否用到"无关。其中 §3.2/§3.1-G/I/K 的可懒加载部分合计约 12 MB，
   但**剩下的 20+ MB（方块状态空间、物品注册表、方块标签、命令树、维度骨架）是原版语义
   要求启动即可查的**，无法后置。

2. **malloc 碎片与小对象基数**。349 031 个节点在 macOS small zone 下产生 27–31% 碎片
   （15.9 MB）。`BlockState`（29 294 个 112 B）、`string_tag`（24 001 个 48 B）、
   `RTreeLeaf`（7 598 个 160 B）这类**大量同尺寸小对象**是碎片的主因。把它们改为**紧凑数组
   或池化**（例如 `BlockState` 从 `new` 改为 arena 分配）能同时降低节点数与该尺寸类的碎片，
   是把 footprint 压到 40 MB 以下的**唯一现实路径**——但那是内存布局重构，不是优化项。

**建议**：把 40 MB 拆成两步目标。第一步（本报告 §5.2，约 −19 MB）到 **约 55 MB**，全部是
低-中风险的局部改动；第二步（小对象池化 + 紧凑数组）到 **40 MB 以下**，需要单独立项。

---

## 六、优化项执行顺序

按「收益 ÷ 风险」排序，前四项可立即执行：

| 序 | 项 | 收益 | 风险 | 涉及文件 |
|---:|---|---:|---|---|
| 1 | spdlog 队列 2048 → 256 | 0.7 MB | 极低 | `common/application/LogManager.hpp` |
| 2 | 结构模板懒加载 | 7.5 MB | 中 | `TemplatePoolLoader` / `SingleJigsawPiece` / `JigsawPiece` |
| 3 | NBT 残留归因与释放 | 1.5 MB | 低 | 需先定位 `compound_tag` 拷贝栈 |
| 4 | JigsawJoint 共享字符串 | 0.7 MB | 低 | `JigsawPiece.cpp` / `SingleJigsawPiece.hpp` |
| 5 | RocksDB 缓冲配置 | 2.0 MB | 中 | `RocksDBConfig.hpp` / `RocksDBDatabase.cpp` |
| 6 | AST 中间产物释放 | 1.0 MB | 低 | `RandomState.cpp` / `BytecodeGen` |
| 7 | RTree 扁平化 | 1.5 MB | 中 | `climate/RTree*.hpp` |
| 8 | 进度/配方/战利品懒加载 | 4.0 MB | 中 | 各自 Loader + Manager |
| 9 | 维度按需创建 | ~5 MB | **高（行为改动）** | `ServerDimensionManager` |

第 9 项收益最大但会改变"三维度始终存在"的语义，需单独立项评估。

---

## 七、测量方法学（复用要点）

1. **必须先关 profiler**：`--profiler_enabled=false`。默认启动时 Perfetto 的
   `TracingTLS`（38 个线程 × 28 KB = 1.09 MB）+ 各后端池使 footprint 从 74 MB 涨到 95–99 MB。
2. **`footprint` 与 `vmmap --summary` 不可混用**：后者的 TOTAL 含 `__TEXT`/`__LINKEDIT`/
   `__OBJC_RO` 等 clean+shared 页（22–60 MB），不进入物理足迹。
3. **`heap` 默认不符号化裸缓冲**：不加 `MallocStackLogging` 时，32.9 MB / 222 103 节点会
   归入 `non-object`。加 `MallocStackLogging=1` 后 `heap` 能从分配回溯反推类型名，代价是
   自身约 +4.3 MB 堆开销——**用比例而非绝对值**。
4. **同一进程内采样**：跨进程比较 `heap` 输出无意义（同尺寸节点的地址分布完全不同）。
5. **按分配点归因比按尺寸直方图有效**：`vmmap` 的尺寸直方图只能给出"2048 B × N 个"，
   无法回答"这 N 个是谁分配的"。`heap <pid>` 的输出每行都带 `malloc in <调用栈顶帧>`。

复现命令：

```bash
# 基准（快）
VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json \
  ./build/bin/RelWithDebInfo/minecraft-server --profiler_enabled=false &
sleep 75 && pid=$(pgrep -x minecraft-server | head -1)
footprint $pid
vmmap --summary $pid
heap $pid > /tmp/steady.txt

# 归因（慢，含 MSL 开销）
MallocStackLogging=1 MallocStackLoggingNoCompact=1 \
VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json \
  ./build/bin/RelWithDebInfo/minecraft-server --profiler_enabled=false &
sleep 80 && heap $(pgrep -x minecraft-server | head -1) > /tmp/attributed.txt
```
