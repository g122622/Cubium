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
 * @brief 下界传送门方块
 *
 * 下界传送门的紫色方块，玩家可以穿过传送到下界。
 *
 * 状态属性：
 * - HORIZONTAL_AXIS: 传送门轴向 (X 或 Z)
 *
 * MC ID: minecraft:nether_portal
 *
 * 参考: net.minecraft.block.NetherPortalBlock
 */
class NetherPortalBlock : public Block {
public:
    explicit NetherPortalBlock(const BlockProperties& properties);
    ~NetherPortalBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] Axis getAxis(const BlockState& state) const;

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

    // ========== 实体交互 ==========

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_xAxisShape;
    CollisionShape m_zAxisShape;
};

} // namespace blocks
} // namespace mc
