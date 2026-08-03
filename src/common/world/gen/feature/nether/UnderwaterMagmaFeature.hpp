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

#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include <optional>
#include <string>

namespace mc {

/**
 * @brief 水下岩浆特征配置
 *
 * 对应 MC 1.21.11: UnderwaterMagmaConfiguration。
 * floorSearchRange: int[0,512]；placementRadiusAroundFloor: int[0,64]；
 * placementProbabilityPerValidPosition: float[0,1]。
 */
struct UnderwaterMagmaConfig : public IFeatureConfig {
    i32 floorSearchRange = 0;
    i32 placementRadiusAroundFloor = 0;
    f32 placementProbabilityPerValidPosition = 0.0f;

    UnderwaterMagmaConfig() = default;
    UnderwaterMagmaConfig(i32 range, i32 radius, f32 probability)
        : floorSearchRange(range)
        , placementRadiusAroundFloor(radius)
        , placementProbabilityPerValidPosition(probability)
    {}
};

/**
 * @brief 水下岩浆特征
 *
 * 对应 MC 1.21.11: UnderwaterMagmaFeature。在水下找到水柱底部（首个非水方块），
 * 在其周围立方体范围内按概率放置岩浆块，仅当位置对外可见时放置。
 */
class UnderwaterMagmaFeature {
public:
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const UnderwaterMagmaConfig& config);

private:
    /// MC: getFloorY —— 从水柱原点向下扫描首个非水方块（原点须为水）
    [[nodiscard]] std::optional<i32> getFloorY(
        WorldGenRegion& world, const BlockPos& pos, const UnderwaterMagmaConfig& config);

    /// MC: isValidPlacement —— 非水非空气，且下方不对外可见，且四水平邻居均不对外可见
    [[nodiscard]] bool isValidPlacement(WorldGenRegion& world, const BlockPos& pos);

    /// MC: isVisibleFromOutside —— 给定面遮挡形状为空或非完整方块
    [[nodiscard]] bool isVisibleFromOutside(WorldGenRegion& world, const BlockPos& pos, Direction dir);
};

/**
 * @brief 配置化水下岩浆特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有，本类在已确定的 pos 处放置。
 */
class ConfiguredUnderwaterMagmaFeature : public ConfiguredFeatureBase {
public:
    ConfiguredUnderwaterMagmaFeature(UnderwaterMagmaConfig config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::UndergroundDecoration; }

private:
    UnderwaterMagmaConfig m_config;
    std::string m_name;
    mutable UnderwaterMagmaFeature m_feature;
};

} // namespace mc
