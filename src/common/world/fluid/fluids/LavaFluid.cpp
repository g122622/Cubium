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

#include "LavaFluid.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/FluidProperties.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/nether/FireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FlowingFluid.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include <utility>
#include <vector>

namespace mc {
namespace fluid {

// ============================================================================
// LavaFluid 基类实现
// ============================================================================

const BlockState* LavaFluid::getBlockState(const FluidState& state) const
{
    // 方块LEVEL映射:
    // - 源头(level=8, isSource=true) -> 方块level=0
    // - 流动(level=1-7) -> 方块level=8-level
    // - 下落(level=8, falling=true) -> 方块level=8

    if (VanillaBlocks::LAVA == nullptr) {
        return nullptr;
    }

    // 获取岩浆的方块
    Block* lavaBlock = isSource(state) ? VanillaBlocks::LAVA : VanillaBlocks::LAVA;

    if (lavaBlock == nullptr) {
        return nullptr;
    }

    // 计算方块level
    i32 blockLevel;
    if (isSource(state)) {
        blockLevel = state.isFalling() ? SOURCE_LEVEL : 0;
    } else {
        i32 fluidLevel = state.getLevel();
        blockLevel = SOURCE_LEVEL - fluidLevel;
        if (state.isFalling()) {
            blockLevel = SOURCE_LEVEL;
        }
    }

    // 设置LEVEL属性
    const auto& levelProp = BlockStateProperties::LEVEL_0_15();
    return &lavaBlock->defaultState().with(levelProp, blockLevel);
}

i32 LavaFluid::getTickDelay(IWorld& world) const
{
    return world.isUltraWarm() ? 10 : 30;
}

i32 LavaFluid::getTickDelay(
    IWorld& world, const BlockPos& pos, const FluidState& state, const FluidState& correctState) const
{
    i32 tickDelay = getTickDelay(world);
    if (!state.isEmpty() && !correctState.isEmpty() && !state.isFalling() && !correctState.isFalling() &&
        correctState.getActualHeight(world, pos) > state.getActualHeight(world, pos)) {
        u64 seed = world.seed();
        seed ^= static_cast<u64>(pos.x) * 341873128712ULL;
        seed ^= static_cast<u64>(pos.y) * 132897987541ULL;
        seed ^= static_cast<u64>(pos.z) * 42317861ULL;
        seed ^= world.currentTick();

        math::Random random(seed);
        if (random.nextInt(4) != 0) {
            tickDelay *= 4;
        }
    }

    return tickDelay;
}

bool LavaFluid::canDisplace(
    const FluidState& state, IWorld& world, const BlockPos& pos, const Fluid& fluid, Direction dir) const
{
    (void)dir;
    return state.getActualHeight(world, pos) >= 0.44444445f && fluid.isIn(FluidTags::WATER());
}

void LavaFluid::randomTick(IWorld& world, const BlockPos& pos, const FluidState& state, math::IRandom& random)
{
    if (!world.doFireTick()) {
        return;
    }

    (void)state;

    i32 i = random.nextInt(3);

    if (i > 0) {
        // 向上搜索可燃位置
        BlockPos checkPos = pos;
        for (i32 j = 0; j < i; ++j) {
            checkPos = BlockPos(checkPos.x + random.nextInt(3) - 1, checkPos.y + 1, checkPos.z + random.nextInt(3) - 1);

            if (!world.isWithinWorldBounds(checkPos)) {
                return;
            }

            const BlockState* blockState = world.getBlockState(checkPos);
            if (blockState == nullptr) {
                return;
            }

            if (blockState->isAir()) {
                // 检查周围是否有可燃方块
                if (_isSurroundingBlockFlammable(world, checkPos)) {
                    const BlockState& fireState = blocks::FireBlock::getFireState(world, checkPos);
                    world.setBlockState(checkPos, &fireState, 3);
                    return;
                }
            } else if (blockState->owner().material().blocksMovement()) {
                // 阻挡移动的方块，停止搜索
                return;
            }
        }
    } else {
        // i == 0: 水平搜索，在可燃方块上方点火
        for (i32 k = 0; k < 3; ++k) {
            BlockPos checkPos = BlockPos(pos.x + random.nextInt(3) - 1, pos.y, pos.z + random.nextInt(3) - 1);

            if (!world.isWithinWorldBounds(checkPos)) {
                continue;
            }

            const BlockState* blockState = world.getBlockState(checkPos);
            if (blockState == nullptr) {
                continue;
            }

            // 检查上方是否是空气，且当前方块是否可燃
            BlockPos abovePos = checkPos.up();
            if (!world.isWithinWorldBounds(abovePos)) {
                continue;
            }

            const BlockState* aboveState = world.getBlockState(abovePos);
            if (aboveState != nullptr && aboveState->isAir() && _isBlockFlammable(world, checkPos)) {
                const BlockState& fireState = blocks::FireBlock::getFireState(world, abovePos);
                world.setBlockState(abovePos, &fireState, 3);
            }
        }
    }
}

bool LavaFluid::_isSurroundingBlockFlammable(IWorld& world, const BlockPos& pos) const
{
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(Directions::toBlockFace(dir));
        if (_isBlockFlammable(world, neighborPos)) {
            return true;
        }
    }
    return false;
}

bool LavaFluid::_isBlockFlammable(IWorld& world, const BlockPos& pos) const
{
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    const BlockState* blockState = world.getBlockState(pos);
    if (blockState == nullptr) {
        return false;
    }

    return blockState->owner().material().isFlammable();
}

void LavaFluid::beforeReplacingBlock(IWorld& world, const BlockPos& pos, const BlockState* state)
{
    // 岩浆替换方块前触发效果
    _triggerEffects(world, pos);

    // MC Java 中流体替换方块时不调用 spawnAfterBreak，仅调用 onReplaced
    MC_UNUSED(state);
}

void LavaFluid::_triggerEffects(IWorld& world, const BlockPos& pos)
{
    // 触发烟雾和嘶嘶声音效果
    world.playEvent(world::WorldEvents::LAVA_EXTINGUISH, pos, 0);
}

void LavaFluid::flowInto(
    IWorld& world, const BlockPos& pos, const BlockState* blockState, Direction dir, const FluidState& fluidState)
{
    // 只有在向下流动时(direction == DOWN)才检查水交互
    // 当岩浆向下流入水时，把目标位置变成石头
    if (dir == Direction::Down) {
        // 检查目标位置是否有水
        const FluidState* targetFluid = world.getFluidState(pos);
        if (targetFluid != nullptr && !targetFluid->isEmpty() && targetFluid->getFluid().isIn(FluidTags::WATER())) {
            // 岩浆向下流入水 -> 生成石头
            if (VanillaBlocks::STONE != nullptr) {
                world.setBlockState(pos, &VanillaBlocks::STONE->defaultState(), 3);
            }
            _triggerEffects(world, pos);
            return;
        }
    }

    // 调用父类方法处理正常流动
    FlowingFluid::flowInto(world, pos, blockState, dir, fluidState);
}

bool LavaFluid::isEquivalentTo(const Fluid& fluid) const noexcept
{
    // 岩浆和流动岩浆视为等效
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" && (loc.path() == "lava" || loc.path() == "flowing_lava");
}

i32 LavaFluid::getLevelDecrease(IWorld& world) const
{
    return world.isUltraWarm() ? 1 : 2;
}

i32 LavaFluid::getSpreadDistance(IWorld& world) const
{
    return world.isUltraWarm() ? 4 : 2;
}

// ============================================================================
// LavaSourceFluid 实现
// ============================================================================

LavaSourceFluid::LavaSourceFluid()
{
    // 源头没有LEVEL属性，只有FALLING
    auto container =
        StateContainer<Fluid, FluidState>::Builder(*this)
            .add(FluidProperties::FALLING())
            .create([this](const Fluid& fluid,
                        auto values,
                        const std::vector<StateHolder<Fluid, FluidState>::PropertyLayout>* propertyLayouts,
                        const std::vector<FluidState*>* allStates,
                        u32 id) {
                return std::make_unique<FluidState>(fluid, std::move(values), propertyLayouts, allStates, id);
            });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

FlowingFluid& LavaSourceFluid::getFlowing()
{
    if (m_flowingCache == nullptr) {
        m_flowingCache =
            static_cast<FlowingFluid*>(FluidRegistry::instance().getFluid(ResourceLocation("minecraft:flowing_lava")));
    }
    return *m_flowingCache;
}

bool LavaSourceFluid::isEquivalentTo(const Fluid& fluid) const noexcept
{
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" && (loc.path() == "lava" || loc.path() == "flowing_lava");
}

// ============================================================================
// LavaFlowingFluid 实现
// ============================================================================

LavaFlowingFluid::LavaFlowingFluid()
{
    auto container =
        StateContainer<Fluid, FluidState>::Builder(*this)
            .add(FluidProperties::LEVEL_1_8())
            .add(FluidProperties::FALLING())
            .create([this](const Fluid& fluid,
                        auto values,
                        const std::vector<StateHolder<Fluid, FluidState>::PropertyLayout>* propertyLayouts,
                        const std::vector<FluidState*>* allStates,
                        u32 id) {
                return std::make_unique<FluidState>(fluid, std::move(values), propertyLayouts, allStates, id);
            });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

i32 LavaFlowingFluid::getLevel(const FluidState& state) const
{
    auto& levelProp = FluidProperties::LEVEL_1_8();
    auto opt = state.getOptional(levelProp);
    return opt.has_value() ? opt.value() : SOURCE_LEVEL;
}

FlowingFluid& LavaFlowingFluid::getStill()
{
    if (m_stillCache == nullptr) {
        m_stillCache =
            static_cast<FlowingFluid*>(FluidRegistry::instance().getFluid(ResourceLocation("minecraft:lava")));
    }
    return *m_stillCache;
}

bool LavaFlowingFluid::isEquivalentTo(const Fluid& fluid) const noexcept
{
    const auto& loc = fluid.fluidLocation();
    return loc.namespace_() == "minecraft" && (loc.path() == "lava" || loc.path() == "flowing_lava");
}

} // namespace fluid
} // namespace mc
