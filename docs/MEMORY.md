# Cubium 服务端内存实测报告（Windows / viewDistance=16）

> 测量日期：2026-09-25　构建：`windows-clang-relwithdebinfo`（`MC_ENABLE_TRACY=ON`，Commit 7553d96）
> 测量方法：`VirtualQueryEx` 区域枚举 + `QueryWorkingSetEx` 逐页驻留判定 + `!heap -stat` 尺寸直方图 + UMDH 分配栈归因
> 脚本沉淀于 `scripts/diag/windows-memory/`

---

## 一、结论摘要

| 场景 | 工作集（WS） | 说明 |
|---|---|---|
| 空载，默认启动 | **298 MB** | 含 Tracy 诊断开销 |
| 空载，`TRACY_SYMBOL_OFFLINE_RESOLVE=1` | **120 MB** | 剥离 Tracy 开销后的**真实空载基线** |
| 加载世界 + 1 玩家在线（同上环境变量） | **323 MB** | 区块已加载至 viewDistance=16 |

**两项关键结论**：

1. **当前窗口下最大的单项内存是 Tracy 的 Windows 诊断开销，而非游戏逻辑**。默认启动时 Tracy 的 `SymbolWorker` 线程在静态初始化阶段调用 `SymInitialize` + `SymLoadModuleEx`，把全部模块的调试符号加载进堆（dbghelp TPI 表），实测占用 **~96 MB 且全程不释放**；同时 Tracy 自带的 rpmalloc 按 `DEFAULT_SPAN_MAP_COUNT(64) × 64 KiB = 4 MiB` 为单位 `VirtualAlloc`，提交了 72 个 4 MB 区域（288 MB 提交量，实际驻留仅 1 MB）。

2. **剥离该开销后，游戏逻辑内存在区块满载时为 323 MB，距离 200 MB 目标尚差约 123 MB，缺口全部在区块数据侧**。区块相关的堆分配实测 174.6 MB，其中 `2048 B × 29897 = 58.4 MB` 与 `13777 B × 2535 = 33.3 MB` 两项即占 91.7 MB。

> **达标路径评估**：靶点 1（关 Tracy）在 Windows 上直接落到 ~145–200 MB，是**唯一的确定性达标手段**；但该收益是 Windows 平台特有的诊断开销，macOS/Linux 上不存在。若目标是在所有平台上都压到 200 MB 以内，则须叠加靶点 2（天空光压缩，12–38 MB）+ 靶点 3（高度图位压缩，6.3 MB）+ 靶点 5（SectionCache 缩容，≤38 MB），合计约 56–82 MB，从 323 MB 降到 241–267 MB——**仍不足以达标**，说明区块数据本身的固定开销（`ChunkData` 外壳 13.8 KB × 1225 = 16.9 MB、palette storage、`BiomeContainer` 3 KB × 1225 = 3.8 MB）之外还有未被本次测量拆分清楚的部分，需进一步定位 8657 B（22.6 MB）与 7377 B（7.9 MB）两个尺寸类的归属。

---

## 二、Tracy 诊断开销（Windows 特有，可一步消除）

### 2.1 实测 A/B 对照

在完全相同的启动参数（`--profiler-enabled=false`、同一存档、同一端口）下，仅改变 Tracy 的环境变量，空载工作集如下：

| 配置 | WS | 相对基线 |
|---|---|---|
| A 基线（无环境变量） | 298.3 MB | — |
| B `TRACY_NO_DBGHELP_INIT_LOAD=1` | 215.0 MB | −83.3 MB |
| C `TRACY_SYMBOL_OFFLINE_RESOLVE=1` | 120.5 MB | **−177.8 MB** |
| D `C` + `B` | 119.5 MB | −178.8 MB |

### 2.2 根因（两处，均为 Windows 分支特性）

**根因一：dbghelp 符号表预加载（~96 MB，不可回收）**

`third_party/tracy/public/client/TracyCallstack.cpp:542-565` 的 `InitCallstack()` 在 `TRACY_NO_DBGHELP_INIT_LOAD` 未设时会调用 `CacheProcessDrivers()` + `CacheProcessModules()`，对每个已加载模块执行 `SymLoadModuleEx`。UMDH 分配栈归因（`scripts/diag/windows-memory/parse_umdh.py`）显示 Top 分配点中 `dbghelp!TPI1::fInitReally` / `dbghelp!LoadExportSymbols` 两处合计 **167 MB 的请求量**，其中：

- `116DB260`：单块 30.45 MB ← `TPI1::fInitHashToPchnMap`
- `116DA960`：单块 20.28 MB ← `pdb_internal::Array<TPI1::PRECEX>::growMaxSize`
- `1061D960`：69 块合计 25.59 MB ← `LoadExportSymbols → seCreateSymbolTable`
- 其余十余项同源

这解释了此前观察到的「`0x1C389000` 处 30.45 MB 单块分配，100% 常驻、全程不释放」。

**根因二：Tracy 的 rpmalloc 分配器 span map（288 MB 提交 / 73 个 4 MB 区域）**

`tracy_rpmalloc.cpp` 的 `DEFAULT_SPAN_MAP_COUNT = 64` × 64 KiB span = **4 MiB/次**，且 `_rpmalloc_mmap_os` 在 Windows 上直接 `MEM_RESERVE|MEM_COMMIT`。实测 73 个精确 4.000 MB 的 `PAGE_READWRITE` 私有区域，4 MB 相位**各不相同**（说明是各自独立的 `VirtualAlloc`，而非同一 reserve 内的分段提交），驻留仅 1 MB —— 属「只增不还」的保留型开销，对工作集影响小，但把提交量推到 835 MB。

**注意**：两者都**不受 `--profiler-enabled=false` 影响**。该运行期 flag 只门控 `ProfilerManager`（Perfetto 侧）；Tracy 是编译期接入，其静态期初始化与 `SymbolWorker` 线程（`TracyProfiler.cpp:1636-1637`）无条件启动。`MC_TRACY_ON_DEMAND=ON` 只让**事件**不入队，不影响上述两项。

### 2.3 消除方式

**根本解（推荐）**：改用无 profiler 预设构建。仓库已有 `windows-clang-release-noprof` 预设（`CMakePresets.json`，`MC_ENABLE_TRACING/TRACY/MEMORY` 全 OFF），且 `build-release-noprof/bin/Release/minecraft-server.exe` 已存在产物。

**临时解（无需重新构建，仅用于排查期）**：设置 `TRACY_SYMBOL_OFFLINE_RESOLVE=1`，使 `DbgHelpInit` / `DbgHelpLoadSymbolsForModule` 直接短路返回。本报告后续所有游戏逻辑内存数据均在此环境下测得。

---

## 三、游戏逻辑内存构成（区块满载，1 玩家在线）

### 3.1 驻留内存按区域类型

| 类型 | 提交 | 驻留 |
|---|---|---|
| PRIVATE | 676.13 MB | **285.75 MB** |
| IMAGE | 102.10 MB | 38.20 MB |
| MAPPED | 21.42 MB | 0.46 MB |
| **合计** | 799.65 MB | **324.41 MB** |

其中 PRIVATE 未驻留的 390 MB 绝大部分是 rpmalloc 的 4 MB 空 span（76 个 4–16 MB 区域共 308 MB 提交、仅 1.05 MB 驻留）。

### 3.2 堆尺寸直方图（`!heap -stat`，满载 busy 174.59 MB）

| 尺寸(B) | 块数 | 合计 | 归属判定 |
|---|---|---|---|
| **2048** | **29897** | **58.39 MB** | 区块数据（palette storage / 天空光 nibble / NibbleArray） |
| **13777** | **2535** | **33.31 MB** | 区块数据；cdb 符号注释确证为 `std::_Ref_count_obj2<mc::world::chunk::ChunkData>` |
| **8657** | **2731** | **22.55 MB** | 区块数据（未确证具体类型） |
| 7377 | 1129 | 7.94 MB | 区块数据（未确证） |
| 2560 | 2072 | 5.06 MB | palette storage（bits=5） |
| 128 | 40663 | 4.96 MB | 各类控制块/桶数组（混合） |
| 3072 | 1513 | 4.43 MB | 疑为 `BiomeContainer`（1536 × u16），待确证 |
| 32 | 129055 | 3.94 MB | 小对象（混合） |
| 200 | 19175 | 3.66 MB | 小对象（混合） |
| **32807** | **114** | **3.57 MB** | 区块数据（未确证） |
| 16 | 217050 | 3.31 MB | 小对象（混合） |
| 104 | 32303 | 3.20 MB | 小对象（混合） |
| 184 | 17506 | 3.07 MB | 疑为 `ChunkSection`（`sizeof` 推导为 184 B），待确证 |
| 48 / 72 / 56 | 64711 / 40332 / 43541 | 2.96 / 2.77 / 2.33 MB | 小对象（混合） |
| 1024 | 2803 | 2.74 MB | 未确证 |
| 856 | 2838 | 2.32 MB | 疑为 `ChunkPrimer`（`sizeof` 推导为 856 B），待确证 |
| 2187399 | 1 | 2.09 MB | RocksDB `StatisticsImpl`（`createDBOptions`），启动期固定 |
| 1048576 | 2 | 2.00 MB | RocksDB `Arena::AllocateNewBlock`（MemTable），启动期固定 |

**已确证随世界加载增长的类**：`2048`、`13777`、`8657`、`7377`、`32807`、`2560`、`3072` —— 这 7 类在真正空载进程的 top-20（截断阈值 1.59 MB）中**完全不存在**，却合计占满载 busy 的 **135.3 MB（77.5%）**。

> **测量局限（须注意）**：两份直方图来自不同进程，且空载那份未设 `TRACY_SYMBOL_OFFLINE_RESOLVE`，其 top-20 被 dbghelp 的分配（`4072`、`24240` 等类）占据，无法作为「干净空载」的对照。上表中的区块类归属结论来自**块数绝对量 + 尺寸与结构体 `sizeof` 的吻合度**，而非严格差分。要得到逐类精确归属，应在 `TRACY_SYMBOL_OFFLINE_RESOLVE=1` 下分别采集「玩家进入前」与「进入后」两份直方图做差分。

> **`184` / `856` / `3072` 三类的意义**（若确证）：`184` ≈ `sizeof(ChunkSection)` × 17506 ≈ 1225 区块 × 14.3 段/区块，符合「每区块平均 14 个非空段」；`856` ≈ `sizeof(ChunkPrimer)` × 2838 ≈ 1225 有效 + 边界区块，符合「每区块一个 primer 常驻不析构」；`3072` = `BiomeContainer::TOTAL_SIZE(1536) × sizeof(u16)` × 1513 ≈ 1225，符合「每区块一个 biomes 容器」。三者都是**每区块一份的固定开销**，合计 12.8 MB。

### 3.3 2048 字节块的内容指纹（抽样 14 块）

对 2048 B 类按等间距抽样转储前 32 字节：

- **全 `0x00`**：5/14 —— 地下段的无光 nibble（方块光为 0）或全同值 palette storage
- **全 `0xFF`**：3/14 —— **天空光「全 15」的 nibble**，即最高非空段之上、被 `setFull()` 强制分配的那些段
- **`0x1111…` / `0x2222…` / `0x5555…`**：3/14 —— PalettedContainer 的 bits=4 palette 索引打包（索引恒为 1/2/5，即该段只含 2–6 种方块）
- 混合：3/14 —— 地表段（既有方块的索引、也有天空光的 `0xFF` 边缘）

`29897 / 1225 ≈ 24.4 块/区块`。按 `LIGHT_SECTIONS = 26` 计，**其中约 16 块/区块是天空光 nibble**（最高非空段以上恒为全 15），其余为 palette storage。这与 `SkyLightEngine::initNibble` 对「最高非空段之上」无条件 `setFull()` → `_allocateBytes()` 的行为一致。

### 3.4 ChunkData 外壳的固定开销

`sizeof(ChunkData)` 在 `MC_ENABLE_TRACY=ON` 下实测落于 13777 B 桶（含 `shared_ptr` 控制块）。其内部构成（按 `ChunkData.hpp:559-638` 逐成员推导）：

| 成员 | 字节 | 占比 |
|---|---|---|
| `m_heightmaps` = `array<Heightmap, 7>` | **7,196** | **52.3%** |
| `m_biomes`（`array<u16,1536>`） | 3,072 | 22.3% |
| `m_skyNibbles` + `m_blockNibbles` + 指针表 | 2,080 | 15.1% |
| `m_postProcessingSections`（24 × vector） | 576 | 4.2% |
| `m_sections`（24 × unique_ptr） | 192 | 1.4% |
| 其余（坐标、锁、标志、map） | ~656 | 4.8% |

**原版对照**：MC 1.21.11 `Heightmap.java:39-41` 用 `SimpleBitStorage(Mth.ceillog2(chunk.getHeight()+1), 256)`，即按维度高度**位压缩**（384+1 → 9 bit/列 → **288 B/类型**）。Cubium 用 `array<BlockCoord=i32, 256>` = **1,024 B/类型**，且 7 个槽位全部内联。

---

## 四、优化靶点（按收益/风险排序）

### 靶点 1 · 编译期关闭 Tracy —— 收益 −178 MB（Windows），风险：无

见第二章。这是唯一能「一步达标」的措施，且不涉及任何游戏逻辑。使用 `windows-clang-release-noprof` 或 `windows-clang-relwithdebinfo-noprof` 预设。

> 若需保留 Perfetto 而仅关 Tracy：`MC_ENABLE_TRACING=ON` + `MC_ENABLE_TRACY=OFF` + `MC_ENABLE_MEMORY=OFF`。此组合下 4 MB rpmalloc 区域与 dbghelp 符号表均消失（`MC_ENABLE_MEMORY` 必须一并关，否则 `MemoryTracking.hpp` 的 Tracy 工具链仍会拉入 Tracy 头文件）。

### 靶点 2 · 天空光「全亮段」压缩 —— 收益上限 ~38 MB，风险：中

`29897 / 1225 ≈ 24.4 块/区块`。2048 B 块由三类用途混用：天空光 nibble、palette storage、`NibbleArray` 副本。

抽样 14 块的指纹分布：全 `0xFF` **3 块**、全 `0x00` **5 块**、规律半字节（palette 索引）**3 块**、混合 3 块。按此比例外推，全 `0xFF` 的天空光 nibble 约占 **21% ≈ 6,300 块 ≈ 12.3 MB**。

理论推算的上限更高：若按 `LIGHT_SECTIONS(26) − 最高非空段` 计，每区块可有约 16 段处于「全亮」态，即 1225 × 16 ≈ 19,600 块 ≈ **38.4 MB**。两者差距说明抽样样本（14 块）过小，**实际收益须以更大样本或按用途分类统计确认**，区间约为 **12–38 MB**。

这些段语义上就是常量「全 15」，无需 2,048 B 实体缓冲。改动面：`SWMRNibbleArray`（`src/common/world/chunk/data/light/`）与 `SkyLightEngine::initNibble`（`SkyLightEngine.cpp:185-190`）。需评估在 `SWMRNibbleArray` 增加「全满/全零常量态」标记（现有 `State` 枚举已有 Null/Uninit 语义，可扩展），并在读取路径（`get()` / `toByteArray()`）与写入路径（首次 `set()` 时实体化）分别处理。风险在于这是光照引擎核心，任何遗漏会导致光照计算偏差。

### 靶点 3 · Heightmap 位压缩 —— 收益 ~8.8 MB，风险：低-中

7,196 B → 按原版 9 bit/列 × 256 列 = 288 B/类型。若 7 个类型全保留则为 2,016 B，节省 5,180 B/区块 × 1225 = **6.3 MB**。

若进一步去掉服务端从无读取点的类型（需实测确认哪些 `HeightmapType` 在服务端零消费），收益可到 8.8 MB。

改动面：`Heightmap.hpp` 的 `std::array<BlockCoord, SIZE> m_heights` → 位压缩存储，并保持 `getHeight`/`setHeight`/`getData`/`setData` 接口语义。`getData()` 会从「返回 const 引用」变为「返回值/填充缓冲」，需检查调用点。

### 靶点 4 · 网络层每玩家深拷贝 —— 收益取决于玩家数，风险：低

`MinecraftServer.cpp:1698` 的 `LevelChunkWithLight pkt = ir;` 对每个接收玩家各做一次完整深拷贝（60–100 KB/区块）。多玩家同区域时应改为共享只读包或按需 move。单玩家场景收益有限。

### 靶点 5 · SectionCache 容量 —— 收益上限 ~38 MB，风险：低

`MinecraftServer.cpp:709` 传入容量 2048，每 `SectionData` ≈ 18.7 KB（blockStates 扁平 4096 × u32 = 16 KB + 双光照 4 KB + biomes 128 B）→ LRU 满时 **38.4 MB 常驻**。存储层（RocksDB）是权威数据源，缩小容量只影响读盘次数，不改变游戏行为。建议把容量做成 `ServerSettings` 的可配置项。

> 本次测量未单独验证该项（其分配发生在异步 IO 线程且尺寸分散），建议后续用 `TRACY_SYMBOL_OFFLINE_RESOLVE=1` 环境下的堆尺寸直方图对比「玩家进入前/后」验证。

### 靶点 6 · `m_maxLoadedChunks` 未生效 —— 风险：需先修

`ServerChunkManager.hpp:1241` 默认 0（不限），唯一 setter 无生产调用点，源码自带 TODO 标注该软上限「当前不可启用」。玩家持续快速移动时，`m_chunks` 在 30 秒卸载延迟窗口内缺乏硬边界。

---

## 五、结论的确证程度（哪些是实测、哪些是推断）

**实测确证**：

- Tracy 环境变量 A/B/C/D 对照的 4 组工作集数字（同一存档、同一参数，唯一变量是环境变量）
- Tracy 诊断开销的分配栈归因：UMDH 快照中 `dbghelp!TPI1::fInitReally` / `LoadExportSymbols` 两条链的完整调用栈（`parse_umdh.py` 解析 445,488 条堆记录、64,592 条独立栈）
- 73 个 4 MB 区域的相位互异性与 1 MB 驻留量（`resident2.ps1` 逐页 `QueryWorkingSetEx`）
- `13777 B` 类的类型：cdb 在块清单中直接标注 `minecraft_server!std::_Ref_count_obj2<mc::world::chunk::ChunkData>::vftable`
- 2048 B 块的内容指纹分布（14 块抽样转储）
- 区块满载时堆 busy 174.59 MB 的完整尺寸直方图
- `sizeof(Heightmap)` = 1,028 B（`enum(1) + pad(3) + array<i32,256>(1024)`）与原版 `SimpleBitStorage(9, 256) = 288 B` 的对照

**尚未确证（报告的推断部分）**：

- `8657`（22.55 MB）、`7377`（7.94 MB）、`32807`（3.57 MB）、`1024`（2.74 MB）四个尺寸类的具体归属。它们**只可能**是区块/世界数据（启动期空载进程的 top-20 中没有它们），但具体是哪个结构体未确证。
- `3072`（`BiomeContainer`）、`184`（`ChunkSection`）、`856`（`ChunkPrimer`）三类的归属是基于 `sizeof` 推导与块数量级吻合（1513 ≈ 1225 区块）的**推断**，未由 cdb 符号注释或调用栈直接确证。
- SectionCache 的 38.4 MB 是依据「容量 2048 × 每 SectionData ≈ 18.7 KB」的**计算值**，未在本轮测量中单独剥离验证。
- 天空光全亮段的实际占比：抽样 14 块中 3 块全 `0xFF`（21%），按此外推得 ~12 MB；而按 `LIGHT_SECTIONS` 理论推算上限为 ~38 MB。两者相差 3 倍，**取样量不足**，真实值须更大样本确认。

---

## 六、测量方法学与陷阱（复用要点）

1. **`--profiler_enabled=false` 不能关闭 Tracy**。它只门控 Perfetto 侧。Windows 上若不剥离 Tracy 诊断开销，测量结果会被 178 MB 噪声主导，所有优化都会「看不见」。排查期务必设 `TRACY_SYMBOL_OFFLINE_RESOLVE=1`。

2. **看工作集，别看提交量**。rpmalloc 的 4 MB span 把提交量推到 835 MB，但驻留仅 1 MB。`QueryWorkingSetEx` 逐页判定（`scripts/diag/windows-memory/resident2.ps1`）才能得到真实占用。

3. **`!heap -stat` 是定位大头的第一手段**。尺寸直方图能立刻把「58.4 MB / 2048 B × 29897」这类集中分配暴露出来，比逐个符号化调用栈快得多。

4. **对 2048 B 类抽样转储内容指纹**（全 `0xFF` = 天空光全亮、全 `0x00` = 无光、规律半字节 = palette 索引）可以在不看调用栈的情况下区分同尺寸不同用途的块。

5. **`gflags //i <exe> +ust` 开启用户态堆栈追踪后，`!heap -p -a <addr>` 可直接归因**（本报告第二章的根因一即由此确证）。注意 UST 本身有显著开销，测完须 `-ust` 关闭。

6. **PowerShell 脚本文件须带 UTF-8 BOM**（本仓库脚本用 `-File` 方式调用时，无 BOM 的中文注释会导致 parser error）。

7. **`cdb` 的 `!heap -flt s <size>` 输出中，`UserSize` 是十六进制且不带 `0x`**；块清单里夹杂的符号注释行（如 `minecraft_server!...vftable`）可直接给出类型名，是确认归属的捷径。
