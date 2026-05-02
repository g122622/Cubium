#pragma once

#include "../../Block.hpp"
#include "../../IWaterLoggable.hpp"
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
 * - 实现 IWaterLoggable 接口支持含水功能
 *
 * 参考: net.minecraft.block.LanternBlock
 */
class LanternBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param lightValue 光照等级（默认15）
     */
    explicit LanternBlock(BlockProperties properties, u8 lightValue = 15);

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

    // ========== 光照 ==========

    /**
     * @brief 获取光照等级
     *
     * 普通灯笼始终发出15级光照。
     * 灵魂灯笼的光照等级在构造函数中通过属性设置。
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 光照等级
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr) const override {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return m_lightValue;
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
    /// 悬挂形状
    CollisionShape m_hangingShape;
    /// 站立形状
    CollisionShape m_standingShape;
    /// 光照等级
    u8 m_lightValue = 15;
};

} // namespace blocks
} // namespace mc
