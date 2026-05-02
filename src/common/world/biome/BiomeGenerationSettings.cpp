#include "BiomeGenerationSettings.hpp"
#include "../gen/feature/ConfiguredFeature.hpp"
#include "../gen/feature/FeatureIds.hpp"
#include "../gen/chunk/IChunkGenerator.hpp"
#include "../chunk/ChunkPrimer.hpp"
#include "../../util/math/random/Random.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {

namespace {

void addDefaultOverworldOres(BiomeGenerationSettings& settings) {
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CoalOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::IronOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::GoldOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::RedstoneOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::DiamondOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::LapisOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CopperOre);
}

} // namespace

// ============================================================================
// BiomeGenerationSettings 实现
// ============================================================================

BiomeGenerationSettings::BiomeGenerationSettings() {
    // 预分配阶段数量
    m_featuresByStage.resize(static_cast<size_t>(DecorationStage::Count));
}

BiomeGenerationSettings::~BiomeGenerationSettings() = default;

void BiomeGenerationSettings::addFeature(DecorationStage stage, u32 featureId) {
    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        return;
    }
    m_featuresByStage[stageIndex].push_back(featureId);
}

const std::vector<u32>& BiomeGenerationSettings::getFeatures(DecorationStage stage) const {
    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        static const std::vector<u32> empty;
        return empty;
    }
    return m_featuresByStage[stageIndex];
}

bool BiomeGenerationSettings::hasFeatures() const {
    for (const auto& features : m_featuresByStage) {
        if (!features.empty()) {
            return true;
        }
    }
    return false;
}

void BiomeGenerationSettings::clear() {
    for (auto& features : m_featuresByStage) {
        features.clear();
    }
}

BiomeGenerationSettings BiomeGenerationSettings::createDefault() {
    // 默认设置：包含主世界矿石与基础湖泊
    BiomeGenerationSettings settings;

    // 添加湖泊（LAKES 阶段）
    settings.addFeature(DecorationStage::Lakes, LakeFeatureIds::WaterLake);
    settings.addFeature(DecorationStage::Lakes, LakeFeatureIds::LavaLake);

    // 添加矿石（UNDERGROUND_ORES 阶段）
    // 使用 FeatureIds 中定义的常量
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CoalOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::IronOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::GoldOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::RedstoneOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::DiamondOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::LapisOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CopperOre);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createPlains() {
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

BiomeGenerationSettings BiomeGenerationSettings::createForest() {
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

BiomeGenerationSettings BiomeGenerationSettings::createTaiga() {
    // 针叶林：基础矿石 + 云杉树 + 针叶林草丛
    BiomeGenerationSettings settings = createDefault();

    // 添加云杉树
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::SpruceTree);
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::SpruceTree);

    // 添加针叶林草丛（蕨类）
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::TaigaGrass);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createJungle() {
    // 丛林：基础矿石 + 丛林树 + 丛林草丛
    BiomeGenerationSettings settings = createDefault();

    // 添加丛林树
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::JungleTree);
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::JungleTree);

    // 添加丛林草丛
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::JungleGrass);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createSavanna() {
    // 稀树草原：基础矿石 + 稀疏橡树 + 稀树草原草丛
    BiomeGenerationSettings settings = createDefault();

    // 添加稀疏橡树（代替未实现的相思树）
    settings.addFeature(DecorationStage::VegetalDecoration, TreeFeatureIds::SparseOakTree);

    // 添加稀树草原草丛
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::SavannaGrass);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDesert() {
    // 沙漠：矿石 + 仙人掌 + 枯萎灌木
    BiomeGenerationSettings settings = createDefault();

    // 添加沙漠仙人掌
    settings.addFeature(DecorationStage::VegetalDecoration, CactusFeatureIds::DesertCactus);

    // 添加枯萎灌木（使用恶地枯萎灌木ID）
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::BadlandsDeadBush);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createSwamp() {
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

BiomeGenerationSettings BiomeGenerationSettings::createRiver() {
    // 河流：矿石 + 河岸甘蔗 + 浅水海草（不额外生成湖泊）
    BiomeGenerationSettings settings;

    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CoalOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::IronOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::GoldOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::RedstoneOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::DiamondOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::LapisOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CopperOre);

    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Simple);
    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Simple);
    settings.addFeature(DecorationStage::VegetalDecoration, SugarCaneFeatureIds::Normal);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createFrozenRiver() {
    // 冻河：保留基础矿石，不放置温暖水域植被
    BiomeGenerationSettings settings;

    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CoalOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::IronOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::GoldOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::RedstoneOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::DiamondOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::LapisOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CopperOre);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createSwampHills() {
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

BiomeGenerationSettings BiomeGenerationSettings::createIceSpikes() {
    // 冰刺平原：矿石 + 冰刺结构
    BiomeGenerationSettings settings = createDefault();

    // 添加冰刺和冰丘（SurfaceStructures 阶段）
    settings.addFeature(DecorationStage::SurfaceStructures, IceSpikeFeatureIds::Spike);
    settings.addFeature(DecorationStage::SurfaceStructures, IceSpikeFeatureIds::Iceberg);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createBadlands() {
    // 恶地：矿石 + 恶地仙人掌 + 枯萎灌木
    BiomeGenerationSettings settings = createDefault();

    // 添加恶地仙人掌（比沙漠仙人掌更高）
    settings.addFeature(DecorationStage::VegetalDecoration, CactusFeatureIds::BadlandsCactus);

    // 添加枯萎灌木
    settings.addFeature(DecorationStage::VegetalDecoration, GrassFeatureIds::BadlandsDeadBush);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createFlowerForest() {
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

BiomeGenerationSettings BiomeGenerationSettings::createMountains() {
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

BiomeGenerationSettings BiomeGenerationSettings::createOcean() {
    // 常温海洋：海草 + 海带（参考原版普通海洋组合）
    BiomeGenerationSettings settings;

    addDefaultOverworldOres(settings);
    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Simple);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDeepOcean() {
    // 深海：深海草 + 海带（参考原版深海组合）
    BiomeGenerationSettings settings;

    addDefaultOverworldOres(settings);
    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Mixed);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createWarmOcean() {
    // 暖水海洋：珊瑚植被 + 暖水海草 + 海泡菜
    BiomeGenerationSettings settings;

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

BiomeGenerationSettings BiomeGenerationSettings::createLukewarmOcean() {
    // 温水海洋：海带 + 常规海草
    BiomeGenerationSettings settings;

    addDefaultOverworldOres(settings);

    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Normal);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Warm);
    settings.addFeature(DecorationStage::VegetalDecoration, SeaPickleFeatureIds::Normal);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createColdOcean() {
    // 冷水海洋：海带 + 冷水海草
    BiomeGenerationSettings settings;

    addDefaultOverworldOres(settings);

    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createFrozenOcean() {
    // 冻洋：冷水植被 + 蓝冰（参考原版 frozen ocean 特征）
    BiomeGenerationSettings settings;

    addDefaultOverworldOres(settings);

    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Cold);

    settings.addFeature(DecorationStage::VegetalDecoration, BlueIceFeatureIds::Normal);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDeepWarmOcean() {
    // 深暖水海洋：深海草 + 常规海草（原版无海带）
    BiomeGenerationSettings settings;

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

BiomeGenerationSettings BiomeGenerationSettings::createDeepLukewarmOcean() {
    // 深温水海洋：深海草 + 海带
    // 注意：MC 1.16.5 中深海温水海洋没有珊瑚，珊瑚只在暖水海洋生成
    BiomeGenerationSettings settings;

    addDefaultOverworldOres(settings);

    // 无珊瑚特征 - 珊瑚只在暖水海洋生成
    settings.addFeature(DecorationStage::VegetalDecoration, SeaPickleFeatureIds::Normal);
    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::Deep);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Warm);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDeepColdOcean() {
    // 深冷水海洋：深海草 + 海带
    BiomeGenerationSettings settings;

    addDefaultOverworldOres(settings);

    settings.addFeature(DecorationStage::VegetalDecoration, SeagrassFeatureIds::DeepCold);
    settings.addFeature(DecorationStage::VegetalDecoration, KelpFeatureIds::Cold);
    settings.addFeature(DecorationStage::VegetalDecoration, OceanDecorationFeatureIds::OceanProps);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createDeepFrozenOcean() {
    // 深冻洋：冷水植被 + 蓝冰
    BiomeGenerationSettings settings;

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

BiomeGenerationSettings BiomeGenerationSettings::createNether() {
    // 下界荒地：下界石英矿石 + 下界金矿石 + 萤石 + 岩浆池
    BiomeGenerationSettings settings;

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

BiomeGenerationSettings BiomeGenerationSettings::createSoulSandValley() {
    // 灵魂沙谷：下界矿石 + 玄武岩柱 + 火焰
    BiomeGenerationSettings settings;

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

BiomeGenerationSettings BiomeGenerationSettings::createCrimsonForest() {
    // 绯红森林：下界矿石 + 绯红巨型真菌 + 岩浆池
    BiomeGenerationSettings settings;

    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherQuartzOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherGoldOre);

    // 绯红巨型真菌
    settings.addFeature(DecorationStage::VegetalDecoration, NetherFungusIds::CrimsonFungus);
    settings.addFeature(DecorationStage::VegetalDecoration, NetherFungusIds::CrimsonFungus);  // 增加密度

    // 岩浆池（较少）
    settings.addFeature(DecorationStage::UndergroundDecoration, MagmaFeatureIds::PatchNormal);

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createWarpedForest() {
    // 诡异森林：下界矿石 + 诡异巨型真菌
    BiomeGenerationSettings settings;

    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherQuartzOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::NetherGoldOre);

    // 诡异巨型真菌
    settings.addFeature(DecorationStage::VegetalDecoration, NetherFungusIds::WarpedFungus);
    settings.addFeature(DecorationStage::VegetalDecoration, NetherFungusIds::WarpedFungus);  // 增加密度

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createBasaltDeltas() {
    // 玄武岩三角洲：下界矿石 + 玄武岩柱 + 玄武岩地面 + 岩浆池
    BiomeGenerationSettings settings;

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
// 末地生物群系生成设置
// ============================================================================

BiomeGenerationSettings BiomeGenerationSettings::createTheEnd() {
    // 末地主岛：黑曜石柱 + 末影龙战斗
    BiomeGenerationSettings settings;

    // 黑曜石柱（主岛核心地表结构）
    settings.addFeature(DecorationStage::SurfaceStructures, EndSurfaceFeatureIds::ObsidianSpike);

    // 末地主岛没有常规矿石生成

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createSmallEndIslands() {
    // 小型末地岛屿：末地岛特征
    BiomeGenerationSettings settings;

    // TODO: 添加末地岛特征

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createEndMidlands() {
    // 末地中部：过渡区域，无特殊特征
    BiomeGenerationSettings settings;

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createEndHighlands() {
    // 末地高地：末地城 + 紫颂树
    BiomeGenerationSettings settings;

    // 末地折跃门（用于连接外岛区域）
    settings.addFeature(DecorationStage::SurfaceStructures, EndSurfaceFeatureIds::EndGateway);

    // TODO: 添加末地城结构
    // TODO: 添加紫颂树特征

    return settings;
}

BiomeGenerationSettings BiomeGenerationSettings::createEndBarrens() {
    // 末地荒地：空旷区域，无特征
    BiomeGenerationSettings settings;

    return settings;
}

// ============================================================================
// BiomeFeaturePlacer 实现
// ============================================================================

void BiomeFeaturePlacer::placeAllFeatures(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    const BiomeGenerationSettings& settings,
    u64 seed)
{
    // 按顺序放置每个阶段的特征
    for (DecorationStage stage : DecorationStages::getAll()) {
        placeFeaturesForStage(region, chunk, generator, settings, stage, seed);
    }
}

void BiomeFeaturePlacer::placeFeaturesForStage(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    const BiomeGenerationSettings& settings,
    DecorationStage stage,
    u64 seed)
{
    const auto& featureIds = settings.getFeatures(stage);
    if (featureIds.empty()) {
        return;
    }

    const i32 chunkX = chunk.x();
    const i32 chunkZ = chunk.z();
    const i32 startX = chunkX * 16;
    const i32 startZ = chunkZ * 16;

    // 参考 MC ChunkGenerator.func_230351_a_ 和 Biome.func_242427_a
    // 使用 setDecorationSeed 算法计算装饰种子
    // setDecorationSeed(baseSeed, x, z):
    //   setSeed(baseSeed)
    //   i = nextLong() | 1
    //   j = nextLong() | 1
    //   k = x * i + z * j ^ baseSeed
    //   setSeed(k)
    //   return k
    math::Random decorRng(seed);
    const u64 i = decorRng.nextLong() | 1ULL;
    const u64 j = decorRng.nextLong() | 1ULL;
    const u64 decorSeed = (static_cast<u64>(startX) * i + static_cast<u64>(startZ) * j) ^ seed;
    decorRng.setSeed(decorSeed);

    // 区块原点位置
    const BlockPos chunkOrigin(startX, 0, startZ);

    // 获取特征注册表中的特征
    FeatureRegistry& registry = FeatureRegistry::instance();
    const auto& allFeatures = registry.getFeatures(stage);

    // 特征索引计数器
    i32 featureIndex = 0;

    // 放置每个特征
    // 参考 MC: 使用 setFeatureSeed(decorSeed, index, stageOrdinal)
    // setFeatureSeed(baseSeed, x, z):
    //   i = baseSeed + x + 10000 * z
    //   setSeed(i)
    const i32 stageOrdinal = static_cast<i32>(stage);

    for (u32 featureId : featureIds) {
        if (featureId < allFeatures.size() && allFeatures[featureId]) {
            ConfiguredFeatureBase* feature = allFeatures[featureId];

            // 使用 setFeatureSeed 算法设置特征种子
            const u64 featureSeed = decorSeed + static_cast<u64>(featureIndex) + static_cast<u64>(10000 * stageOrdinal);
            decorRng.setSeed(featureSeed);

            feature->place(region, chunk, generator, decorRng, chunkOrigin);
            featureIndex++;
        }
    }
}

} // namespace mc
