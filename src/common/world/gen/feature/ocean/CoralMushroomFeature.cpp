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

#include "CoralMushroomFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/coral/CoralBlock.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ocean/CoralFeature.hpp"

namespace mc {

bool CoralMushroomFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config)
{
    const i32 dimX = random.nextInt(3) + 3;       // 3~5
    const i32 dimY = random.nextInt(3) + 3;       // 3~5
    const i32 dimZ = random.nextInt(3) + 3;       // 3~5
    const i32 downOffset = random.nextInt(3) + 1; // 1~3

    bool placedAny = false;

    for (i32 i1 = 0; i1 <= dimX; ++i1) {
        for (i32 j1 = 0; j1 <= dimY; ++j1) {
            for (i32 k1 = 0; k1 <= dimZ; ++k1) {
                // 创建空心蘑菇盖形状
                const bool cond1 = (i1 != 0 && i1 != dimX) || (j1 != 0 && j1 != dimY);
                const bool cond2 = (k1 != 0 && k1 != dimZ) || (j1 != 0 && j1 != dimY);
                const bool cond3 = (i1 != 0 && i1 != dimX) || (k1 != 0 && k1 != dimZ);
                const bool cond4 = (i1 == 0 || i1 == dimX || j1 == 0 || j1 == dimY || k1 == 0 || k1 == dimZ);

                if (cond1 && cond2 && cond3 && cond4 && !(random.nextFloat() < 0.1f)) {
                    BlockPos capPos(pos.x + i1, pos.y + j1 - downOffset, pos.z + k1);
                    if (placeCoralWithDecorations(
                            world, random, capPos, config.color, config.isDead, config.includeWallFan)) {
                        placedAny = true;
                    }
                }
            }
        }
    }

    return placedAny;
}

void CoralMushroomFeature::_generateCap(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    i32 radius,
    bool includeDecorations)
{
    (void)world;
    (void)random;
    (void)pos;
    (void)color;
    (void)isDead;
    (void)radius;
    (void)includeDecorations;
    // 已被 place() 中的算法替代，此方法不再使用
}

} // namespace mc
