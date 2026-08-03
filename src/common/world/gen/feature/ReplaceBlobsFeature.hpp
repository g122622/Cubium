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

#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <string>

namespace mc {

// 前向声明
class BlockState;
class WorldGenRegion;

namespace world::gen::feature {

/**
 * @brief 替换球配置（netherrack_replace_blobs）
 *
 * 对应 MC 1.21.11 ReplaceSphereConfiguration{targetState, replaceState, radius}。
 * target 为待替换的目标方块状态；state 为替换后的方块状态；radius 为 IntProvider(0..12)。
 */
struct ReplaceSphereConfig {
    /// 目标方块状态（仅替换与此完全相同的方块，按方块身份比较）。
    const BlockState* targetState = nullptr;
    /// 替换后的方块状态。
    const BlockState* replaceState = nullptr;
    /// 球半径 IntProvider。
    std::unique_ptr<valueprovider::IntProvider> radius;

    ReplaceSphereConfig() = default;
};

/**
 * @brief 替换球特征（netherrack_replace_blobs）
 *
 * 忠实复刻 MC 1.21.11 ReplaceBlobsFeature：
 * - 从 origin（Y 钳制到 [minY+1, maxY]）向下查找第一个 target 方块作为球心；
 * - i/j/k = radius.sample() 各采样一次，l = max(i,j,k)；
 * - 遍历 withinManhattan(球心, i, j, k)，distManhattan > l 则 break；
 * - 球心范围内 target 方块替换为 replaceState。
 * 装饰阶段 UndergroundDecoration（MC 注册于 NETHER_BIOMES 的 UNDERGROUND_DECORATION 步）。
 */
class ConfiguredReplaceBlobsFeature : public ConfiguredFeatureBase {
public:
    ConfiguredReplaceBlobsFeature(
        std::unique_ptr<ReplaceSphereConfig> config, const char* featureName = "netherrack_replace_blobs");

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }

    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }

private:
    std::unique_ptr<ReplaceSphereConfig> m_config;
    std::string m_name;
};

} // namespace world::gen::feature
} // namespace mc
