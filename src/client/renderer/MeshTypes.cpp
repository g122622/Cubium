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

#include "MeshTypes.hpp"
#include "common/core/Types.hpp"
#include <array>

namespace mc {

// ============================================================================
// BlockGeometry 实现
// ============================================================================

namespace BlockGeometry {

std::array<f64, 3> getFaceNormal(Face face)
{
    switch (face) {
        case Face::Bottom:
            return {0.0, -1.0, 0.0};
        case Face::Top:
            return {0.0, 1.0, 0.0};
        case Face::North:
            return {0.0, 0.0, -1.0};
        case Face::South:
            return {0.0, 0.0, 1.0};
        case Face::West:
            return {-1.0, 0.0, 0.0};
        case Face::East:
            return {1.0, 0.0, 0.0};
        default:
            return {0.0, 0.0, 0.0};
    }
}

std::array<f64, 12> getFaceVertices(Face face)
{
    // 顶点按逆时针顺序排列 (从面外侧看)
    // 方块范围为 [0,0,0] 到 [1,1,1]
    switch (face) {
        case Face::Bottom: // Y- (下)
            return {0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0};
        case Face::Top: // Y+ (上)
            return {0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0};
        case Face::North: // Z- (北)
            return {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0};
        case Face::South: // Z+ (南)
            return {0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 1.0};
        case Face::West: // X- (西)
            return {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0};
        case Face::East: // X+ (东)
            return {1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 1.0};
        default:
            return {};
    }
}

std::array<u16, 6> getFaceIndices()
{
    // 两个三角形组成一个四边形
    // 顶点顺序: 0-1-2, 0-2-3
    return {0, 1, 2, 0, 2, 3};
}

std::array<i32, 3> getFaceDirection(Face face)
{
    switch (face) {
        case Face::Bottom:
            return {0, -1, 0};
        case Face::Top:
            return {0, 1, 0};
        case Face::North:
            return {0, 0, -1};
        case Face::South:
            return {0, 0, 1};
        case Face::West:
            return {-1, 0, 0};
        case Face::East:
            return {1, 0, 0};
        default:
            return {0, 0, 0};
    }
}

bool shouldRenderFace(Face face, bool neighborOpaque)
{
    // 如果邻居不透明，不渲染该面
    (void)face;
    return !neighborOpaque;
}

} // namespace BlockGeometry

// ============================================================================
// MeshData 实现
// ============================================================================

void MeshData::addFace(const std::array<Vertex, 4>& faceVertices, u16 baseIndex)
{
    // 添加顶点
    for (const auto& v : faceVertices) {
        vertices.push_back(v);
    }

    // 添加索引
    auto faceIndices = BlockGeometry::getFaceIndices();
    for (u16 idx : faceIndices) {
        indices.push_back(static_cast<u16>(baseIndex + idx));
    }
}

// ============================================================================
// TextureAtlas 实现
// ============================================================================

TextureAtlas::TextureAtlas(u32 textureWidth, u32 textureHeight, u32 tileSize)
    : m_textureWidth(textureWidth)
    , m_textureHeight(textureHeight)
    , m_tileSize(tileSize)
    , m_tilesPerRow(textureWidth / tileSize)
    , m_tileU(1.0 / static_cast<f64>(textureWidth / tileSize))
    , m_tileV(1.0 / static_cast<f64>(textureHeight / tileSize))
{}

TextureRegion TextureAtlas::getRegion(u32 tileX, u32 tileY) const
{
    f64 u0 = static_cast<f64>(tileX) * m_tileU;
    f64 v0 = static_cast<f64>(tileY) * m_tileV;
    return TextureRegion(u0, v0, u0 + m_tileU, v0 + m_tileV);
}

TextureRegion TextureAtlas::getRegion(u32 tileIndex) const
{
    u32 tileX = tileIndex % m_tilesPerRow;
    u32 tileY = tileIndex / m_tilesPerRow;
    return getRegion(tileX, tileY);
}

} // namespace mc
