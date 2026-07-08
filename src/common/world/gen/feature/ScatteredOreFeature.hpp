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
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include <memory>
#include <string>

namespace mc {

// 前向声明
class WorldGenRegion;

/**
 * @brief 散点矿石特征（scattered_ore）
 *
 * 忠实复刻 MC 1.21.11 ScatteredOreFeature：与 OreFeature 共用 OreConfiguration，
 * 但放置策略不同——不做椭圆形矿脉，而是从 origin 起 i=nextInt(size+1) 次散点放置，
 * 每次按 j（min(j, MAX_DIST_FROM_ORIGIN=7)）在三个轴上各偏移
 * round((nextFloat - nextFloat) * j)，命中 canPlaceOre 时放置。
 * 装饰阶段 UndergroundOres。
 */
class ConfiguredScatteredOreFeature : public ConfiguredFeatureBase {
public:
    ConfiguredScatteredOreFeature(
        std::unique_ptr<OreFeatureConfig> featureConfig, const char* featureName = "scattered_ore");

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }

    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundOres; }

private:
    std::unique_ptr<OreFeatureConfig> m_config;
    std::string m_name;
};

} // namespace mc
