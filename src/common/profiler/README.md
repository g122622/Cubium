# 性能追踪模块（Perfetto + Tracy 双轨）

本模块提供双轨性能追踪基础设施，同时集成 Google Perfetto 与 Tracy 两套 profiler。两套后端可独立开关、同时启用（双轨录制），对外 `MC_TRACE_*` API 宏名统一不变。两者皆关时所有宏空展开、零开销。

| 后端 | 开关（CMake option） | 默认 | 采集方式 | 查看方式 |
|------|---------------------|------|----------|----------|
| Perfetto | `MC_ENABLE_TRACING` | ON | 进程内录制到 `.perfetto-trace` 文件 | ui.perfetto.dev |
| Tracy | `MC_ENABLE_TRACY` | ON | in-memory，client 自动监听 8086 端口 | tracy GUI 连接 8086 拉取 |

> Tracy 无法进程内写文件（需外部 `tracy-capture` 连 8086 拉取），故 `ProfilerManager` 对 Tracy 不做 start/stop/capture 管理，只做进程/线程命名的双写。

## 目录结构

```
src/common/profiler/
├── CMakeLists.txt        # 构建配置：perfetto_sdk + TracyClient(vendored submodule) + mc_profiler + mc::profiler 别名
├── ProfilerConfig.hpp    # 编译时开关（MC_ENABLE_TRACING / MC_ENABLE_TRACY / MC_PROFILER_ENABLED）+ 缓冲区/输出路径
├── ProfilerManager.hpp   # 门面单例 + TraceConfig（持有 PerfettoBackend，tracy 命名双写）
├── ProfilerManager.cpp   # 门面实现：生命周期委托 PerfettoBackend，setProcessName/setThreadName 双写 tracy
├── PerfettoBackend.hpp   # Perfetto 后端声明（Pimpl，仅 MC_ENABLE_TRACING 时编译）
├── PerfettoBackend.cpp   # Perfetto 后端实现：TracingSession 生命周期 / root track / 写文件 / sibling_order_rank
├── TraceCategories.hpp   # 分类枚举树 mc::trace::TraceEvents + PERFETTO_DEFINE_CATEGORIES 注册
├── TraceCategories.cpp   # 静态存储定义（PERFETTO_TRACK_EVENT_STATIC_STORAGE，仅 MC_ENABLE_TRACING）
├── TraceEvents.hpp       # MC_TRACE_* 双轨宏（四种开关组合分支）+ MC_TRACE_MEM_ALLOC/FREE 手动内存宏
└── MemoryTracking.hpp    # 分配级内存追踪安全工具：TracyTrackingAlloc（vector 追踪）+ TracyObjectTracker（对象守卫）
```

## 架构（两层门面）

```
ProfilerConfig.hpp（编译时开关）
        │
        ├─────────────────────────────────────┐
        ▼                                     ▼
TraceCategories.hpp                   ProfilerManager.hpp（门面 + TraceConfig）
(枚举树 + PERFETTO_DEFINE_CATEGORIES)        │  ▲
        │                                     │  │ 持有 unique_ptr<PerfettoBackend>
        ▼                                     │  │（前向声明，避免循环 include）
TraceCategories.cpp                           │  │
(PERFETTO_TRACK_EVENT_STATIC_STORAGE)         │  │
        │                                     ▼  │
        └──────────────► PerfettoBackend.hpp ───┘
                         (Pimpl，仅 TRACING)
                                │
                                ▼
                         PerfettoBackend.cpp
                         (TracingSession/root track/写文件)
                                │
        ┌───────────────────────┘
        ▼
ProfilerManager.cpp（门面实现：生命周期委托 PerfettoBackend；tracy 命名内联双写）
        │
        ▼
TraceEvents.hpp（MC_TRACE_* 双轨宏，四种组合分支）
        │
        ▼
   应用代码（MC_TRACE_* 宏）
```

- **ProfilerManager（门面）**：单例，统一管理两套后端。生命周期方法（`initialize`/`shutdown`/`startTracing`/`stopTracing`/`flush`）仅委托 PerfettoBackend——Tracy 采集是 client 自动完成的，无需门面驱动。`setProcessName`/`setThreadName` 双写：同时写 PerfettoBackend（若启用）与 tracy（若启用）。
- **PerfettoBackend**：承载 Perfetto 重逻辑（TracingSession、root track descriptor、写文件、sibling_order_rank 查表）。仅在 `MC_ENABLE_TRACING` 时编译，由门面经 Pimpl 持有。
- **TraceCategories**：枚举树 `mc::trace::TraceEvents`（无条件定义，两套后端都可用）；`PERFETTO_DEFINE_CATEGORIES` 注册仅 Perfetto 需要。
- **TraceEvents.hpp**：四种开关组合的宏分支（双轨 / 仅 Perfetto / 仅 Tracy / 全关）。

## 上下游外部依赖关系

### 被谁依赖

整个项目的性能追踪都依赖此模块。主要使用方：

- **服务端核心**：`MinecraftServer`、`ServerWorld`、`ServerChunkManager`、`ChunkGenerateTask` 等
- **客户端核心**：`ClientApplication`、`ClientWorld`、`TridentEngine`、`ChunkRenderer` 等
- **世界生成**：`NoiseChunkGenerator`、`BiomeRegistry`、光照引擎等
- **网络同步**：`ChunkSendManager`、`ClientNetwork` 等
- **存储系统**：`RocksDBDatabase`、`SectionManager`、`AutoSave` 等

### 依赖了谁

| 依赖 | 用途 |
|------|------|
| **Perfetto SDK** | 第三方单文件 SDK（`third_party/perfetto/perfetto.{h,cc}`，官方 releases，含 PR #6219 thread_ordering 支持） |
| **Tracy** | 第三方 profiler（vendored git submodule `third_party/tracy`，通过 `add_subdirectory` + `TRACY_ENABLE` FORCE 编译 TracyClient） |
| **spdlog** | 日志输出（追踪系统状态信息） |
| **ws2_32 / dbghelp / secur32**（Windows） | WinSock2 / 调试 / 安全库，Perfetto 与 Tracy 网络功能需要 |

## CMake 开关组合

| `MC_ENABLE_TRACING` | `MC_ENABLE_TRACY` | `mc_profiler` | 行为 |
|---|---|---|---|
| ON | ON | STATIC（ProfilerManager + PerfettoBackend + TraceCategories） | 双轨录制，宏同时发两套 |
| ON | OFF | STATIC（ProfilerManager + PerfettoBackend + TraceCategories） | 仅 Perfetto，宏只发 TRACE_EVENT |
| OFF | ON | STATIC（ProfilerManager only） | 仅 Tracy，宏只发 ZoneScopedN/TracyPlot；ProfilerManager 生命周期方法为空操作但命名仍双写 |
| OFF | OFF | INTERFACE 空库 | 全关，ProfilerManager 内联存根，宏空展开 |

Tracy 后端编译时由 CMake 设置 `TRACY_ENABLE=ON`、`TRACY_NO_SYSTEM_TRACING=ON`（FORCE，在 `add_subdirectory` 之前），`Tracy::TracyClient` 自带 PUBLIC `TRACY_ENABLE`，消费方无需再 define。

## 双轨宏设计（TraceEvents.hpp）

四种组合由 `MC_ENABLE_TRACING` 与 `MC_ENABLE_TRACY` 控制：

| 宏 | 双轨 | 仅 Perfetto | 仅 Tracy |
|----|------|-------------|----------|
| `MC_TRACE_SCOPED_EVENT` | `TRACE_EVENT + MC_TRACY_SCOPED_ZONE` | `TRACE_EVENT` | `MC_TRACY_SCOPED_ZONE` |
| `MC_TRACE_INSTANT_EVENT` | `TRACE_EVENT_INSTANT + TracyMessageL` | `TRACE_EVENT_INSTANT` | `TracyMessageL` |
| `MC_TRACE_COUNTER` | `TRACE_COUNTER + TracyPlot(double)` | `TRACE_COUNTER` | `TracyPlot(double)` |
| `MC_TRACE_EVENT_BEGIN/END` | `TRACE_EVENT_BEGIN/END + TracyMessageL` | `TRACE_EVENT_BEGIN/END` | `TracyMessageL` |
| `MC_TRACE_SET_THREAD_NAME` | `ProfilerManager::setThreadName`（双写） | 同左 | 同左 |

### 同一作用域多个 SCOPED_EVENT 安全（重要）

Perfetto 的 `TRACE_EVENT` RAII 变量按 `__LINE__` 后缀命名，天然允许同一作用域多个。但 Tracy 的 `ZoneScopedN` 用**固定变量名** `___tracy_scoped_zone`，同一作用域两个会 redefinition。本模块的 `MC_TRACY_SCOPED_ZONE` 改用 `ZoneNamedN` 配合 `__LINE__` 后缀变量名（`___tracy_zone_<line>`），使每个 zone 变量唯一，**双轨下同一作用域多个 `MC_TRACE_SCOPED_EVENT` 可安全共存**。

```cpp
// 双轨下合法：两个 zone 变量名按行号唯一
void tick() {
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "EnvironmentTick");
    tickEnvironment();
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "PrecipitationTick");
    tickPrecipitation();
}
```

> 注意：双轨宏展开为**多条语句**（perfetto 声明 + tracy 声明），不能塞进 `EXPECT_NO_THROW(...)` 等只接受单语句的宏参数。测试中应直接作为语句调用。

## 线程排序机制（Perfetto 侧）

基于 PR #6219（release v57.1+），可在 Perfetto UI 中固定线程显示顺序：

- **根 track descriptor（uuid=0）** 上设 `thread_ordering = THREAD_ORDERING_EXPLICIT`，由 `PerfettoBackend::startTracing` 用 `Track::Global(0)` 发出。**禁止用 `Track(0)`**——它会与 per-process 随机 cookie 异或得到非 0 uuid，根 track 失效。
- 每个线程 track descriptor 上设 `sibling_order_rank`（`PerfettoBackend::setThreadName` 内部完成），值越小越靠前；未设默认 0（排最前），故命名线程显式给 1-100 避免意外排前。
- 固定线程 rank：`MemoryTrace=1`、`ClientMainThread=2`、`IntegratedServerThread=3`、`AudioEngineWorker=4`、`ServerMainThread=5`。
- worker 池用 `rankBase + workerId` 精确排序，三组分块排列、组内按 workerId 升序：`ServerCompute-N` = 100+N，`ServerIO-N` = 200+N，`ClientCompute-N` = 300+N。每组间隔 100，避免线程数 >10 时跨组相交。UI 顺序为 ServerCompute → ServerIO → ClientCompute。`UniversalWorkerPool` 的 `rankBase` 为构造参数（ServerCompute=100、ServerIO=200、ClientCompute=300），由各宿主显式指定。
- trace processor 对同 uuid descriptor 是 first-proto-wins，重复发无副作用，但根 track 只在 startTracing 发一次。

### 根 track descriptor 必须直接写 packet（不能只用 SetTrackDescriptor）

这是本机制最隐蔽的坑，务必注意：

SDK 的 `TrackEvent::SetTrackDescriptor` 对"从未发过事件的 track"会 **defer**——见 `track_event_data_source.h` 中 `SetTrackDescriptor` 的实现，它在 `incr_state->seen_tracks.count(track.uuid) == 0` 时直接 return，不写 buffer。而 `seen_tracks` 只在 `WriteTrackDescriptorIfNeeded`（发事件时）插入，且对 `uuid==0` 因 `if (uuid)` 守卫**永远不会**插入 0。

后果：uuid=0 根 track 永远不会发事件，故 `seen_tracks` 永远不含 0，`SetTrackDescriptor` 永远 defer，**根 track descriptor 永不落盘**。trace processor（`track_event_tracker.cc` 中 `ResolveDescriptorTrack`）查 `kDefaultDescriptorTrackUuid`(=0) 的 reservation 找不到 `thread_ordering=EXPLICIT`，于是对所有线程跳过 `SetThreadSortIndex`，排序彻底失效。

PR #6219 只修了 trace_processor 侧，**没修 SDK emit 侧**（其 diff test 用手工 textproto 绕过 SDK）。C++ SDK 用户要让 uuid=0 descriptor 进 buffer，必须绕过 `SetTrackDescriptor`。

本模块的做法（`PerfettoBackend::startTracing`）：

```cpp
::perfetto::Track rootTrack = ::perfetto::Track::Global(0);
::perfetto::TrackEvent::Trace([rootTrack](::perfetto::TrackEvent::TraceContext ctx) {
    auto packet = ctx.NewTracePacket();           // 公开 API，造一个任意 packet
    auto* td = packet->set_track_descriptor();
    rootTrack.Serialize(td);                       // 写 uuid=0、parent_uuid=0
    td->set_thread_ordering(
        ::perfetto::protos::pbzero::TrackDescriptor::THREAD_ORDERING_EXPLICIT);
});
```

`TrackEvent::Trace`（public static）+ `TraceContext::NewTracePacket()`（public）直接往 buffer 写一个 `track_descriptor` packet，绕过 defer。线程 track 的 descriptor 不受此坑影响——`setThreadName` 仍用 `SetTrackDescriptor`，因为线程迟早会发事件，首事件触发 `WriteTrackDescriptorIfNeeded` 从 registry 取出（含 `sibling_order_rank`）写出。

## 容易踩的坑

### 1. perfetto_sdk 链接必须是 PUBLIC

如果将 `perfetto_sdk` 作为 PRIVATE 链接，链接器会报错找不到 `DataSourceHelper<TrackEventDataSource>::type()` 符号（Perfetto SDK 模板实例化，符号须对依赖者可见）。

### 2. PERFETTO_TRACK_EVENT_STATIC_STORAGE 只能定义一次

只在 `TraceCategories.cpp` 中定义一次（仅 `MC_ENABLE_TRACING` 时编译该文件），多处定义会符号重复。

### 3. 分类必须预先定义（仅 Perfetto）

使用未定义的分类会导致 Perfetto 运行时不记录事件、编译期报错。所有使用的分类必须在 `TraceCategories.hpp` 中通过 `PERFETTO_DEFINE_CATEGORIES` 定义。Tracy 后端不消费 category 参数。

### 4. 追踪文件可能为空（Perfetto）

如果没有调用 `stopTracing()` 或 `shutdown()`，Perfetto 追踪数据可能不写入文件。确保程序退出前调用这两个方法。Tracy 无此问题（实时流式采集）。

### 5. Windows 平台需要 /bigobj

Perfetto SDK 与 Tracy 的 `TracyClient.cpp`（内联包含 10+ 个 .cpp）都会生成大量符号，编译时报 "too many sections"。`profiler/CMakeLists.txt` 已为 `perfetto_sdk` 与 `TracyClient` 配置 `/bigobj`。

### 6. MC_TRACE_SCOPED_EVENT 是 RAII 作用域事件

`MC_TRACE_SCOPED_EVENT` 在作用域结束时自动结束事件。双轨下同一作用域可放多个（zone 变量名按 `__LINE__` 唯一化，见上文"双轨宏设计"）。事件持续到所在 `{}` 作用域结束——若想让某 zone 提前结束，用独立 `{}` 包裹。

### 7. 不要在 Lambda 中使用 MC_TRACE_SCOPED_EVENT（MSVC Bug）

在 MSVC 中，lambda 表达式内使用 `MC_TRACE_SCOPED_EVENT` 宏可能导致编译器卡死或内部错误（Perfetto SDK 与 MSVC 的已知兼容性问题）。改用 `spdlog::info` 等日志手段代替。

### 8. 双轨宏不能塞进 EXPECT_NO_THROW 等单语句宏

双轨启用时 `MC_TRACE_*` 展开为多条语句（perfetto 声明 + tracy 声明），塞进 `EXPECT_NO_THROW(stmt)` / `ASSERT_NO_THROW(stmt)` 等只接受单语句的宏会报 "too many arguments"。`MC_TRACE_SET_THREAD_NAME` 是单表达式语句，可正常用于这些宏。测试中对其他宏应直接作为语句调用。

### 9. 分类必须来自枚举树且已注册（仅 Perfetto）

所有 trace 宏的第一个参数必须是 `mc::trace::TraceEvents` 枚举树的叶子节点（如 `TraceEvents.Server.Tick`），其字符串值必须在 `TraceCategories.hpp` 的 `PERFETTO_DEFINE_CATEGORIES` 中注册，否则编译错误。Perfetto SDK 在编译期用字符串内容匹配注册表（非指针比较），故枚举叶等价于字符串字面量，走静态路径、零运行时开销。**不要传 `perfetto::Category` 对象**——SDK 的 `TRACE_EVENT` 宏只接受 `const char*`。

### 10. vcpkg 残留头文件冲突（Perfetto）

本项目曾通过 vcpkg 装 Perfetto v53.0，现已改用 `third_party/perfetto/` 单文件 SDK。若 `vcpkg.json` 删除 perfetto 依赖后未清理 build 目录，残留的 `build/vcpkg_installed/x64-windows/include/perfetto.h`（v53.0，无 `thread_ordering`）会被全局 include 优先命中，导致 `set_thread_ordering` 编译报错。

**解决**：改 `vcpkg.json` 后必须 `rm -rf build` 再重新 configure。CMakeLists 中 `perfetto_sdk` 用 `target_include_directories(... BEFORE PUBLIC ...)` 强制 third_party 路径插队首作为双保险。

### 11. 根 track 必须用 Track::Global(0)，且必须直接写 packet

`PerfettoBackend::startTracing` 里建立根 track 时必须用 `::perfetto::Track::Global(0)`。**不要用 `::perfetto::Track(0)`**——`Track(id)` 构造函数默认 parent 为 `MakeProcessTrack()`，会把 id 与 per-process 随机 cookie 异或，`0 ^ cookie` 得到非 0 uuid，根 track descriptor 不会被 trace processor 识别为 uuid=0，`thread_ordering` 失效。`Track::Global(0)` 用空 parent 构造，不异或，才是真正的 uuid=0。

同时**不能只用 `TrackEvent::SetTrackDescriptor`** 发根 track——它对 uuid=0 永远 defer 不落盘（详见上文"线程排序机制"小节）。必须用 `TrackEvent::Trace` + `TraceContext::NewTracePacket` 直接写 `track_descriptor` packet。

### 12. Tracy 集成走 vendored submodule，不能用 vcpkg

vcpkg 的 tracy port（0.13.1）编译 client 库时**不定义 `TRACY_ENABLE`**，而 tracy 所有采集符号都在 `#ifdef TRACY_ENABLE` 守卫内，消费方自行 `#define TRACY_ENABLE` 会因库里无符号而链接失败，port 也无开启该宏的 feature。故走 `third_party/tracy` git submodule + `add_subdirectory` + `set(TRACY_ENABLE ON CACHE BOOL ... FORCE)`，与 Perfetto 的 vendored 模式一致。`vcpkg.json` 不含 tracy。

**版本锁定 v0.13.1（协议76）**：submodule 固定到正式 release tag `v0.13.1`（commit `05cceee0`），**不跟 master HEAD**。tracy 的 client↔GUI 协议版本必须严格匹配，否则 GUI 报 "Protocol mismatch" 拒绝连接。winget 官方包 `wolfpld.tracy`（0.13.1）是最易获取的 GUI；master HEAD 无对应预编译 GUI。升级 submodule 时须同步告知用户升级 GUI 版本。验证方式：构建后启动 client，`netstat` 应见 `0.0.0.0:8086 LISTENING`，tracy-profiler GUI 连接 `127.0.0.1:8086` 应成功。

### 13. TRACY_NO_SYSTEM_TRACING 必须设 ON（Windows）

Tracy 默认在 Windows 启用 ETW context-switch 采样，需管理员权限运行，普通用户启动会异常。`profiler/CMakeLists.txt` 在 `add_subdirectory` 前 `set(TRACY_NO_SYSTEM_TRACING ON CACHE BOOL ... FORCE)` 关闭它。若需采样上下文切换，需以管理员身份运行并去掉该设置。

### 14. 内存追踪不能绑 `vector::data()` / `shared_ptr::get()`（Tracy 硬失败）

`MC_TRACE_MEM_ALLOC/FREE` 与 `TracyTrackingAlloc`/`TracyObjectTracker` 都受 Tracy 硬不变量约束：对每个 `(name, ptr)`，alloc 与 free 必须严格一对一，`ptr` 须是一次真实堆分配的返回值。违反 → `MemAllocTwice`（"already tracked and not freed"）或 `MemFree` → **终止整个会话，无 flag 可关闭**（`TRACY_IGNORE_MEMORY_FAULTS` 只压 MemFree）。

**绝对不要**用手动宏标 `std::vector::data()`（`reserve` 不 realloc 时同指针重复 alloc、`clear` 不释放不改 `data()`、realloc 旧指针静默失效）、`shared_ptr::get()`（析构时机不定、只 alloc 不 free 后地址被堆复用）。这些都会必然触发硬失败。

**正确工具**（`MemoryTracking.hpp`，自动维持不变量）：
- 追踪 `std::vector` 内部缓冲区 → `TracyTrackingAlloc<T, "Name">`（截获每次 allocate/deallocate，含 realloc 成对 free+alloc）
- 追踪对象驻留 → `TracyObjectTracker<"Name">` 成员守卫（ctor bind(this) 发 alloc、dtor 发 free；move 时宿主须显式 `unbind()` 源 + `bind(this)` 目标，见 `ChunkData` 实现）
- 手动宏仅留给纯 C 式 malloc/free 且能精确捕获 free 的极少数场景

`TracyTrackingAlloc` 有两个模板参数（`T` + NTTP `kName`），MSVC STL 的 `allocator_traits` 默认 rebind（`_Replace_first_parameter`）处理不了多参数/含 NTTP 分配器，**必须显式提供 `template<class U> struct rebind`**，否则 vector 实例化报 "_Replace_first_parameters undefined"。

### 14. Tracy 仅 in-memory，不写文件

Tracy client 无法进程内写 trace 文件——它实时监听 8086 端口，由 tracy GUI 或 `tracy-capture` 工具连接拉取。`ProfilerManager` 对 tracy 不做 start/stop/capture，仅 `setProcessName`/`setThreadName` 双写。要保存 tracy 数据，用 `tracy-capture -o out.tracy -a 127.0.0.1` 在程序运行时抓取。

### 15. Tracy 命名用 tracy::SetThreadName，不用 TracyCSetThreadName

`TracyCSetThreadName` 宏在 `TRACY_ENABLE` 未定义时会展开为对未定义符号 `___tracy_set_thread_name` 的调用（它定义在 `#ifndef TRACY_ENABLE` 之外）。故门面里线程命名走 C++ API `tracy::SetThreadName(name.c_str())`（始终编译、`TRACY_API` 导出），程序命名走安全宏 `TracySetProgramName`（禁用时空展开）。
