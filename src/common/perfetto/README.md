# Perfetto 性能追踪模块

Perfetto 模块提供了一套完整的性能追踪基础设施，基于 Google Perfetto SDK 实现，支持编译时开关、零开销禁用、细粒度分类控制。

## 目录结构

```
src/common/perfetto/
├── CMakeLists.txt        # 构建配置
├── PerfettoConfig.hpp    # 编译时配置开关
├── PerfettoManager.hpp   # 追踪管理器头文件
├── PerfettoManager.cpp   # 追踪管理器实现
├── TraceCategories.hpp   # 追踪分类定义
├── TraceCategories.cpp   # 追踪分类静态存储
└── TraceEvents.hpp       # 追踪事件便捷宏
```

## 文件详解

### CMakeLists.txt

构建配置文件，负责：

1. **perfetto_sdk 静态库**：按照 Perfetto SDK 官方文档建议，将 `third_party/perfetto/perfetto.cc` 编译为独立的静态库
2. **mc_perfetto 库**：项目追踪库，包含管理器和分类实现
3. **编译选项**：Windows 平台的 `/bigobj`、`WIN32_LEAN_AND_MEAN`、`NOMINMAX` 等
4. **链接配置**：**关键点 - perfetto_sdk 必须作为 PUBLIC 链接库**，确保符号对所有依赖者可用

```cmake
# 关键配置
if(MC_ENABLE_TRACING)
    target_compile_definitions(mc_perfetto PUBLIC MC_ENABLE_TRACING=1)
    target_link_libraries(mc_perfetto PUBLIC perfetto_sdk)  # 必须是 PUBLIC
endif()
```

### PerfettoConfig.hpp

编译时配置开关，定义了：

#### 总开关

```cpp
#define MC_ENABLE_TRACING 1  // 由 CMake 设置，禁用时所有追踪宏展开为空操作
```

#### 子系统开关

| 开关 | 控制的分类 |
|------|-----------|
| `MC_TRACE_RENDERING` | rendering.* |
| `MC_TRACE_GAME_TICK` | game.* |
| `MC_TRACE_CHUNK_GENERATION` | world.chunk_gen, world.biome |
| `MC_TRACE_CHUNK_LOAD` | world.chunk, world.chunk_load |
| `MC_TRACE_NETWORK` | network.* |
| `MC_TRACE_IO` | io.* |
| `MC_TRACE_MEMORY` | memory.* |

#### 缓冲区和输出配置

```cpp
#define MC_TRACE_BUFFER_SIZE_KB 65536  // 64 MB 默认缓冲区
#define MC_TRACE_DEFAULT_OUTPUT "trace.perfetto-trace"
```

#### 便捷宏

```cpp
#define MC_TRACING_ENABLED 1                    // 总开关状态
#define MC_TRACING_RENDERING_ENABLED 1          // 渲染追踪状态
#define MC_TRACING_GAME_TICK_ENABLED 1          // 游戏刻追踪状态
// ...
```

### PerfettoManager.hpp

单例追踪管理器，负责：

- 追踪系统生命周期管理（初始化、启动、停止、关闭）
- 运行时启用/禁用控制
- 进程和线程命名设置
- 配置管理

#### TraceConfig 配置结构

```cpp
struct TraceConfig {
    bool enabled = true;                              // 运行时开关
    bool outputToFile = true;                         // 输出到文件
    std::string outputPath = MC_TRACE_DEFAULT_OUTPUT; // 输出路径
    size_t bufferSizeKb = MC_TRACE_BUFFER_SIZE_KB;    // 缓冲区大小
    bool recordProcessMetadata = true;                // 记录进程元数据
    bool recordThreadNames = true;                    // 记录线程名称
    std::vector<std::string> enabledCategories;       // 启用的分类
    std::vector<std::string> disabledCategories;      // 禁用的分类
};
```

#### 核心 API

```cpp
class PerfettoManager {
public:
    static PerfettoManager& instance();

    void initialize(const TraceConfig& config = {});
    void shutdown();
    void startTracing();
    void stopTracing();
    void flush();

    [[nodiscard]] bool isEnabled() const;
    [[nodiscard]] bool isInitialized() const;
    void setEnabled(bool enabled);

    void setProcessName(const std::string& name);
    void setThreadName(const std::string& name);
};
```

#### 存根实现

当 `MC_ENABLE_TRACING=0` 时，所有方法展开为空操作，确保零开销。

### PerfettoManager.cpp

PerfettoManager 的实现，包含：

1. **Pimpl 模式**：使用 `class Impl` 隐藏 Perfetto 特定类型
2. **初始化流程**：调用 `perfetto::Tracing::Initialize()` 和 `perfetto::TrackEvent::Register()`
3. **追踪会话管理**：创建、配置、启动、停止追踪会话
4. **文件写入**：手动读取追踪数据并写入文件

#### 关键实现细节

```cpp
void PerfettoManager::startTracing() {
    // 配置追踪会话
    ::perfetto::TraceConfig cfg;
    cfg.add_buffers()->set_size_kb(static_cast<uint32_t>(m_config.bufferSizeKb));

    // 配置数据源
    auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");

    // 创建并启动追踪会话
    m_impl->tracingSession = ::perfetto::Tracing::NewTrace();
    m_impl->tracingSession->Setup(cfg);
    m_impl->tracingSession->StartBlocking();
}
```

### TraceCategories.hpp

追踪分类定义文件，定义了项目中所有可用的追踪分类。

#### 分类列表

| 分类 | 描述 |
|------|------|
| **渲染分类** | |
| `rendering.frame` | 帧渲染生命周期事件 |
| `rendering.vulkan` | Vulkan API 调用 |
| `rendering.chunk_mesh` | 区块网格生成和上传 |
| `rendering.entity` | 实体渲染 |
| `rendering.begin_frame` | 帧开始阶段 |
| `rendering.uniform_update` | Uniform 缓冲区更新 |
| `rendering.sky` | 天空渲染 |
| `rendering.chunk_draw` | 区块绘制 |
| `rendering.gui` | GUI 渲染 |
| `rendering.end_frame` | 帧结束阶段 |
| `rendering.viewport` | 视口和裁剪设置 |
| `rendering.descriptor_bind` | 描述符集绑定 |
| `rendering.push_constants` | 推送常量更新 |
| `rendering.command_buffer` | 命令缓冲区操作 |
| `rendering.weather` | 天气渲染 |
| `rendering.cloud` | 云层渲染 |
| **游戏逻辑分类** | |
| `game.tick` | 游戏刻处理 |
| `game.entity` | 实体更新 |
| `game.physics` | 物理模拟 |
| `game.ai` | AI 目标处理 |
| **世界分类** | |
| `world.chunk` | 区块操作 |
| `world.chunk_gen` | 区块生成各阶段 |
| `world.chunk_load` | 区块加载和卸载 |
| `world.biome` | 生物群系生成 |
| **网络分类** | |
| `network.packet` | 网络包处理 |
| `network.sync` | 状态同步 |
| `network.connection` | 连接管理 |
| **I/O 分类** | |
| `io.file` | 文件 I/O 操作 |
| `io.resource` | 资源加载 |
| **内存分类** | |
| `memory.allocation` | 内存分配追踪 |
| `memory.cache` | 缓存操作 |
| **服务端分类** | |
| `server.tick` | 服务端游戏刻处理 |
| `server.network` | 服务端网络处理 |
| `server.player` | 服务端玩家管理 |
| `server.world` | 服务端世界更新 |
| `server.chunk` | 服务端区块处理 |
| `server.entity` | 服务端实体更新 |
| `server.lighting` | 服务端光照处理 |
| **挖掘分类** | |
| `client.input.mining` | 挖掘输入事件处理 |
| `server.world.mining` | 进一步的挖掘处理 |

### TraceCategories.cpp

追踪分类静态存储定义文件。

```cpp
// 必须且只能在一个 .cpp 文件中定义
PERFETTO_TRACK_EVENT_STATIC_STORAGE();
```

**重要**：这是 Perfetto SDK 的要求，`PERFETTO_TRACK_EVENT_STATIC_STORAGE()` 必须且只能在一个编译单元中定义。

### TraceEvents.hpp

追踪事件便捷宏，提供统一的追踪接口。

#### 基础宏

```cpp
// 作用域事件（推荐使用）
MC_TRACE_EVENT(category, name, ...)
MC_TRACE_EVENT_IF(condition, category, name, ...)

// 计数器
MC_TRACE_COUNTER(category, name, value)

// 手动事件
MC_TRACE_EVENT_BEGIN(category, name, ...)
MC_TRACE_EVENT_END(category)

// 瞬时事件
MC_TRACE_INSTANT(category, name, ...)

// 分类检查
MC_TRACE_CATEGORY_ENABLED(category)
```

#### 子系统专用宏

```cpp
// 渲染
MC_TRACE_RENDERING_EVENT(name, ...)
MC_TRACE_RENDERING_COUNTER(name, value)
MC_TRACE_VULKAN_EVENT(name, ...)
MC_TRACE_CHUNK_MESH_EVENT(name, ...)
MC_TRACE_BEGIN_FRAME(name, ...)
MC_TRACE_UNIFORM_UPDATE(name, ...)
MC_TRACE_SKY(name, ...)
MC_TRACE_CHUNK_DRAW(name, ...)
MC_TRACE_GUI(name, ...)
MC_TRACE_END_FRAME(name, ...)
MC_TRACE_VIEWPORT(name, ...)
MC_TRACE_DESCRIPTOR_BIND(name, ...)
MC_TRACE_PUSH_CONSTANTS(name, ...)
MC_TRACE_CMD_BUFFER(name, ...)

// 游戏逻辑
MC_TRACE_TICK_EVENT(name, ...)
MC_TRACE_TICK_COUNTER(name, value)
MC_TRACE_ENTITY_EVENT(name, ...)
MC_TRACE_AI_EVENT(name, ...)

// 世界
MC_TRACE_CHUNK_GEN_EVENT(name, ...)
MC_TRACE_CHUNK_LOAD_EVENT(name, ...)

// 网络
MC_TRACE_NETWORK_EVENT(name, ...)

// 服务端
MC_TRACE_SERVER_TICK_EVENT(name, ...)
MC_TRACE_SERVER_NETWORK_EVENT(name, ...)
MC_TRACE_SERVER_PLAYER_EVENT(name, ...)
MC_TRACE_SERVER_WORLD_EVENT(name, ...)
MC_TRACE_SERVER_CHUNK_EVENT(name, ...)
MC_TRACE_SERVER_ENTITY_EVENT(name, ...)
MC_TRACE_SERVER_TICK_COUNTER(name, value)
```

#### 线程和进程命名

```cpp
MC_TRACE_SET_THREAD_NAME(name)
MC_TRACE_SET_PROCESS_NAME(name)
MC_TRACE_SET_THREAD_NAME_FOR(tid, name)
```

## 文件关系图

```
                    PerfettoConfig.hpp
                           │
           ┌───────────────┴───────────────┐
           │                               │
           ▼                               ▼
    TraceCategories.hpp              PerfettoManager.hpp
           │                               │
           │                               │
           ▼                               │
    TraceCategories.cpp                    │
           │                               │
           └───────────────┬───────────────┘
                           │
                           ▼
                    PerfettoManager.cpp
                           │
           ┌───────────────┴───────────────┐
           │                               │
           ▼                               ▼
    TraceEvents.hpp                   应用代码
           │
           ▼
    (使用 MC_TRACE_* 宏)
```

## 模块职责

### 整体职责

1. **性能追踪基础设施**：提供统一的性能追踪接口
2. **零开销禁用**：编译时禁用时，所有宏展开为空操作，无运行时开销
3. **细粒度控制**：支持按子系统启用/禁用追踪
4. **文件输出**：生成标准 Perfetto 追踪文件，可用 Perfetto UI 分析
5. **线程安全**：Perfetto SDK 内部保证线程安全

### 输入

- **追踪事件**：通过 `MC_TRACE_*` 宏记录的事件
- **计数器值**：通过 `MC_TRACE_COUNTER` 记录的数值
- **配置**：通过 `TraceConfig` 结构传入的配置

### 输出

- **追踪文件**：`.perfetto-trace` 格式的二进制文件
- **分析界面**：可通过 https://ui.perfetto.dev 打开分析

### 依赖项

| 依赖 | 用途 |
|------|------|
| **Perfetto SDK** | 第三方追踪库，位于 `third_party/perfetto/` |
| **spdlog** | 日志输出 |
| **ws2_32** (Windows) | WinSock2 库，Perfetto 网络功能需要 |

## 使用方法

### 启用追踪

```powershell
# CMake 配置时启用追踪
cmake -B build -DMC_ENABLE_TRACING=ON -DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### 初始化

```cpp
#include "common/perfetto/TraceEvents.hpp"
#include "common/perfetto/PerfettoManager.hpp"

int main() {
    // 初始化追踪系统
    mc::perfetto::TraceConfig config;
    config.outputPath = "trace.perfetto-trace";
    config.bufferSizeKb = 65536;  // 64 MB
    mc::perfetto::PerfettoManager::instance().initialize(config);
    mc::perfetto::PerfettoManager::instance().startTracing();

    // 设置进程/线程名称
    MC_TRACE_SET_PROCESS_NAME("MinecraftServer");
    MC_TRACE_SET_THREAD_NAME("MainThread");

    // ... 应用程序主循环 ...

    // 关闭追踪系统
    mc::perfetto::PerfettoManager::instance().stopTracing();
    mc::perfetto::PerfettoManager::instance().shutdown();

    return 0;
}
```

### 记录事件

```cpp
// 作用域事件（推荐）
void renderFrame() {
    MC_TRACE_EVENT("rendering.frame", "RenderFrame");
    // 函数结束时自动结束事件
}

// 带参数的事件
void processChunk(int x, int z) {
    MC_TRACE_EVENT("world.chunk_gen", "ProcessChunk", "x", x, "z", z);
}

// 计数器
void updateFPS(double fps) {
    MC_TRACE_COUNTER("rendering.frame", "FPS", static_cast<int64_t>(fps));
}

// 手动开始/结束事件（跨函数场景）
void startAsync() {
    MC_TRACE_EVENT_BEGIN("network", "AsyncOp", "id", 123);
}
void endAsync() {
    MC_TRACE_EVENT_END("network");
}

// 瞬时事件
void onEvent() {
    MC_TRACE_INSTANT("game.tick", "SomethingHappened");
}

// 条件事件
void maybeTrace(bool condition) {
    MC_TRACE_EVENT_IF(condition, "rendering.frame", "ConditionalEvent");
}
```

### 分析追踪文件

1. 运行程序生成 `trace.perfetto-trace` 文件
2. 打开 https://ui.perfetto.dev
3. 点击 "Open trace file" 加载文件
4. 在时间线上查看事件、分析性能瓶颈

## 容易踩的坑

### 1. perfetto_sdk 链接必须是 PUBLIC

**问题**：如果将 `perfetto_sdk` 作为 PRIVATE 链接，链接器会报错找不到 `DataSourceHelper<TrackEventDataSource>::type()` 符号。

**解决**：
```cmake
# 正确
target_link_libraries(mc_perfetto PUBLIC perfetto_sdk)

# 错误
target_link_libraries(mc_perfetto PRIVATE perfetto_sdk)  # 会导致链接错误
```

**原因**：Perfetto SDK 使用模板实例化，符号必须在链接时对所有依赖者可见。

### 2. PERFETTO_TRACK_EVENT_STATIC_STORAGE 只能定义一次

**问题**：如果在多个 .cpp 文件中定义，会导致符号重复定义错误。

**解决**：只在 `TraceCategories.cpp` 中定义一次：
```cpp
// TraceCategories.cpp
PERFETTO_TRACK_EVENT_STATIC_STORAGE();
```

### 3. 分类必须预先定义

**问题**：使用未定义的分类会导致运行时事件不被记录。

**解决**：所有使用的分类必须在 `TraceCategories.hpp` 中通过 `PERFETTO_DEFINE_CATEGORIES` 定义。

### 4. 追踪文件可能为空

**问题**：如果没有调用 `stopTracing()` 或 `shutdown()`，追踪数据可能不会写入文件。

**解决**：确保程序退出前调用：
```cpp
PerfettoManager::instance().stopTracing();
PerfettoManager::instance().shutdown();
```

### 5. Windows 平台需要 /bigobj

**问题**：Perfetto SDK 生成大量符号，编译时会报错 "too many sections"。

**解决**：CMakeLists.txt 中已配置：
```cmake
if(WIN32)
    target_compile_options(perfetto_sdk PRIVATE "/bigobj")
    target_compile_options(mc_perfetto PRIVATE "/bigobj")
endif()
```

### 6. 禁用追踪时的存根实现

**问题**：当 `MC_ENABLE_TRACING=0` 时，`PerfettoManager` 的方法应该仍然可以调用但什么都不做。

**解决**：`PerfettoManager.hpp` 提供了存根实现：
```cpp
#if MC_ENABLE_TRACING
// 完整实现
#else
// 存根实现，所有方法为空操作
#endif
```

### 7. 调试级别日志默认不可见

**问题**：PerfettoManager 使用 `spdlog::debug` 输出调试信息，但项目默认日志级别可能不显示 debug。

**解决**：使用 info 级别或调整日志配置（已在使用 info 级别）。

## 测试用例

### 测试文件位置

```
tests/common/perfetto/
├── CMakeLists.txt          # 测试构建配置
├── PerfettoTest.cpp        # 集成测试（包含完整的 Manager 和 Events 测试）
├── PerfettoManagerTest.cpp # Manager 单元测试
└── TraceEventsTest.cpp     # 事件宏单元测试
```

### 测试内容

#### PerfettoManager 测试

| 测试 | 描述 |
|------|------|
| `SingletonPattern` | 验证单例模式正确性 |
| `InitializeAndShutdown` | 验证初始化和关闭流程 |
| `DoubleInitialize` | 验证二次初始化安全忽略 |
| `DoubleShutdown` | 验证二次关闭安全 |
| `StartStopTracing` | 验证启动/停止追踪流程 |
| `StartTracingWithoutInitialize` | 验证未初始化时启动追踪安全失败 |
| `DoubleStartTracing` | 验证二次启动追踪安全忽略 |
| `StopTracingWithoutStart` | 验证未启动时停止追踪安全 |
| `RuntimeEnableDisable` | 验证运行时启用/禁用 |
| `ConfigAccess` | 验证配置访问 |
| `Flush` | 验证刷新功能 |
| `FlushWithoutTracing` | 验证未追踪时刷新安全 |
| `StubImplementation` | 验证禁用时的存根实现 |

#### TraceEvents 测试

| 测试 | 描述 |
|------|------|
| `TraceEventCompiles` | 验证基本事件编译 |
| `TraceEventWithArguments` | 验证带参数的事件 |
| `TraceCounterCompiles` | 验证计数器编译 |
| `TraceCounterTypes` | 验证不同类型的计数器值 |
| `TraceEventBeginEnd` | 验证手动开始/结束事件 |
| `TraceInstant` | 验证瞬时事件 |
| `ScopedEvent` | 验证作用域事件 |
| `NestedScopedEvents` | 验证嵌套作用域事件 |
| `RenderingMacros` | 验证渲染子系统宏 |
| `GameTickMacros` | 验证游戏刻子系统宏 |
| `WorldMacros` | 验证世界子系统宏 |
| `NetworkMacros` | 验证网络子系统宏 |
| `ConditionalEvent` | 验证条件事件 |
| `CategoryEnabledCheck` | 验证分类检查 |
| `MultipleEvents` | 验证多事件记录 |
| `MultipleCounters` | 验证多计数器记录 |
| `SimulateFrameRendering` | 模拟帧渲染流程 |
| `SimulateChunkGeneration` | 模拟区块生成流程 |
| `SimulateServerTick` | 模拟服务端刻流程 |
| `DisabledMacrosAreNoOps` | 验证禁用时宏为空操作 |

### 运行测试

```powershell
# 构建并运行测试
cmake --build build --config Release
./build/bin/Release/mc_tests.exe --gtest_filter="Perfetto*"
```

### 测试注意事项

1. **集成到主测试套件**：由于 Perfetto SDK 的链接要求，测试集成到 `mc_tests` 中，而不是独立的可执行文件
2. **编译时开关**：测试会根据 `MC_ENABLE_TRACING` 自动选择不同的测试路径
3. **文件清理**：测试生成的 `test_trace.perfetto-trace` 文件会在测试后保留（可用于验证）

## 构建选项

| CMake 选项 | 默认值 | 描述 |
|-----------|--------|------|
| `MC_ENABLE_TRACING` | ON | 启用性能追踪 |

```powershell
# 启用追踪
cmake -B build -DMC_ENABLE_TRACING=ON ...

# 禁用追踪（零开销）
cmake -B build -DMC_ENABLE_TRACING=OFF ...
```
