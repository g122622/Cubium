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

#include "CarverConfiguration.hpp"
#include "CarvingContext.hpp"
#include "CarvingMask.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <bitset>
#include <functional>
#include <memory>
#include <vector>

namespace mc {

// 前向声明
namespace world::biome {
class IBiomeSource;
}

/**
 * @brief 雕刻椭球位置参数
 *
 * 用于 CarveSkipChecker 回调，提供类型安全的参数传递。
 */
struct CarverEllipsePos {
    f32 dx;
    f32 dy;
    f32 dz;
    i32 y;
};

/**
 * @brief 椭球跳过检查回调
 *
 * 用于不同类型雕刻器的椭球内位置过滤逻辑。
 * 洞穴使用基于 floorLevel 的 dy 检查，峡谷使用高度相关的宽度因子。
 */
using CarveSkipChecker = std::function<bool(const CarverEllipsePos&)>;

/**
 * @brief 世界雕刻器基类
 *
 * @tparam Config 配置类型（CarverConfiguration 或其子类）
 */
template <typename Config>
class WorldCarver {
public:
    explicit WorldCarver(i32 maxHeight = world::MAX_BUILD_HEIGHT)
        : m_maxHeight(maxHeight)
    {}

    virtual ~WorldCarver() = default;

    /**
     * @brief 在区块中执行雕刻
     *
     * @param chunk 目标区块（雕刻写入的目标）
     * @param context 雕刻上下文
     * @param biomeSource 生物群系源
     * @param targetChunkX 目标区块 X 坐标（雕刻写入的区块）
     * @param targetChunkZ 目标区块 Z 坐标
     * @param originChunkX 起始区块 X 坐标（雕刻起源的区块）
     * @param originChunkZ 起始区块 Z 坐标
     * @param carvingMask 雕刻掩码
     * @param rng 已初始化的随机数生成器
     * @param config 配置
     * @return 是否雕刻了任何方块
     */
    virtual bool carve(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::IBiomeSource& biomeSource,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        ChunkCoord originChunkX,
        ChunkCoord originChunkZ,
        CarvingMask& carvingMask,
        math::IRandom& rng,
        const Config& config) = 0;

    /**
     * @brief 检查是否应该在这个起始区块执行雕刻
     */
    [[nodiscard]] virtual bool shouldCarve(
        math::IRandom& rng, ChunkCoord chunkX, ChunkCoord chunkZ, const Config& config) const = 0;

    /** @brief 获取雕刻器的影响范围（以区块为单位），默认 4 */
    [[nodiscard]] virtual i32 getRange() const { return 4; }

    /** @brief 获取最大雕刻高度 */
    [[nodiscard]] i32 getMaxHeight() const { return m_maxHeight; }

    /** @brief 返回 CAVE_AIR 方块状态 */
    [[nodiscard]] virtual const BlockState* getCaveAirState() const;

    /**
     * @brief 检查方块是否可被雕刻（基于配置中的 replaceable tag）
     */
    [[nodiscard]] virtual bool canReplaceBlock(const BlockState& state, const Config& config) const;

    /** @brief 是否在雕刻时处理草地/菌丝表面替换 */
    [[nodiscard]] virtual bool handlesSurfaceReplacement() const { return true; }

    /** @brief 是否在雕刻前检查区域是否有流体 */
    [[nodiscard]] virtual bool shouldCheckForFluid() const { return true; }

protected:
    i32 m_maxHeight;

    /**
     * @brief 雕刻一个椭球区域
     */
    bool carveEllipsoid(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::IBiomeSource& biomeSource,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        f32 centerX,
        f32 centerY,
        f32 centerZ,
        f32 horizontalRadius,
        f32 verticalRadius,
        CarvingMask& carvingMask,
        const CarveSkipChecker& skipChecker,
        const Config& config);

    /**
     * @brief 检查椭球是否在雕刻范围内
     */
    [[nodiscard]] static bool isInCarvingRange(
        ChunkCoord targetChunkX, ChunkCoord targetChunkZ, f32 x, f32 z, i32 step, i32 maxSteps, f32 radius);

    /**
     * @brief 检查椭球范围内是否有流体
     */
    [[nodiscard]] bool checkAreaForFluid(ChunkPrimer& chunk,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        i32 minX,
        i32 maxX,
        i32 minY,
        i32 maxY,
        i32 minZ,
        i32 maxZ) const;

    /**
     * @brief 获取雕刻后方块状态
     *
     * 1. 如果 y <= lavaLevel → LAVA
     * 2. 否则如果含水层可用 → aquifer.computeSubstance()
     * 3. 如果含水层返回 nullptr → 不雕刻
     *
     * @return 雕刻后方块状态，nullptr 表示不雕刻
     */
    [[nodiscard]] virtual const BlockState* getCarveState(
        CarvingContext& context, i32 worldX, i32 worldY, i32 worldZ, const Config& config) const;
};

/**
 * @brief 配置化雕刻器的类型擦除基类
 */
class ConfiguredCarverBase {
public:
    virtual ~ConfiguredCarverBase() = default;

    virtual bool carve(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::IBiomeSource& biomeSource,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        ChunkCoord originChunkX,
        ChunkCoord originChunkZ,
        CarvingMask& carvingMask,
        math::IRandom& rng) const = 0;

    [[nodiscard]] virtual bool shouldCarve(math::IRandom& rng, ChunkCoord chunkX, ChunkCoord chunkZ) const = 0;
};

/**
 * @brief 配置化的雕刻器
 *
 * 组合雕刻器和配置，方便注册和使用。
 */
template <typename Carver, typename Config>
class ConfiguredCarver : public ConfiguredCarverBase {
public:
    ConfiguredCarver(std::unique_ptr<Carver> carver, Config config)
        : m_carver(std::move(carver))
        , m_config(std::move(config))
    {}

    bool carve(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::IBiomeSource& biomeSource,
        ChunkCoord targetChunkX,
        ChunkCoord targetChunkZ,
        ChunkCoord originChunkX,
        ChunkCoord originChunkZ,
        CarvingMask& carvingMask,
        math::IRandom& rng) const override
    {
        return m_carver->carve(chunk,
            context,
            biomeSource,
            targetChunkX,
            targetChunkZ,
            originChunkX,
            originChunkZ,
            carvingMask,
            rng,
            m_config);
    }

    [[nodiscard]] bool shouldCarve(math::IRandom& rng, ChunkCoord chunkX, ChunkCoord chunkZ) const override
    {
        return m_carver->shouldCarve(rng, chunkX, chunkZ, m_config);
    }

    [[nodiscard]] Carver& getCarver() { return *m_carver; }
    [[nodiscard]] const Carver& getCarver() const { return *m_carver; }
    [[nodiscard]] const Config& getConfig() const { return m_config; }

private:
    std::unique_ptr<Carver> m_carver;
    Config m_config;
};

} // namespace mc
