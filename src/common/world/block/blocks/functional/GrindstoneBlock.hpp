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
 * @brief 砂轮方块
 *
 * 用于修复物品和移除附魔的功能方块。
 * 双方块结构，可以朝向四个方向。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向 (NORTH, SOUTH, EAST, WEST)
 *
 * 参考: net.minecraft.block.GrindstoneBlock
 */
class GrindstoneBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit GrindstoneBlock(const BlockProperties& properties);
    ~GrindstoneBlock() override = default;

    // ========== 状态属性 ==========

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

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

protected:
    /// 各朝向的形状缓存
    std::array<CollisionShape, 6> m_shapesByFacing;

    /// 碰撞形状（北朝向）
    CollisionShape m_collisionShape;
};

} // namespace blocks
} // namespace mc
