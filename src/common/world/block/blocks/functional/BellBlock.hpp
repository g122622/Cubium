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
 * @brief 钟方块
 *
 * 可以被敲响的功能方块，会发出声音和动画。
 * 可以附着在墙、地面或天花板上。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向 (NORTH, SOUTH, EAST, WEST) - 墙面附着
 * - ATTACHMENT: 附着类型 (FLOOR, CEILING, SINGLE_WALL, DOUBLE_WALL)
 * - POWERED: 是否被激活
 *
 * 参考: net.minecraft.block.BellBlock
 */
class BellBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BellBlock(const BlockProperties& properties);
    ~BellBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

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

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    // ========== 红石 ==========

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

protected:
    /// 各状态的形状缓存
    std::array<CollisionShape, 16> m_shapesByState;

    /// 地面附着形状
    CollisionShape m_floorShape;

    /// 天花板附着形状
    CollisionShape m_ceilingShape;

    /// 墙面附着形状
    CollisionShape m_wallShape;
};

} // namespace blocks
} // namespace mc
