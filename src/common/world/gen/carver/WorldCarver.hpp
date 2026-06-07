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

#include "CarvingContext.hpp"
#include "CarvingMask.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include <bitset>
#include <functional>
#include <memory>
#include <vector>

namespace mc {

// 前向声明
namespace world::biome {
class BiomeSource;
}
class Random;

/**
 * @brief 雕刻器配置接口
 */
struct ICarverConfig {
    virtual ~ICarverConfig() = default;
};

/**
 * @brief 概率配置
 *
 * 控制雕刻器生成概率。
 */
struct ProbabilityConfig : public ICarverConfig {
    /// 生成概率 (0.0 - 1.0)
    f32 probability;

    explicit ProbabilityConfig(f32 prob = 0.14285715f)
        : probability(prob)
    {}
};

/**
 * @brief 世界雕刻器基类
 *
 * 定义雕刻器的通用接口和工具方法。
 *
 * @tparam Config 配置类型
 */
template <typename Config>
class WorldCarver {
public:
    /**
     * @brief 构造雕刻器
     * @param maxHeight 最大雕刻高度
     */
    explicit WorldCarver(i32 maxHeight = world::MAX_BUILD_HEIGHT)
        : m_maxHeight(maxHeight)
    {}

    virtual ~WorldCarver() = default;

    /**
     * @brief 在区块中执行雕刻
     *
     * MC 1.21: RNG 由调用方通过 setLargeFeatureSeed 初始化，
     * shouldCarve 已在调用方完成，carve 直接使用传入的 RNG。
     *
     * @param chunk 要雕刻的区块
     * @param context 雕刻上下文（含水层引用等）
     * @param biomeSource 生物群系源
     * @param seaLevel 海平面高度
     * @param chunkX 雕刻起始区块 X 坐标（可能不是目标区块）
     * @param chunkZ 雕刻起始区块 Z 坐标（可能不是目标区块）
     * @param carvingMask 雕刻掩码
     * @param rng 已初始化的随机数生成器（由 applyCarvers 通过 setLargeFeatureSeed 初始化）
     * @param config 配置
     * @return 是否雕刻了任何方块
     */
    virtual bool carve(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::BiomeSource& biomeSource,
        i32 seaLevel,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        CarvingMask& carvingMask,
        math::IRandom& rng,
        const Config& config) = 0;

    /**
     * @brief 检查是否应该在这个区块执行雕刻
     *
     * @param rng 随机数生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param config 配置
     * @return 是否应该雕刻
     */
    [[nodiscard]] virtual bool shouldCarve(
        math::IRandom& rng, ChunkCoord chunkX, ChunkCoord chunkZ, const Config& config) const = 0;

    /**
     * @brief 获取雕刻器的影响范围（以区块为单位）
     * @return 影响范围
     */
    [[nodiscard]] virtual i32 getRange() const { return 4; }

    /**
     * @brief 获取最大雕刻高度
     * @return 最大高度
     */
    [[nodiscard]] i32 getMaxHeight() const { return m_maxHeight; }

    /**
     * @brief 获取熔岩填充高度
     * 低于此高度的雕刻区域用熔岩填充
     * @return 熔岩高度（默认 11）
     */
    [[nodiscard]] virtual i32 getLavaLevel() const { return 11; }

    /**
     * @brief 获取空气方块状态
     *
     * 返回 CAVE_AIR 方块状态，用于洞穴、峡谷等地下结构的空气空间。
     * 下界同样使用 CAVE_AIR（通过 getLavaLevel() 区分熔岩填充区域）。
     *
     * @return CAVE_AIR 方块状态
     */
    [[nodiscard]] virtual const BlockState* getCaveAirState() const;

    /**
     * @brief 检查方块是否可以被雕刻
     * @param state 方块状态
     * @return 是否可雕刻
     */
    [[nodiscard]] static bool isCarvable(const BlockState& state);

    /**
     * @brief 检查是否可以雕刻该方块（考虑上方方块）
     * @param state 当前方块状态
     * @param aboveState 上方方块状态
     * @return 是否可以雕刻
     */
    [[nodiscard]] virtual bool canCarveBlock(const BlockState* state, const BlockState* aboveState) const;

    /**
     * @brief 是否在雕刻时处理草地/菌丝表面替换
     *
     * MC原版中，NetherWorldCarver重写carveBlock时不做草地替换。
     * 返回false的子类将跳过草地方块下方的泥土替换逻辑。
     * @return 是否需要处理草地替换（默认true）
     */
    [[nodiscard]] virtual bool handlesSurfaceReplacement() const { return true; }

    /**
     * @brief 是否在雕刻前检查区域是否有流体
     *
     * MC原版中，水下雕刻器和下界雕刻器不检查流体（或检查不同的流体集合）。
     * 返回false的子类将跳过checkAreaForFluid预检查。
     * @return 是否需要检查流体（默认true）
     */
    [[nodiscard]] virtual bool shouldCheckForFluid() const { return true; }

protected:
    i32 m_maxHeight;

    /**
     * @brief 雕刻一个椭球区域
     *
     * @param chunk 区块数据
     * @param context 雕刻上下文（含水层引用等）
     * @param biomeSource 生物群系源
     * @param seaLevel 海平面高度
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param centerX 椭球中心X
     * @param centerY 椭球中心Y
     * @param centerZ 椭球中心Z
     * @param horizontalRadius 水平半径
     * @param verticalRadius 垂直半径
     * @param carvingMask 雕刻掩码
     * @param seed 随机种子
     * @return 是否雕刻了任何方块
     */
    bool carveEllipsoid(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::BiomeSource& biomeSource,
        i32 seaLevel,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        f32 centerX,
        f32 centerY,
        f32 centerZ,
        f32 horizontalRadius,
        f32 verticalRadius,
        CarvingMask& carvingMask,
        i64 seed);

    /**
     * @brief 检查椭球是否在雕刻范围内
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param x 当前X坐标
     * @param z 当前Z坐标
     * @param step 当前步数
     * @param maxSteps 最大步数
     * @param radius 当前半径
     * @return 是否在范围内
     */
    [[nodiscard]] static bool isInCarvingRange(
        ChunkCoord chunkX, ChunkCoord chunkZ, f32 x, f32 z, i32 step, i32 maxSteps, f32 radius);

    /**
     * @brief 检查椭球位置是否有效（检查水面）
     *
     * @param chunk 区块数据
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param minX 最小X
     * @param maxX 最大X
     * @param minY 最小Y
     * @param maxY 最大Y
     * @param minZ 最小Z
     * @param maxZ 最大Z
     * @return 是否有效（无水）
     */
    [[nodiscard]] bool checkAreaForFluid(ChunkPrimer& chunk,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        i32 minX,
        i32 maxX,
        i32 minY,
        i32 maxY,
        i32 minZ,
        i32 maxZ) const;

    /**
     * @brief 检查是否应该跳过椭球内的这个位置
     * @param dx X偏移（归一化）
     * @param dy Y偏移（归一化）
     * @param dz Z偏移（归一化）
     * @param y Y坐标（世界坐标）
     * @return 是否应该跳过
     */
    [[nodiscard]] virtual bool shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const = 0;
};

/**
 * @brief 配置化的雕刻器
 *
 * 组合雕刻器和配置，方便注册和使用。
 */
template <typename Carver, typename Config>
class ConfiguredCarver {
public:
    ConfiguredCarver(std::unique_ptr<Carver> carver, Config config)
        : m_carver(std::move(carver))
        , m_config(std::move(config))
    {}

    /**
     * @brief 在区块中执行雕刻
     */
    bool carve(ChunkPrimer& chunk,
        CarvingContext& context,
        const world::biome::BiomeSource& biomeSource,
        i32 seaLevel,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        CarvingMask& carvingMask,
        math::IRandom& rng)
    {
        return m_carver->carve(chunk, context, biomeSource, seaLevel, chunkX, chunkZ, carvingMask, rng, m_config);
    }

    /**
     * @brief 检查是否应该雕刻
     */
    [[nodiscard]] bool shouldCarve(math::IRandom& rng, ChunkCoord chunkX, ChunkCoord chunkZ) const
    {
        return m_carver->shouldCarve(rng, chunkX, chunkZ, m_config);
    }

    /**
     * @brief 获取雕刻器
     */
    [[nodiscard]] Carver& getCarver() { return *m_carver; }

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const Config& getConfig() const { return m_config; }

private:
    std::unique_ptr<Carver> m_carver;
    Config m_config;
};

} // namespace mc
