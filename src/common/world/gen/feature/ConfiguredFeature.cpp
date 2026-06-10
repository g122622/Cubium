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

#include "ConfiguredFeature.hpp"
#include "common/core/Constants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/cave/LushCavesFeatures.hpp"
#include "common/world/gen/feature/gateway/EndGatewayFeature.hpp"
#include "common/world/gen/feature/lake/LakeFeature.hpp"
#include "common/world/gen/feature/nether/NetherFeatures.hpp"
#include "common/world/gen/feature/ocean/BlueIceFeature.hpp"
#include "common/world/gen/feature/ocean/CoralFeature.hpp"
#include "common/world/gen/feature/ocean/KelpFeature.hpp"
#include "common/world/gen/feature/ocean/OceanDecorationFeature.hpp"
#include "common/world/gen/feature/ocean/SeaPickleFeature.hpp"
#include "common/world/gen/feature/ocean/SeagrassFeature.hpp"
#include "common/world/gen/feature/ore/OreFeature.hpp"
#include "common/world/gen/feature/spike/EndSpikeFeature.hpp"
#include "common/world/gen/feature/tree/TreeFeature.hpp"
#include "common/world/gen/feature/vegetation/VegetationFeatures.hpp"
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// FeatureRegistry 实现
// ============================================================================

FeatureRegistry& FeatureRegistry::instance()
{
    static FeatureRegistry s_instance;
    return s_instance;
}

FeatureRegistry::FeatureRegistry()
{
    // 预分配阶段数量
    m_featuresByStage.resize(static_cast<size_t>(DecorationStage::Count));
}

FeatureRegistry::~FeatureRegistry() = default;

void FeatureRegistry::initialize()
{
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

    // 注册繁茂洞穴特征（VegetalDecoration 阶段）
    LushCavesFeatures::initialize();
    auto lushCaveFeatures = LushCavesFeatures::getAllFeaturesAndClear();
    for (auto& feature : lushCaveFeatures) {
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

void FeatureRegistry::registerFeature(std::unique_ptr<ConfiguredFeatureBase> feature, DecorationStage stage)
{
    if (!feature) {
        return;
    }

    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        return;
    }

    // 自动分配递增的特征ID
    const u32 featureId = static_cast<u32>(m_ownedFeatures.size());
    feature->setFeatureId(featureId);

    ConfiguredFeatureBase* ptr = feature.get();
    m_ownedFeatures.push_back(std::move(feature));
    m_featuresByStage[stageIndex].push_back(ptr);
}

const std::vector<ConfiguredFeatureBase*>& FeatureRegistry::getFeatures(DecorationStage stage) const
{
    const size_t stageIndex = static_cast<size_t>(stage);
    if (stageIndex >= m_featuresByStage.size()) {
        static const std::vector<ConfiguredFeatureBase*> empty;
        return empty;
    }
    return m_featuresByStage[stageIndex];
}

void FeatureRegistry::clear()
{
    m_featuresByStage.clear();
    m_featuresByStage.resize(static_cast<size_t>(DecorationStage::Count));
    m_ownedFeatures.clear();
}

} // namespace mc
