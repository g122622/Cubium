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

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"

#include <memory>

namespace mc {

class PlacedFeature;

namespace world::gen::feature {

/**
 * @brief random_patch 配置
 *
 * 对应 MC 1.21.11 RandomPatchConfiguration{tries, xzSpread, ySpread, feature: Holder<PlacedFeature>}。
 * tries：放置尝试次数；xzSpread/ySpread：每次尝试相对 origin 的三角形分布偏移半径。
 * feature：内联 PlacedFeature（拥有所有权）。每次尝试在该偏移位置委托它执行
 * （先走其 placement 链，再 place 配置化特征），成功一次即计数。
 *
 * 因内联 configured_feature 不进 ConfiguredFeatureRegistry，故 config 同时持有
 * inlineFeature（拥有所有权）与 feature（持有 inlineFeature 的裸指针 + placement 链）。
 */
struct RandomPatchFeatureConfig {
    /// 放置尝试次数
    i32 tries = 128;
    /// XZ 平面偏移半径
    i32 xzSpread = 7;
    /// Y 轴偏移半径
    i32 ySpread = 3;
    /// 内联配置化特征（拥有所有权，供 PlacedFeature 引用）
    std::unique_ptr<ConfiguredFeatureBase> inlineFeature;
    /// 内联放置特征（拥有 placement 链，引用 inlineFeature）
    std::unique_ptr<PlacedFeature> feature;
};

/**
 * @brief random_patch 特征
 *
 * 忠实复刻 MC RandomPatchFeature.place：tries 次循环，每次以三角形分布
 * 偏移 origin 得到候选位置，委托内联 PlacedFeature 放置；任意一次成功即返回 true。
 *
 * 三角形偏移：offset = nextInt(j) - nextInt(j)，j=xzSpread+1（XZ）/ k=ySpread+1（Y）。
 */
class RandomPatchFeature {
public:
    /**
     * @brief 放置 random_patch
     * @return 任一尝试成功放置即 true
     */
    [[nodiscard]] static bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& origin,
        const RandomPatchFeatureConfig& config);
};

/**
 * @brief random_patch 配置化特征包装
 */
class ConfiguredRandomPatchFeature : public ConfiguredFeatureBase {
public:
    explicit ConfiguredRandomPatchFeature(std::unique_ptr<RandomPatchFeatureConfig> config, const char* featureName)
        : m_config(std::move(config))
        , m_name(featureName)
    {}

    [[nodiscard]] bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& origin) const override;

    [[nodiscard]] DecorationStage stage() const noexcept override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const char* name() const noexcept override { return m_name; }

private:
    std::unique_ptr<RandomPatchFeatureConfig> m_config;
    const char* m_name;
};

} // namespace world::gen::feature
} // namespace mc
