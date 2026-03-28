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
 * @brief 制箭台方块
 *
 * 用于制作箭矢的功能方块，也是制箭师村民的工作站。
 *
 * 参考: net.minecraft.block.FletchingTableBlock
 */
class FletchingTableBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit FletchingTableBlock(const BlockProperties& properties);
    ~FletchingTableBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    /// 制箭台形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
