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

#include "../../../gen/noise/OctavesNoiseGenerator.hpp"
#include "../../BiomeProvider.hpp"
#include <memory>

namespace mc {
namespace biome {
namespace end {

/**
 * @brief 末地生物群系提供者
 *
 * 参考 MC 1.16.5 EndBiomeProvider
 * 使用原版岛屿高度函数区分主岛与外岛四类生物群系。
 *
 * 末地生物群系：
 * - The End (末地) - 主岛，包含末地龙战斗区域
 * - Small End Islands (小型末地岛屿) - 外岛的小型岛屿
 * - End Midlands (末地中部) - 外岛的中部区域
 * - End Highlands (末地高地) - 外岛的高地区域
 * - End Barrens (末地荒地) - 外岛的荒地区域
 *
 * 使用示例:
 * @code
 * EndBiomeProvider provider(seed);
 * BiomeId biome = provider.getBiome(x, y, z);
 * @endcode
 */
class EndBiomeProvider : public BiomeProvider {
public:
    /**
     * @brief 构造函数
     * @param seed 世界种子
     */
    explicit EndBiomeProvider(u64 seed);

    ~EndBiomeProvider() override = default;

    // ========== BiomeProvider 接口实现 ==========

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] f32 getDepth(i32 x, i32 z) const override;
    [[nodiscard]] f32 getScale(i32 x, i32 z) const override;
    void fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) override;

    // ========== 末地特有方法 ==========

    /**
     * @brief 检查是否在主岛范围内
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 是否在主岛范围内
     */
    [[nodiscard]] bool isInMainIsland(i32 x, i32 z) const;

    /**
     * @brief 获取岛屿高度值
     * @param x 采样 X（与原版 func_235317_a_ 输入一致）
     * @param z 采样 Z（与原版 func_235317_a_ 输入一致）
     * @return 岛屿高度值
     */
    [[nodiscard]] f32 getIslandHeight(i32 x, i32 z) const;

private:
    // 岛屿噪声生成器
    std::unique_ptr<SimplexNoiseGenerator> m_islandNoise;

    // 原版主岛判定阈值：在 getNoiseBiome 中使用 (i*i + j*j <= 4096)
    // 其中 i = noiseX >> 2（等价于世界坐标 x >> 4）。
    static constexpr i64 MAIN_ISLAND_RADIUS_SQ = 4096;

    /**
     * @brief 根据噪声坐标选择末地生物群系
     */
    [[nodiscard]] BiomeId selectBiome(i32 noiseX, i32 noiseZ) const;

    /**
     * @brief 原版末地岛屿高度函数（func_235317_a_）
     */
    [[nodiscard]] static f32 computeIslandHeight(const SimplexNoiseGenerator& noise, i32 x, i32 z);
};

} // namespace end
} // namespace biome
} // namespace mc
