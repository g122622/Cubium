#include "ShipwreckStructure.hpp"

#include "../../../biome/Biome.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../../util/math/random/Random.hpp"

#include <algorithm>

namespace mc::world::gen::structure {

const String ShipwreckStructure::m_name = "shipwreck";

ShipwreckStructure::ShipwreckStructure()
    : Structure(StructureType::Shipwreck)
{
    initializeBiomes();
}

void ShipwreckStructure::initializeBiomes() {
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
        Biomes::DeepFrozenOcean,
        Biomes::Beach,
        Biomes::SnowyBeach
    };
}

bool ShipwreckStructure::canGenerate(
    IWorld& /*world*/,
    IChunkGenerator& /*generator*/,
    math::Random& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/)
{
    // 保持与现有结构系统一致的轻量概率判定。
    return rng.nextFloat() < 0.35f;
}

std::unique_ptr<StructureStart> ShipwreckStructure::generate(
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    const i32 baseX = (chunkX << 4) + rng.nextInt(16);
    const i32 baseZ = (chunkZ << 4) + rng.nextInt(16);

    i32 oceanFloorY = generator.getHeight(baseX, baseZ, HeightmapType::OceanFloorWG);
    if (oceanFloorY <= 0) {
        oceanFloorY = generator.seaLevel() - 6;
    }

    const i32 yOffset = rng.nextInt(3, 8);
    const i32 baseY = std::max(20, oceanFloorY - yOffset);

    generateShipwreck(world, rng, BlockPos(baseX, baseY, baseZ));
    return start;
}

void ShipwreckStructure::generateShipwreck(
    IWorldWriter& world,
    math::Random& rng,
    const BlockPos& origin) const
{
    const BlockState* planks = VanillaBlocks::getState(VanillaBlocks::OAK_PLANKS);
    const BlockState* supportLog = VanillaBlocks::getState(
        VanillaBlocks::STRIPPED_OAK_LOG ? VanillaBlocks::STRIPPED_OAK_LOG : VanillaBlocks::OAK_LOG);
    const BlockState* deckFence = VanillaBlocks::getState(
        VanillaBlocks::OAK_FENCE ? VanillaBlocks::OAK_FENCE : VanillaBlocks::OAK_PLANKS);
    const BlockState* deckStairs = VanillaBlocks::getState(
        VanillaBlocks::OAK_STAIRS ? VanillaBlocks::OAK_STAIRS : VanillaBlocks::OAK_PLANKS);
    const BlockState* water = VanillaBlocks::getState(VanillaBlocks::WATER);

    const i32 length = rng.nextInt(12, 17);
    const i32 width = 5;
    const i32 height = 6;

    // 船体底板与船舷。
    for (i32 x = 0; x < length; ++x) {
        for (i32 z = 0; z < width; ++z) {
            world.setBlock(origin.x + x, origin.y, origin.z + z, planks, 18);

            const bool isSide = (z == 0 || z == width - 1);
            const bool isBowOrStern = (x == 0 || x == length - 1);
            if (isSide || isBowOrStern) {
                for (i32 y = 1; y <= 2; ++y) {
                    world.setBlock(origin.x + x, origin.y + y, origin.z + z, planks, 18);
                }
            }
        }
    }

    // 甲板。
    const i32 deckY = origin.y + 3;
    for (i32 x = 1; x < length - 1; ++x) {
        for (i32 z = 1; z < width - 1; ++z) {
            world.setBlock(origin.x + x, deckY, origin.z + z, planks, 18);
        }
    }

    // 船桅杆与横梁。
    const i32 mastX = origin.x + length / 2;
    const i32 mastZ = origin.z + width / 2;
    for (i32 y = 1; y < height; ++y) {
        world.setBlock(mastX, origin.y + y, mastZ, supportLog, 18);
    }
    for (i32 x = -2; x <= 2; ++x) {
        world.setBlock(mastX + x, origin.y + height - 1, mastZ, supportLog, 18);
    }

    // 船尾小舱。
    const i32 cabinStartX = origin.x + length - 5;
    for (i32 x = 0; x < 4; ++x) {
        for (i32 z = 1; z < width - 1; ++z) {
            world.setBlock(cabinStartX + x, origin.y + 4, origin.z + z, planks, 18);
        }
    }

    // 护栏。
    for (i32 x = 2; x < length - 2; ++x) {
        world.setBlock(origin.x + x, deckY + 1, origin.z, deckFence, 18);
        world.setBlock(origin.x + x, deckY + 1, origin.z + width - 1, deckFence, 18);
    }

    // 船首/船尾坡面。
    world.setBlock(origin.x + 1, origin.y + 1, origin.z + width / 2, deckStairs, 18);
    world.setBlock(origin.x + length - 2, origin.y + 1, origin.z + width / 2, deckStairs, 18);

    // 破损开口，增强沉船观感。
    for (i32 i = 0; i < 8; ++i) {
        const i32 holeX = origin.x + rng.nextInt(1, length - 1);
        const i32 holeY = origin.y + rng.nextInt(1, 4);
        const i32 holeZ = origin.z + rng.nextInt(1, width - 1);
        world.setBlock(holeX, holeY, holeZ, water, 18);
    }
}

} // namespace mc::world::gen::structure
