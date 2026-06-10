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
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include <memory>
#include <vector>

namespace mc::world::gen::feature::cave {

/**
 * @brief 随机选择配置
 *
 * 从特征列表中均匀随机选择一个。
 */
struct SimpleRandomFeatureConfig {
    /// 可选特征ID列表
    std::vector<u32> featureIds;

    SimpleRandomFeatureConfig() = default;
    explicit SimpleRandomFeatureConfig(std::vector<u32> ids)
        : featureIds(std::move(ids))
    {}
};

/**
 * @brief 随机选择特征
 *
 * 从特征列表中均匀随机选择一个放置。
 * 用于垂滴叶（选择小型垂滴叶或大型垂滴叶方向）。
 */
class SimpleRandomSelectorFeature {
public:
    static bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const SimpleRandomFeatureConfig& config);
};

/**
 * @brief 配置化随机选择特征
 */
class ConfiguredSimpleRandomSelectorFeature : public ConfiguredFeatureBase {
public:
    ConfiguredSimpleRandomSelectorFeature(std::unique_ptr<SimpleRandomFeatureConfig> config,
        std::unique_ptr<ConfiguredPlacement> placement,
        const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<SimpleRandomFeatureConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_placement;
    std::string m_name;
};

} // namespace mc::world::gen::feature::cave
