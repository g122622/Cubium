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

#include "common/world/block/blocks/sculk/SculkBlock.hpp"

#include "common/util/math/MathUtils.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "world/block/blocks/sculk/SculkSpreader.hpp"
#include "world/fluid/Fluid.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

namespace mc {
namespace blocks {

// ============================================================================
// SculkBlock
// ============================================================================

i32 SculkBlock::attemptUseCharge(ChargeCursor& cursor,
    IWorld& world,
    const BlockPos& origin,
    math::IRandom& random,
    SculkSpreader& spreader,
    bool /*shouldUpdateBlocks*/)
{
    // MC SculkBlock.attemptUseCharge:
    //   charge==0 → 直接返回；nextInt(chargeDecayRate)==0 才进入衰减/生长。
    //   距 origin 在 noGrowthRadius 内（flag=true）→ 不生长，按 additionalDecayRate 衰减。
    //   否则 canPlaceGrowth 且 nextInt(growthSpawnCost) < charge → 在上方放生长物，charge -= growthSpawnCost。
    //   否则按 additionalDecayRate 概率衰减（flag 时 -1，否则 -getDecayPenalty）。
    const i32 charge = cursor.charge();
    if (charge == 0 || random.nextInt(spreader.chargeDecayRate()) != 0) {
        return charge;
    }
    const BlockPos pos = cursor.pos();
    const i64 noGrowthSq = static_cast<i64>(spreader.noGrowthRadius()) * spreader.noGrowthRadius();
    const bool flag = pos.distanceSq(origin) < noGrowthSq;
    if (!flag && canPlaceGrowth(world, pos)) {
        const i32 cost = spreader.growthSpawnCost();
        if (random.nextInt(cost) < charge) {
            const BlockPos above = pos.up();
            const BlockState* growth = getRandomGrowthState(world, above, random, spreader.isWorldGeneration());
            if (growth != nullptr) {
                world.setBlockState(above, growth, world::BlockUpdateFlags::UPDATE_ALL);
            }
        }
        return std::max(0, charge - cost);
    }
    if (random.nextInt(spreader.additionalDecayRate()) != 0) {
        return charge;
    }
    return charge - (flag ? 1 : getDecayPenalty(spreader, pos, origin, charge));
}

const BlockState* SculkBlock::getRandomGrowthState(
    IWorld& world, const BlockPos& pos, math::IRandom& random, bool worldGen)
{
    // MC: nextInt(11)==0 → shrieker(CAN_SUMMON=worldGen)，否则 sensor。
    //     若方块有 WATERLOGGED 属性且 pos 处有流体 → 设 WATERLOGGED=true。
    const Block* block = random.nextInt(11) == 0 ? VanillaBlocks::SCULK_SHRIEKER : VanillaBlocks::SCULK_SENSOR;
    if (block == nullptr) {
        return nullptr;
    }
    const BlockState& state = block->defaultState();
    const BlockState* result = &state;
    if (block == VanillaBlocks::SCULK_SHRIEKER) {
        result = &result->with(BlockStateProperties::CAN_SUMMON(), worldGen);
    }
    // MC: hasProperty(WATERLOGGED) && !getFluidState(pos).isEmpty()
    const BlockState* posState = world.getBlockState(pos);
    const fluid::FluidState* fluid = posState != nullptr ? posState->getFluidState() : nullptr;
    if (fluid != nullptr && !fluid->isEmpty()) {
        result = &result->with(BlockStateProperties::WATERLOGGED(), true);
    }
    return result;
}

bool SculkBlock::canPlaceGrowth(IWorld& world, const BlockPos& pos)
{
    // MC: 上方为空气 或 (WATER 且 水源) → 继续；否则 false。
    //     4×3×4（offset(-4,0,-4)→(4,2,4)）范围内 SCULK_SENSOR+SCULK_SHRIEKER 计数 ≤ 2。
    const BlockState* above = world.getBlockState(pos.up());
    const bool air = (above == nullptr) || above->isAir();
    const bool waterSource = above != nullptr && above->is(VanillaBlocks::WATER) && above->getFluidState() != nullptr &&
        above->getFluidState()->isSource();
    if (!air && !waterSource) {
        return false;
    }
    i32 count = 0;
    for (i32 dx = -4; dx <= 4; ++dx) {
        for (i32 dy = 0; dy <= 2; ++dy) {
            for (i32 dz = -4; dz <= 4; ++dz) {
                const BlockState* s = world.getBlockState(pos + BlockPos{dx, dy, dz});
                if (s != nullptr && (s->is(VanillaBlocks::SCULK_SENSOR) || s->is(VanillaBlocks::SCULK_SHRIEKER))) {
                    ++count;
                    if (count > 2) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

i32 SculkBlock::getDecayPenalty(const SculkSpreader& spreader, const BlockPos& pos, const BlockPos& origin, i32 charge)
{
    // MC: f = (sqrt(distSqr) - noGrowthRadius)²; j = (24 - noGrowthRadius)²;
    //     f1 = min(1, f/j); penalty = max(1, (i32)(charge * f1 * 0.5))
    const i32 noGrowth = spreader.noGrowthRadius();
    const f32 dist = std::sqrt(static_cast<f64>(pos.distanceSq(origin)));
    const f32 f = math::square(dist - static_cast<f32>(noGrowth));
    const f32 j = static_cast<f32>(math::square(24 - noGrowth));
    const f32 f1 = std::min(1.0f, f / j);
    return std::max(1, static_cast<i32>(static_cast<f32>(charge) * f1 * 0.5f));
}

} // namespace blocks
} // namespace mc
