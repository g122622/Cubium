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
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"

namespace mc::particle {
enum class ParticleTypeId : u16;
}

namespace mc {
namespace blocks {

/**
 * @brief 火把方块
 *
 * 可放置在地面上的火把，发光等级14，生成火焰和烟雾粒子。
 * 必须放置在具有坚固上表面的方块上方。
 *
 * 参考: net.minecraft.world.level.block.TorchBlock
 */
class TorchBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param flameParticle 火焰粒子类型（普通火把为 Flame，灵魂火把为 SoulFireFlame）
     */
    explicit TorchBlock(const BlockProperties& properties, particle::ParticleTypeId flameParticle);

    ~TorchBlock() override = default;

    // ========== Block 接口实现 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 检查是否可以放置在指定位置
     *
     * 下方方块必须有坚固的上表面。
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居方块更新时检查支撑
     *
     * 当下方方块变化时，如果不再满足放置条件则变为空气。
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
     * 生成火焰粒子和烟雾粒子。
     */
    void animateTick(IBlockAnimateContext& context,
        const BlockPos& pos,
        const BlockState& state,
        math::IRandom& random) const override;

protected:
    /// 碰撞形状
    CollisionShape m_shape;

    /// 火焰粒子类型
    particle::ParticleTypeId m_flameParticle;

private:
    /**
     * @brief 检查火把是否可以在指定位置存活（IWorld版本）
     *
     * 供updatePostPlacement使用，避免IWorld到IBlockReader的向下转型。
     */
    [[nodiscard]] bool _canSurvive(IWorld& world, const BlockPos& pos) const;
};

} // namespace blocks
} // namespace mc
