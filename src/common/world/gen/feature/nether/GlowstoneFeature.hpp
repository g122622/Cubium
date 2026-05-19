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
#include <vector>

namespace mc {

/**
 * @brief 萤石簇特征配置
 *
 * 参考 MC GlowstoneFeature
 */
struct GlowstoneFeatureConfig : public IFeatureConfig {
    /// 萤石簇的最大延伸距离
    i32 maxDistance = 8;

    /// 分支数量
    i32 branchCount = 4;

    /// 每个分支的最大长度
    i32 maxBranchLength = 6;

    GlowstoneFeatureConfig() = default;

    explicit GlowstoneFeatureConfig(i32 distance, i32 branches, i32 branchLen)
        : maxDistance(distance)
        , branchCount(branches)
        , maxBranchLength(branchLen)
    {}
};

/**
 * @brief 萤石簇特征
 *
 * 在下界天花板生成萤石簇。
 * 参考 MC GlowstoneFeature
 */
class GlowstoneFeature {
public:
    /**
     * @brief 放置萤石簇
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置（天花板上的基岩或下界岩）
     * @param config 配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const GlowstoneFeatureConfig& config);

private:
    /**
     * @brief 从起点向指定方向延伸萤石
     */
    void growBranch(
        WorldGenRegion& world, math::Random& random, const BlockPos& start, i32 dx, i32 dy, i32 dz, i32 length);

    /**
     * @brief 检查位置是否可以放置萤石
     */
    [[nodiscard]] bool canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化萤石簇特征
 */
class ConfiguredGlowstoneFeature : public ConfiguredFeatureBase {
public:
    ConfiguredGlowstoneFeature(std::unique_ptr<GlowstoneFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }
    [[nodiscard]] const GlowstoneFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<GlowstoneFeatureConfig> m_config;
    std::string m_name;
    GlowstoneFeature m_feature;
};

/**
 * @brief 预定义萤石簇特征
 */
struct GlowstoneFeatures {
    static void initialize();
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredGlowstoneFeature>>& getAllFeatures();
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredGlowstoneFeature>> getAllFeaturesAndClear();

    /// 创建普通萤石簇
    static std::unique_ptr<ConfiguredGlowstoneFeature> createNormal();

    /// 创建大型萤石簇
    static std::unique_ptr<ConfiguredGlowstoneFeature> createLarge();

private:
    static std::vector<std::unique_ptr<ConfiguredGlowstoneFeature>> s_features;
};

} // namespace mc
