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

#include "RandomSelectorFeature.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ConfiguredFeatureRegistry.hpp"

namespace mc::world::gen::feature {

// ============================================================================
// RandomSelectorFeature
// ============================================================================

bool RandomSelectorFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos,
    const RandomSelectorFeatureConfig& config)
{
    // 顺序概率检查：遍历 features，每项 nextFloat() < chance 即命中委派并返回。
    // nextFloat() ∈ [0.0, 1.0)，故 chance=1.0 必触发，chance=0.0 必不触发。
    for (const auto& entry : config.features) {
        if (random.nextFloat() < entry.chance) {
            const ConfiguredFeatureBase* feature = ConfiguredFeatureRegistry::instance().get(entry.featureId);
            if (feature == nullptr) {
                return false;
            }
            return feature->place(region, chunk, generator, random, pos);
        }
    }

    // 全部未命中，走 default
    const ConfiguredFeatureBase* fallback = ConfiguredFeatureRegistry::instance().get(config.defaultFeatureId);
    if (fallback == nullptr) {
        return false;
    }
    return fallback->place(region, chunk, generator, random, pos);
}

// ============================================================================
// ConfiguredRandomSelectorFeature
// ============================================================================

ConfiguredRandomSelectorFeature::ConfiguredRandomSelectorFeature(
    std::unique_ptr<RandomSelectorFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredRandomSelectorFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    if (!m_config) {
        return false;
    }
    return RandomSelectorFeature::place(region, chunk, generator, random, pos, *m_config);
}

} // namespace mc::world::gen::feature
