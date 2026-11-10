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
#include "block/blocks/nether/FireBlock.hpp"
#include "blockentity/BlockEntity.hpp"
#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/effect/base/LevelEventId.hpp"
#include "entity/core/Entity.hpp"
#include "fluid/Fluid.hpp"
#include "gameevent/GameEvents.hpp"
#include "gamerule/GameRules.hpp"
#include "redstone/RedstoneSystem.hpp"
#include "util/Direction.hpp"
#include "util/math/MathUtils.hpp"
#include <memory>

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

void IWorld::setBlockEntity(const BlockPos& pos, std::unique_ptr<BlockEntity> entity)
{
    (void)pos;
    (void)entity;
    // 默认实现：不支持方块实体（entity 随 unique_ptr 析构）
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

void IWorld::notifyShapeChanged(Direction direction,
    const BlockPos& neighborPos,
    const BlockPos& sourcePos,
    const BlockState& sourceState,
    i32 flags,
    i32 recursionLeft,
    bool movedByPiston)
{
    // 默认实现与 vanilla `Level` 一致：交给形状更新执行器。需要特殊排队语义的世界
    // （如收集式邻居更新器）可重写本方法，但形状级联的语义必须保持等价。
    Block::executeShapeUpdate(
        direction, sourceState, *this, neighborPos, sourcePos, flags, recursionLeft, movedByPiston);
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

bool IWorld::canSpreadFireAround(const BlockPos& pos) const
{
    // -1 表示不做玩家半径门控；其余值要求该位置附近存在至少一名非旁观者玩家
    const i32 radius = getGameRules().getInt(world::gamerule::GameRuleKeys::FIRE_SPREAD_RADIUS_AROUND_PLAYER);
    return radius == -1 || anyPlayerCloseEnoughTo(pos, radius);
}

bool IWorld::mayInteract(const Entity& entity, const BlockPos& pos) const
{
    // 基类世界（客户端世界与测试世界）不做世界级限制，对齐 vanilla Level#mayInteract 返回 true。
    // ServerWorld 重写补全世界边界与出生点保护。
    (void)entity;
    (void)pos;
    return true;
}

bool IWorld::removeBlock(const BlockPos& pos, bool isMoving)
{
    // 遗留方块取该位置当前流体对应的方块形态：无流体 → 空气，水/岩浆 → 对应流体方块。
    // 与 vanilla FluidState.createLegacyBlock() 语义一致。
    const fluid::FluidState* fluidState = getFluidState(pos);
    const BlockState* replacement = nullptr;
    if (fluidState != nullptr && !fluidState->isEmpty()) {
        replacement = fluidState->getBlockState();
    }
    if (replacement == nullptr) {
        replacement = BlockRegistry::instance().airState();
    }
    MC_ASSERT_RELEASE(replacement != nullptr);

    i32 flags = world::BlockUpdateFlags::UPDATE_ALL;
    if (isMoving) {
        flags |= world::BlockUpdateFlags::UPDATE_MOVE_BY_PISTON;
    }
    return setBlockState(pos, replacement, flags);
}

bool IWorld::destroyBlock(const BlockPos& pos, bool dropBlock, const Entity* entity, i32 recursionLeft)
{
    // recursionLeft 是形状更新级联的深度预算，由调用方（Block::updateOrDestroy）负责消耗；
    // 本方法当前不自行发起形状级联，保留参数以对齐原版签名。
    MC_UNUSED(recursionLeft);

    const BlockState* state = getBlockState(pos);
    if (state == nullptr || state->isAir()) {
        return false;
    }

    // 保存破坏前的状态快照：后续写入会替换该位置的方块
    const BlockState brokenState = *state;
    const Block& block = brokenState.getBlock();

    // 火焰方块被移除时不产生破坏粒子（火焰原本就没有对应的破坏音效/粒子）
    if (dynamic_cast<const blocks::FireBlock*>(&block) == nullptr) {
        playEvent(::mc::world::LevelEventId::PARTICLES_DESTROY_BLOCK, pos, static_cast<i32>(brokenState.stateId()));
    }

    // 掉落物只在服务端生成（Block::dropResources 内部以掉落表管理器为空判定客户端）。
    // 方块实体必须在移除前取出：掉落表（copy_components / copy_custom_data）需要读取它。
    if (dropBlock) {
        Block::dropResources(*this, pos, brokenState, getBlockEntity(pos));
    }

    const bool written = removeBlock(pos, false);
    if (written) {
        gameEvent(gameevent::GameEvents::BLOCK_DESTROY, pos, gameevent::GameEvent::Context::of(entity, &brokenState));
    }
    return written;
}

bool IWorld::isSpawningMonsters() const
{
    if (difficulty() == Difficulty::Peaceful) {
        return false;
    }
    const world::gamerule::GameRules& rules = getGameRules();
    return rules.getBoolean(world::gamerule::GameRuleKeys::SPAWN_MOBS) &&
        rules.getBoolean(world::gamerule::GameRuleKeys::SPAWN_MONSTERS);
}
} // namespace mc
