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
 * @brief 蛋糕方块
 *
 * 可食用的方块，可以被分成7片食用。
 * 每次食用消耗一片，最后一片吃完后方块消失。
 *
 * 状态属性：
 * - BITES_0_6: 已吃的片数 (0-6)
 *
 * 参考: net.minecraft.block.CakeBlock
 */
class CakeBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CakeBlock(const BlockProperties& properties);
    ~CakeBlock() override = default;

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

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

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

    // ========== 工具方法 ==========

    /**
     * @brief 获取已吃的片数
     */
    [[nodiscard]] static int getBites(const BlockState& state) {
        return state.get(BlockStateProperties::BITES_0_6());
    }

    /**
     * @brief 尝试吃蛋糕
     * @return 如果成功吃了返回true
     */
    static bool eatSlice(IWorld& world, const BlockPos& pos, BlockState& state);

protected:
    /// 各片数的形状缓存
    std::array<CollisionShape, 7> m_shapesByBites;
};

} // namespace blocks
} // namespace mc
