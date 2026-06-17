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

#include "BiomeGenerationSettings.hpp"
#include "../../core/Constants.hpp"
#include "../block/BlockTags.hpp"
#include "../gen/carver/CanyonCarver.hpp"
#include "../gen/carver/CarverConfiguration.hpp"
#include "../gen/carver/CaveCarver.hpp"
#include "../gen/carver/NetherWorldCarver.hpp"
#include "../gen/carver/WorldCarver.hpp"
#include "../gen/feature/ConfiguredFeature.hpp"
#include "../gen/feature/FeatureIds.hpp"
#include <algorithm>

namespace mc {
namespace world {
namespace biome {

namespace {

/**
 * @brief 为主世界生物群系添加默认矿石特征
 * @param settings 生物群系生成设置
 *
 * 添加煤矿石、铁矿石、金矿石、红石矿石、钻石矿石、青金石矿石、铜矿石
 */
void addDefaultOverworldOres(BiomeGenerationSettings& settings)
{
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CoalOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::IronOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::GoldOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::RedstoneOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::DiamondOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::LapisOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CopperOre);
}

/**
 * @brief 为主世界生物群系添加标准雕刻器（洞穴 + 额外地下洞穴 + 峡谷）
 *
 * 所有主世界非海洋生物群系都有这三种雕刻器。
 * 参考: NoiseBasedChunkGenerator 中的 carver 列表注册。
 */
void addOverworldCarvers(BiomeGenerationSettings& settings)
{
    const BlockTag* replaceable = &BlockTags::OVERWORLD_CARVER_REPLACEABLES();

    auto caveCarver = std::make_unique<CaveCarver>();
    auto caveConfig = ConfiguredCarvers::createOverworldCaveConfig(replaceable);
    settings.addCarver(std::make_unique<ConfiguredCarver<CaveCarver, CaveCarverConfiguration>>(
        std::move(caveCarver), std::move(caveConfig)));

    auto caveExtraCarver = std::make_unique<CaveCarver>();
    auto caveExtraConfig = ConfiguredCarvers::createOverworldCaveExtraConfig(replaceable);
    settings.addCarver(std::make_unique<ConfiguredCarver<CaveCarver, CaveCarverConfiguration>>(
        std::move(caveExtraCarver), std::move(caveExtraConfig)));

    auto canyonCarver = std::make_unique<CanyonCarver>();
    auto canyonConfig = ConfiguredCarvers::createOverworldCanyonConfig(replaceable);
    settings.addCarver(std::make_unique<ConfiguredCarver<CanyonCarver, CanyonCarverConfiguration>>(
        std::move(canyonCarver), std::move(canyonConfig)));
}

/**
 * @brief 为主世界海洋生物群系添加雕刻器（洞穴 + 峡谷，无额外地下洞穴）
 *
 * 海洋生物群系只有 cave + canyon，没有 cave_extra_underground。
 */
void addOverworldOceanCarvers(BiomeGenerationSettings& settings)
{
    const BlockTag* replaceable = &BlockTags::OVERWORLD_CARVER_REPLACEABLES();

    auto caveCarver = std::make_unique<CaveCarver>();
    auto caveConfig = ConfiguredCarvers::createOverworldCaveConfig(replaceable);
    settings.addCarver(std::make_unique<ConfiguredCarver<CaveCarver, CaveCarverConfiguration>>(
        std::move(caveCarver), std::move(caveConfig)));

    auto canyonCarver = std::make_unique<CanyonCarver>();
    auto canyonConfig = ConfiguredCarvers::createOverworldCanyonConfig(replaceable);
    settings.addCarver(std::make_unique<ConfiguredCarver<CanyonCarver, CanyonCarverConfiguration>>(
        std::move(canyonCarver), std::move(canyonConfig)));
}

/**
 * @brief 为下界生物群系添加雕刻器（下界洞穴）
 *
 * 所有下界生物群系都只有 nether_cave。
 */
void addNetherCarvers(BiomeGenerationSettings& settings)
{
    const BlockTag* replaceable = &BlockTags::NETHER_CARVER_REPLACEABLES();

    auto netherCarver = std::make_unique<NetherWorldCarver>();
    auto netherConfig = ConfiguredCarvers::createNetherCaveConfig(replaceable);
    settings.addCarver(std::make_unique<ConfiguredCarver<NetherWorldCarver, CaveCarverConfiguration>>(
        std::move(netherCarver), std::move(netherConfig)));
}

} // namespace

// ============================================================================
// BiomeGenerationSettings 实现
// ============================================================================

BiomeGenerationSettings::BiomeGenerationSettings()
{
    // 预分配阶段数量
    m_featuresByStage.resize(static_cast<size_t>(DecorationStage::Count));
}

BiomeGenerationSettings::~BiomeGenerationSettings() = default;

BiomeGenerationSettings::BiomeGenerationSettings(BiomeGenerationSettings&& other) noexcept
    : m_featuresByStage(std::move(other.m_featuresByStage))
    , m_carvers(std::move(other.m_carvers))
{}

BiomeGenerationSettings& BiomeGenerationSettings::operator=(BiomeGenerationSettings&& other) noexcept
{
    if (this != &other) {
        m_featuresByStage = std::move(other.m_featuresByStage);
        m_carvers = std::move(other.m_carvers);
    }
    return *this;
}

void BiomeGenerationSettings::addFeature(DecorationStage stage, u32 featureId)
{
    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        return;
    }
    m_featuresByStage[stageIndex].push_back(featureId);
}

const std::vector<u32>& BiomeGenerationSettings::getFeatures(DecorationStage stage) const noexcept
{
    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        static const std::vector<u32> empty;
        return empty;
    }
    return m_featuresByStage[stageIndex];
}

bool BiomeGenerationSettings::hasFeatures() const noexcept
{
    for (const auto& features : m_featuresByStage) {
        if (!features.empty()) {
            return true;
        }
    }
    return false;
}

bool BiomeGenerationSettings::hasFeature(u32 featureId) const noexcept
{
    for (const auto& features : m_featuresByStage) {
        for (u32 id : features) {
            if (id == featureId) {
                return true;
            }
        }
    }
    return false;
}

void BiomeGenerationSettings::clear() noexcept
{
    for (auto& features : m_featuresByStage) {
        features.clear();
    }
    m_carvers.clear();
}

void BiomeGenerationSettings::addCarver(std::unique_ptr<ConfiguredCarverBase> carver)
{
    if (carver) {
        m_carvers.push_back(std::move(carver));
    }
}

const std::vector<std::unique_ptr<ConfiguredCarverBase>>& BiomeGenerationSettings::getCarvers() const noexcept
{
    return m_carvers;
}

bool BiomeGenerationSettings::hasCarvers() const noexcept
{
    return !m_carvers.empty();
}

BiomeGenerationSettings BiomeGenerationSettings::createDefault()
{
    // 默认设置：包含主世界矿石与基础湖泊
    BiomeGenerationSettings settings;

    // 主世界默认雕刻器
    addOverworldCarvers(settings);

    // 添加湖泊（LAKES 阶段）
    settings.addFeature(DecorationStage::Lakes, LakeFeatureIds::WaterLake);
    settings.addFeature(DecorationStage::Lakes, LakeFeatureIds::LavaLake);

    // 添加矿石（UNDERGROUND_ORES 阶段）
    addDefaultOverworldOres(settings);

    // 顶层冻结特征（TOP_LAYER_MODIFICATION 阶段）
    // 所有主世界生物群系都会获得此特征；shouldFreeze/shouldSnow 的温度检查
    // 会自动过滤掉温暖生物群系中的冰雪放置
    settings.addFeature(DecorationStage::TopLayerModification, SnowAndFreezeFeatureIds::FreezeTopLayer);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createPlains()
{
    // 平原：基础矿石 + 稀疏的树木 + 花卉 + 草丛
    BiomeGenerationSettings settings = createDefault();

    // 添加稀疏橡树
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::SparseOakTree);

    // 添加平原花卉
    settings.addFeature(DecorationStage::VegetalDecoration, FlowerFeatureIds::PlainsFlowers);

    // 添加平原草丛
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::PlainsGrass);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createForest()
{
    // 森林：基础矿石 + 密集的树木 + 森林花卉 + 森林草丛
    BiomeGenerationSettings settings = createDefault();

    // 添加树木（多添加几次增加密度）
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::OakTree);
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::OakTree);
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::BirchTree);

    // 添加森林花卉
    settings.addFeature(DecorationStage::VegetalDecoration, FlowerFeatureIds::ForestFlowers);

    // 添加森林草丛
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::ForestGrass);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createTaiga()
{
    // 针叶林：基础矿石 + 云杉树 + 针叶林草丛
    BiomeGenerationSettings settings = createDefault();

    // 添加云杉树
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::SpruceTree);
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::SpruceTree);

    // 添加针叶林草丛（蕨类）
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::TaigaGrass);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createJungle()
{
    // 丛林：基础矿石 + 丛林树 + 丛林草丛
    BiomeGenerationSettings settings = createDefault();

    // 添加丛林树
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::JungleTree);
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::JungleTree);

    // 添加丛林草丛
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::JungleGrass);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createSavanna()
{
    // 稀树草原：基础矿石 + 稀疏橡树 + 稀树草原草丛
    BiomeGenerationSettings settings = createDefault();

    // 添加稀疏橡树（代替未实现的相思树）
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::SparseOakTree);

    // 添加稀树草原草丛
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::SavannaGrass);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDesert()
{
    // 沙漠：矿石 + 仙人掌 + 枯萎灌木
    BiomeGenerationSettings settings = createDefault();

    // 添加沙漠仙人掌
    settings.addFeature(DecorationStage::VegetalDecoration, CactusFeatureIds::DesertCactus);

    // 添加枯萎灌木（使用恶地枯萎灌木ID）
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::BadlandsDeadBush);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createSwamp()
{
    // 沼泽：矿石 + 橡树 + 沼泽花卉 + 沼泽草丛 + 甘蔗 + 巨型蘑菇
    BiomeGenerationSettings settings = createDefault();

    // 添加稀疏橡树
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::SparseOakTree);

    // 添加沼泽花卉（兰花）
    settings.addFeature(DecorationStage::VegetalDecoration, FlowerFeatureIds::SwampFlowers);

    // 添加沼泽草丛
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::SwampGrass);

    // 添加密集甘蔗
    settings.addFeature(DecorationStage::VegetalDecoration, SugarCaneFeatureIds::Dense);

    // 添加巨型蘑菇
    settings.addFeature(DecorationStage::VegetalDecoration, MushroomFeatureIds::BrownMushroom);
    settings.addFeature(DecorationStage::VegetalDecoration, MushroomFeatureIds::RedMushroom);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createRiver()
{
    // 河流：矿石 + 河岸甘蔗 + 浅水海草（不额外生成湖泊）
    BiomeGenerationSettings settings;

    addOverworldCarvers(settings);
    addDefaultOverworldOres(settings);

    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Simple);
    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Simple);
    settings.addFeature(DecorationStage::VegetalDecoration, SugarCaneFeatureIds::Normal);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createFrozenRiver()
{
    // 冻河：保留基础矿石，不放置温暖水域植被
    BiomeGenerationSettings settings;

    addOverworldCarvers(settings);
    addDefaultOverworldOres(settings);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createSwampHills()
{
    // 沼泽山丘：保留湿地装饰，但降低甘蔗密度
    BiomeGenerationSettings settings = createDefault();

    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::SparseOakTree);
    settings.addFeature(DecorationStage::VegetalDecoration, FlowerFeatureIds::SwampFlowers);
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::SwampGrass);
    settings.addFeature(DecorationStage::VegetalDecoration, SugarCaneFeatureIds::Normal);
    settings.addFeature(DecorationStage::VegetalDecoration, MushroomFeatureIds::BrownMushroom);
    settings.addFeature(DecorationStage::VegetalDecoration, MushroomFeatureIds::RedMushroom);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createIceSpikes()
{
    // 冰刺平原：矿石 + 冰刺结构
    BiomeGenerationSettings settings = createDefault();

    // 添加冰刺和冰丘（SurfaceStructures 阶段）
    settings.addFeature(DecorationStage::SurfaceStructures, IceSpikeFeatureIds::Spike);
    settings.addFeature(DecorationStage::SurfaceStructures, IceSpikeFeatureIds::Iceberg);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createBadlands()
{
    // 恶地：矿石 + 恶地仙人掌 + 枯萎灌木
    BiomeGenerationSettings settings = createDefault();

    // 添加恶地仙人掌（比沙漠仙人掌更高）
    settings.addFeature(DecorationStage::VegetalDecoration, CactusFeatureIds::BadlandsCactus);

    // 添加枯萎灌木
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::BadlandsDeadBush);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createFlowerForest()
{
    // 繁花森林：矿石 + 密集树木 + 繁花森林花卉 + 森林草丛
    BiomeGenerationSettings settings = createDefault();

    // 添加树木
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::OakTree);
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::BirchTree);

    // 添加繁花森林花卉（最丰富的花卉）
    settings.addFeature(DecorationStage::VegetalDecoration, FlowerFeatureIds::FlowerForestFlowers);
    settings.addFeature(DecorationStage::VegetalDecoration, FlowerFeatureIds::FlowerForestFlowers);

    // 添加森林草丛
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::ForestGrass);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createCherryGrove()
{
    // 樱花树林：矿石 + 樱花树 + 粉色花瓣 + 草丛
    BiomeGenerationSettings settings = createDefault();

    // 添加樱花树（两次增加密度）
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::CherryTree);
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::CherryTree);

    // 添加粉色花瓣
    settings.addFeature(DecorationStage::VegetalDecoration, FlowerFeatureIds::CherryGrovePetals);

    // 添加草丛
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::PlainsGrass);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createMountains()
{
    // 山地：矿石 + 绿宝石 + 云杉树 + 针叶林草丛
    BiomeGenerationSettings settings = createDefault();

    // 添加绿宝石矿石
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::EmeraldOre);

    // 添加云杉树
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::SpruceTree);

    // 添加针叶林草丛
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::TaigaGrass);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createOcean()
{
    // 常温海洋：海草 + 海带（参考原版普通海洋组合）
    BiomeGenerationSettings settings;

    addOverworldOceanCarvers(settings);

    addDefaultOverworldOres(settings);
    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Simple);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDeepOcean()
{
    // 深海：深海草 + 海带（参考原版深海组合）
    BiomeGenerationSettings settings;

    addOverworldOceanCarvers(settings);

    addDefaultOverworldOres(settings);
    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Mixed);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createWarmOcean()
{
    // 暖水海洋：珊瑚植被 + 暖水海草 + 海泡菜
    BiomeGenerationSettings settings;

    addOverworldOceanCarvers(settings);

    addDefaultOverworldOres(settings);

    // 以活珊瑚结构模拟 warm_ocean_vegetation
    settings.addFeature(DecorationStage::VegetalDecoration, CoralFeatureIds::Tube);
    settings.addFeature(DecorationStage::VegetalDecoration, CoralFeatureIds::Brain);
    settings.addFeature(DecorationStage::VegetalDecoration, CoralFeatureIds::Bubble);
    settings.addFeature(DecorationStage::VegetalDecoration, CoralFeatureIds::Fire);
    settings.addFeature(DecorationStage::VegetalDecoration, CoralFeatureIds::Horn);

    // 海泡菜（暖水海洋）
    settings.addFeature(DecorationStage::VegetalDecoration, SeaPickleFeatureIds::Normal);

    // 暖水海草
    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Warm);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createLukewarmOcean()
{
    // 温水海洋：海带 + 常规海草
    BiomeGenerationSettings settings;

    addOverworldOceanCarvers(settings);

    addDefaultOverworldOres(settings);

    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Normal);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Warm);
    settings.addFeature(DecorationStage::VegetalDecoration, SeaPickleFeatureIds::Normal);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createColdOcean()
{
    // 冷水海洋：海带 + 冷水海草
    BiomeGenerationSettings settings;

    addOverworldOceanCarvers(settings);

    addDefaultOverworldOres(settings);

    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createFrozenOcean()
{
    // 冻洋：冷水植被 + 蓝冰（参考原版 frozen ocean 特征）
    BiomeGenerationSettings settings;

    addOverworldOceanCarvers(settings);

    addDefaultOverworldOres(settings);

    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Cold);

    settings.addFeature(DecorationStage::VegetalDecoration, BlueIceFeatureIds::Normal);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDeepWarmOcean()
{
    // 深暖水海洋：深海草 + 常规海草（原版无海带）
    BiomeGenerationSettings settings;

    addOverworldOceanCarvers(settings);

    addDefaultOverworldOres(settings);

    settings.addFeature(DecorationStage::VegetalDecoration, CoralFeatureIds::Tube);
    settings.addFeature(DecorationStage::VegetalDecoration, CoralFeatureIds::Brain);
    settings.addFeature(DecorationStage::VegetalDecoration, CoralFeatureIds::Bubble);
    settings.addFeature(DecorationStage::VegetalDecoration, CoralFeatureIds::Fire);
    settings.addFeature(DecorationStage::VegetalDecoration, CoralFeatureIds::Horn);
    settings.addFeature(DecorationStage::VegetalDecoration, SeaPickleFeatureIds::Normal);
    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::DeepWarm);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDeepLukewarmOcean()
{
    // 深温水海洋：深海草 + 海带
    // 注意：深海温水海洋没有珊瑚，珊瑚只在暖水海洋生成
    BiomeGenerationSettings settings;

    addOverworldOceanCarvers(settings);

    addDefaultOverworldOres(settings);

    // 无珊瑚特征 - 珊瑚只在暖水海洋生成
    settings.addFeature(DecorationStage::VegetalDecoration, SeaPickleFeatureIds::Normal);
    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Deep);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Warm);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDeepColdOcean()
{
    // 深冷水海洋：深海草 + 海带
    BiomeGenerationSettings settings;

    addOverworldOceanCarvers(settings);

    addDefaultOverworldOres(settings);

    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::DeepCold);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDeepFrozenOcean()
{
    // 深冻洋：冷水植被 + 蓝冰
    BiomeGenerationSettings settings;

    addOverworldOceanCarvers(settings);

    addDefaultOverworldOres(settings);

    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::DeepCold);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, BlueIceFeatureIds::Normal);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

// ============================================================================
// 下界生物群系生成设置
// ============================================================================

BiomeGenerationSettings BiomeGenerationSettings::createNether()
{
    // 下界荒地：下界石英矿石 + 下界金矿石 + 萤石 + 岩浆池
    BiomeGenerationSettings settings;

    addNetherCarvers(settings);

    // 下界矿石
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherQuartzOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherGoldOre);

    // 萤石簇 (UndergroundDecoration 阶段)
    settings.addFeature(DecorationStage::UndergroundDecoration, GlowstoneFeatureIds::Normal);
    settings.addFeature(DecorationStage::UndergroundDecoration, GlowstoneFeatureIds::Large);

    // 岩浆池
    settings.addFeature(DecorationStage::UndergroundDecoration, MagmaFeatureIds::PatchNormal);

    // TODO: 添加下界要塞、堡垒遗迹等结构

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createSoulSandValley()
{
    // 灵魂沙谷：下界矿石 + 玄武岩柱 + 火焰
    BiomeGenerationSettings settings;

    addNetherCarvers(settings);

    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherQuartzOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherGoldOre);

    // 玄武岩柱
    settings.addFeature(DecorationStage::UndergroundDecoration, BasaltFeatureIds::ColumnNormal);
    settings.addFeature(DecorationStage::UndergroundDecoration, BasaltFeatureIds::ColumnLarge);

    // 下界火焰
    settings.addFeature(DecorationStage::VegetalDecoration, NetherFungusIds::NetherFire);

    // TODO: 添加下界化石结构

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createCrimsonForest()
{
    // 绯红森林：下界矿石 + 绯红巨型真菌 + 岩浆池
    BiomeGenerationSettings settings;

    addNetherCarvers(settings);

    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherQuartzOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherGoldOre);

    // 绯红巨型真菌
    settings.addFeature(DecorationStage::VegetalDecoration, NetherFungusIds::CrimsonFungus);
    settings.addFeature(DecorationStage::VegetalDecoration, NetherFungusIds::CrimsonFungus); // 增加密度

    // 岩浆池（较少）
    settings.addFeature(DecorationStage::UndergroundDecoration, MagmaFeatureIds::PatchNormal);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createWarpedForest()
{
    // 诡异森林：下界矿石 + 诡异巨型真菌
    BiomeGenerationSettings settings;

    addNetherCarvers(settings);

    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherQuartzOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherGoldOre);

    // 诡异巨型真菌
    settings.addFeature(DecorationStage::VegetalDecoration, NetherFungusIds::WarpedFungus);
    settings.addFeature(DecorationStage::VegetalDecoration, NetherFungusIds::WarpedFungus); // 增加密度

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createBasaltDeltas()
{
    // 玄武岩三角洲：下界矿石 + 玄武岩柱 + 玄武岩地面 + 岩浆池
    BiomeGenerationSettings settings;

    addNetherCarvers(settings);

    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherQuartzOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherGoldOre);

    // 玄武岩柱（密集）
    settings.addFeature(DecorationStage::UndergroundDecoration, BasaltFeatureIds::ColumnNormal);
    settings.addFeature(DecorationStage::UndergroundDecoration, BasaltFeatureIds::ColumnNormal);
    settings.addFeature(DecorationStage::UndergroundDecoration, BasaltFeatureIds::ColumnLarge);

    // 玄武岩三角洲地面
    settings.addFeature(DecorationStage::UndergroundDecoration, BasaltFeatureIds::Delta);

    // 密集岩浆池
    settings.addFeature(DecorationStage::UndergroundDecoration, MagmaFeatureIds::PatchDense);
    settings.addFeature(DecorationStage::UndergroundDecoration, MagmaFeatureIds::PatchDense);

    return settings;
}

// ============================================================================
// 洞穴生物群系生成设置
// ============================================================================

BiomeGenerationSettings BiomeGenerationSettings::createLushCaves()
{
    // 繁茂洞穴：矿石 + 苔藓 + 洞穴藤蔓 + 孢子花 + 黏土池 + 垂滴叶 + 杜鹃树 + 藤蔓
    BiomeGenerationSettings settings = createDefault();

    // VegetalDecoration阶段 - 繁茂洞穴特有特征
    // 注意：特征ID是全局ID，包含之前所有特征组的偏移量
    settings.addFeature(DecorationStage::VegetalDecoration, LushCaveFeatureIds::LushCavesCeilingVegetation);
    settings.addFeature(DecorationStage::VegetalDecoration, LushCaveFeatureIds::CaveVines);
    settings.addFeature(DecorationStage::VegetalDecoration, LushCaveFeatureIds::LushCavesClay);
    settings.addFeature(DecorationStage::VegetalDecoration, LushCaveFeatureIds::LushCavesVegetation);
    settings.addFeature(DecorationStage::VegetalDecoration, LushCaveFeatureIds::RootedAzaleaTree);
    settings.addFeature(DecorationStage::VegetalDecoration, LushCaveFeatureIds::SporeBlossom);
    settings.addFeature(DecorationStage::VegetalDecoration, LushCaveFeatureIds::ClassicVines);

    return settings;
}

// ============================================================================
// 末地生物群系生成设置
// ============================================================================

BiomeGenerationSettings BiomeGenerationSettings::createTheEnd()
{
    // 末地主岛：黑曜石柱 + 末影龙战斗
    BiomeGenerationSettings settings;

    // 黑曜石柱（主岛核心地表结构）
    settings.addFeature(DecorationStage::SurfaceStructures, EndSurfaceFeatureIds::ObsidianSpike);

    // 末地主岛没有常规矿石生成

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createSmallEndIslands()
{
    // 小型末地岛屿：末地岛特征
    BiomeGenerationSettings settings;

    // TODO: 添加末地岛特征

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createEndMidlands()
{
    // 末地中部：过渡区域，无特殊特征
    BiomeGenerationSettings settings;

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createEndHighlands()
{
    // 末地高地：末地城 + 紫颂树
    BiomeGenerationSettings settings;

    // 末地折跃门（用于连接外岛区域）
    settings.addFeature(DecorationStage::SurfaceStructures, EndSurfaceFeatureIds::EndGateway);

    // TODO: 添加末地城结构
    // TODO: 添加紫颂树特征

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createEndBarrens()
{
    // 末地荒地：空旷区域，无特征
    BiomeGenerationSettings settings;

    return settings;
}

} // namespace biome
} // namespace world
} // namespace mc
