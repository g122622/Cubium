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
 * @brief 磁石方块
 *
 * 可以让指南针指向它的功能方块。
 * 用于在下界和末地设置指南针指向。
 *
 * 参考: net.minecraft.block.LodestoneBlock
 */
class LodestoneBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit LodestoneBlock(const BlockProperties& properties);
    ~LodestoneBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    /// 磁石形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
