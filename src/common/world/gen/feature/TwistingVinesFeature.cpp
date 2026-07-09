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

#include "TwistingVinesFeature.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc::world::gen::feature {

ConfiguredTwistingVinesFeature::ConfiguredTwistingVinesFeature(std::unique_ptr<TwistingVinesFeatureConfig> config)
    : m_config(std::move(config))
{}

namespace {

/// MC: isEmptyBlock = state == null || state.isAir()
bool isEmptyBlock(WorldGenRegion& region, const BlockPos& pos)
{
    const BlockState* state = region.getBlockState(pos);
    return state == nullptr || state->isAir();
}

/// MC: isInvalidPlacementLocation：origin 非空 或 below 非
/// NETHERRACK|WARPED_NYLIUM|WARPED_WART_BLOCK。
bool isInvalidPlacementLocation(WorldGenRegion& world, const BlockPos& pos)
{
    if (!isEmptyBlock(world, pos)) {
        return true;
    }
    const BlockState* below = world.getBlockState(pos.down());
    if (below == nullptr) {
        return true;
    }
    return !(below->is(VanillaBlocks::NETHERRACK) || below->is(VanillaBlocks::WARPED_NYLIUM) ||
        below->is(VanillaBlocks::WARPED_WART_BLOCK));
}

/// MC: findFirstAirBlockAboveGround：向下移动直到非空（或越界），再上移一格返回 true。
bool findFirstAirBlockAboveGround(WorldGenRegion& world, BlockPosMutable& pos)
{
    do {
        pos.move(0, -1, 0);
        if (!world.isWithinWorldBounds(pos.x, pos.y, pos.z)) {
            return false;
        }
        const BlockState* state = world.getBlockState(pos);
        if (state == nullptr) {
            // 未加载/越界视为非空（停止下移），与 MC isAir() == false 等价。
            break;
        }
        if (!state->isAir()) {
            break;
        }
    } while (true);
    pos.move(0, 1, 0);
    return true;
}

/// MC TwistingVinesFeature.placeWeepingVinesColumn（向上生长，名字沿用 MC）。
/// p_225304_=length, p_225305_=17(minAge), p_225306_=25(maxAge)。
void placeTwistingVinesColumn(
    WorldGenRegion& world, math::Random& random, BlockPosMutable pos, i32 length, i32 minAge, i32 maxAge)
{
    // MC: for (int i = 1; i <= length; i++) —— 注意从 1 开始。
    for (i32 i = 1; i <= length; ++i) {
        if (isEmptyBlock(world, pos)) {
            if (i == length || !isEmptyBlock(world, pos.up())) {
                const i32 age = random.nextInt(minAge, maxAge);
                const BlockState* head = &VanillaBlocks::TWISTING_VINES->defaultState().with(
                    BlockStateProperties::AGE_0_25(), std::min(age, 25));
                world.setBlockState(pos, head, 2);
                break;
            }
            world.setBlockState(pos, &VanillaBlocks::TWISTING_VINES_PLANT->defaultState(), 2);
        }
        pos.move(Direction::Up);
    }
}

} // namespace

bool ConfiguredTwistingVinesFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin) const
{
    if (m_config == nullptr) {
        return false;
    }
    // MC: if (isInvalidPlacementLocation(world, origin)) return false;
    if (isInvalidPlacementLocation(region, origin)) {
        return false;
    }

    const i32 spreadWidth = m_config->spreadWidth;
    const i32 spreadHeight = m_config->spreadHeight;
    const i32 maxHeight = m_config->maxHeight;

    // MC: for (int l = 0; l < i*i; l++) ...
    BlockPosMutable probe;
    for (i32 l = 0; l < spreadWidth * spreadWidth; ++l) {
        // MC: set(blockpos).move(nextInt(-i,i), nextInt(-j,j), nextInt(-i,i))
        probe.set(origin.x + random.nextInt(-spreadWidth, spreadWidth),
            origin.y + random.nextInt(-spreadHeight, spreadHeight),
            origin.z + random.nextInt(-spreadWidth, spreadWidth));
        if (findFirstAirBlockAboveGround(region, probe) && !isInvalidPlacementLocation(region, probe)) {
            i32 length = random.nextInt(1, maxHeight);
            if (random.nextInt(6) == 0) {
                length *= 2;
            }
            if (random.nextInt(5) == 0) {
                length = 1;
            }
            placeTwistingVinesColumn(region, random, probe, length, 17, 25);
        }
    }
    return true;
}

} // namespace mc::world::gen::feature
