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

#include "PointedDripstoneFeature.hpp"

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/DripstoneUtils.hpp"
#include "common/world/gen/structure/Structure.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

namespace mc::world::gen::feature::cave {

namespace {

/// MC Direction.getRandom(random)：6 个方向随机取一个。
Direction randomDirection(math::IRandom& random)
{
    static constexpr std::array<Direction, 6> kAll = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    return kAll[static_cast<std::size_t>(random.nextInt(6))];
}

} // namespace

ConfiguredPointedDripstoneFeature::ConfiguredPointedDripstoneFeature(
    std::unique_ptr<PointedDripstoneConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredPointedDripstoneFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& pos) const
{
    return m_feature.place(region, random, pos, *m_config);
}

bool PointedDripstoneFeature::place(
    IWorld& world, math::Random& random, const BlockPos& pos, const PointedDripstoneConfig& config)
{
    const std::optional<Direction> direction = getTipDirection(world, pos, random);
    if (!direction.has_value()) {
        return false;
    }

    const Direction dir = *direction;
    const BlockPos basePos = pos.offset(Directions::opposite(dir));
    createPatchOfDripstoneBlocks(world, random, basePos, config);

    const bool taller = random.nextFloat() < config.chanceOfTallerDripstone &&
        DripstoneUtils::isEmptyOrWater(world.getBlockState(pos.offset(dir)));
    const i32 height = taller ? 2 : 1;
    DripstoneUtils::growPointedDripstone(world, pos, dir, height, false);
    return true;
}

std::optional<Direction> PointedDripstoneFeature::getTipDirection(
    IWorld& world, const BlockPos& pos, math::Random& random)
{
    const bool aboveIsBase = DripstoneUtils::isDripstoneBase(world.getBlockState(pos.up()));
    const bool belowIsBase = DripstoneUtils::isDripstoneBase(world.getBlockState(pos.down()));
    if (aboveIsBase && belowIsBase) {
        return random.nextBoolean() ? Direction::Down : Direction::Up;
    }
    if (aboveIsBase) {
        return Direction::Down;
    }
    if (belowIsBase) {
        return Direction::Up;
    }
    return std::nullopt;
}

void PointedDripstoneFeature::createPatchOfDripstoneBlocks(
    IWorld& world, math::Random& random, const BlockPos& pos, const PointedDripstoneConfig& config)
{
    DripstoneUtils::placeDripstoneBlockIfPossible(world, pos);

    for (Direction direction : Directions::horizontal()) {
        if (!(random.nextFloat() > config.chanceOfDirectionalSpread)) {
            const BlockPos pos1 = pos.offset(direction);
            DripstoneUtils::placeDripstoneBlockIfPossible(world, pos1);
            if (!(random.nextFloat() > config.chanceOfSpreadRadius2)) {
                const BlockPos pos2 = pos1.offset(randomDirection(random));
                DripstoneUtils::placeDripstoneBlockIfPossible(world, pos2);
                if (!(random.nextFloat() > config.chanceOfSpreadRadius3)) {
                    const BlockPos pos3 = pos2.offset(randomDirection(random));
                    DripstoneUtils::placeDripstoneBlockIfPossible(world, pos3);
                }
            }
        }
    }
}

} // namespace mc::world::gen::feature::cave
