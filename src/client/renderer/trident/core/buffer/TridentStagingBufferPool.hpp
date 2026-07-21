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

#pragma once

#include "client/renderer/api/buffer/IStagingBufferPool.hpp"
#include <memory>
#include <mutex>
#include <vector>

#include "offsetAllocator.hpp" // OffsetAllocator::Allocator / Allocation

#include <vulkan/vulkan.h>

namespace mc::client::renderer::trident {

// 前置声明
class TridentContext;

/**
 * @brief Vulkan 统一暂存缓冲池实现
 *
 * 在一个持久映射的 HOST_VISIBLE|HOST_COHERENT 大 buffer 内，用 OffsetAllocator
 * 子分配暂存区间。OffsetAllocator 不可 resize，当前实现为单段固定容量，溢出时
 * stage 返回 valid=false 由调用方走 fallback 一次性上传（TODO: 多段 Segment 扩容）。
 */
class TridentStagingBufferPool : public api::IStagingBufferPool {
public:
    TridentStagingBufferPool();
    ~TridentStagingBufferPool() override;

    // 禁止拷贝
    TridentStagingBufferPool(const TridentStagingBufferPool&) = delete;
    TridentStagingBufferPool& operator=(const TridentStagingBufferPool&) = delete;

    // IStagingBufferPool 接口实现
    [[nodiscard]] Result<void> initialize(void* context, u64 initialCapacity, u32 maxFramesInFlight) override;
    void destroy() override;

    [[nodiscard]] api::StagingHandle stage(u64 size) override;
    [[nodiscard]] Result<void> copyToBuffer(const api::StagingHandle& handle, void* dstBuffer, u64 dstOffset) override;
    void release(const api::StagingHandle& handle) override;

    [[nodiscard]] api::StagingHandle stageAsync(u64 size, u32 frameIndex) override;
    [[nodiscard]] void* backingBuffer(u32 segmentIndex) const override;
    void recycleFrame(u32 frameIndex) override;

    [[nodiscard]] u64 totalFreeSpace() const override;

private:
    /// 对齐粒度：满足 vkCmdCopyBufferToImage 的 bufferOffset 4 字节要求，并兼顾顶点属性访问
    static constexpr u64 kStagingAlign = 16;

    /// 异步回收桶内的单条分配记录
    struct AsyncAlloc {
        OffsetAllocator::Allocation alloc; // 由调用方释放，此处仅记账
        u64 alignedSize = 0;
        u32 segmentIndex = 0;
    };

    /// 将 size 向上对齐到 kStagingAlign
    [[nodiscard]] static u64 _alignUp(u64 size);

    /// 实际从分配器分配对齐区间，返回 Allocation 与对齐大小；失败返回 NO_SPACE
    [[nodiscard]] OffsetAllocator::Allocation _allocate(u64 alignedSize);

    /// 归还一条分配并校验守恒
    void _free(OffsetAllocator::Allocation alloc, u64 alignedSize);

    /// 归还单段内的分配（加锁）
    void _freeLocked(OffsetAllocator::Allocation alloc, u64 alignedSize);

    TridentContext* m_context = nullptr;
    VkBuffer m_buffer = VK_NULL_HANDLE;        // 单段 backing buffer
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_capacity = 0;
    void* m_persistentMapped = nullptr;        // 全程持久映射
    u32 m_maxFramesInFlight = 2;

    std::unique_ptr<OffsetAllocator::Allocator> m_allocator;

    /// 异步回收桶：按 frameIndex % maxFramesInFlight 分桶
    std::vector<std::vector<AsyncAlloc>> m_pendingAsyncBuckets;

    mutable std::mutex m_allocatorMutex; // 保护 m_allocator 与异步桶（OffsetAllocator 非线程安全）

    // 本地记账，用于守恒断言（应对 OffsetAllocator prototype 的可靠性风险）
    u64 m_localUsedBytes = 0;
    u64 m_localFreeBytes = 0;
};

} // namespace mc::client::renderer::trident
