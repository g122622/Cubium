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

#include "common/core/Types.hpp"

namespace mc::client::renderer::api {

// ============================================================================
// 缓冲区类型枚举
// ============================================================================

/**
 * @brief 缓冲区用途类型
 */
enum class BufferUsage : u8 {
    Vertex,  // 顶点缓冲区
    Index,   // 索引缓冲区
    Uniform, // Uniform 缓冲区
    Staging, // 暂存缓冲区
    Storage  // 存储/SSBO 缓冲区
};

/**
 * @brief 内存类型
 */
enum class MemoryType : u8 {
    DeviceLocal, // 仅 GPU 可访问，性能最优
    HostVisible, // CPU 可访问
    HostCoherent // CPU 可访问，无需手动刷新
};

/**
 * @brief 索引类型
 */
enum class IndexType : u8 {
    U16, // 16位索引
    U32  // 32位索引
};

} // namespace mc::client::renderer::api
