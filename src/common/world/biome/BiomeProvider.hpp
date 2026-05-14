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

#include "../../core/Types.hpp"
#include "../chunk/IChunk.hpp"
#include "../gen/noise/OctavesNoiseGenerator.hpp"
#include "Biome.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <random>

namespace mc {

/**
 * @brief 生物群系提供者基类
 *
 * 参考 MC BiomeProvider，负责提供世界中的生物群系信息。
 *
 * 使用方法：
 * @code
 * auto provider = createBiomeProvider(seed);
 * BiomeId biome = provider->getBiome(x, y, z);
 * @endcode
 *
 * @note 参考 MC 1.16.5 BiomeProvider
 */
class BiomeProvider {
public:
    explicit BiomeProvider(u64 seed);
    virtual ~BiomeProvider() = default;

    /**
     * @brief 获取世界坐标处的生物群系
     * @param x 世界 X 坐标
     * @param y 世界 Y 坐标
     * @param z 世界 Z 坐标
     * @return 生物群系ID
     */
    [[nodiscard]] virtual BiomeId getBiome(i32 x, i32 y, i32 z) const = 0;

    /**
     * @brief 获取噪声坐标处的生物群系
     *
     * 噪声坐标是 4x4 方块一个大块
     * @param noiseX 噪声 X 坐标 (x / 4)
     * @param noiseY 噪声 Y 坐标 (y / 4)
     * @param noiseZ 噪声 Z 坐标 (z / 4)
     * @return 生物群系ID
     */
    [[nodiscard]] virtual BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const = 0;

    /**
     * @brief 获取生物群系的深度参数
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 深度值（影响地形高度）
     */
    [[nodiscard]] virtual f32 getDepth(i32 x, i32 z) const = 0;

    /**
     * @brief 获取生物群系的比例参数
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 比例值（影响高度变化）
     */
    [[nodiscard]] virtual f32 getScale(i32 x, i32 z) const = 0;

    /**
     * @brief 获取生物群系定义
     * @param id 生物群系ID
     * @return 生物群系定义
     */
    [[nodiscard]] virtual const Biome& getBiomeDefinition(BiomeId id) const;

    /**
     * @brief 填充区块的生物群系数据
     * @param container 生物群系容器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     */
    virtual void fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) = 0;

    /**
     * @brief 批量获取生物群系
     *
     * 默认实现通过循环调用 getBiome()，子类可以覆盖以优化性能。
     *
     * @param startX 起始 X 坐标
     * @param startY 起始 Y 坐标
     * @param startZ 起始 Z 坐标
     * @param width 宽度（X 方向）
     * @param height 高度（Z 方向）
     * @param output 输出数组（大小必须 >= width * height）
     */
    virtual void getBiomesBatch(i32 startX, i32 startY, i32 startZ, i32 width, i32 height, BiomeId* output) const
    {
        if (output == nullptr || width <= 0 || height <= 0) {
            return;
        }
        size_t idx = 0;
        for (i32 z = 0; z < height; ++z) {
            for (i32 x = 0; x < width; ++x) {
                output[idx++] = getBiome(startX + x, startY, startZ + z);
            }
        }
    }

    /**
     * @brief 批量获取噪声坐标下的生物群系
     *
     * 默认实现通过循环调用 getNoiseBiome()。
     *
     * @param startNoiseX 起始噪声 X 坐标
     * @param startNoiseY 起始噪声 Y 坐标
     * @param startNoiseZ 起始噪声 Z 坐标
     * @param width 宽度（X 方向）
     * @param height 高度（Z 方向）
     * @param output 输出数组（大小必须 >= width * height）
     */
    virtual void getNoiseBiomesBatch(
        i32 startNoiseX, i32 startNoiseY, i32 startNoiseZ, i32 width, i32 height, BiomeId* output) const
    {
        if (output == nullptr || width <= 0 || height <= 0) {
            return;
        }
        size_t idx = 0;
        for (i32 z = 0; z < height; ++z) {
            for (i32 x = 0; x < width; ++x) {
                output[idx++] = getNoiseBiome(startNoiseX + x, startNoiseY, startNoiseZ + z);
            }
        }
    }

    [[nodiscard]] u64 seed() const { return m_seed; }

    // ========== 生物群系搜索 ==========

    /**
     * @brief 在指定范围内搜索生物群系
     *
     * 参考 MC 1.16.5 BiomeProvider.func_230321_a_
     * 从中心点向外螺旋搜索，直到找到匹配的生物群系。
     *
     * @param centerX 中心 X 坐标（世界坐标）
     * @param centerY 中心 Y 坐标（世界坐标）
     * @param centerZ 中心 Z 坐标（世界坐标）
     * @param radius 搜索半径（方块）
     * @param step 搜索步长（方块，默认为 64，即 16 个噪声采样点）
     * @param predicate 生物群系匹配条件
     * @param random 随机数生成器（用于在多个匹配中随机选择）
     * @param stopOnFirst 找到第一个匹配即返回（不随机选择）
     * @return 找到的位置，如果未找到返回 std::nullopt
     */
    [[nodiscard]] std::optional<BlockPos> findBiome(i32 centerX,
        i32 centerY,
        i32 centerZ,
        i32 radius,
        i32 step,
        const std::function<bool(BiomeId)>& predicate,
        math::Random& random,
        bool stopOnFirst = false) const;

protected:
    u64 m_seed;
};

} // namespace mc
