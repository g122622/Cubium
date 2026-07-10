/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file TraceCategories.hpp
 * @brief Perfetto 追踪分类定义与分类枚举树
 *
 * 本文件提供两样东西：
 *
 * 1. 分类枚举树 `mc::trace::TraceEvents`：一组 `const char*` 常量，按子系统
 *    组织成嵌套结构体。这是调用方唯一应该使用的分类来源，例如：
 *    @code
 *    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "MinecraftServer::initializeRegistries");
 *    @endcode
 *    每个枚举节点的值都是一个字符串字面量（如 "server.initialization"），
 *    Perfetto SDK 据此在编译期将其匹配到已注册分类，走静态路径、零运行时开销。
 *
 * 2. PERFETTO_DEFINE_CATEGORIES：把上述枚举树中出现的分类注册到 Perfetto SDK。
 *    必须在使用前注册；使用未注册分类会导致编译错误。
 *
 * 设计要点：
 * - 分类以 `const char*` 而非 `perfetto::Category` 对象暴露，因为 Perfetto 的
 *   TRACE_EVENT 宏只接受 `const char*`（或 DynamicCategory）。
 * - 枚举树是对旧分类的一次精简：删除了从未使用的分类，合并了过度细分的
 *   storage.task.* 子类。新增分类时，请同时在此树和
 *   PERFETTO_DEFINE_CATEGORIES 中登记。
 */

#pragma once

#include "PerfettoConfig.hpp"

// ============================================================================
// 分类枚举树
// ============================================================================
//
// TraceEvents 是一个 inline constexpr 匿名结构体实例，按子系统组织分类。
// 每个叶子节点都是 `const char*`，其值必须与下方 PERFETTO_DEFINE_CATEGORIES
// 中注册的某个分类字符串完全一致，否则编译失败。
//
// 使用方式：MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick");

namespace mc {
namespace trace {

inline constexpr struct TraceCategoriesTree {
    // === 服务端 ===
    struct ServerGroup {
        const char* const Initialization = "server.initialization"; // 服务端初始化
        const char* const Tick = "server.tick";                     // 服务端游戏刻
        const char* const Network = "server.network";               // 服务端网络处理
        const char* const World = "server.world";                   // 服务端世界更新
        const char* const Chunk = "server.chunk";                   // 服务端区块处理
        const char* const Lighting = "server.lighting";             // 服务端光照
        const char* const Entity = "server.entity";                 // 服务端实体更新
        const char* const Player = "server.player";                 // 服务端玩家管理
        const char* const Mining = "server.mining";                 // 服务端挖掘处理
    };

    // === 客户端 ===
    struct ClientGroup {
        const char* const Initialization = "client.initialization"; // 客户端初始化
        const char* const Network = "client.network";               // 客户端网络
        const char* const Resource = "client.resource";             // 客户端资源包
        const char* const Sound = "client.sound";                   // 客户端声音播放
        const char* const Entity = "client.entity";                 // 客户端实体更新
        const char* const Lighting = "client.lighting";             // 客户端光照
        const char* const Mining = "client.mining";                 // 客户端挖掘输入
    };

    // === 渲染 ===
    struct RenderingGroup {
        const char* const Frame = "rendering.frame";                   // 帧渲染生命周期
        const char* const Initialization = "rendering.initialization"; // 渲染系统初始化
        const char* const ChunkMesh = "rendering.chunk_mesh";          // 区块网格生成与上传
        const char* const Entity = "rendering.entity";                 // 实体渲染
        const char* const Sky = "rendering.sky";                       // 天空渲染
        const char* const Weather = "rendering.weather";               // 天气渲染（雨、雪）
        const char* const Cloud = "rendering.cloud";                   // 云层渲染
    };

    // === 世界生成 ===
    struct WorldGroup {
        const char* const ChunkGen = "world.chunk_gen"; // 区块生成各阶段
    };

    // === 游戏 ===
    struct GameGroup {
        const char* const Tick = "game.tick";     // 游戏刻处理
        const char* const Entity = "game.entity"; // 实体更新
    };

    // === 网络 ===
    struct NetworkGroup {
        const char* const Packet = "network.packet"; // 网络包处理
    };

    // === I/O ===
    struct IOGroup {
        const char* const Resource = "io.resource"; // 资源加载
    };

    struct FluidGroup {
        const char* const Tick = "fluid.tick"; // 流体更新
    };

    struct StorageGroup {
        const char* const Db = "storage.db";           // 数据库操作
        const char* const Section = "storage.section"; // Section 管理
        const char* const Task = "storage.task";       // 存储任务（加载/保存/快照/刷盘）
    };

    struct WorkerPoolGroup {
        const char* const Generic = "worker_pool"; // 通用任务池操作
    };

    struct MemoryGroup {
        const char* const Usage = "memory"; // 内存使用统计
    };

    // === 基准测试 ===
    struct BenchmarkGroup {
        const char* const Run = "benchmark"; // 基准测试运行（套件/用例/迭代）
    };

    ServerGroup Server;
    ClientGroup Client;
    RenderingGroup Rendering;
    WorldGroup World;
    GameGroup Game;
    NetworkGroup Network;
    IOGroup IO;
    FluidGroup Fluid;
    StorageGroup Storage;
    WorkerPoolGroup WorkerPool;
    MemoryGroup Memory;
    BenchmarkGroup Benchmark;
} TraceEvents{};

} // namespace trace
} // namespace mc

#if MC_ENABLE_TRACING

// ============================================================================
// Perfetto SDK 分类注册
// ============================================================================

// 禁用 Perfetto SDK 的警告
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <perfetto.h>

// 注册枚举树中出现的所有分类。新增分类时：
// 1. 在上方 TraceCategoriesTree 中加叶子节点；
// 2. 在此处加对应的 perfetto::Category(...).SetDescription(...)。
PERFETTO_DEFINE_CATEGORIES(
    // === 服务端 ===
    perfetto::Category("server.initialization").SetDescription("服务端初始化"),
    perfetto::Category("server.tick").SetDescription("服务端游戏刻处理"),
    perfetto::Category("server.network").SetDescription("服务端网络处理"),
    perfetto::Category("server.world").SetDescription("服务端世界更新"),
    perfetto::Category("server.chunk").SetDescription("服务端区块处理"),
    perfetto::Category("server.lighting").SetDescription("服务端光照处理"),
    perfetto::Category("server.entity").SetDescription("服务端实体更新"),
    perfetto::Category("server.player").SetDescription("服务端玩家管理"),
    perfetto::Category("server.mining").SetDescription("服务端挖掘处理"),

    // === 客户端 ===
    perfetto::Category("client.initialization").SetDescription("客户端初始化"),
    perfetto::Category("client.network").SetDescription("客户端网络"),
    perfetto::Category("client.resource").SetDescription("客户端资源包"),
    perfetto::Category("client.sound").SetDescription("客户端声音播放"),
    perfetto::Category("client.entity").SetDescription("客户端实体更新"),
    perfetto::Category("client.lighting").SetDescription("客户端光照处理"),
    perfetto::Category("client.mining").SetDescription("客户端挖掘输入"),

    // === 渲染 ===
    perfetto::Category("rendering.frame").SetDescription("帧渲染生命周期事件"),
    perfetto::Category("rendering.initialization").SetDescription("渲染系统初始化"),
    perfetto::Category("rendering.chunk_mesh").SetDescription("区块网格生成和上传"),
    perfetto::Category("rendering.entity").SetDescription("实体渲染"),
    perfetto::Category("rendering.sky").SetDescription("天空渲染"),
    perfetto::Category("rendering.weather").SetDescription("天气渲染（雨、雪）"),
    perfetto::Category("rendering.cloud").SetDescription("云层渲染"),

    // === 世界生成 ===
    perfetto::Category("world.chunk_gen").SetDescription("区块生成各阶段"),

    // === 游戏 ===
    perfetto::Category("game.tick").SetDescription("游戏刻处理"),
    perfetto::Category("game.entity").SetDescription("实体更新"),

    // === 网络 ===
    perfetto::Category("network.packet").SetDescription("网络包处理"),

    // === I/O ===
    perfetto::Category("io.resource").SetDescription("资源加载"),

    // === 流体 ===
    perfetto::Category("fluid.tick").SetDescription("流体更新处理"),

    // === 存储 ===
    perfetto::Category("storage.db").SetDescription("数据库操作"),
    perfetto::Category("storage.section").SetDescription("Section管理操作"),
    perfetto::Category("storage.task").SetDescription("存储任务执行（加载/保存/快照/刷盘）"),

    // === 通用任务池 ===
    perfetto::Category("worker_pool").SetDescription("通用任务池操作"),

    // === 内存 ===
    perfetto::Category("memory").SetDescription("内存使用统计"),

    // === 基准测试 ===
    perfetto::Category("benchmark").SetDescription("基准测试运行"));

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace mc {
namespace perfetto {

/**
 * @brief 初始化追踪分类
 *
 * 必须在程序启动时调用，通常由 PerfettoManager::initialize() 自动调用。
 */
void initTraceCategories();

} // namespace perfetto
} // namespace mc

#else // MC_ENABLE_TRACING == 0

// 禁用追踪时的空声明

namespace mc {
namespace perfetto {

inline void initTraceCategories() {}

} // namespace perfetto
} // namespace mc

#endif // MC_ENABLE_TRACING
