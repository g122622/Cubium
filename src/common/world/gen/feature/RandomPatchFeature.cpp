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

#include "RandomPatchFeature.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/placement/PlacedFeature.hpp"

namespace mc::world::gen::feature {

bool RandomPatchFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& origin,
    const RandomPatchFeatureConfig& config)
{
    if (config.feature == nullptr) {
        return false;
    }

    i32 placed = 0;
    // MC RandomPatchFeature: j=xzSpread+1, k=ySpread+1；偏移 nextInt(j)-nextInt(j)（三角形分布）
    const i32 j = config.xzSpread + 1;
    const i32 k = config.ySpread + 1;

    for (i32 attempt = 0; attempt < config.tries; ++attempt) {
        const i32 dx = random.nextInt(j) - random.nextInt(j);
        const i32 dy = random.nextInt(k) - random.nextInt(k);
        const i32 dz = random.nextInt(j) - random.nextInt(j);
        const BlockPos candidate(origin.x + dx, origin.y + dy, origin.z + dz);

        // 委托内联 PlacedFeature：先走其 placement 链，再 place 配置化特征
        if (config.feature->place(region, chunk, generator, random, candidate)) {
            ++placed;
        }
    }

    return placed > 0;
}

bool ConfiguredRandomPatchFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& origin) const
{
    if (m_config == nullptr) {
        return false;
    }
    return RandomPatchFeature::place(region, chunk, generator, random, origin, *m_config);
}

} // namespace mc::world::gen::feature
