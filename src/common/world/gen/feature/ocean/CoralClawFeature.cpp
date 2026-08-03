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

#include "CoralClawFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/coral/CoralBlock.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ocean/CoralFeature.hpp"
#include <array>
#include <cstddef>

namespace mc {

bool CoralClawFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config)
{
    bool placedAny = placeCoralWithDecorations(world, random, pos, config.color, config.isDead, config.includeWallFan);

    const auto directions = Directions::horizontal();
    const Direction mainDirection = directions[static_cast<size_t>(random.nextInt(4))];
    const std::array<Direction, 3> clawDirections = {
        mainDirection, Directions::rotateY(mainDirection), Directions::rotateYCCW(mainDirection)};

    for (Direction direction : clawDirections) {
        if (random.nextFloat() < 0.75f) {
            _generateClaw(world, random, pos, config.color, config.isDead, direction, config.includeWallFan);
            placedAny = true;
        }
    }

    return placedAny;
}

void CoralClawFeature::_generateClaw(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    Direction direction,
    bool includeDecorations)
{
    i32 clawLength = random.nextInt(3) + 2;
    BlockPos currentPos = pos;
    Direction currentDirection = direction;

    for (i32 i = 0; i < clawLength; ++i) {
        currentPos = currentPos.offset(currentDirection);
        if (i > 0 && random.nextFloat() < 0.35f) {
            currentPos = currentPos.offset(Direction::Up);
        }

        if (!placeCoralWithDecorations(world, random, currentPos, color, isDead, includeDecorations)) {
            break;
        }

        if (i > 0 && random.nextFloat() < 0.25f) {
            currentDirection =
                random.nextBoolean() ? Directions::rotateY(currentDirection) : Directions::rotateYCCW(currentDirection);
        }
    }
}

} // namespace mc
