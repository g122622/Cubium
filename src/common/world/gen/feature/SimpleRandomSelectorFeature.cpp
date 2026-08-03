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
 */

#include "SimpleRandomSelectorFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/placement/PlacedFeature.hpp"
#include <memory>
#include <utility>

namespace mc::world::gen::feature::cave {

// ============================================================================
// SimpleRandomSelectorFeature
// ============================================================================

bool SimpleRandomSelectorFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos,
    const SimpleRandomFeatureConfig& config)
{
    if (config.features.empty()) {
        return false;
    }

    // 对齐 MC SimpleRandomSelectorFeature.place：均匀随机选一个 PlacedFeature，委托其 place(origin)
    // （先走 placement 链，再 place 配置化特征）。
    const i32 count = static_cast<i32>(config.features.size());
    const u32 index = static_cast<u32>(random.nextInt(count));
    return config.features[index]->place(region, chunk, generator, random, pos);
}

// ============================================================================
// ConfiguredSimpleRandomSelectorFeature
// ============================================================================

ConfiguredSimpleRandomSelectorFeature::ConfiguredSimpleRandomSelectorFeature(
    std::unique_ptr<SimpleRandomFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredSimpleRandomSelectorFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    return SimpleRandomSelectorFeature::place(region, chunk, generator, random, pos, *m_config);
}

} // namespace mc::world::gen::feature::cave
