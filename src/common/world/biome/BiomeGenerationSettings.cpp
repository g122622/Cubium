#include "BiomeGenerationSettings.hpp"
#include "../gen/feature/ConfiguredFeature.hpp"
#include "../gen/feature/FeatureIds.hpp"
#include "../gen/chunk/IChunkGenerator.hpp"
#include "../chunk/ChunkPrimer.hpp"
#include "../../util/math/random/Random.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {

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
    // 默认设置：包含所有主世界矿石
    BiomeGenerationSettings settings;

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
    // 海洋：只有矿石，没有植被
    BiomeGenerationSettings settings;

    // 添加矿石（不包含绿宝石，因为绿宝石只在山地生成）
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CoalOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::IronOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::GoldOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::RedstoneOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::DiamondOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::LapisOre);
    settings.addFeature(DecorationStage::UndergroundOres, OreFeatureIds::CopperOre);

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
