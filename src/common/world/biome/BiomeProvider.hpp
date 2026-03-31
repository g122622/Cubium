#pragma once

#include "Biome.hpp"
#include "../chunk/IChunk.hpp"
#include "../gen/noise/OctavesNoiseGenerator.hpp"
#include "../../core/Types.hpp"
#include <memory>
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
    virtual void getBiomesBatch(i32 startX, i32 startY, i32 startZ, i32 width, i32 height,
                                 BiomeId* output) const {
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
    virtual void getNoiseBiomesBatch(i32 startNoiseX, i32 startNoiseY, i32 startNoiseZ,
                                      i32 width, i32 height, BiomeId* output) const {
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

protected:
    u64 m_seed;
};

} // namespace mc
