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
 * @brief 性能追踪事件便捷宏（Perfetto + Tracy 双轨）
 *
 * 此文件提供统一的追踪事件宏。两个后端由编译时开关控制：
 *   - MC_ENABLE_TRACING (Perfetto)：TRACE_EVENT / TRACE_COUNTER / TRACE_EVENT_BEGIN/END
 *   - MC_ENABLE_TRACY   (Tracy)  ：ZoneScopedN / TracyPlot / TracyMessageL
 * 同时启用时宏会同时向两套后端发事件（双轨录制）；两者皆关时所有宏展开为空操作。
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
 * - 两后端皆关时（MC_PROFILER_ENABLED=0）所有宏空展开，无性能开销
 * - category 参数必须来自 TraceEvents 枚举树，其字符串值必须在
 *   TraceCategories.hpp 的 PERFETTO_DEFINE_CATEGORIES 中注册，否则编译错误
 *   （此约束仅在 Perfetto 后端启用时生效）
 * - Tracy 侧 BEGIN/END 降级为 message 边界标记（tracy 无独立 begin/end 概念），
 *   计数器用 TracyPlot（double，>2^53 大值会丢精度）
 * - 双轨宏无变量名冲突：perfetto RAII 变量是 scoped_event<N>/ScopedEvent<N>
 *   （__LINE__ 后缀）；tracy 侧的 MC_TRACY_SCOPED_ZONE 也用 __LINE__ 后缀变量名
 *   （绕开 ZoneScopedN 的固定名 ___tracy_scoped_zone），故同一作用域多个
 *   MC_TRACE_SCOPED_EVENT 可安全共存。
 */

#pragma once

#include "ProfilerConfig.hpp"
#include "TraceCategories.hpp"

// 注：TraceCategories.hpp 无条件定义 mc::trace::TraceEvents 枚举树（仅依赖
// ProfilerConfig.hpp，不依赖 perfetto/tracy）。这里无条件 include，确保即便
// 两个后端皆关（noprof 构建），业务代码中的 `using namespace mc::trace;` 仍能
// 解析——否则空展开的 MC_TRACE_* 宏虽无开销，但残留的 using 声明会导致编译失败。

// ============================================================================
// 头文件包含
// ============================================================================
//
// Perfetto 后端启用时：需 TraceCategories.hpp（枚举树+分类注册）+ <perfetto.h>。
// 关键顺序坑：ProfilerManager.hpp 必须在 <perfetto.h> 之后 include——否则
// mc::profiler 命名空间先于 ::perfetto 声明，会导致 SDK 头文件内对 `perfetto::`
// 的名字查找在 mc::profiler 与 ::perfetto 之间歧义。
//
// Tracy 后端启用时：需 <tracy/Tracy.hpp>（含 ZoneScopedN/TracyPlot/TracyMessageL/
// tracy::SetThreadName）。ProfilerManager.hpp 始终需要（MC_TRACE_SET_THREAD_NAME 指向它）。

#if MC_ENABLE_TRACING

#include "TraceCategories.hpp"

// 禁用 Perfetto SDK 的警告
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <perfetto.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// 必须在 <perfetto.h> 之后（见上方顺序坑说明）
#include "ProfilerManager.hpp"

#endif // MC_ENABLE_TRACING

#if MC_ENABLE_TRACY

// 禁用 Tracy 的警告
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <tracy/Tracy.hpp>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// Tracy 单独启用时（无 Perfetto）也需要 ProfilerManager 做命名双写，
// 并引入 TraceCategories.hpp 提供 mc::trace::TraceEvents 枚举树（该枚举树在
// TraceCategories.hpp 中无条件定义，Perfetto 关闭时仍可用）。
#if !MC_ENABLE_TRACING
#include "ProfilerManager.hpp"
#include "TraceCategories.hpp"
#endif

// ----------------------------------------------------------------------------
// Tracy zone 变量名唯一化
// ----------------------------------------------------------------------------
// Tracy 的 ZoneScopedN(name) 内部用固定变量名 ___tracy_scoped_zone 声明 RAII
// zone 对象，同一作用域内出现两个 MC_TRACE_SCOPED_EVENT（Perfetto 常见用法）会
// 触发 redefinition。改用 ZoneNamedN 配合 __LINE__ 后缀的变量名，使每个 zone
// 变量唯一，允许同作用域多个 zone 共存。TRACY_ENABLE 未定义时 ZoneNamedN 空展开。
#define MC_TRACY_ZONE_CONCAT_INDIR(a, b) a##b
#define MC_TRACY_ZONE_CONCAT(a, b) MC_TRACY_ZONE_CONCAT_INDIR(a, b)
#define MC_TRACY_SCOPED_ZONE(name) ZoneNamedN(MC_TRACY_ZONE_CONCAT(___tracy_zone_, __LINE__), name, true)

#endif // MC_ENABLE_TRACY

// ============================================================================
// 追踪事件宏
// ============================================================================
//
// 所有宏的第一个参数 category 取自 mc::trace::TraceEvents 枚举树
// （类型为 const char*，值是字符串字面量）。Perfetto SDK 在编译期将其匹配到
// 已注册分类，走静态路径，零运行时查找开销。
//
// 引用枚举树时需用全限定名 ::mc::trace::TraceEvents（或当前作用域已可见
// mc::trace）。为简化书写，下方宏不限定命名空间，调用方按需自行限定。

#if MC_ENABLE_TRACING && MC_ENABLE_TRACY

// === 双轨：Perfetto + Tracy 同时录制 ===

#define MC_TRACE_SCOPED_EVENT(category, name, ...)          \
    TRACE_EVENT(category, name __VA_OPT__(, ) __VA_ARGS__); \
    MC_TRACY_SCOPED_ZONE(name)

#define MC_TRACE_INSTANT_EVENT(category, name, ...)                 \
    TRACE_EVENT_INSTANT(category, name __VA_OPT__(, ) __VA_ARGS__); \
    TracyMessageL(name)

#define MC_TRACE_COUNTER(category, name, value) \
    TRACE_COUNTER(category, name, value);       \
    TracyPlot(name, static_cast<double>(value))

// BEGIN/END 用得极少（仅 WeatherRenderer 5 处），Tracy 侧降级为 message 边界标记
#define MC_TRACE_EVENT_BEGIN(category, name, ...)                 \
    TRACE_EVENT_BEGIN(category, name __VA_OPT__(, ) __VA_ARGS__); \
    TracyMessageL(name)

#define MC_TRACE_EVENT_END(category) \
    TRACE_EVENT_END(category);       \
    TracyMessageL("[trace end]")

#elif MC_ENABLE_TRACING

// === 仅 Perfetto ===

#define MC_TRACE_SCOPED_EVENT(category, name, ...) TRACE_EVENT(category, name __VA_OPT__(, ) __VA_ARGS__)
#define MC_TRACE_INSTANT_EVENT(category, name, ...) TRACE_EVENT_INSTANT(category, name __VA_OPT__(, ) __VA_ARGS__)
#define MC_TRACE_COUNTER(category, name, value) TRACE_COUNTER(category, name, value)
#define MC_TRACE_EVENT_BEGIN(category, name, ...) TRACE_EVENT_BEGIN(category, name __VA_OPT__(, ) __VA_ARGS__)
#define MC_TRACE_EVENT_END(category) TRACE_EVENT_END(category)

#elif MC_ENABLE_TRACY

// === 仅 Tracy ===
// Tracy 的 zone 用 name 字面量做标题；Perfetto 专有的 category/键值对参数被忽略。
// 计数器走 TracyPlot（double）；BEGIN/END 降级为 message 边界标记。

#define MC_TRACE_SCOPED_EVENT(category, name, ...) MC_TRACY_SCOPED_ZONE(name)

#define MC_TRACE_INSTANT_EVENT(category, name, ...) TracyMessageL(name)

#define MC_TRACE_COUNTER(category, name, value) TracyPlot(name, static_cast<double>(value))

#define MC_TRACE_EVENT_BEGIN(category, name, ...) TracyMessageL(name)

#define MC_TRACE_EVENT_END(category) TracyMessageL("[trace end]")

#else

// === 两后端皆关：全部空操作 ===

#define MC_TRACE_SCOPED_EVENT(category, name, ...) ((void)0)
#define MC_TRACE_INSTANT_EVENT(category, name, ...) ((void)0)
#define MC_TRACE_COUNTER(category, name, value) ((void)0)
#define MC_TRACE_EVENT_BEGIN(category, name, ...) ((void)0)
#define MC_TRACE_EVENT_END(category) ((void)0)

#endif

// ============================================================================
// 内存追踪宏
// ============================================================================
//
// 分配级内存追踪，与上面的 CPU zone 事件正交。语义为「在 name 标记的内存池中
// 分配/释放 ptr 处 size 字节」。调用方只感知 (name, ptr, size) 三元组，不关心
// 底层由哪个分析器实现——这是高层语义化抽象，便于以后接入其他内存分析器
// （如 jemalloc stats、Heaptrack、自研 arena 统计）时只改本文件底部分支，
// 调用方宏签名不变。
//
// 当前仅 Tracy 后端提供实现（TracyAllocN/TracyFreeN，带调用栈，Tracy UI 按
// name 分组显示）。Perfetto SDK 无等价的分配插桩 API，故 MC_ENABLE_MEMORY
// 开启但 MC_ENABLE_TRACY 关闭时宏空展开。
//
// 注意事项：
// - alloc/free 必须严格配对，ptr 必须是同一指针；错配会污染 Tracy 泄漏视图
// - std::vector 等 realloc 容器会使旧指针失效，建议只标大块分配点（reserve/
//   resize/构造），勿标高频小分配（如 per-vertex push_back），否则刷爆缓冲
// - shared_ptr 析构时机不确定，对象级标记是近似；要精确需在析构函数插 free
// - 标的是「对象级事件」：sizeof(T) 只反映外层结构体，对象内部 vector 等分配另算

#if MC_ENABLE_MEMORY && MC_ENABLE_TRACY

// === 内存追踪：Tracy 分配级追踪 ===
#define MC_TRACE_MEM_ALLOC(name, ptr, size) TracyAllocN(ptr, size, name)
#define MC_TRACE_MEM_FREE(name, ptr) TracyFreeN(ptr, name)

#elif MC_ENABLE_MEMORY

// === 内存追踪启用但无可用后端（Perfetto 无分配插桩 API）：空操作 ===
// NOTE: 未来接入其他内存分析器时在此分支扩展。
#define MC_TRACE_MEM_ALLOC(name, ptr, size) ((void)0)
#define MC_TRACE_MEM_FREE(name, ptr) ((void)0)

#else

// === 内存追踪关闭：空操作，零开销 ===
#define MC_TRACE_MEM_ALLOC(name, ptr, size) ((void)0)
#define MC_TRACE_MEM_FREE(name, ptr) ((void)0)

#endif

// ============================================================================
// 线程命名宏
// ============================================================================
//
// 始终转调 ProfilerManager::setThreadName，由 Manager 统一注入 Perfetto 的
// sibling_order_rank（基于线程名查表）并双写 Tracy。至少一个后端启用时有效。

#if MC_PROFILER_ENABLED

/**
 * @brief 设置当前线程名称（双轨）
 *
 * 在 Perfetto/Tracy 中显示有意义的线程名称，便于分析。内部转调
 * ProfilerManager::setThreadName，由 Manager 统一注入 sibling_order_rank
 * （基于线程名查表）并双写两套后端。应在线程启动后尽早调用。
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
#define MC_TRACE_SET_THREAD_NAME(name) ::mc::profiler::ProfilerManager::instance().setThreadName(name)

#else // MC_PROFILER_ENABLED == 0

#define MC_TRACE_SET_THREAD_NAME(name) ((void)0)

#endif // MC_PROFILER_ENABLED
