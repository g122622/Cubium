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
#include "../../../resource/DataPackList.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include "../jigsaw/JigsawPattern.hpp"
#include "../jigsaw/JigsawPiece.hpp"
#include "../jigsaw/TemplatePoolLoader.hpp"
#include "pools/Pools.hpp"
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

    // 初始化 Jigsaw 模板池（使用硬编码注册系统）
    // 参考 MC 1.16.5: Pools.bootstrap()
    pools::Pools::initialize();

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

// StructureRegistry loadTemplatePoolsFromDataPacks 实现
size_t mc::world::gen::structure::StructureRegistry::loadTemplatePoolsFromDataPacks(
    const resource::DataPackList& dataPackList)
{
    auto result = jigsaw::TemplatePoolLoader::loadFromDataPackList(dataPackList);
    if (result.success()) {
        return result.value();
    }
    return 0;
}
