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

#include "RandomBooleanSelectorFeature.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ConfiguredFeatureRegistry.hpp"
#include <memory>
#include <utility>

namespace mc::world::gen::feature::cave {

// ============================================================================
// RandomBooleanSelectorFeature
// ============================================================================

bool RandomBooleanSelectorFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos,
    const RandomBooleanFeatureConfig& config)
{
    const ResourceLocation& featureId = random.nextBoolean() ? config.featureTrueId : config.featureFalseId;
    const ConfiguredFeatureBase* feature = ConfiguredFeatureRegistry::instance().get(featureId);

    if (feature == nullptr) {
        return false;
    }

    return feature->place(region, chunk, generator, random, pos);
}

// ============================================================================
// ConfiguredRandomBooleanSelectorFeature
// ============================================================================

ConfiguredRandomBooleanSelectorFeature::ConfiguredRandomBooleanSelectorFeature(
    std::unique_ptr<RandomBooleanFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredRandomBooleanSelectorFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    if (!m_config) {
        return false;
    }

    return RandomBooleanSelectorFeature::place(region, chunk, generator, random, pos, *m_config);
}

} // namespace mc::world::gen::feature::cave
