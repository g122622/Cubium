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

#include "WeepingVinesFeature.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc::world::gen::feature {

namespace {

/// MC: isEmptyBlock = state == null || state.isAir()
bool isEmptyBlock(WorldGenRegion& region, const BlockPos& pos)
{
    const BlockState* state = region.getBlockState(pos);
    return state == nullptr || state->isAir();
}

/// MC: 邻居为 NETHERRACK 或 NETHER_WART_BLOCK
bool isRoofBlock(const BlockState* state)
{
    if (state == nullptr) {
        return false;
    }
    return state->is(VanillaBlocks::NETHERRACK) || state->is(VanillaBlocks::NETHER_WART_BLOCK);
}

/// MC WeepingVinesFeature.placeRoofNetherWart。
void placeRoofNetherWart(WorldGenRegion& world, math::Random& random, const BlockPos& origin)
{
    world.setBlockState(origin, &VanillaBlocks::NETHER_WART_BLOCK->defaultState(), 2);

    static constexpr Direction kAllDirections[] = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    BlockPosMutable probe;
    BlockPosMutable neighbor;
    for (i32 i = 0; i < 200; ++i) {
        // MC setWithOffset(origin, nextInt(6)-nextInt(6), nextInt(2)-nextInt(5), nextInt(6)-nextInt(6))
        probe.set(origin.x + (random.nextInt(6) - random.nextInt(6)),
            origin.y + (random.nextInt(2) - random.nextInt(5)),
            origin.z + (random.nextInt(6) - random.nextInt(6)));
        if (!isEmptyBlock(world, probe)) {
            continue;
        }
        i32 count = 0;
        for (Direction direction : kAllDirections) {
            neighbor.set(probe.x + Directions::xOffset(direction),
                probe.y + Directions::yOffset(direction),
                probe.z + Directions::zOffset(direction));
            if (isRoofBlock(world.getBlockState(neighbor))) {
                ++count;
            }
            if (count > 1) {
                break;
            }
        }
        if (count == 1) {
            world.setBlockState(probe, &VanillaBlocks::NETHER_WART_BLOCK->defaultState(), 2);
        }
    }
}

/// MC WeepingVinesFeature.placeWeepingVinesColumn（向下生长）。
/// p_225356_=length, p_225357_=17(minAge), p_225358_=25(maxAge)。
void placeWeepingVinesColumn(
    WorldGenRegion& world, math::Random& random, BlockPosMutable pos, i32 length, i32 minAge, i32 maxAge)
{
    for (i32 i = 0; i <= length; ++i) {
        if (isEmptyBlock(world, pos)) {
            if (i == length || !isEmptyBlock(world, pos.down())) {
                const i32 age = random.nextInt(minAge, maxAge);
                const BlockState* head = &VanillaBlocks::WEEPING_VINES->defaultState().with(
                    BlockStateProperties::AGE_0_25(), std::min(age, 25));
                world.setBlockState(pos, head, 2);
                break;
            }
            world.setBlockState(pos, &VanillaBlocks::WEEPING_VINES_PLANT->defaultState(), 2);
        }
        pos.move(Direction::Down);
    }
}

/// MC WeepingVinesFeature.placeRoofWeepingVines。
void placeRoofWeepingVines(WorldGenRegion& world, math::Random& random, const BlockPos& origin)
{
    BlockPosMutable probe;
    for (i32 i = 0; i < 100; ++i) {
        // MC setWithOffset(origin, nextInt(8)-nextInt(8), nextInt(2)-nextInt(7), nextInt(8)-nextInt(8))
        probe.set(origin.x + (random.nextInt(8) - random.nextInt(8)),
            origin.y + (random.nextInt(2) - random.nextInt(7)),
            origin.z + (random.nextInt(8) - random.nextInt(8)));
        if (!isEmptyBlock(world, probe)) {
            continue;
        }
        if (isRoofBlock(world.getBlockState(probe.up()))) {
            i32 length = random.nextInt(1, 8);
            if (random.nextInt(6) == 0) {
                length *= 2;
            }
            if (random.nextInt(5) == 0) {
                length = 1;
            }
            placeWeepingVinesColumn(world, random, probe, length, 17, 25);
        }
    }
}

} // namespace

bool ConfiguredWeepingVinesFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin) const
{
    // MC: if (!isEmptyBlock(origin)) return false;
    if (!isEmptyBlock(region, origin)) {
        return false;
    }
    // MC: above = getBlockState(origin.above()); if (!NETHERRACK && !NETHER_WART_BLOCK) return false;
    if (!isRoofBlock(region.getBlockState(origin.up()))) {
        return false;
    }
    placeRoofNetherWart(region, random, origin);
    placeRoofWeepingVines(region, random, origin);
    return true;
}

} // namespace mc::world::gen::feature
