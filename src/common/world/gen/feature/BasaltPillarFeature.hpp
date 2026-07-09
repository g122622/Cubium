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
 * @brief 玄武岩柱特征（basalt_pillar）
 *
 * 仅当 origin 为空且其上方非空时生成：从 origin 向下逐格放置 BASALT，
 * 每格在水平四方向尝试概率性"垂挂"（nextInt(10)!=0 放 BASALT，一旦失败
 * 该方向停止垂挂）。到达底部后，在停止柱底周围随机放基座垂挂；再在柱底
 * 下方 7x7 区域内按 |i|*|j| 衰减概率放散落 BASALT（向下找支撑）。
 *
 * 配置为 NoneFeatureConfiguration。装饰阶段为 UndergroundDecoration。
 */
class ConfiguredBasaltPillarFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBasaltPillarFeature() = default;

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return "basalt_pillar"; }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }
};

} // namespace world::gen::feature
} // namespace mc
