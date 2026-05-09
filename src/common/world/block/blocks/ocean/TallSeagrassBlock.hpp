#pragma once

#include "../../Block.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 高海草方块
 *
 * 高度为2格的海草，使用 DOUBLE_BLOCK_HALF 属性区分上下半部分。
 *
 * ## 状态属性
 * - HALF: DoubleBlockHalf (UPPER, LOWER)
 * - WATERLOGGED: 是否含水
 *
 * ## 特性
 * - 双格水下植物
 * - 下半部分需要固体支撑
 * - 上半部分需要连接到下半部分
 * - 掉落物为海草物品（非自身）
 *
 * 参考: net.minecraft.block.TallSeaGrassBlock
 */
class TallSeagrassBlock : public Block {
public:
    explicit TallSeagrassBlock(const BlockProperties& properties);
    ~TallSeagrassBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取方块半部
     * @param state 方块状态
     * @return DoubleBlockHalf 上半部或下半部
     */
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

} // namespace blocks
} // namespace mc
