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

namespace mc {

/**
 * @brief 仙人掌特征配置
 *
 * 参考 MC BlockStateFeatureConfig
 */
struct CactusFeatureConfig : public IFeatureConfig {
    /// 仙人掌方块状态
    const BlockState* state = nullptr;

    /// 最大高度
    i32 maxHeight = 3;

    CactusFeatureConfig() = default;

    explicit CactusFeatureConfig(const BlockState* cactusState, i32 maxH = 3)
        : state(cactusState)
        , maxHeight(maxH)
    {}
};

/**
 * @brief 仙人掌特征
 *
 * 在沙漠中生成仙人掌。
 * 参考 MC CactusFeature
 */
class CactusFeature {
public:
    /**
     * @brief 放置仙人掌特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 仙人掌配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CactusFeatureConfig& config);

private:
    /**
     * @brief 检查仙人掌是否可以放置在指定位置
     */
    [[nodiscard]] bool canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 检查指定位置是否适合仙人掌生长
     * 仙人掌需要周围没有实体方块
     */
    [[nodiscard]] bool hasValidSpace(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 检查下方方块是否支持仙人掌生长
     */
    [[nodiscard]] bool isValidGround(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化仙人掌特征
 */
class ConfiguredCactusFeature : public ConfiguredFeatureBase {
public:
    ConfiguredCactusFeature(std::unique_ptr<CactusFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const CactusFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<CactusFeatureConfig> m_config;
    std::string m_name;
    CactusFeature m_feature;
};

/**
 * @brief 预定义仙人掌配置
 *
 * 注意：调用 getAllFeaturesAndClear() 后，所有权转移给调用者。
 */
struct CactusFeatures {
    /// 初始化所有仙人掌特征
    static void initialize();

    /// 获取所有仙人掌特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredCactusFeature>>& getAllFeatures();

    /// 获取所有仙人掌特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredCactusFeature>> getAllFeaturesAndClear();

    /// 创建沙漠仙人掌
    static std::unique_ptr<ConfiguredCactusFeature> createDesertCactus();

    /// 创建恶地仙人掌（较高）
    static std::unique_ptr<ConfiguredCactusFeature> createBadlandsCactus();

private:
    static std::vector<std::unique_ptr<ConfiguredCactusFeature>> s_features;
};

} // namespace mc
