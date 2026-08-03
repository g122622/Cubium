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
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/valueprovider/FloatProvider.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"

#include <memory>
#include <optional>
#include <string>

namespace mc::world::gen::feature::cave {

/**
 * @brief 大型滴石特征配置（MC LargeDripstoneConfiguration）
 */
struct LargeDripstoneConfig : public IFeatureConfig {
    i32 floorToCeilingSearchRange = 30;
    std::unique_ptr<valueprovider::IntProvider> columnRadius;
    std::unique_ptr<valueprovider::FloatProvider> heightScale;
    f32 maxColumnRadiusToCaveHeightRatio = 0.0F;
    std::unique_ptr<valueprovider::FloatProvider> stalactiteBluntness;
    std::unique_ptr<valueprovider::FloatProvider> stalagmiteBluntness;
    std::unique_ptr<valueprovider::FloatProvider> windSpeed;
    i32 minRadiusForWind = 0;
    f32 minBluntnessForWind = 0.0F;

    LargeDripstoneConfig(i32 searchRange,
        std::unique_ptr<valueprovider::IntProvider> radius,
        std::unique_ptr<valueprovider::FloatProvider> hScale,
        f32 maxRatio,
        std::unique_ptr<valueprovider::FloatProvider> stalactiteBlunt,
        std::unique_ptr<valueprovider::FloatProvider> stalagmiteBlunt,
        std::unique_ptr<valueprovider::FloatProvider> wind,
        i32 minRadiusWind,
        f32 minBluntWind);
};

/**
 * @brief 大型滴石特征（MC LargeDripstoneFeature）
 *
 * 在洞穴天花板/地板之间生成大型钟乳石/石笋柱。先 Column.scan 找到 ≥4 高的空腔，
 * 再按半径/钝度/缩放构造 LargeDripstone（钟乳石+石笋），WindOffsetter 施加风偏移，
 * moveBackUntilBaseIsInsideStoneAndShrinkRadiusIfNecessary 回退到石头内后铺放方块。
 */
class LargeDripstoneFeature {
public:
    bool place(IWorld& world, math::Random& random, const BlockPos& pos, const LargeDripstoneConfig& config);
};

/**
 * @brief 配置化大型滴石特征
 */
class ConfiguredLargeDripstoneFeature : public ConfiguredFeatureBase {
public:
    ConfiguredLargeDripstoneFeature(std::unique_ptr<LargeDripstoneConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }
    [[nodiscard]] const LargeDripstoneConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<LargeDripstoneConfig> m_config;
    std::string m_name;
    mutable LargeDripstoneFeature m_feature;
};

} // namespace mc::world::gen::feature::cave
