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

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include <memory>
#include <string>
#include <vector>

// Forward declarations
namespace mc {
class Block;
class BlockState;
class IWorldWriter;
} // namespace mc

namespace mc::world::gen::feature::lake {

/**
 * @brief 湖泊特征配置
 *
 * 配置湖泊或熔岩湖的参数。
 */
struct LakeFeatureConfig {
    Block* fluidBlock;             ///< 流体方块（用于比较）
    Block* borderBlock;            ///< 边界方块（用于比较）
    const BlockState* fluidState;  ///< 流体方块状态
    const BlockState* borderState; ///< 边界方块状态

    LakeFeatureConfig(Block* fluid = nullptr, Block* border = nullptr)
        : fluidBlock(fluid)
        , borderBlock(border)
        , fluidState(fluid ? &fluid->defaultState() : nullptr)
        , borderState(border ? &border->defaultState() : nullptr)
    {}
};

/**
 * @brief 湖泊特征
 *
 * 生成湖泊或熔岩湖。
 * 参考 MC 1.16.5: net.minecraft.world.gen.feature.LakesFeature
 */
class LakeFeature {
public:
    /**
     * @brief 构造函数
     * @param config 配置
     */
    explicit LakeFeature(const LakeFeatureConfig& config);

    /**
     * @brief 生成湖泊
     * @param world 世界写入器
     * @param rng 随机数生成器
     * @param x 中心 X 坐标
     * @param y 中心 Y 坐标
     * @param z 中心 Z 坐标
     * @return 是否成功生成
     */
    bool place(IWorldWriter& world, math::Random& rng, i32 x, i32 y, i32 z);

    /**
     * @brief 创建水湖配置
     */
    static LakeFeatureConfig createWaterLake();

    /**
     * @brief 创建熔岩湖配置
     */
    static LakeFeatureConfig createLavaLake();

private:
    /**
     * @brief 检查位置是否适合生成湖泊
     */
    [[nodiscard]] bool canPlaceAt(IWorldWriter& world, i32 x, i32 y, i32 z) const;

    LakeFeatureConfig m_config;
};

/**
 * @brief 创建水湖特征
 */
std::unique_ptr<LakeFeature> createWaterLakeFeature();

/**
 * @brief 创建熔岩湖特征
 */
std::unique_ptr<LakeFeature> createLavaLakeFeature();

} // namespace mc::world::gen::feature::lake

namespace mc {

/**
 * @brief 配置化湖泊特征
 *
 * 将 LakeFeature 适配到 ConfiguredFeatureBase 流水线，
 * 用于在 Lakes 阶段统一注册和触发。
 */
class ConfiguredLakeFeature : public ConfiguredFeatureBase {
public:
    /**
     * @brief 构造配置化湖泊特征
     * @param config 湖泊配置
     * @param featureName 特征名称
     * @param chance 触发概率分母（1/chance）
     * @param minY 采样最小高度（含）
     * @param maxY 采样最大高度（含）
     */
    ConfiguredLakeFeature(
        world::gen::feature::lake::LakeFeatureConfig config, const char* featureName, i32 chance, i32 minY, i32 maxY);

    /**
     * @brief 在区块中尝试放置湖泊
     *
     * @note 本方法会自行做概率门控和高度采样。
     */
    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::Lakes; }

private:
    world::gen::feature::lake::LakeFeature m_feature;
    std::string m_name;
    i32 m_chance;
    i32 m_minY;
    i32 m_maxY;
    bool m_isLava;
};

/**
 * @brief 湖泊特征集合
 *
 * 负责创建并缓存 Lakes 阶段的配置化特征。
 */
struct LakeFeatures {
    /// 初始化所有湖泊特征
    static void initialize();

    /// 获取所有湖泊特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredLakeFeature>>& getAllFeatures();

    /// 获取所有湖泊特征并转移所有权
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredLakeFeature>> getAllFeaturesAndClear();

    /// 创建水湖特征
    static std::unique_ptr<ConfiguredLakeFeature> createWaterLake();

    /// 创建熔岩湖特征
    static std::unique_ptr<ConfiguredLakeFeature> createLavaLake();

private:
    static std::vector<std::unique_ptr<ConfiguredLakeFeature>> s_features;
};

} // namespace mc
