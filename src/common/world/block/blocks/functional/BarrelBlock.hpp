#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockEntity;

namespace blocks {

/**
 * @brief 木桶方块
 *
 * 类似箱子的存储方块，可以朝任意方向放置。
 *
 * 状态属性：
 * - FACING: 朝向 (6个方向)
 * - OPEN: 是否打开
 *
 * 参考: net.minecraft.block.BarrelBlock
 */
class BarrelBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BarrelBlock(const BlockProperties& properties);
    ~BarrelBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 红石 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] int getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

protected:
    /// 木桶形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
