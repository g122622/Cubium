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

#include "common/world/block/blocks/LiquidBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/FluidProperties.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FlowingFluid.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <functional>
#include <utility>
#include <vector>

namespace mc {
namespace block {

// ============================================================================
// LiquidBlock 实现
// ============================================================================

// 方块LEVEL属性 (0-15)
namespace {
const IntegerProperty& LEVEL_0_15()
{
    return BlockStateProperties::LEVEL_0_15();
}
} // namespace

LiquidBlock::LiquidBlock(fluid::FlowingFluid& fluid, BlockProperties properties)
    : Block(properties)
    , m_fluid(fluid)
{

    // 创建方块状态容器，包含LEVEL属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(LEVEL_0_15())
            .create([this](const Block& block,
                        auto values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态（level=0，即源头）
    setDefaultState(defaultState().with(LEVEL_0_15(), 0));

    // 构建流体状态缓存
    _buildFluidStateCache();
}

const mc::fluid::FluidState* LiquidBlock::getFluidState(const BlockState& state) const
{
    i32 blockLevel = state.get(LEVEL_0_15());
    return &m_fluidStateCache[blockLevel];
}

const CollisionShape& LiquidBlock::getCollisionShape(const BlockState& state) const
{
    // 液体方块没有碰撞形状
    (void)state;
    return VoxelShapes::empty();
}

bool LiquidBlock::ticksRandomly() const noexcept
{
    return m_fluid.ticksRandomly();
}

void LiquidBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    const fluid::FluidState* fluidState = getFluidState(state);
    if (fluidState != nullptr && !fluidState->isEmpty()) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast) — randomTick 是非 const 方法，需要 const_cast
        fluid::Fluid& fluidRef = const_cast<fluid::Fluid&>(fluidState->getFluid());
        fluidRef.randomTick(world, pos, *fluidState, random);
    }
}

void LiquidBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 只有当 reactWithNeighbors 返回 true 时才调度流体 tick
    // 如果岩浆放置时旁边有水，会先反应变成石头/黑曜石，返回 false，不再调度 tick
    if (reactWithNeighbors(world, pos, state)) {
        const fluid::FluidState* fluidState = getFluidState(state);
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            const fluid::Fluid& fluidRef = fluidState->getFluid();
            world.tickManager().scheduleFluidTick(pos, fluidRef, fluidRef.getTickDelay(world));
        }
    }
}

void LiquidBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    // 只有当 reactWithNeighbors 返回 true 时才调度流体 tick
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr || !currentState->is(this)) {
        return;
    }

    // 首先检查是否发生了反应（岩浆+水）
    if (reactWithNeighbors(world, pos, *currentState)) {
        const fluid::FluidState* fluidState = currentState->getFluidState();
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            const fluid::Fluid& fluidRef = fluidState->getFluid();
            world.tickManager().scheduleFluidTick(pos, fluidRef, fluidRef.getTickDelay(world));
        }
    }

    (void)neighborBlock;
    (void)neighborPos;
    (void)isMoving;
}

// 注意: LiquidBlock 不应该有 tick() 方法
// 流体 tick 由 TickManager 的 fluidTicks 列表直接调用 Fluid.tick()

BlockState LiquidBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 只有当当前流体是源头或邻居流体是源头时才调度tick
    // 如果反应发生则不调度

    (void)facingPos;

    // 检查岩浆水反应
    if (reactWithNeighbors(world, currentPos, state)) {
        // 反应没有发生，检查是否需要调度流体tick
        const fluid::FluidState* fluidState = getFluidState(state);
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            // MC条件：当前流体是源头 或 邻居流体是源头
            bool currentIsSource = fluidState->isSource();
            bool neighborIsSource = false;

            const fluid::FluidState* neighborFluid = facingState.getFluidState();
            if (neighborFluid != nullptr && !neighborFluid->isEmpty()) {
                neighborIsSource = neighborFluid->isSource();
            }

            if (currentIsSource || neighborIsSource) {
                const fluid::Fluid& fluidRef = fluidState->getFluid();
                world.tickManager().scheduleFluidTick(currentPos, fluidRef, fluidRef.getTickDelay(world));
            }
        }
    }

    return state;
}

bool LiquidBlock::reactWithNeighbors(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 只处理岩浆方块

    const fluid::FluidState* fluidState = getFluidState(state);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return true;
    }

    // 检查是否是岩浆
    if (!fluidState->getFluid().isIn(fluid::FluidTags::LAVA())) {
        return true;
    }

    // 检查下方是否有灵魂土（用于玄武岩生成）
    const BlockState* belowState = world.getBlockState(pos.x, pos.y - 1, pos.z);
    bool hasSoulSoilBelow =
        (belowState != nullptr && VanillaBlocks::SOUL_SOIL != nullptr && belowState->is(VanillaBlocks::SOUL_SOIL));

    // 检查所有方向（除了下方）
    for (Direction dir : Directions::all()) {
        if (dir == Direction::Down) {
            continue;
        }

        BlockPos neighborPos = pos.offset(dir);
        const fluid::FluidState* neighborFluid = world.getFluidState(neighborPos);

        // 检查是否是水
        if (neighborFluid != nullptr && !neighborFluid->isEmpty() &&
            neighborFluid->getFluid().isIn(fluid::FluidTags::WATER())) {
            // 岩浆 + 水 -> 生成方块
            Block* resultBlock = nullptr;

            if (fluidState->isSource()) {
                // 源头岩浆 + 水 -> 黑曜石
                resultBlock = VanillaBlocks::OBSIDIAN;
            } else {
                // 流动岩浆 + 水 -> 圆石
                resultBlock = VanillaBlocks::COBBLESTONE;
            }

            if (resultBlock != nullptr) {
                world.setBlockState(pos, &resultBlock->defaultState(), 3);
                triggerMixEffects(world, pos);
                return false;
            }
        }

        // 检查蓝冰 + 灵魂土 -> 玄武岩
        if (hasSoulSoilBelow) {
            const BlockState* neighborBlock = world.getBlockState(neighborPos);
            if (neighborBlock != nullptr && VanillaBlocks::BLUE_ICE != nullptr &&
                neighborBlock->is(VanillaBlocks::BLUE_ICE)) {
                // 岩浆 + 蓝冰 + 灵魂土 -> 玄武岩
                if (VanillaBlocks::BASALT != nullptr) {
                    world.setBlockState(pos, &VanillaBlocks::BASALT->defaultState(), 3);
                    triggerMixEffects(world, pos);
                    return false;
                }
            }
        }
    }

    return true;
}

void LiquidBlock::triggerMixEffects(IWorld& world, const BlockPos& pos)
{
    // 播放嘶嘶声和烟雾粒子效果

    // 播放嘶嘶声
    world.playSound(ResourceLocation("minecraft:block.lava.extinguish"),
        sound::SoundCategory::Blocks,
        Vector3(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f),
        0.5f, // 音量
        1.0f  // 音调
    );

    // 生成烟雾粒子 - 使用位置和索引派生确定性随机数
    for (i32 i = 0; i < 8; ++i) {
        u64 particleSeed = world.seed() ^ static_cast<u64>(std::hash<BlockPos>{}(pos)) ^ static_cast<u64>(i);
        math::Random random(particleSeed);
        f32 offsetX = random.nextFloat() * 0.6f - 0.3f;
        f32 offsetZ = random.nextFloat() * 0.6f - 0.3f;
        world.addParticle(particle::ParticleTypeId::Smoke,
            Vector3(pos.x + 0.5f + offsetX, pos.y + 1.0f, pos.z + 0.5f + offsetZ),
            Vector3(0.0f, 0.1f, 0.0f));
    }
}

fluid::Fluid* LiquidBlock::pickupFluid(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 只有源头方块可以被舀起

    i32 blockLevel = state.get(LEVEL_0_15());
    if (blockLevel == 0) { // 源头
        // 移除流体方块
        if (VanillaBlocks::AIR != nullptr) {
            world.setBlockState(pos, &VanillaBlocks::AIR->defaultState(), 11);
        }
        return &m_fluid.getStill();
    }

    return nullptr; // 非源头，无法舀起
}

i32 LiquidBlock::blockLevelToFluidLevel(i32 blockLevel) noexcept
{
    // 方块level=0 -> 流体level=SOURCE_LEVEL（源头）
    // 方块level=1-7 -> 流体level=SOURCE_LEVEL-blockLevel（反向）
    // 方块level=8-15 -> 流体level=SOURCE_LEVEL（下落）
    if (blockLevel == 0) {
        return fluid::SOURCE_LEVEL; // 源头
    } else if (blockLevel >= 1 && blockLevel <= 7) {
        return fluid::SOURCE_LEVEL - blockLevel; // 1->7, 2->6, ..., 7->1
    } else {
        return fluid::SOURCE_LEVEL; // 下落，level>=SOURCE_LEVEL
    }
}

i32 LiquidBlock::fluidLevelToBlockLevel(i32 fluidLevel, bool falling) noexcept
{
    // 流体level=SOURCE_LEVEL, falling=false -> 方块level=0（源头）
    // 流体level=1-7 -> 方块level=SOURCE_LEVEL-fluidLevel
    // 流体level=SOURCE_LEVEL, falling=true -> 方块level=SOURCE_LEVEL（下落）
    if (falling) {
        return fluid::SOURCE_LEVEL;
    } else if (fluidLevel == fluid::SOURCE_LEVEL) {
        return 0; // 源头
    } else {
        return fluid::SOURCE_LEVEL - fluidLevel;
    }
}

void LiquidBlock::_buildFluidStateCache()
{
    m_fluidStateCache.clear();
    m_fluidStateCache.reserve(16);

    auto& levelProp = fluid::FluidProperties::LEVEL_1_8();
    auto& fallingProp = fluid::FluidProperties::FALLING();

    for (i32 blockLevel = 0; blockLevel <= 15; ++blockLevel) {
        i32 fluidLevel = blockLevelToFluidLevel(blockLevel);
        bool falling = isFallingLevel(blockLevel);

        if (fluidLevel == fluid::SOURCE_LEVEL && !falling) {
            // 源头状态
            m_fluidStateCache.push_back(m_fluid.getStill().defaultState());
        } else {
            // 流动状态
            m_fluidStateCache.push_back(
                m_fluid.getFlowing().defaultState().with(levelProp, fluidLevel).with(fallingProp, falling));
        }
    }
}

void LiquidBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 检查流体是否为岩浆
    const fluid::FluidState* fluidState = getFluidState(state);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return;
    }

    if (!fluidState->getFluid().isIn(fluid::FluidTags::LAVA())) {
        return;
    }

    // 岩浆流体碰撞：先点燃实体，再造成岩浆伤害
    // 参考 MC Java: LavaFluid.entityInside() -> LAVA_IGNITE + lavaHurt
    entity.lavaIgnite();
    entity.lavaHurt();
}

} // namespace block
} // namespace mc
