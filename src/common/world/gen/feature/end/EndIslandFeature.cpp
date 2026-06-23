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

#include "common/util/math/MathUtils.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/placement/PlacementUtils.hpp"

namespace mc {

// ============================================================================
// EndIslandFeature
// ============================================================================

bool EndIslandFeature::place(WorldGenRegion& world, math::Random& random, const BlockPos& pos)
{
    // 对应 MC 原版 EndIslandFeature.place()
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

ConfiguredEndIslandFeature::ConfiguredEndIslandFeature(
    std::unique_ptr<ConfiguredPlacement> placement, const char* featureName)
    : m_placement(std::move(placement))
    , m_name(featureName)
{}

bool ConfiguredEndIslandFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    bool placed = false;

    if (m_placement != nullptr) {
        auto positions = m_placement->getPositions(region, random, pos);
        for (const auto& placePos : positions) {
            if (EndIslandFeature::place(region, random, placePos)) {
                placed = true;
            }
        }
    } else {
        placed = EndIslandFeature::place(region, random, pos);
    }

    return placed;
}

// ============================================================================
// EndIslandFeatures
// ============================================================================

std::vector<std::unique_ptr<ConfiguredEndIslandFeature>> EndIslandFeatures::s_features;

void EndIslandFeatures::initialize()
{
    s_features.clear();
    s_features.push_back(createEndIsland());
}

const std::vector<std::unique_ptr<ConfiguredEndIslandFeature>>& EndIslandFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredEndIslandFeature>> EndIslandFeatures::getAllFeaturesAndClear()
{
    std::vector<std::unique_ptr<ConfiguredEndIslandFeature>> result;
    result.swap(s_features);
    return result;
}

std::unique_ptr<ConfiguredEndIslandFeature> EndIslandFeatures::createEndIsland()
{
    // 对应 MC 1.21.11: End Island 使用 RarityFilter(1/14) + CountExtra(1, 0.25, 1)
    //   + InSquarePlacement + HeightRange(55-70) + BiomeFilter
    // 在小型末地岛屿生物群系中以 RawGeneration 阶段生成
    auto placement = PlacementUtils::createCountedHeightPlacement(1, 55, 70);
    return std::make_unique<ConfiguredEndIslandFeature>(std::move(placement), "end_island");
}

} // namespace mc
