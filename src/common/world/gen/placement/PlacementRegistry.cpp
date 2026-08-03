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

#include "PlacementRegistry.hpp"

#include "BlockPredicateFilterPlacement.hpp"
#include "common/world/gen/placement/BiomeFilterPlacement.hpp"
#include "common/world/gen/placement/EnvironmentScanPlacement.hpp"
#include "common/world/gen/placement/Placement.hpp"
#include "common/world/gen/placement/Placements.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// PlacementRegistry 实现
// ============================================================================

PlacementRegistry& PlacementRegistry::instance() noexcept
{
    static PlacementRegistry s_instance;
    return s_instance;
}

void PlacementRegistry::initialize()
{
    if (m_initialized) {
        return;
    }

    // 注册所有内置放置器
    registerPlacement("count", Placements::count());
    registerPlacement("height_range", Placements::heightRange());
    registerPlacement("square", Placements::square());
    registerPlacement("biome", Placements::biome());
    registerPlacement("chance", Placements::chance());
    registerPlacement("surface", Placements::surface());
    registerPlacement("heightmap", Placements::heightmap());
    registerPlacement("rarity_filter", Placements::rarityFilter());
    registerPlacement("noise", Placements::noise());
    registerPlacement("count_noise", Placements::countNoise());
    registerPlacement("depth_average", Placements::depthAverage());
    registerPlacement("top_solid", Placements::topSolid());
    registerPlacement("carving_mask", Placements::carvingMask());
    registerPlacement("random_offset", Placements::randomOffset());
    registerPlacement("water_depth_threshold", Placements::waterDepthThreshold());
    registerPlacement("sea_level", Placements::seaLevel());
    registerPlacement("spread", Placements::spread());
    registerPlacement("biome_filter", Placements::biomeFilter());
    registerPlacement("environment_scan", Placements::environmentScan());
    registerPlacement("block_predicate_filter", std::make_unique<BlockPredicateFilterPlacement>());
    registerPlacement("fixed_placement", Placements::fixedPlacement());
    registerPlacement("count_on_every_layer", Placements::countOnEveryLayer());
    registerPlacement("noise_threshold_count", Placements::noiseThresholdCount());
    registerPlacement("noise_based_count", Placements::noiseBasedCount());
    registerPlacement("surface_relative_threshold_filter", Placements::surfaceRelativeThresholdFilter());

    m_initialized = true;
}

void PlacementRegistry::registerPlacement(const std::string& name, std::unique_ptr<Placement> placement)
{
    if (!placement) {
        return;
    }

    m_placements[name] = std::move(placement);
}

const Placement* PlacementRegistry::get(const std::string& name) const noexcept
{
    auto it = m_placements.find(name);
    return it != m_placements.end() ? it->second.get() : nullptr;
}

std::vector<std::string> PlacementRegistry::getNames() const
{
    std::vector<std::string> names;
    names.reserve(m_placements.size());
    for (const auto& pair : m_placements) {
        names.push_back(pair.first);
    }
    return names;
}

// ============================================================================
// Placements 工厂方法
// ============================================================================

namespace Placements {

std::unique_ptr<Placement> count()
{
    return std::make_unique<CountPlacement>();
}

std::unique_ptr<Placement> heightRange()
{
    return std::make_unique<HeightRangePlacement>();
}

std::unique_ptr<Placement> square()
{
    return std::make_unique<SquarePlacement>();
}

std::unique_ptr<Placement> biome()
{
    return std::make_unique<BiomePlacement>();
}

std::unique_ptr<Placement> chance()
{
    return std::make_unique<ChancePlacement>();
}

std::unique_ptr<Placement> surface()
{
    return std::make_unique<SurfacePlacement>();
}

std::unique_ptr<Placement> heightmap()
{
    return std::make_unique<HeightmapPlacement>();
}

std::unique_ptr<Placement> rarityFilter()
{
    return std::make_unique<RarityFilterPlacement>();
}

std::unique_ptr<Placement> noise()
{
    return std::make_unique<NoisePlacement>();
}

std::unique_ptr<Placement> countNoise()
{
    return std::make_unique<CountNoisePlacement>();
}

std::unique_ptr<Placement> depthAverage()
{
    return std::make_unique<DepthAveragePlacement>();
}

std::unique_ptr<Placement> topSolid()
{
    return std::make_unique<TopSolidPlacement>();
}

std::unique_ptr<Placement> carvingMask()
{
    return std::make_unique<CarvingMaskPlacement>();
}

std::unique_ptr<Placement> randomOffset()
{
    return std::make_unique<RandomOffsetPlacement>();
}

std::unique_ptr<Placement> waterDepthThreshold()
{
    return std::make_unique<WaterDepthThresholdPlacement>();
}

std::unique_ptr<Placement> seaLevel()
{
    return std::make_unique<SeaLevelPlacement>();
}

std::unique_ptr<Placement> spread()
{
    return std::make_unique<SpreadPlacement>();
}

std::unique_ptr<Placement> biomeFilter()
{
    return std::make_unique<BiomeFilterPlacement>();
}

std::unique_ptr<Placement> environmentScan()
{
    return std::make_unique<EnvironmentScanPlacement>();
}

std::unique_ptr<Placement> fixedPlacement()
{
    return std::make_unique<FixedPlacement>();
}

std::unique_ptr<Placement> countOnEveryLayer()
{
    return std::make_unique<CountOnEveryLayerPlacement>();
}

std::unique_ptr<Placement> noiseThresholdCount()
{
    return std::make_unique<NoiseThresholdCountPlacement>();
}

std::unique_ptr<Placement> noiseBasedCount()
{
    return std::make_unique<NoiseBasedCountPlacement>();
}

std::unique_ptr<Placement> surfaceRelativeThresholdFilter()
{
    return std::make_unique<SurfaceRelativeThresholdFilterPlacement>();
}

} // namespace Placements

} // namespace mc
