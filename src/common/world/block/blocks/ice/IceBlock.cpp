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

#include "common/world/block/blocks/ice/IceBlock.hpp"

#include "common/core/Types.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "util/math/random/IRandom.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// 常量与冰块替换助手
// ============================================================================

namespace {

/// 冰融化的最小光照等级阈值
constexpr i32 MELT_LIGHT_LEVEL = 11;

thread_local bool s_skipIceReplacementCallback = false;

class IceReplacementGuard {
public:
    IceReplacementGuard()
    {
        s_skipIceReplacementCallback = true;
    }

    ~IceReplacementGuard()
    {
        s_skipIceReplacementCallback = false;
    }
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
    world.setBlockState(pos, replacementState, world::BlockUpdateFlags::UPDATE_ALL);
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

void IceBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
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

    // 冰在方块光照足够时融化：门槛为 blockLight > 11 - opacity（opacity 即 lightBlock）。
    // 冰的 lightBlock=1（vanilla IceBlock#solidRender=false，见 isSolidRender/getOpacity），故条件为
    // blockLight > 10，即方块光照 >= 11 时融化。仅检查方块光照，阳光（天空光照）不直接融化冰。
    u8 blockLight = world.getBlockLight(pos);
    i32 opacity = state.getOpacity();

    if (blockLight > MELT_LIGHT_LEVEL - opacity) {
        meltIce(world, pos);
    }
}

} // namespace blocks
} // namespace mc
