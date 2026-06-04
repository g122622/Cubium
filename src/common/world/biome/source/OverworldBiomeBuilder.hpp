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

#include "common/world/biome/Biomes.hpp"
#include "common/world/biome/climate/Climate.hpp"
#include <vector>

namespace mc::world::biome::source {

/**
 * @brief 主世界生物群系参数构建器（MC 1.21）
 *
 * 定义主世界所有生物群系到 Climate 参数的映射关系。
 * 通过温度×湿度矩阵、大陆度、侵蚀、深度、奇异度等参数
 * 组合确定每个位置应生成的生物群系。
 *
 * 映射规则：
 * - 表面生物群系：根据温度×湿度×大陆度×侵蚀×奇异度选择
 * - 高原生物群系：中等大陆度时的特殊生物群系
 * - 山峰生物群系：高奇异度值时
 * - 山坡生物群系：中高奇异度值时
 * - 海洋生物群系：负大陆度值时
 * - 地下生物群系：高深度值时（滴水石洞、繁茂洞穴、深暗之域）
 */
class OverworldBiomeBuilder {
public:
    OverworldBiomeBuilder();

    /**
     * @brief 构建完整的 ParameterList<BiomeId>
     *
     * 包含所有主世界生物群系的参数映射。
     * @return 可用于 MultiNoiseBiomeSource 的参数列表
     */
    [[nodiscard]] climate::ParameterList<BiomeId> buildParameterList() const;

private:
    // ========== 气候参数范围 ==========

    /// 温度范围（5 档：冰冻/冷/温和/暖/热）
    climate::Parameter m_temperatures[5];
    /// 湿度范围（5 档：干旱/干燥/中性/湿润/潮湿）
    climate::Parameter m_humidities[5];
    /// 侵蚀范围（7 档）
    climate::Parameter m_erosions[7];

    /// 大陆度范围
    climate::Parameter m_mushroomFieldsContinentalness;
    climate::Parameter m_deepOceanContinentalness;
    climate::Parameter m_oceanContinentalness;
    climate::Parameter m_coastContinentalness;
    climate::Parameter m_nearInlandContinentalness;
    climate::Parameter m_midInlandContinentalness;
    climate::Parameter m_farInlandContinentalness;

    /// 奇异度切片范围
    climate::Parameter m_valleyWeirdness;
    climate::Parameter m_lowWeirdness;
    climate::Parameter m_midWeirdness;
    climate::Parameter m_highWeirdness;
    climate::Parameter m_peakWeirdness;

    /// 深度范围
    climate::Parameter m_surfaceDepth;
    climate::Parameter m_undergroundDepth;

    // ========== 生物群系查找表 ==========

    /// 中部生物群系[温度][湿度]
    BiomeId m_middleBiomes[5][5];
    /// 中部变体生物群系[温度][湿度]（奇异度>=0时使用，null 表示无变体）
    BiomeId m_middleBiomesVariant[5][5];
    /// 高原生物群系[温度][湿度]
    BiomeId m_plateauBiomes[5][5];
    /// 高原变体生物群系[温度][湿度]
    BiomeId m_plateauBiomesVariant[5][5];
    /// 破碎生物群系[温度][湿度]
    BiomeId m_shatteredBiomes[5][5];
    /// 海洋生物群系[温度][深浅]
    BiomeId m_oceans[5][2];

    // ========== 辅助方法 ==========

    /**
     * @brief 添加表面生物群系
     *
     * 同时注册 depth=0 和 depth=1 两个参数点。
     */
    void addSurfaceBiome(climate::ParameterList<BiomeId>& list,
        const climate::Parameter& temperature,
        const climate::Parameter& humidity,
        const climate::Parameter& continentalness,
        const climate::Parameter& erosion,
        const climate::Parameter& weirdness,
        f64 offset,
        BiomeId biome) const;

    /**
     * @brief 添加地下生物群系
     *
     * 使用 depth=[0.2, 0.9] 范围。
     */
    void addUndergroundBiome(climate::ParameterList<BiomeId>& list,
        const climate::Parameter& temperature,
        const climate::Parameter& humidity,
        const climate::Parameter& continentalness,
        const climate::Parameter& erosion,
        const climate::Parameter& depth,
        const climate::Parameter& weirdness,
        f64 offset,
        BiomeId biome) const;

    /**
     * @brief 选择中部生物群系
     */
    [[nodiscard]] BiomeId pickMiddleBiome(i32 temperature, i32 humidity, f64 weirdness) const;

    /**
     * @brief 选择高原生物群系
     */
    [[nodiscard]] BiomeId pickPlateauBiome(i32 temperature, i32 humidity, f64 weirdness) const;

    /**
     * @brief 选择山峰生物群系
     */
    [[nodiscard]] BiomeId pickPeakBiome(i32 temperature, i32 humidity, f64 weirdness) const;

    /**
     * @brief 选择山坡生物群系
     */
    [[nodiscard]] BiomeId pickSlopeBiome(i32 temperature, i32 humidity, f64 weirdness) const;

    /**
     * @brief 选择恶地生物群系
     */
    [[nodiscard]] BiomeId pickBadlandsBiome(i32 humidity, f64 weirdness) const;

    /**
     * @brief 选择海滩生物群系
     */
    [[nodiscard]] BiomeId pickBeachBiome(i32 temperature) const;
};

} // namespace mc::world::biome::source
