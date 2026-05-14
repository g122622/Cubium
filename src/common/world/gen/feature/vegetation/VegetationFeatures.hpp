#pragma once

/**
 * @file VegetationFeatures.hpp
 * @brief 植被特征统一头文件
 *
 * 包含所有植被相关特征：
 * - FlowerFeature: 花卉生成
 * - GrassFeature: 草丛生成
 * - BigMushroomFeature: 巨型蘑菇生成
 * - CactusFeature: 仙人掌生成
 * - SugarCaneFeature: 甘蔗生成
 * - IceSpikeFeature: 冰刺生成
 */

#include "BigMushroomFeature.hpp"
#include "CactusFeature.hpp"
#include "FlowerFeature.hpp"
#include "GrassFeature.hpp"
#include "IceSpikeFeature.hpp"
#include "SugarCaneFeature.hpp"
#include <memory>
#include <vector>

namespace mc {

/**
 * @brief 植被特征管理器
 *
 * 统一管理所有植被特征的初始化和注册。
 * 所有特征通过 getAllFeaturesAndClear() 转移所有权给 FeatureRegistry。
 */
struct VegetationFeatureManager {
    /**
     * @brief 初始化所有植被特征
     *
     * 必须在方块系统初始化后调用。
     * 特征创建后存储在静态变量中，等待 FeatureRegistry 注册。
     */
    static void initialize()
    {
        FlowerFeatures::initialize();
        GrassFeatures::initialize();
        BigMushroomFeatures::initialize();
        CactusFeatures::initialize();
        SugarCaneFeatures::initialize();
        IceSpikeFeatures::initialize();
    }

    /**
     * @brief 获取所有花卉特征并清空静态存储
     */
    static std::vector<std::unique_ptr<ConfiguredFlowerFeature>> getFlowerFeaturesAndClear()
    {
        return FlowerFeatures::getAllFeaturesAndClear();
    }

    /**
     * @brief 获取所有草丛特征并清空静态存储
     */
    static std::vector<std::unique_ptr<ConfiguredGrassFeature>> getGrassFeaturesAndClear()
    {
        return GrassFeatures::getAllFeaturesAndClear();
    }

    /**
     * @brief 获取所有巨型蘑菇特征并清空静态存储
     */
    static std::vector<std::unique_ptr<ConfiguredBigMushroomFeature>> getBigMushroomFeaturesAndClear()
    {
        return BigMushroomFeatures::getAllFeaturesAndClear();
    }

    /**
     * @brief 获取所有仙人掌特征并清空静态存储
     */
    static std::vector<std::unique_ptr<ConfiguredCactusFeature>> getCactusFeaturesAndClear()
    {
        return CactusFeatures::getAllFeaturesAndClear();
    }

    /**
     * @brief 获取所有甘蔗特征并清空静态存储
     */
    static std::vector<std::unique_ptr<ConfiguredSugarCaneFeature>> getSugarCaneFeaturesAndClear()
    {
        return SugarCaneFeatures::getAllFeaturesAndClear();
    }

    /**
     * @brief 获取所有冰刺特征并清空静态存储
     */
    static std::vector<std::unique_ptr<ConfiguredIceSpikeFeature>> getIceSpikeFeaturesAndClear()
    {
        return IceSpikeFeatures::getAllFeaturesAndClear();
    }
};

} // namespace mc
