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

#include "ScatteredOreFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/Feature.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace mc {

namespace {

/// MC ScatteredOreFeature.MAX_DIST_FROM_ORIGIN = 7。
constexpr i32 kMaxDistFromOrigin = 7;

/// MC OreFeature.canPlaceOre：target.test 失败返回 false；
/// discardChance<=0 跳过空气检查；否则 nextFloat>=discardChance 才检查，相邻有空气则不放。
bool canPlaceOre(const BlockState* current,
    WorldGenRegion& region,
    math::Random& random,
    const OreFeatureConfig& config,
    const OreTarget& target,
    const BlockPos& pos)
{
    if (current == nullptr || target.target == nullptr) {
        return false;
    }
    if (!target.target->test(*current, random)) {
        return false;
    }
    if (config.discardChanceOnAirExposure <= 0.0f) {
        return true;
    }
    if (config.discardChanceOnAirExposure >= 1.0f) {
        return false;
    }
    if (random.nextFloat() >= config.discardChanceOnAirExposure) {
        return true;
    }
    // 相邻 6 面有空气则不放。
    static constexpr i32 offsets[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (i32 d = 0; d < 6; ++d) {
        const BlockState* neighbor =
            region.getBlockState(pos.x + offsets[d][0], pos.y + offsets[d][1], pos.z + offsets[d][2]);
        if (neighbor == nullptr || neighbor->isAir()) {
            return false;
        }
    }
    return true;
}

/// MC ScatteredOreFeature.getRandomPlacementInOneAxisRelativeToOrigin：round((nextFloat - nextFloat) * j)。
i32 randomAxisOffset(math::Random& random, i32 j)
{
    return static_cast<i32>(std::round((random.nextFloat() - random.nextFloat()) * static_cast<f32>(j)));
}

} // namespace

ConfiguredScatteredOreFeature::ConfiguredScatteredOreFeature(
    std::unique_ptr<OreFeatureConfig> featureConfig, const char* featureName)
    : m_config(std::move(featureConfig))
    , m_name(featureName)
{}

bool ConfiguredScatteredOreFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin) const
{
    if (!m_config || m_config->targets.empty()) {
        return false;
    }

    // MC: i = nextInt(size + 1)，散点放置 i 次。
    const i32 count = random.nextInt(m_config->size + 1);

    for (i32 j = 0; j < count; ++j) {
        const i32 dist = std::min(j, kMaxDistFromOrigin);
        const i32 ox = randomAxisOffset(random, dist);
        const i32 oy = randomAxisOffset(random, dist);
        const i32 oz = randomAxisOffset(random, dist);
        const BlockPos pos(origin.x + ox, origin.y + oy, origin.z + oz);

        const BlockState* current = region.getBlockState(pos);
        for (const auto& target : m_config->targets) {
            if (canPlaceOre(current, region, random, *m_config, target, pos)) {
                if (target.state != nullptr) {
                    region.setBlockState(pos, target.state, 2);
                }
                break;
            }
        }
    }

    return true;
}

} // namespace mc
