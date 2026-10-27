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

#include "FrogspawnBlock.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace blocks {

namespace {

/// 蝌蚪出生位置偏移范围（方块局部坐标，等价原版 Mth.clamp(nextDouble(), 0.2, 0.8)）
constexpr f64 TADPOLE_POSITION_OFFSET_MIN = 0.2;
constexpr f64 TADPOLE_POSITION_OFFSET_MAX = 0.8;

} // namespace

FrogspawnBlock::FrogspawnBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 碰撞箱: 16x1.5x16像素（非常薄，漂浮在水面上）
    m_shape = CollisionShape::fromPixelBox(0, 0, 0, 16, 1.5f, 16);
}

const CollisionShape& FrogspawnBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool FrogspawnBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 等价原版 mayPlaceOn(world, pos.below())：下方为水，上方无流体
    const fluid::FluidState* belowFluid = world.getFluidState(pos.down());
    const fluid::FluidState* aboveFluid = world.getFluidState(pos.up());
    const bool belowIsWater =
        belowFluid != nullptr && !belowFluid->isEmpty() && &belowFluid->getFluid() == fluid::Fluids::WATER();
    const bool aboveIsEmpty = aboveFluid == nullptr || aboveFluid->isEmpty();
    return belowIsWater && aboveIsEmpty;
}

void FrogspawnBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    // 等价原版 FrogspawnBlock.onPlace：放置时调度一次随机孵化 tick
    MC_UNUSED(state);
    MC_UNUSED(movedByPiston);
    world.tickManager().scheduleBlockTick(
        pos, *this, getHatchDelayForTest(world.getRandom()), world::tick::TickPriority::Normal);
}

void FrogspawnBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 计划刻由 TickManager 调度，仅在服务端触发；此处再显式守卫，避免测试桩/客户端误触。
    if (world.isClientSide()) {
        return;
    }

    auto& reader = static_cast<IBlockReader&>(world);
    if (!isValidPosition(state, reader, pos)) {
        // 等价原版 tick → destroyBlock(level, pos, false)：不可存活时消失，不产生掉落
        const BlockState* airState = BlockRegistry::instance().airState();
        MC_ASSERT_RELEASE(airState != nullptr);
        world.setBlockState(pos, airState, world::BlockUpdateFlags::UPDATE_ALL);
        return;
    }

    // hatchFrogspawn：先移除青蛙卵并播放孵化音效，再生成蝌蚪
    world.playSound(SoundEvents::FROGSPAWN_HATCH, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
    const BlockState* airState = BlockRegistry::instance().airState();
    MC_ASSERT_RELEASE(airState != nullptr);
    world.setBlockState(pos, airState, world::BlockUpdateFlags::UPDATE_ALL);
    _spawnTadpoles(world, pos, random);
}

BlockState FrogspawnBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 等价原版 updateShape：每次邻居变化都重新校验（下方水流走或上方被占用时立即消失），
    // 与 Block::updatePostPlacement 默认实现在此处直接返回结果值。
    if (!isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
        const BlockState* airState = BlockRegistry::instance().airState();
        MC_ASSERT_RELEASE(airState != nullptr);
        return *airState;
    }
    return state;
}

void FrogspawnBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    // 等价原版 entityInside：下落方块落入时击碎青蛙卵
    if (entity.entityType() != entity::VanillaEntityTypeKeys::FALLING_BLOCK) {
        return;
    }
    const BlockState* airState = BlockRegistry::instance().airState();
    MC_ASSERT_RELEASE(airState != nullptr);
    world.setBlockState(pos, airState, world::BlockUpdateFlags::UPDATE_ALL);
}

i32 FrogspawnBlock::getHatchDelayForTest(math::IRandom& random)
{
    // 原版 nextInt(min, max) 取 [min, max)，本项目 nextInt(min, max) 为闭区间，故上限减一
    return random.nextInt(MIN_HATCH_TICK_DELAY, MAX_HATCH_TICK_DELAY - 1);
}

void FrogspawnBlock::_spawnTadpoles(IWorld& world, const BlockPos& pos, math::IRandom& random)
{
    auto* registry = world.entityRegistry();
    if (registry == nullptr) {
        // 客户端世界不接入 ECS：孵化只在服务端触发，此分支仅在测试桩世界出现。
        return;
    }

    // 经实体类型工厂创建（与刷怪笼/GameTest 一致）：工厂负责绑定 typeId、初始化属性与满血。
    const entity::EntityType* tadpoleType = entity::VanillaEntityTypeKeys::TADPOLE;
    // 原版 nextInt(2, 6) 取 [2, 6)，本项目闭区间写法为 [2, 5]
    const i32 count = random.nextInt(MIN_TADPOLES_SPAWN, MAX_TADPOLES_SPAWN);
    for (i32 i = 0; i < count; ++i) {
        auto tadpole = tadpoleType->create(&world, *registry);
        if (tadpole == nullptr) {
            continue;
        }

        // 出生位置：卵的 Y 向下半格（浸入水中），水平方向各自随机偏移
        const f64 offsetX =
            math::clamp<f64>(random.nextDouble(), TADPOLE_POSITION_OFFSET_MIN, TADPOLE_POSITION_OFFSET_MAX);
        const f64 offsetZ =
            math::clamp<f64>(random.nextDouble(), TADPOLE_POSITION_OFFSET_MIN, TADPOLE_POSITION_OFFSET_MAX);
        tadpole->absSnapTo(static_cast<f32>(static_cast<f64>(pos.x) + offsetX),
            static_cast<f32>(static_cast<f64>(pos.y) - 0.5),
            static_cast<f32>(static_cast<f64>(pos.z) + offsetZ));
        // 原版 nextInt(1, 361) 取 [1, 361)，闭区间写法为 [1, 360]
        tadpole->setRotation(static_cast<f32>(random.nextInt(1, 360)), 0.0f);

        auto* mob = dynamic_cast<MobEntity*>(tadpole.get());
        MC_ASSERT_RELEASE(mob != nullptr);
        mob->enablePersistence();

        world.spawnEntity(std::move(tadpole));
    }
}

} // namespace blocks
} // namespace mc
