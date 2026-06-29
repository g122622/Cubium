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

#include "../../IWaterLoggable.hpp"
#include "../FallingBlock.hpp"
#include "../HorizontalBlock.hpp"
#include "world/blockentity/BlockEntityType.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 雕纹书架方块
 *
 * 可放置6本书的书架，可被红石比较器检测。
 * 状态属性：FACING, SLOT_0_OCCUPIED ~ SLOT_5_OCCUPIED
 *
 * 参考: net.minecraft.block.ChiseledBookShelfBlock
 */
class ChiseledBookshelfBlock : public HorizontalBlock {
public:
    explicit ChiseledBookshelfBlock(const BlockProperties& properties);

    ~ChiseledBookshelfBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

/**
 * @brief 饰纹陶罐方块
 *
 * 由陶片合成的装饰性容器方块，可存放一个物品。
 * 状态属性：FACING, CRACKED, WATERLOGGED
 *
 * 参考: net.minecraft.block.DecoratedPotBlock
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

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

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

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockEntityType getBlockEntityType() const noexcept { return BlockEntityType::DecoratedPot; }

    // ========== 红石比较器 ==========

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
    CollisionShape m_shape;
};

/**
 * @brief 可刷方块（可疑沙/可疑沙砾）
 *
 * 可被刷子刷出考古物品的方块。受重力影响。
 * 状态属性：DUSTED (0-3)
 *
 * 参考: net.minecraft.block.BrushableBlock
 */
class BrushableBlock : public FallingBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BrushableBlock(const BlockProperties& properties);

    ~BrushableBlock() override = default;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

/**
 * @brief 嗅探兽蛋方块
 *
 * 可孵化出嗅探兽生物的蛋方块。
 * 状态属性：HATCH (0-2)
 *
 * 参考: net.minecraft.block.SnifferEggBlock
 */
class SnifferEggBlock : public Block {
public:
    explicit SnifferEggBlock(const BlockProperties& properties);

    ~SnifferEggBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 随机Tick - 孵化进度
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_noCrackShape;
    CollisionShape m_crackedShape;
    CollisionShape m_hatchingShape;
};

} // namespace blocks
} // namespace mc
