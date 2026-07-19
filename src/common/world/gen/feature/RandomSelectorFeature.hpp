/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is
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
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include <memory>
#include <vector>

namespace mc::world::gen::feature {

/**
 * @brief 加权特征条目
 *
 * 对应 MC 1.21.11 WeightedPlacedFeature{feature, chance}。
 * featureId 为 PlacedFeature 引用 id（字符串形式直接指向已注册 placed_feature，
 * 如 "minecraft:spruce_checked"）或内联 PlacedFeature 的 {"feature":..,"placement":[]}
 * 中提取的 configured_feature id（如 "minecraft:oak_bees_005"）。place() 时按 id 先查
 * PlacedFeatureRegistry、未命中再查 ConfiguredFeatureRegistry 委派放置（见
 * RandomSelectorFeature.cpp 的 dispatchChildFeature）。
 */
struct WeightedFeatureEntry {
    ResourceLocation featureId;
    f32 chance = 0.0f; // [0.0, 1.0]
};

/**
 * @brief 随机选择配置
 *
 * 对应 MC 1.21.11 RandomFeatureConfiguration{features[], default}。
 * place() 顺序遍历 features，每项按 chance 概率命中即委派并返回；
 * 全部未命中则走 default。
 */
struct RandomSelectorFeatureConfig {
    std::vector<WeightedFeatureEntry> features;
    ResourceLocation defaultFeatureId;

    RandomSelectorFeatureConfig() = default;
};

/**
 * @brief 随机选择特征
 *
 * 对应 MC 1.21.11 RandomSelectorFeature。顺序概率检查 + default 兜底。
 * 与 simple_random_selector（均匀随机选一个）的根本区别：本类按 chance
 * 顺序检查，命中即返回，列表顺序影响最终概率分布。
 */
class RandomSelectorFeature {
public:
    static bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const RandomSelectorFeatureConfig& config);
};

/**
 * @brief 配置化随机选择特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有并在 place() 前走完，本类只负责在已确定的 pos 处放置。
 */
class ConfiguredRandomSelectorFeature : public ConfiguredFeatureBase {
public:
    ConfiguredRandomSelectorFeature(std::unique_ptr<RandomSelectorFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<RandomSelectorFeatureConfig> m_config;
    std::string m_name;
};

} // namespace mc::world::gen::feature
