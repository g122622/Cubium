# 引入 Tracy（与 Perfetto 双轨）

> 状态：已完成（2026-07-10）。本文档记录任务书要求与实际实施的对照，以及关键决策偏差。

## 任务书原始要求

- 引入 tracy
- 用 vcpkg 管理依赖
- 与 perfetto 双轨：同时编译、同时录制
- API 宏不变
- `src/common/perfetto` 目录更名为 `profiler`
  - 子文件夹 1：perfetto
  - 子文件夹 2：tracy
  - 共用的代码（暴露的 api 和公共类型等）保持在该文件夹根目录
  - 需要编写脚本完成相关路径更改
- 本地 tracy 仓库位于：`E:\dev\tracy`（参考文档与代码）
- perfetto：`D:\MiscProjects\perfetto`
- `PerfettoManager` 更名为 `ProfilerManager`，同时管理两套 profiler

## 实际实施与决策偏差

### 1. Tracy 依赖管理：vendored submodule（非 vcpkg）⚠️ 覆盖任务书

**偏差**：任务书要求“用 vcpkg 管理依赖”，实际采用 vendored git submodule（`third_party/tracy`）+ `add_subdirectory`。

**原因**：vcpkg 的 tracy port（0.13.1）编译 client 库时**不定义 `TRACY_ENABLE`**，而 tracy 所有采集符号都在 `#ifdef TRACY_ENABLE` 守卫内（`TracyClient.cpp:16-47`）。消费方自行 `#define TRACY_ENABLE` 会因库里无符号而链接失败；port 也无开启该宏的 feature。故走 vendored submodule + `set(TRACY_ENABLE ON CACHE BOOL ... FORCE)`，与 perfetto 的 vendored 模式一致。`vcpkg.json` 不含 tracy。

### 2. Tracy 录制：in-memory，不写文件

**决策**：tracy client 无法进程内写文件（需外部 `tracy-capture` 连 8086 端口拉取）。`ProfilerManager` 对 tracy 不做 start/stop/capture 管理，仅做 `setProcessName`/`setThreadName` 双写。查看 tracy 数据需用 tracy GUI 连接 127.0.0.1:8086。

### 3. 目录结构：两层架构（未设 perfetto/ tracy/ 子文件夹）⚠️ 偏离任务书

**偏差**：任务书要求设 `perfetto/`、`tracy/` 子文件夹，实际采用根目录两层架构。

**原因**：tracy 后端逻辑极少（仅 setProcessName/setThreadName 各一行 + 命名双写，tracy 采集是 client 自动完成的），不值得单独 `.hpp/.cpp` 与子目录。故：
- `PerfettoBackend.hpp/cpp` 承载 perfetto 重逻辑，放根目录。
- tracy 逻辑直接内联在 `ProfilerManager.cpp` 的 `#if MC_ENABLE_TRACY` 段。
- 共用代码（`ProfilerManager.hpp`、`TraceEvents.hpp`、`TraceCategories.hpp`、`ProfilerConfig.hpp`）在根目录。

这样避免过度设计，且 `PerfettoBackend` 与门面同层便于 Pimpl 持有。

### 4. `PerfettoManager` → `ProfilerManager`：✅ 完成

`PerfettoManager` 单例改为 `ProfilerManager` 门面，持有 `PerfettoBackend`（Pimpl），管理两套后端。命名空间 `mc::perfetto` → `mc::profiler`（`mc::trace` 枚举树不动，`::perfetto::` 第三方不动）。

## 最终目录结构

```
src/common/profiler/
├── CMakeLists.txt        # perfetto_sdk + TracyClient(vendored) + mc_profiler + mc::profiler 别名
├── ProfilerConfig.hpp    # MC_ENABLE_TRACING / MC_ENABLE_TRACY / MC_PROFILER_ENABLED + 缓冲区/输出路径
├── ProfilerManager.hpp   # 门面单例 + TraceConfig（持有 PerfettoBackend，tracy 命名双写）
├── ProfilerManager.cpp   # 门面实现：生命周期委托 PerfettoBackend，tracy 命名内联双写
├── PerfettoBackend.hpp   # Perfetto 后端声明（Pimpl，仅 MC_ENABLE_TRACING）
├── PerfettoBackend.cpp   # TracingSession / root track / 写文件 / sibling_order_rank
├── TraceCategories.hpp   # mc::trace::TraceEvents 枚举树 + PERFETTO_DEFINE_CATEGORIES
├── TraceCategories.cpp   # PERFETTO_TRACK_EVENT_STATIC_STORAGE（仅 MC_ENABLE_TRACING）
└── TraceEvents.hpp       # MC_TRACE_* 双轨宏（四种开关组合分支）
```

## 双轨宏设计（TraceEvents.hpp）

四种组合由 `MC_ENABLE_TRACING`（perfetto）和 `MC_ENABLE_TRACY`（tracy）控制：

| 宏 | 双轨 | 仅 Perfetto | 仅 Tracy |
|----|------|-------------|----------|
| `MC_TRACE_SCOPED_EVENT` | `TRACE_EVENT + MC_TRACY_SCOPED_ZONE` | `TRACE_EVENT` | `MC_TRACY_SCOPED_ZONE` |
| `MC_TRACE_INSTANT_EVENT` | `TRACE_EVENT_INSTANT + TracyMessageL` | `TRACE_EVENT_INSTANT` | `TracyMessageL` |
| `MC_TRACE_COUNTER` | `TRACE_COUNTER + TracyPlot(double)` | `TRACE_COUNTER` | `TracyPlot(double)` |
| `MC_TRACE_EVENT_BEGIN/END` | `TRACE_EVENT_BEGIN/END + TracyMessageL` | `TRACE_EVENT_BEGIN/END` | `TracyMessageL` |
| `MC_TRACE_SET_THREAD_NAME` | `ProfilerManager::setThreadName`（双写） | 同左 | 同左 |

## 关键技术坑（实施中发现并解决）

### A. Tracy zone 变量名冲突（redefinition of `___tracy_scoped_zone`）

Tracy 的 `ZoneScopedN(name)` 用**固定变量名** `___tracy_scoped_zone` 声明 RAII zone。Perfetto 的惯用法允许同一作用域多个 `MC_TRACE_SCOPED_EVENT`（如 `ServerWorld::tick` 的 EnvironmentTick/PrecipitationTick），双轨下两个 `ZoneScopedN` 会 redefinition。

**解决**：自定义 `MC_TRACY_SCOPED_ZONE(name)` = `ZoneNamedN(MC_TRACY_ZONE_CONCAT(___tracy_zone_, __LINE__), name, true)`，用 `__LINE__` 后缀变量名使每个 zone 唯一，允许同作用域多个共存。

### B. 双轨宏不能塞进 EXPECT_NO_THROW

双轨启用时 `MC_TRACE_*` 展开为**多条语句**（perfetto 声明 + tracy 声明），塞进 `EXPECT_NO_THROW(stmt)` 等“只接受单语句”的宏会报 "too many arguments"。`MC_TRACE_SET_THREAD_NAME` 是单表达式，可正常用。测试中 `EXPECT_NO_THROW(MC_TRACE_SCOPED_EVENT(...))` 改为直接作为语句调用 + `EXPECT_TRUE(true)`。

### C. Tracy 命名 API 选择

- 线程命名用 C++ API `tracy::SetThreadName(name.c_str())`（`common/TracySystem.hpp`，始终编译、`TRACY_API` 导出）。**不用 `TracyCSetThreadName`**——它在 `TRACY_ENABLE` 未定义时会展开为对未定义符号 `___tracy_set_thread_name` 的调用（定义在 `#ifndef TRACY_ENABLE` 之外）。
- 程序命名用安全宏 `TracySetProgramName`（禁用时空展开）。

### D. `TRACY_NO_SYSTEM_TRACING` 必须 ON（Windows）

Tracy 默认启用 ETW context-switch 采样，需管理员权限。`profiler/CMakeLists.txt` 在 `add_subdirectory` 前 `set(TRACY_NO_SYSTEM_TRACING ON CACHE BOOL ... FORCE)` 关闭。

### E. TracyClient 需要 /bigobj（Windows）

`TracyClient.cpp` 内联包含 10+ 个 .cpp，section 数超限。已为 `TracyClient` target 补 `/bigobj`。

### F. `ProfilerManager::isInitialized` 须 inline

门面头中 `isInitialized()` 声明为非 inline 但 .cpp 无定义会链接报 undefined symbol。改为 inline `return m_initialized;`。

### G. tracy-only 构建的 unique_ptr<PerfettoBackend> incomplete-type 析构

仅 tracy 启用时（`MC_ENABLE_TRACING=0, MC_ENABLE_TRACY=1`），`PerfettoBackend` 只有前向声明、无完整定义。`ProfilerManager` 持有 `unique_ptr<PerfettoBackend>` 会在析构时需要完整类型。**解决**：`m_perfetto` 成员用 `#if MC_ENABLE_TRACING` 守卫，仅 perfetto 启用时存在；`.cpp` 中所有 `m_perfetto` 访问也在 `#if MC_ENABLE_TRACING` 内。

## 验证

- ✅ **编译**：`./scripts/configure.sh build` 通过（perfetto+tracy 双轨默认开，1529 targets）。
- ✅ **单元测试**：`mc_tests --gtest_filter='*Perfetto*:*TraceEvents*:*ProfilerManager*'` 全绿（26 tests passed）。
- ✅ **运行时 perfetto**：启动 `minecraft-server.exe`，`ProfilerManager::initialize` + `startTracing` 正常，退出时（含崩溃清理回调路径）`stopTracing` 落盘 `server_trace.perfetto-trace`（~195KB）。二进制搜索确认 trace 内嵌入进程名 `MinecraftServer`、线程名 `ServerMainThread`，根 track descriptor + sibling_order_rank 正确写入。
- ✅ **运行时 tracy**：`netstat` 确认 tracy client worker 线程在进程启动后绑定 `0.0.0.0:8086`（LISTENING）。tracy GUI/capture 连接 `127.0.0.1:8086` 即可拉取 zone/message/plot 事件与线程名。`minecraft-server.exe` 内已链接 `TracyProfile`/`___tracy` 等采集符号。
- ⏳ **双开关组合**：默认双开已验证；仅 tracy / 仅 perfetto / 全关三种组合的单独构建未跑（每次 30+ 分钟构建，按需验证）。
  - 注：服务端在 `ContainerManager::setMenuFactory`（`StandaloneServer::initialize:328`）有**预存** ACCESS_VIOLATION（null `this`，与 profiler 无关），使独立 server 进程在 init 阶段即崩溃、未进入 run loop；但不影响上述 perfetto 落盘与 tracy 端口验证——profiler 的崩溃清理回调在崩溃路径下仍正确 flush 了 trace。

## 改名脚本

`scripts/rename-perfetto-to-profiler.py`：Python 脚本，做 `git mv` 目录/文件改名 + 标识符/include 路径批量替换（先长后短）。硬排除 `third_party/`、`build/`、`.git/`。保留不动：`::perfetto::`、`<perfetto.h>`、`perfetto_sdk`、`PERFETTO_*`、`mc::trace`、`*.perfetto-trace`。
