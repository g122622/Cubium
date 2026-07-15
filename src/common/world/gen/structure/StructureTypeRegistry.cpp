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

#include "StructureTypeRegistry.hpp"

#include "JigsawStructure.hpp"
#include "StructureDefinitionLoader.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/gen/jigsaw/PoolAliasBinding.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"
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
#include "structures/TrialChambersStructure.hpp"
#include "structures/VillageStructure.hpp"
#include "structures/WoodlandMansionStructure.hpp"

#include <spdlog/spdlog.h>

namespace mc {
namespace world::gen::structure {

namespace {

/**
 * @brief 剥离 "minecraft:" 命名空间前缀（type 字符串用）
 */
std::string stripNamespace(const std::string& s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return s.substr(prefix.size());
    }
    return s;
}

/**
 * @brief 把 def.biomes（形如 "#minecraft:has_structure/village_plains"）解析为 BiomeTag*
 *
 * ResourceLocation 构造器按 ':' 切分，故 "#minecraft:..." 的命名空间被存为 "#minecraft"（含 '#')。
 * BiomeTags::getTag 期望无 '#' 前缀的 minecraft:has_structure/X，此处剥离。
 * 标签未加载返回 nullptr（不报错：biome 标签表可能晚于结构加载，回退到子类 defaultBiomeTag）。
 */
const biome::BiomeTag* resolveBiomeTag(const ResourceLocation& biomes)
{
    if (!biomes.isValid()) {
        return nullptr;
    }
    std::string ns = biomes.namespace_();
    std::string path = biomes.path();
    if (!ns.empty() && ns[0] == '#') {
        ns = ns.substr(1);
    }
    return biome::BiomeTags::getTag(ResourceLocation(ns, path));
}

/**
 * @brief 把派生 Structure unique_ptr 提升为基类 unique_ptr
 */
std::unique_ptr<Structure> toBase(std::unique_ptr<Structure> s)
{
    return s;
}

/**
 * @brief 从 def 构造 JigsawConfig（用于裸 JigsawStructure 工厂）
 *
 * 字段一一对应：startPool/size/startHeight/poolAliases/maxDistance/dimensionPadding/liquidSettings。
 * startHeight 缺省（nullptr）时构造 ConstantHeight(absolute 0)，避免 JigsawConfig 空高度。
 */
JigsawConfig buildJigsawConfig(const StructureDefinition& def)
{
    // 起始高度：def 已解析为 HeightProvider（简写锚点或分派形式）；
    // 缺省时回退 absolute 0（与 MC 默认 start_height 一致）。
    // def 所有权留在 Loader，JigsawConfig 需自有实例，故深拷贝（HeightProvider::clone）。
    std::unique_ptr<valueprovider::HeightProvider> height = def.startHeight
        ? def.startHeight->clone()
        : valueprovider::ConstantHeight::create(valueprovider::VerticalAnchor::absolute(0));

    // pool_aliases 同理深拷贝：PoolAliasBindings 持 vector<unique_ptr> 不可拷贝，
    // 逐个 clone() 重建到新集合，def 原集合保持完整。
    jigsaw::PoolAliasBindings aliases;
    for (const auto& binding : def.poolAliases.bindings()) {
        aliases.addBinding(binding->clone());
    }

    JigsawConfig config(def.startPool,
        def.size,
        std::move(height),
        std::move(aliases),
        def.maxDistanceFromCenter,
        def.dimensionPadding,
        def.liquidSettings);

    if (def.startJigsawName.has_value()) {
        config.startJigsawName = def.startJigsawName;
    }
    config.projectStartToHeightmap = def.projectStartToHeightmap;
    return config;
}

/**
 * @brief 从 def.id 路径段推断 VillageType
 *
 * Vanilla 5 个村庄 JSON：village_desert/village_plains/village_savanna/village_snowy/village_taiga。
 * def.id.path() 形如 "village_desert"。
 */
VillageType villageTypeFromId(const ResourceLocation& id)
{
    const std::string& p = id.path();
    if (p == "village_desert") return VillageType::Desert;
    if (p == "village_savanna") return VillageType::Savanna;
    if (p == "village_snowy") return VillageType::Snowy;
    if (p == "village_taiga") return VillageType::Taiga;
    return VillageType::Plains; // village_plains 及未知
}

/**
 * @brief 应用数据驱动覆盖字段到已构造的结构实例
 *
 * 注入 biomes/step/terrainAdaptation（覆盖子类硬编码默认）。spawn_overrides 暂缓：
 * def.spawnOverrides 是按类别分键的 StructureSpawnOverrideMap，与现有平铺 SpawnOverrides*
 * 形状不符，待后续接入生物生成链路时补全。
 */
void applyDefinitionOverrides(Structure& structure, const StructureDefinition& def)
{
    if (const biome::BiomeTag* tag = resolveBiomeTag(def.biomes)) {
        structure.setBiomeTag(tag);
    }
    structure.setDecorationStage(def.step);
    structure.setTerrainAdaptation(def.terrainAdaptation);
}

// ----------------------------------------------------------------------------
// minecraft:jigsaw 工厂
// ----------------------------------------------------------------------------

/**
 * @brief jigsaw 类型工厂
 *
 * 按 def.id 分流：
 * - pillager_outpost / trial_chambers：委托子类（构造期硬编码 JigsawConfig + 特殊逻辑：
 *   村庄距离检查 / 池别名创建）。
 * - village_*：委托 VillageStructure（按 id 选 VillageType）。
 * - bastion_remnant：委托 BastionRemnantStructure（运行时加权选 4 种堡垒）。
 * - 其余 id（ancient_city / trail_ruins / 自定义）：裸 JigsawStructure，JigsawConfig 从 def 填充。
 */
Result<std::unique_ptr<Structure>> createJigsaw(const StructureDefinition& def)
{
    const std::string& p = def.id.path();

    if (p == "pillager_outpost") {
        return toBase(std::make_unique<PillagerOutpostStructure>());
    }
    if (p == "trial_chambers") {
        return toBase(std::make_unique<TrialChambersStructure>());
    }
    if (p == "bastion_remnant") {
        return toBase(std::make_unique<BastionRemnantStructure>());
    }
    if (p.rfind("village", 0) == 0) { // village / village_desert / ...
        return toBase(std::make_unique<VillageStructure>(villageTypeFromId(def.id)));
    }

    // 裸 JigsawStructure：必须提供 start_pool（默认 ResourceLocation 命名空间为 "minecraft"、
    // 路径为空，故以路径非空判定是否显式提供池）。
    if (def.startPool.path().empty()) {
        return Error(ErrorCode::InvalidData, "jigsaw structure '" + def.id.toString() + "' missing 'start_pool'");
    }
    auto structure = std::make_unique<JigsawStructure>(def.id, buildJigsawConfig(def));
    return toBase(std::move(structure));
}

// ----------------------------------------------------------------------------
// 程序化类型工厂（15 种）
// ----------------------------------------------------------------------------

Result<std::unique_ptr<Structure>> createMineshaft(const StructureDefinition& def)
{
    // mineshaft_type: normal（默认）/ mesa。def.id 路径 "mineshaft" 或 "mineshaft_mesa"。
    if (def.id.path() == "mineshaft_mesa") {
        return toBase(std::make_unique<MineshaftStructure>(MineshaftType::Mesa));
    }
    return toBase(std::make_unique<MineshaftStructure>(MineshaftType::Normal));
}

/// 程序化零参构造子类工厂（无变体分流，运行时按生物群系判定变体）
template <typename T>
Result<std::unique_ptr<Structure>> createZeroArg(const StructureDefinition& /*def*/)
{
    return toBase(std::make_unique<T>());
}

} // namespace

// ----------------------------------------------------------------------------
// StructureTypeRegistry 实现
// ----------------------------------------------------------------------------

StructureTypeRegistry& StructureTypeRegistry::instance()
{
    static StructureTypeRegistry s_instance;
    return s_instance;
}

void StructureTypeRegistry::registerType(const std::string& type, Factory factory)
{
    m_factories[stripNamespace(type)] = std::move(factory);
}

Result<std::unique_ptr<Structure>> StructureTypeRegistry::create(
    const std::string& type, const StructureDefinition& def) const
{
    const std::string key = stripNamespace(type);
    const auto it = m_factories.find(key);
    if (it == m_factories.end()) {
        return Error(ErrorCode::NotFound,
            "Unregistered structure type: '" + type +
                "'. This structure type has no C++ implementation yet; "
                "implement it and register in StructureTypeRegistry.");
    }

    auto result = it->second(def);
    if (!result.success()) {
        return result;
    }
    auto structure = result.value();
    if (!structure) {
        return Error(ErrorCode::InvalidData, "structure factory returned null for type '" + type + "'");
    }

    // 应用数据驱动覆盖字段（biomes/step/terrainAdaptation）
    applyDefinitionOverrides(*structure, def);
    return structure;
}

bool StructureTypeRegistry::has(const std::string& type) const noexcept
{
    return m_factories.find(stripNamespace(type)) != m_factories.end();
}

void StructureTypeRegistry::clear() noexcept
{
    m_factories.clear();
}

/**
 * @brief 初始化内置结构类型工厂
 *
 * 注册全部 16 种 Vanilla structure type（minecraft:jigsaw + 15 程序化类型）。
 * 未注册的 type 在加载时严格报错（见 create()）。
 */
void initializeBuiltinStructureTypes()
{
    auto& reg = StructureTypeRegistry::instance();
    reg.clear();

    // jigsaw 类型（9 个 Vanilla JSON + 自定义共用此工厂）
    reg.registerType("jigsaw", createJigsaw);

    // 程序化类型（15 种），按 def.id 分流变体或零参构造
    reg.registerType("mineshaft", createMineshaft);
    reg.registerType("buried_treasure", createZeroArg<BuriedTreasureStructure>);
    reg.registerType("desert_pyramid", createZeroArg<DesertPyramidStructure>);
    reg.registerType("end_city", createZeroArg<EndCityStructure>);
    reg.registerType("fortress", createZeroArg<FortressStructure>);
    reg.registerType("igloo", createZeroArg<IglooStructure>);
    reg.registerType("jungle_pyramid", createZeroArg<JungleTempleStructure>);
    reg.registerType("nether_fossil", createZeroArg<NetherFossilStructure>);
    reg.registerType("monument", createZeroArg<OceanMonumentStructure>);
    reg.registerType("ocean_ruin", createZeroArg<OceanRuinStructure>);
    reg.registerType("ruined_portal", createZeroArg<RuinedPortalStructure>);
    reg.registerType("shipwreck", createZeroArg<ShipwreckStructure>);
    reg.registerType("stronghold", createZeroArg<StrongholdStructure>);
    reg.registerType("swamp_hut", createZeroArg<SwampHutStructure>);
    reg.registerType("woodland_mansion", createZeroArg<WoodlandMansionStructure>);
}

} // namespace world::gen::structure
} // namespace mc
