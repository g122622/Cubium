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
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/MultifaceBlock.hpp"

#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace cave {

/**
 * @brief 多面方块生长配置（MC MultifaceGrowthConfiguration）
 *
 * placeBlock 必须是 MultifaceBlock（glow_lichen / sculk_vein）。canBePlacedOn 为
 * 方块标签与/或方块列表。validDirections 由 can_place_on_floor/ceiling/wall 派生。
 */
struct MultifaceGrowthConfig : public IFeatureConfig {
    /// 要放置的多面方块（MC 配置的 block 字段，直接是 Block 而非 BlockStateProvider）。
    const blocks::MultifaceBlock* placeBlock = nullptr;

    /// 沿单方向搜索可放置点的步数上限（MC searchRange，默认 10）。
    i32 searchRange = 10;

    /// 放置成功后触发一次扩散的概率（MC chanceOfSpreading，默认 0.5）。
    f32 chanceOfSpreading = 0.5f;

    /// 可附着的方块标签（"#xxx"），与 canBePlacedOnBlocks 任一命中即可。可为 nullptr。
    const BlockTag* canBePlacedOnTag = nullptr;

    /// 可附着的方块列表（MC canBePlacedOn 数组项）。
    std::vector<const Block*> canBePlacedOnBlocks;

    /// 由 can_place_on_floor/ceiling/wall 派生的有效朝向集合。
    std::vector<Direction> validDirections;
};

/**
 * @brief 多面方块生长特征（MC MultifaceGrowthFeature）
 *
 * 算法：origin 必须为空气/水 → 取打乱的有效朝向 → placeGrowthIfPossible 直接尝试 →
 * 失败则沿各方向逐步搜索 searchRange 步（撞到非空气/非本方块即停）→ 每个候选点
 * 再 placeGrowthIfPossible。放置成功后以 chanceOfSpreading 概率做一次单方向扩散。
 * 装饰阶段为 UndergroundDecoration（与原版 glow_lichen/sculk_vein 一致）。
 */
class MultifaceGrowthFeature {
public:
    bool place(IWorld& world,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const MultifaceGrowthConfig& config);
};

/**
 * @brief 配置化多面方块生长特征
 */
class ConfiguredMultifaceGrowthFeature : public ConfiguredFeatureBase {
public:
    ConfiguredMultifaceGrowthFeature(std::unique_ptr<MultifaceGrowthConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }

private:
    std::unique_ptr<MultifaceGrowthConfig> m_config;
    std::string m_name;
    mutable MultifaceGrowthFeature m_feature;
};

} // namespace cave
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
