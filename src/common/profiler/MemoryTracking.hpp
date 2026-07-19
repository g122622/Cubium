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
 * @file MemoryTracking.hpp
 * @brief 分配级内存追踪：自定义分配器 + 对象级守卫（Tracy 后端）
 *
 * TraceEvents.hpp 里的 `MC_TRACE_MEM_ALLOC/FREE` 宏是「手动插桩」API——调用方
 * 自己保证 alloc/free 配对与指针稳定。但实践中最常见的两类追踪对象——
 * `std::vector` 的内部缓冲区与堆对象本身——其指针生命周期无法在调用点可靠
 * 捕获，手动宏极易违反 Tracy 的硬不变量（见下），导致会话被终止。本文件提供
 * 两个能自动维持不变量的高层工具：
 *
 *   - TracyTrackingAlloc<T, kName>：有状态分配器，截获 vector 每次 allocate/
 *     deallocate（含 realloc 的成对 free+alloc），从根上保证「同一指针严格一对一」
 *   - TracyObjectTracker<kName>：RAII 成员守卫，绑定宿主对象地址，ctor 发 alloc、
 *     dtor 发 free，并正确处理 move 语义（追踪权随对象转移）
 *
 * ## Tracy 硬不变量（不可绕过）
 *
 * `TracyAllocN(ptr, size, name)` 要求 `(name, ptr)` 当前不在活跃集中；`TracyFreeN
 * (ptr, name)` 要求 `(name, ptr)` 当前在活跃集中。违反前者触发 Failure::MemAllocTwice
 * （"already tracked and not freed"），违反后者触发 Failure::MemFree（"free without
 * matching allocation"）。**两者都是硬失败、无任何 flag 可关闭**（TRACY_IGNORE_
 * MEMORY_FAULTS 只压 MemFree、压不了 MemAllocTwice），直接终止整个 profiling 会话。
 * 详见 third_party/tracy/server/TracyWorker.cpp 的 ProcessMemAllocImpl /
 * ProcessMemFreeImpl。
 *
 * 因此不能把 `MC_TRACE_MEM_ALLOC/FREE` 直接绑到 `std::vector::data()` 上：vector 的
 * reserve（不 realloc 时返回同指针）、clear（不释放、不改 data()）、reassign、
 * realloc（旧指针静默释放、Tracy 不知情）都会破坏不变量。本文件的两个工具正是为
 * 规避此问题而生。
 *
 * ## 与手动宏的关系
 *
 * 这两个工具是 `MC_TRACE_MEM_ALLOC/FREE` 的「安全封装」，仅当 MC_ENABLE_MEMORY &&
 * MC_ENABLE_TRACY 同时开启时才真正发 Tracy 事件；其余分支空操作零开销。优先使用
 * 本文件的工具，手动宏仅留给无法套用这两种模式的极少数场景（且须极其小心配对）。
 */

#pragma once

#include "ProfilerConfig.hpp"

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>

#if MC_ENABLE_MEMORY && MC_ENABLE_TRACY

// Tracy 头文件含较多警告，统一屏蔽
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <tracy/Tracy.hpp>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif

namespace mc::profiler {

// ============================================================================
// fixed_string：把字符串字面量做成 NTTP，让 name 进类型系统
// ============================================================================
//
// Tracy 的 alloc/free 按 name 分组（同一 name 的指针在同一活跃集）。把 name 作为
// NTTP 编进分配器/守卫的类型，使「不同 name 的容器」成为不同类型，编译期即可隔离，
// 运行期零开销传递 name（无需 strcmp）。C++20 fixed-string NTTP 是标准做法。

template <std::size_t N>
struct FixedString {
    char chars[N] = {};

    // NOLINTNEXTLINE(google-explicit-constructor)：字符串字面量隐式构造是本类型的目的
    constexpr FixedString(const char (&str)[N]) // NOLINT(google-explicit-constructor)
    {
        for (std::size_t i = 0; i < N; ++i) {
            chars[i] = str[i];
        }
    }
};

#if MC_ENABLE_MEMORY && MC_ENABLE_TRACY

// ============================================================================
// TracyTrackingAlloc：有状态分配器（方案 A）
// ============================================================================
//
// 截获容器的每次 allocate/deallocate。vector 的 realloc 在标准层面是「deallocate
// 旧块 + allocate 新块」的成对调用（即便底层 malloc 实现复用地址，allocate 与
// deallocate 的先后顺序也保证 Tracy 看到正确的 free→alloc 序列），故每次真实分配
// 都被精确捕获，无指针泄漏。
//
// 注意：
// - 这是等比（propagating）分配器：相同 (T, kName) 的实例互相视为相等，可互换；
//   不同 kName 是不同类型，容器不可互相赋值（这符合预期——不同内存池本不应混用）。
// - TracyAllocN 的 size 用 n * sizeof(T)，deallocate 不带 size（Tracy free 不需要）。
// - allocate 失败时 new 自身抛 bad_alloc，不会发出 Tracy 事件（正确：未分配无需追踪）。

template <typename T, FixedString kName>
class TracyTrackingAlloc {
public:
    using value_type = T;
    // 无状态分配器：所有实例等价（name 已进类型，不存运行期状态）。显式声明
    // is_always_equal 让 allocator_traits 走「直接偷缓冲区」的 move/swap 快路径，
    // 不做分配器比较与重分配。即便 MSVC 对空类默认推断为 true_type，显式声明可
    // 防止将来误加数据成员时 trait 静默翻成 false 触发意外的重分配路径。
    using is_always_equal = std::true_type;

    // rebind：std::allocator_traits 用它把 T 换成 U（如 vector 内部按节点重绑）。本分配器
    // 第二个模板参数 kName 是 NTTP，MSVC STL 的默认 rebind 回退（_Replace_first_parameter）
    // 无法处理多参数/含 NTTP 的分配器，故必须显式提供 rebind，保留 kName 不变。
    template <typename U>
    struct rebind {
        using other = TracyTrackingAlloc<U, kName>;
    };

    TracyTrackingAlloc() noexcept = default;

    // 等比分配器：相同类型可从任意实例构造（含 const）
    template <typename U>
    // NOLINTNEXTLINE(google-explicit-constructor)：分配器转换需隐式
    TracyTrackingAlloc(const TracyTrackingAlloc<U, kName>&) noexcept
    {}

    [[nodiscard]] T* allocate(std::size_t n)
    {
        T* p = std::allocator<T>{}.allocate(n);
        TracyAllocN(static_cast<const void*>(p), n * sizeof(T), kName.chars);
        return p;
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        // nullptr free 被 Tracy 静默忽略（见 ProcessMemFreeImpl），但此处仍可能出现
        // p==nullptr（某些容器对空区间调 deallocate），交给 Tracy 处理即可。
        // 【关键】必须把真实计数 n 透传给底层 allocator——MSVC 的 std::allocator::
        // deallocate 会用 sizeof(T)*n 作尺寸提示转发给带尺寸 operator delete，传 0 会
        // 给 CRT 堆管理器错误尺寸、破坏堆元数据，导致后续堆操作崩溃。
        TracyFreeN(static_cast<const void*>(p), kName.chars);
        std::allocator<T>{}.deallocate(p, n);
    }
};

// 等比分配器相等性：相同 (T, kName) 相等，否则不等（不同 name 不可互换容器）
template <typename T, FixedString kName, typename U, FixedString kOtherName>
[[nodiscard]] inline bool operator==(
    const TracyTrackingAlloc<T, kName>&, const TracyTrackingAlloc<U, kOtherName>&) noexcept
{
    return std::is_same_v<T, U> && kName.chars == kOtherName.chars;
}

// ============================================================================
// TracyObjectTracker：对象级 RAII 守卫（方案 B）
// ============================================================================
//
// 用于追踪「对象本身」的驻留：作为宿主类的成员，绑定宿主地址，发出 alloc/free。
// `this`（宿主地址）在对象生命周期内稳定，天然满足 Tracy 的一对一不变量。比在
// make_unique 调用点插 alloc 更稳健——调用点无法捕获析构。
//
// 核心难点：宿主 move 时地址会变。Tracy 追踪的是地址，move 后旧地址的内存被回收、
// 可能被堆复用于下一次分配，若旧地址仍留在活跃集中，下次 alloc 该地址即触发
// MemAllocTwice 硬失败（正是本提交修的 bug）。故 move 必须「释放旧地址 + 分配新地址」，
// 而非简单转移指针。因此守卫【不可移动】，由宿主在 move ctor/assign 中显式重绑定：
//
//   - bind(host)：标记 host 为活跃（发 alloc）
//   - unbind()：若当前活跃则释放（发 free），变为非活跃
//   - 析构：若仍活跃则释放
//
// 宿主约定（见 ChunkData 实现）：
//   - 普通 ctor：初始化列表 m_memTrack(this)（或默认构造后 bind(this)）
//   - move ctor：源 m_memTrack.unbind()（释放旧地址），目标 bind(this)（分配新地址）
//   - move assign：目标 unbind()，源 unbind()，目标 bind(this)
//   - dtor：自动（若仍活跃则释放）
//
// 用法：作为宿主类成员（位置无强制要求——守卫只存一个 void* + 一个 size_t，不访问
//       其余成员），在构造函数初始化列表传 this。alloc 的 size 取 sizeof(宿主类型)
//       （由 ctor/bind 的模板参数从传入的 this 指针推导），故曲线反映「对象数 × 对象
//       体积」。注意这**只计入外层结构体**，内部 vector 等堆分配另算——对象级语义下
//       绝对值会偏低，但能正确反映对象的驻留波动（加载增长、卸载回落）。
//       【不要传 size=0】——Tracy 内存曲线按 `usage += size` 累加（见 TracyWorker.cpp
//       的 ProcessMemAllocImpl），size=0 则曲线全程为 0，事件虽发但不可见。

template <FixedString kName>
class TracyObjectTracker {
public:
    TracyObjectTracker() noexcept = default;

    // 宿主在构造列表传入 this：立即标记活跃并发 alloc，size 取 sizeof(宿主类型)
    // （从指针推导）。模板化以保留宿主类型，避免退化为 void* 丢掉 size 信息。
    template <typename T>
    explicit TracyObjectTracker(const T* host) noexcept
        : m_ptr(host)
        , m_owns(host != nullptr)
    {
        if (m_owns) {
            TracyAllocN(m_ptr, sizeof(T), kName.chars);
        }
    }

    ~TracyObjectTracker() { unbind(); }

    // 不可拷贝、不可移动：地址追踪语义要求宿主显式重绑定（见类注释）
    TracyObjectTracker(const TracyObjectTracker&) = delete;
    TracyObjectTracker& operator=(const TracyObjectTracker&) = delete;
    TracyObjectTracker(TracyObjectTracker&&) = delete;
    TracyObjectTracker& operator=(TracyObjectTracker&&) = delete;

    /** @brief 绑定宿主地址并标记活跃（发 alloc，size=sizeof(宿主)）。重复调用会先释放旧绑定。 */
    template <typename T>
    void bind(const T* host) noexcept
    {
        if (m_owns) {
            unbind();
        }
        m_ptr = host;
        m_owns = (host != nullptr);
        if (m_owns) {
            TracyAllocN(m_ptr, sizeof(T), kName.chars);
        }
    }

    /** @brief 释放当前绑定（发 free），变为非活跃。已非活跃时为空操作。 */
    void unbind() noexcept
    {
        if (m_owns) {
            TracyFreeN(m_ptr, kName.chars);
            m_owns = false;
            m_ptr = nullptr;
        }
    }

private:
    const void* m_ptr = nullptr;
    bool m_owns = false;
};

#else // !(MC_ENABLE_MEMORY && MC_ENABLE_TRACY)：空操作实现，零开销

template <typename T, FixedString kName>
class TracyTrackingAlloc {
public:
    using value_type = T;
    using is_always_equal = std::true_type; // 见上方启用版本注释

    // rebind：见上方启用版本的同名注释（多参数/NTTP 分配器必须显式提供）
    template <typename U>
    struct rebind {
        using other = TracyTrackingAlloc<U, kName>;
    };

    TracyTrackingAlloc() noexcept = default;
    template <typename U>
    // NOLINTNEXTLINE(google-explicit-constructor)
    TracyTrackingAlloc(const TracyTrackingAlloc<U, kName>&) noexcept
    {}

    [[nodiscard]] T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
    void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>{}.deallocate(p, n); }
};

template <typename T, FixedString kName, typename U, FixedString kOtherName>
[[nodiscard]] inline bool operator==(
    const TracyTrackingAlloc<T, kName>&, const TracyTrackingAlloc<U, kOtherName>&) noexcept
{
    return std::is_same_v<T, U> && kName.chars == kOtherName.chars;
}

// 空操作守卫：无成员、无开销。bind/unbind 保留为空方法，使宿主代码在开关
// 关闭时无需 #if 守卫即可统一调用。ctor/bind 同样模板化以与启用版本签名一致。
template <FixedString kName>
class TracyObjectTracker {
public:
    TracyObjectTracker() noexcept = default;
    template <typename T>
    explicit TracyObjectTracker(const T* /*host*/) noexcept
    {}
    template <typename T>
    void bind(const T* /*host*/) noexcept
    {}
    void unbind() noexcept {}
};

#endif // MC_ENABLE_MEMORY && MC_ENABLE_TRACY

} // namespace mc::profiler
