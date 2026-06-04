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
#include "../../chunk/ChunkPos.hpp"
#include <memory>
#include <vector>

namespace mc {

// 前向声明
class ChunkPrimer;
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
