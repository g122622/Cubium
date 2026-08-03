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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ON AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "../../Block.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 孢子花方块
 *
 * 一种悬挂在天花板上的装饰性植物，会向下滴落孢子粒子。
 * 只能放置在天花板下方（上方方块必须有向下的坚固面），且不能放置在水中。
 * 当上方支撑丢失时自动脱落变为空气。
 *
 * 参考: net.minecraft.world.level.block.SporeBlossomBlock (MC 1.21.11)
 */
class SporeBlossomBlock : public Block {
public:
    explicit SporeBlossomBlock(const BlockProperties& properties);

    ~SporeBlossomBlock() override = default;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 检查是否可以放置在指定位置
     *
     * MC 1.21.11: canSupportCenter(world, above, DOWN) && !isWaterAt(pos)
     * 上方方块必须有向下的坚固面（isSolidSide），且当前位置不在水中
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居方块更新时检查支撑
     *
     * MC 1.21.11: 当上方方块变化时，如果不再满足 canSurvive 则变为空气
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 客户端方块动画 tick
     *
     * MC 1.21.11: 生成两种粒子效果：
     * - falling_spore_blossom: 从花正下方掉落的绿色孢子粒子
     * - spore_blossom_air: 在花周围10格半径内漂浮的环境粒子（14次尝试）
     */
    void animateTick(IBlockAnimateContext& context,
        const BlockPos& pos,
        const BlockState& state,
        math::IRandom& random) const override;

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
