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

#include "NetherFeatures.hpp"

namespace mc {

void NetherFeatureRegistry::initialize()
{
    // 初始化各个特征模块
    GlowstoneFeatures::initialize();
    BasaltColumnFeatures::initialize();
    BasaltDeltaFeatures::initialize();
    MagmaPatchFeatures::initialize();
    NetherFireFeatures::initialize();
    HugeFungusFeatures::initialize();
}

std::vector<std::unique_ptr<ConfiguredFeatureBase>> NetherFeatureRegistry::getAllUndergroundFeaturesAndClear()
{
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

std::vector<std::unique_ptr<ConfiguredFeatureBase>> NetherFeatureRegistry::getAllVegetationFeaturesAndClear()
{
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
