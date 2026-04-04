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

#include "GlowstoneFeature.hpp"
#include "BasaltFeature.hpp"
#include "MagmaPatchFeature.hpp"
#include "../fungus/HugeFungusFeature.hpp"
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

private:
    static bool s_initialized;
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
}

} // namespace mc
