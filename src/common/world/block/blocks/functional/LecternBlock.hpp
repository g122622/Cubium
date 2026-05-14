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
namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 讲台方块
 *
 * 用于放置和阅读书籍的方块。
 * 可以放置书和笔、成书，支持翻页。
 * 有书时可以发出红石信号。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向 (NORTH, SOUTH, EAST, WEST)
 * - POWERED: 是否发出红石信号
 * - HAS_BOOK: 是否有书
 *
 * 参考: net.minecraft.block.LecternBlock
 */
class LecternBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit LecternBlock(const BlockProperties& properties);
    ~LecternBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 红石 ==========

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const override;

    [[nodiscard]] i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const override;

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] int getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 工具方法 ==========

    /**
     * @brief 尝试放置书本
     */
    static bool tryPlaceBook(IWorld& world, const BlockPos& pos, BlockState& state, u32 itemId);

    /**
     * @brief 设置有书状态
     */
    static void setHasBook(IWorld& world, const BlockPos& pos, BlockState& state, bool hasBook);

    /**
     * @brief 发出红石脉冲
     */
    static void pulse(IWorld& world, const BlockPos& pos, BlockState& state);

protected:
    /// 各朝向的形状缓存
    std::array<CollisionShape, 6> m_shapesByFacing;

    /// 碰撞形状
    CollisionShape m_collisionShape;
};

} // namespace blocks
} // namespace mc
