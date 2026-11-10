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

#include "NetherPortalBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/dimension/teleport/PortalSize.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

NetherPortalBlock::NetherPortalBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_AXIS())
            .create([this](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), Axis::X));

    // 创建形状
    m_xAxisShape = CollisionShape::box(0.0f, 0.0f, 0.375f, 1.0f, 1.0f, 0.625f);
    m_zAxisShape = CollisionShape::box(0.375f, 0.0f, 0.0f, 0.625f, 1.0f, 1.0f);
}

Axis NetherPortalBlock::getAxis(const BlockState& state) const
{
    return state.get(BlockStateProperties::HORIZONTAL_AXIS());
}

BlockState NetherPortalBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction facing = context.horizontalDirection();
    Axis axis = Directions::getAxis(facing);
    if (axis == Axis::Y) {
        axis = Axis::X; // 水平轴
    }
    return defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), axis);
}

BlockState NetherPortalBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingPos);

    // 门平面外的更新不影响框架；逐格填充门时不校验尚未放完的内部区域。
    const Axis changedAxis = Directions::getAxis(facing);
    const bool outsidePlane = changedAxis != getAxis(state) && changedAxis != Axis::Y;
    if (!outsidePlane && !facingState.is(this) && !PortalSize::isCompletePortal(world, currentPos, getAxis(state))) {
        return *BlockRegistry::instance().airState();
    }
    return state;
}

void NetherPortalBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 实体进入传送门后开始传送计时
    // 玩家需要站立在传送门中约 4 秒（80 ticks）才能传送
    // 其他实体约 1 tick

    MC_UNUSED(state);
    MC_UNUSED(world);

    // 检查实体是否是乘客或被骑乘
    if (entity.isRiding() || entity.hasPassengers()) {
        return;
    }

    // Boss 不能使用传送门（末影龙、凋灵等）
    if (!entity.isNonBoss()) {
        return;
    }

    // 检查传送冷却
    if (!entity.canTeleport()) {
        return;
    }

    // 设置实体在传送门中
    entity.setInPortal(true);
    // 记录传送门位置
    entity.setPortalPos(pos);

    // 注意：传送逻辑由 Entity::tickPortal() 处理
    // 玩家的 getMaxInPortalTime() 返回 80 ticks
    // 其他实体的 getMaxInPortalTime() 返回 1 tick
}

const CollisionShape& NetherPortalBlock::getShape(const BlockState& state) const
{
    Axis axis = getAxis(state);
    return (axis == Axis::X) ? m_xAxisShape : m_zAxisShape;
}

const CollisionShape& NetherPortalBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

void NetherPortalBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(state);
    // 覆盖测试见 tests/unit/common/world/block/blocks/NetherPortalSpawnTest.cpp：
    // 该测试世界用位置层把 NETHER_PORTAL_SPAWNS_PIGLINS 钉成 true，从而能正面断言生成，
    // 而不只是断言各条早退。
    // 与原版一致：仅服务端、允许生成敌对生物、该维度声明了「传送门刷猪灵」属性、
    // 按难度概率命中、且附近有玩家时才继续。
    if (world.isClientSide() || !world.isSpawningMonsters()) {
        return;
    }

    const Vector3 queryPos(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
    if (!world.environmentAttributes().getValue(
            world::attribute::EnvironmentAttributes::NETHER_PORTAL_SPAWNS_PIGLINS(), queryPos)) {
        return;
    }

    // 难度 id：和平=0 已在 isSpawningMonsters 中排除，简单=1、普通=2、困难=3。
    if (random.nextInt(2000) >= static_cast<i32>(world.difficulty())) {
        return;
    }

    // 128 格内有玩家才生成（与原版 anyPlayerCloseEnoughForSpawning 等价）。
    constexpr i32 SPAWN_PLAYER_RADIUS = 128;
    if (!world.anyPlayerCloseEnoughTo(pos, SPAWN_PLAYER_RADIUS)) {
        return;
    }

    // 向下找到门体下方第一格（门柱底部），生成位置在其上方。
    BlockPos spawnBase = pos;
    const BlockState* current = world.getBlockState(spawnBase);
    while (current != nullptr && current->is(VanillaBlocks::NETHER_PORTAL)) {
        spawnBase = spawnBase.down();
        current = world.getBlockState(spawnBase);
    }
    if (current == nullptr || world.entityRegistry() == nullptr) {
        return;
    }

    const entity::EntityType* piglinType =
        entity::EntityRegistry::instance().getType(entity::EntityTypeKeys::ZOMBIFIED_PIGLIN);
    if (piglinType == nullptr) {
        return;
    }

    if (!current->getBlock().isValidSpawn(world, spawnBase, *current, *piglinType)) {
        return;
    }

    std::unique_ptr<Entity> piglin = piglinType->create(&world, *world.entityRegistry());
    if (piglin == nullptr) {
        return;
    }

    const BlockPos spawnPos = spawnBase.up();
    piglin->setPosition(
        static_cast<f32>(spawnPos.x) + 0.5f, static_cast<f32>(spawnPos.y), static_cast<f32>(spawnPos.z) + 0.5f);
    // 生成后立刻设置传送门冷却，避免新生成的僵尸猪灵被同一扇门带走。
    piglin->triggerPortalCooldown();
    world.spawnEntity(std::move(piglin));
}

} // namespace blocks
} // namespace mc
