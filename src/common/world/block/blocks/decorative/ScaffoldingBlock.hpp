#pragma once

#include "../../Block.hpp"
#include "../../IWaterLoggable.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 脚手架方块
 *
 * 脚手架是一种特殊的可攀爬方块：
 * - 可以堆叠放置
 * - 可以攀爬
 * - 玩家可以在上面行走
 * - 距离底部过远会掉落
 * - 实现 IWaterLoggable 接口支持含水功能
 *
 * 参考: net.minecraft.block.ScaffoldingBlock
 */
class ScaffoldingBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ScaffoldingBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 获取放置状态
     */
    BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 更新 ==========

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

    // ========== 放置检测 ==========

    /**
     * @brief 检查是否可以放置
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 攀爬 ==========

    /**
     * @brief 检查方块是否可攀爬
     *
     * 脚手架始终可攀爬。
     *
     * @return 始终返回 true
     */
    [[nodiscard]] bool isLadder(
        const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr,
        const Entity* entity = nullptr) const override {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(entity);
        MC_UNUSED(state);
        return true;
    }

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

protected:
    /// 底部形状（站立平台）
    CollisionShape m_baseShape;
    /// 完整形状（含支撑柱）
    CollisionShape m_fullShape;

    /**
     * @brief 检查是否有支撑
     */
    [[nodiscard]] bool hasSupport(IBlockReader& world, const BlockPos& pos) const;
};

} // namespace blocks
} // namespace mc
