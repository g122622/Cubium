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
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"

namespace mc {

// 前向声明
class WorldGenRegion;

namespace world::gen::feature {

/**
 * @brief 虚空起始平台特征（void_start_platform）
 *
 * 在 (8,3,8) 为中心的 33x33 区域（切比雪夫距离≤16）放置石制平台：
 * 中心格 (8,3,8) 为 COBBLESTONE，其余为 STONE。仅处理当前区块与平台
 * 中心所在区块切比雪夫距离≤1 的区块（否则直接返回 true）。
 *
 * 配置为 NoneFeatureConfiguration。装饰阶段为 RawGeneration。
 */
class ConfiguredVoidStartPlatformFeature : public ConfiguredFeatureBase {
public:
    ConfiguredVoidStartPlatformFeature() = default;

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return "void_start_platform"; }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::RawGeneration; }
};

} // namespace world::gen::feature
} // namespace mc
