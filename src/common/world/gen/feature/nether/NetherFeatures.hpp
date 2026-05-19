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
 * @file NetherFeatures.hpp
 * @brief 下界特征模块
 *
 * 包含所有下界特有的地形特征：
 * - 萤石簇 (GlowstoneFeature)
 * - 玄武岩柱 (BasaltColumnFeature)
 * - 玄武岩三角洲 (BasaltDeltaFeature)
 * - 岩浆池 (MagmaPatchFeature)
 * - 下界火焰 (NetherFireFeature)
 * - 巨型真菌 (HugeFungusFeature)
 */

#include "../fungus/HugeFungusFeature.hpp"
#include "BasaltFeature.hpp"
#include "GlowstoneFeature.hpp"
#include "MagmaPatchFeature.hpp"
#include <vector>

namespace mc {

/**
 * @brief 下界特征注册表
 *
 * 管理所有下界特征的初始化和注册。
 */
struct NetherFeatureRegistry {
    /// 初始化所有下界特征
    static void initialize();

    /// 获取所有下界特征（UndergroundDecoration阶段）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredFeatureBase>> getAllUndergroundFeaturesAndClear();

    /// 获取所有下界特征（VegetalDecoration阶段）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredFeatureBase>> getAllVegetationFeaturesAndClear();
};

/**
 * @brief 下界特征ID命名空间
 *
 * 与 FeatureIds.hpp 中的 ID 对应。
 */
namespace NetherFeatureIds {
// UndergroundDecoration 阶段
constexpr u32 GlowstoneNormal = 0;
constexpr u32 GlowstoneLarge = 1;
constexpr u32 BasaltColumnNormal = 2;
constexpr u32 BasaltColumnLarge = 3;
constexpr u32 BasaltDelta = 4;
constexpr u32 MagmaPatchNormal = 5;
constexpr u32 MagmaPatchDense = 6;
constexpr u32 UndergroundCount = 7;

// VegetalDecoration 阶段（巨型真菌等）
constexpr u32 CrimsonFungus = 0;
constexpr u32 WarpedFungus = 1;
constexpr u32 NetherFire = 2;
constexpr u32 VegetationCount = 3;
} // namespace NetherFeatureIds

} // namespace mc
