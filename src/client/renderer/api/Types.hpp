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
#include <array>
#include <vector>

namespace mc::client::renderer::api {

// ============================================================================
// 顶点格式
// ============================================================================

/**
 * @brief 顶点数据结构
 *
 * 包含位置、法线、纹理坐标、颜色和光照信息。
 * 与着色器中的顶点输入布局对应。
 */
struct Vertex {
    f64 x = 0.0f, y = 0.0f, z = 0.0f;    // 位置
    f64 nx = 0.0f, ny = 0.0f, nz = 0.0f; // 法线
    f64 u = 0.0f, v = 0.0f;              // 纹理坐标
    u32 color = 0xFFFFFFFF;              // 顶点颜色 (RGBA)
    u8 light = 255;                      // 光照 (R8_UNORM 编码，0-255)

    Vertex() = default;
    Vertex(f64 px, f64 py, f64 pz, f64 nu, f64 nv, f64 nw, f64 tu, f64 tv, u32 col = 0xFFFFFFFF, u8 l = 255)
        : x(px)
        , y(py)
        , z(pz)
        , nx(nu)
        , ny(nv)
        , nz(nw)
        , u(tu)
        , v(tv)
        , color(col)
        , light(l)
    {}

    /**
     * @brief 获取顶点格式的字节大小
     */
    static constexpr u64 stride() { return sizeof(Vertex); }
};

// ============================================================================
// 方块朝向
// ============================================================================

/**
 * @brief 方块面朝向枚举
 *
 * 定义方块六个面的方向，用于几何生成和面剔除。
 */
enum class Face : u8 {
    Bottom = 0, // Y- (下)
    Top = 1,    // Y+ (上)
    North = 2,  // Z- (北)
    South = 3,  // Z+ (南)
    West = 4,   // X- (西)
    East = 5,   // X+ (东)
    Count = 6
};

// ============================================================================
// 方块面顶点数据
// ============================================================================

namespace BlockGeometry {

// 每个面的顶点数
constexpr u32 VERTICES_PER_FACE = 4;

// 每个面的索引数 (两个三角形)
constexpr u32 INDICES_PER_FACE = 6;

/**
 * @brief 获取面的法线向量
 * @param face 面朝向
 * @return 法线向量 (3个分量: x, y, z)
 */
[[nodiscard]] std::array<f64, 3> getFaceNormal(Face face);

/**
 * @brief 获取面的顶点位置
 * @param face 面朝向
 * @return 4个顶点的位置，每个顶点3个分量 (共12个f64)
 */
[[nodiscard]] std::array<f64, 12> getFaceVertices(Face face);

/**
 * @brief 获取标准面的索引 (两个三角形)
 * @return 6个索引值，形成两个三角形
 */
[[nodiscard]] std::array<u32, 6> getFaceIndices();

/**
 * @brief 获取面的方向向量
 * @param face 面朝向
 * @return 方向向量 (3个整数分量，用于邻居检测)
 */
[[nodiscard]] std::array<i32, 3> getFaceDirection(Face face);

/**
 * @brief 检查面是否应该在给定朝向渲染
 * @param face 面朝向
 * @param neighborOpaque 邻居方块是否不透明
 * @return 如果应该渲染则返回 true
 */
[[nodiscard]] bool shouldRenderFace(Face face, bool neighborOpaque);

} // namespace BlockGeometry

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
