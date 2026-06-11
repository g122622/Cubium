#include "OceanMonumentStructure.hpp"

#include "common/core/Constants.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string OceanMonumentStructure::m_name = "ocean_monument";

const SpawnOverrides OceanMonumentStructure::m_spawnOverrides = {
    SpawnOverrideType::Full, {SpawnOverrideEntry{"monster", 4, 4}}};

OceanMonumentStructure::OceanMonumentStructure()
    : Structure(StructureType::Monument)
{
    _initializeBiomes();
}

void OceanMonumentStructure::_initializeBiomes()
{
    m_validBiomes = {DeepOcean, DeepWarmOcean, DeepLukewarmOcean, DeepColdOcean, DeepFrozenOcean};
}

bool OceanMonumentStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    using namespace mc::world;

    const i32 centerX = chunkX * CHUNK_WIDTH + 9;
    const i32 centerZ = chunkZ * CHUNK_WIDTH + 9;
    const BiomeId centerBiome = generator.getBiome(centerX, SEA_LEVEL, centerZ);
    if (!isValidBiome(centerBiome)) {
        return false;
    }

    constexpr i32 outerRadius = 29;
    constexpr i32 step = CHUNK_WIDTH;
    for (i32 dx = -outerRadius; dx <= outerRadius; dx += step) {
        for (i32 dz = -outerRadius; dz <= outerRadius; dz += step) {
            const BiomeId biome = generator.getBiome(centerX + dx, SEA_LEVEL, centerZ + dz);
            if (!_isOceanOrRiverBiome(biome)) {
                return false;
            }
        }
    }
    return true;
}

std::unique_ptr<StructureStart> OceanMonumentStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(generator);

    using namespace mc::world;

    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);
    const i32 startX = chunkX * CHUNK_WIDTH - 29;
    const i32 startZ = chunkZ * CHUNK_WIDTH - 29;

    // 随机水平方向
    static const Direction horizontalDirections[] = {
        Direction::North, Direction::South, Direction::East, Direction::West};
    Direction direction = horizontalDirections[rng.nextInt(4)];

    auto building = std::make_unique<OceanMonumentBuilding>(rng, startX, startZ, direction);

    // 只添加片段到 StructureStart；方块写入延迟到 FEATURES 阶段由 placeInChunk() 执行
    // OceanMonumentBuilding::generate() 会在 FEATURES 阶段被调用，并分发给其子片段
    start->addPiece(std::move(building));
    start->recalculateStructureSize();
    return start;
}

bool OceanMonumentStructure::_isOceanOrRiverBiome(BiomeId biomeId) const
{
    switch (biomeId) {
        case Ocean:
        case WarmOcean:
        case LukewarmOcean:
        case ColdOcean:
        case FrozenOcean:
        case DeepOcean:
        case DeepWarmOcean:
        case DeepLukewarmOcean:
        case DeepColdOcean:
        case DeepFrozenOcean:
        case River:
        case FrozenRiver:
            return true;
        default:
            return false;
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
