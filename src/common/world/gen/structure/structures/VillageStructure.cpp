#include "VillageStructure.hpp"
#include "../../jigsaw/JigsawManager.hpp"
#include "../../jigsaw/JigsawPattern.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../IWorldWriter.hpp"
#include <algorithm>
#include <limits>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const String VillageStructure::m_name = "village";

VillageStructure::VillageStructure(VillageType type)
    : Structure(StructureType::Village)
{
    m_config.type = type;
    initializeBiomes();
}

VillageStructure::VillageStructure(const VillageConfig& config)
    : Structure(StructureType::Village)
    , m_config(config)
{
    initializeBiomes();
}

void VillageStructure::initializeBiomes() {
    // 根据村庄类型设置有效生物群系
    switch (m_config.type) {
        case VillageType::Plains:
            m_validBiomes = { Plains, SunflowerPlains };
            break;
        case VillageType::Desert:
            m_validBiomes = { Desert, DesertHills, DesertLakes };
            break;
        case VillageType::Savanna:
            m_validBiomes = { Savanna, SavannaPlateau, ShatteredSavanna };
            break;
        case VillageType::Taiga:
            m_validBiomes = { Taiga, TaigaHills, TaigaMountains, SnowyTaiga, SnowyTaigaHills, SnowyTaigaMountains };
            break;
        case VillageType::Snowy:
            m_validBiomes = { SnowyPlains, SnowyMountains };
            break;
        case VillageType::Zombie:
            // 僵尸村庄可以在任何村庄生物群系生成
            m_validBiomes = { Plains, Desert, Savanna, Taiga, SnowyPlains };
            break;
    }
}

bool VillageStructure::canGenerate(
    IWorld& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ)
{
    (void)world;
    (void)rng;

    const i32 centerX = chunkX * 16 + 8;
    const i32 centerZ = chunkZ * 16 + 8;

    // 检查生物群系
    const BiomeId biomeId = generator.getBiome(centerX, 64, centerZ);
    if (!isValidBiome(biomeId)) {
        return false;
    }

    // 检查地形是否具备建造空间：中心与四角高差不能过大
    constexpr i32 SAMPLE_OFFSETS[5][2] = {
        {0, 0}, {-8, -8}, {-8, 8}, {8, -8}, {8, 8}
    };

    i32 minHeight = std::numeric_limits<i32>::max();
    i32 maxHeight = std::numeric_limits<i32>::min();

    for (const auto& offset : SAMPLE_OFFSETS) {
        const i32 sampleX = centerX + offset[0];
        const i32 sampleZ = centerZ + offset[1];
        const i32 h = generator.getHeight(sampleX, sampleZ, HeightmapType::WorldSurfaceWG);
        minHeight = std::min(minHeight, h);
        maxHeight = std::max(maxHeight, h);
    }

    if (minHeight < 50) {
        return false;
    }

    return (maxHeight - minHeight) <= 12;
}

std::unique_ptr<StructureStart> VillageStructure::generate(
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 获取起始模板池
    ResourceLocation startPoolLocation = getStartPool(m_config.type);
    auto& patternRegistry = jigsaw::JigsawPatternRegistry::instance();
    const jigsaw::JigsawPattern* startPool = patternRegistry.getPattern(startPoolLocation);

    if (!startPool || startPool->isEmpty()) {
        // 如果模板池不存在，创建一个简单的村庄标记
        // 放置一个简单的平台作为占位符
        const BlockState* cobblestone = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE);
        const BlockState* oakPlanks = VanillaBlocks::getState(VanillaBlocks::OAK_PLANKS);

        i32 baseX = chunkX * 16 + 8;
        i32 baseZ = chunkZ * 16 + 8;
        i32 baseY = generator.getHeight(baseX, baseZ, HeightmapType::WorldSurfaceWG);
        if (baseY < 60) baseY = 64;

        // 简单的 5x5 平台
        for (i32 x = -2; x <= 2; ++x) {
            for (i32 z = -2; z <= 2; ++z) {
                world.setBlock(baseX + x, baseY - 1, baseZ + z, cobblestone, 18);
            }
        }
        // 中心标记
        world.setBlock(baseX, baseY, baseZ, oakPlanks, 18);

        return start;
    }

    // 计算起始位置
    i32 startX = chunkX * 16 + rng.nextInt(16);
    i32 startZ = chunkZ * 16 + rng.nextInt(16);
    i32 startY = generator.getHeight(startX, startZ, HeightmapType::WorldSurfaceWG);
    if (startY < 60) startY = 64;

    BlockPos startPos(startX, startY, startZ);

    // 使用 JigsawManager 组装并放置村庄
    jigsaw::JigsawManager::assembleAndPlace(
        world,
        patternRegistry,
        *startPool,
        m_config.size,
        startPos,
        rng
    );

    return start;
}

ResourceLocation VillageStructure::getStartPool(VillageType type) {
    switch (type) {
        case VillageType::Plains:
            return ResourceLocation("minecraft", "village/plains/town_centers");
        case VillageType::Desert:
            return ResourceLocation("minecraft", "village/desert/town_centers");
        case VillageType::Savanna:
            return ResourceLocation("minecraft", "village/savanna/town_centers");
        case VillageType::Taiga:
            return ResourceLocation("minecraft", "village/taiga/town_centers");
        case VillageType::Snowy:
            return ResourceLocation("minecraft", "village/snowy/town_centers");
        case VillageType::Zombie:
            // 僵尸村庄使用独立的模板池（使用平原模板，但可扩展）
            return ResourceLocation("minecraft", "village/zombie/town_centers");
        default:
            return ResourceLocation("minecraft", "village/plains/town_centers");
    }
}

const char* VillageStructure::getVillageTypeName(VillageType type) {
    switch (type) {
        case VillageType::Plains:   return "plains";
        case VillageType::Desert:   return "desert";
        case VillageType::Savanna:  return "savanna";
        case VillageType::Taiga:    return "taiga";
        case VillageType::Snowy:    return "snowy";
        case VillageType::Zombie:   return "zombie";
        default:                     return "unknown";
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
