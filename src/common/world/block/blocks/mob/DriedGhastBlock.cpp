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

#include "DriedGhastBlock.hpp"

#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

namespace {

/// 湿润等级提升所需的计划刻延迟（对齐 vanilla DriedGhastBlock.HYDRATION_TICK_DELAY）
constexpr i32 HYDRATION_TICK_DELAY = 5000;

/// 触发环境音效的下方方块标签
constexpr const char* TRIGGERS_AMBIENT_TAG = "triggers_ambient_dried_ghast_block_sounds";

} // namespace

DriedGhastBlock::DriedGhastBlock(const BlockProperties& properties)
    : Block(properties)
    , m_shape(CollisionShape::fromPixelBox(3.0f, 0.0f, 3.0f, 13.0f, 10.0f, 13.0f))
{
    // 创建状态容器（FACING / HYDRATION_LEVEL / WATERLOGGED）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认状态：朝向北、湿润等级 0、不含水
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS(), 0)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

i32 DriedGhastBlock::getHydrationLevel(const BlockState& state) const
{
    return state.get(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS());
}

bool DriedGhastBlock::_isReadyToSpawn(const BlockState& state) const
{
    return getHydrationLevel(state) == 3;
}

BlockState DriedGhastBlock::getStateForPlacement(BlockItemUseContext& context)
{
    IWorld& world = context.getWorld();
    const BlockPos pos = context.placementPos();

    // 检测放置位置是否为水源（对齐 vanilla DriedGhastBlock.getStateForPlacement）
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    const bool waterlogged = waterloggable::isWaterSourceFluidState(fluidState);

    // 朝向玩家水平方向的反方向
    Direction facing = Directions::opposite(context.horizontalDirection());

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS(), 0)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

void DriedGhastBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(stack);
    // 播放放置音效（含水/无水区分，对齐 vanilla setPlacedBy）
    const ResourceLocation& sound = state.get(BlockStateProperties::WATERLOGGED())
        ? SoundEvents::DRIED_GHAST_PLACE_IN_WATER
        : SoundEvents::DRIED_GHAST_PLACE;
    world.playSound(sound, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
}

BlockState DriedGhastBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 含水时调度水流体 tick（对齐 vanilla DriedGhastBlock.updateShape）
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

void DriedGhastBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 对齐 vanilla DriedGhastBlock.randomTick：
    //   当含水或湿润等级 > 0 且尚无已调度的计划刻时，调度 5000 tick 后执行 tick()。
    const bool waterlogged = state.get(BlockStateProperties::WATERLOGGED());
    const i32 hydration = getHydrationLevel(state);
    if ((waterlogged || hydration > 0) && !world.tickManager().isBlockTickScheduled(pos, *this)) {
        world.tickManager().scheduleBlockTick(pos, *this, HYDRATION_TICK_DELAY);
    }
}

void DriedGhastBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 对齐 vanilla DriedGhastBlock.tick：
    //   含水 → 提升湿润等级或孵化；无水 → 湿润等级逐级下降
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        _tickWaterlogged(world, pos, state);
    } else {
        const i32 hydration = getHydrationLevel(state);
        if (hydration > 0) {
            world.setBlockState(pos,
                &state.with(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS(), hydration - 1),
                world::BlockUpdateFlags::UPDATE_CLIENTS);
            world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &state);
        }
    }
}

void DriedGhastBlock::_tickWaterlogged(IWorld& world, const BlockPos& pos, BlockState& state)
{
    if (!_isReadyToSpawn(state)) {
        // 未达阈值：播放过渡音效并提升一级湿润等级
        world.playSound(
            SoundEvents::DRIED_GHAST_TRANSITION, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
        world.setBlockState(pos,
            &state.with(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS(), getHydrationLevel(state) + 1),
            world::BlockUpdateFlags::UPDATE_CLIENTS);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &state);
    } else {
        _spawnGhastling(world, pos, state);
    }
}

void DriedGhastBlock::_spawnGhastling(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 移除方块本身
    if (const BlockState* airState = BlockRegistry::instance().airState()) {
        world.setBlockState(pos, airState, world::BlockUpdateFlags::UPDATE_ALL);
    }

    // TODO: 生成幼年快乐恶魂（HappyGhast, baby=true）并播放 GHASTLING_SPAWN 音效。
    //   当前 happy_ghast 实体尚未在 VanillaEntities 注册，EntityRegistry 查不到该类型，
    //   待快乐恶魂实体实现后补全：
    //     auto* type = EntityRegistry::instance().getType("minecraft:happy_ghast");
    //     auto* ecs = world.entityRegistry();
    //     if (type != nullptr && ecs != nullptr) {
    //         auto entity = type->create(&world, *ecs);
    //         // 设为幼年、朝向 FACING、置于 pos 底部中心
    //         world.spawnEntity(std::move(entity));
    //         world.playSound(SoundEvents::GHASTLING_SPAWN, SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
    //     }
    world.playSound(SoundEvents::GHASTLING_SPAWN, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
}

void DriedGhastBlock::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    const f32 x = static_cast<f32>(pos.x) + 0.5f;
    const f32 y = static_cast<f32>(pos.y) + 0.5f;
    const f32 z = static_cast<f32>(pos.z) + 0.5f;

    if (!state.get(BlockStateProperties::WATERLOGGED())) {
        // 无水：偶发环境音（下方为 soul_sand/soul_soil 时）+ 白烟粒子
        if (random.nextInt(40) == 0) {
            const BlockState* below = context.getBlockState(pos.x, pos.y - 1, pos.z);
            if (below != nullptr) {
                BlockTag* tag = BlockTags::getTag(ResourceLocation("minecraft", TRIGGERS_AMBIENT_TAG));
                if (tag != nullptr && tag->contains(*below)) {
                    context.playLocalSound(
                        SoundEvents::DRIED_GHAST_AMBIENT, sound::SoundCategory::Blocks, Vector3(x, y, z), 1.0f, 1.0f);
                }
            }
        }
        if (random.nextInt(6) == 0) {
            context.addAnimateParticle(
                particle::ParticleTypeId::WhiteSmoke, Vector3(x, y, z), Vector3(0.0f, 0.02f, 0.0f));
        }
    } else {
        // 含水：偶发环境音 + 快乐村民粒子
        if (random.nextInt(40) == 0) {
            context.playLocalSound(
                SoundEvents::DRIED_GHAST_AMBIENT_WATER, sound::SoundCategory::Blocks, Vector3(x, y, z), 1.0f, 1.0f);
        }
        if (random.nextInt(6) == 0) {
            const f32 ox = (random.nextFloat() * 2.0f - 1.0f) / 3.0f;
            const f32 oz = (random.nextFloat() * 2.0f - 1.0f) / 3.0f;
            context.addAnimateParticle(particle::ParticleTypeId::HappyVillager,
                Vector3(x + ox, y + 0.4f, z + oz),
                Vector3(0.0f, random.nextFloat(), 0.0f));
        }
    }
}

const CollisionShape& DriedGhastBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool DriedGhastBlock::allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 干燥恶魂碰撞箱非完整方块，须显式禁止路径寻找通过（对齐 vanilla isPathfindable=false）
    return false;
}

const BlockState& DriedGhastBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), Directions::rotateDirection(facing, rotation));
}

const BlockState& DriedGhastBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    return state.with(BlockStateProperties::HORIZONTAL_FACING(),
        Directions::rotateDirection(facing, Directions::mirrorToRotation(mirror, facing)));
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* DriedGhastBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

bool DriedGhastBlock::receiveFluid(
    IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::FluidState& fluidState)
{
    // 对齐 vanilla DriedGhastBlock.placeLiquid：未含水且流入水时，设置 WATERLOGGED=true，
    // 调度水流体 tick 并播放入水音效
    if (!state.get(BlockStateProperties::WATERLOGGED()) && waterloggable::isWaterFluidState(&fluidState)) {
        if (!world.isClientSide()) {
            world.setBlockState(pos,
                &state.with(BlockStateProperties::WATERLOGGED(), true),
                world::BlockUpdateFlags::UPDATE_ALL);
            waterloggable::scheduleWaterTick(world, pos);
            world.playSound(
                SoundEvents::DRIED_GHAST_PLACE_IN_WATER, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
        }
        return true;
    }
    return false;
}

} // namespace blocks
} // namespace mc
