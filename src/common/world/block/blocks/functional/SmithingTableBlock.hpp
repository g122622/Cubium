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
 * @brief 锻造台方块
 *
 * 用于升级装备的功能方块，也是武器匠村民的工作站。
 *
 * 参考: net.minecraft.block.SmithingTableBlock
 */
class SmithingTableBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit SmithingTableBlock(const BlockProperties& properties);
    ~SmithingTableBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    /// 锻造台形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
