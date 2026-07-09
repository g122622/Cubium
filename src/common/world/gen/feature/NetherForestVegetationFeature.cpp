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

#include "NetherForestVegetationFeature.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc::world::gen::feature {

ConfiguredNetherForestVegetationFeature::ConfiguredNetherForestVegetationFeature(
    std::unique_ptr<NetherForestVegetationConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredNetherForestVegetationFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin) const
{
    if (!m_config || m_config->stateProvider == nullptr || m_config->spreadWidth <= 0 || m_config->spreadHeight <= 0) {
        return false;
    }

    // origin 下方须为 NYLIUM 标签方块。
    const BlockState* belowState = region.getBlockState(origin.down());
    if (belowState == nullptr || !BlockTags::NYLIUM().contains(*belowState)) {
        return false;
    }

    const i32 minY = region.getMinBuildHeight();
    const i32 maxY = region.getMaxBuildHeight();
    const i32 y = origin.y;
    if (y < minY + 1 || y + 1 > maxY) {
        return false;
    }

    const i32 w = m_config->spreadWidth;
    const i32 h = m_config->spreadHeight;
    const auto& provider = *m_config->stateProvider;

    i32 placed = 0;
    const i32 attempts = w * w;
    for (i32 k = 0; k < attempts; ++k) {
        // MC: offset(nextInt(w)-nextInt(w), nextInt(h)-nextInt(h), nextInt(w)-nextInt(w))
        const i32 ox = random.nextInt(w) - random.nextInt(w);
        const i32 oy = random.nextInt(h) - random.nextInt(h);
        const i32 oz = random.nextInt(w) - random.nextInt(w);
        const BlockPos target(origin.x + ox, origin.y + oy, origin.z + oz);

        const BlockState* current = region.getBlockState(target);
        const bool empty = (current == nullptr) || current->isAir();
        if (!empty || target.y <= minY) {
            continue;
        }
        // canSurvive 检查省略：下方 nylium 已确认，下界植被在此 canSurvive 恒真。
        const BlockState* state = parser::BlockStateProviderParser::sampleState(provider, region, random, target);
        if (state != nullptr) {
            region.setBlockState(target, state, 2);
            ++placed;
        }
    }

    return placed > 0;
}

} // namespace mc::world::gen::feature
