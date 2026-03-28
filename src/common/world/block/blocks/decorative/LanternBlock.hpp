#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 灯笼方块
 *
 * 灯笼是一种光源方块：
 * - 可以放置在地上或悬挂在天花板上
 * - 光照等级15（普通灯笼）或10（灵魂灯笼）
 * - 水中不可放置
 *
 * 参考: net.minecraft.block.LanternBlock
 */
class LanternBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit LanternBlock(const BlockProperties& properties);

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
        const BlockPos& pos) const;

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

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    /// 悬挂形状
    CollisionShape m_hangingShape;
    /// 站立形状
    CollisionShape m_standingShape;
    /// 光照等级
    u8 m_lightValue = 15;
};

} // namespace blocks
} // namespace mc
