#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 梯子方块
 *
 * 梯子是一种可以攀爬的方块：
 * - 只能附在固体方块的侧面
 * - 水logged支持
 * - 可以攀爬
 * - 没有碰撞箱
 *
 * 参考: net.minecraft.block.LadderBlock
 */
class LadderBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit LadderBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 获取放置状态
     */
    BlockState getStateForPlacement(BlockItemUseContext& context) override;

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

    // ========== 旋转 ==========

    /**
     * @brief 旋转方块
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（梯子没有碰撞）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

protected:
    /// 各方向的形状
    CollisionShape m_shapes[6];
};

} // namespace blocks
} // namespace mc
