#include "NetherFeatures.hpp"

namespace mc {

bool NetherFeatureRegistry::s_initialized = false;

void NetherFeatureRegistry::initialize() {
    if (s_initialized) return;

    // 初始化各个特征模块
    GlowstoneFeatures::initialize();
    BasaltColumnFeatures::initialize();
    BasaltDeltaFeatures::initialize();
    MagmaPatchFeatures::initialize();
    NetherFireFeatures::initialize();
    HugeFungusFeatures::initialize();

    s_initialized = true;
}

std::vector<std::unique_ptr<ConfiguredFeatureBase>> NetherFeatureRegistry::getAllUndergroundFeaturesAndClear() {
    std::vector<std::unique_ptr<ConfiguredFeatureBase>> features;

    // 添加萤石特征
    auto glowstoneFeatures = GlowstoneFeatures::getAllFeaturesAndClear();
    for (auto& f : glowstoneFeatures) {
        features.push_back(std::move(f));
    }

    // 添加玄武岩柱特征
    auto basaltColumnFeatures = BasaltColumnFeatures::getAllFeaturesAndClear();
    for (auto& f : basaltColumnFeatures) {
        features.push_back(std::move(f));
    }

    // 添加玄武岩三角洲特征
    auto basaltDeltaFeatures = BasaltDeltaFeatures::getAllFeaturesAndClear();
    for (auto& f : basaltDeltaFeatures) {
        features.push_back(std::move(f));
    }

    // 添加岩浆池特征
    auto magmaPatchFeatures = MagmaPatchFeatures::getAllFeaturesAndClear();
    for (auto& f : magmaPatchFeatures) {
        features.push_back(std::move(f));
    }

    return features;
}

std::vector<std::unique_ptr<ConfiguredFeatureBase>> NetherFeatureRegistry::getAllVegetationFeaturesAndClear() {
    std::vector<std::unique_ptr<ConfiguredFeatureBase>> features;

    // 添加巨型真菌特征
    auto fungusFeatures = HugeFungusFeatures::getAllFeaturesAndClear();
    for (auto& f : fungusFeatures) {
        features.push_back(std::move(f));
    }

    // 添加下界火焰特征
    auto fireFeatures = NetherFireFeatures::getAllFeaturesAndClear();
    for (auto& f : fireFeatures) {
        features.push_back(std::move(f));
    }

    return features;
}

} // namespace mc
