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
 * @brief 制图台方块
 *
 * 用于复制、扩展和锁定地图的功能方块。
 *
 * 参考: net.minecraft.block.CartographyTableBlock
 */
class CartographyTableBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CartographyTableBlock(const BlockProperties& properties);
    ~CartographyTableBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    /// 制图台形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
