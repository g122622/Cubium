#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 藤蔓方块
 *
 * 可以附着在墙面上的攀爬植物。
 * 玩家可以攀爬藤蔓。
 *
 * 状态属性：
 * - UP: 是否向上延伸
 * - NORTH/SOUTH/EAST/WEST: 各方向是否附着
 *
 * 参考: net.minecraft.block.VineBlock
 */
class VineBlock : public Block {
public:
    explicit VineBlock(const BlockProperties& properties);
    ~VineBlock() override = default;

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

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 攀爬 ==========

    /**
     * @brief 是否可以攀爬
     */
    [[nodiscard]] bool isLadder(const BlockState& state) const {
        MC_UNUSED(state);
        return true;
    }

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const override { return true; }

private:
    /**
     * @brief 检查是否可以附着到指定方向的方块
     */
    [[nodiscard]] bool canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const;

    /**
     * @brief 获取藤蔓连接数
     */
    [[nodiscard]] i32 getConnectionCount(const BlockState& state) const;

    /// 各方向的形状
    CollisionShape m_northShape;
    CollisionShape m_southShape;
    CollisionShape m_eastShape;
    CollisionShape m_westShape;
};

} // namespace blocks
} // namespace mc
