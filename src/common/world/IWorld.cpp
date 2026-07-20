/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "IWorld.hpp"
#include "block/Block.hpp"
#include "entity/core/Entity.hpp"
#include "fluid/Fluid.hpp"
#include "fluid/FluidRegistry.hpp"
#include "gamerule/GameRules.hpp"
#include "redstone/RedstoneSystem.hpp"
#include "util/Direction.hpp"
#include "util/math/MathUtils.hpp"

namespace mc {

// 静态默认 GameRules 实例，用于 IWorld 默认实现
// 这避免了每个调用 getGameRules() 的地方都需要处理返回值问题
namespace {
world::gamerule::GameRules& getDefaultGameRules()
{
    static world::gamerule::GameRules s_defaultRules;
    return s_defaultRules;
}
} // namespace

bool IWorld::hasFluid(i32 x, i32 y, i32 z) const
{
    const fluid::FluidState* fluidState = getFluidState(x, y, z);
    return fluidState != nullptr && !fluidState->isEmpty();
}

bool IWorld::isWaterAt(i32 x, i32 y, i32 z) const
{
    const fluid::FluidState* fluidState = getFluidState(x, y, z);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 检查是否为水（minecraft:water 或 minecraft:flowing_water）
    const fluid::Fluid& fluid = fluidState->getFluid();
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" && (loc.path() == "water" || loc.path() == "flowing_water");
}

bool IWorld::isLavaAt(i32 x, i32 y, i32 z) const
{
    const fluid::FluidState* fluidState = getFluidState(x, y, z);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 检查是否为岩浆（minecraft:lava 或 minecraft:flowing_lava）
    const fluid::Fluid& fluid = fluidState->getFluid();
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" && (loc.path() == "lava" || loc.path() == "flowing_lava");
}

bool IWorld::containsAnyLiquid(const AxisAlignedBB& box) const
{
    // 遍历碰撞箱覆盖的所有方块位置，检查是否存在流体
    i32 minX = math::floorTo<i32>(box.minX);
    i32 maxX = math::floorTo<i32>(box.maxX);
    i32 minY = math::floorTo<i32>(box.minY);
    i32 maxY = math::floorTo<i32>(box.maxY);
    i32 minZ = math::floorTo<i32>(box.minZ);
    i32 maxZ = math::floorTo<i32>(box.maxZ);

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                if (hasFluid(x, y, z)) {
                    return true;
                }
            }
        }
    }
    return false;
}

EntityInstanceId IWorld::spawnEntity(std::unique_ptr<Entity> entity)
{
    (void)entity;
    // 默认实现：不支持生成实体
    // ServerWorld 会重写此方法
    return 0;
}

void IWorld::notifyNeighborChanged(const BlockPos& neighborPos,
    const BlockState& neighborState,
    Block& sourceBlock,
    const BlockPos& sourcePos,
    bool isMoving)
{
    // neighborChanged 需要调度 tick，因此不能是 const 方法
    neighborState.getBlockMutable().neighborChanged(*this, neighborPos, sourceBlock, sourcePos, isMoving);
}

void IWorld::updateNeighbors(const BlockPos& pos, Block& sourceBlock)
{
    // 委托给 RedstoneSystem 实现
    world::redstone::RedstoneSystem::instance().updateNeighbors(*this, pos, sourceBlock);
}

void IWorld::updateNeighborsExcept(const BlockPos& pos, Block& sourceBlock, Direction except)
{
    // 委托给 RedstoneSystem 实现
    world::redstone::RedstoneSystem::instance().updateNeighborsExcept(*this, pos, sourceBlock, except);
}

const world::gamerule::GameRules& IWorld::getGameRules() const
{
    // 默认实现返回静态默认规则
    // ServerWorld 会重写此方法返回实际的游戏规则
    return getDefaultGameRules();
}

world::gamerule::GameRules& IWorld::getGameRules()
{
    // 默认实现返回静态默认规则
    return getDefaultGameRules();
}

} // namespace mc
