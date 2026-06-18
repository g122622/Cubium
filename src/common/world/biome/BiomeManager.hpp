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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/crypto/Sha256.hpp"
#include "common/util/math/random/LinearCongruentialGenerator.hpp"
#include "common/world/biome/BiomeSource.hpp"

namespace mc::world::biome {

/**
 * @brief 生物群系管理器 — MC 1.21.11 BiomeManager
 *
 * 将 quart 分辨率的噪声生物群系（4x4x4 网格）通过 Voronoi 缩放
 * 提升到方块分辨率。Voronoi 缩放使用 LCG 驱动的 fiddling 使生物群系边界
 * 自然而非硬 4x4x4 格子。
 *
 * 核心算法：
 * 1. 对查询位置偏移 -2（避免边界问题）
 * 2. 计算 quart 坐标和子 quart 偏移
 * 3. 遍历 2x2x2 的 8 个 quart 角点
 * 4. 对每个角点用 LCG 计算 fiddle offset（伪随机抖动）
 * 5. 选择 fiddled distance 最小的角点
 * 6. 返回该角点的噪声生物群系
 *
 * 与 MC 1.21.11 BiomeManager 完全对齐。
 */
class BiomeManager {
public:
    /**
     * @brief 构造生物群系管理器
     * @param source 噪声生物群系源（非拥有引用，调用方保证生命周期）
     * @param biomeZoomSeed 生物群系缩放种子（由 obfuscateSeed 生成）
     */
    BiomeManager(const IBiomeSource& source, u64 biomeZoomSeed);

    /**
     * @brief 使用 SHA-256 哈希世界种子，生成生物群系缩放种子
     *
     * 等价于 MC Java 版 BiomeManager.obfuscateSeed(worldSeed)。
     * 使用 Guava Hashing.sha256().hashLong(seed).asLong()。
     * 与 Sha256::hashWorldSeed() 完全一致。
     *
     * @param worldSeed 世界种子
     * @return 哈希后的缩放种子
     */
    [[nodiscard]] static u64 obfuscateSeed(u64 worldSeed);

    /**
     * @brief 使用不同生物群系源创建副本（共享 zoomSeed）
     *
     * 等价于 MC Java 版 BiomeManager.withDifferentSource()。
     * 用于 applyCarvers 中创建使用噪声生物群系源的缩放 BiomeManager。
     *
     * @param source 新的生物群系源
     * @return 使用新源的 BiomeManager 副本
     */
    [[nodiscard]] BiomeManager withDifferentSource(const IBiomeSource& source) const;

    /**
     * @brief 获取方块坐标处的生物群系（带 Voronoi 缩放）
     *
     * 等价于 MC Java 版 BiomeManager.getBiome(BlockPos)。
     * 通过 2x2x2 quart 角点的 Voronoi 缩放选择最近角点的生物群系。
     *
     * @param blockX 方块 X 坐标（世界坐标）
     * @param blockY 方块 Y 坐标（世界坐标）
     * @param blockZ 方块 Z 坐标（世界坐标）
     * @return 生物群系 ID
     */
    [[nodiscard]] BiomeId getBiome(i32 blockX, i32 blockY, i32 blockZ) const;

    /**
     * @brief 在 quart 坐标直接查询生物群系（无缩放）
     *
     * 等价于 MC Java 版 BiomeManager.getNoiseBiomeAtQuart()。
     * 直接委托给噪声生物群系源。
     *
     * @param quartX 四分位 X 坐标
     * @param quartY 四分位 Y 坐标
     * @param quartZ 四分位 Z 坐标
     * @return 生物群系 ID
     */
    [[nodiscard]] BiomeId getNoiseBiomeAtQuart(i32 quartX, i32 quartY, i32 quartZ) const;

    /**
     * @brief 从方块坐标转 quart 后查询噪声生物群系（无缩放）
     *
     * 等价于 MC Java 版 BiomeManager.getNoiseBiomeAtPosition(double, double, double)。
     *
     * @param blockX 方块 X 坐标
     * @param blockY 方块 Y 坐标
     * @param blockZ 方块 Z 坐标
     * @return 生物群系 ID
     */
    [[nodiscard]] BiomeId getNoiseBiomeAtPosition(i32 blockX, i32 blockY, i32 blockZ) const;

    /**
     * @brief 获取生物群系缩放种子
     */
    [[nodiscard]] u64 biomeZoomSeed() const { return m_biomeZoomSeed; }

private:
    const IBiomeSource* m_source; ///< 非拥有引用
    u64 m_biomeZoomSeed;          ///< 缩放种子（obfuscateSeed 生成）

    /**
     * @brief 计算 fiddled distance（Voronoi 缩放核心）
     *
     * 等价于 MC Java 版 BiomeManager.getFiddledDistance()。
     * 对 8 个 quart 角点中的每一个，计算其 fiddled squared distance。
     *
     * @param seed 缩放种子
     * @param x quart X 坐标
     * @param y quart Y 坐标
     * @param z quart Z 坐标
     * @param fudgeX X 方向子 quart 偏移（已调整）
     * @param fudgeY Y 方向子 quart 偏移（已调整）
     * @param fudgeZ Z 方向子 quart 偏移（已调整）
     * @return fiddled squared distance
     */
    [[nodiscard]] static f64 getFiddledDistance(u64 seed, i32 x, i32 y, i32 z, f64 fudgeX, f64 fudgeY, f64 fudgeZ);

    /**
     * @brief 从 LCG 种子中提取 fiddle 值
     *
     * 等价于 MC Java 版 BiomeManager.getFiddle()。
     * 提取种子的高 10 位 (bits 24-33)，映射到 [-0.45, 0.45)。
     *
     * @param seed LCG 状态
     * @return fiddle 值，范围 [-0.45, 0.45)
     */
    [[nodiscard]] static f64 getFiddle(i64 seed);
};

} // namespace mc::world::biome
