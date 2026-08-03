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

#include "common/world/biome/source/OverworldBiomeBuilder.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/climate/ParameterList.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include <limits>
#include <utility>
#include <vector>

namespace mc::world::biome::source {

using namespace climate;

namespace B = Biomes;

/// 哨兵值，表示该位置无生物群系变体
static constexpr BiomeId BIOME_NULL = std::numeric_limits<BiomeId>::max();

// ============================================================================
// 构造函数
// ============================================================================

OverworldBiomeBuilder::OverworldBiomeBuilder()
{
    // ========== 温度范围 ==========
    m_temperatures[0] = Parameter::span(-1.0f, -0.45f);
    m_temperatures[1] = Parameter::span(-0.45f, -0.15f);
    m_temperatures[2] = Parameter::span(-0.15f, 0.2f);
    m_temperatures[3] = Parameter::span(0.2f, 0.55f);
    m_temperatures[4] = Parameter::span(0.55f, 1.0f);

    // ========== 湿度范围 ==========
    m_humidities[0] = Parameter::span(-1.0f, -0.35f);
    m_humidities[1] = Parameter::span(-0.35f, -0.1f);
    m_humidities[2] = Parameter::span(-0.1f, 0.1f);
    m_humidities[3] = Parameter::span(0.1f, 0.3f);
    m_humidities[4] = Parameter::span(0.3f, 1.0f);

    // ========== 侵蚀范围 ==========
    m_erosions[0] = Parameter::span(-1.0f, -0.78f);
    m_erosions[1] = Parameter::span(-0.78f, -0.375f);
    m_erosions[2] = Parameter::span(-0.375f, -0.2225f);
    m_erosions[3] = Parameter::span(-0.2225f, 0.05f);
    m_erosions[4] = Parameter::span(0.05f, 0.45f);
    m_erosions[5] = Parameter::span(0.45f, 0.55f);
    m_erosions[6] = Parameter::span(0.55f, 1.0f);

    // ========== 全局范围 ==========
    // MC 1.21.11: FULL_RANGE = Climate.Parameter.span(-1.0F, 1.0F)
    // 注意：Climate.Parameter.fullRange() 返回 [-2, 2]，而主世界生物群系构建器使用 [-1, 1]
    m_fullRange = Parameter::span(-1.0f, 1.0f);
    m_frozenRange = m_temperatures[0];
    m_unfrozenRange = Parameter::span(-0.45f, 1.0f);

    // ========== 大陆度范围 ==========
    m_mushroomFieldsContinentalness = Parameter::span(-1.2f, -1.05f);
    m_deepOceanContinentalness = Parameter::span(-1.05f, -0.455f);
    m_oceanContinentalness = Parameter::span(-0.455f, -0.19f);
    m_coastContinentalness = Parameter::span(-0.19f, -0.11f);
    m_inlandContinentalness = Parameter::span(-0.11f, 0.55f);
    m_nearInlandContinentalness = Parameter::span(-0.11f, 0.03f);
    m_midInlandContinentalness = Parameter::span(0.03f, 0.3f);
    m_farInlandContinentalness = Parameter::span(0.3f, 1.0f);

    // ========== 海洋生物群系 ==========
    m_oceans[0][0] = B::DeepFrozenOcean;
    m_oceans[0][1] = B::FrozenOcean;
    m_oceans[1][0] = B::DeepColdOcean;
    m_oceans[1][1] = B::ColdOcean;
    m_oceans[2][0] = B::DeepOcean;
    m_oceans[2][1] = B::Ocean;
    m_oceans[3][0] = B::DeepLukewarmOcean;
    m_oceans[3][1] = B::LukewarmOcean;
    m_oceans[4][0] = B::WarmOcean;
    m_oceans[4][1] = B::WarmOcean;

    // ========== 中部生物群系 ==========
    m_middleBiomes[0][0] = B::SnowyPlains;
    m_middleBiomes[0][1] = B::SnowyPlains;
    m_middleBiomes[0][2] = B::SnowyPlains;
    m_middleBiomes[0][3] = B::SnowyTaiga;
    m_middleBiomes[0][4] = B::Taiga;
    m_middleBiomes[1][0] = B::Plains;
    m_middleBiomes[1][1] = B::Plains;
    m_middleBiomes[1][2] = B::Forest;
    m_middleBiomes[1][3] = B::Taiga;
    m_middleBiomes[1][4] = B::OldGrowthSpruceTaiga;
    m_middleBiomes[2][0] = B::FlowerForest;
    m_middleBiomes[2][1] = B::Plains;
    m_middleBiomes[2][2] = B::Forest;
    m_middleBiomes[2][3] = B::BirchForest;
    m_middleBiomes[2][4] = B::DarkForest;
    m_middleBiomes[3][0] = B::Savanna;
    m_middleBiomes[3][1] = B::Savanna;
    m_middleBiomes[3][2] = B::Forest;
    m_middleBiomes[3][3] = B::Jungle;
    m_middleBiomes[3][4] = B::Jungle;
    m_middleBiomes[4][0] = B::Desert;
    m_middleBiomes[4][1] = B::Desert;
    m_middleBiomes[4][2] = B::Desert;
    m_middleBiomes[4][3] = B::Desert;
    m_middleBiomes[4][4] = B::Desert;

    // ========== 中部变体生物群系 ==========
    m_middleBiomesVariant[0][0] = B::IceSpikes;
    m_middleBiomesVariant[0][1] = BIOME_NULL;
    m_middleBiomesVariant[0][2] = B::SnowyTaiga;
    m_middleBiomesVariant[0][3] = BIOME_NULL;
    m_middleBiomesVariant[0][4] = BIOME_NULL;
    m_middleBiomesVariant[1][0] = BIOME_NULL;
    m_middleBiomesVariant[1][1] = BIOME_NULL;
    m_middleBiomesVariant[1][2] = BIOME_NULL;
    m_middleBiomesVariant[1][3] = BIOME_NULL;
    m_middleBiomesVariant[1][4] = B::OldGrowthPineTaiga;
    m_middleBiomesVariant[2][0] = B::SunflowerPlains;
    m_middleBiomesVariant[2][1] = BIOME_NULL;
    m_middleBiomesVariant[2][2] = BIOME_NULL;
    m_middleBiomesVariant[2][3] = B::OldGrowthBirchForest;
    m_middleBiomesVariant[2][4] = BIOME_NULL;
    m_middleBiomesVariant[3][0] = BIOME_NULL;
    m_middleBiomesVariant[3][1] = BIOME_NULL;
    m_middleBiomesVariant[3][2] = B::Plains;
    m_middleBiomesVariant[3][3] = B::SparseJungle;
    m_middleBiomesVariant[3][4] = B::BambooJungle;
    m_middleBiomesVariant[4][0] = BIOME_NULL;
    m_middleBiomesVariant[4][1] = BIOME_NULL;
    m_middleBiomesVariant[4][2] = BIOME_NULL;
    m_middleBiomesVariant[4][3] = BIOME_NULL;
    m_middleBiomesVariant[4][4] = BIOME_NULL;

    // ========== 高原生物群系 ==========
    m_plateauBiomes[0][0] = B::SnowyPlains;
    m_plateauBiomes[0][1] = B::SnowyPlains;
    m_plateauBiomes[0][2] = B::SnowyPlains;
    m_plateauBiomes[0][3] = B::SnowyTaiga;
    m_plateauBiomes[0][4] = B::SnowyTaiga;
    m_plateauBiomes[1][0] = B::Meadow;
    m_plateauBiomes[1][1] = B::Meadow;
    m_plateauBiomes[1][2] = B::Forest;
    m_plateauBiomes[1][3] = B::Taiga;
    m_plateauBiomes[1][4] = B::OldGrowthSpruceTaiga;
    m_plateauBiomes[2][0] = B::Meadow;
    m_plateauBiomes[2][1] = B::Meadow;
    m_plateauBiomes[2][2] = B::Meadow;
    m_plateauBiomes[2][3] = B::Meadow;
    m_plateauBiomes[2][4] = B::PaleGarden;
    m_plateauBiomes[3][0] = B::SavannaPlateau;
    m_plateauBiomes[3][1] = B::SavannaPlateau;
    m_plateauBiomes[3][2] = B::Forest;
    m_plateauBiomes[3][3] = B::Forest;
    m_plateauBiomes[3][4] = B::Jungle;
    m_plateauBiomes[4][0] = B::Badlands;
    m_plateauBiomes[4][1] = B::Badlands;
    m_plateauBiomes[4][2] = B::Badlands;
    m_plateauBiomes[4][3] = B::WoodedBadlands;
    m_plateauBiomes[4][4] = B::WoodedBadlands;

    // ========== 高原变体生物群系 ==========
    m_plateauBiomesVariant[0][0] = B::IceSpikes;
    m_plateauBiomesVariant[0][1] = BIOME_NULL;
    m_plateauBiomesVariant[0][2] = BIOME_NULL;
    m_plateauBiomesVariant[0][3] = BIOME_NULL;
    m_plateauBiomesVariant[0][4] = BIOME_NULL;
    m_plateauBiomesVariant[1][0] = B::CherryGrove;
    m_plateauBiomesVariant[1][1] = BIOME_NULL;
    m_plateauBiomesVariant[1][2] = B::Meadow;
    m_plateauBiomesVariant[1][3] = B::Meadow;
    m_plateauBiomesVariant[1][4] = B::OldGrowthPineTaiga;
    m_plateauBiomesVariant[2][0] = B::CherryGrove;
    m_plateauBiomesVariant[2][1] = B::CherryGrove;
    m_plateauBiomesVariant[2][2] = B::Forest;
    m_plateauBiomesVariant[2][3] = B::BirchForest;
    m_plateauBiomesVariant[2][4] = BIOME_NULL;
    m_plateauBiomesVariant[3][0] = BIOME_NULL;
    m_plateauBiomesVariant[3][1] = BIOME_NULL;
    m_plateauBiomesVariant[3][2] = BIOME_NULL;
    m_plateauBiomesVariant[3][3] = BIOME_NULL;
    m_plateauBiomesVariant[3][4] = BIOME_NULL;
    m_plateauBiomesVariant[4][0] = B::ErodedBadlands;
    m_plateauBiomesVariant[4][1] = B::ErodedBadlands;
    m_plateauBiomesVariant[4][2] = BIOME_NULL;
    m_plateauBiomesVariant[4][3] = BIOME_NULL;
    m_plateauBiomesVariant[4][4] = BIOME_NULL;

    // ========== 破碎生物群系 ==========
    m_shatteredBiomes[0][0] = B::WindsweptGravellyHills;
    m_shatteredBiomes[0][1] = B::WindsweptGravellyHills;
    m_shatteredBiomes[0][2] = B::WindsweptHills;
    m_shatteredBiomes[0][3] = B::WindsweptForest;
    m_shatteredBiomes[0][4] = B::WindsweptForest;
    m_shatteredBiomes[1][0] = B::WindsweptGravellyHills;
    m_shatteredBiomes[1][1] = B::WindsweptGravellyHills;
    m_shatteredBiomes[1][2] = B::WindsweptHills;
    m_shatteredBiomes[1][3] = B::WindsweptForest;
    m_shatteredBiomes[1][4] = B::WindsweptForest;
    m_shatteredBiomes[2][0] = B::WindsweptHills;
    m_shatteredBiomes[2][1] = B::WindsweptHills;
    m_shatteredBiomes[2][2] = B::WindsweptHills;
    m_shatteredBiomes[2][3] = B::WindsweptForest;
    m_shatteredBiomes[2][4] = B::WindsweptForest;
    m_shatteredBiomes[3][0] = BIOME_NULL;
    m_shatteredBiomes[3][1] = BIOME_NULL;
    m_shatteredBiomes[3][2] = BIOME_NULL;
    m_shatteredBiomes[3][3] = BIOME_NULL;
    m_shatteredBiomes[3][4] = BIOME_NULL;
    m_shatteredBiomes[4][0] = BIOME_NULL;
    m_shatteredBiomes[4][1] = BIOME_NULL;
    m_shatteredBiomes[4][2] = BIOME_NULL;
    m_shatteredBiomes[4][3] = BIOME_NULL;
    m_shatteredBiomes[4][4] = BIOME_NULL;
}

// ============================================================================
// 生物群系选择方法
// ============================================================================

BiomeId OverworldBiomeBuilder::pickMiddleBiome(i32 temperature, i32 humidity, const Parameter& weirdness) const
{
    if (weirdness.max < 0) {
        return m_middleBiomes[temperature][humidity];
    }
    const BiomeId variant = m_middleBiomesVariant[temperature][humidity];
    return variant != BIOME_NULL ? variant : m_middleBiomes[temperature][humidity];
}

BiomeId OverworldBiomeBuilder::pickPlateauBiome(i32 temperature, i32 humidity, const Parameter& weirdness) const
{
    if (weirdness.max >= 0) {
        const BiomeId variant = m_plateauBiomesVariant[temperature][humidity];
        if (variant != BIOME_NULL) {
            return variant;
        }
    }
    return m_plateauBiomes[temperature][humidity];
}

BiomeId OverworldBiomeBuilder::pickPeakBiome(i32 temperature, i32 humidity, const Parameter& weirdness) const
{
    MC_UNUSED(humidity);
    if (temperature <= 2) {
        return weirdness.max < 0 ? B::JaggedPeaks : B::FrozenPeaks;
    }
    if (temperature == 3) {
        return B::StonyPeaks;
    }
    return pickBadlandsBiome(humidity, weirdness);
}

BiomeId OverworldBiomeBuilder::pickSlopeBiome(i32 temperature, i32 humidity, const Parameter& weirdness) const
{
    if (temperature >= 3) {
        return pickPlateauBiome(temperature, humidity, weirdness);
    }
    return humidity <= 1 ? B::SnowySlopes : B::Grove;
}

BiomeId OverworldBiomeBuilder::pickBadlandsBiome(i32 humidity, const Parameter& weirdness) const
{
    if (humidity < 2) {
        return weirdness.max < 0 ? B::Badlands : B::ErodedBadlands;
    }
    if (humidity < 3) {
        return B::Badlands;
    }
    return B::WoodedBadlands;
}

BiomeId OverworldBiomeBuilder::pickBeachBiome(i32 temperature) const
{
    if (temperature == 0) {
        return B::SnowyBeach;
    }
    if (temperature == 4) {
        return B::Desert;
    }
    return B::Beach;
}

BiomeId OverworldBiomeBuilder::pickShatteredBiome(i32 temperature, i32 humidity, const Parameter& weirdness) const
{
    const BiomeId shattered = m_shatteredBiomes[temperature][humidity];
    return shattered != BIOME_NULL ? shattered : pickMiddleBiome(temperature, humidity, weirdness);
}

BiomeId OverworldBiomeBuilder::pickMiddleBiomeOrBadlandsIfHot(
    i32 temperature, i32 humidity, const Parameter& weirdness) const
{
    return temperature == 4 ? pickBadlandsBiome(humidity, weirdness)
                            : pickMiddleBiome(temperature, humidity, weirdness);
}

BiomeId OverworldBiomeBuilder::pickMiddleBiomeOrBadlandsIfHotOrSlopeIfCold(
    i32 temperature, i32 humidity, const Parameter& weirdness) const
{
    return temperature == 0 ? pickSlopeBiome(temperature, humidity, weirdness)
                            : pickMiddleBiomeOrBadlandsIfHot(temperature, humidity, weirdness);
}

BiomeId OverworldBiomeBuilder::maybePickWindsweptSavannaBiome(
    i32 temperature, i32 humidity, const Parameter& weirdness, BiomeId defaultBiome) const
{
    MC_UNUSED(weirdness);
    return (temperature > 1 && humidity < 4 && weirdness.max >= 0) ? B::WindsweptSavanna : defaultBiome;
}

BiomeId OverworldBiomeBuilder::pickShatteredCoastBiome(i32 temperature, i32 humidity, const Parameter& weirdness) const
{
    const BiomeId biome =
        weirdness.max >= 0 ? pickMiddleBiome(temperature, humidity, weirdness) : pickBeachBiome(temperature);
    return maybePickWindsweptSavannaBiome(temperature, humidity, weirdness, biome);
}

// ============================================================================
// 参数添加辅助方法
// ============================================================================

void OverworldBiomeBuilder::addSurfaceBiome(std::vector<ParameterList<BiomeId>::Entry>& entries,
    const Parameter& temperature,
    const Parameter& humidity,
    const Parameter& continentalness,
    const Parameter& erosion,
    const Parameter& weirdness,
    f32 offset,
    BiomeId biome) const
{
    const i64 qOffset = quantizeCoord(offset);
    // depth=0 和 depth=1 两个参数点
    entries.emplace_back(
        ParameterPoint{temperature, humidity, continentalness, erosion, Parameter::point(0.0f), weirdness, qOffset},
        biome);
    entries.emplace_back(
        ParameterPoint{temperature, humidity, continentalness, erosion, Parameter::point(1.0f), weirdness, qOffset},
        biome);
}

void OverworldBiomeBuilder::addUndergroundBiome(std::vector<ParameterList<BiomeId>::Entry>& entries,
    const Parameter& temperature,
    const Parameter& humidity,
    const Parameter& continentalness,
    const Parameter& erosion,
    const Parameter& weirdness,
    f32 offset,
    BiomeId biome) const
{
    entries.emplace_back(ParameterPoint{temperature,
                             humidity,
                             continentalness,
                             erosion,
                             Parameter::span(0.2f, 0.9f),
                             weirdness,
                             quantizeCoord(offset)},
        biome);
}

void OverworldBiomeBuilder::addBottomBiome(std::vector<ParameterList<BiomeId>::Entry>& entries,
    const Parameter& temperature,
    const Parameter& humidity,
    const Parameter& continentalness,
    const Parameter& erosion,
    const Parameter& weirdness,
    f32 offset,
    BiomeId biome) const
{
    entries.emplace_back(
        ParameterPoint{
            temperature, humidity, continentalness, erosion, Parameter::point(1.1f), weirdness, quantizeCoord(offset)},
        biome);
}

// ============================================================================
// 近海生物群系
// ============================================================================

void OverworldBiomeBuilder::addOffCoastBiomes(std::vector<ParameterList<BiomeId>::Entry>& entries) const
{
    addSurfaceBiome(entries,
        m_fullRange,
        m_fullRange,
        m_mushroomFieldsContinentalness,
        m_fullRange,
        m_fullRange,
        0.0f,
        B::MushroomFields);

    for (i32 i = 0; i < 5; ++i) {
        addSurfaceBiome(entries,
            m_temperatures[i],
            m_fullRange,
            m_deepOceanContinentalness,
            m_fullRange,
            m_fullRange,
            0.0f,
            m_oceans[i][0]);
        addSurfaceBiome(entries,
            m_temperatures[i],
            m_fullRange,
            m_oceanContinentalness,
            m_fullRange,
            m_fullRange,
            0.0f,
            m_oceans[i][1]);
    }
}

// ============================================================================
// 内陆生物群系（13 个奇异度切片）
// ============================================================================

void OverworldBiomeBuilder::addInlandBiomes(std::vector<ParameterList<BiomeId>::Entry>& entries) const
{
    addMidSlice(entries, Parameter::span(-1.0f, -0.93333334f));
    addHighSlice(entries, Parameter::span(-0.93333334f, -0.7666667f));
    addPeaks(entries, Parameter::span(-0.7666667f, -0.56666666f));
    addHighSlice(entries, Parameter::span(-0.56666666f, -0.4f));
    addMidSlice(entries, Parameter::span(-0.4f, -0.26666668f));
    addLowSlice(entries, Parameter::span(-0.26666668f, -0.05f));
    addValleys(entries, Parameter::span(-0.05f, 0.05f));
    addLowSlice(entries, Parameter::span(0.05f, 0.26666668f));
    addMidSlice(entries, Parameter::span(0.26666668f, 0.4f));
    addHighSlice(entries, Parameter::span(0.4f, 0.56666666f));
    addPeaks(entries, Parameter::span(0.56666666f, 0.7666667f));
    addHighSlice(entries, Parameter::span(0.7666667f, 0.93333334f));
    addMidSlice(entries, Parameter::span(0.93333334f, 1.0f));
}

// ============================================================================
// 中间切片
// ============================================================================

void OverworldBiomeBuilder::addMidSlice(
    std::vector<ParameterList<BiomeId>::Entry>& entries, const Parameter& weirdness) const
{
    // 石岸
    addSurfaceBiome(entries,
        m_fullRange,
        m_fullRange,
        m_coastContinentalness,
        Parameter::span(m_erosions[0], m_erosions[2]),
        weirdness,
        0.0f,
        B::StonyShore);

    // 沼泽（冷-温和温度，低侵蚀）
    addSurfaceBiome(entries,
        Parameter::span(m_temperatures[1], m_temperatures[2]),
        m_fullRange,
        Parameter::span(m_nearInlandContinentalness, m_farInlandContinentalness),
        m_erosions[6],
        weirdness,
        0.0f,
        B::Swamp);

    // 红树林沼泽（暖-热温度，低侵蚀）
    addSurfaceBiome(entries,
        Parameter::span(m_temperatures[3], m_temperatures[4]),
        m_fullRange,
        Parameter::span(m_nearInlandContinentalness, m_farInlandContinentalness),
        m_erosions[6],
        weirdness,
        0.0f,
        B::MangroveSwamp);

    for (i32 i = 0; i < 5; ++i) {
        const Parameter& temp = m_temperatures[i];
        for (i32 j = 0; j < 5; ++j) {
            const Parameter& humid = m_humidities[j];
            const BiomeId midBiome = pickMiddleBiome(i, j, weirdness);
            const BiomeId midOrBadlands = pickMiddleBiomeOrBadlandsIfHot(i, j, weirdness);
            const BiomeId midOrBadlandsOrSlope = pickMiddleBiomeOrBadlandsIfHotOrSlopeIfCold(i, j, weirdness);
            const BiomeId shattered = pickShatteredBiome(i, j, weirdness);
            const BiomeId plateau = pickPlateauBiome(i, j, weirdness);
            const BiomeId beach = pickBeachBiome(i);
            const BiomeId windsweptSavanna = maybePickWindsweptSavannaBiome(i, j, weirdness, midBiome);
            const BiomeId shatteredCoast = pickShatteredCoastBiome(i, j, weirdness);
            const BiomeId slope = pickSlopeBiome(i, j, weirdness);

            // 侵蚀 0（最抗侵蚀）：山坡/山峰
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_nearInlandContinentalness, m_farInlandContinentalness),
                m_erosions[0],
                weirdness,
                0.0f,
                slope);

            // 侵蚀 1，近-中内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_nearInlandContinentalness, m_midInlandContinentalness),
                m_erosions[1],
                weirdness,
                0.0f,
                midOrBadlandsOrSlope);

            // 侵蚀 1，远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                m_farInlandContinentalness,
                m_erosions[1],
                weirdness,
                0.0f,
                i == 0 ? slope : plateau);

            // 侵蚀 2，近内陆
            addSurfaceBiome(
                entries, temp, humid, m_nearInlandContinentalness, m_erosions[2], weirdness, 0.0f, midBiome);

            // 侵蚀 2，中内陆
            addSurfaceBiome(
                entries, temp, humid, m_midInlandContinentalness, m_erosions[2], weirdness, 0.0f, midOrBadlands);

            // 侵蚀 2，远内陆
            addSurfaceBiome(entries, temp, humid, m_farInlandContinentalness, m_erosions[2], weirdness, 0.0f, plateau);

            // 侵蚀 3，海岸-近内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_coastContinentalness, m_nearInlandContinentalness),
                m_erosions[3],
                weirdness,
                0.0f,
                midBiome);

            // 侵蚀 3，中-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                m_erosions[3],
                weirdness,
                0.0f,
                midOrBadlands);

            // 侵蚀 4：根据奇异度符号决定海岸/内陆
            if (weirdness.max < 0) {
                addSurfaceBiome(entries, temp, humid, m_coastContinentalness, m_erosions[4], weirdness, 0.0f, beach);
                addSurfaceBiome(entries,
                    temp,
                    humid,
                    Parameter::span(m_nearInlandContinentalness, m_farInlandContinentalness),
                    m_erosions[4],
                    weirdness,
                    0.0f,
                    midBiome);
            } else {
                addSurfaceBiome(entries,
                    temp,
                    humid,
                    Parameter::span(m_coastContinentalness, m_farInlandContinentalness),
                    m_erosions[4],
                    weirdness,
                    0.0f,
                    midBiome);
            }

            // 侵蚀 5
            addSurfaceBiome(
                entries, temp, humid, m_coastContinentalness, m_erosions[5], weirdness, 0.0f, shatteredCoast);
            addSurfaceBiome(
                entries, temp, humid, m_nearInlandContinentalness, m_erosions[5], weirdness, 0.0f, windsweptSavanna);
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                m_erosions[5],
                weirdness,
                0.0f,
                shattered);

            // 侵蚀 6：海岸
            if (weirdness.max < 0) {
                addSurfaceBiome(entries, temp, humid, m_coastContinentalness, m_erosions[6], weirdness, 0.0f, beach);
            } else {
                addSurfaceBiome(entries, temp, humid, m_coastContinentalness, m_erosions[6], weirdness, 0.0f, midBiome);
            }

            // 侵蚀 6：冰冻温度的内陆
            if (i == 0) {
                addSurfaceBiome(entries,
                    temp,
                    humid,
                    Parameter::span(m_nearInlandContinentalness, m_farInlandContinentalness),
                    m_erosions[6],
                    weirdness,
                    0.0f,
                    midBiome);
            }
        }
    }
}

// ============================================================================
// 高切片
// ============================================================================

void OverworldBiomeBuilder::addHighSlice(
    std::vector<ParameterList<BiomeId>::Entry>& entries, const Parameter& weirdness) const
{
    for (i32 i = 0; i < 5; ++i) {
        const Parameter& temp = m_temperatures[i];
        for (i32 j = 0; j < 5; ++j) {
            const Parameter& humid = m_humidities[j];
            const BiomeId midBiome = pickMiddleBiome(i, j, weirdness);
            const BiomeId midOrBadlands = pickMiddleBiomeOrBadlandsIfHot(i, j, weirdness);
            const BiomeId midOrBadlandsOrSlope = pickMiddleBiomeOrBadlandsIfHotOrSlopeIfCold(i, j, weirdness);
            const BiomeId plateau = pickPlateauBiome(i, j, weirdness);
            const BiomeId shattered = pickShatteredBiome(i, j, weirdness);
            const BiomeId windsweptSavanna = maybePickWindsweptSavannaBiome(i, j, weirdness, midBiome);
            const BiomeId slope = pickSlopeBiome(i, j, weirdness);
            const BiomeId peak = pickPeakBiome(i, j, weirdness);

            // 侵蚀 0-1，海岸
            addSurfaceBiome(entries,
                temp,
                humid,
                m_coastContinentalness,
                Parameter::span(m_erosions[0], m_erosions[1]),
                weirdness,
                0.0f,
                midBiome);

            // 侵蚀 0，近内陆
            addSurfaceBiome(entries, temp, humid, m_nearInlandContinentalness, m_erosions[0], weirdness, 0.0f, slope);

            // 侵蚀 0，中-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                m_erosions[0],
                weirdness,
                0.0f,
                peak);

            // 侵蚀 1，近内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                m_nearInlandContinentalness,
                m_erosions[1],
                weirdness,
                0.0f,
                midOrBadlandsOrSlope);

            // 侵蚀 1，中-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                m_erosions[1],
                weirdness,
                0.0f,
                slope);

            // 侵蚀 2-3，海岸-近内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_coastContinentalness, m_nearInlandContinentalness),
                Parameter::span(m_erosions[2], m_erosions[3]),
                weirdness,
                0.0f,
                midBiome);

            // 侵蚀 2，中-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                m_erosions[2],
                weirdness,
                0.0f,
                plateau);

            // 侵蚀 3，中内陆
            addSurfaceBiome(
                entries, temp, humid, m_midInlandContinentalness, m_erosions[3], weirdness, 0.0f, midOrBadlands);

            // 侵蚀 3，远内陆
            addSurfaceBiome(entries, temp, humid, m_farInlandContinentalness, m_erosions[3], weirdness, 0.0f, plateau);

            // 侵蚀 4
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_coastContinentalness, m_farInlandContinentalness),
                m_erosions[4],
                weirdness,
                0.0f,
                midBiome);

            // 侵蚀 5，海岸-近内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_coastContinentalness, m_nearInlandContinentalness),
                m_erosions[5],
                weirdness,
                0.0f,
                windsweptSavanna);

            // 侵蚀 5，中-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                m_erosions[5],
                weirdness,
                0.0f,
                shattered);

            // 侵蚀 6
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_coastContinentalness, m_farInlandContinentalness),
                m_erosions[6],
                weirdness,
                0.0f,
                midBiome);
        }
    }
}

// ============================================================================
// 山峰切片
// ============================================================================

void OverworldBiomeBuilder::addPeaks(
    std::vector<ParameterList<BiomeId>::Entry>& entries, const Parameter& weirdness) const
{
    for (i32 i = 0; i < 5; ++i) {
        const Parameter& temp = m_temperatures[i];
        for (i32 j = 0; j < 5; ++j) {
            const Parameter& humid = m_humidities[j];
            const BiomeId midBiome = pickMiddleBiome(i, j, weirdness);
            const BiomeId midOrBadlands = pickMiddleBiomeOrBadlandsIfHot(i, j, weirdness);
            const BiomeId midOrBadlandsOrSlope = pickMiddleBiomeOrBadlandsIfHotOrSlopeIfCold(i, j, weirdness);
            const BiomeId plateau = pickPlateauBiome(i, j, weirdness);
            const BiomeId shattered = pickShatteredBiome(i, j, weirdness);
            const BiomeId windsweptSavanna = maybePickWindsweptSavannaBiome(i, j, weirdness, shattered);
            const BiomeId peak = pickPeakBiome(i, j, weirdness);

            // 侵蚀 0
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_coastContinentalness, m_farInlandContinentalness),
                m_erosions[0],
                weirdness,
                0.0f,
                peak);

            // 侵蚀 1，海岸-近内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_coastContinentalness, m_nearInlandContinentalness),
                m_erosions[1],
                weirdness,
                0.0f,
                midOrBadlandsOrSlope);

            // 侵蚀 1，中-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                m_erosions[1],
                weirdness,
                0.0f,
                peak);

            // 侵蚀 2-3，海岸-近内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_coastContinentalness, m_nearInlandContinentalness),
                Parameter::span(m_erosions[2], m_erosions[3]),
                weirdness,
                0.0f,
                midBiome);

            // 侵蚀 2，中-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                m_erosions[2],
                weirdness,
                0.0f,
                plateau);

            // 侵蚀 3，中内陆
            addSurfaceBiome(
                entries, temp, humid, m_midInlandContinentalness, m_erosions[3], weirdness, 0.0f, midOrBadlands);

            // 侵蚀 3，远内陆
            addSurfaceBiome(entries, temp, humid, m_farInlandContinentalness, m_erosions[3], weirdness, 0.0f, plateau);

            // 侵蚀 4
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_coastContinentalness, m_farInlandContinentalness),
                m_erosions[4],
                weirdness,
                0.0f,
                midBiome);

            // 侵蚀 5，海岸-近内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_coastContinentalness, m_nearInlandContinentalness),
                m_erosions[5],
                weirdness,
                0.0f,
                windsweptSavanna);

            // 侵蚀 5，中-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                m_erosions[5],
                weirdness,
                0.0f,
                shattered);

            // 侵蚀 6
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_coastContinentalness, m_farInlandContinentalness),
                m_erosions[6],
                weirdness,
                0.0f,
                midBiome);
        }
    }
}

// ============================================================================
// 低切片
// ============================================================================

void OverworldBiomeBuilder::addLowSlice(
    std::vector<ParameterList<BiomeId>::Entry>& entries, const Parameter& weirdness) const
{
    // 石岸
    addSurfaceBiome(entries,
        m_fullRange,
        m_fullRange,
        m_coastContinentalness,
        Parameter::span(m_erosions[0], m_erosions[2]),
        weirdness,
        0.0f,
        B::StonyShore);

    // 沼泽（冷-温和温度）
    addSurfaceBiome(entries,
        Parameter::span(m_temperatures[1], m_temperatures[2]),
        m_fullRange,
        Parameter::span(m_nearInlandContinentalness, m_farInlandContinentalness),
        m_erosions[6],
        weirdness,
        0.0f,
        B::Swamp);

    // 红树林沼泽（暖-热温度）
    addSurfaceBiome(entries,
        Parameter::span(m_temperatures[3], m_temperatures[4]),
        m_fullRange,
        Parameter::span(m_nearInlandContinentalness, m_farInlandContinentalness),
        m_erosions[6],
        weirdness,
        0.0f,
        B::MangroveSwamp);

    for (i32 i = 0; i < 5; ++i) {
        const Parameter& temp = m_temperatures[i];
        for (i32 j = 0; j < 5; ++j) {
            const Parameter& humid = m_humidities[j];
            const BiomeId midBiome = pickMiddleBiome(i, j, weirdness);
            const BiomeId midOrBadlands = pickMiddleBiomeOrBadlandsIfHot(i, j, weirdness);
            const BiomeId midOrBadlandsOrSlope = pickMiddleBiomeOrBadlandsIfHotOrSlopeIfCold(i, j, weirdness);
            const BiomeId beach = pickBeachBiome(i);
            const BiomeId windsweptSavanna = maybePickWindsweptSavannaBiome(i, j, weirdness, midBiome);
            const BiomeId shatteredCoast = pickShatteredCoastBiome(i, j, weirdness);

            // 侵蚀 0-1，近内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                m_nearInlandContinentalness,
                Parameter::span(m_erosions[0], m_erosions[1]),
                weirdness,
                0.0f,
                midOrBadlands);

            // 侵蚀 0-1，中-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                Parameter::span(m_erosions[0], m_erosions[1]),
                weirdness,
                0.0f,
                midOrBadlandsOrSlope);

            // 侵蚀 2-3，近内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                m_nearInlandContinentalness,
                Parameter::span(m_erosions[2], m_erosions[3]),
                weirdness,
                0.0f,
                midBiome);

            // 侵蚀 2-3，中-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                Parameter::span(m_erosions[2], m_erosions[3]),
                weirdness,
                0.0f,
                midOrBadlands);

            // 侵蚀 3-4，海岸
            addSurfaceBiome(entries,
                temp,
                humid,
                m_coastContinentalness,
                Parameter::span(m_erosions[3], m_erosions[4]),
                weirdness,
                0.0f,
                beach);

            // 侵蚀 4，近-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_nearInlandContinentalness, m_farInlandContinentalness),
                m_erosions[4],
                weirdness,
                0.0f,
                midBiome);

            // 侵蚀 5，海岸
            addSurfaceBiome(
                entries, temp, humid, m_coastContinentalness, m_erosions[5], weirdness, 0.0f, shatteredCoast);

            // 侵蚀 5，近内陆
            addSurfaceBiome(
                entries, temp, humid, m_nearInlandContinentalness, m_erosions[5], weirdness, 0.0f, windsweptSavanna);

            // 侵蚀 5，中-远内陆
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                m_erosions[5],
                weirdness,
                0.0f,
                midBiome);

            // 侵蚀 6，海岸
            addSurfaceBiome(entries, temp, humid, m_coastContinentalness, m_erosions[6], weirdness, 0.0f, beach);

            // 侵蚀 6，冰冻温度的内陆
            if (i == 0) {
                addSurfaceBiome(entries,
                    temp,
                    humid,
                    Parameter::span(m_nearInlandContinentalness, m_farInlandContinentalness),
                    m_erosions[6],
                    weirdness,
                    0.0f,
                    midBiome);
            }
        }
    }
}

// ============================================================================
// 山谷切片（河流、冻河、沼泽）
// ============================================================================

void OverworldBiomeBuilder::addValleys(
    std::vector<ParameterList<BiomeId>::Entry>& entries, const Parameter& weirdness) const
{
    // 冻河/石岸（冰冻温度，低侵蚀，海岸）
    addSurfaceBiome(entries,
        m_frozenRange,
        m_fullRange,
        m_coastContinentalness,
        Parameter::span(m_erosions[0], m_erosions[1]),
        weirdness,
        0.0f,
        weirdness.max < 0 ? B::StonyShore : B::FrozenRiver);

    // 河流/石岸（非冰冻温度，低侵蚀，海岸）
    addSurfaceBiome(entries,
        m_unfrozenRange,
        m_fullRange,
        m_coastContinentalness,
        Parameter::span(m_erosions[0], m_erosions[1]),
        weirdness,
        0.0f,
        weirdness.max < 0 ? B::StonyShore : B::River);

    // 冻河（冰冻温度，低侵蚀，近内陆）
    addSurfaceBiome(entries,
        m_frozenRange,
        m_fullRange,
        m_nearInlandContinentalness,
        Parameter::span(m_erosions[0], m_erosions[1]),
        weirdness,
        0.0f,
        B::FrozenRiver);

    // 河流（非冰冻温度，低侵蚀，近内陆）
    addSurfaceBiome(entries,
        m_unfrozenRange,
        m_fullRange,
        m_nearInlandContinentalness,
        Parameter::span(m_erosions[0], m_erosions[1]),
        weirdness,
        0.0f,
        B::River);

    // 冻河（冰冻温度，中侵蚀，海岸-远内陆）
    addSurfaceBiome(entries,
        m_frozenRange,
        m_fullRange,
        Parameter::span(m_coastContinentalness, m_farInlandContinentalness),
        Parameter::span(m_erosions[2], m_erosions[5]),
        weirdness,
        0.0f,
        B::FrozenRiver);

    // 河流（非冰冻温度，中侵蚀，海岸-远内陆）
    addSurfaceBiome(entries,
        m_unfrozenRange,
        m_fullRange,
        Parameter::span(m_coastContinentalness, m_farInlandContinentalness),
        Parameter::span(m_erosions[2], m_erosions[5]),
        weirdness,
        0.0f,
        B::River);

    // 冻河（冰冻温度，高侵蚀，海岸）
    addSurfaceBiome(
        entries, m_frozenRange, m_fullRange, m_coastContinentalness, m_erosions[6], weirdness, 0.0f, B::FrozenRiver);

    // 河流（非冰冻温度，高侵蚀，海岸）
    addSurfaceBiome(
        entries, m_unfrozenRange, m_fullRange, m_coastContinentalness, m_erosions[6], weirdness, 0.0f, B::River);

    // 沼泽（冷-温和温度，高侵蚀，内陆）
    addSurfaceBiome(entries,
        Parameter::span(m_temperatures[1], m_temperatures[2]),
        m_fullRange,
        Parameter::span(m_inlandContinentalness, m_farInlandContinentalness),
        m_erosions[6],
        weirdness,
        0.0f,
        B::Swamp);

    // 红树林沼泽（暖-热温度，高侵蚀，内陆）
    addSurfaceBiome(entries,
        Parameter::span(m_temperatures[3], m_temperatures[4]),
        m_fullRange,
        Parameter::span(m_inlandContinentalness, m_farInlandContinentalness),
        m_erosions[6],
        weirdness,
        0.0f,
        B::MangroveSwamp);

    // 冻河（冰冻温度，高侵蚀，内陆）
    addSurfaceBiome(entries,
        m_frozenRange,
        m_fullRange,
        Parameter::span(m_inlandContinentalness, m_farInlandContinentalness),
        m_erosions[6],
        weirdness,
        0.0f,
        B::FrozenRiver);

    // 低侵蚀内陆（所有温度×湿度）
    for (i32 i = 0; i < 5; ++i) {
        const Parameter& temp = m_temperatures[i];
        for (i32 j = 0; j < 5; ++j) {
            const Parameter& humid = m_humidities[j];
            const BiomeId midOrBadlands = pickMiddleBiomeOrBadlandsIfHot(i, j, weirdness);
            addSurfaceBiome(entries,
                temp,
                humid,
                Parameter::span(m_midInlandContinentalness, m_farInlandContinentalness),
                Parameter::span(m_erosions[0], m_erosions[1]),
                weirdness,
                0.0f,
                midOrBadlands);
        }
    }
}

// ============================================================================
// 地下生物群系
// ============================================================================

void OverworldBiomeBuilder::addUndergroundBiomesEntries(std::vector<ParameterList<BiomeId>::Entry>& entries) const
{
    // 滴水石洞：高大陆度
    addUndergroundBiome(entries,
        m_fullRange,
        m_fullRange,
        Parameter::span(0.8f, 1.0f),
        m_fullRange,
        m_fullRange,
        0.0f,
        B::DripstoneCaves);

    // 繁茂洞穴：高湿度
    addUndergroundBiome(
        entries, m_fullRange, Parameter::span(0.7f, 1.0f), m_fullRange, m_fullRange, m_fullRange, 0.0f, B::LushCaves);

    // 深暗之域：低侵蚀 + 最深层
    addBottomBiome(entries,
        m_fullRange,
        m_fullRange,
        m_fullRange,
        Parameter::span(m_erosions[0], m_erosions[1]),
        m_fullRange,
        0.0f,
        B::DeepDark);
}

// ============================================================================
// 构建参数列表
// ============================================================================

ParameterList<BiomeId> OverworldBiomeBuilder::buildParameterList() const
{
    std::vector<ParameterList<BiomeId>::Entry> entries;

    addOffCoastBiomes(entries);
    addInlandBiomes(entries);
    addUndergroundBiomesEntries(entries);

    return ParameterList<BiomeId>(std::move(entries));
}

std::vector<ParameterPoint> OverworldBiomeBuilder::spawnTarget() const
{
    // MC 1.21.11: OverworldBiomeBuilder.spawnTarget()
    // 返回 2 个 ParameterPoint：
    //   - temperature/humidity/erosion = FULL_RANGE [-1, 1]
    //   - continentalness = span(inlandContinentalness, FULL_RANGE)
    //     = span([-0.11, 0.55], [-1, 1]) = [-0.11, 1]
    //     （表示「内陆及远内陆」大陆度，确保出生点不在海洋/海岸）
    //   - depth = point(0.0F)  （出生点在地表）
    //   - weirdness: 分别为 span(-1, -0.16) 与 span(0.16, 1)
    //     （覆盖非河谷/非奇异的主地形与变体）
    //   - offset = 0
    const Parameter depth = Parameter::point(0.0f);
    const Parameter continentalness = Parameter::span(m_inlandContinentalness, m_fullRange);
    constexpr f32 weirdnessSplit = 0.16f;

    return std::vector<ParameterPoint>{
        ParameterPoint{
            m_fullRange,                             // temperature
            m_fullRange,                             // humidity
            continentalness,                         // continentalness
            m_fullRange,                             // erosion
            depth,                                   // depth
            Parameter::span(-1.0f, -weirdnessSplit), // weirdness
            0                                        // offset
        },
        ParameterPoint{
            m_fullRange,                           // temperature
            m_fullRange,                           // humidity
            continentalness,                       // continentalness
            m_fullRange,                           // erosion
            depth,                                 // depth
            Parameter::span(weirdnessSplit, 1.0f), // weirdness
            0                                      // offset
        },
    };
}

} // namespace mc::world::biome::source
