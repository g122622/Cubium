#include "OceanMonumentStructure.hpp"

#include "common/core/Constants.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string OceanMonumentStructure::m_name = "ocean_monument";

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
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(generator);

    using namespace mc::world;

    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);
    const i32 startX = chunkX * CHUNK_WIDTH - 29;
    const i32 startZ = chunkZ * CHUNK_WIDTH - 29;
    auto building = std::make_unique<OceanMonumentBuilding>(rng, startX, startZ, Direction::North);

    StructureBoundingBox boundingBox = building->boundingBox();
    const i32 minChunkX = boundingBox.minX() >> CHUNK_SHIFT;
    const i32 maxChunkX = boundingBox.maxX() >> CHUNK_SHIFT;
    const i32 minChunkZ = boundingBox.minZ() >> CHUNK_SHIFT;
    const i32 maxChunkZ = boundingBox.maxZ() >> CHUNK_SHIFT;

    for (i32 targetChunkX = minChunkX; targetChunkX <= maxChunkX; ++targetChunkX) {
        for (i32 targetChunkZ = minChunkZ; targetChunkZ <= maxChunkZ; ++targetChunkZ) {
            const StructureBoundingBox chunkBounds = StructureBoundingBox::fromChunk(targetChunkX, targetChunkZ);
            building->generate(world, rng, targetChunkX, targetChunkZ, chunkBounds);
        }
    }

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
