#include "OceanRuinStructure.hpp"

#include "../../../biome/Biome.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc::world::gen::structure {

const String OceanRuinStructure::m_name = "ocean_ruin";

OceanRuinStructure::OceanRuinStructure()
    : Structure(StructureType::OceanRuin)
{
    initializeBiomes();
}

void OceanRuinStructure::initializeBiomes() {
    m_validBiomes = {
        Biomes::Ocean,
        Biomes::WarmOcean,
        Biomes::LukewarmOcean,
        Biomes::ColdOcean,
        Biomes::FrozenOcean,
        Biomes::DeepOcean,
        Biomes::DeepWarmOcean,
        Biomes::DeepLukewarmOcean,
        Biomes::DeepColdOcean,
        Biomes::DeepFrozenOcean
    };
}

bool OceanRuinStructure::canGenerate(
    IWorld& /*world*/,
    IChunkGenerator& /*generator*/,
    math::Random& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/)
{
    return rng.nextFloat() < 0.4f;
}

std::unique_ptr<StructureStart> OceanRuinStructure::generate(
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    const i32 baseX = (chunkX << 4) + rng.nextInt(16);
    const i32 baseZ = (chunkZ << 4) + rng.nextInt(16);

    i32 floorY = generator.getHeight(baseX, baseZ, HeightmapType::OceanFloorWG);
    if (floorY <= 0) {
        floorY = generator.seaLevel() - 5;
    }

    const BiomeId biome = generator.getBiome(baseX, floorY, baseZ);
    const bool warmVariant = isWarmBiome(biome);

    generateRuin(world, rng, BlockPos(baseX, floorY, baseZ), warmVariant);
    return start;
}

void OceanRuinStructure::generateRuin(
    IWorldWriter& world,
    math::Random& rng,
    const BlockPos& origin,
    bool warmVariant) const
{
    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* mossyStoneBricks = VanillaBlocks::getState(VanillaBlocks::MOSSY_STONE_BRICKS);
    const BlockState* cobblestone = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE);
    const BlockState* sandstone = VanillaBlocks::getState(VanillaBlocks::SANDSTONE);
    const BlockState* cutSandstone = VanillaBlocks::getState(VanillaBlocks::CUT_SANDSTONE);
    const BlockState* gravel = VanillaBlocks::getState(VanillaBlocks::GRAVEL);
    const BlockState* water = VanillaBlocks::getState(VanillaBlocks::WATER);
    const BlockState* treasure = VanillaBlocks::getState(VanillaBlocks::GOLD_BLOCK);

    const BlockState* deadCoralA = VanillaBlocks::getState(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK);
    const BlockState* deadCoralB = VanillaBlocks::getState(VanillaBlocks::DEAD_BRAIN_CORAL_BLOCK);

    const i32 width = rng.nextInt(6, 10);
    const i32 depth = rng.nextInt(6, 10);
    const i32 height = rng.nextInt(3, 6);

    auto pickMainBlock = [&]() -> const BlockState* {
        if (warmVariant) {
            return rng.nextBoolean() ? sandstone : cutSandstone;
        }

        const i32 roll = rng.nextInt(100);
        if (roll < 45) {
            return stoneBricks;
        }
        if (roll < 75) {
            return mossyStoneBricks;
        }
        return cobblestone;
    };

    // 基座与墙体。
    for (i32 x = 0; x < width; ++x) {
        for (i32 z = 0; z < depth; ++z) {
            world.setBlock(origin.x + x, origin.y, origin.z + z, pickMainBlock(), 18);

            const bool isEdge = (x == 0 || x == width - 1 || z == 0 || z == depth - 1);
            if (!isEdge) {
                continue;
            }

            for (i32 y = 1; y < height; ++y) {
                if (rng.nextFloat() < 0.12f) {
                    world.setBlock(origin.x + x, origin.y + y, origin.z + z, water, 18);
                } else {
                    world.setBlock(origin.x + x, origin.y + y, origin.z + z, pickMainBlock(), 18);
                }
            }
        }
    }

    // 中心塌陷区域。
    const i32 centerX = origin.x + width / 2;
    const i32 centerZ = origin.z + depth / 2;
    world.setBlock(centerX, origin.y + 1, centerZ, gravel, 18);
    if (rng.nextFloat() < 0.35f) {
        world.setBlock(centerX, origin.y + 2, centerZ, treasure, 18);
    }

    // 死珊瑚装饰（体现海底侵蚀）。
    if (deadCoralA != nullptr && deadCoralB != nullptr) {
        for (i32 i = 0; i < 6; ++i) {
            const i32 x = origin.x + rng.nextInt(width);
            const i32 z = origin.z + rng.nextInt(depth);
            const BlockState* coral = rng.nextBoolean() ? deadCoralA : deadCoralB;
            world.setBlock(x, origin.y + 1, z, coral, 18);
        }
    }
}

bool OceanRuinStructure::isWarmBiome(BiomeId biomeId) const {
    return biomeId == Biomes::WarmOcean ||
           biomeId == Biomes::LukewarmOcean ||
           biomeId == Biomes::DeepWarmOcean ||
           biomeId == Biomes::DeepLukewarmOcean;
}

} // namespace mc::world::gen::structure
