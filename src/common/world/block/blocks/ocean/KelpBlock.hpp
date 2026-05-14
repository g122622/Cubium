#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 海带方块
 *
 * 水下生长的植物，可以堆叠到很高。
 * 从海带方块可以获得海带物品。
 *
 * ## 状态属性
 * - AGE_0_25: 年龄 (0-25)，控制生长阶段
 * - WATERLOGGED: 是否含水
 *
 * ## 生长机制 (MC 1.16.5)
 * - 通过随机 tick 生长
 * - 高度限制基于 AGE_0_25 (最大 25 格)
 * - 只能在水中生长
 * - 生长概率约 14%
 *
 * 参考: net.minecraft.block.KelpBlock
 */
class KelpBlock : public Block {
public:
    explicit KelpBlock(const BlockProperties& properties);
    ~KelpBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取当前年龄
     * @param state 方块状态
     * @return i32 年龄值（0-25）
     */
    [[nodiscard]] i32 getAge(const BlockState& state) const;

    /**
     * @brief 设置年龄
     * @param age 年龄值（0-25）
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] BlockState withAge(i32 age) const;

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

    [[nodiscard]] bool ticksRandomly() const override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
