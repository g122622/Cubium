#pragma once

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 花盆方块
 *
 * 花盆是一种装饰性方块：
 * - 可以放置花卉、树苗、仙人掌等植物
 * - 无碰撞箱
 * - 可以放置在任何固体表面
 *
 * 参考: net.minecraft.block.FlowerPotBlock
 */
class FlowerPotBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param content 内容物方块ID（空为0）
     */
    FlowerPotBlock(const BlockProperties& properties, u32 content = 0);

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（花盆没有完整碰撞）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 放置检测 ==========

    /**
     * @brief 检查是否可以放置
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居更新
     */
    BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 内容物 ==========

    /**
     * @brief 获取内容物方块ID
     */
    [[nodiscard]] u32 getContent() const { return m_content; }

    /**
     * @brief 是否为空花盆
     */
    [[nodiscard]] bool isEmpty() const { return m_content == 0; }

protected:
    /// 内容物方块ID
    u32 m_content;
    /// 花盆形状
    CollisionShape m_shape;
    /// 碰撞形状
    CollisionShape m_collisionShape;
};

} // namespace blocks
} // namespace mc
