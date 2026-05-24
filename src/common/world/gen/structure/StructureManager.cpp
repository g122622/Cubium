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

#include "StructureManager.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include "../jigsaw/JigsawPattern.hpp"
#include "../jigsaw/JigsawPiece.hpp"
#include "structures/BastionRemnantStructure.hpp"
#include "structures/BuriedTreasureStructure.hpp"
#include "structures/DesertPyramidStructure.hpp"
#include "structures/EndCityStructure.hpp"
#include "structures/FortressStructure.hpp"
#include "structures/IglooStructure.hpp"
#include "structures/JungleTempleStructure.hpp"
#include "structures/MineshaftStructure.hpp"
#include "structures/NetherFossilStructure.hpp"
#include "structures/OceanMonumentStructure.hpp"
#include "structures/OceanRuinStructure.hpp"
#include "structures/PillagerOutpostStructure.hpp"
#include "structures/RuinedPortalStructure.hpp"
#include "structures/ShipwreckStructure.hpp"
#include "structures/StrongholdStructure.hpp"
#include "structures/SwampHutStructure.hpp"
#include "structures/VillageStructure.hpp"
#include "structures/WoodlandMansionStructure.hpp"

namespace mc::world::gen::structure {

// StructureRegistry 实现
bool StructureRegistry::s_initialized = false;

std::unordered_map<std::string, std::unique_ptr<Structure>>& StructureRegistry::getStructures()
{
    static std::unordered_map<std::string, std::unique_ptr<Structure>> structures;
    return structures;
}

std::vector<const Structure*>& StructureRegistry::getStructureList()
{
    static std::vector<const Structure*> structureList;
    return structureList;
}

void StructureRegistry::initialize()
{
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
    registerStructure(std::make_unique<IglooStructure>());
    registerStructure(std::make_unique<SwampHutStructure>());
    registerStructure(std::make_unique<NetherFossilStructure>());
    registerStructure(std::make_unique<PillagerOutpostStructure>());
    registerStructure(std::make_unique<WoodlandMansionStructure>());
    registerStructure(std::make_unique<EndCityStructure>());
    registerStructure(std::make_unique<BastionRemnantStructure>());

    s_initialized = true;
}

void StructureRegistry::initializeDefaultJigsawPatterns()
{
    auto& registry = mc::world::gen::jigsaw::JigsawPatternRegistry::instance();

    // 注册村庄模板池（简化版本，实际应从资源包加载）
    registerVillagePatterns(registry);
    registerStrongholdPatterns(registry);
    registerFortressPatterns(registry);
}

void StructureRegistry::registerVillagePatterns(mc::world::gen::jigsaw::JigsawPatternRegistry& registry)
{
    using namespace mc::world::gen::jigsaw;

    // =========================================================================
    // 平原村庄 (Plains Village)
    // =========================================================================
    auto plainsStart = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/plains/town_centers"), mc::ResourceLocation("minecraft", "empty"));
    plainsStart->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/plains/town_center_01", JigsawPlacementBehaviour::Rigid),
        1);
    registry.registerPattern(std::move(plainsStart));

    auto plainsStreets = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/plains/streets"), mc::ResourceLocation("minecraft", "empty"));
    plainsStreets->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/plains/street_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(plainsStreets));

    auto plainsHouses = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/plains/houses"), mc::ResourceLocation("minecraft", "empty"));
    plainsHouses->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/plains/house_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(plainsHouses));

    // =========================================================================
    // 沙漠村庄 (Desert Village)
    // =========================================================================
    auto desertStart = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/desert/town_centers"), mc::ResourceLocation("minecraft", "empty"));
    desertStart->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/desert/town_center_01", JigsawPlacementBehaviour::Rigid),
        1);
    registry.registerPattern(std::move(desertStart));

    auto desertStreets = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/desert/streets"), mc::ResourceLocation("minecraft", "empty"));
    desertStreets->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/desert/street_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(desertStreets));

    auto desertHouses = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/desert/houses"), mc::ResourceLocation("minecraft", "empty"));
    desertHouses->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/desert/house_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(desertHouses));

    // =========================================================================
    // 热带草原村庄 (Savanna Village)
    // =========================================================================
    auto savannaStart = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/savanna/town_centers"), mc::ResourceLocation("minecraft", "empty"));
    savannaStart->addPiece(std::make_unique<SingleJigsawPiece>(
                               "minecraft:village/savanna/town_center_01", JigsawPlacementBehaviour::Rigid),
        1);
    registry.registerPattern(std::move(savannaStart));

    auto savannaStreets = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/savanna/streets"), mc::ResourceLocation("minecraft", "empty"));
    savannaStreets->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/savanna/street_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(savannaStreets));

    auto savannaHouses = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/savanna/houses"), mc::ResourceLocation("minecraft", "empty"));
    savannaHouses->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/savanna/house_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(savannaHouses));

    // =========================================================================
    // 针叶林村庄 (Taiga Village)
    // =========================================================================
    auto taigaStart = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/taiga/town_centers"), mc::ResourceLocation("minecraft", "empty"));
    taigaStart->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/taiga/town_center_01", JigsawPlacementBehaviour::Rigid),
        1);
    registry.registerPattern(std::move(taigaStart));

    auto taigaStreets = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/taiga/streets"), mc::ResourceLocation("minecraft", "empty"));
    taigaStreets->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/taiga/street_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(taigaStreets));

    auto taigaHouses = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/taiga/houses"), mc::ResourceLocation("minecraft", "empty"));
    taigaHouses->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/taiga/house_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(taigaHouses));

    // =========================================================================
    // 雪地村庄 (Snowy Village)
    // =========================================================================
    auto snowyStart = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/snowy/town_centers"), mc::ResourceLocation("minecraft", "empty"));
    snowyStart->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/snowy/town_center_01", JigsawPlacementBehaviour::Rigid),
        1);
    registry.registerPattern(std::move(snowyStart));

    auto snowyStreets = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/snowy/streets"), mc::ResourceLocation("minecraft", "empty"));
    snowyStreets->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/snowy/street_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(snowyStreets));

    auto snowyHouses = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/snowy/houses"), mc::ResourceLocation("minecraft", "empty"));
    snowyHouses->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/snowy/house_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(snowyHouses));

    // =========================================================================
    // 僵尸村庄 (Zombie Village) - 使用平原村庄模板
    // 僵尸村民生成在村庄结构放置后由实体系统处理
    // =========================================================================
    auto zombieStart = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/zombie/town_centers"), mc::ResourceLocation("minecraft", "empty"));
    zombieStart->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/plains/town_center_01", JigsawPlacementBehaviour::Rigid),
        1);
    registry.registerPattern(std::move(zombieStart));

    auto zombieStreets = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/zombie/streets"), mc::ResourceLocation("minecraft", "empty"));
    zombieStreets->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/plains/street_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(zombieStreets));

    auto zombieHouses = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "village/zombie/houses"), mc::ResourceLocation("minecraft", "empty"));
    zombieHouses->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:village/plains/house_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(zombieHouses));

    // =========================================================================
    // 空模板池（终止符）
    // =========================================================================
    auto empty = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "empty"), mc::ResourceLocation("minecraft", "empty"));
    empty->addPiece(EmptyJigsawPiece::instance().clone(), 1);
    registry.registerPattern(std::move(empty));
}

void StructureRegistry::registerStrongholdPatterns(mc::world::gen::jigsaw::JigsawPatternRegistry& registry)
{
    using namespace mc::world::gen::jigsaw;

    // 要塞起始模板池
    auto strongholdStart = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "stronghold/start"), mc::ResourceLocation("minecraft", "empty"));
    strongholdStart->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:stronghold/portal_room", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(strongholdStart));

    // 要塞走廊模板池
    auto corridors = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "stronghold/corridor"), mc::ResourceLocation("minecraft", "empty"));
    corridors->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:stronghold/corridor_01", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(corridors));
}

void StructureRegistry::registerFortressPatterns(mc::world::gen::jigsaw::JigsawPatternRegistry& registry)
{
    using namespace mc::world::gen::jigsaw;

    // 下界要塞起始模板池
    auto fortressStart = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "nether_fortress/start"), mc::ResourceLocation("minecraft", "empty"));
    fortressStart->addPiece(std::make_unique<SingleJigsawPiece>(
                                "minecraft:nether_fortress/bridge_crossing", JigsawPlacementBehaviour::Rigid),
        1);
    registry.registerPattern(std::move(fortressStart));

    // 下界要塞桥模板池
    auto bridge = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "nether_fortress/bridge"), mc::ResourceLocation("minecraft", "empty"));
    bridge->addPiece(std::make_unique<SingleJigsawPiece>(
                         "minecraft:nether_fortress/bridge_straight", JigsawPlacementBehaviour::Rigid),
        30);
    bridge->addPiece(std::make_unique<SingleJigsawPiece>(
                         "minecraft:nether_fortress/bridge_crossing", JigsawPlacementBehaviour::Rigid),
        10);
    bridge->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:nether_fortress/bridge_stairs", JigsawPlacementBehaviour::Rigid),
        10);
    registry.registerPattern(std::move(bridge));

    // 下界要塞走廊模板池
    auto corridor = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "nether_fortress/corridor"), mc::ResourceLocation("minecraft", "empty"));
    corridor->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:nether_fortress/corridor_5", JigsawPlacementBehaviour::Rigid),
        25);
    corridor->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:nether_fortress/corridor_crossing", JigsawPlacementBehaviour::Rigid),
        15);
    registry.registerPattern(std::move(corridor));

    // 下界要塞房间模板池
    auto rooms = std::make_unique<JigsawPattern>(
        mc::ResourceLocation("minecraft", "nether_fortress/rooms"), mc::ResourceLocation("minecraft", "empty"));
    rooms->addPiece(
        std::make_unique<SingleJigsawPiece>("minecraft:nether_fortress/throne_room", JigsawPlacementBehaviour::Rigid),
        5);
    rooms->addPiece(std::make_unique<SingleJigsawPiece>(
                        "minecraft:nether_fortress/nether_wart_room", JigsawPlacementBehaviour::Rigid),
        5);
    registry.registerPattern(std::move(rooms));
}

void StructureRegistry::registerStructure(std::unique_ptr<Structure> structure)
{
    if (!structure) return;

    const std::string name = structure->name();
    auto& structures = getStructures();
    auto& list = getStructureList();

    if (structures.find(name) == structures.end()) {
        list.push_back(structure.get());
        structures[name] = std::move(structure);
    }
}

const Structure* StructureRegistry::get(const std::string& name)
{
    auto& structures = getStructures();
    auto it = structures.find(name);
    return it != structures.end() ? it->second.get() : nullptr;
}

const std::vector<const Structure*>& StructureRegistry::getAll()
{
    return getStructureList();
}

// StructureManager 实现
StructureManager::StructureManager(i64 seed)
    : m_seed(seed)
{}

bool StructureManager::shouldGenerateStructureStart(const Structure& structure, i32 chunkX, i32 chunkZ) const
{
    // 使用结构的间距设置检查是否应该在此位置生成
    i32 startX, startZ;
    return Structure::findStructureStart(m_seed, chunkX, chunkZ, structure.separationSettings(), startX, startZ);
}

std::unique_ptr<StructureStart> StructureManager::generateStructureStart(const Structure& structure,
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
    const Structure& structure, IWorldWriter& world, ChunkPrimer& chunk, StructureStart& start, i32 chunkX, i32 chunkZ)
{
    // 调用结构的放置方法
    structure.placeInChunk(world, chunk, start, chunkX, chunkZ);
}

void StructureManager::clearCache()
{
    // 清理缓存（简化版本）
}

math::Random StructureManager::createRandom(i32 chunkX, i32 chunkZ, i32 salt) const
{
    // MC 1.16.5 使用常量 341873128712
    u64 combinedSeed = static_cast<u64>(chunkX) * 341873128712ULL + static_cast<u64>(chunkZ) * 132897987541ULL +
        static_cast<u64>(m_seed) + static_cast<u64>(salt);
    return math::Random(static_cast<i64>(combinedSeed));
}

} // namespace mc::world::gen::structure
