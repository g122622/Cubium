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

#include "MemoryProfiler.hpp"

#include <atomic>
#include <cstdlib>
#include <new>

// ============================================================================
// 全局分配计数器与统计开关（文件级，全局命名空间）
// ============================================================================

namespace {

// 原子计数器：分配次数 / 分配字节 / 释放次数 / 释放字节。
// 全局 operator new/delete 在进程任何线程都可能被调用，必须原子。
std::atomic<std::uint64_t> g_allocCount{0};
std::atomic<std::uint64_t> g_allocBytes{0};
std::atomic<std::uint64_t> g_freeCount{0};
std::atomic<std::uint64_t> g_freeBytes{0};

// 统计开关：Start() 置位、Stop() 复位。未置位时钩子只透传不计数，
// 保证 Stop 之后的分配（库自身的收尾、下一次用例的 setup 等）不污染本次结果。
std::atomic<bool> g_counting{false};

inline void recordAlloc(std::size_t size)
{
    if (g_counting.load(std::memory_order::relaxed)) {
        g_allocCount.fetch_add(1, std::memory_order::relaxed);
        g_allocBytes.fetch_add(static_cast<std::uint64_t>(size), std::memory_order::relaxed);
    }
}

inline void recordFree(std::size_t size)
{
    if (g_counting.load(std::memory_order::relaxed)) {
        g_freeCount.fetch_add(1, std::memory_order::relaxed);
        g_freeBytes.fetch_add(static_cast<std::uint64_t>(size), std::memory_order::relaxed);
    }
}

inline void recordFreeCountOnly()
{
    if (g_counting.load(std::memory_order::relaxed)) {
        g_freeCount.fetch_add(1, std::memory_order::relaxed);
    }
}

} // namespace

// ============================================================================
// 全局 operator new/delete 替换（必须位于全局命名空间）
//
// 覆盖全部常规 + 对齐 + nothrow 变体。释放字节统计依赖带 size 的 sized delete
// （C++14，编译器在静态类型已知时选用）；无 size 的 delete 变体只计次数。
// net_heap_growth 因此是"下界近似"口径（见 MemoryProfiler.hpp 注释）。
// ============================================================================

void* operator new(std::size_t size)
{
    if (void* ptr = std::malloc(size == 0 ? 1 : size); ptr != nullptr) {
        recordAlloc(size);
        return ptr;
    }
    throw std::bad_alloc{};
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void operator delete(void* ptr) noexcept
{
    if (ptr == nullptr) {
        return;
    }
    std::free(ptr);
    recordFreeCountOnly();
}

void operator delete[](void* ptr) noexcept
{
    ::operator delete(ptr);
}

void operator delete(void* ptr, std::size_t size) noexcept
{
    if (ptr == nullptr) {
        return;
    }
    std::free(ptr);
    recordFree(size);
}

void operator delete[](void* ptr, std::size_t size) noexcept
{
    ::operator delete(ptr, size);
}

// —— 对齐分配版本（C++17 over-aligned types）——

void* operator new(std::size_t size, std::align_val_t alignment)
{
    const std::size_t align = static_cast<std::size_t>(alignment);
    void* ptr = nullptr;
#if defined(_WIN32)
    ptr = _aligned_malloc(size == 0 ? 1 : size, align);
#else
    if (posix_memalign(&ptr, align, size == 0 ? 1 : size) != 0) {
        ptr = nullptr;
    }
#endif
    if (ptr != nullptr) {
        recordAlloc(size);
        return ptr;
    }
    throw std::bad_alloc{};
}

void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return ::operator new(size, alignment);
}

void operator delete(void* ptr, std::align_val_t) noexcept
{
    if (ptr == nullptr) {
        return;
    }
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
    recordFreeCountOnly();
}

void operator delete[](void* ptr, std::align_val_t alignment) noexcept
{
    ::operator delete(ptr, alignment);
}

void operator delete(void* ptr, std::size_t size, std::align_val_t) noexcept
{
    if (ptr == nullptr) {
        return;
    }
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
    recordFree(size);
}

void operator delete[](void* ptr, std::size_t size, std::align_val_t alignment) noexcept
{
    ::operator delete(ptr, size, alignment);
}

// —— nothrow 版本：转发 throwing 版本，异常吞掉返回空指针 ——

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    try {
        return ::operator new(size);
    }
    catch (...) {
        return nullptr;
    }
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
    try {
        return ::operator new[](size);
    }
    catch (...) {
        return nullptr;
    }
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    try {
        return ::operator new(size, alignment);
    }
    catch (...) {
        return nullptr;
    }
}

void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    try {
        return ::operator new[](size, alignment);
    }
    catch (...) {
        return nullptr;
    }
}

// ============================================================================
// MemoryProfiler 实现
// ============================================================================

namespace mc::benchmark {

MemoryProfiler::Snapshot MemoryProfiler::takeSnapshot()
{
    Snapshot snapshot;
    snapshot.allocCount = g_allocCount.load(std::memory_order::relaxed);
    snapshot.allocBytes = g_allocBytes.load(std::memory_order::relaxed);
    snapshot.freeCount = g_freeCount.load(std::memory_order::relaxed);
    snapshot.freeBytes = g_freeBytes.load(std::memory_order::relaxed);
    return snapshot;
}

MemoryProfiler& MemoryProfiler::instance() noexcept
{
    // 函数级静态：构造于首次使用（早于 RunSpecifiedBenchmarks），析构于进程退出。
    // google/benchmark 持有的裸指针在整个运行期间有效。
    static MemoryProfiler profiler;
    return profiler;
}

void MemoryProfiler::enableHook() noexcept
{
    // 全局 operator new/delete 替换是链接期行为（本 TU 参与链接即生效），
    // 无需运行期安装。此函数保留为显式语义入口：main() 调用它以声明"统计已就绪"。
}

void MemoryProfiler::Start()
{
    m_startSnapshot = takeSnapshot();
    g_counting.store(true, std::memory_order::release);
}

void MemoryProfiler::Stop(Result& result)
{
    const Snapshot current = takeSnapshot();
    g_counting.store(false, std::memory_order::release);

    const std::uint64_t allocCount = current.allocCount - m_startSnapshot.allocCount;
    const std::uint64_t allocBytes = current.allocBytes - m_startSnapshot.allocBytes;
    const std::uint64_t freeBytes = current.freeBytes - m_startSnapshot.freeBytes;

    result.num_allocs = static_cast<std::int64_t>(allocCount);
    // 峰值在用内存无法从纯 new/delete 钩子精确得到（需逐块跟踪）；以
    // "累计分配 - 累计释放"（近似在用）代替峰值，口径在头文件注释中说明。
    const std::uint64_t netBytes = allocBytes >= freeBytes ? allocBytes - freeBytes : 0;
    result.max_bytes_used = static_cast<std::int64_t>(netBytes);
    result.total_allocated_bytes = static_cast<std::int64_t>(allocBytes);
    result.net_heap_growth = static_cast<std::int64_t>(netBytes);
    result.memory_iterations = 0;
}

} // namespace mc::benchmark
