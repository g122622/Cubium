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

#include "../util/Direction.hpp"
#include "../util/math/Vector3.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"

namespace mc {

/**
 * @brief 射线检测结果类型
 */
enum class RaycastType : u8 {
    Miss, ///< 未击中任何方块
    Block ///< 击中方块
};

/**
 * @brief 方块射线检测结果
 *
 * 存储射线击中方块的详细信息。
 * 参考MC的BlockRayTraceResult。
 *
 * 使用示例:
 * @code
 * auto result = raycastBlocks(context, world);
 * if (result.isHit()) {
 *     BlockPos pos = result.blockPos();
 *     Direction face = result.face();
 *     // 放置方块到 result.adjacentPos()
 * }
 * @endcode
 */
class BlockRaycastResult {
public:
    /**
     * @brief 默认构造（miss结果）
     */
    BlockRaycastResult() = default;

    /**
     * @brief 构造hit结果
     */
    BlockRaycastResult(const Vector3& hitPos, const BlockPos& blockPos, Direction face, f32 distance)
        : m_type(RaycastType::Block)
        , m_hitPos(hitPos)
        , m_blockPos(blockPos)
        , m_face(face)
        , m_distance(distance)
    {}

    // ==================== 静态工厂方法 ====================

    /**
     * @brief 创建miss结果
     */
    [[nodiscard]] static BlockRaycastResult miss() { return BlockRaycastResult(); }

    /**
     * @brief 创建hit结果
     * @param hitPos 击中点（世界坐标）
     * @param blockPos 方块位置
     * @param face 击中的面
     * @param distance 从射线起点到击中点的距离
     */
    [[nodiscard]] static BlockRaycastResult hit(
        const Vector3& hitPos, const BlockPos& blockPos, Direction face, f32 distance)
    {
        return BlockRaycastResult(hitPos, blockPos, face, distance);
    }

    // ==================== 查询方法 ====================

    /**
     * @brief 获取结果类型
     */
    [[nodiscard]] RaycastType type() const { return m_type; }

    /**
     * @brief 是否未击中
     */
    [[nodiscard]] bool isMiss() const { return m_type == RaycastType::Miss; }

    /**
     * @brief 是否击中方块
     */
    [[nodiscard]] bool isHit() const { return m_type == RaycastType::Block; }

    /**
     * @brief 获取击中点（世界坐标）
     * @note 仅当isHit()为true时有效
     */
    [[nodiscard]] const Vector3& hitPosition() const { return m_hitPos; }

    /**
     * @brief 获取方块位置
     * @note 仅当isHit()为true时有效
     */
    [[nodiscard]] const BlockPos& blockPos() const { return m_blockPos; }

    /**
     * @brief 获取击中的面
     * @note 仅当isHit()为true时有效
     */
    [[nodiscard]] Direction face() const { return m_face; }

    /**
     * @brief 获取从射线起点到击中点的距离
     * @note 仅当isHit()为true时有效
     */
    [[nodiscard]] f32 distance() const { return m_distance; }

    /**
     * @brief 获取相邻方块位置（用于放置方块）
     *
     * 返回击中面相邻的方块位置，即如果玩家想在该面放置方块，
     * 方块应该放置在这个位置。
     *
     * @return 相邻方块的坐标
     */
    [[nodiscard]] BlockPos adjacentPos() const
    {
        return BlockPos(m_blockPos.x + Directions::xOffset(m_face),
            m_blockPos.y + Directions::yOffset(m_face),
            m_blockPos.z + Directions::zOffset(m_face));
    }

private:
    RaycastType m_type = RaycastType::Miss;
    Vector3 m_hitPos;
    BlockPos m_blockPos;
    Direction m_face = Direction::None;
    f32 m_distance = 0.0f;
};

} // namespace mc
