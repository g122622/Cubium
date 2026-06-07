# Perfetto 性能追踪模块

Perfetto 模块提供了一套完整的性能追踪基础设施，基于 Google Perfetto SDK 实现，支持编译时开关、零开销禁用、细粒度分类控制。

## 目录结构

```
src/common/perfetto/
├── CMakeLists.txt        # 构建配置，定义 perfetto_sdk 和 mc_perfetto 库
├── PerfettoConfig.hpp    # 编译时配置开关（总开关、子系统开关、缓冲区配置）
├── PerfettoManager.hpp   # 单例追踪管理器，负责生命周期管理
├── PerfettoManager.cpp   # 管理器实现（Pimpl 模式，追踪会话管理）
├── TraceCategories.hpp   # 追踪分类定义（rendering.*、game.*、world.* 等）
├── TraceCategories.cpp   # 静态存储定义（PERFETTO_TRACK_EVENT_STATIC_STORAGE）
└── TraceEvents.hpp       # 追踪事件便捷宏（MC_TRACE_EVENT 等）
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

- **PerfettoConfig.hpp**：定义编译时开关，控制哪些子系统启用追踪
- **TraceCategories.hpp**：定义所有追踪分类，必须在使用前注册
- **PerfettoManager**：单例模式，管理追踪系统的初始化、启动、停止、文件输出
- **TraceEvents.hpp**：封装 Perfetto 原生宏，提供统一的 MC_TRACE_* 接口

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
| **Perfetto SDK** | 第三方追踪库（`third_party/perfetto/`） |
| **spdlog** | 日志输出（追踪系统状态信息） |
| **ws2_32**（Windows） | WinSock2 库，Perfetto 网络功能需要 |

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

### 6. MC_TRACE_EVENT 是 RAII 作用域事件

`MC_TRACE_EVENT` 在作用域结束时自动结束事件。**同一作用域内不能有多个 MC_TRACE_EVENT**，否则会导致事件嵌套错误。每个 `MC_TRACE_EVENT` 必须放在独立的大括号作用域内：

```cpp
// 错误：同一作用域内有多个 trace event
void processFrame() {
    MC_TRACE_EVENT("rendering.frame", "HandleEvents");  // 不会正确结束
    handleEvents();
    MC_TRACE_EVENT("rendering.frame", "Update");  // 嵌套错误
    update(deltaTime);
}

// 正确：每个 trace event 放在独立的作用域内
void processFrame() {
    {
        MC_TRACE_EVENT("rendering.frame", "HandleEvents");
        handleEvents();
    }
    {
        MC_TRACE_EVENT("rendering.frame", "Update");
        update(deltaTime);
    }
    {
        MC_TRACE_EVENT("rendering.frame", "Render");
        render();
    }
}
```

### 7. 不要在 Lambda 中使用 MC_TRACE_EVENT（MSVC Bug）

在 MSVC 中，lambda 表达式内使用 `MC_TRACE_EVENT` 宏可能导致编译器卡死或内部错误。这是 Perfetto SDK 与 MSVC 的已知兼容性问题。**改用 `spdlog::info` 等日志手段代替**。

### 8. 禁用追踪时的存根实现

当 `MC_ENABLE_TRACING=0` 时，`PerfettoManager` 的所有方法仍可调用但为空操作（零开销）。无需修改调用代码。

### 9. 追踪分类注册要求

你必须保证 `MC_TRACE_EVENT`/`MC_TRACE_COUNTER` 等的第一个参数（分类名）在 `TraceCategories.hpp` 中已经被注册，否则会导致编译错误。
