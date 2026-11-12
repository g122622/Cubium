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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/blocks/base/HorizontalBlock.hpp"
#include "common/world/block/capability/IWaterLoggable.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"

#include <memory>

namespace mc {

class Player;

namespace blocks {

/**
 * @brief 饰纹陶罐方块
 *
 * 由陶片合成的装饰性容器方块，可存放一个物品。
 * 状态属性：FACING, CRACKED, WATERLOGGED
 */
class DecoratedPotBlock : public HorizontalBlock, public IWaterLoggable {
public:
    explicit DecoratedPotBlock(const BlockProperties& properties);

    ~DecoratedPotBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 恒不可通行
     *
     * 原版 `DecoratedPotBlock#isPathfindable` 显式返回 false：陶罐碰撞形状比满格矮，
     * 默认「碰撞不完整即可通行」的判据会让生物把罐子当可走的台阶踩上去。
     */
    [[nodiscard]] bool isPathfindable(const BlockState& state, PathComputationType type) const override;

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Destroy;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /**
     * @brief 饰纹陶罐的声音组随 `CRACKED` 状态切换。
     *
     * 对齐原版 `DecoratedPotBlock#getSoundType`：陶罐被投射物打破后进入 `CRACKED` 状态，
     * 此后的破坏声改为碎裂声（`block.decorated_pot.shatter`），其余四种不变。
     */
    [[nodiscard]] const BlockSoundType& getSoundType(const BlockState& state) const noexcept override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockEntityType getBlockEntityType() const noexcept
    {
        return BlockEntityType::DecoratedPot;
    }

    // ========== 红石比较器 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    /**
     * @brief 玩家右键交互
     *
     * 手持物品时：向陶罐中放入1个物品（如果可以放入），
     * 触发正摇晃(Positive wobble)动画和插入音效。
     * 空手时：触发负摇晃(Negative wobble)动画和插入失败音效。
     */
    [[nodiscard]] BlockActionResult useItemOn(ItemStack& stack,
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    [[nodiscard]] BlockActionResult useWithoutItem(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 玩家破坏前处理
     *
     * 如果玩家手持的物品具有 BREAKS_DECORATED_POTS 标签
     * （如剑、斧等工具），且没有精准采集附魔，
     * 则将陶罐设为 CRACKED 状态。
     * CRACKED 状态的陶罐被破坏时会掉落4个单独的陶片而非陶罐物品。
     */
    void playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player) override;

    /**
     * @brief 投射物命中处理
     *
     * 投射物命中陶罐时，总是将陶罐设为 CRACKED 状态然后破坏。
     * 投射物破坏的陶罐总是碎裂为陶片，不受精准采集保护。
     */
    void onProjectileHit(
        IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile) override;

    /**
     * @brief 方块移除时处理
     *
     * 掉落陶罐内存储的物品，并触发容器邻居更新。
     * 陶罐物品本身由战利品表系统处理掉落。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    /**
     * @brief 中键选取物品（创造模式）
     *
     * 返回带有陶罐图案数据的物品堆，保留 sherds 信息。
     * 不包含罐内存储的物品。
     */
    [[nodiscard]] ItemStack getCloneItemStack(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
