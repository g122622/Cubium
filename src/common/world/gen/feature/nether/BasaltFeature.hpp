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

#include "common/core/Types.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include <memory>
#include <vector>

namespace mc {

/**
 * @brief 玄武岩柱特征配置
 */
struct BasaltColumnFeatureConfig : public IFeatureConfig {
    /// 最小柱高
    i32 minHeight = 0;

    /// 最大柱高
    i32 maxHeight = 5;

    /// 是否尝试达到天花板
    bool reachCeiling = false;

    BasaltColumnFeatureConfig() = default;

    explicit BasaltColumnFeatureConfig(i32 minH, i32 maxH, bool reach = false)
        : minHeight(minH)
        , maxHeight(maxH)
        , reachCeiling(reach)
    {}
};

/**
 * @brief 玄武岩柱特征
 *
 * 生成从地板向上延伸的玄武岩柱。
 */
class BasaltColumnFeature {
public:
    /**
     * @brief 放置玄武岩柱
     */
    bool place(
        WorldGenRegion& world, math::Random& random, const BlockPos& pos, const BasaltColumnFeatureConfig& config);

private:
    /**
     * @brief 检查位置是否可以放置玄武岩
     */
    [[nodiscard]] bool _canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 获取柱高度
     */
    [[nodiscard]] i32 _getColumnHeight(WorldGenRegion& world, const BlockPos& pos, i32 minH, i32 maxH) const;
};

/**
 * @brief 配置化玄武岩柱特征
 */
class ConfiguredBasaltColumnFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBasaltColumnFeature(std::unique_ptr<BasaltColumnFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }
    [[nodiscard]] const BasaltColumnFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<BasaltColumnFeatureConfig> m_config;
    std::string m_name;
    BasaltColumnFeature m_feature;
};

/**
 * @brief 预定义玄武岩柱特征
 */
struct BasaltColumnFeatures {
    static void initialize();
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredBasaltColumnFeature>>& getAllFeatures();
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredBasaltColumnFeature>> getAllFeaturesAndClear();

    /// 创建普通玄武岩柱
    static std::unique_ptr<ConfiguredBasaltColumnFeature> createNormal();

    /// 创建大型玄武岩柱
    static std::unique_ptr<ConfiguredBasaltColumnFeature> createLarge();

private:
    static std::vector<std::unique_ptr<ConfiguredBasaltColumnFeature>> s_features;
};

/**
 * @brief 玄武岩簇特征配置
 */
struct BasaltDeltaFeatureConfig : public IFeatureConfig {
    /// 簇的大小
    i32 size = 8;

    /// 岩浆块替换下界岩的概率 (0.0 - 1.0)
    f32 magmaChance = 0.2f;

    /// 是否使用玄武岩
    bool useBasalt = true;

    BasaltDeltaFeatureConfig() = default;

    explicit BasaltDeltaFeatureConfig(i32 s, f32 magma, bool basalt = true)
        : size(s)
        , magmaChance(magma)
        , useBasalt(basalt)
    {}
};

/**
 * @brief 玄武岩三角洲特征
 *
 * 生成玄武岩三角洲特有的地貌：玄武岩地面和岩浆块池。
 */
class BasaltDeltaFeature {
public:
    /**
     * @brief 放置玄武岩三角洲特征
     */
    bool place(
        WorldGenRegion& world, math::Random& random, const BlockPos& pos, const BasaltDeltaFeatureConfig& config);
};

/**
 * @brief 配置化玄武岩三角洲特征
 */
class ConfiguredBasaltDeltaFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBasaltDeltaFeature(std::unique_ptr<BasaltDeltaFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }

private:
    std::unique_ptr<BasaltDeltaFeatureConfig> m_config;
    std::string m_name;
    BasaltDeltaFeature m_feature;
};

/**
 * @brief 预定义玄武岩三角洲特征
 */
struct BasaltDeltaFeatures {
    static void initialize();
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>>& getAllFeatures();
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>> getAllFeaturesAndClear();

    static std::unique_ptr<ConfiguredBasaltDeltaFeature> createNormal();

private:
    static std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>> s_features;
};

} // namespace mc
