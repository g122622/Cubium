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
 * The above copyright notice and this permission shall be included in all
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

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 草径方块（土径）
 *
 * 由泥土用锹右键踩出的小路方块，顶部比完整方块矮 1 像素（15/16 格高）。
 * 渲染形状与碰撞形状均为 15/16 高。
 *
 * 上方存在固体方块（栅栏门除外）时无法存活：放置时直接改放泥土，
 * 运行中则安排 1 tick 的计划刻把自身转变为泥土（土径被掩埋后消失）。
 *
 * 参考: net.minecraft.block.DirtPathBlock
 */
class DirtPathBlock : public Block {
public:
    explicit DirtPathBlock(const BlockProperties& properties);

    ~DirtPathBlock() override = default;

    // ========== 放置和更新 ==========

    /**
     * @brief 计算放置状态
     *
     * 目标位置上方为固体方块（栅栏门除外）时无法存活，此时改放泥土
     * （pushEntitiesUp 会把嵌入新增碰撞形状的实体向上推出）。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查是否可在指定位置存活
     *
     * 上方为空气或非固体方块时返回 true；栅栏门例外（允许土径位于栅栏门下方）。
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居方块更新
     *
     * 上方方块变化且导致自身无法存活时，安排 1 tick 后的计划刻转变为泥土。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 计划刻：转变为泥土
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 形状 ==========

    /**
     * @brief 获取渲染形状（15/16 格高）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（15/16 格高，与渲染形状一致）
     *
     * 注意：与 FarmlandBlock 不同，FarmlandBlock 的碰撞形状是完整方块。
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 光照遮挡使用形状
     *
     * 土径高度不足一整格，须用形状而非整格判定遮挡。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override;

    // ========== 寻路 ==========

    /**
     * @brief 草径不可被路径寻找通过
     *
     * 草径碰撞箱比完整方块矮（15/16格高），导致默认的 allowsMovement
     * 会返回 true（非完整碰撞箱方块默认允许路径寻找通过）。因此必须
     * 显式重写返回 false，防止实体将草径视为可通过的路径。
     */
    [[nodiscard]] bool allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

private:
    /// 15/16 格高的形状（渲染与碰撞共用）
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
