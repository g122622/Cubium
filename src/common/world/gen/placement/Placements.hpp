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

#include "Placement.hpp"
#include <memory>

namespace mc {

/**
 * @brief 噪声阈值放置配置
 *
 * 根据噪声值决定是否放置特征。
 */
struct NoisePlacementConfig : public IPlacementConfig {
    f64 noiseLevel;  ///< 噪声阈值
    f64 noiseFactor; ///< 噪声因子
    f64 noiseOffset; ///< 噪声偏移

    NoisePlacementConfig(f64 level, f64 factor, f64 offset)
        : noiseLevel(level)
        , noiseFactor(factor)
        , noiseOffset(offset)
    {}
};

/**
 * @brief 噪声控制数量放置配置
 *
 * 根据噪声值决定放置数量。
 */
struct CountNoiseConfig : public IPlacementConfig {
    f64 noiseLevel; ///< 噪声阈值
    i32 belowCount; ///< 低于阈值时的数量
    i32 aboveCount; ///< 高于阈值时的数量

    CountNoiseConfig(f64 level, i32 below, i32 above)
        : noiseLevel(level)
        , belowCount(below)
        , aboveCount(above)
    {}
};

/**
 * @brief 深度平均放置配置
 *
 * 在基准深度附近放置特征。
 */
struct DepthAverageConfig : public IPlacementConfig {
    i32 baseline; ///< 基准深度
    i32 spread;   ///< 扩散范围

    DepthAverageConfig(i32 base, i32 spread)
        : baseline(base)
        , spread(spread)
    {}
};

/**
 * @brief 随机偏移放置配置
 *
 * 参考 MC 1.21.11: RandomOffsetPlacement
 * 使用 IntProvider 采样偏移量，支持动态范围。
 */
struct RandomOffsetConfig : public IPlacementConfig {
    /// XZ平面偏移量提供者
    std::unique_ptr<world::gen::valueprovider::IntProvider> xzSpread;

    /// Y轴偏移量提供者
    std::unique_ptr<world::gen::valueprovider::IntProvider> ySpread;

    RandomOffsetConfig(std::unique_ptr<world::gen::valueprovider::IntProvider> xz,
        std::unique_ptr<world::gen::valueprovider::IntProvider> y)
        : xzSpread(std::move(xz))
        , ySpread(std::move(y))
    {}

    /**
     * @brief 便捷构造：固定偏移
     */
    RandomOffsetConfig(i32 xz, i32 y)
        : xzSpread(std::make_unique<world::gen::valueprovider::ConstantInt>(xz))
        , ySpread(std::make_unique<world::gen::valueprovider::ConstantInt>(y))
    {}

    /**
     * @brief 便捷构造：仅垂直偏移（XZ为0）
     */
    static std::unique_ptr<RandomOffsetConfig> vertical(i32 y) { return std::make_unique<RandomOffsetConfig>(0, y); }

    static std::unique_ptr<RandomOffsetConfig> vertical(
        std::unique_ptr<world::gen::valueprovider::IntProvider> yProvider)
    {
        return std::make_unique<RandomOffsetConfig>(
            std::make_unique<world::gen::valueprovider::ConstantInt>(0), std::move(yProvider));
    }

    /**
     * @brief 便捷构造：仅水平偏移（Y为0）
     */
    static std::unique_ptr<RandomOffsetConfig> horizontal(i32 xz)
    {
        return std::make_unique<RandomOffsetConfig>(xz, 0);
    }
};

/**
 * @brief 水深阈值放置配置
 *
 * 根据水深决定是否放置。
 */
struct WaterDepthThresholdConfig : public IPlacementConfig {
    i32 maxWaterDepth; ///< 最大水深

    explicit WaterDepthThresholdConfig(i32 depth)
        : maxWaterDepth(depth)
    {}
};

/**
 * @brief 海平面放置配置
 *
 * 在海平面附近放置特征。
 */
struct SeaLevelConfig : public IPlacementConfig {
    i32 offset; ///< 相对于海平面的偏移

    explicit SeaLevelConfig(i32 off)
        : offset(off)
    {}
};

// ============================================================================
// 放置器类定义
// ============================================================================

/**
 * @brief 噪声阈值放置器
 *
 * 根据噪声值决定是否放置特征。
 */
class NoisePlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "noise"; }
};

/**
 * @brief 噪声控制数量放置器
 *
 * 根据噪声值决定放置数量。
 */
class CountNoisePlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "count_noise"; }
};

/**
 * @brief 深度平均放置器
 *
 * 在基准深度附近放置特征。
 */
class DepthAveragePlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "depth_average"; }
};

/**
 * @brief 顶层固体放置器
 *
 * 在顶层固体方块上放置特征。
 */
class TopSolidPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "top_solid"; }
};

/**
 * @brief 雕刻掩码放置器
 *
 * 在雕刻掩码指定的位置放置特征。
 */
class CarvingMaskPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "carving_mask"; }
};

/**
 * @brief 随机偏移放置器
 *
 * 对位置进行随机偏移。
 */
class RandomOffsetPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "random_offset"; }
};

/**
 * @brief 水深阈值放置器
 *
 * 根据水深决定是否放置特征。
 */
class WaterDepthThresholdPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "water_depth_threshold"; }
};

/**
 * @brief 海平面放置器
 *
 * 在海平面附近放置特征。
 */
class SeaLevelPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "sea_level"; }
};

/**
 * @brief 扩散放置器
 *
 * 在原始位置周围扩散放置特征。
 */
class SpreadPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "spread"; }
};

} // namespace mc
