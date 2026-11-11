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
#include "common/world/block/blocks/base/HorizontalBlock.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"
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
 */
class TrialSpawnerBlock : public Block {
public:
    explicit TrialSpawnerBlock(const BlockProperties& properties);

    ~TrialSpawnerBlock() override = default;

    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    /**
     * @brief 发光等级（依赖试炼刷怪笼状态）
     *
     * 原版按状态分级：`waiting_for_players` 为 **4**，其余非 `inactive` 状态为 **8**，
     * `inactive` 为 0。此前用统一的 `lightLevel(8)`，导致待机状态亮了一倍。
     * 由差分能力 block_light_emission 逐状态实测。
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

/**
 * @brief 宝库方块
 *
 * 试炼密室的奖励容器，需要特定钥匙才能打开。
 * 状态属性：VAULT_STATE, FACING, OMINOUS
 *
 */
class VaultBlock : public HorizontalBlock {
public:
    explicit VaultBlock(const BlockProperties& properties);

    ~VaultBlock() override = default;

    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 发光等级（依赖宝库状态）
     *
     * 原版按状态分级：`inactive` 为 **6**，其余状态（`active` / `unlocking` / `ejecting`）
     * 为 **12**。此前是统一的 6，导致激活后的宝库不发亮。由差分能力
     * block_light_emission 逐状态实测。
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

/**
 * @brief 自动合成器方块
 *
 * 可通过红石触发的自动合成方块。
 * 红石信号上升沿触发4 tick延时后执行合成，合成成功后CRAFTING属性置true，
 * CrafterBlockEntity 维护6 tick倒计时动画。
 * 状态属性：ORIENTATION, TRIGGERED, CRAFTING
 *
 */
class CrafterBlock : public Block {
public:
    static constexpr i32 CRAFTING_TICK_DELAY = 4; ///< 红石触发到执行的延时（tick）

    explicit CrafterBlock(const BlockProperties& properties);

    ~CrafterBlock() override = default;

    [[nodiscard]] bool hasBlockEntity() const noexcept override
    {
        return true;
    }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // 注意：vanilla CrafterBlock 没有重写 isSignalSource——合成器不是红石信号源。
    // 它只重写 hasAnalogOutputSignal，把内部物品栏的填充率提供给比较器。

    [[nodiscard]] std::optional<ContainerType> getMenuProvider(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockActionResult useWithoutItem(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        const BlockRaycastResult& hit) override;

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept override;

    // 与读数成对：没有这个开关，比较器不会来问合成器要信号。
    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

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
