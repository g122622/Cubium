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
#include <vector>

namespace mc {
class PlacedFeature;
}

namespace mc::world::gen::feature::cave {

/**
 * @brief 随机选择配置
 *
 * 对齐 MC 1.21.11 SimpleRandomFeatureConfiguration{features: HolderSet<PlacedFeature>}：
 * 从 PlacedFeature 列表中均匀随机选择一个放置（先走其 placement 链，再 place 配置化特征）。
 *
 * features[] 每项可为一：
 * - 字符串 id / {feature:"id", placement:[...]}：引用 ConfiguredFeatureRegistry 中已注册的特征，
 *   placement 链由该项持有；
 * - {feature:{type,config}, placement:[...]}：内联 configured_feature，所有权由 inlineFeatures 托管，
 *   PlacedFeature 持其裸指针 + placement 链。
 */
struct SimpleRandomFeatureConfig {
    /// 可选放置特征列表（每项含 CF 指针 + placement 链）
    std::vector<std::unique_ptr<PlacedFeature>> features;
    /// 内联 configured_feature 所有权容器（生命周期需长于 features 中的 PlacedFeature）
    std::vector<std::unique_ptr<ConfiguredFeatureBase>> inlineFeatures;

    SimpleRandomFeatureConfig() = default;
};

/**
 * @brief 随机选择特征
 *
 * 从 PlacedFeature 列表中均匀随机选择一个放置（委托其 place(origin)，先走 placement 链）。
 * 用于垂滴叶（选择小型垂滴叶或大型垂滴叶方向）、温水海洋植被等。
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
 *
 * 数据驱动下 placement 链由父 PlacedFeature 走完，本类在已确定的 pos 处随机选一个内联 PlacedFeature 委托放置。
 */
class ConfiguredSimpleRandomSelectorFeature : public ConfiguredFeatureBase {
public:
    ConfiguredSimpleRandomSelectorFeature(std::unique_ptr<SimpleRandomFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<SimpleRandomFeatureConfig> m_config;
    std::string m_name;
};

} // namespace mc::world::gen::feature::cave
