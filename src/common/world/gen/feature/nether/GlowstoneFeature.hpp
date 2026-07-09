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
 * @brief 萤石簇特征配置
 *
 * 参考 MC 1.21.11: GlowstoneConfiguration
 * MC 1.21 的 GlowstoneFeature 实际上不使用配置——它使用固定的扩散算法。
 * 此配置保留以兼容 ConfiguredFeatureBase 流水线。
 */
struct GlowstoneFeatureConfig : public IFeatureConfig {
    GlowstoneFeatureConfig() = default;
};

/**
 * @brief 萤石簇特征
 *
 * 在下界天花板生成萤石簇，使用扩散算法。
 * 算法从天花板的基岩/下界岩/玄武岩/黑石处开始放置一个萤石块，
 * 然后迭代 1500 次尝试扩展：每次随机选一个偏移位置，
 * 如果该位置是空气且恰好只有 1 个相邻萤石块，则放置萤石。
 * 这产生下垂的钟乳石状结构。
 *
 * 参考 MC 1.21.11: GlowstoneFeature.place()
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
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }
    [[nodiscard]] const GlowstoneFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<GlowstoneFeatureConfig> m_config;
    std::string m_name;
    mutable GlowstoneFeature m_feature;
};

} // namespace mc
