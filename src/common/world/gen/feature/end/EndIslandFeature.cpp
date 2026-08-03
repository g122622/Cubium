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
 * IMPLIED, INCLUDING NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "EndIslandFeature.hpp"

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"

namespace mc {

// ============================================================================
// EndIslandFeature
// ============================================================================

bool EndIslandFeature::place(WorldGenRegion& world, math::Random& random, const BlockPos& pos)
{
    // 生成锥形/泪滴形末地石岛屿
    // 初始半径在 4.0-6.0 之间随机（nextInt(3) + 4.0）
    f32 radius = static_cast<f32>(random.nextInt(3)) + 4.0f;

    const BlockState* endStone = &VanillaBlocks::END_STONE->defaultState();

    bool placed = false;
    i32 layer = 0;

    while (radius > 0.5f) {
        i32 minOffset = math::floorTo<i32>(-radius);
        i32 maxOffset = math::ceilTo<i32>(radius);
        f32 radiusSq = (radius + 1.0f) * (radius + 1.0f);

        for (i32 dx = minOffset; dx <= maxOffset; ++dx) {
            for (i32 dz = minOffset; dz <= maxOffset; ++dz) {
                if (static_cast<f32>(dx * dx + dz * dz) <= radiusSq) {
                    BlockPos blockPos(pos.x + dx, pos.y - layer, pos.z + dz);
                    world.setBlockState(blockPos, endStone, 3);
                    placed = true;
                }
            }
        }

        // 每层向下收缩半径
        radius -= static_cast<f32>(random.nextInt(2)) + 0.5f;
        ++layer;
    }

    return placed;
}

// ============================================================================
// ConfiguredEndIslandFeature
// ============================================================================

ConfiguredEndIslandFeature::ConfiguredEndIslandFeature(const char* featureName)
    : m_name(featureName)
{}

bool ConfiguredEndIslandFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    return EndIslandFeature::place(region, random, pos);
}

} // namespace mc
