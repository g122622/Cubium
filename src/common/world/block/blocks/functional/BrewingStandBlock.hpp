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
 * @brief 酿造台方块
 *
 * 用于酿造药水的功能方块。
 * 具有三个药水瓶槽位，每个槽位都有独立的"是否有瓶子"状态。
 *
 * 状态属性：
 * - HAS_BOTTLE_0: 第一个槽位是否有瓶子
 * - HAS_BOTTLE_1: 第二个槽位是否有瓶子
 * - HAS_BOTTLE_2: 第三个槽位是否有瓶子
 *
 * 参考: net.minecraft.block.BrewingStandBlock
 */
class BrewingStandBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BrewingStandBlock(const BlockProperties& properties);
    ~BrewingStandBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    // ========== 红石 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] int getComparatorInputOverride(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos) const override;

protected:
    /// 酿造台形状
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
