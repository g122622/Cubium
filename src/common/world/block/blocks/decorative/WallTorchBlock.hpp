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

#include "TorchBlock.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 墙上的火把方块
 *
 * 附着在墙上的火把，继承 TorchBlock，增加水平朝向属性。
 * 火把朝向其附着的墙面的反方向。
 *
 * 参考: net.minecraft.world.level.block.WallTorchBlock
 */
class WallTorchBlock : public TorchBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param flameParticle 火焰粒子类型（普通火把为 Flame，灵魂火把为 SoulFireFlame）
     */
    explicit WallTorchBlock(const BlockProperties& properties, particle::ParticleTypeId flameParticle);

    ~WallTorchBlock() override = default;

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
     * 检查火把朝向的反方向（即附着面）是否有坚固的侧面。
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居方块更新时检查支撑
     *
     * 当附着面方块变化时，如果不再满足放置条件则变为空气。
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 获取放置状态
     *
     * 根据点击面确定火把朝向，若点击面不可用则尝试其他水平方向。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 客户端方块动画 tick
     *
     * 生成火焰粒子和烟雾粒子，位置根据朝向偏移。
     */
    void animateTick(IBlockAnimateContext& context,
        const BlockPos& pos,
        const BlockState& state,
        math::IRandom& random) const override;

    /**
     * @brief 旋转方块状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 墙火把特有方法 ==========

    /**
     * @brief 获取火把朝向
     * @param state 方块状态
     * @return 朝向方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 设置火把朝向
     * @param state 方块状态
     * @param facing 朝向
     * @return 更新后的方块状态
     */
    [[nodiscard]] static const BlockState& withFacing(const BlockState& state, Direction facing);

private:
    /**
     * @brief 检查是否可以放置在指定位置
     * @param world 世界引用
     * @param pos 火把位置
     * @param facing 火把朝向
     * @return 是否可以放置
     */
    [[nodiscard]] bool _canPlaceAt(IWorld& world, const BlockPos& pos, Direction facing) const;

    /// 各方向的碰撞形状
    CollisionShape m_northShape;
    CollisionShape m_southShape;
    CollisionShape m_westShape;
    CollisionShape m_eastShape;
};

} // namespace blocks
} // namespace mc
