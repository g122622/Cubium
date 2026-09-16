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

#include "ConfiguredFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include <memory>
#include <string>

namespace mc {

// 前向声明
class WorldGenRegion;

namespace world::gen::feature {

/**
 * @brief 单一方块状态配置
 *
 * 对应 MC 1.21.11 BlockStateConfiguration{state}。用于 forest_rock 等
 * 只需一个目标方块状态的 feature。
 */
struct BlockStateConfig {
    const BlockState* state = nullptr;

    BlockStateConfig() = default;
    explicit BlockStateConfig(const BlockState* s)
        : state(s)
    {}
};

/**
 * @brief 森林岩石特征（forest_rock / BlockBlobFeature）
 *
 * 从 origin 向下寻找第一个"下方为泥土或石头"的格子作为放置点；
 * 若降到 minY+3 以下则放弃。找到后在放置点周围放置 3 个小岩球：
 * 每个岩球由 (i,j,k) ∈ [0,1]^3 决定半轴，半径 f=(i+j+k)/3+0.5，
 * 在 [-i,-j,-k]..[i,j,k] 范围内、距中心 ≤ f 的格子放置配置方块。
 * 每个岩球后中心随机偏移一格。
 *
 * 装饰阶段为 RawGeneration。
 */
class ConfiguredBlockBlobFeature : public ConfiguredFeatureBase {
public:
    explicit ConfiguredBlockBlobFeature(std::unique_ptr<BlockStateConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::RawGeneration; }

private:
    std::unique_ptr<BlockStateConfig> m_config;
    std::string m_name;
};

} // namespace world::gen::feature
} // namespace mc
