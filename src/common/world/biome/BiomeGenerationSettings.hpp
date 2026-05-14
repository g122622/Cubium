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

#include "../gen/feature/DecorationStage.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace mc {

// 前向声明
class ConfiguredFeatureBase;
class WorldGenRegion;
class ChunkPrimer;
class IChunkGenerator;
class Random;

/**
 * @brief 生物群系生成设置
 *
 * 存储生物群系特有的特征列表，按装饰阶段组织。
 * 参考 MC BiomeGenerationSettings
 */
class BiomeGenerationSettings {
public:
    BiomeGenerationSettings();
    ~BiomeGenerationSettings();

    /**
     * @brief 添加特征到指定阶段
     * @param stage 装饰阶段
     * @param feature 特征ID（用于引用全局特征）
     */
    void addFeature(DecorationStage stage, u32 featureId);

    /**
     * @brief 获取指定阶段的特征ID列表
     * @param stage 装饰阶段
     * @return 特征ID列表
     */
    [[nodiscard]] const std::vector<u32>& getFeatures(DecorationStage stage) const;

    /**
     * @brief 检查是否有任何特征
     */
    [[nodiscard]] bool hasFeatures() const;

    /**
     * @brief 清除所有特征
     */
    void clear();

    /**
     * @brief 创建默认的生物群系生成设置
     * @return 默认设置
     */
    static BiomeGenerationSettings createDefault();

    /**
     * @brief 创建平原生物群系的生成设置
     * @return 平原设置
     */
    static BiomeGenerationSettings createPlains();

    /**
     * @brief 创建森林生物群系的生成设置
     * @return 森林设置
     */
    static BiomeGenerationSettings createForest();

    static BiomeGenerationSettings createTaiga();

    static BiomeGenerationSettings createJungle();

    static BiomeGenerationSettings createSavanna();

    /**
     * @brief 创建沙漠生物群系的生成设置
     * @return 沙漠设置
     */
    static BiomeGenerationSettings createDesert();

    /**
     * @brief 创建沼泽生物群系的生成设置
     * @return 沼泽设置
     */
    static BiomeGenerationSettings createSwamp();

    /**
     * @brief 创建河流生物群系的生成设置
     * @return 河流设置
     */
    static BiomeGenerationSettings createRiver();

    /**
     * @brief 创建冻河生物群系的生成设置
     * @return 冻河设置
     */
    static BiomeGenerationSettings createFrozenRiver();

    /**
     * @brief 创建沼泽山丘生物群系的生成设置
     * @return 沼泽山丘设置
     */
    static BiomeGenerationSettings createSwampHills();

    /**
     * @brief 创建冰刺平原生物群系的生成设置
     * @return 冰刺平原设置
     */
    static BiomeGenerationSettings createIceSpikes();

    /**
     * @brief 创建恶地生物群系的生成设置
     * @return 恶地设置
     */
    static BiomeGenerationSettings createBadlands();

    /**
     * @brief 创建繁花森林生物群系的生成设置
     * @return 繁花森林设置
     */
    static BiomeGenerationSettings createFlowerForest();

    /**
     * @brief 创建山地生物群系的生成设置
     * @return 山地设置
     */
    static BiomeGenerationSettings createMountains();

    /**
     * @brief 创建海洋生物群系的生成设置
     * @return 海洋设置
     */
    static BiomeGenerationSettings createOcean();

    /**
     * @brief 创建深海生物群系的生成设置
     * @return 深海设置
     */
    static BiomeGenerationSettings createDeepOcean();

    /**
     * @brief 创建暖水海洋生物群系的生成设置
     * 参考原版：珊瑚植被 + 暖水海草 + 海泡菜
     * @return 暖水海洋设置
     */
    static BiomeGenerationSettings createWarmOcean();

    /**
     * @brief 创建温水海洋生物群系的生成设置
     * 参考原版：海带 + 常规海草
     * @return 温水海洋设置
     */
    static BiomeGenerationSettings createLukewarmOcean();

    /**
     * @brief 创建冷水海洋生物群系的生成设置
     * 参考原版：海带 + 冷水海草
     * @return 冷水海洋设置
     */
    static BiomeGenerationSettings createColdOcean();

    /**
     * @brief 创建冻洋生物群系的生成设置
     * 参考原版：冷水植被 + 蓝冰局部改造
     * @return 冻洋设置
     */
    static BiomeGenerationSettings createFrozenOcean();

    /**
     * @brief 创建深暖水海洋生物群系的生成设置
     */
    static BiomeGenerationSettings createDeepWarmOcean();

    /**
     * @brief 创建深温水海洋生物群系的生成设置
     */
    static BiomeGenerationSettings createDeepLukewarmOcean();

    /**
     * @brief 创建深冷水海洋生物群系的生成设置
     */
    static BiomeGenerationSettings createDeepColdOcean();

    /**
     * @brief 创建深冻洋生物群系的生成设置
     */
    static BiomeGenerationSettings createDeepFrozenOcean();

    // ========================================================================
    // 下界生物群系生成设置
    // ========================================================================

    /**
     * @brief 创建下界荒地生物群系的生成设置
     */
    static BiomeGenerationSettings createNether();

    /**
     * @brief 创建灵魂沙谷生物群系的生成设置
     */
    static BiomeGenerationSettings createSoulSandValley();

    /**
     * @brief 创建绯红森林生物群系的生成设置
     */
    static BiomeGenerationSettings createCrimsonForest();

    /**
     * @brief 创建诡异森林生物群系的生成设置
     */
    static BiomeGenerationSettings createWarpedForest();

    /**
     * @brief 创建玄武岩三角洲生物群系的生成设置
     */
    static BiomeGenerationSettings createBasaltDeltas();

    // ========================================================================
    // 末地生物群系生成设置
    // ========================================================================

    /**
     * @brief 创建末地主岛生物群系的生成设置
     */
    static BiomeGenerationSettings createTheEnd();

    /**
     * @brief 创建小型末地岛屿生物群系的生成设置
     */
    static BiomeGenerationSettings createSmallEndIslands();

    /**
     * @brief 创建末地中部生物群系的生成设置
     */
    static BiomeGenerationSettings createEndMidlands();

    /**
     * @brief 创建末地高地生物群系的生成设置
     */
    static BiomeGenerationSettings createEndHighlands();

    /**
     * @brief 创建末地荒地生物群系的生成设置
     */
    static BiomeGenerationSettings createEndBarrens();

private:
    // 按阶段存储特征ID列表
    // 使用特征ID而不是直接存储特征对象，以减少内存占用
    std::vector<std::vector<u32>> m_featuresByStage;
};

/**
 * @brief 生物群系特征放置器
 *
 * 在区块中放置生物群系特有的特征。
 */
class BiomeFeaturePlacer {
public:
    /**
     * @brief 在区块中放置所有阶段的特征
     * @param region 世界生成区域
     * @param chunk 区块数据
     * @param generator 区块生成器
     * @param settings 生物群系生成设置
     * @param seed 世界种子
     */
    static void placeAllFeatures(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        const BiomeGenerationSettings& settings,
        u64 seed);

    /**
     * @brief 在区块中放置指定阶段的特征
     * @param region 世界生成区域
     * @param chunk 区块数据
     * @param generator 区块生成器
     * @param settings 生物群系生成设置
     * @param stage 装饰阶段
     * @param seed 世界种子
     */
    static void placeFeaturesForStage(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        const BiomeGenerationSettings& settings,
        DecorationStage stage,
        u64 seed);
};

} // namespace mc
