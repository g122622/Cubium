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

namespace mc {

// 前向声明
class WorldGenRegion;

namespace world::gen::feature {

/**
 * @brief 末地平台特征（end_platform）
 *
 * 在 origin 周围 5x5 水平范围、y∈[-1,2]（相对 origin.y）放置：
 * - y==-1 层：OBSIDIAN（平台底）
 * - y∈[0,2] 层：AIR（清空平台上方）
 * 仅当当前方块与目标方块不同时才 setBlock（避免无意义写入）。
 *
 * 配置为 NoneFeatureConfiguration。装饰阶段为 RawGeneration。
 */
class ConfiguredEndPlatformFeature : public ConfiguredFeatureBase {
public:
    ConfiguredEndPlatformFeature() = default;

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return "end_platform"; }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::RawGeneration; }
};

} // namespace world::gen::feature
} // namespace mc
