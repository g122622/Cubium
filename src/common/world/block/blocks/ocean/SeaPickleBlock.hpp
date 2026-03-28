#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 海泡菜方块
 *
 * 可放置在水下的发光方块，可以堆叠最多4个。
 * 放置在水中的海泡菜会发光。
 *
 * 状态属性：
 * - PICKLES_1_4: 海泡菜数量 (1-4)
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.SeaPickleBlock
 */
class SeaPickleBlock : public Block {
public:
    explicit SeaPickleBlock(const BlockProperties& properties);
    ~SeaPickleBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getPickles(const BlockState& state) const;
    [[nodiscard]] BlockState withPickles(i32 count) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 光照 ==========

    [[nodiscard]] u8 getLightLevel(const BlockState& state) const;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    /// 各数量的形状
    std::array<CollisionShape, 4> m_shapesByCount;
};

/**
 * @brief 海带方块
 *
 * 水下生长的植物，可以堆叠到很高。
 * 从海带方块可以获得海带物品。
 *
 * 状态属性：
 * - AGE_0_25: 年龄 (0-25)
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.KelpBlock
 */
class KelpBlock : public Block {
public:
    explicit KelpBlock(const BlockProperties& properties);
    ~KelpBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getAge(const BlockState& state) const;
    [[nodiscard]] BlockState withAge(i32 age) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_shape;
};

/**
 * @brief 海草方块
 *
 * 水下植物，可以放置在水下地面上。
 * 有两种高度：普通海草和高海草。
 *
 * 状态属性：
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.SeagrassBlock
 */
class SeagrassBlock : public Block {
public:
    explicit SeagrassBlock(const BlockProperties& properties);
    ~SeagrassBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_shape;
};

/**
 * @brief 高海草方块
 *
 * 高度为2格的海草，使用 DOUBLE_BLOCK_HALF 属性。
 *
 * 状态属性：
 * - HALF: DoubleBlockHalf (UPPER, LOWER)
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.TallSeagrassBlock
 */
class TallSeagrassBlock : public Block {
public:
    explicit TallSeagrassBlock(const BlockProperties& properties);
    ~TallSeagrassBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockStateProperties::DoubleBlockHalf getHalf(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_lowerShape;
    CollisionShape m_upperShape;
};

/**
 * @brief 气泡柱方块
 *
 * 由岩浆块或灵魂沙产生的水下气泡柱。
 * 可以推动实体向上或向下。
 *
 * 状态属性：
 * - DRAG: 是否为下拖（灵魂沙产生向上气泡，岩浆块产生向下气泡）
 *
 * 参考: net.minecraft.block.BubbleColumnBlock
 */
class BubbleColumnBlock : public Block {
public:
    explicit BubbleColumnBlock(const BlockProperties& properties);
    ~BubbleColumnBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] bool isDrag(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 实体交互 ==========

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    /**
     * @brief 检查下方是否产生气泡
     */
    [[nodiscard]] bool checkSource(const IWorld& world, const BlockPos& pos) const;
};

} // namespace blocks
} // namespace mc
