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

namespace mc {

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
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }

private:
    std::unique_ptr<BasaltDeltaFeatureConfig> m_config;
    std::string m_name;
    mutable BasaltDeltaFeature m_feature;
};

} // namespace mc
