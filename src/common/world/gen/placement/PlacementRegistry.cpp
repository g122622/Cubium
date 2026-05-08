#include "PlacementRegistry.hpp"

namespace mc {

// ============================================================================
// PlacementRegistry 实现
// ============================================================================

PlacementRegistry& PlacementRegistry::instance()
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
    registerPlacement("noise", Placements::noise());
    registerPlacement("count_noise", Placements::countNoise());
    registerPlacement("depth_average", Placements::depthAverage());
    registerPlacement("top_solid", Placements::topSolid());
    registerPlacement("carving_mask", Placements::carvingMask());
    registerPlacement("random_offset", Placements::randomOffset());
    registerPlacement("water_depth_threshold", Placements::waterDepthThreshold());
    registerPlacement("sea_level", Placements::seaLevel());
    registerPlacement("spread", Placements::spread());

    m_initialized = true;
}

void PlacementRegistry::registerPlacement(const std::string& name, std::unique_ptr<Placement> placement)
{
    if (!placement) {
        return;
    }

    m_placements[name] = std::move(placement);
}

const Placement* PlacementRegistry::get(const std::string& name) const
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

} // namespace Placements

} // namespace mc
