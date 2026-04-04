#include "StructureManager.hpp"
#include "structures/RuinedPortalStructure.hpp"
#include "structures/BuriedTreasureStructure.hpp"
#include "structures/MineshaftStructure.hpp"
#include "structures/VillageStructure.hpp"
#include "structures/StrongholdStructure.hpp"
#include "structures/DesertPyramidStructure.hpp"
#include "structures/JungleTempleStructure.hpp"
#include "structures/OceanMonumentStructure.hpp"
#include "structures/ShipwreckStructure.hpp"
#include "structures/OceanRuinStructure.hpp"
#include "structures/FortressStructure.hpp"
#include "../jigsaw/JigsawPattern.hpp"
#include "../jigsaw/JigsawPiece.hpp"
#include "../../../resource/ResourceLocation.hpp"

namespace mc::world::gen::structure {

// StructureRegistry 实现
bool StructureRegistry::s_initialized = false;

std::unordered_map<String, std::unique_ptr<Structure>>& StructureRegistry::getStructures() {
    static std::unordered_map<String, std::unique_ptr<Structure>> structures;
    return structures;
}

std::vector<const Structure*>& StructureRegistry::getStructureList() {
    static std::vector<const Structure*> structureList;
    return structureList;
}

void StructureRegistry::initialize() {
    if (s_initialized) return;

    // 初始化 Jigsaw 模板池
    initializeDefaultJigsawPatterns();

    // 注册原版结构
    registerStructure(std::make_unique<RuinedPortalStructure>());
    registerStructure(std::make_unique<BuriedTreasureStructure>());
    registerStructure(std::make_unique<MineshaftStructure>());
    registerStructure(std::make_unique<VillageStructure>());
    registerStructure(std::make_unique<StrongholdStructure>());
    registerStructure(std::make_unique<DesertPyramidStructure>());
    registerStructure(std::make_unique<JungleTempleStructure>());
    registerStructure(std::make_unique<OceanMonumentStructure>());
    registerStructure(std::make_unique<ShipwreckStructure>());
    registerStructure(std::make_unique<OceanRuinStructure>());
    registerStructure(std::make_unique<FortressStructure>());

    s_initialized = true;
}

void StructureRegistry::initializeDefaultJigsawPatterns() {
    auto& registry = mc::world::gen::jigsaw::JigsawPatternRegistry::instance();

    // 注册村庄模板池（简化版本，实际应从资源包加载）
    registerVillagePatterns(registry);
    registerStrongholdPatterns(registry);
    registerFortressPatterns(registry);
}

void StructureRegistry::registerVillagePatterns(mc::world::gen::jigsaw::JigsawPatternRegistry& registry) {
    using namespace mc::world::gen::jigsaw;

    // 村庄起始模板池 - 平原
    auto plainsStart = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/plains/town_centers"),
        mc::ResourceLocation("minecraft", "empty")
    );
    // 添加一个简单的起始块
    plainsStart->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:village/plains/town_center_01",
        JigsawPlacementBehaviour::Rigid
    ), 1);
    registry.registerPattern(std::move(plainsStart));

    // 村庄街道模板池
    auto streets = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/plains/streets"),
        mc::ResourceLocation("minecraft", "empty")
    );
    streets->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:village/plains/street_01",
        JigsawPlacementBehaviour::Rigid
    ), 1);
    registry.registerPattern(std::move(streets));

    // 村庄房屋模板池
    auto houses = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/plains/houses"),
        mc::ResourceLocation("minecraft", "empty")
    );
    houses->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:village/plains/house_01",
        JigsawPlacementBehaviour::Rigid
    ), 1);
    registry.registerPattern(std::move(houses));

    // 空模板池（终止符）
    auto empty = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "empty"),
        mc::ResourceLocation("minecraft", "empty")
    );
    empty->addPiece(EmptyJigsawPiece::instance().clone(), 1);
    registry.registerPattern(std::move(empty));
}

void StructureRegistry::registerStrongholdPatterns(mc::world::gen::jigsaw::JigsawPatternRegistry& registry) {
    using namespace mc::world::gen::jigsaw;

    // 要塞起始模板池
    auto strongholdStart = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "stronghold/start"),
        mc::ResourceLocation("minecraft", "empty")
    );
    strongholdStart->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:stronghold/portal_room",
        JigsawPlacementBehaviour::Rigid
    ), 1);
    registry.registerPattern(std::move(strongholdStart));

    // 要塞走廊模板池
    auto corridors = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "stronghold/corridor"),
        mc::ResourceLocation("minecraft", "empty")
    );
    corridors->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:stronghold/corridor_01",
        JigsawPlacementBehaviour::Rigid
    ), 1);
    registry.registerPattern(std::move(corridors));
}

void StructureRegistry::registerFortressPatterns(mc::world::gen::jigsaw::JigsawPatternRegistry& registry) {
    using namespace mc::world::gen::jigsaw;

    // 下界要塞起始模板池
    auto fortressStart = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "nether_fortress/start"),
        mc::ResourceLocation("minecraft", "empty")
    );
    fortressStart->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:nether_fortress/bridge_crossing",
        JigsawPlacementBehaviour::Rigid
    ), 1);
    registry.registerPattern(std::move(fortressStart));

    // 下界要塞桥模板池
    auto bridge = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "nether_fortress/bridge"),
        mc::ResourceLocation("minecraft", "empty")
    );
    bridge->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:nether_fortress/bridge_straight",
        JigsawPlacementBehaviour::Rigid
    ), 30);
    bridge->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:nether_fortress/bridge_crossing",
        JigsawPlacementBehaviour::Rigid
    ), 10);
    bridge->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:nether_fortress/bridge_stairs",
        JigsawPlacementBehaviour::Rigid
    ), 10);
    registry.registerPattern(std::move(bridge));

    // 下界要塞走廊模板池
    auto corridor = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "nether_fortress/corridor"),
        mc::ResourceLocation("minecraft", "empty")
    );
    corridor->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:nether_fortress/corridor_5",
        JigsawPlacementBehaviour::Rigid
    ), 25);
    corridor->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:nether_fortress/corridor_crossing",
        JigsawPlacementBehaviour::Rigid
    ), 15);
    registry.registerPattern(std::move(corridor));

    // 下界要塞房间模板池
    auto rooms = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "nether_fortress/rooms"),
        mc::ResourceLocation("minecraft", "empty")
    );
    rooms->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:nether_fortress/throne_room",
        JigsawPlacementBehaviour::Rigid
    ), 5);
    rooms->addPiece(std::make_unique<SingleJigsawPiece>(
        "minecraft:nether_fortress/nether_wart_room",
        JigsawPlacementBehaviour::Rigid
    ), 5);
    registry.registerPattern(std::move(rooms));
}

void StructureRegistry::registerStructure(std::unique_ptr<Structure> structure) {
    if (!structure) return;

    const String name = structure->name();
    auto& structures = getStructures();
    auto& list = getStructureList();

    if (structures.find(name) == structures.end()) {
        list.push_back(structure.get());
        structures[name] = std::move(structure);
    }
}

const Structure* StructureRegistry::get(const String& name) {
    auto& structures = getStructures();
    auto it = structures.find(name);
    return it != structures.end() ? it->second.get() : nullptr;
}

const std::vector<const Structure*>& StructureRegistry::getAll() {
    return getStructureList();
}

// StructureManager 实现
StructureManager::StructureManager(i64 seed)
    : m_seed(seed)
{
}

bool StructureManager::shouldGenerateStructureStart(
    const Structure& structure,
    i32 chunkX,
    i32 chunkZ) const
{
    // 使用结构的间距设置检查是否应该在此位置生成
    i32 startX, startZ;
    return Structure::findStructureStart(
        m_seed, chunkX, chunkZ,
        structure.separationSettings(),
        startX, startZ);
}

std::unique_ptr<StructureStart> StructureManager::generateStructureStart(
    const Structure& structure,
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ)
{
    // 调用结构的生成方法
    return structure.generate(world, generator, rng, chunkX, chunkZ);
}

void StructureManager::placeStructureInChunk(
    const Structure& structure,
    IWorldWriter& world,
    ChunkPrimer& chunk,
    StructureStart& start,
    i32 chunkX,
    i32 chunkZ)
{
    // 调用结构的放置方法
    structure.placeInChunk(world, chunk, start, chunkX, chunkZ);
}

void StructureManager::clearCache() {
    // 清理缓存（简化版本）
}

math::Random StructureManager::createRandom(i32 chunkX, i32 chunkZ, i32 salt) const {
    i64 combinedSeed = m_seed ^ (static_cast<i64>(chunkX) * 3418731287LL) ^
                       (static_cast<i64>(chunkZ) * 132897987541LL) +
                       static_cast<i64>(salt);
    return math::Random(combinedSeed);
}

} // namespace mc::world::gen::structure
