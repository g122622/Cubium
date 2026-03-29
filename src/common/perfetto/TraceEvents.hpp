/**
 * @file TraceEvents.hpp
 * @brief Perfetto 追踪事件便捷宏
 *
 * 此文件提供了统一的追踪事件宏，当追踪禁用时展开为空操作。
 *
 * 使用方法：
 * @code
 * // 作用域事件（推荐）
 * void myFunction() {
 *     MC_TRACE_EVENT("rendering.frame", "MyFunction");
 *     // 函数结束时自动结束事件
 * }
 *
 * // 带参数的事件
 * void processChunk(int x, int z) {
 *     MC_TRACE_EVENT("world.chunk_gen", "ProcessChunk",
 *                    "x", x, "z", z);
 * }
 *
 * // 计数器
 * void updateFPS(double fps) {
 *     MC_TRACE_COUNTER("rendering.frame", "FPS", static_cast<int64_t>(fps));
 * }
 *
 * // 手动开始/结束事件（跨函数场景）
 * void startAsync() {
 *     MC_TRACE_EVENT_BEGIN("network", "AsyncOp", "id", 123);
 * }
 * void endAsync() {
 *     MC_TRACE_EVENT_END("network");
 * }
 *
 * // 瞬时事件（零持续时间）
 * void onEvent() {
 *     MC_TRACE_INSTANT("game.tick", "SomethingHappened");
 * }
 * @endcode
 *
 * 注意事项：
 * - 所有宏在 MC_ENABLE_TRACING=0 时展开为空操作，无性能开销
 * - category 参数必须是在 TraceCategories.hpp 中定义的分类
 * - 避免在热路径中使用复杂的参数计算
 */

#pragma once

#include "PerfettoConfig.hpp"

#if MC_ENABLE_TRACING

// 禁用 Perfetto SDK 的警告
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <perfetto.h>
#include "TraceCategories.hpp"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// ============================================================================
// 追踪事件宏
// ============================================================================

/**
 * @brief 记录作用域事件
 *
 * 事件在作用域开始时自动开始，作用域结束时自动结束。
 * 这是最常用的追踪宏，推荐在大多数场景使用。
 *
 * @param category 分类名称（字符串字面量）
 * @param name 事件名称（字符串字面量）
 * @param ... 可选的键值对参数（"key", value, ...）
 *
 * 示例：
 * @code
 * MC_TRACE_EVENT("rendering.frame", "RenderFrame");
 * MC_TRACE_EVENT("world.chunk_gen", "GenerateBiomes", "x", chunkX, "z", chunkZ);
 * @endcode
 */
#define MC_TRACE_EVENT(category, name, ...) \
    TRACE_EVENT(category, name, ##__VA_ARGS__)

/**
 * @brief 记录计数器值
 *
 * 用于记录随时间变化的数值，如 FPS、内存使用等。
 *
 * @param category 分类名称
 * @param name 计数器名称
 * @param value 计数器值（必须是 int64_t 类型）
 *
 * 示例：
 * @code
 * MC_TRACE_COUNTER("rendering.frame", "FPS", 60);
 * MC_TRACE_COUNTER("memory", "AllocatedMB", static_cast<int64_t>(mb));
 * @endcode
 */
#define MC_TRACE_COUNTER(category, name, value) \
    TRACE_COUNTER(category, name, value)

/**
 * @brief 手动开始一个事件
 *
 * 用于跨多个函数的事件追踪。必须与 MC_TRACE_EVENT_END 配对使用。
 *
 * @param category 分类名称
 * @param name 事件名称
 * @param ... 可选的键值对参数
 *
 * 示例：
 * @code
 * void startRequest(int id) {
 *     MC_TRACE_EVENT_BEGIN("network", "HttpRequest", "id", id);
 * }
 * void endRequest() {
 *     MC_TRACE_EVENT_END("network");
 * }
 * @endcode
 */
#define MC_TRACE_EVENT_BEGIN(category, name, ...) \
    TRACE_EVENT_BEGIN(category, name, ##__VA_ARGS__)

/**
 * @brief 手动结束一个事件
 *
 * 结束由 MC_TRACE_EVENT_BEGIN 开始的事件。
 *
 * @param category 分类名称（必须与 BEGIN 匹配）
 */
#define MC_TRACE_EVENT_END(category) \
    TRACE_EVENT_END(category)

/**
 * @brief 记录瞬时事件
 *
 * 瞬时事件没有持续时间，用于标记某个时刻发生的事情。
 *
 * @param category 分类名称
 * @param name 事件名称
 */
#define MC_TRACE_INSTANT(category, name, ...) \
    TRACE_EVENT_INSTANT(category, name, ##__VA_ARGS__)

/**
 * @brief 检查分类是否启用
 *
 * 用于在执行昂贵追踪操作前检查分类是否启用。
 *
 * @param category 分类名称
 * @return 如果分类启用返回 true
 *
 * 示例：
 * @code
 * if (MC_TRACE_CATEGORY_ENABLED("rendering.frame")) {
 *     // 执行昂贵的追踪数据收集
 * }
 * @endcode
 */
#define MC_TRACE_CATEGORY_ENABLED(category) \
    TRACE_EVENT_CATEGORY_ENABLED(category)

// ============================================================================
// 条件追踪宏
// ============================================================================

/**
 * @brief 条件作用域事件
 *
 * 仅当条件为真时记录事件。
 *
 * @param condition 条件表达式
 * @param category 分类名称
 * @param name 事件名称
 */
#define MC_TRACE_EVENT_IF(condition, category, name, ...) \
    do { \
        if (condition) { \
            MC_TRACE_EVENT(category, name, ##__VA_ARGS__); \
        } \
    } while (0)

#else // MC_ENABLE_TRACING == 0

// ============================================================================
// 禁用追踪时，所有宏展开为空操作
// ============================================================================

#define MC_TRACE_EVENT(category, name, ...)              ((void)0)
#define MC_TRACE_COUNTER(category, name, value)          ((void)0)
#define MC_TRACE_EVENT_BEGIN(category, name, ...)        ((void)0)
#define MC_TRACE_EVENT_END(category)                     ((void)0)
#define MC_TRACE_INSTANT(category, name, ...)            ((void)0)
#define MC_TRACE_CATEGORY_ENABLED(category)              (false)
#define MC_TRACE_EVENT_IF(condition, category, name, ...) ((void)0)

#endif // MC_ENABLE_TRACING

// ============================================================================
// 子系统特定的追踪宏
// ============================================================================

#if MC_TRACE_RENDERING
#define MC_TRACE_RENDERING_EVENT(name, ...) MC_TRACE_EVENT("rendering.frame", name, ##__VA_ARGS__)
#define MC_TRACE_RENDERING_COUNTER(name, value) MC_TRACE_COUNTER("rendering.frame", name, value)
#define MC_TRACE_VULKAN_EVENT(name, ...) MC_TRACE_EVENT("rendering.vulkan", name, ##__VA_ARGS__)
#define MC_TRACE_CHUNK_MESH_EVENT(name, ...) MC_TRACE_EVENT("rendering.chunk_mesh", name, ##__VA_ARGS__)
// 细粒度渲染追踪
#define MC_TRACE_BEGIN_FRAME(name, ...) MC_TRACE_EVENT("rendering.begin_frame", name, ##__VA_ARGS__)
#define MC_TRACE_UNIFORM_UPDATE(name, ...) MC_TRACE_EVENT("rendering.uniform_update", name, ##__VA_ARGS__)
#define MC_TRACE_SKY(name, ...) MC_TRACE_EVENT("rendering.sky", name, ##__VA_ARGS__)
#define MC_TRACE_CHUNK_DRAW(name, ...) MC_TRACE_EVENT("rendering.chunk_draw", name, ##__VA_ARGS__)
#define MC_TRACE_GUI(name, ...) MC_TRACE_EVENT("rendering.gui", name, ##__VA_ARGS__)
#define MC_TRACE_END_FRAME(name, ...) MC_TRACE_EVENT("rendering.end_frame", name, ##__VA_ARGS__)
#define MC_TRACE_VIEWPORT(name, ...) MC_TRACE_EVENT("rendering.viewport", name, ##__VA_ARGS__)
#define MC_TRACE_DESCRIPTOR_BIND(name, ...) MC_TRACE_EVENT("rendering.descriptor_bind", name, ##__VA_ARGS__)
#define MC_TRACE_PUSH_CONSTANTS(name, ...) MC_TRACE_EVENT("rendering.push_constants", name, ##__VA_ARGS__)
#define MC_TRACE_CMD_BUFFER(name, ...) MC_TRACE_EVENT("rendering.command_buffer", name, ##__VA_ARGS__)
#else
#define MC_TRACE_RENDERING_EVENT(name, ...) ((void)0)
#define MC_TRACE_RENDERING_COUNTER(name, value) ((void)0)
#define MC_TRACE_VULKAN_EVENT(name, ...) ((void)0)
#define MC_TRACE_CHUNK_MESH_EVENT(name, ...) ((void)0)
#define MC_TRACE_BEGIN_FRAME(name, ...) ((void)0)
#define MC_TRACE_UNIFORM_UPDATE(name, ...) ((void)0)
#define MC_TRACE_SKY(name, ...) ((void)0)
#define MC_TRACE_CHUNK_DRAW(name, ...) ((void)0)
#define MC_TRACE_GUI(name, ...) ((void)0)
#define MC_TRACE_END_FRAME(name, ...) ((void)0)
#define MC_TRACE_VIEWPORT(name, ...) ((void)0)
#define MC_TRACE_DESCRIPTOR_BIND(name, ...) ((void)0)
#define MC_TRACE_PUSH_CONSTANTS(name, ...) ((void)0)
#define MC_TRACE_CMD_BUFFER(name, ...) ((void)0)
#endif

#if MC_TRACE_GAME_TICK
#define MC_TRACE_TICK_EVENT(name, ...) MC_TRACE_EVENT("game.tick", name, ##__VA_ARGS__)
#define MC_TRACE_TICK_COUNTER(name, value) MC_TRACE_COUNTER("game.tick", name, value)
#define MC_TRACE_ENTITY_EVENT(name, ...) MC_TRACE_EVENT("game.entity", name, ##__VA_ARGS__)
#define MC_TRACE_AI_EVENT(name, ...) MC_TRACE_EVENT("game.ai", name, ##__VA_ARGS__)
#else
#define MC_TRACE_TICK_EVENT(name, ...) ((void)0)
#define MC_TRACE_TICK_COUNTER(name, value) ((void)0)
#define MC_TRACE_ENTITY_EVENT(name, ...) ((void)0)
#define MC_TRACE_AI_EVENT(name, ...) ((void)0)
#endif

#if MC_TRACE_CHUNK_GENERATION
#define MC_TRACE_CHUNK_GEN_EVENT(name, ...) MC_TRACE_EVENT("world.chunk_gen", name, ##__VA_ARGS__)
#else
#define MC_TRACE_CHUNK_GEN_EVENT(name, ...) ((void)0)
#endif

#if MC_TRACE_CHUNK_LOAD
#define MC_TRACE_CHUNK_LOAD_EVENT(name, ...) MC_TRACE_EVENT("world.chunk_load", name, ##__VA_ARGS__)
#else
#define MC_TRACE_CHUNK_LOAD_EVENT(name, ...) ((void)0)
#endif

#if MC_TRACE_NETWORK
#define MC_TRACE_NETWORK_EVENT(name, ...) MC_TRACE_EVENT("network.packet", name, ##__VA_ARGS__)
#else
#define MC_TRACE_NETWORK_EVENT(name, ...) ((void)0)
#endif

// ============================================================================
// 线程和进程命名宏
// ============================================================================

#if MC_ENABLE_TRACING

/**
 * @brief 设置当前线程名称
 *
 * 在 Perfetto UI 中显示有意义的线程名称，便于分析。
 * 应在线程启动后尽早调用。
 *
 * @param name 线程名称（字符串字面量）
 *
 * 示例：
 * @code
 * void workerThread() {
 *     MC_TRACE_SET_THREAD_NAME("ChunkWorker");
 *     // ... 线程工作 ...
 * }
 * @endcode
 */
#define MC_TRACE_SET_THREAD_NAME(name) \
    do { \
        auto _desc = ::perfetto::ThreadTrack::Current().Serialize(); \
        _desc.mutable_thread()->set_thread_name(name); \
        ::perfetto::TrackEvent::SetTrackDescriptor( \
            ::perfetto::ThreadTrack::Current(), _desc); \
    } while (0)

/**
 * @brief 设置当前进程名称
 *
 * 在 Perfetto UI 中显示有意义的进程名称，便于分析。
 * 应在进程启动后尽早调用。
 *
 * @param name 进程名称（字符串字面量）
 *
 * 示例：
 * @code
 * int main() {
 *     MC_TRACE_SET_PROCESS_NAME("MinecraftServer");
 *     // ... 主程序 ...
 * }
 * @endcode
 */
#define MC_TRACE_SET_PROCESS_NAME(name) \
    do { \
        auto _desc = ::perfetto::ProcessTrack::Current().Serialize(); \
        _desc.mutable_process()->set_process_name(name); \
        ::perfetto::TrackEvent::SetTrackDescriptor( \
            ::perfetto::ProcessTrack::Current(), _desc); \
    } while (0)

/**
 * @brief 为指定线程设置名称
 *
 * 用于在创建线程后从外部设置线程名称。
 *
 * @param tid 线程ID（base::PlatformThreadId 类型）
 * @param name 线程名称（字符串字面量）
 */
#define MC_TRACE_SET_THREAD_NAME_FOR(tid, name) \
    do { \
        auto _track = ::perfetto::ThreadTrack::ForThread(tid); \
        auto _desc = _track.Serialize(); \
        _desc.mutable_thread()->set_thread_name(name); \
        ::perfetto::TrackEvent::SetTrackDescriptor(_track, _desc); \
    } while (0)

#else // MC_ENABLE_TRACING == 0

#define MC_TRACE_SET_THREAD_NAME(name)            ((void)0)
#define MC_TRACE_SET_PROCESS_NAME(name)           ((void)0)
#define MC_TRACE_SET_THREAD_NAME_FOR(tid, name)   ((void)0)

#endif // MC_ENABLE_TRACING

// ============================================================================
// 服务端细粒度追踪宏
// ============================================================================

#if MC_ENABLE_TRACING
#define MC_TRACE_SERVER_TICK_EVENT(name, ...)     MC_TRACE_EVENT("server.tick", name, ##__VA_ARGS__)
#define MC_TRACE_SERVER_NETWORK_EVENT(name, ...)  MC_TRACE_EVENT("server.network", name, ##__VA_ARGS__)
#define MC_TRACE_SERVER_PLAYER_EVENT(name, ...)   MC_TRACE_EVENT("server.player", name, ##__VA_ARGS__)
#define MC_TRACE_SERVER_WORLD_EVENT(name, ...)    MC_TRACE_EVENT("server.world", name, ##__VA_ARGS__)
#define MC_TRACE_SERVER_CHUNK_EVENT(name, ...)    MC_TRACE_EVENT("server.chunk", name, ##__VA_ARGS__)
#define MC_TRACE_SERVER_ENTITY_EVENT(name, ...)   MC_TRACE_EVENT("server.entity", name, ##__VA_ARGS__)
#define MC_TRACE_SERVER_TICK_COUNTER(name, value) MC_TRACE_COUNTER("server.tick", name, value)
#else
#define MC_TRACE_SERVER_TICK_EVENT(name, ...)     ((void)0)
#define MC_TRACE_SERVER_NETWORK_EVENT(name, ...)  ((void)0)
#define MC_TRACE_SERVER_PLAYER_EVENT(name, ...)   ((void)0)
#define MC_TRACE_SERVER_WORLD_EVENT(name, ...)    ((void)0)
#define MC_TRACE_SERVER_CHUNK_EVENT(name, ...)    ((void)0)
#define MC_TRACE_SERVER_ENTITY_EVENT(name, ...)   ((void)0)
#define MC_TRACE_SERVER_TICK_COUNTER(name, value) ((void)0)
#endif

// ============================================================================
// 声音系统追踪宏
// ============================================================================

#if MC_ENABLE_TRACING
#define MC_TRACE_SERVER_SOUND_EVENT(name, ...)    MC_TRACE_EVENT("server.sound", name, ##__VA_ARGS__)
#define MC_TRACE_CLIENT_SOUND_EVENT(name, ...)    MC_TRACE_EVENT("client.sound", name, ##__VA_ARGS__)
#else
#define MC_TRACE_SERVER_SOUND_EVENT(name, ...)    ((void)0)
#define MC_TRACE_CLIENT_SOUND_EVENT(name, ...)    ((void)0)
#endif
