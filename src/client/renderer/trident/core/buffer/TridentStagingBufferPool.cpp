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

#include "client/renderer/trident/core/buffer/TridentStagingBufferPool.hpp"

#include "client/renderer/trident/core/TridentContext.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <cstring>

namespace mc::client::renderer::trident {

// ============================================================================
// 构造 / 析构
// ============================================================================

TridentStagingBufferPool::TridentStagingBufferPool() = default;

TridentStagingBufferPool::~TridentStagingBufferPool()
{
    destroy();
}

// ============================================================================
// 对齐辅助
// ============================================================================

u64 TridentStagingBufferPool::_alignUp(u64 size)
{
    return (size + kStagingAlign - 1) & ~(kStagingAlign - 1);
}

// ============================================================================
// 初始化 / 销毁
// ============================================================================

Result<void> TridentStagingBufferPool::initialize(void* context, u64 initialCapacity, u32 maxFramesInFlight)
{
    auto* tridentContext = static_cast<TridentContext*>(context);
    if (!tridentContext) {
        return Error(ErrorCode::NullPointer, "TridentStagingBufferPool::initialize: context is null");
    }
    if (initialCapacity == 0 || maxFramesInFlight == 0) {
        return Error(ErrorCode::InvalidArgument, "TridentStagingBufferPool::initialize: capacity and maxFramesInFlight must be non-zero");
    }

    m_context = tridentContext;
    m_capacity = initialCapacity;
    m_maxFramesInFlight = maxFramesInFlight;

    VkDevice device = m_context->device();

    // 创建 backing buffer：仅作传输源
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = initialCapacity;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "TridentStagingBufferPool: failed to create backing buffer: " + std::to_string(result));
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

    // HOST_VISIBLE | HOST_COHERENT：持久映射后 memcpy 即可，无需手动 flush
    auto typeResult = m_context->findMemoryType(
        memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (typeResult.failed()) {
        vkDestroyBuffer(device, m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
        return typeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = typeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &m_memory);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(device, m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "TridentStagingBufferPool: failed to allocate backing memory: " + std::to_string(result));
    }

    vkBindBufferMemory(device, m_buffer, m_memory, 0);

    // 持久映射：整段映射一次，后续 stage 直接返回 mappedPtr+offset
    result = vkMapMemory(device, m_memory, 0, initialCapacity, 0, &m_persistentMapped);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(device, m_buffer, nullptr);
        vkFreeMemory(device, m_memory, nullptr);
        m_buffer = VK_NULL_HANDLE;
        m_memory = VK_NULL_HANDLE;
        return Error(ErrorCode::OperationFailed, "TridentStagingBufferPool: failed to map backing memory: " + std::to_string(result));
    }

    // OffsetAllocator 用 u32 offset，容量上限 4GB；这里显式截断校验
    m_allocator = std::make_unique<OffsetAllocator::Allocator>(static_cast<u32>(initialCapacity));
    m_localUsedBytes = 0;
    m_localFreeBytes = initialCapacity;

    m_pendingAsyncBuckets.assign(maxFramesInFlight, {});

    return {};
}

void TridentStagingBufferPool::destroy()
{
    if (m_buffer == VK_NULL_HANDLE) return;

    VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE) return;

    // 销毁前校验无泄漏（同步分配应已全部 release，异步分配应已全部 recycle）
    MC_ASSERT_RELEASE_MSG(m_localUsedBytes == 0,
                          "TridentStagingBufferPool destroyed with staging allocations still in use");

    if (m_persistentMapped) {
        vkUnmapMemory(device, m_memory);
        m_persistentMapped = nullptr;
    }

    vkDestroyBuffer(device, m_buffer, nullptr);
    vkFreeMemory(device, m_memory, nullptr);

    m_buffer = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
    m_allocator.reset();
    m_pendingAsyncBuckets.clear();
    m_capacity = 0;
    m_localUsedBytes = 0;
    m_localFreeBytes = 0;
    m_context = nullptr;
}

// ============================================================================
// 内部分配 / 释放
// ============================================================================

OffsetAllocator::Allocation TridentStagingBufferPool::_allocate(u64 alignedSize)
{
    std::lock_guard<std::mutex> lock(m_allocatorMutex);
    OffsetAllocator::Allocation alloc = m_allocator->allocate(static_cast<u32>(alignedSize));
    if (alloc.offset != OffsetAllocator::Allocation::NO_SPACE) {
        m_localUsedBytes += alignedSize;
        m_localFreeBytes -= alignedSize;
    }
    return alloc;
}

void TridentStagingBufferPool::_freeLocked(OffsetAllocator::Allocation alloc, u64 alignedSize)
{
    m_allocator->free(alloc);
    m_localUsedBytes -= alignedSize;
    m_localFreeBytes += alignedSize;

    // 守恒断言：OffsetAllocator 自报空闲量须与本地记账一致（防范 prototype bug）
    const u32 reported = m_allocator->storageReport().totalFreeSpace;
    MC_ASSERT_RELEASE_MSG(reported == static_cast<u32>(m_localFreeBytes),
                          "TridentStagingBufferPool conservation mismatch: OffsetAllocator free space disagrees with local bookkeeping");
}

void TridentStagingBufferPool::_free(OffsetAllocator::Allocation alloc, u64 alignedSize)
{
    std::lock_guard<std::mutex> lock(m_allocatorMutex);
    _freeLocked(alloc, alignedSize);
}

// ============================================================================
// 同步模式
// ============================================================================

api::StagingHandle TridentStagingBufferPool::stage(u64 size)
{
    api::StagingHandle handle;
    if (size == 0) return handle;

    const u64 alignedSize = _alignUp(size);
    OffsetAllocator::Allocation alloc = _allocate(alignedSize);
    if (alloc.offset == OffsetAllocator::Allocation::NO_SPACE) {
        // 容量不足：返回 valid=false，调用方走 fallback 一次性上传
        return handle;
    }

    handle.mappedPtr = static_cast<u8*>(m_persistentMapped) + alloc.offset;
    handle.offset = alloc.offset;
    handle.metadata = alloc.metadata;
    handle.segmentIndex = 0;
    handle.size = size;
    handle.valid = true;
    return handle;
}

Result<void> TridentStagingBufferPool::copyToBuffer(const api::StagingHandle& handle, void* dstBuffer, u64 dstOffset)
{
    if (!handle.valid) {
        return Error(ErrorCode::InvalidArgument, "TridentStagingBufferPool::copyToBuffer: invalid handle");
    }
    auto dstVkBuffer = static_cast<VkBuffer>(dstBuffer);
    if (dstVkBuffer == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidArgument, "TridentStagingBufferPool::copyToBuffer: null destination buffer");
    }

    // 同步单次命令：submit 后立即等待 fence，返回时 GPU 已完成复制
    VkCommandBuffer cmd = m_context->beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = handle.offset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = handle.size;
    vkCmdCopyBuffer(cmd, m_buffer, dstVkBuffer, 1, &copyRegion);

    m_context->endSingleTimeCommands(cmd);
    return {};
}

void TridentStagingBufferPool::release(const api::StagingHandle& handle)
{
    if (!handle.valid) return;

    // 重建完整 Allocation（offset + metadata），metadata 是 free 定位 node 的关键
    OffsetAllocator::Allocation alloc;
    alloc.offset = static_cast<u32>(handle.offset);
    alloc.metadata = handle.metadata;

    const u64 alignedSize = _alignUp(handle.size);
    _free(alloc, alignedSize);
}

// ============================================================================
// 异步模式
// ============================================================================

api::StagingHandle TridentStagingBufferPool::stageAsync(u64 size, u32 frameIndex)
{
    api::StagingHandle handle;
    if (size == 0) return handle;

    const u64 alignedSize = _alignUp(size);
    OffsetAllocator::Allocation alloc = _allocate(alignedSize);
    if (alloc.offset == OffsetAllocator::Allocation::NO_SPACE) {
        return handle;
    }

    handle.mappedPtr = static_cast<u8*>(m_persistentMapped) + alloc.offset;
    handle.offset = alloc.offset;
    handle.metadata = alloc.metadata;
    handle.segmentIndex = 0;
    handle.size = size;
    handle.valid = true;

    // 登记 Allocation 以便 recycleFrame 时 free（metadata 是 free 定位 node 的关键）
    {
        std::lock_guard<std::mutex> lock(m_allocatorMutex);
        const u32 bucket = frameIndex % m_maxFramesInFlight;
        m_pendingAsyncBuckets[bucket].push_back(AsyncAlloc{alloc, alignedSize, 0});
    }
    return handle;
}

void TridentStagingBufferPool::recycleFrame(u32 frameIndex)
{
    const u32 bucket = frameIndex % m_maxFramesInFlight;
    std::vector<AsyncAlloc> bucketToFree;
    {
        std::lock_guard<std::mutex> lock(m_allocatorMutex);
        bucketToFree.swap(m_pendingAsyncBuckets[bucket]);
    }
    // 已取回 bucket，持锁释放（_freeLocked 复用守恒断言）
    std::lock_guard<std::mutex> lock(m_allocatorMutex);
    for (const AsyncAlloc& a : bucketToFree) {
        _freeLocked(a.alloc, a.alignedSize);
    }
}

void* TridentStagingBufferPool::backingBuffer(u32 segmentIndex) const
{
    MC_UNUSED(segmentIndex); // 当前单段实现，segmentIndex 恒为 0
    return m_buffer;
}

u64 TridentStagingBufferPool::totalFreeSpace() const
{
    std::lock_guard<std::mutex> lock(m_allocatorMutex);
    return m_localFreeBytes;
}

} // namespace mc::client::renderer::trident
