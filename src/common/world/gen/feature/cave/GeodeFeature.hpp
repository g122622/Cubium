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
#include "common/world/block/BlockTags.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace mc::world::gen::feature::cave {

/**
 * @brief 晶洞方块设置（MC GeodeBlockSettings）
 *
 * 五层 BlockStateProvider（filling/inner_layer/alternate_inner_layer/middle/outer）、
 * inner_placements 候选状态列表、cannot_replace 与 invalid_blocks 标签。
 */
struct GeodeBlockSettings {
    std::unique_ptr<state::BlockStateProvider> fillingProvider;
    std::unique_ptr<state::BlockStateProvider> innerLayerProvider;
    std::unique_ptr<state::BlockStateProvider> alternateInnerLayerProvider;
    std::unique_ptr<state::BlockStateProvider> middleLayerProvider;
    std::unique_ptr<state::BlockStateProvider> outerLayerProvider;
    std::vector<const BlockState*> innerPlacements;
    const BlockTag* cannotReplace = nullptr;
    const BlockTag* invalidBlocks = nullptr;
};

/**
 * @brief 晶洞层厚设置（MC GeodeLayerSettings）
 */
struct GeodeLayerSettings {
    f64 filling = 1.7;
    f64 innerLayer = 2.2;
    f64 middleLayer = 3.2;
    f64 outerLayer = 4.2;
};

/**
 * @brief 晶洞裂缝设置（MC GeodeCrackSettings）
 */
struct GeodeCrackSettings {
    f64 generateCrackChance = 1.0;
    f64 baseCrackSize = 2.0;
    i32 crackPointOffset = 2;
};

/**
 * @brief 晶洞特征配置（MC GeodeConfiguration）
 *
 * 晶洞由分布点集合经 NormalNoise 扰动 + distSqr 逆平方根累加得到层归属：
 * d6>=d4 在外层壁外跳过；d6∈[d3,d4) 外层；[d2,d3) 中层；[d1,d2) 内层（含
 * alternate 概率分支与 potential 放置候选）；>=d1 填充层。裂缝以独立点集 d7
 * 累加，d7>=d5 且 d6<d1 时凿穿为空气并调度相邻流体 tick。
 */
struct GeodeConfig : public IFeatureConfig {
    GeodeBlockSettings blockSettings;
    GeodeLayerSettings layerSettings;
    GeodeCrackSettings crackSettings;
    f64 usePotentialPlacementsChance = 0.35;
    f64 useAlternateLayer0Chance = 0.0;
    bool placementsRequireLayer0Alternate = true;
    std::unique_ptr<valueprovider::IntProvider> outerWallDistance;
    std::unique_ptr<valueprovider::IntProvider> distributionPoints;
    std::unique_ptr<valueprovider::IntProvider> pointOffset;
    i32 minGenOffset = -16;
    i32 maxGenOffset = 16;
    f64 noiseMultiplier = 0.05;
    i32 invalidBlocksThreshold = 1;

    GeodeConfig() = default;
    GeodeConfig(const GeodeConfig&) = delete;
    GeodeConfig& operator=(const GeodeConfig&) = delete;
};

/**
 * @brief 晶洞特征（MC GeodeFeature）
 *
 * 装饰阶段为 UndergroundDecoration。
 */
class GeodeFeature {
public:
    bool place(IWorld& world,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const GeodeConfig& config);

private:
    bool safeSetBlock(IWorld& world, const BlockPos& pos, const BlockState* state, const BlockTag* cannotReplace) const;
};

/**
 * @brief 配置化晶洞特征
 */
class ConfiguredGeodeFeature : public ConfiguredFeatureBase {
public:
    ConfiguredGeodeFeature(std::unique_ptr<GeodeConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }
    [[nodiscard]] const GeodeConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<GeodeConfig> m_config;
    std::string m_name;
    mutable GeodeFeature m_feature;
};

} // namespace mc::world::gen::feature::cave
