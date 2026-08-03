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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/PlantType.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/// 竹子最大生长高度（格）
inline constexpr i32 BAMBOO_MAX_HEIGHT = 16;

/**
 * @brief 竹子方块
 *
 * 高大的竹子植物，可以生长到16格高。
 * 使用 AGE、BAMBOO_LEAVES 和 STAGE 属性。
 */
class BambooBlock : public Block, public IGrowable, public IPlantable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BambooBlock(const BlockProperties& properties);

    ~BambooBlock() noexcept override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取 AGE 属性 (0-1)
     */
    static const IntegerProperty& AGE() { return BlockStateProperties::AGE_0_1(); }

    /**
     * @brief 获取 STAGE 属性 (0-1)
     */
    static const IntegerProperty& STAGE() { return BlockStateProperties::STAGE_0_1(); }

    /**
     * @brief 获取 BAMBOO_LEAVES 属性
     */
    static const EnumProperty<BlockStateProperties::BambooLeaves>& BAMBOO_LEAVES_PROP();

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== IGrowable 接口 ==========

    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== IPlantable 接口 ==========

    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;
    [[nodiscard]] const BlockState& getPlant(IBlockReader& world, const BlockPos& pos) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const noexcept override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const noexcept override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 其他 ==========

    /**
     * @brief 检查是否可以传播天光
     */
    [[nodiscard]] bool propagatesSkylightDown(
        const BlockState& state, IWorld* world, const BlockPos* pos) const noexcept override;

private:
    /// 正常形状
    CollisionShape m_normalShape;

    /// 大叶子形状
    CollisionShape m_largeLeavesShape;

    /// 碰撞形状
    CollisionShape m_collisionShape;

    /**
     * @brief 获取下方连续竹子数量
     */
    [[nodiscard]] i32 _getNumBambooBlocksBelow(IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 生长竹子
     */
    void _growBamboo(
        const BlockState& currentState, IWorld& world, const BlockPos& pos, math::IRandom& random, i32 bambooHeight);
};

/**
 * @brief 竹子幼苗方块
 *
 * 竹子的幼苗形态，可以生长成竹子。
 */
class BambooSaplingBlock : public Block, public IGrowable, public IPlantable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BambooSaplingBlock(const BlockProperties& properties);

    ~BambooSaplingBlock() noexcept override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== IGrowable 接口 ==========

    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== IPlantable 接口 ==========

    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;
    [[nodiscard]] const BlockState& getPlant(IBlockReader& world, const BlockPos& pos) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const noexcept override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_shape;

    /**
     * @brief 生长成竹子
     */
    void _growBamboo(IWorld& world, const BlockPos& pos);
};

} // namespace blocks
} // namespace mc
