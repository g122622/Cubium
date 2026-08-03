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

#pragma once

#include "client/renderer/api/Types.hpp"
#include "client/renderer/api/buffer/IBuffer.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

// 前置声明
class TridentContext;

/**
 * @brief Vulkan 缓冲区基类
 *
 * 实现 api::IBuffer 接口，提供 Vulkan 缓冲区的通用功能。
 */
class TridentBuffer : public api::IBuffer {
public:
    TridentBuffer();
    ~TridentBuffer() override;

    // 禁止拷贝
    TridentBuffer(const TridentBuffer&) = delete;
    TridentBuffer& operator=(const TridentBuffer&) = delete;

    // 允许移动
    TridentBuffer(TridentBuffer&& other) noexcept;
    TridentBuffer& operator=(TridentBuffer&& other) noexcept;

    /**
     * @brief 创建缓冲区
     * @param context Trident 上下文
     * @param size 缓冲区大小
     * @param usage Vulkan 缓冲区用途标志
     * @param properties 内存属性
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> create(
        TridentContext* context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    // IBuffer 接口实现
    void destroy() override;
    [[nodiscard]] u64 size() const override { return m_size; }
    [[nodiscard]] api::BufferUsage usage() const override { return m_usage; }
    [[nodiscard]] bool isValid() const override { return m_buffer != VK_NULL_HANDLE; }
    [[nodiscard]] void* map() override;
    void unmap() override;
    [[nodiscard]] Result<void> upload(const void* data, u64 size, u64 offset = 0) override;
    [[nodiscard]] void* nativeHandle() const override { return m_buffer; }

    // Vulkan 特有访问器
    [[nodiscard]] VkBuffer buffer() const { return m_buffer; }
    [[nodiscard]] VkDeviceMemory memory() const { return m_memory; }

protected:
    TridentContext* m_context = nullptr;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;
    void* m_mapped = nullptr;
    api::BufferUsage m_usage = api::BufferUsage::Vertex;

    [[nodiscard]] Result<u32> findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
};

/**
 * @brief 顶点缓冲区
 */
class TridentVertexBuffer : public api::IVertexBuffer {
public:
    TridentVertexBuffer();
    ~TridentVertexBuffer() override;

    // 禁止拷贝
    TridentVertexBuffer(const TridentVertexBuffer&) = delete;
    TridentVertexBuffer& operator=(const TridentVertexBuffer&) = delete;

    // 允许移动
    TridentVertexBuffer(TridentVertexBuffer&& other) noexcept;
    TridentVertexBuffer& operator=(TridentVertexBuffer&& other) noexcept;

    /**
     * @brief 创建顶点缓冲区
     */
    [[nodiscard]] Result<void> create(TridentContext* context, u64 size, u32 vertexStride);

    // IBuffer 接口实现
    void destroy() override;
    [[nodiscard]] u64 size() const override { return m_size; }
    [[nodiscard]] api::BufferUsage usage() const override { return api::BufferUsage::Vertex; }
    [[nodiscard]] bool isValid() const override { return m_buffer != VK_NULL_HANDLE; }
    [[nodiscard]] void* map() override;
    void unmap() override;
    [[nodiscard]] Result<void> upload(const void* data, u64 size, u64 offset = 0) override;
    [[nodiscard]] void* nativeHandle() const override { return m_buffer; }

    // IVertexBuffer 接口实现
    [[nodiscard]] u32 vertexCount() const override;
    [[nodiscard]] u32 vertexStride() const override { return m_vertexStride; }
    void bind(void* commandBuffer) override;

    // Vulkan 特有访问器
    [[nodiscard]] VkBuffer buffer() const { return m_buffer; }

private:
    TridentContext* m_context = nullptr;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;
    u32 m_vertexStride = 0;
    void* m_mapped = nullptr;
};

/**
 * @brief 索引缓冲区
 */
class TridentIndexBuffer : public api::IIndexBuffer {
public:
    TridentIndexBuffer();
    ~TridentIndexBuffer() override;

    // 禁止拷贝
    TridentIndexBuffer(const TridentIndexBuffer&) = delete;
    TridentIndexBuffer& operator=(const TridentIndexBuffer&) = delete;

    // 允许移动
    TridentIndexBuffer(TridentIndexBuffer&& other) noexcept;
    TridentIndexBuffer& operator=(TridentIndexBuffer&& other) noexcept;

    /**
     * @brief 创建索引缓冲区
     */
    [[nodiscard]] Result<void> create(TridentContext* context, u64 size, api::IndexType type);

    // IBuffer 接口实现
    void destroy() override;
    [[nodiscard]] u64 size() const override { return m_size; }
    [[nodiscard]] api::BufferUsage usage() const override { return api::BufferUsage::Index; }
    [[nodiscard]] bool isValid() const override { return m_buffer != VK_NULL_HANDLE; }
    [[nodiscard]] void* map() override;
    void unmap() override;
    [[nodiscard]] Result<void> upload(const void* data, u64 size, u64 offset = 0) override;
    [[nodiscard]] void* nativeHandle() const override { return m_buffer; }

    // IIndexBuffer 接口实现
    [[nodiscard]] api::IndexType indexType() const override { return m_indexType; }
    [[nodiscard]] u32 indexCount() const override { return m_indexCount; }
    void bind(void* commandBuffer) override;

    // Vulkan 特有访问器
    [[nodiscard]] VkBuffer buffer() const { return m_buffer; }

private:
    TridentContext* m_context = nullptr;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;
    api::IndexType m_indexType = api::IndexType::U32;
    u32 m_indexCount = 0;
    void* m_mapped = nullptr;
};

/**
 * @brief Uniform 缓冲区
 *
 * 支持多帧轮换（双缓冲/三缓冲）。
 */
class TridentUniformBuffer : public api::IUniformBuffer {
public:
    TridentUniformBuffer();
    ~TridentUniformBuffer() override;

    // 禁止拷贝
    TridentUniformBuffer(const TridentUniformBuffer&) = delete;
    TridentUniformBuffer& operator=(const TridentUniformBuffer&) = delete;

    // 允许移动
    TridentUniformBuffer(TridentUniformBuffer&& other) noexcept;
    TridentUniformBuffer& operator=(TridentUniformBuffer&& other) noexcept;

    /**
     * @brief 创建 Uniform 缓冲区
     * @param context Trident 上下文
     * @param size 单个缓冲区大小
     * @param frameCount 帧数（用于多帧轮换）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> create(TridentContext* context, u64 size, u32 frameCount);

    // IBuffer 接口实现
    void destroy() override;
    [[nodiscard]] u64 size() const override { return m_size; }
    [[nodiscard]] api::BufferUsage usage() const override { return api::BufferUsage::Uniform; }
    [[nodiscard]] bool isValid() const override { return !m_buffers.empty() && m_buffers[0] != VK_NULL_HANDLE; }
    [[nodiscard]] void* map() override;
    void unmap() override;
    [[nodiscard]] Result<void> upload(const void* data, u64 size, u64 offset = 0) override;
    [[nodiscard]] void* nativeHandle() const override
    {
        return m_buffers.empty() ? VK_NULL_HANDLE : m_buffers[m_currentFrame];
    }

    // IUniformBuffer 接口实现
    [[nodiscard]] u32 currentFrameIndex() const override { return m_currentFrame; }
    void advanceFrame() override;
    [[nodiscard]] u32 frameCount() const override { return m_frameCount; }

    // Vulkan 特有访问器
    [[nodiscard]] VkBuffer buffer(u32 frameIndex) const;

private:
    TridentContext* m_context = nullptr;
    std::vector<VkBuffer> m_buffers;
    std::vector<VkDeviceMemory> m_memories;
    std::vector<void*> m_mapped;
    u64 m_size = 0;
    u32 m_frameCount = 2;
    u32 m_currentFrame = 0;
};

} // namespace mc::client::renderer::trident
