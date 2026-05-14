#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 切石机方块
 *
 * 用于将石材切割成各种形状的功能方块。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向 (NORTH, SOUTH, EAST, WEST)
 *
 * 参考: net.minecraft.block.StonecutterBlock
 */
class StonecutterBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit StonecutterBlock(const BlockProperties& properties);
    ~StonecutterBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    /// 切石机形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
