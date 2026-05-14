#pragma once

#include "IBucketPickupHandler.hpp"
#include "ILiquidContainer.hpp"

namespace mc {

// 前向声明
class BlockState;
namespace fluid {
class FluidRegistry;
}

/**
 * @brief 含水方块接口
 *
 * 实现此接口的方块可以容纳水（waterlogged）。
 * 继承 IBucketPickupHandler 和 ILiquidContainer，提供完整的含水功能。
 * 参考 MC 1.16.5 IWaterLoggable
 *
 * 注意：实现类必须：
 * 1. 添加 WATERLOGGED 属性
 * 2. 重写 getFluidState 方法
 * 3. 在 updatePostPlacement 中调度流体 tick
 * 4. 在 getStateForPlacement 中检测流体状态
 */
class IWaterLoggable : public IBucketPickupHandler, public ILiquidContainer {
public:
    ~IWaterLoggable() override = default;

    /**
     * @brief 检查方块是否含水
     *
     * 子类必须实现此方法，返回 WATERLOGGED 属性值
     *
     * @param state 方块状态
     * @return 是否含水
     */
    [[nodiscard]] virtual bool isWaterlogged(const BlockState& state) const = 0;

    /**
     * @brief 检查是否可以容纳指定流体
     *
     * 默认实现：只有当方块未含水且流体为水时返回 true
     * 子类可以重写此方法以提供更复杂的逻辑（如双层台阶不能含水）
     */
    [[nodiscard]] bool canContainFluid(
        IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::Fluid& fluid) const override;

    /**
     * @brief 接收流体
     *
     * 默认实现：设置 WATERLOGGED=true 并调度流体 tick
     * 子类可以重写此方法以提供特殊逻辑（如营火熄灭）
     */
    bool receiveFluid(
        IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::FluidState& fluidState) override;

    /**
     * @brief 从方块中取出流体
     *
     * 默认实现：设置 WATERLOGGED=false 并返回水
     */
    [[nodiscard]] fluid::Fluid* pickupFluid(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 检查是否包含流体
     */
    [[nodiscard]] bool containsFluid(IWorld& world, const BlockPos& pos, const BlockState& state) const override;
};

} // namespace mc
