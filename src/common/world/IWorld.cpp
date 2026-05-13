#include "IWorld.hpp"
#include "fluid/Fluid.hpp"
#include "fluid/FluidRegistry.hpp"
#include "block/Block.hpp"
#include "entity/core/Entity.hpp"
#include "redstone/RedstoneSystem.hpp"
#include "util/Direction.hpp"
#include "gamerule/GameRules.hpp"

namespace mc {

// 静态默认 GameRules 实例，用于 IWorld 默认实现
// 这避免了每个调用 getGameRules() 的地方都需要处理返回值问题
namespace {
world::gamerule::GameRules& getDefaultGameRules() {
    static world::gamerule::GameRules s_defaultRules;
    return s_defaultRules;
}
} // namespace

bool IWorld::hasFluid(i32 x, i32 y, i32 z) const {
    const fluid::FluidState* fluidState = getFluidState(x, y, z);
    return fluidState != nullptr && !fluidState->isEmpty();
}

bool IWorld::isWaterAt(i32 x, i32 y, i32 z) const {
    const fluid::FluidState* fluidState = getFluidState(x, y, z);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 检查是否为水（minecraft:water 或 minecraft:flowing_water）
    const fluid::Fluid& fluid = fluidState->getFluid();
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" &&
           (loc.path() == "water" || loc.path() == "flowing_water");
}

bool IWorld::isLavaAt(i32 x, i32 y, i32 z) const {
    const fluid::FluidState* fluidState = getFluidState(x, y, z);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 检查是否为岩浆（minecraft:lava 或 minecraft:flowing_lava）
    const fluid::Fluid& fluid = fluidState->getFluid();
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" &&
           (loc.path() == "lava" || loc.path() == "flowing_lava");
}

EntityId IWorld::spawnEntity(std::unique_ptr<Entity> entity) {
    (void)entity;
    // 默认实现：不支持生成实体
    // ServerWorld 会重写此方法
    return 0;
}

void IWorld::notifyNeighborChanged(const BlockPos& neighborPos,
                                    const BlockState& neighborState,
                                    Block& sourceBlock,
                                    const BlockPos& sourcePos,
                                    bool isMoving) {
    // 封装 const_cast，因为 neighborChanged 是非 const 成员函数
    // neighborChanged 需要调度 tick，因此不能是 const 方法
    const_cast<Block&>(neighborState.getBlock()).neighborChanged(
        *this, neighborPos, sourceBlock, sourcePos, isMoving);
}

void IWorld::updateNeighbors(const BlockPos& pos, Block& sourceBlock) {
    // MC 1.16.5: notifyNeighborsOfStateChange
    // 委托给 RedstoneSystem 实现
    world::redstone::RedstoneSystem::instance().updateNeighbors(*this, pos, sourceBlock);
}

void IWorld::updateNeighborsExcept(const BlockPos& pos, Block& sourceBlock, Direction except) {
    // MC 1.16.5: notifyNeighborsOfStateExcept
    // 委托给 RedstoneSystem 实现
    world::redstone::RedstoneSystem::instance().updateNeighborsExcept(*this, pos, sourceBlock, except);
}

const world::gamerule::GameRules& IWorld::getGameRules() const {
    // 默认实现返回静态默认规则
    // ServerWorld 会重写此方法返回实际的游戏规则
    return getDefaultGameRules();
}

world::gamerule::GameRules& IWorld::getGameRules() {
    // 默认实现返回静态默认规则
    return getDefaultGameRules();
}

} // namespace mc
