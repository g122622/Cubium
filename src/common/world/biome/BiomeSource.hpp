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
 */

#pragma once

#include "BiomeIds.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include <functional>
#include <optional>
#include <unordered_set>
#include <vector>

namespace mc {
namespace world {
namespace biome {

class Biome;

/**
 * @brief 生物群系源接口
 *
 * 替代旧版 BiomeProvider，支持 3D 多噪声生物群系生成。
 * 核心接口是 getNoiseBiome()，接收 quart 坐标（1 quart = 4 blocks）。
 *
 * 子类：
 * - MultiNoiseBiomeSource: 基于 Climate 参数的多噪声生物群系源（主世界、下界）
 * - EndBiomeSource: 末地专用生物群系源
 */
class IBiomeSource {
public:
    explicit IBiomeSource(u64 seed);
    virtual ~IBiomeSource() = default;

    /**
     * @brief 获取噪声坐标处的生物群系
     *
     * quart 坐标 = block 坐标 / 4
     *
     * @param quartX X quart 坐标
     * @param quartY Y quart 坐标（3D biome 使用）
     * @param quartZ Z quart 坐标
     * @return 生物群系ID
     */
    [[nodiscard]] virtual BiomeId getNoiseBiome(i32 quartX, i32 quartY, i32 quartZ) const = 0;

    /**
     * @brief 获取此源可能生成的所有生物群系
     *
     * 用于结构生成检查（判断某位置是否可能生成某结构的生物群系）。
     */
    [[nodiscard]] virtual const std::vector<BiomeId>& possibleBiomes() const = 0;

    /**
     * @brief 填充区块的 3D 生物群系数据
     *
     * 遍历区块内所有 section 的 4x4x4 网格，
     * 通过 getNoiseBiome() 采样填充 BiomeContainer。
     * 此方法为 final，子类只需实现 getNoiseBiome()。
     *
     * @param container 生物群系容器（已扩展到 24 section）
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     */
    void fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 获取生物群系定义
     * @param id 生物群系ID
     * @return 生物群系定义
     */
    [[nodiscard]] virtual const Biome& getBiomeDefinition(BiomeId id) const;

    [[nodiscard]] u64 seed() const { return m_seed; }

    // ========== 生物群系搜索 ==========

    /**
     * @brief 在指定范围内搜索生物群系
     *
     * 从中心点向外螺旋搜索，使用蓄水池采样随机选择匹配结果。
     *
     * @param centerX 中心 X 坐标（世界坐标）
     * @param centerY 中心 Y 坐标（世界坐标）
     * @param centerZ 中心 Z 坐标（世界坐标）
     * @param radius 搜索半径（方块）
     * @param step 搜索步长（方块）
     * @param predicate 生物群系匹配条件
     * @param random 随机数生成器
     * @param stopOnFirst 找到第一个匹配即返回
     * @return 找到的位置，如果未找到返回 std::nullopt
     */
    [[nodiscard]] std::optional<BlockPos> findBiome(i32 centerX,
        i32 centerY,
        i32 centerZ,
        i32 radius,
        i32 step,
        const std::function<bool(BiomeId)>& predicate,
        math::Random& random,
        bool stopOnFirst) const;

    /**
     * @brief 获取指定范围内所有不同的生物群系
     *
     * 在以 (x,y,z) 为中心、radius 为半径的立方体内采样所有生物群系。
     *
     * @param x 中心 X 坐标（世界坐标）
     * @param y 中心 Y 坐标（世界坐标）
     * @param z 中心 Z 坐标（世界坐标）
     * @param radius 搜索半径（方块）
     * @return 范围内所有不同生物群系的集合
     */
    [[nodiscard]] std::unordered_set<BiomeId> getBiomesWithin(i32 x, i32 y, i32 z, i32 radius) const;

protected:
    u64 m_seed;
    std::vector<BiomeId> m_possibleBiomes;
};

// 旧名称兼容别名
using BiomeSource = IBiomeSource;

} // namespace biome
} // namespace world
} // namespace mc
