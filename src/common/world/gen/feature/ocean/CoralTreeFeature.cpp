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

#include "CoralTreeFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/coral/CoralBlock.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ocean/CoralFeature.hpp"
#include <cstddef>

namespace mc {

bool CoralTreeFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config)
{
    const i32 trunkHeight = random.nextInt(3) + 1;

    BlockPos topPos = pos;
    i32 placedTrunk = 0;
    for (i32 y = 0; y < trunkHeight; ++y) {
        const BlockPos trunkPos(pos.x, pos.y + y, pos.z);
        if (!placeCoralWithDecorations(world, random, trunkPos, config.color, config.isDead, config.includeWallFan)) {
            break;
        }
        topPos = trunkPos;
        ++placedTrunk;
    }

    if (placedTrunk == 0) {
        return false;
    }

    // 2-4个分支
    const auto horizontalDirections = Directions::horizontal();
    const i32 branchCount = random.nextInt(3) + 2;
    for (i32 i = 0; i < branchCount; ++i) {
        const Direction direction = horizontalDirections[static_cast<size_t>(random.nextInt(4))];
        // 分支长度2-6
        _generateBranch(world,
            random,
            topPos,
            config.color,
            config.isDead,
            direction,
            random.nextInt(5) + 2,
            config.includeWallFan);
    }

    return true;
}

void CoralTreeFeature::_generateBranch(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    Direction direction,
    i32 length,
    bool includeDecorations)
{
    BlockPos currentPos = pos;
    for (i32 i = 0; i < length; ++i) {
        currentPos = currentPos.offset(direction);
        if (i > 0 && random.nextFloat() < 0.25f) {
            currentPos = currentPos.offset(Direction::Up);
        }

        if (!placeCoralWithDecorations(world, random, currentPos, color, isDead, includeDecorations)) {
            break;
        }
    }
}

} // namespace mc
