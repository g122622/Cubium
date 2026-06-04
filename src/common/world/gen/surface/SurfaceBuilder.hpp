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
#include "common/util/math/random/Random.hpp"
#include "common/world/block/Block.hpp"

namespace mc {

// 前向声明
class BlockState;
class ChunkPrimer;
class Biome;

/**
 * @brief 地表构建器配置
 *
 * 定义地表、次表层和水下层的方块类型。
 * 使用 BlockState* 替代固定 BlockId，支持动态方块注册。
 */
struct SurfaceBuilderConfig {
    /// 表层方块（草方块、沙子等）
    const BlockState* topBlock = nullptr;

    /// 次表层方块（泥土、沙子等）
    const BlockState* underBlock = nullptr;

    /// 水下表面方块（沙砾等）
    const BlockState* underWaterBlock = nullptr;

    SurfaceBuilderConfig() = default;

    SurfaceBuilderConfig(const BlockState* top, const BlockState* under, const BlockState* underWater)
        : topBlock(top)
        , underBlock(under)
        , underWaterBlock(underWater)
    {}

    /**
     * @brief 创建草地配置
     */
    static SurfaceBuilderConfig grass();

    /**
     * @brief 创建沙地配置
     */
    static SurfaceBuilderConfig sand();

    /**
     * @brief 创建石头配置
     */
    static SurfaceBuilderConfig stone();

    /**
     * @brief 创建沙砾配置
     */
    static SurfaceBuilderConfig gravel();

    /**
     * @brief 创建红沙配置
     */
    static SurfaceBuilderConfig redSand();

    // ========== 预设配置 ==========

    /**
     * @brief 灰化土配置（巨型针叶林）
     * PODZOL_DIRT_GRAVEL_CONFIG
     */
    static SurfaceBuilderConfig podzolDirtGravel();

    /**
     * @brief 沙砾配置
     * GRAVEL_CONFIG
     */
    static SurfaceBuilderConfig gravelOnly();

    /**
     * @brief 草地沙砾配置
     * GRASS_DIRT_GRAVEL_CONFIG
     */
    static SurfaceBuilderConfig grassDirtGravel();

    /**
     * @brief 石头配置（山地）
     * STONE_STONE_GRAVEL_CONFIG
     */
    static SurfaceBuilderConfig stoneStoneGravel();

    /**
     * @brief 砂土配置
     * CORASE_DIRT_DIRT_GRAVEL_CONFIG
     */
    static SurfaceBuilderConfig coarseDirtDirtGravel();

    /**
     * @brief 沙子沙砾配置
     * SAND_SAND_GRAVEL_CONFIG
     */
    static SurfaceBuilderConfig sandSandGravel();

    /**
     * @brief 草地沙子配置
     * GRASS_DIRT_SAND_CONFIG
     */
    static SurfaceBuilderConfig grassDirtSand();

    /**
     * @brief 红沙白陶瓦沙砾配置（恶地）
     * RED_SAND_WHITE_TERRACOTTA_GRAVEL_CONFIG
     */
    static SurfaceBuilderConfig redSandWhiteTerracottaGravel();

    /**
     * @brief 菌丝体配置（蘑菇岛）
     * MYCELIUM_DIRT_GRAVEL_CONFIG
     */
    static SurfaceBuilderConfig myceliumDirtGravel();

    /**
     * @brief 下界岩配置
     * NETHERRACK_CONFIG
     */
    static SurfaceBuilderConfig netherrack();
};

/**
 * @brief 地表构建器基类
 *
 * 负责构建区块的地表层，不同的生物群系可以使用不同的地表构建器。
 */
class SurfaceBuilder {
public:
    virtual ~SurfaceBuilder() = default;

    /**
     * @brief 构建地表
     *
     * @param random 随机数生成器
     * @param chunk 区块数据
     * @param biome 生物群系
     * @param x 区块内 X 坐标 (0-15)
     * @param z 区块内 Z 坐标 (0-15)
     * @param startHeight 起始高度（从地表向下遍历）
     * @param surfaceNoise 地表噪声值（用于变化地表深度）
     * @param defaultBlock 默认方块（石头）
     * @param defaultFluid 默认流体（水）
     * @param seaLevel 海平面高度
     * @param worldSeed 世界种子（用于初始化噪声生成器）
     * @param config 地表配置
     */
    virtual void buildSurface(math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x,
        i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) = 0;

    /**
     * @brief 设置世界种子
     *
     * 某些地表构建器（如Badlands、FrozenOcean）需要基于种子初始化噪声生成器。
     * 默认实现为空，子类可以覆盖以初始化噪声生成器。
     *
     * @param seed 世界种子
     */
    virtual void setSeed(u64 seed) { (void)seed; }

    /**
     * @brief 获取地表构建器名称
     */
    [[nodiscard]] virtual const char* name() const = 0;

protected:
    /**
     * @brief 默认地表构建实现
     *
     * 提供标准的地表构建逻辑，其他构建器可以委托调用此方法。
     */
    static void _buildDefaultSurface(math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x,
        i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        const BlockState* top,
        const BlockState* middle,
        const BlockState* bottom);

    /// 世界种子（用于需要噪声的子类）
    u64 m_seed = 0;
};

} // namespace mc
