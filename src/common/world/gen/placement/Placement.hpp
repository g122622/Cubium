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

#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../block/Block.hpp"
#include "../../chunk/data/Heightmap.hpp"
#include "../valueprovider/HeightProvider.hpp"
#include "../valueprovider/IntProvider.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/gen/surface/VerticalAnchor.hpp"
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace mc {

// 前向声明
namespace world::chunk {
class ChunkPrimer;
}
using world::chunk::ChunkPrimer;
class WorldGenRegion;
class IChunkGenerator;
class ConfiguredFeature;

/**
 * @brief 放置配置基类
 */
struct IPlacementConfig {
    virtual ~IPlacementConfig() = default;
};

/**
 * @brief 空放置配置（用于不需要配置的放置器）
 */
struct EmptyPlacementConfig : public IPlacementConfig {
    EmptyPlacementConfig() = default;
};

/**
 * @brief 数量放置配置
 *
 * 控制每个区块中特征出现的次数。
 */
struct CountPlacementConfig : public IPlacementConfig {
    /// 每区块尝试次数
    i32 count;

    explicit CountPlacementConfig(i32 count)
        : count(count)
    {}
};

/**
 * @brief IntProvider 数量放置配置
 *
 * 参考 MC 1.21.11: CountPlacement
 * 使用 IntProvider 采样数量，支持固定值、均匀分布、正态分布等。
 */
struct CountWithProviderConfig : public IPlacementConfig {
    /// 数量提供者
    std::unique_ptr<world::gen::valueprovider::IntProvider> countProvider;

    explicit CountWithProviderConfig(std::unique_ptr<world::gen::valueprovider::IntProvider> provider)
        : countProvider(std::move(provider))
    {}

    /// 便捷构造：固定数量
    explicit CountWithProviderConfig(i32 fixedCount)
        : countProvider(std::make_unique<world::gen::valueprovider::ConstantInt>(fixedCount))
    {}
};

/**
 * @brief 高度范围放置配置
 *
 * 控制特征的Y坐标范围。
 */
struct HeightRangePlacementConfig : public IPlacementConfig {
    /// 最小Y坐标（底部偏移）
    i32 bottomOffset;

    /// 最大Y坐标偏移（从顶部向下）
    i32 topOffset;

    /// 最大高度
    i32 maximum;

    /**
     * @brief 构造高度范围配置
     * @param bottom 底部偏移（从Y=0开始）
     * @param top 顶部偏移（从最大高度向下）
     * @param max 最大高度限制
     */
    HeightRangePlacementConfig(i32 bottom, i32 top, i32 max)
        : bottomOffset(bottom)
        , topOffset(top)
        , maximum(max)
    {}

    /**
     * @brief 创建均匀分布的高度范围
     * @param minY 最小Y
     * @param maxY 最大Y
     */
    static HeightRangePlacementConfig uniform(i32 minY, i32 maxY) { return HeightRangePlacementConfig(minY, 0, maxY); }

    /**
     * @brief 创建三角形分布（青金石风格）
     * @param baseHeight 基准高度
     * @param spread 扩散范围
     */
    static HeightRangePlacementConfig triangle(i32 baseHeight, i32 spread)
    {
        return HeightRangePlacementConfig(baseHeight - spread, 0, baseHeight + spread);
    }

    /**
     * @brief 获取随机Y坐标
     * @param random 随机数生成器
     * @return Y坐标
     */
    [[nodiscard]] i32 getRandomY(math::Random& random) const noexcept;
};

/**
 * @brief HeightProvider 高度放置配置
 *
 * 参考 MC 1.21.11: HeightRangePlacement
 * 使用 HeightProvider 采样高度，支持固定高度、均匀分布、
 * 偏向底部分布、梯形分布等。
 */
struct HeightProviderPlacementConfig : public IPlacementConfig {
    /// 高度提供者
    std::unique_ptr<world::gen::valueprovider::HeightProvider> heightProvider;

    explicit HeightProviderPlacementConfig(std::unique_ptr<world::gen::valueprovider::HeightProvider> provider)
        : heightProvider(std::move(provider))
    {}

    /// 便捷构造：固定高度
    explicit HeightProviderPlacementConfig(i32 fixedY)
        : heightProvider(std::make_unique<world::gen::valueprovider::ConstantHeight>(
              world::gen::surface::VerticalAnchor::absolute(fixedY)))
    {}
};

/**
 * @brief 生物群系过滤放置配置
 *
 * 只在指定生物群系中生成特征。
 */
struct BiomePlacementConfig : public IPlacementConfig {
    /// 允许的生物群系ID列表
    std::vector<u32> allowedBiomes;

    explicit BiomePlacementConfig(std::vector<u32> biomes)
        : allowedBiomes(std::move(biomes))
    {}

    /**
     * @brief 检查生物群系是否允许
     * @param biomeId 生物群系ID
     * @return 是否允许
     */
    [[nodiscard]] bool isAllowed(u32 biomeId) const noexcept;
};

/**
 * @brief 概率放置配置
 *
 * 以一定概率生成特征。
 */
struct ChancePlacementConfig : public IPlacementConfig {
    /// 成功概率（0.0 - 1.0）
    f32 chance;

    explicit ChancePlacementConfig(f32 c)
        : chance(c)
    {}
};

/**
 * @brief 稀有度过滤配置
 *
 * 以 "1/chance" 的概率通过，即 N 分之一的概率。
 * 比 ChancePlacement 更适合整数概率的场景。
 */
struct RarityFilterConfig : public IPlacementConfig {
    /// 概率分母（例如 chance=4 表示 1/4 概率）
    i32 chance;

    explicit RarityFilterConfig(i32 c)
        : chance(std::max(1, c))
    {}
};

/**
 * @brief 固定坐标放置配置
 *
 * 持有一组固定 BlockPos，仅当 basePos 所在区块包含其中某些坐标时返回它们。
 */
struct FixedPlacementConfig : public IPlacementConfig {
    std::vector<BlockPos> positions;

    explicit FixedPlacementConfig(std::vector<BlockPos> pos)
        : positions(std::move(pos))
    {}
};

/**
 * @brief 逐层数量放置配置
 *
 * count 为 IntProvider（CODEC 范围 [0,256]），每层采样一次。
 */
struct CountOnEveryLayerConfig : public IPlacementConfig {
    std::unique_ptr<world::gen::valueprovider::IntProvider> count;

    explicit CountOnEveryLayerConfig(std::unique_ptr<world::gen::valueprovider::IntProvider> c)
        : count(std::move(c))
    {}
};

/**
 * @brief 噪声阈值数量放置配置
 *
 * BIOME_INFO_NOISE(x/200,z/200) < noiseLevel ? belowNoise : aboveNoise。
 */
struct NoiseThresholdCountConfig : public IPlacementConfig {
    f64 noiseLevel;
    i32 belowNoise;
    i32 aboveNoise;

    NoiseThresholdCountConfig(f64 level, i32 below, i32 above)
        : noiseLevel(level)
        , belowNoise(below)
        , aboveNoise(above)
    {}
};

/**
 * @brief 噪声数量放置配置
 *
 * count = ceil((BIOME_INFO_NOISE(x/factor,z/factor) + offset) * ratio)。
 */
struct NoiseBasedCountConfig : public IPlacementConfig {
    i32 noiseToCountRatio;
    f64 noiseFactor;
    f64 noiseOffset;

    NoiseBasedCountConfig(i32 ratio, f64 factor, f64 offset)
        : noiseToCountRatio(ratio)
        , noiseFactor(factor)
        , noiseOffset(offset)
    {}
};

/**
 * @brief 地表相对阈值过滤配置
 *
 * heightmap(x,z)+minInclusive <= y <= heightmap(x,z)+maxInclusive 才保留。
 * min/max 缺省为 INT_MIN/INT_MAX（不过滤）。
 */
struct SurfaceRelativeThresholdFilterConfig : public IPlacementConfig {
    HeightmapType heightmap;
    i32 minInclusive;
    i32 maxInclusive;

    explicit SurfaceRelativeThresholdFilterConfig(HeightmapType type,
        i32 minInclusive = std::numeric_limits<i32>::min(),
        i32 maxInclusive = std::numeric_limits<i32>::max())
        : heightmap(type)
        , minInclusive(minInclusive)
        , maxInclusive(maxInclusive)
    {}
};

/**
 * @brief 地表放置配置
 *
 * 在地表高度放置特征（用于树木等）。
 */
struct SurfacePlacementConfig : public IPlacementConfig {
    /// 最大水深（树木不能种在太深的水中）
    i32 maxWaterDepth;

    /// 是否需要在阳光下
    bool requireSunlight;

    explicit SurfacePlacementConfig(i32 waterDepth = 0, bool sunlight = false)
        : maxWaterDepth(waterDepth)
        , requireSunlight(sunlight)
    {}
};

/**
 * @brief 高度图放置配置
 *
 * 参考 MC 1.21.11: HeightmapPlacement
 * 持有 JSON "heightmap" 字段指定的高度图类型，getPositions 用它查 (x,z) 列最高方块 Y。
 */
struct HeightmapPlacementConfig : public IPlacementConfig {
    /// 高度图类型（WORLD_SURFACE_WG / OCEAN_FLOOR / MOTION_BLOCKING 等）
    HeightmapType heightmap;

    explicit HeightmapPlacementConfig(HeightmapType type)
        : heightmap(type)
    {}
};

/**
 * @brief 放置器基类
 *
 * 控制特征在世界中的放置位置。
 */
class Placement {
public:
    virtual ~Placement() = default;

    /**
     * @brief 获取放置位置
     * @param region 世界生成区域
     * @param random 随机数生成器
     * @param config 放置配置
     * @param basePos 基础位置（通常是区块坐标）
     * @return 放置位置列表
     */
    [[nodiscard]] virtual std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const = 0;

    /**
     * @brief 获取放置器名称
     */
    [[nodiscard]] virtual const char* name() const noexcept = 0;
};

/**
 * @brief 恒等放置器
 *
 * 直接返回基础位置 {basePos}，对应 MC 空 placement 修饰符链的语义
 * （PlacedFeature 的 placement 列表为空时，特征在 origin 处放置）。
 * 用于内联 PlacedFeature（如 simple_random_selector 的 features[]）placement 数组为空的情形。
 */
class IdentityPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "identity"; }
};

/**
 * @brief 数量放置器
 *
 * 在每个区块中放置指定数量的特征。
 */
class CountPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "count"; }
};

/**
 * @brief 高度范围放置器
 *
 * 在指定的Y坐标范围内放置特征。
 */
class HeightRangePlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "height_range"; }
};

/**
 * @brief 方形分散放置器
 *
 * 将位置在XZ平面内随机分散。
 */
class SquarePlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "square"; }
};

/**
 * @brief 生物群系过滤放置器
 *
 * 只在特定生物群系中放置特征。
 */
class BiomePlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "biome"; }
};

/**
 * @brief 概率放置器
 *
 * 以一定概率放置特征。
 */
class ChancePlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "chance"; }
};

/**
 * @brief 地表放置器
 *
 * 在地表高度放置特征（用于树木等）。
 * 从顶部向下搜索第一个非空气方块。
 */
class SurfacePlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "surface"; }
};

/**
 * @brief 高度图放置器
 *
 * 参考 MC 1.21.11: HeightmapPlacement
 * 基于高度图类型（MOTION_BLOCKING, OCEAN_FLOOR 等）查找 Y 坐标。
 */
class HeightmapPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "heightmap"; }
};

/**
 * @brief 稀有度过滤放置器
 *
 * 以 1/chance 的概率通过，使用整数概率。
 */
class RarityFilterPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "rarity_filter"; }
};

/**
 * @brief 固定坐标放置器
 *
 * 仅当 basePos 所在区块包含配置中的某些坐标时，返回那些坐标。
 */
class FixedPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "fixed_placement"; }
};

/**
 * @brief 逐层数量放置器
 *
 * 从上到下逐层寻找"非空方块上方"的地面层，每层按 count 采样多次。
 */
class CountOnEveryLayerPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "count_on_every_layer"; }
};

/**
 * @brief 噪声阈值数量放置器
 *
 * BIOME_INFO_NOISE(x/200,z/200) < noiseLevel ? belowNoise : aboveNoise。
 */
class NoiseThresholdCountPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "noise_threshold_count"; }
};

/**
 * @brief 噪声数量放置器
 *
 * count = ceil((BIOME_INFO_NOISE(x/factor,z/factor) + offset) * ratio)。
 */
class NoiseBasedCountPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "noise_based_count"; }
};

/**
 * @brief 地表相对阈值过滤放置器
 *
 * heightmap(x,z)+minInclusive <= y <= heightmap(x,z)+maxInclusive 才保留。
 */
class SurfaceRelativeThresholdFilterPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "surface_relative_threshold_filter"; }
};

/**
 * @brief 配置化的放置器
 *
 * 组合放置器和配置。
 */
class ConfiguredPlacement {
public:
    ConfiguredPlacement(std::unique_ptr<Placement> placement, std::unique_ptr<IPlacementConfig> config);

    /**
     * @brief 获取放置位置
     */
    [[nodiscard]] std::vector<BlockPos> getPositions(
        WorldGenRegion& region, math::Random& random, const BlockPos& basePos) const;

    /**
     * @brief 链式添加放置器
     * @param placement 放置器
     * @param config 配置
     * @return 新的配置化放置器
     */
    [[nodiscard]] std::unique_ptr<ConfiguredPlacement> then(
        std::unique_ptr<Placement> placement, std::unique_ptr<IPlacementConfig> config) const;

    /**
     * @brief 设置下一个放置器
     * @param next 下一个放置器
     */
    void setNext(std::unique_ptr<ConfiguredPlacement> next) { m_next = std::move(next); }

    [[nodiscard]] ConfiguredPlacement* next() noexcept { return m_next.get(); }
    [[nodiscard]] const ConfiguredPlacement* next() const noexcept { return m_next.get(); }

private:
    std::unique_ptr<Placement> m_placement;
    std::unique_ptr<IPlacementConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_next;
};

} // namespace mc
