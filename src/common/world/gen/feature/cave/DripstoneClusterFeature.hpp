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

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include "common/world/gen/valueprovider/FloatProvider.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"

#include <memory>

namespace mc::world::gen::feature::cave {

/**
 * @brief 滴石簇特征配置（MC DripstoneClusterConfiguration）
 */
struct DripstoneClusterConfig : public IFeatureConfig {
    i32 floorToCeilingSearchRange = 0;
    std::unique_ptr<valueprovider::IntProvider> height;
    std::unique_ptr<valueprovider::IntProvider> radius;
    i32 maxStalagmiteStalactiteHeightDiff = 0;
    i32 heightDeviation = 0;
    std::unique_ptr<valueprovider::IntProvider> dripstoneBlockLayerThickness;
    std::unique_ptr<valueprovider::FloatProvider> density;
    std::unique_ptr<valueprovider::FloatProvider> wetness;
    f32 chanceOfDripstoneColumnAtMaxDistanceFromCenter = 0.0F;
    i32 maxDistanceFromEdgeAffectingChanceOfDripstoneColumn = 0;
    i32 maxDistanceFromCenterAffectingHeightBias = 0;

    DripstoneClusterConfig(i32 searchRange,
        std::unique_ptr<valueprovider::IntProvider> h,
        std::unique_ptr<valueprovider::IntProvider> r,
        i32 maxHeightDiff,
        i32 hDeviation,
        std::unique_ptr<valueprovider::IntProvider> blockLayerThickness,
        std::unique_ptr<valueprovider::FloatProvider> dens,
        std::unique_ptr<valueprovider::FloatProvider> wet,
        f32 chanceAtMaxDist,
        i32 maxDistEdge,
        i32 maxDistCenter);
};

/**
 * @brief 滴石簇特征（MC DripstoneClusterFeature）
 *
 * 在 origin 周围一片半径范围内逐列放置滴石：每列先 Column.scan 找空腔，
 * 按概率放水池、铺滴水石块层、生成钟乳石/石笋尖端，遇岩浆跳过。
 */
class DripstoneClusterFeature {
public:
    bool place(IWorld& world, math::Random& random, const BlockPos& pos, const DripstoneClusterConfig& config);

private:
    void placeColumn(IWorld& world,
        math::Random& random,
        const BlockPos& colPos,
        i32 dx,
        i32 dz,
        f32 wetness,
        double chance,
        i32 maxHeight,
        f32 density,
        const DripstoneClusterConfig& config);
};

/**
 * @brief 配置化滴石簇特征
 */
class ConfiguredDripstoneClusterFeature : public ConfiguredFeatureBase {
public:
    ConfiguredDripstoneClusterFeature(std::unique_ptr<DripstoneClusterConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }
    [[nodiscard]] const DripstoneClusterConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<DripstoneClusterConfig> m_config;
    std::string m_name;
    mutable DripstoneClusterFeature m_feature;
};

} // namespace mc::world::gen::feature::cave
