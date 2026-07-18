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
#include "common/profiler/TraceEvents.hpp"
#include <array>
#include <vector>

namespace mc {

#ifdef __APPLE__
using VertexScalar = f32;
#else
using VertexScalar = f64;
#endif

// ============================================================================
// 顶点格式
// ============================================================================

struct Vertex {
    VertexScalar x = 0.0, y = 0.0, z = 0.0;    // 位置
    VertexScalar nx = 0.0, ny = 0.0, nz = 0.0; // 法线
    VertexScalar u = 0.0, v = 0.0;             // 纹理坐标
    u32 color = 0xFFFFFFFF;                    // 顶点颜色 (RGBA)
    u8 light = 255;                            // 光照 (R8_UNORM 编码，0-255)

    Vertex() = default;
    Vertex(f64 px, f64 py, f64 pz, f64 nu, f64 nv, f64 nw, f64 tu, f64 tv, u32 col, u8 l)
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
};

// ============================================================================
// 方块朝向
// ============================================================================

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

// 获取面的法线
[[nodiscard]] std::array<f64, 3> getFaceNormal(Face face);

// 获取面的顶点位置 (相对于方块左下角)
// 返回4个顶点的位置，每个顶点3个分量
[[nodiscard]] std::array<f64, 12> getFaceVertices(Face face);

// 获取标准面的索引 (两个三角形)
[[nodiscard]] std::array<u32, 6> getFaceIndices();

// 获取面的方向向量 (用于邻居检测)
[[nodiscard]] std::array<i32, 3> getFaceDirection(Face face);

// 检查面是否应该在给定朝向渲染
[[nodiscard]] bool shouldRenderFace(Face face, bool neighborOpaque);

} // namespace BlockGeometry

// ============================================================================
// 网格数据
// ============================================================================

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    void clear()
    {
        MC_TRACE_MEM_FREE("ChunkMesh", vertices.data());
        MC_TRACE_MEM_FREE("ChunkMesh", indices.data());
        vertices.clear();
        indices.clear();
    }

    void reserve(size_t vertexCount, size_t indexCount)
    {
        vertices.reserve(vertexCount);
        indices.reserve(indexCount);
        // reserve 后向量可能 realloc，用当前 data() 标记此次分配快照。
        // vector 内部多次 realloc 会使旧指针失效，此处为粗粒度标记，足以观察池整体趋势。
        MC_TRACE_MEM_ALLOC("ChunkMesh", vertices.data(), vertices.capacity() * sizeof(Vertex));
        MC_TRACE_MEM_ALLOC("ChunkMesh", indices.data(), indices.capacity() * sizeof(u32));
    }

    [[nodiscard]] bool empty() const { return vertices.empty(); }

    [[nodiscard]] size_t vertexCount() const { return vertices.size(); }

    [[nodiscard]] size_t indexCount() const { return indices.size(); }

    // 添加一个面 (4个顶点 + 6个索引)
    void addFace(const std::array<Vertex, 4>& faceVertices, u32 baseIndex);
};

// ============================================================================
// 纹理坐标
// ============================================================================

struct TextureRegion {
    f64 u0, v0; // 左上角
    f64 u1, v1; // 右下角

    TextureRegion() = default;
    TextureRegion(f64 u0_, f64 v0_, f64 u1_, f64 v1_)
        : u0(u0_)
        , v0(v0_)
        , u1(u1_)
        , v1(v1_)
    {}
};

// ============================================================================
// 纹理图集
// ============================================================================

class TextureAtlas {
public:
    TextureAtlas(u32 textureWidth, u32 textureHeight, u32 tileSize);

    // 获取纹理区域 (以格子坐标)
    [[nodiscard]] TextureRegion getRegion(u32 tileX, u32 tileY) const;

    // 获取纹理区域 (以线性索引)
    [[nodiscard]] TextureRegion getRegion(u32 tileIndex) const;

    // 获取图集尺寸
    [[nodiscard]] u32 textureWidth() const { return m_textureWidth; }
    [[nodiscard]] u32 textureHeight() const { return m_textureHeight; }
    [[nodiscard]] u32 tileSize() const { return m_tileSize; }
    [[nodiscard]] u32 tilesPerRow() const { return m_tilesPerRow; }

private:
    u32 m_textureWidth;
    u32 m_textureHeight;
    u32 m_tileSize;
    u32 m_tilesPerRow;
    f64 m_tileU;
    f64 m_tileV;
};

} // namespace mc
