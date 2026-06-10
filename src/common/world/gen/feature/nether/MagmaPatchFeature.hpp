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

// 岩浆池特征聚合头文件（包含岩浆池 + 下界火焰）
#include "NetherFireFeature.hpp"

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include <memory>
#include <vector>

namespace mc {

/**
 * @brief 岩浆池特征配置
 */
struct MagmaPatchFeatureConfig : public IFeatureConfig {
    /// 池的半径
    i32 radius = 4;

    /// 岩浆块出现的概率
    f32 magmaChance = 0.3f;

    /// 火焰出现的概率
    f32 fireChance = 0.1f;

    /// 最小深度
    i32 minDepth = 1;

    /// 最大深度
    i32 maxDepth = 3;

    MagmaPatchFeatureConfig() = default;

    explicit MagmaPatchFeatureConfig(i32 r, f32 magma, f32 fire, i32 minD, i32 maxD)
        : radius(r)
        , magmaChance(magma)
        , fireChance(fire)
        , minDepth(minD)
        , maxDepth(maxD)
    {}
};

/**
 * @brief 岩浆池特征
 *
 * 生成岩浆块和火焰的池子
 */
class MagmaPatchFeature {
public:
    /**
     * @brief 放置岩浆池
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const MagmaPatchFeatureConfig& config);

private:
    /**
     * @brief 检查位置是否有效
     */
    [[nodiscard]] bool _isValidLocation(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化岩浆池特征
 */
class ConfiguredMagmaPatchFeature : public ConfiguredFeatureBase {
public:
    ConfiguredMagmaPatchFeature(std::unique_ptr<MagmaPatchFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }

private:
    std::unique_ptr<MagmaPatchFeatureConfig> m_config;
    std::string m_name;
    MagmaPatchFeature m_feature;
};

/**
 * @brief 预定义岩浆池特征
 */
struct MagmaPatchFeatures {
    static void initialize();
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredMagmaPatchFeature>>& getAllFeatures();
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredMagmaPatchFeature>> getAllFeaturesAndClear();

    static std::unique_ptr<ConfiguredMagmaPatchFeature> createNormal();
    static std::unique_ptr<ConfiguredMagmaPatchFeature> createDense();

private:
    static std::vector<std::unique_ptr<ConfiguredMagmaPatchFeature>> s_features;
};

} // namespace mc
