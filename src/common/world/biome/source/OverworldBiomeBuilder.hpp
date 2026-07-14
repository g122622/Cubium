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
#include "common/world/biome/climate/ParameterList.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include <functional>
#include <vector>

namespace mc::world::biome::source {

/**
 * @brief 主世界生物群系参数构建器
 *
 * 定义主世界所有生物群系到 Climate 参数的映射关系。
 * 通过温度×湿度×大陆度×侵蚀×奇异度×深度参数组合确定每个位置应生成的生物群系。
 *
 * 13 个奇异度切片将地形分为：
 * - MidSlice（4个）：中间地形，河流/山谷区域
 * - HighSlice（4个）：山坡地形
 * - Peaks（2个）：山峰地形
 * - LowSlice（2个）：低地地形
 * - Valleys（1个）：山谷/河流地形
 *
 * 每个切片内按 5 温度 × 5 湿度 × 7 侵蚀 × 大陆度组合注册生物群系。
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

    /**
     * @brief 主世界出生点气候目标
     *
     * MC 1.21.11: OverworldBiomeBuilder.spawnTarget()
     * 返回 2 个 ParameterPoint，depth=0、continentalness 跨内陆到全范围、
     * weirdness 分别覆盖 [-1, -0.16] 与 [0.16, 1]。
     * Climate.Sampler.findSpawnPosition() 用此目标在气候空间径向搜索最佳出生区块。
     *
     * @return 出生点目标 ParameterPoint 列表（始终为 2 项）
     */
    [[nodiscard]] std::vector<climate::ParameterPoint> spawnTarget() const;

private:
    // ========== 气候参数范围 ==========

    /// 温度范围（5 档：冰冻/冷/温和/暖/热）
    climate::Parameter m_temperatures[5];
    /// 湿度范围（5 档：干旱/干燥/中性/湿润/潮湿）
    climate::Parameter m_humidities[5];
    /// 侵蚀范围（7 档）
    climate::Parameter m_erosions[7];

    /// 全范围参数 [-1, 1]（MC OverworldBiomeBuilder.FULL_RANGE）
    climate::Parameter m_fullRange;
    /// 冰冻温度范围 = temperatures[0]
    climate::Parameter m_frozenRange;
    /// 非冰冻温度范围 = span(temperatures[1], temperatures[4])
    climate::Parameter m_unfrozenRange;

    /// 大陆度范围
    climate::Parameter m_mushroomFieldsContinentalness;
    climate::Parameter m_deepOceanContinentalness;
    climate::Parameter m_oceanContinentalness;
    climate::Parameter m_coastContinentalness;
    climate::Parameter m_inlandContinentalness;
    climate::Parameter m_nearInlandContinentalness;
    climate::Parameter m_midInlandContinentalness;
    climate::Parameter m_farInlandContinentalness;

    // ========== 生物群系查找表 ==========

    /// 海洋生物群系[温度][深浅: 0=深海, 1=浅海]
    BiomeId m_oceans[5][2];
    /// 中部生物群系[温度][湿度]
    BiomeId m_middleBiomes[5][5];
    /// 中部变体生物群系[温度][湿度]（奇异度>=0时使用，BIOME_NULL 表示无变体）
    BiomeId m_middleBiomesVariant[5][5];
    /// 高原生物群系[温度][湿度]
    BiomeId m_plateauBiomes[5][5];
    /// 高原变体生物群系[温度][湿度]
    BiomeId m_plateauBiomesVariant[5][5];
    /// 破碎生物群系[温度][湿度]
    BiomeId m_shatteredBiomes[5][5];

    // ========== 生物群系注册方法 ==========

    /** 注册近海生物群系（蘑菇岛、深海、浅海） */
    void addOffCoastBiomes(std::vector<climate::ParameterList<BiomeId>::Entry>& entries) const;

    /** 注册内陆生物群系（13 个奇异度切片） */
    void addInlandBiomes(std::vector<climate::ParameterList<BiomeId>::Entry>& entries) const;

    /** 注册地下生物群系（滴水石洞、繁茂洞穴、深暗之域） */
    void addUndergroundBiomesEntries(std::vector<climate::ParameterList<BiomeId>::Entry>& entries) const;

    /** 注册中间切片生物群系 */
    void addMidSlice(
        std::vector<climate::ParameterList<BiomeId>::Entry>& entries, const climate::Parameter& weirdness) const;

    /** 注册高切片生物群系 */
    void addHighSlice(
        std::vector<climate::ParameterList<BiomeId>::Entry>& entries, const climate::Parameter& weirdness) const;

    /** 注册山峰切片生物群系 */
    void addPeaks(
        std::vector<climate::ParameterList<BiomeId>::Entry>& entries, const climate::Parameter& weirdness) const;

    /** 注册低切片生物群系 */
    void addLowSlice(
        std::vector<climate::ParameterList<BiomeId>::Entry>& entries, const climate::Parameter& weirdness) const;

    /** 注册山谷切片生物群系（河流、冻河、沼泽等） */
    void addValleys(
        std::vector<climate::ParameterList<BiomeId>::Entry>& entries, const climate::Parameter& weirdness) const;

    // ========== 参数添加辅助方法 ==========

    /**
     * @brief 添加表面生物群系
     *
     * 同时注册 depth=0 和 depth=1 两个参数点。
     */
    void addSurfaceBiome(std::vector<climate::ParameterList<BiomeId>::Entry>& entries,
        const climate::Parameter& temperature,
        const climate::Parameter& humidity,
        const climate::Parameter& continentalness,
        const climate::Parameter& erosion,
        const climate::Parameter& weirdness,
        f32 offset,
        BiomeId biome) const;

    /**
     * @brief 添加地下生物群系
     *
     * 使用 depth=[0.2, 0.9] 范围。
     */
    void addUndergroundBiome(std::vector<climate::ParameterList<BiomeId>::Entry>& entries,
        const climate::Parameter& temperature,
        const climate::Parameter& humidity,
        const climate::Parameter& continentalness,
        const climate::Parameter& erosion,
        const climate::Parameter& weirdness,
        f32 offset,
        BiomeId biome) const;

    /**
     * @brief 添加底层生物群系
     *
     * 使用 depth=1.1 点匹配。
     */
    void addBottomBiome(std::vector<climate::ParameterList<BiomeId>::Entry>& entries,
        const climate::Parameter& temperature,
        const climate::Parameter& humidity,
        const climate::Parameter& continentalness,
        const climate::Parameter& erosion,
        const climate::Parameter& weirdness,
        f32 offset,
        BiomeId biome) const;

    // ========== 生物群系选择方法 ==========

    /** 选择中部生物群系，根据奇异度选择变体 */
    [[nodiscard]] BiomeId pickMiddleBiome(i32 temperature, i32 humidity, const climate::Parameter& weirdness) const;

    /** 选择高原生物群系，根据奇异度选择变体 */
    [[nodiscard]] BiomeId pickPlateauBiome(i32 temperature, i32 humidity, const climate::Parameter& weirdness) const;

    /** 选择山峰生物群系 */
    [[nodiscard]] BiomeId pickPeakBiome(i32 temperature, i32 humidity, const climate::Parameter& weirdness) const;

    /** 选择山坡生物群系 */
    [[nodiscard]] BiomeId pickSlopeBiome(i32 temperature, i32 humidity, const climate::Parameter& weirdness) const;

    /** 选择恶地生物群系 */
    [[nodiscard]] BiomeId pickBadlandsBiome(i32 humidity, const climate::Parameter& weirdness) const;

    /** 选择海滩生物群系 */
    [[nodiscard]] BiomeId pickBeachBiome(i32 temperature) const;

    /** 选择破碎生物群系，null 时回退到 pickMiddleBiome */
    [[nodiscard]] BiomeId pickShatteredBiome(i32 temperature, i32 humidity, const climate::Parameter& weirdness) const;

    /** 热带时选择恶地，否则选择中部 */
    [[nodiscard]] BiomeId pickMiddleBiomeOrBadlandsIfHot(
        i32 temperature, i32 humidity, const climate::Parameter& weirdness) const;

    /** 热带时选择恶地，冰冻时选择山坡，否则选择中部 */
    [[nodiscard]] BiomeId pickMiddleBiomeOrBadlandsIfHotOrSlopeIfCold(
        i32 temperature, i32 humidity, const climate::Parameter& weirdness) const;

    /** 条件性选择风袭热带草原 */
    [[nodiscard]] BiomeId maybePickWindsweptSavannaBiome(
        i32 temperature, i32 humidity, const climate::Parameter& weirdness, BiomeId defaultBiome) const;

    /** 选择破碎海岸生物群系 */
    [[nodiscard]] BiomeId pickShatteredCoastBiome(
        i32 temperature, i32 humidity, const climate::Parameter& weirdness) const;
};

} // namespace mc::world::biome::source
