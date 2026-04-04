#include "ConfiguredFeature.hpp"
#include "lake/LakeFeature.hpp"
#include "ore/OreFeature.hpp"
#include "tree/TreeFeature.hpp"
#include "vegetation/VegetationFeatures.hpp"
#include "ocean/KelpFeature.hpp"
#include "ocean/SeagrassFeature.hpp"
#include "ocean/SeaPickleFeature.hpp"
#include "ocean/CoralFeature.hpp"
#include "ocean/OceanDecorationFeature.hpp"
#include "ocean/BlueIceFeature.hpp"
#include "nether/NetherFeatures.hpp"
#include "spike/EndSpikeFeature.hpp"
#include "gateway/EndGatewayFeature.hpp"
#include "../chunk/IChunkGenerator.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../biome/Biome.hpp"
#include "../../block/BlockRegistry.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// FeatureRegistry 实现
// ============================================================================

FeatureRegistry& FeatureRegistry::instance() {
    static FeatureRegistry s_instance;
    return s_instance;
}

FeatureRegistry::FeatureRegistry() {
    // 预分配阶段数量
    m_featuresByStage.resize(static_cast<size_t>(DecorationStage::Count));
}

FeatureRegistry::~FeatureRegistry() = default;

void FeatureRegistry::initialize() {
    clear();

    // 注册湖泊特征（LAKES 阶段）
    LakeFeatures::initialize();
    auto lakeFeatures = LakeFeatures::getAllFeaturesAndClear();
    for (auto& feature : lakeFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::Lakes);
        }
    }

    // 注册矿石特征（UNDERGROUND_ORES 阶段）
    OreFeatures::initialize();
    auto oreFeatures = OreFeatures::getAllFeaturesAndClear();
    for (auto& feature : oreFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::UndergroundOres);
        }
    }

    // 注册树木特征（VEGETAL_DECORATION 阶段）
    TreeFeatures::initialize();
    auto treeFeatures = TreeFeatures::getAllFeaturesAndClear();
    for (auto& feature : treeFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 注册植被特征（VEGETAL_DECORATION 阶段）
    VegetationFeatureManager::initialize();

    // 注册花卉特征
    auto flowerFeatures = VegetationFeatureManager::getFlowerFeaturesAndClear();
    for (auto& feature : flowerFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 注册草丛特征
    auto grassFeatures = VegetationFeatureManager::getGrassFeaturesAndClear();
    for (auto& feature : grassFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 注册巨型蘑菇特征
    auto bigMushroomFeatures = VegetationFeatureManager::getBigMushroomFeaturesAndClear();
    for (auto& feature : bigMushroomFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 注册仙人掌特征
    auto cactusFeatures = VegetationFeatureManager::getCactusFeaturesAndClear();
    for (auto& feature : cactusFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 注册甘蔗特征
    auto sugarCaneFeatures = VegetationFeatureManager::getSugarCaneFeaturesAndClear();
    for (auto& feature : sugarCaneFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 注册冰刺特征
    auto iceSpikeFeatures = VegetationFeatureManager::getIceSpikeFeaturesAndClear();
    for (auto& feature : iceSpikeFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::SurfaceStructures);
        }
    }

    // 注册末地地表结构特征（SURFACE_STRUCTURES 阶段）
    EndSpikeFeatures::initialize();
    auto endSpikeFeatures = EndSpikeFeatures::getAllFeaturesAndClear();
    for (auto& feature : endSpikeFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::SurfaceStructures);
        }
    }

    EndGatewayFeatures::initialize();
    auto endGatewayFeatures = EndGatewayFeatures::getAllFeaturesAndClear();
    for (auto& feature : endGatewayFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::SurfaceStructures);
        }
    }

    // 注册海洋特征（VEGETAL_DECORATION 阶段）
    // 海带
    KelpFeatures::initialize();
    auto kelpFeatures = KelpFeatures::getAllFeaturesAndClear();
    for (auto& feature : kelpFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 海草
    SeagrassFeatures::initialize();
    auto seagrassFeatures = SeagrassFeatures::getAllFeaturesAndClear();
    for (auto& feature : seagrassFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 海泡菜
    SeaPickleFeatures::initialize();
    auto seaPickleFeatures = SeaPickleFeatures::getAllFeaturesAndClear();
    for (auto& feature : seaPickleFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 珊瑚
    CoralFeatures::initialize();
    auto coralFeatures = CoralFeatures::getAllFeaturesAndClear();
    for (auto& feature : coralFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 海洋装饰（潮涌核心/气泡柱/海晶石部件等）
    OceanDecorationFeatures::initialize();
    auto oceanDecorationFeatures = OceanDecorationFeatures::getAllFeaturesAndClear();
    for (auto& feature : oceanDecorationFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 蓝冰
    BlueIceFeatures::initialize();
    auto blueIceFeatures = BlueIceFeatures::getAllFeaturesAndClear();
    for (auto& feature : blueIceFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 注册下界特征（UndergroundDecoration 阶段）
    NetherFeatureRegistry::initialize();
    auto netherUndergroundFeatures = NetherFeatureRegistry::getAllUndergroundFeaturesAndClear();
    for (auto& feature : netherUndergroundFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::UndergroundDecoration);
        }
    }

    // 注册下界植被特征（VegetalDecoration 阶段）
    auto netherVegetationFeatures = NetherFeatureRegistry::getAllVegetationFeaturesAndClear();
    for (auto& feature : netherVegetationFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    spdlog::info("[FeatureRegistry] Initialized features:");
    for (size_t i = 0; i < m_featuresByStage.size(); ++i) {
        if (!m_featuresByStage[i].empty()) {
            spdlog::info("  Stage {}: {} features", i, m_featuresByStage[i].size());
        }
    }
}

void FeatureRegistry::registerFeature(std::unique_ptr<ConfiguredFeatureBase> feature, DecorationStage stage) {
    if (!feature) {
        return;
    }

    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        return;
    }

    ConfiguredFeatureBase* ptr = feature.get();
    m_ownedFeatures.push_back(std::move(feature));
    m_featuresByStage[stageIndex].push_back(ptr);
}

const std::vector<ConfiguredFeatureBase*>& FeatureRegistry::getFeatures(DecorationStage stage) const {
    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        static const std::vector<ConfiguredFeatureBase*> empty;
        return empty;
    }
    return m_featuresByStage[stageIndex];
}

void FeatureRegistry::clear() {
    m_featuresByStage.clear();
    m_featuresByStage.resize(static_cast<size_t>(DecorationStage::Count));
    m_ownedFeatures.clear();
}

// ============================================================================
// FeatureGenerator 实现
// ============================================================================

void FeatureGenerator::placeFeatures(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    const Biome& biome,
    DecorationStage stage,
    u64 seed)
{
    const auto& featureIds = biome.generationSettings().getFeatures(stage);
    if (featureIds.empty()) {
        return;
    }

    const auto& features = FeatureRegistry::instance().getFeatures(stage);

    const i32 chunkX = chunk.x();
    const i32 chunkZ = chunk.z();
    const i32 startX = chunkX * 16;
    const i32 startZ = chunkZ * 16;

    // 参考 MC setDecorationSeed：
    // i = nextLong() | 1
    // j = nextLong() | 1
    // decorSeed = x * i + z * j ^ worldSeed
    math::Random random(seed);
    const u64 i = random.nextLong() | 1ULL;
    const u64 j = random.nextLong() | 1ULL;
    const u64 decorSeed = (static_cast<u64>(startX) * i + static_cast<u64>(startZ) * j) ^ seed;
    random.setSeed(decorSeed);

    // 区块原点位置
    const BlockPos chunkOrigin(startX, 0, startZ);

    // 参考 MC setFeatureSeed：featureSeed = decorSeed + featureIndex + 10000 * stageOrdinal
    const i32 stageOrdinal = static_cast<i32>(stage);
    i32 featureIndex = 0;

    // 放置每个特征
    for (u32 featureId : featureIds) {
        if (featureId < features.size() && features[featureId]) {
            ConfiguredFeatureBase* feature = features[featureId];
            const u64 featureSeed = decorSeed + static_cast<u64>(featureIndex) + static_cast<u64>(10000 * stageOrdinal);
            random.setSeed(featureSeed);
            feature->place(region, chunk, generator, random, chunkOrigin);
            ++featureIndex;
        }
    }
}

} // namespace mc
