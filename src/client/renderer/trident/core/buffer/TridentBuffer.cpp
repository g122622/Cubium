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

#include "TridentBuffer.hpp"
#include "client/renderer/api/Types.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <cstring>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

// ============================================================================
// TridentBuffer 实现
// ============================================================================

TridentBuffer::TridentBuffer() = default;

TridentBuffer::~TridentBuffer()
{
    destroy();
}

TridentBuffer::TridentBuffer(TridentBuffer&& other) noexcept
    : m_context(other.m_context)
    , m_buffer(other.m_buffer)
    , m_memory(other.m_memory)
    , m_size(other.m_size)
    , m_mapped(other.m_mapped)
    , m_usage(other.m_usage)
{
    other.m_context = nullptr;
    other.m_buffer = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_size = 0;
    other.m_mapped = nullptr;
}

TridentBuffer& TridentBuffer::operator=(TridentBuffer&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_buffer = other.m_buffer;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_mapped = other.m_mapped;
        m_usage = other.m_usage;

        other.m_context = nullptr;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_mapped = nullptr;
    }
    return *this;
}

Result<void> TridentBuffer::create(
    TridentContext* context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    m_context = context;
    m_size = size;

    // 转换用途标志
    if (usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
        m_usage = api::BufferUsage::Vertex;
    } else if (usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
        m_usage = api::BufferUsage::Index;
    } else if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
        m_usage = api::BufferUsage::Uniform;
    } else {
        m_usage = api::BufferUsage::Staging;
    }

    VkDevice device = m_context->device();

    // 创建缓冲区
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create buffer: " + std::to_string(result));
    }

    // 获取内存需求
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

    // 查找内存类型
    auto typeResult = findMemoryType(memRequirements.memoryTypeBits, properties);
    if (typeResult.failed()) {
        vkDestroyBuffer(device, m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
        return typeResult.error();
    }

    // 分配内存
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = typeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &m_memory);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(device, m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate buffer memory: " + std::to_string(result));
    }

    // 绑定内存
    vkBindBufferMemory(device, m_buffer, m_memory, 0);

    return {};
}

void TridentBuffer::destroy()
{
    if (m_buffer == VK_NULL_HANDLE) return;

    VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE) return;

    if (m_mapped) {
        vkUnmapMemory(device, m_memory);
        m_mapped = nullptr;
    }

    vkDestroyBuffer(device, m_buffer, nullptr);
    vkFreeMemory(device, m_memory, nullptr);

    m_buffer = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
    m_size = 0;
    m_context = nullptr;
}

void* TridentBuffer::map()
{
    if (m_buffer == VK_NULL_HANDLE || !m_context) return nullptr;
    if (m_mapped) return m_mapped;

    VkResult result = vkMapMemory(m_context->device(), m_memory, 0, m_size, 0, &m_mapped);
    if (result != VK_SUCCESS) {
        spdlog::error("Failed to map buffer memory: {}", static_cast<i32>(result));
        return nullptr;
    }

    return m_mapped;
}

void TridentBuffer::unmap()
{
    if (m_mapped && m_context) {
        vkUnmapMemory(m_context->device(), m_memory);
        m_mapped = nullptr;
    }
}

Result<void> TridentBuffer::upload(const void* data, u64 size, u64 offset)
{
    if (size + offset > m_size) {
        return Error(ErrorCode::OutOfRange, "Upload size exceeds buffer size");
    }

    void* mapped = map();
    if (!mapped) {
        return Error(ErrorCode::OperationFailed, "Failed to map buffer");
    }

    std::memcpy(static_cast<u8*>(mapped) + offset, data, size);
    return {};
}

Result<u32> TridentBuffer::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    return m_context->findMemoryType(typeFilter, properties);
}

// ============================================================================
// TridentVertexBuffer 实现
// ============================================================================

TridentVertexBuffer::TridentVertexBuffer() = default;

TridentVertexBuffer::~TridentVertexBuffer()
{
    destroy();
}

TridentVertexBuffer::TridentVertexBuffer(TridentVertexBuffer&& other) noexcept
    : m_context(other.m_context)
    , m_buffer(other.m_buffer)
    , m_memory(other.m_memory)
    , m_size(other.m_size)
    , m_vertexStride(other.m_vertexStride)
    , m_mapped(other.m_mapped)
{
    other.m_context = nullptr;
    other.m_buffer = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_size = 0;
    other.m_vertexStride = 0;
    other.m_mapped = nullptr;
}

TridentVertexBuffer& TridentVertexBuffer::operator=(TridentVertexBuffer&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_buffer = other.m_buffer;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_vertexStride = other.m_vertexStride;
        m_mapped = other.m_mapped;

        other.m_context = nullptr;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_vertexStride = 0;
        other.m_mapped = nullptr;
    }
    return *this;
}

Result<void> TridentVertexBuffer::create(TridentContext* context, u64 size, u32 vertexStride)
{
    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    m_context = context;
    m_size = size;
    m_vertexStride = vertexStride;

    VkDevice device = m_context->device();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create vertex buffer: " + std::to_string(result));
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

    auto typeResult = m_context->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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
        return Error(ErrorCode::OutOfMemory, "Failed to allocate vertex buffer memory: " + std::to_string(result));
    }

    vkBindBufferMemory(device, m_buffer, m_memory, 0);
    return {};
}

void TridentVertexBuffer::destroy()
{
    if (m_buffer == VK_NULL_HANDLE) return;

    VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE) return;

    if (m_mapped) {
        vkUnmapMemory(device, m_memory);
        m_mapped = nullptr;
    }

    vkDestroyBuffer(device, m_buffer, nullptr);
    vkFreeMemory(device, m_memory, nullptr);

    m_buffer = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
    m_size = 0;
    m_vertexStride = 0;
    m_context = nullptr;
}

void* TridentVertexBuffer::map()
{
    // 设备本地缓冲区不能直接映射，需要通过暂存缓冲区
    // 这里暂时返回 nullptr，实际使用时需要配合暂存缓冲区
    spdlog::warn("Vertex buffer is device-local and cannot be directly mapped");
    return nullptr;
}

void TridentVertexBuffer::unmap()
{
    // 设备本地缓冲区不需要取消映射
}

Result<void> TridentVertexBuffer::upload(const void* data, u64 size, u64 offset)
{
    if (!m_context) {
        return Error(ErrorCode::NotInitialized, "Buffer not initialized");
    }

    if (!data || size == 0) {
        return Error(ErrorCode::InvalidArgument, "Invalid data pointer or size");
    }

    if (offset + size > m_size) {
        return Error(ErrorCode::OutOfRange, "Upload range exceeds buffer size");
    }

    // 通过统一暂存缓冲池上传：stage → memcpy → copyToBuffer（内部单次命令+等待 fence）→ release
    auto* pool = m_context->stagingPool();
    auto handle = pool->stage(size);
    MC_ASSERT_RELEASE_MSG(handle.valid, "TridentVertexBuffer::upload: staging pool out of space");
    std::memcpy(handle.mappedPtr, data, static_cast<size_t>(size));
    auto copyResult = pool->copyToBuffer(handle, m_buffer, offset);
    pool->release(handle);
    return copyResult;
}

u32 TridentVertexBuffer::vertexCount() const
{
    return m_vertexStride > 0 ? static_cast<u32>(m_size / m_vertexStride) : 0;
}

void TridentVertexBuffer::bind(void* commandBuffer)
{
    VkCommandBuffer cmd = static_cast<VkCommandBuffer>(commandBuffer);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_buffer, &offset);
}

// ============================================================================
// TridentIndexBuffer 实现
// ============================================================================

TridentIndexBuffer::TridentIndexBuffer() = default;

TridentIndexBuffer::~TridentIndexBuffer()
{
    destroy();
}

TridentIndexBuffer::TridentIndexBuffer(TridentIndexBuffer&& other) noexcept
    : m_context(other.m_context)
    , m_buffer(other.m_buffer)
    , m_memory(other.m_memory)
    , m_size(other.m_size)
    , m_indexType(other.m_indexType)
    , m_indexCount(other.m_indexCount)
    , m_mapped(other.m_mapped)
{
    other.m_context = nullptr;
    other.m_buffer = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_size = 0;
    other.m_indexType = api::IndexType::U32;
    other.m_indexCount = 0;
    other.m_mapped = nullptr;
}

TridentIndexBuffer& TridentIndexBuffer::operator=(TridentIndexBuffer&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_buffer = other.m_buffer;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_indexType = other.m_indexType;
        m_indexCount = other.m_indexCount;
        m_mapped = other.m_mapped;

        other.m_context = nullptr;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_indexType = api::IndexType::U32;
        other.m_indexCount = 0;
        other.m_mapped = nullptr;
    }
    return *this;
}

Result<void> TridentIndexBuffer::create(TridentContext* context, u64 size, api::IndexType type)
{
    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    m_context = context;
    m_size = size;
    m_indexType = type;

    // 计算索引数量
    u32 stride = (type == api::IndexType::U16) ? 2 : 4;
    m_indexCount = static_cast<u32>(size / stride);

    VkDevice device = m_context->device();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create index buffer: " + std::to_string(result));
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

    auto typeResult = m_context->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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
        return Error(ErrorCode::OutOfMemory, "Failed to allocate index buffer memory: " + std::to_string(result));
    }

    vkBindBufferMemory(device, m_buffer, m_memory, 0);
    return {};
}

void TridentIndexBuffer::destroy()
{
    if (m_buffer == VK_NULL_HANDLE) return;

    VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE) return;

    vkDestroyBuffer(device, m_buffer, nullptr);
    vkFreeMemory(device, m_memory, nullptr);

    m_buffer = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
    m_size = 0;
    m_indexCount = 0;
    m_context = nullptr;
}

void* TridentIndexBuffer::map()
{
    spdlog::warn("Index buffer is device-local and cannot be directly mapped");
    return nullptr;
}

void TridentIndexBuffer::unmap() {}

Result<void> TridentIndexBuffer::upload(const void* data, u64 size, u64 offset)
{
    if (!m_context) {
        return Error(ErrorCode::NotInitialized, "Buffer not initialized");
    }

    if (!data || size == 0) {
        return Error(ErrorCode::InvalidArgument, "Invalid data pointer or size");
    }

    if (offset + size > m_size) {
        return Error(ErrorCode::OutOfRange, "Upload range exceeds buffer size");
    }

    // 通过统一暂存缓冲池上传：stage → memcpy → copyToBuffer（内部单次命令+等待 fence）→ release
    auto* pool = m_context->stagingPool();
    auto handle = pool->stage(size);
    MC_ASSERT_RELEASE_MSG(handle.valid, "TridentIndexBuffer::upload: staging pool out of space");
    std::memcpy(handle.mappedPtr, data, static_cast<size_t>(size));
    auto copyResult = pool->copyToBuffer(handle, m_buffer, offset);
    pool->release(handle);
    return copyResult;
}

void TridentIndexBuffer::bind(void* commandBuffer)
{
    VkCommandBuffer cmd = static_cast<VkCommandBuffer>(commandBuffer);
    VkIndexType vkIndexType = (m_indexType == api::IndexType::U16) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
    vkCmdBindIndexBuffer(cmd, m_buffer, 0, vkIndexType);
}

// ============================================================================
// TridentUniformBuffer 实现
// ============================================================================

TridentUniformBuffer::TridentUniformBuffer() = default;

TridentUniformBuffer::~TridentUniformBuffer()
{
    destroy();
}

TridentUniformBuffer::TridentUniformBuffer(TridentUniformBuffer&& other) noexcept
    : m_context(other.m_context)
    , m_buffers(std::move(other.m_buffers))
    , m_memories(std::move(other.m_memories))
    , m_mapped(std::move(other.m_mapped))
    , m_size(other.m_size)
    , m_frameCount(other.m_frameCount)
    , m_currentFrame(other.m_currentFrame)
{
    other.m_context = nullptr;
    other.m_size = 0;
    other.m_frameCount = 0;
    other.m_currentFrame = 0;
}

TridentUniformBuffer& TridentUniformBuffer::operator=(TridentUniformBuffer&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_buffers = std::move(other.m_buffers);
        m_memories = std::move(other.m_memories);
        m_mapped = std::move(other.m_mapped);
        m_size = other.m_size;
        m_frameCount = other.m_frameCount;
        m_currentFrame = other.m_currentFrame;

        other.m_context = nullptr;
        other.m_size = 0;
        other.m_frameCount = 0;
        other.m_currentFrame = 0;
    }
    return *this;
}

Result<void> TridentUniformBuffer::create(TridentContext* context, u64 size, u32 frameCount)
{
    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    m_context = context;
    m_size = size;
    m_frameCount = frameCount;

    m_buffers.resize(frameCount);
    m_memories.resize(frameCount);
    m_mapped.resize(frameCount, nullptr);

    VkDevice device = m_context->device();

    for (u32 i = 0; i < frameCount; ++i) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffers[i]);
        if (result != VK_SUCCESS) {
            destroy();
            return Error(ErrorCode::OutOfMemory, "Failed to create uniform buffer: " + std::to_string(result));
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, m_buffers[i], &memRequirements);

        auto typeResult = m_context->findMemoryType(
            memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (typeResult.failed()) {
            destroy();
            return typeResult.error();
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = typeResult.value();

        result = vkAllocateMemory(device, &allocInfo, nullptr, &m_memories[i]);
        if (result != VK_SUCCESS) {
            destroy();
            return Error(ErrorCode::OutOfMemory, "Failed to allocate uniform buffer memory: " + std::to_string(result));
        }

        vkBindBufferMemory(device, m_buffers[i], m_memories[i], 0);

        // 持久映射
        result = vkMapMemory(device, m_memories[i], 0, size, 0, &m_mapped[i]);
        if (result != VK_SUCCESS) {
            destroy();
            return Error(ErrorCode::OperationFailed, "Failed to map uniform buffer: " + std::to_string(result));
        }
    }

    return {};
}

void TridentUniformBuffer::destroy()
{
    if (m_buffers.empty()) return;

    VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE) return;

    for (u32 i = 0; i < m_buffers.size(); ++i) {
        if (m_mapped[i]) {
            vkUnmapMemory(device, m_memories[i]);
        }
        if (m_buffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_buffers[i], nullptr);
        }
        if (m_memories[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_memories[i], nullptr);
        }
    }

    m_buffers.clear();
    m_memories.clear();
    m_mapped.clear();
    m_size = 0;
    m_frameCount = 0;
    m_currentFrame = 0;
    m_context = nullptr;
}

void* TridentUniformBuffer::map()
{
    if (m_mapped.empty()) return nullptr;
    return m_mapped[m_currentFrame];
}

void TridentUniformBuffer::unmap()
{
    // 持久映射，不需要取消映射
}

Result<void> TridentUniformBuffer::upload(const void* data, u64 size, u64 offset)
{
    if (size + offset > m_size) {
        return Error(ErrorCode::OutOfRange, "Upload size exceeds buffer size");
    }

    void* mapped = map();
    if (!mapped) {
        return Error(ErrorCode::OperationFailed, "Failed to map uniform buffer");
    }

    std::memcpy(static_cast<u8*>(mapped) + offset, data, size);
    return {};
}

void TridentUniformBuffer::advanceFrame()
{
    m_currentFrame = (m_currentFrame + 1) % m_frameCount;
}

VkBuffer TridentUniformBuffer::buffer(u32 frameIndex) const
{
    if (frameIndex >= m_buffers.size()) return VK_NULL_HANDLE;
    return m_buffers[frameIndex];
}

} // namespace mc::client::renderer::trident
