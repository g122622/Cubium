#include "VillageStructure.hpp"
#include "../../jigsaw/JigsawManager.hpp"
#include "../../jigsaw/JigsawPattern.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../block/BlockPos.hpp"

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
    // 基本检查
    // TODO: 检查生物群系是否合适
    // TODO: 检查是否有足够的空间
    return true;
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
        // 如果模板池不存在，创建一个简单的村庄
        // TODO: 注册默认模板池
        return start;
    }

    // 计算起始位置
    // 村庄通常在地表生成，需要获取地表高度
    i32 startY = 64;  // TODO: 从地形获取实际高度
    BlockPos startPos(chunkX * 16 + 8, startY, chunkZ * 16 + 8);

    // 使用 JigsawManager 组装村庄
    auto placedPieces = jigsaw::JigsawManager::assemble(
        patternRegistry,
        *startPool,
        m_config.size,
        startPos,
        rng
    );

    // TODO: 将 placedPieces 转换为 StructurePieces 并添加到 StructureStart
    // TODO: 实际将方块放置到世界中

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
            // 僵尸村庄使用普通村庄的模板，但生成僵尸村民
            return ResourceLocation("minecraft", "village/plains/town_centers");
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
