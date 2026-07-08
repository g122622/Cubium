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
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include <memory>

namespace mc::world::gen::feature::cave {

/**
 * @brief 随机布尔选择配置
 *
 * 50%概率选择两个特征中的一个。
 */
struct RandomBooleanFeatureConfig {
    /// true时放置的特征（ConfiguredFeatureRegistry 中的 ResourceLocation）
    ResourceLocation featureTrueId;

    /// false时放置的特征（ConfiguredFeatureRegistry 中的 ResourceLocation）
    ResourceLocation featureFalseId;

    RandomBooleanFeatureConfig() = default;
    RandomBooleanFeatureConfig(ResourceLocation trueId, ResourceLocation falseId)
        : featureTrueId(std::move(trueId))
        , featureFalseId(std::move(falseId))
    {}
};

/**
 * @brief 随机布尔选择特征
 *
 * 50%概率选择两个特征中的一个放置。
 * 用于LUSH_CAVES_CLAY（选择干黏土或水黏土池）。
 */
class RandomBooleanSelectorFeature {
public:
    static bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const RandomBooleanFeatureConfig& config);
};

/**
 * @brief 配置化随机布尔选择特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有并在 place() 前走完，
 * 本类只负责在已确定的 pos 处放置选中的子特征。
 */
class ConfiguredRandomBooleanSelectorFeature : public ConfiguredFeatureBase {
public:
    ConfiguredRandomBooleanSelectorFeature(std::unique_ptr<RandomBooleanFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<RandomBooleanFeatureConfig> m_config;
    std::string m_name;
};

} // namespace mc::world::gen::feature::cave
