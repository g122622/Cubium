#pragma once

#include "../../util/assert/AssertAll.hpp"
#include "../../util/property/Properties.hpp"
#include "../IWorld.hpp"
#include "../fluid/Fluid.hpp"
#include "../fluid/FluidRegistry.hpp"
#include "../fluid/FluidTags.hpp"
#include "../tick/manager/TickManager.hpp"

namespace mc {
namespace waterloggable {

/**
 * @brief 获取水流体实例（带缓存）
 * @return 水流体指针，如果未初始化返回 nullptr
 */
[[nodiscard]] inline fluid::Fluid* getWaterFluid()
{
    // 使用静态缓存避免重复的注册表查找
    static fluid::Fluid* s_waterFluid = nullptr;
    if (s_waterFluid == nullptr) {
        s_waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    }
    return s_waterFluid;
}

/**
 * @brief 检查流体状态是否为水
 * @param fluidState 流体状态指针
 * @return 如果是水返回 true
 */
[[nodiscard]] inline bool isWaterFluidState(const fluid::FluidState* fluidState)
{
    return fluidState != nullptr && !fluidState->isEmpty() && fluidState->getFluid().isIn(fluid::FluidTags::WATER());
}

/**
 * @brief 检查流体是否为水
 * @param fluid 流体引用
 * @return 如果是水返回 true
 */
[[nodiscard]] inline bool isWaterFluid(const fluid::Fluid& fluid)
{
    return fluid.isIn(fluid::FluidTags::WATER());
}

/**
 * @brief 检查流体状态是否为水源
 * @param fluidState 流体状态指针
 * @return 如果是水源返回 true
 */
[[nodiscard]] inline bool isWaterSourceFluidState(const fluid::FluidState* fluidState)
{
    return isWaterFluidState(fluidState) && fluidState->isSource();
}

/**
 * @brief 调度水流体的 tick
 * @param world 世界引用
 * @param pos 方块位置
 */
inline void scheduleWaterTick(IWorld& world, const BlockPos& pos)
{
    fluid::Fluid* waterFluid = getWaterFluid();
    MC_ASSERT(waterFluid != nullptr);
    world.tickManager().scheduleFluidTick(pos, *waterFluid, waterFluid->getTickDelay(world));
}

/**
 * @brief 获取水流体状态（用于 getFluidState 实现）
 * @param state 方块状态
 * @return 如果含水返回水的默认状态，否则返回 nullptr
 */
[[nodiscard]] inline const fluid::FluidState* getWaterFluidState(const BlockState& state)
{
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        fluid::Fluid* waterFluid = getWaterFluid();
        if (waterFluid != nullptr) {
            return &waterFluid->defaultState();
        }
    }
    return nullptr;
}

/**
 * @brief 检测放置位置是否应该含水（用于 getStateForPlacement）
 *
 * 根据 MC 1.16.5 原版行为，放置含水方块时只检测水源，不检测流动水。
 * 这是因为 MC 源码中使用 fluidstate.getFluid() == Fluids.WATER，
 * 而 Fluids.WATER 只匹配静止水源，不匹配 Fluids.FLOWING_WATER。
 *
 * @param world 世界引用
 * @param pos 方块位置
 * @return 如果该位置有水源返回 true
 */
[[nodiscard]] inline bool shouldWaterlogAt(const IWorld& world, const BlockPos& pos)
{
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    return isWaterSourceFluidState(fluidState);
}

/**
 * @brief 检测放置位置是否有任何水（包括流动水）
 *
 * 用于检测附近是否有水的场景，如珊瑚检测周围水源。
 *
 * @param world 世界引用
 * @param pos 方块位置
 * @return 如果该位置有任何水返回 true
 */
[[nodiscard]] inline bool hasAnyWaterAt(const IWorld& world, const BlockPos& pos)
{
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    return isWaterFluidState(fluidState);
}

} // namespace waterloggable
} // namespace mc
