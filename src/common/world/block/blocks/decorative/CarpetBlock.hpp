#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 地毯方块
 *
 * 地毯是极薄的装饰性方块：
 * - 高度只有1/16格（1像素）
 * - 可放置在任何固体方块上
 * - 16种颜色
 * - 可以被活塞推动
 *
 * 参考: net.minecraft.block.CarpetBlock
 */
class CarpetBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CarpetBlock(const BlockProperties& properties);

    // ========== 形状 ==========

    /**
     * @brief 获取形状（用于渲染）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 放置检测 ==========

    /**
     * @brief 检查是否可以放置
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    /**
     * @brief 邻居更新
     */
    BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

protected:
    /// 地毯形状（高度1/16格）
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
