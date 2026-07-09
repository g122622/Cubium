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

namespace mc {

/**
 * @brief 下界火焰特征配置
 *
 * 参考 MC Java NetherForestVegetationConfig 的 spreadWidth/spreadHeight 设计。
 * - spread：水平蔓延范围，对应 Java 的 spreadWidth
 * - minHeight：火焰生成位置的最低相对 Y 偏移
 * - maxHeight：火焰生成位置的最高相对 Y 偏移
 */
struct NetherFireFeatureConfig : public IFeatureConfig {
    /// 火焰水平蔓延范围
    i32 spread = 4;

    /// 火焰生成位置的最低相对 Y 偏移（相对特征原点向下）
    i32 minHeight = 1;

    /// 火焰生成位置的最高相对 Y 偏移（相对特征原点向上）
    i32 maxHeight = 3;

    NetherFireFeatureConfig() = default;

    explicit NetherFireFeatureConfig(i32 s, i32 minH, i32 maxH)
        : spread(s)
        , minHeight(minH)
        , maxHeight(maxH)
    {}
};

/**
 * @brief 下界火焰特征
 *
 * 在下界生成火焰。
 */
class NetherFireFeature {
public:
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const NetherFireFeatureConfig& config);
};

/**
 * @brief 配置化下界火焰特征
 */
class ConfiguredNetherFireFeature : public ConfiguredFeatureBase {
public:
    ConfiguredNetherFireFeature(std::unique_ptr<NetherFireFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<NetherFireFeatureConfig> m_config;
    std::string m_name;
    mutable NetherFireFeature m_feature;
};

} // namespace mc
