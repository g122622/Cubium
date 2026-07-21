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
#include "common/core/Result.hpp"

namespace mc::client::renderer::api {

/**
 * @brief 缓冲区接口
 *
 * 平台无关的缓冲区抽象接口。
 * 用于顶点、索引、Uniform等缓冲区的基础接口。
 */
class IBuffer {
public:
    virtual ~IBuffer() = default;

    /**
     * @brief 销毁缓冲区
     */
    virtual void destroy() = 0;

    /**
     * @brief 获取缓冲区大小（字节）
     */
    [[nodiscard]] virtual u64 size() const = 0;

    /**
     * @brief 获取缓冲区用途
     */
    [[nodiscard]] virtual BufferUsage usage() const = 0;

    /**
     * @brief 检查缓冲区是否有效
     */
    [[nodiscard]] virtual bool isValid() const = 0;

    /**
     * @brief 映射缓冲区到CPU内存
     * @return 映射后的指针，失败返回 nullptr
     */
    [[nodiscard]] virtual void* map() = 0;

    /**
     * @brief 取消映射
     */
    virtual void unmap() = 0;

    /**
     * @brief 上传数据到缓冲区
     * @param data 数据指针
     * @param size 数据大小
     * @param offset 缓冲区偏移量
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> upload(const void* data, u64 size, u64 offset = 0) = 0;

    /**
     * @brief 获取原生句柄
     * @return 平台特定的缓冲区句柄（Vulkan 为 VkBuffer）
     */
    [[nodiscard]] virtual void* nativeHandle() const = 0;
};

/**
 * @brief 顶点缓冲区接口
 */
class IVertexBuffer : public IBuffer {
public:
    /**
     * @brief 获取顶点数量
     */
    [[nodiscard]] virtual u32 vertexCount() const = 0;

    /**
     * @brief 获取顶点步长（字节）
     */
    [[nodiscard]] virtual u32 vertexStride() const = 0;

    /**
     * @brief 绑定到渲染管线
     */
    virtual void bind(void* commandBuffer) = 0;
};

/**
 * @brief 索引缓冲区接口
 */
class IIndexBuffer : public IBuffer {
public:
    /**
     * @brief 获取索引类型
     */
    [[nodiscard]] virtual IndexType indexType() const = 0;

    /**
     * @brief 获取索引数量
     */
    [[nodiscard]] virtual u32 indexCount() const = 0;

    /**
     * @brief 绑定到渲染管线
     */
    virtual void bind(void* commandBuffer) = 0;
};

/**
 * @brief Uniform 缓冲区接口
 */
class IUniformBuffer : public IBuffer {
public:
    /**
     * @brief 获取当前帧索引
     * 对于双缓冲/三缓冲 Uniform，返回当前帧的缓冲区索引
     */
    [[nodiscard]] virtual u32 currentFrameIndex() const = 0;

    /**
     * @brief 切换到下一帧
     * 对于多帧 Uniform 缓冲区，切换到下一个缓冲区
     */
    virtual void advanceFrame() = 0;

    /**
     * @brief 获取帧数量
     */
    [[nodiscard]] virtual u32 frameCount() const = 0;
};

} // namespace mc::client::renderer::api
