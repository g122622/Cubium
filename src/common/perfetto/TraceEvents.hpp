/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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
 * @file TraceEvents.hpp
 * @brief Perfetto 追踪事件便捷宏
 *
 * 此文件提供统一的追踪事件宏。当追踪禁用时所有宏展开为空操作，无性能开销。
 *
 * 分类通过 `mc::trace::TraceEvents` 枚举树传入（见 TraceCategories.hpp），
 * 不再使用字符串字面量。例如：
 * @code
 * // 作用域事件（推荐）
 * void myFunction() {
 *     MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "MyFunction");
 * }
 *
 * // 带键值对参数
 * void processChunk(int x, int z) {
 *     MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "ProcessChunk",
 *                           "x", x, "z", z);
 * }
 *
 * // 计数器
 * void updateFPS(double fps) {
 *     MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "FPS", static_cast<int64_t>(fps));
 * }
 *
 * // 手动开始/结束事件（跨函数场景）
 * void startAsync() {
 *     MC_TRACE_EVENT_BEGIN(TraceEvents.Network.Packet, "AsyncOp", "id", 123);
 * }
 * void endAsync() {
 *     MC_TRACE_EVENT_END(TraceEvents.Network.Packet);
 * }
 *
 * // 瞬时事件（零持续时间）
 * void onEvent() {
 *     MC_TRACE_INSTANT_EVENT(TraceEvents.Game.Tick, "SomethingHappened");
 * }
 * @endcode
 *
 * 注意事项：
 * - 所有宏在 MC_ENABLE_TRACING=0 时展开为空操作，无性能开销
 * - category 参数必须来自 TraceEvents 枚举树，其字符串值必须在
 *   TraceCategories.hpp 的 PERFETTO_DEFINE_CATEGORIES 中注册，否则编译错误
 * - 避免在热路径中使用复杂的参数计算
 */

#pragma once

#include "PerfettoConfig.hpp"

#if MC_ENABLE_TRACING

// 禁用 Perfetto SDK 的警告
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include "TraceCategories.hpp"
#include <perfetto.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// PerfettoManager.hpp 必须在 <perfetto.h> 之后 include：
// 否则 mc::perfetto 命名空间先于 ::perfetto 声明，会导致 SDK 头文件内
// 对 `perfetto::` 的名字查找在 mc::perfetto 与 ::perfetto 之间歧义。
#include "PerfettoManager.hpp"

// ============================================================================
// 追踪事件宏
// ============================================================================
//
// 所有宏的第一个参数 category 取自 mc::trace::TraceEvents 枚举树
// （类型为 const char*，值是字符串字面量）。它会被 Perfetto SDK 在编译期
// 匹配到已注册分类，走静态路径，零运行时查找开销。
//
// 引用枚举树时需用全限定名 ::mc::trace::TraceEvents（或当前作用域已可见
// mc::trace）。为简化书写，下方宏不限定命名空间，调用方按需自行限定。

/**
 * @brief 记录作用域事件
 *
 * 事件在作用域开始时自动开始，作用域结束时自动结束。
 * 这是最常用的追踪宏，推荐在大多数场景使用。
 *
 * @param category 分类（TraceEvents 枚举树的叶子节点，如 TraceEvents.Server.Tick）
 * @param name 事件名称（字符串字面量）
 * @param ... 可选的键值对参数（"key", value, ...）
 *
 * 示例：
 * @code
 * MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "RenderFrame");
 * MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen, "GenerateBiomes", "x", chunkX, "z", chunkZ);
 * @endcode
 */
#define MC_TRACE_SCOPED_EVENT(category, name, ...) TRACE_EVENT(category, name __VA_OPT__(, ) __VA_ARGS__)

/**
 * @brief 记录瞬时事件
 *
 * 瞬时事件没有持续时间，用于标记某个时刻发生的事情。
 *
 * @param category 分类（TraceEvents 枚举树叶子节点）
 * @param name 事件名称
 * @param ... 可选的键值对参数
 */
#define MC_TRACE_INSTANT_EVENT(category, name, ...) TRACE_EVENT_INSTANT(category, name __VA_OPT__(, ) __VA_ARGS__)

/**
 * @brief 记录计数器值
 *
 * 用于记录随时间变化的数值，如 FPS、内存使用等。
 *
 * @param category 分类（TraceEvents 枚举树叶子节点）
 * @param name 计数器名称
 * @param value 计数器值（必须是 int64_t 类型）
 *
 * 示例：
 * @code
 * MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "FPS", 60);
 * MC_TRACE_COUNTER(TraceEvents.Memory.Usage, "AllocatedMB", static_cast<int64_t>(mb));
 * @endcode
 */
#define MC_TRACE_COUNTER(category, name, value) TRACE_COUNTER(category, name, value)

/**
 * @brief 手动开始一个事件
 *
 * 用于跨多个函数的事件追踪。必须与 MC_TRACE_EVENT_END 配对使用。
 *
 * @param category 分类（TraceEvents 枚举树叶子节点）
 * @param name 事件名称
 * @param ... 可选的键值对参数
 *
 * 示例：
 * @code
 * void startRequest(int id) {
 *     MC_TRACE_EVENT_BEGIN(TraceEvents.Network.Packet, "HttpRequest", "id", id);
 * }
 * void endRequest() {
 *     MC_TRACE_EVENT_END(TraceEvents.Network.Packet);
 * }
 * @endcode
 */
#define MC_TRACE_EVENT_BEGIN(category, name, ...) TRACE_EVENT_BEGIN(category, name __VA_OPT__(, ) __VA_ARGS__)

/**
 * @brief 手动结束一个事件
 *
 * 结束由 MC_TRACE_EVENT_BEGIN 开始的事件。
 *
 * @param category 分类（必须与 BEGIN 匹配）
 */
#define MC_TRACE_EVENT_END(category) TRACE_EVENT_END(category)

#else // MC_ENABLE_TRACING == 0

// ============================================================================
// 禁用追踪时，所有宏展开为空操作
// ============================================================================

#define MC_TRACE_SCOPED_EVENT(category, name, ...) ((void)0)
#define MC_TRACE_INSTANT_EVENT(category, name, ...) ((void)0)
#define MC_TRACE_COUNTER(category, name, value) ((void)0)
#define MC_TRACE_EVENT_BEGIN(category, name, ...) ((void)0)
#define MC_TRACE_EVENT_END(category) ((void)0)

#endif // MC_ENABLE_TRACING

// ============================================================================
// 线程命名宏
// ============================================================================

#if MC_ENABLE_TRACING

/**
 * @brief 设置当前线程名称
 *
 * 在 Perfetto UI 中显示有意义的线程名称，便于分析。内部转调
 * PerfettoManager::setThreadName，由 Manager 统一注入 sibling_order_rank
 * （基于线程名查表），保证排序一致。应在线程启动后尽早调用。
 *
 * @param name 线程名称（字符串字面量）
 *
 * 示例：
 * @code
 * void workerThread() {
 *     MC_TRACE_SET_THREAD_NAME("ChunkWorker");
 * }
 * @endcode
 */
#define MC_TRACE_SET_THREAD_NAME(name) ::mc::perfetto::PerfettoManager::instance().setThreadName(name)

#else // MC_ENABLE_TRACING == 0

#define MC_TRACE_SET_THREAD_NAME(name) ((void)0)

#endif // MC_ENABLE_TRACING
