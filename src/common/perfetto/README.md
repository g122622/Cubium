# Perfetto 性能追踪模块

Perfetto 模块提供了一套完整的性能追踪基础设施，基于 Google Perfetto SDK 实现，支持编译时开关、零开销禁用、细粒度分类控制。

## 目录结构

```
src/common/perfetto/
├── CMakeLists.txt        # 构建配置，定义 perfetto_sdk 和 mc_perfetto 库
├── PerfettoConfig.hpp    # 编译时配置开关（总开关、子系统开关、缓冲区配置）
├── PerfettoManager.hpp   # 单例追踪管理器，负责生命周期管理
├── PerfettoManager.cpp   # 管理器实现（Pimpl 模式，追踪会话管理）
├── TraceCategories.hpp   # 分类枚举树（TraceEvents）+ PERFETTO_DEFINE_CATEGORIES 注册
├── TraceCategories.cpp   # 静态存储定义（PERFETTO_TRACK_EVENT_STATIC_STORAGE）
└── TraceEvents.hpp       # 追踪事件便捷宏（MC_TRACE_SCOPED_EVENT 等）
```

## 内部模块关系

```
PerfettoConfig.hpp（编译时配置）
        │
        ├──────────────────────┐
        ▼                      ▼
TraceCategories.hpp    PerfettoManager.hpp
        │                      │
        ▼                      │
TraceCategories.cpp            │
        │                      │
        └──────────┬───────────┘
                   ▼
           PerfettoManager.cpp
                   │
                   ▼
            TraceEvents.hpp（便捷宏封装）
                   │
                   ▼
              应用代码（MC_TRACE_* 宏）
```

- **PerfettoConfig.hpp**：定义编译时总开关（MC_ENABLE_TRACING）与缓冲区/输出路径配置
- **TraceCategories.hpp**：定义分类枚举树 `mc::trace::TraceEvents`（按子系统组织的 `const char*` 嵌套结构体），并通过 `PERFETTO_DEFINE_CATEGORIES` 注册所有分类。调用方只能使用枚举树叶节点
- **PerfettoManager**：单例模式，管理追踪系统的初始化、启动、停止、文件输出
- **TraceEvents.hpp**：封装 Perfetto 原生宏，提供统一的 MC_TRACE_* 接口（category 参数取自枚举树）

## 上下游外部依赖关系

### 被谁依赖

整个项目的性能追踪都依赖此模块。主要使用方：

- **服务端核心**：`MinecraftServer`、`ServerWorld`、`ServerChunkManager`、`ChunkGenerateTask` 等
- **客户端核心**：`ClientApplication`、`ClientWorld`、`TridentEngine`、`ChunkRenderer` 等
- **世界生成**：`NoiseChunkGenerator`、`BiomeRegistry`、光照引擎等
- **网络同步**：`ChunkSync`、`EntitySyncManager`、`NetworkClient` 等
- **存储系统**：`RocksDBDatabase`、`SectionManager`、`AutoSave` 等

### 依赖了谁

| 依赖 | 用途 |
|------|------|
| **Perfetto SDK** | 第三方单文件 SDK（`third_party/perfetto/perfetto.{h,cc}`，官方 releases，含 PR #6219 thread_ordering 支持） |
| **spdlog** | 日志输出（追踪系统状态信息） |
| **ws2_32**（Windows） | WinSock2 库，Perfetto 网络功能需要 |

## 线程排序机制

基于 PR #6219（release v57.1+），可在 Perfetto UI 中固定线程显示顺序：

- **根 track descriptor（uuid=0）** 上设 `thread_ordering = THREAD_ORDERING_EXPLICIT`，由 `PerfettoManager::startTracing` 用 `Track::Global(0)` 发出。**禁止用 `Track(0)`**——它会与 per-process 随机 cookie 异或得到非 0 uuid，根 track 失效。
- 每个线程 track descriptor 上设 `sibling_order_rank`（`PerfettoManager::setThreadName` 内部完成），值越小越靠前；未设默认 0（排最前），故命名线程显式给 1-100 避免意外排前。
- 固定线程 rank：`MemoryTrace=1`、`ClientMainThread=2`、`IntegratedServerThread=3`、`AudioEngineWorker=4`、`ServerMainThread=5`。
- worker 池用 `rankBase + workerId` 精确排序，三组分块排列、组内按 workerId 升序：`ServerCompute-N` = 100+N，`ServerIO-N` = 200+N，`ChunkMeshWorker-N` = 300+N。每组间隔 100，避免线程数 >10 时跨组相交。UI 顺序为 ServerCompute → ServerIO → ChunkMeshWorker。`ServerWorkerPool::workerThread` 按 `m_poolName`（`"ServerCompute"`/`"ServerIO"`）选 rankBase；`MeshWorkerPool` 固定 300。
- trace processor 对同 uuid descriptor 是 first-proto-wins，重复发无副作用，但根 track 只在 startTracing 发一次。

### 根 track descriptor 必须直接写 packet（不能只用 SetTrackDescriptor）

这是本机制最隐蔽的坑，务必注意：

SDK 的 `TrackEvent::SetTrackDescriptor` 对"从未发过事件的 track"会 **defer**——见 `track_event_data_source.h` 中 `SetTrackDescriptor` 的实现，它在 `incr_state->seen_tracks.count(track.uuid) == 0` 时直接 return，不写 buffer。而 `seen_tracks` 只在 `WriteTrackDescriptorIfNeeded`（发事件时）插入，且对 `uuid==0` 因 `if (uuid)` 守卫**永远不会**插入 0。

后果：uuid=0 根 track 永远不会发事件，故 `seen_tracks` 永远不含 0，`SetTrackDescriptor` 永远 defer，**根 track descriptor 永不落盘**。trace processor（`track_event_tracker.cc` 中 `ResolveDescriptorTrack`）查 `kDefaultDescriptorTrackUuid`(=0) 的 reservation 找不到 `thread_ordering=EXPLICIT`，于是对所有线程跳过 `SetThreadSortIndex`，排序彻底失效。

PR #6219 只修了 trace_processor 侧，**没修 SDK emit 侧**（其 diff test 用手工 textproto 绕过 SDK）。C++ SDK 用户要让 uuid=0 descriptor 进 buffer，必须绕过 `SetTrackDescriptor`。

本模块的做法（`PerfettoManager::startTracing`）：

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

`TrackEvent::Trace`（public static）+ `TraceContext::NewTracePacket()`（public）直接往 buffer 写一个 `track_descriptor` packet，绕过 defer。验证方式：用 `perfetto.protos.perfetto.trace.perfetto_trace_pb2` 解析 trace 文件，应看到 `uuid=0, thread_ordering=1` 的 descriptor 包，以及各线程 `sibling_order_rank` 包。trace processor 据此调 `SetThreadSortIndex`，UI 升序排列。

注意：线程 track 的 descriptor 不受此坑影响——`setThreadName` 仍用 `SetTrackDescriptor`，因为线程迟早会发事件（counter/event），首事件触发 `WriteTrackDescriptorIfNeeded` 从 registry 取出（含 `sibling_order_rank`）写出。

## 容易踩的坑

### 1. perfetto_sdk 链接必须是 PUBLIC

如果将 `perfetto_sdk` 作为 PRIVATE 链接，链接器会报错找不到 `DataSourceHelper<TrackEventDataSource>::type()` 符号。

```cmake
# 正确
target_link_libraries(mc_perfetto PUBLIC perfetto_sdk)

# 错误 - 会导致链接错误
target_link_libraries(mc_perfetto PRIVATE perfetto_sdk)
```

**原因**：Perfetto SDK 使用模板实例化，符号必须在链接时对所有依赖者可见。

### 2. PERFETTO_TRACK_EVENT_STATIC_STORAGE 只能定义一次

如果在多个 .cpp 文件中定义，会导致符号重复定义错误。只在 `TraceCategories.cpp` 中定义一次即可。

### 3. 分类必须预先定义

使用未定义的分类会导致运行时事件不被记录。所有使用的分类必须在 `TraceCategories.hpp` 中通过 `PERFETTO_DEFINE_CATEGORIES` 定义。

### 4. 追踪文件可能为空

如果没有调用 `stopTracing()` 或 `shutdown()`，追踪数据可能不会写入文件。确保程序退出前调用这两个方法。

### 5. Windows 平台需要 /bigobj

Perfetto SDK 生成大量符号，编译时会报错 "too many sections"。CMakeLists.txt 中已配置此选项。

### 6. MC_TRACE_SCOPED_EVENT 是 RAII 作用域事件

`MC_TRACE_SCOPED_EVENT` 在作用域结束时自动结束事件。**同一作用域内不能有多个 MC_TRACE_SCOPED_EVENT**，否则会导致事件嵌套错误。每个 `MC_TRACE_SCOPED_EVENT` 必须放在独立的大括号作用域内：

```cpp
// 错误：同一作用域内有多个 trace event
void processFrame() {
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "HandleEvents");  // 不会正确结束
    handleEvents();
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Update");  // 嵌套错误
    update(deltaTime);
}

// 正确：每个 trace event 放在独立的作用域内
void processFrame() {
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "HandleEvents");
        handleEvents();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Update");
        update(deltaTime);
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Render");
        render();
    }
}
```

### 7. 不要在 Lambda 中使用 MC_TRACE_SCOPED_EVENT（MSVC Bug）

在 MSVC 中，lambda 表达式内使用 `MC_TRACE_SCOPED_EVENT` 宏可能导致编译器卡死或内部错误。这是 Perfetto SDK 与 MSVC 的已知兼容性问题。**改用 `spdlog::info` 等日志手段代替**。

### 8. 禁用追踪时的存根实现

当 `MC_ENABLE_TRACING=0` 时，`PerfettoManager` 的所有方法仍可调用但为空操作（零开销）。无需修改调用代码。

### 9. 分类必须来自枚举树且已注册

所有 trace 宏的第一个参数必须是 `mc::trace::TraceEvents` 枚举树的叶子节点（如 `TraceEvents.Server.Tick`），其字符串值必须在 `TraceCategories.hpp` 的 `PERFETTO_DEFINE_CATEGORIES` 中注册，否则编译错误。Perfetto SDK 在编译期用字符串内容匹配注册表（非指针比较），故枚举叶等价于字符串字面量，走静态路径、零运行时开销。**不要传 `perfetto::Category` 对象**——SDK 的 `TRACE_EVENT` 宏只接受 `const char*`，传 `Category` 对象无法编译。

### 10. vcpkg 残留头文件冲突

本项目曾通过 vcpkg 装 Perfetto v53.0，现已改用 `third_party/perfetto/` 单文件 SDK。若 `vcpkg.json` 删除了 perfetto 依赖后未清理 build 目录，残留的 `build/vcpkg_installed/x64-windows/include/perfetto.h`（v53.0，无 `thread_ordering`）会被全局 include 优先命中，导致 `set_thread_ordering` 编译报错。

**解决**：改 `vcpkg.json` 后必须 `rm -rf build` 再重新 configure。CMakeLists 中 `perfetto_sdk` 用 `target_include_directories(... BEFORE PUBLIC ...)` 强制 third_party 路径插队首作为双保险。

### 11. 根 track 必须用 Track::Global(0)，且必须直接写 packet

`PerfettoManager::startTracing` 里建立根 track 时必须用 `::perfetto::Track::Global(0)`。**不要用 `::perfetto::Track(0)`**——`Track(id)` 构造函数默认 parent 为 `MakeProcessTrack()`，会把 id 与 per-process 随机 cookie 异或，`0 ^ cookie` 得到非 0 uuid，根 track descriptor 不会被 trace processor 识别为 uuid=0，`thread_ordering` 失效。`Track::Global(0)` 用空 parent 构造，不异或，才是真正的 uuid=0。

同时**不能只用 `TrackEvent::SetTrackDescriptor`** 发根 track——它对 uuid=0 永远 defer 不落盘（详见上文"线程排序机制"小节）。必须用 `TrackEvent::Trace` + `TraceContext::NewTracePacket` 直接写 `track_descriptor` packet。
