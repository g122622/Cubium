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
 * IMPLIED, INCLUDING BY NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
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
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/HorizontalBlock.hpp"
#include <memory>

namespace mc {

class BlockPos;
class BlockEntity;
namespace blocks {

/**
 * @brief 试炼刷怪笼方块
 *
 * 试炼密室核心方块，根据玩家战斗生成怪物。
 * 状态属性：TRIAL_SPAWNER_STATE, OMINOUS
 *
 * 参考: net.minecraft.block.TrialSpawnerBlock
 */
class TrialSpawnerBlock : public Block {
public:
    explicit TrialSpawnerBlock(const BlockProperties& properties);

    ~TrialSpawnerBlock() override = default;

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

/**
 * @brief 宝库方块
 *
 * 试炼密室的奖励容器，需要特定钥匙才能打开。
 * 状态属性：VAULT_STATE, FACING, OMINOUS
 *
 * 参考: net.minecraft.block.VaultBlock
 */
class VaultBlock : public HorizontalBlock {
public:
    explicit VaultBlock(const BlockProperties& properties);

    ~VaultBlock() override = default;

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

/**
 * @brief 自动合成器方块
 *
 * 可通过红石触发的自动合成方块。
 * 红石信号上升沿触发4 tick延时后执行合成，合成成功后CRAFTING属性置true，
 * CrafterBlockEntity 维护6 tick倒计时动画。
 * 状态属性：FACING, TRIGGERED, CRAFTING
 *
 * 参考: net.minecraft.block.CrafterBlock
 */
class CrafterBlock : public HorizontalBlock {
public:
    static constexpr i32 CRAFTING_TICK_DELAY = 4; ///< 红石触发到执行的延时（tick）

    explicit CrafterBlock(const BlockProperties& properties);

    ~CrafterBlock() override = default;

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override { return true; }

    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    /**
     * @brief 执行合成并射出结果
     *
     * 查找匹配配方，消耗原料，射出合成结果和剩余物品。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     */
    void _dispenseFrom(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 从合成器射出物品
     *
     * 将物品弹出到世界中（面朝方向偏移0.7格处）。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param facing 射出方向
     * @param stack 物品
     */
    static void _spawnItemEntity(IWorld& world, const BlockPos& pos, Direction facing, ItemStack stack);
};

} // namespace blocks
} // namespace mc
