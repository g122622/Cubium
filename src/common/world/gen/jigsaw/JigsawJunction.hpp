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

#include "../../../core/Types.hpp"
#include "JigsawPiece.hpp"

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief Jigsaw 连接点交叉数据结构
 *
 * 用于记录两个拼图块连接点之间的地形高度关系。
 * 主要用于 TerrainMatching 放置行为的拼图块，确保地形高度正确衔接。
 */
class JigsawJunction {
public:
    /**
     * @brief 构造函数
     *
     * @param sourceX 源 X 坐标
     * @param sourceGroundY 源地面高度
     * @param sourceZ 源 Z 坐标
     * @param deltaY 高度偏移量（目标地面高度与源地面高度之差）
     * @param destProjection 目标放置行为
     */
    JigsawJunction(i32 sourceX, i32 sourceGroundY, i32 sourceZ, i32 deltaY, JigsawPlacementBehaviour destProjection)
        : m_sourceX(sourceX)
        , m_sourceGroundY(sourceGroundY)
        , m_sourceZ(sourceZ)
        , m_deltaY(deltaY)
        , m_destProjection(destProjection)
    {}

    /** @brief 获取源 X 坐标 */
    i32 getSourceX() const noexcept { return m_sourceX; }

    /** @brief 获取源地面高度 */
    i32 getSourceGroundY() const noexcept { return m_sourceGroundY; }

    /** @brief 获取源 Z 坐标 */
    i32 getSourceZ() const noexcept { return m_sourceZ; }

    /** @brief 获取高度偏移量 */
    i32 getDeltaY() const noexcept { return m_deltaY; }

    /** @brief 获取目标放置行为 */
    JigsawPlacementBehaviour getDestProjection() const noexcept { return m_destProjection; }

    /**
     * @brief 相等比较运算符
     *
     * 注意：比较时不包含 m_sourceGroundY，因为 MC 1.16.5 中 Junction 的相等性判断
     * 仅基于 X、Z 坐标、高度偏移和放置行为。
     */
    bool operator==(const JigsawJunction& other) const noexcept
    {
        return m_sourceX == other.m_sourceX && m_sourceZ == other.m_sourceZ && m_deltaY == other.m_deltaY &&
            m_destProjection == other.m_destProjection;
    }

    /** @brief 不相等比较运算符 */
    bool operator!=(const JigsawJunction& other) const noexcept { return !(*this == other); }

private:
    i32 m_sourceX;                             ///< 源 X 坐标
    i32 m_sourceGroundY;                       ///< 源地面高度
    i32 m_sourceZ;                             ///< 源 Z 坐标
    i32 m_deltaY;                              ///< 高度偏移量
    JigsawPlacementBehaviour m_destProjection; ///< 目标放置行为
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
