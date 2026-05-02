#pragma once

#include "../fluid/Fluid.hpp"
#include "../fluid/FluidTags.hpp"
#include "../fluid/FluidRegistry.hpp"
#include "../tick/manager/TickManager.hpp"
#include "../../util/property/Properties.hpp"
#include "../../util/assert/AssertAll.hpp"
#include "../IWorld.hpp"

namespace mc {
namespace waterloggable {

/**
 * @brief 检查流体状态是否为水
 * @param fluidState 流体状态指针
 * @return 如果是水返回 true
 */
[[nodiscard]] inline bool isWaterFluidState(const fluid::FluidState* fluidState) {
    return fluidState != nullptr && !fluidState->isEmpty() &&
           fluidState->getFluid().isIn(fluid::FluidTags::WATER());
}

/**
 * @brief 检查流体状态是否为水源
 * @param fluidState 流体状态指针
 * @return 如果是水源返回 true
 */
[[nodiscard]] inline bool isWaterSourceFluidState(const fluid::FluidState* fluidState) {
    return isWaterFluidState(fluidState) && fluidState->isSource();
}

/**
 * @brief 调度水流体的 tick
 * @param world 世界引用
 * @param pos 方块位置
 */
inline void scheduleWaterTick(IWorld& world, const BlockPos& pos) {
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    MC_ASSERT(waterFluid != nullptr);
    world.tickManager().scheduleFluidTick(pos, *waterFluid, waterFluid->getTickDelay(world));
}

/**
 * @brief 获取水流体状态（用于 getFluidState 实现）
 * @param state 方块状态
 * @return 如果含水返回水的默认状态，否则返回 nullptr
 */
[[nodiscard]] inline const fluid::FluidState* getWaterFluidState(const BlockState& state) {
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        if (waterFluid != nullptr) {
            return &waterFluid->defaultState();
        }
    }
    return nullptr;
}

/**
 * @brief 检测放置位置是否应该含水
 * @param world 世界引用
 * @param pos 方块位置
 * @return 如果该位置有水返回 true
 */
[[nodiscard]] inline bool shouldWaterlogAt(const IWorld& world, const BlockPos& pos) {
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    return isWaterFluidState(fluidState);
}

/**
 * @brief 检测放置位置是否为水源（用于 getStateForPlacement）
 * @param world 世界引用
 * @param pos 方块位置
 * @return 如果该位置有水源返回 true
 */
[[nodiscard]] inline bool hasWaterSourceAt(const IWorld& world, const BlockPos& pos) {
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    return isWaterSourceFluidState(fluidState);
}

} // namespace waterloggable
} // namespace mc
