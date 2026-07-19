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
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/gen/jigsaw/JigsawPiece.hpp"
#include "common/world/gen/jigsaw/TemplatePoolLoader.hpp"
#include "common/world/gen/jigsaw/TemplatePoolRegistry.hpp"
#include "common/world/gen/structure/pools/Pools.hpp"
#include "common/world/gen/structure/structures/BastionRemnantStructure.hpp"
#include "common/world/gen/structure/structures/BuriedTreasureStructure.hpp"
#include "common/world/gen/structure/structures/DesertPyramidStructure.hpp"
#include "common/world/gen/structure/structures/EndCityStructure.hpp"
#include "common/world/gen/structure/structures/FortressStructure.hpp"
#include "common/world/gen/structure/structures/IglooStructure.hpp"
#include "common/world/gen/structure/structures/JungleTempleStructure.hpp"
#include "common/world/gen/structure/structures/MineshaftStructure.hpp"
#include "common/world/gen/structure/structures/NetherFossilStructure.hpp"
#include "common/world/gen/structure/structures/OceanMonumentStructure.hpp"
#include "common/world/gen/structure/structures/OceanRuinStructure.hpp"
#include "common/world/gen/structure/structures/PillagerOutpostStructure.hpp"
#include "common/world/gen/structure/structures/RuinedPortalStructure.hpp"
#include "common/world/gen/structure/structures/ShipwreckStructure.hpp"
#include "common/world/gen/structure/structures/StrongholdStructure.hpp"
#include "common/world/gen/structure/structures/SwampHutStructure.hpp"
#include "common/world/gen/structure/structures/VillageStructure.hpp"
#include "common/world/gen/structure/structures/WoodlandMansionStructure.hpp"
#include "structures/TrialChambersStructure.hpp"
#include <spdlog/spdlog.h>

namespace mc::world::gen::structure {

// StructureRegistry 实现
bool StructureRegistry::s_initialized = false;

std::unordered_map<ResourceLocation, std::unique_ptr<Structure>>& StructureRegistry::getStructures()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<Structure>> structures;
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
    pools::Pools::initialize();

    // 注册原版结构（兜底路径：数据驱动未加载时使用，键为结构类型基础名）
    registerStructure(std::make_unique<RuinedPortalStructure>(ResourceLocation("minecraft", "ruined_portal")));
    registerStructure(std::make_unique<BuriedTreasureStructure>(ResourceLocation("minecraft", "buried_treasure")));
    registerStructure(std::make_unique<MineshaftStructure>(ResourceLocation("minecraft", "mineshaft")));
    registerStructure(std::make_unique<VillageStructure>(ResourceLocation("minecraft", "village")));
    registerStructure(std::make_unique<StrongholdStructure>(ResourceLocation("minecraft", "stronghold")));
    registerStructure(std::make_unique<DesertPyramidStructure>(ResourceLocation("minecraft", "desert_pyramid")));
    registerStructure(std::make_unique<JungleTempleStructure>(ResourceLocation("minecraft", "jungle_pyramid")));
    registerStructure(std::make_unique<OceanMonumentStructure>(ResourceLocation("minecraft", "monument")));
    registerStructure(std::make_unique<ShipwreckStructure>(ResourceLocation("minecraft", "shipwreck")));
    registerStructure(std::make_unique<OceanRuinStructure>(ResourceLocation("minecraft", "ocean_ruin")));
    registerStructure(std::make_unique<FortressStructure>(ResourceLocation("minecraft", "fortress")));
    registerStructure(std::make_unique<IglooStructure>(ResourceLocation("minecraft", "igloo")));
    registerStructure(std::make_unique<SwampHutStructure>(ResourceLocation("minecraft", "swamp_hut")));
    registerStructure(std::make_unique<NetherFossilStructure>(ResourceLocation("minecraft", "nether_fossil")));
    registerStructure(std::make_unique<PillagerOutpostStructure>(ResourceLocation("minecraft", "pillager_outpost")));
    registerStructure(std::make_unique<WoodlandMansionStructure>(ResourceLocation("minecraft", "mansion")));
    registerStructure(std::make_unique<EndCityStructure>(ResourceLocation("minecraft", "end_city")));
    registerStructure(std::make_unique<BastionRemnantStructure>(ResourceLocation("minecraft", "bastion_remnant")));
    registerStructure(std::make_unique<TrialChambersStructure>(ResourceLocation("minecraft", "trial_chambers")));

    s_initialized = true;
}

void StructureRegistry::clear()
{
    getStructures().clear();
    getStructureList().clear();
    s_initialized = false;
}

void StructureRegistry::markInitialized()
{
    s_initialized = true;
}

void StructureRegistry::registerStructure(std::unique_ptr<Structure> structure)
{
    if (!structure) return;

    const ResourceLocation& id = structure->id();
    auto& structures = getStructures();
    auto& list = getStructureList();

    if (structures.find(id) == structures.end()) {
        list.push_back(structure.get());
        structures[id] = std::move(structure);
    } else {
        spdlog::warn("StructureRegistry: Re registering structure {}", id.toString());
    }
}

const Structure* StructureRegistry::get(const ResourceLocation& id)
{
    auto& structures = getStructures();
    auto it = structures.find(id);
    return it != structures.end() ? it->second.get() : nullptr;
}

const Structure* StructureRegistry::get(const std::string& name)
{
    // 兼容旧接口：将字符串名称转换为 ResourceLocation
    ResourceLocation id = ResourceLocation::parse(name);
    return get(id);
}

const std::vector<const Structure*>& StructureRegistry::getAll()
{
    return getStructureList();
}

// StructureManager 实现
StructureManager::StructureManager(i64 seed)
    : m_seed(seed)
{}

std::unique_ptr<StructureStart> StructureManager::generateStructureStart(const Structure& structure,
    IWorldWriter& /*world*/,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ)
{
    // 调用结构的生成方法（不写方块，方块写入延迟到 FEATURES 阶段）
    return structure.generate(generator, rng, chunkX, chunkZ);
}

void StructureManager::placeStructureInChunk(
    const Structure& structure, IWorldWriter& world, ChunkPrimer& chunk, StructureStart& start, i32 chunkX, i32 chunkZ)
{
    // 调用结构的放置方法
    structure.placeInChunk(world, chunk, start, chunkX, chunkZ);

    // 调用放置后的钩子
    structure.afterPlace(world, start, chunkX, chunkZ);
}

void StructureManager::clearCache()
{
    m_structureCheck.clearCache();
}

math::Random StructureManager::_createRandom(i32 chunkX, i32 chunkZ, i32 salt) const
{
    // 结构生成使用的常量种子混合参数
    constexpr u64 CHUNK_X_MULTIPLIER = 341873128712ULL;
    constexpr u64 CHUNK_Z_MULTIPLIER = 132897987541ULL;

    u64 combinedSeed = static_cast<u64>(chunkX) * CHUNK_X_MULTIPLIER + static_cast<u64>(chunkZ) * CHUNK_Z_MULTIPLIER +
        static_cast<u64>(m_seed) + static_cast<u64>(salt);
    return math::Random(static_cast<i64>(combinedSeed));
}

} // namespace mc::world::gen::structure

// StructureRegistry loadTemplatePoolsFromDataPacks 实现
size_t mc::world::gen::structure::StructureRegistry::loadTemplatePoolsFromDataPacks(
    const resource::DataPackRepository& dataPackList)
{
    auto result = jigsaw::TemplatePoolLoader::loadFromDataPackRepository(dataPackList);
    if (result.success()) {
        return result.value();
    }
    return 0;
}
