#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 甘蔗方块
 *
 * 生长在水边，可以堆叠到3格高。
 * 用于制作糖和纸。
 *
 * 状态属性：
 * - AGE_0_15: 年龄（用于生长计时）
 *
 * 参考: net.minecraft.block.SugarCaneBlock
 */
class SugarCaneBlock : public Block {
public:
    explicit SugarCaneBlock(const BlockProperties& properties);
    ~SugarCaneBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getAge(const BlockState& state) const;
    [[nodiscard]] const BlockState& withAge(i32 age) const;

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
    /**
     * @brief 检查是否可以放置甘蔗（检查水）
     */
    [[nodiscard]] bool isNearWater(IBlockReader& world, const BlockPos& pos) const;

    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
