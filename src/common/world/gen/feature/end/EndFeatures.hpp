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

#pragma once

/**
 * @file EndFeatures.hpp
 * @brief 末地特征模块
 *
 * 包含所有末地特有的地形特征：
 * - 黑曜石柱 (EndSpikeFeature)
 * - 末地折跃门 (EndGatewayFeature)
 * - 冰刺 (IceSpikeFeature)
 * - 紫颂树 (ChorusPlantFeature)
 * - 末地小岛 (EndIslandFeature)
 */

#include <memory>
#include <vector>

#include "ChorusPlantFeature.hpp"
#include "EndGatewayFeature.hpp"
#include "EndIslandFeature.hpp"
#include "EndSpikeFeature.hpp"
#include "IceSpikeFeature.hpp"

namespace mc {

/**
 * @brief 末地特征注册表
 *
 * 管理所有末地特征的初始化和注册。
 */
struct EndFeatureRegistry {
    /// 初始化所有末地特征
    static void initialize();

    /// 获取所有末地特征（SurfaceStructures阶段）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredFeatureBase>> getAllSurfaceFeaturesAndClear();

    /// 获取所有末地特征（VegetalDecoration阶段）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredFeatureBase>> getAllVegetationFeaturesAndClear();

    /// 获取所有末地特征（RawGeneration阶段）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredFeatureBase>> getAllRawGenerationFeaturesAndClear();
};

} // namespace mc
