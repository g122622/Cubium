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

#include "IceBlock.hpp"

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// 常量
// ============================================================================

namespace {

/// 冰融化的最小光照等级阈值
constexpr i32 MELT_LIGHT_LEVEL = 11;

thread_local bool s_skipIceReplacementCallback = false;

class IceReplacementGuard {
public:
    IceReplacementGuard() { s_skipIceReplacementCallback = true; }

    ~IceReplacementGuard() { s_skipIceReplacementCallback = false; }
};

[[nodiscard]] const BlockState* getWaterState()
{
    fluid::Fluid* waterFluid = fluid::Fluid::getFluid(fluid::FluidRegistry::WATER_ID);
    if (waterFluid != nullptr) {
        return waterFluid->getBlockState(waterFluid->defaultState());
    }
    return nullptr;
}

[[nodiscard]] const BlockState* getAirState()
{
    return BlockRegistry::instance().airState();
}

void replaceIceState(IWorld& world, const BlockPos& pos, const BlockState* replacementState)
{
    if (replacementState == nullptr) {
        return;
    }

    IceReplacementGuard guard;
    world.setBlockState(pos, replacementState, 3);
}

void meltIce(IWorld& world, const BlockPos& pos)
{
    const BlockState* replacementState = world.isUltraWarm() ? getAirState() : getWaterState();
    replaceIceState(world, pos, replacementState);
}

void handleIceBreak(IWorld& world, const BlockPos& pos)
{
    if (world.isUltraWarm()) {
        replaceIceState(world, pos, getAirState());
        return;
    }

    const BlockState* belowState = world.getBlockState(pos.down());
    if (belowState != nullptr && (belowState->isSolid() || belowState->isLiquid())) {
        replaceIceState(world, pos, getWaterState());
        return;
    }

    replaceIceState(world, pos, getAirState());
}

} // namespace

// ============================================================================
// IceBlock 实现
// ============================================================================

IceBlock::IceBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 冰的滑度通过 BlockProperties.slipperiness() 在注册时设置
}

void IceBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    if (s_skipIceReplacementCallback) {
        return;
    }

    handleIceBreak(world, pos);
}

void IceBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // MC 原版: IceBlock.randomTick 检查方块光照减去自身不透明度
    // 条件: getBrightness(LightLayer.BLOCK, pos) > 11 - getLightBlock()
    // 冰的不透明度为 2，因此条件为 blockLight > 11 - 2 = 9
    // 即方块光照 > 9 时冰融化
    // 参考: net.minecraft.world.level.block.IceBlock.randomTick
    u8 blockLight = world.getBlockLight(pos);
    i32 opacity = state.getOpacity();

    if (blockLight > MELT_LIGHT_LEVEL - opacity) {
        meltIce(world, pos);
    }
}

// ============================================================================
// PackedIceBlock 实现
// ============================================================================

PackedIceBlock::PackedIceBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 浮冰不需要特殊逻辑，不融化
}

// ============================================================================
// BlueIceBlock 实现
// ============================================================================

BlueIceBlock::BlueIceBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 蓝冰摩擦力最低（0.989），通过BlockProperties设置
}

// ============================================================================
// FrostedIceBlock 实现
// ============================================================================

FrostedIceBlock::FrostedIceBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 创建状态容器，添加 AGE 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(AGE_PROP())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(AGE_PROP(), 0));
}

void FrostedIceBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // MC 原版: scheduleTick(this, Mth.nextInt(random, 60, 120))
    // 初始 tick 延迟为 60-120 ticks
    world.tickManager().scheduleBlockTick(
        pos, *this, math::Random(world.seed() ^ pos.toId()).nextInt(60, 120), world::tick::TickPriority::Normal);
}

void FrostedIceBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // MC 原版: neighborChanged 中检查触发变化的邻居是否为霜冰，
    // 如果是，检查自身霜冰邻居是否少于2个，若不足则融化
    // 参考: net.minecraft.world.level.block.FrostedIceBlock.neighborChanged
    if (neighborBlock.defaultState().is(this)) {
        const BlockState* state = world.getBlockState(pos);
        if (state && state->is(this)) {
            IBlockReader& blockReader = static_cast<IBlockReader&>(world);
            if (_shouldMelt(blockReader, pos, 2)) {
                meltIce(world, pos);
            }
        }
    }

    Block::neighborChanged(world, pos, neighborBlock, neighborPos, isMoving);
}

void FrostedIceBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // MC 原版: FrostedIceBlock.tick
    // 条件1: random.nextInt(3) == 0 || fewerNeighboursThan(4)
    // 条件2: 光照 > 11 - AGE - opacity
    // 光源: 末地维度仅用方块光, 其他维度用 getMaxLocalRawBrightness
    // 参考: net.minecraft.world.level.block.FrostedIceBlock.tick
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    i32 age = getAge(state);

    // 光照检查
    // MC 原版: dimension() == Level.END 时仅用方块光，否则用 getMaxLocalRawBrightness
    // 等价判断: 无天空光照的维度仅用方块光
    // 参考: net.minecraft.world.level.block.FrostedIceBlock.tick
    i32 lightLevel;
    if (!world.hasSkyLight()) {
        // 无天空光照的维度(末地): 仅使用方块光照
        lightLevel = static_cast<i32>(world.getBlockLight(pos));
    } else {
        // 有天空光照的维度(主世界): 使用 getMaxLocalRawBrightness (含天气衰减的天空光照)
        lightLevel = world.getMaxLocalRawBrightness(pos);
    }

    i32 opacity = state.getOpacity();

    bool shouldMeltNow =
        (random.nextInt(3) == 0 || _shouldMelt(blockReader, pos, 4)) && lightLevel > MELT_LIGHT_LEVEL - age - opacity;

    if (shouldMeltNow && _slightlyMelt(world, pos, state)) {
        // 完全融化，通知相邻霜冰检查
        // MC 原版: 对每个霜冰邻居调用 slightlyMelt，只调度未完全融化的邻居
        for (Direction dir : Directions::all()) {
            BlockPos neighborPos = pos.offset(dir);
            const BlockState* neighborState = world.getBlockState(neighborPos);
            if (neighborState && neighborState->is(this)) {
                // 对邻居调用 slightlyMelt
                BlockState neighborStateMutable = *neighborState;
                if (!_slightlyMelt(world, neighborPos, neighborStateMutable)) {
                    // 邻居未完全融化，调度其 tick
                    world.tickManager().scheduleBlockTick(
                        neighborPos, *this, random.nextInt(20, 40), world::tick::TickPriority::Normal);
                }
            }
        }
    } else {
        // 继续调度下一次 tick
        world.tickManager().scheduleBlockTick(pos, *this, random.nextInt(20, 40), world::tick::TickPriority::Normal);
    }
}

void FrostedIceBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // MC 原版: FrostedIceBlock 继承自 IceBlock，不重写 randomTick
    // 因此 randomTick 使用 IceBlock 的逻辑：仅检查方块光照 > 11 - opacity
    // 参考: net.minecraft.world.level.block.IceBlock.randomTick
    u8 blockLight = world.getBlockLight(pos);
    i32 opacity = state.getOpacity();

    if (blockLight > MELT_LIGHT_LEVEL - opacity) {
        meltIce(world, pos);
    }
}

bool FrostedIceBlock::_shouldMelt(IBlockReader& world, const BlockPos& pos, i32 neighborsRequired) const
{
    // 检查周围霜冰邻居数量
    // 如果霜冰邻居数量 >= neighborsRequired，则不应该融化
    i32 frostNeighborCount = 0;

    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState && neighborState->is(this)) {
            ++frostNeighborCount;
            if (frostNeighborCount >= neighborsRequired) {
                return false;
            }
        }
    }

    return true;
}

bool FrostedIceBlock::_slightlyMelt(IWorld& world, const BlockPos& pos, BlockState& state)
{
    i32 age = getAge(state);

    if (age < 3) {
        // 增加 AGE
        BlockState newState = state.with(AGE_PROP(), age + 1);
        world.setBlockState(pos, &newState, 2);
        return false;
    } else {
        // 完全融化成水
        meltIce(world, pos);
        return true;
    }
}

} // namespace blocks
} // namespace mc
