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
namespace nether {

/**
 * @brief 下界生物群系提供者
 *
 * 参考 MC 1.16.5 NetherBiomeProvider
 * 使用 3D 噪声采样确定生物群系，不同于主世界的 Layer 系统。
 *
 * 下界生物群系：
 * - Nether Wastes (下界荒地) - 默认生物群系
 * - Soul Sand Valley (灵魂沙谷) - 灵魂沙和灵魂土
 * - Crimson Forest (绯红森林) - 绯红菌和疣猪兽
 * - Warped Forest (诡异森林) - 诡异菌和末影人
 * - Basalt Deltas (玄武岩三角洲) - 玄武岩和岩浆块
 *
 * 使用示例:
 * @code
 * NetherBiomeProvider provider(seed);
 * BiomeId biome = provider.getBiome(x, y, z);
 * @endcode
 */
class NetherBiomeProvider : public BiomeProvider {
public:
    /**
     * @brief 构造函数
     * @param seed 世界种子
     */
    explicit NetherBiomeProvider(u64 seed);

    ~NetherBiomeProvider() override = default;

    // ========== BiomeProvider 接口实现 ==========

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] f32 getDepth(i32 x, i32 z) const override;
    [[nodiscard]] f32 getScale(i32 x, i32 z) const override;
    void fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) override;

    // ========== 下界特有方法 ==========

    /**
     * @brief 获取温度噪声值
     */
    [[nodiscard]] f32 getTemperature(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取湿度噪声值
     */
    [[nodiscard]] f32 getHumidity(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取生物群系选择噪声值
     */
    [[nodiscard]] f32 getBiomeNoise(i32 x, i32 y, i32 z) const;

private:
    // 噪声生成器
    std::unique_ptr<SimplexNoiseGenerator> m_temperatureNoise;
    std::unique_ptr<SimplexNoiseGenerator> m_humidityNoise;
    std::unique_ptr<PerlinNoiseGenerator> m_biomeNoise;

    // 噪声参数
    static constexpr f32 TEMPERATURE_SCALE = 0.015625f; // 1/64
    static constexpr f32 HUMIDITY_SCALE = 0.015625f;
    static constexpr f32 BIOME_SCALE = 0.0078125f; // 1/128

    /**
     * @brief 根据噪声值选择生物群系
     */
    [[nodiscard]] BiomeId selectBiome(f32 temperature, f32 humidity, f32 biomeNoise) const;
};

} // namespace nether
} // namespace biome
} // namespace mc
