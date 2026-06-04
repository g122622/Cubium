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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "common/world/biome/source/OverworldBiomeBuilder.hpp"
#include "common/world/biome/climate/Climate.hpp"

namespace mc::world::biome::source {

using namespace climate;

// ============================================================================
// 生物群系 ID 别名（来自 Biomes 命名空间）
// ============================================================================

namespace B = Biomes;

// 无效/空生物群系标记
static constexpr BiomeId BIOME_NULL = static_cast<BiomeId>(-1);

// ============================================================================
// 构造函数
// ============================================================================

OverworldBiomeBuilder::OverworldBiomeBuilder()
{
    // ========== 温度范围 ==========
    m_temperatures[0] = Parameter::span(-1.0f, -0.45f);  // 冰冻
    m_temperatures[1] = Parameter::span(-0.45f, -0.15f); // 冷
    m_temperatures[2] = Parameter::span(-0.15f, 0.2f);   // 温和
    m_temperatures[3] = Parameter::span(0.2f, 0.55f);    // 暖
    m_temperatures[4] = Parameter::span(0.55f, 1.0f);    // 热

    // ========== 湿度范围 ==========
    m_humidities[0] = Parameter::span(-1.0f, -0.35f); // 干旱
    m_humidities[1] = Parameter::span(-0.35f, -0.1f); // 干燥
    m_humidities[2] = Parameter::span(-0.1f, 0.1f);   // 中性
    m_humidities[3] = Parameter::span(0.1f, 0.3f);    // 湿润
    m_humidities[4] = Parameter::span(0.3f, 1.0f);    // 潮湿

    // ========== 侵蚀范围 ==========
    m_erosions[0] = Parameter::span(-1.0f, -0.78f);
    m_erosions[1] = Parameter::span(-0.78f, -0.375f);
    m_erosions[2] = Parameter::span(-0.375f, -0.2225f);
    m_erosions[3] = Parameter::span(-0.2225f, 0.05f);
    m_erosions[4] = Parameter::span(0.05f, 0.45f);
    m_erosions[5] = Parameter::span(0.45f, 0.55f);
    m_erosions[6] = Parameter::span(0.55f, 1.0f);

    // ========== 大陆度范围 ==========
    m_mushroomFieldsContinentalness = Parameter::span(-1.2f, -1.05f);
    m_deepOceanContinentalness = Parameter::span(-1.05f, -0.455f);
    m_oceanContinentalness = Parameter::span(-0.455f, -0.19f);
    m_coastContinentalness = Parameter::span(-0.19f, -0.11f);
    m_nearInlandContinentalness = Parameter::span(-0.11f, 0.03f);
    m_midInlandContinentalness = Parameter::span(0.03f, 0.3f);
    m_farInlandContinentalness = Parameter::span(0.3f, 1.0f);

    // ========== 奇异度切片范围 ==========
    // 使用 peaksAndValleys 变换后的值
    m_valleyWeirdness = Parameter::span(-0.05f, 0.05f);
    m_lowWeirdness = Parameter::span(-0.267f, -0.05f);   // 负侧
    m_midWeirdness = Parameter::span(-0.4f, -0.267f);    // 负侧
    m_highWeirdness = Parameter::span(-0.567f, -0.4f);   // 负侧
    m_peakWeirdness = Parameter::span(-0.767f, -0.567f); // 负侧

    // ========== 深度范围 ==========
    m_surfaceDepth = Parameter::span(-1.0f, 0.0f);    // 表面
    m_undergroundDepth = Parameter::span(0.2f, 0.9f); // 地下

    // ========== 海洋生物群系 ==========
    // [温度索引][深浅: 0=深海, 1=浅海]
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
    // [温度][湿度]
    // 冰冻
    m_middleBiomes[0][0] = B::SnowyPlains;
    m_middleBiomes[0][1] = B::SnowyPlains;
    m_middleBiomes[0][2] = B::SnowyPlains;
    m_middleBiomes[0][3] = B::SnowyTaiga;
    m_middleBiomes[0][4] = B::Taiga;
    // 冷
    m_middleBiomes[1][0] = B::Plains;
    m_middleBiomes[1][1] = B::Plains;
    m_middleBiomes[1][2] = B::Forest;
    m_middleBiomes[1][3] = B::Taiga;
    m_middleBiomes[1][4] = B::OldGrowthSpruceTaiga;
    // 温和
    m_middleBiomes[2][0] = B::FlowerForest;
    m_middleBiomes[2][1] = B::Plains;
    m_middleBiomes[2][2] = B::Forest;
    m_middleBiomes[2][3] = B::BirchForest;
    m_middleBiomes[2][4] = B::DarkForest;
    // 暖
    m_middleBiomes[3][0] = B::Savanna;
    m_middleBiomes[3][1] = B::Savanna;
    m_middleBiomes[3][2] = B::Forest;
    m_middleBiomes[3][3] = B::Jungle;
    m_middleBiomes[3][4] = B::Jungle;
    // 热
    m_middleBiomes[4][0] = B::Desert;
    m_middleBiomes[4][1] = B::Desert;
    m_middleBiomes[4][2] = B::Desert;
    m_middleBiomes[4][3] = B::Desert;
    m_middleBiomes[4][4] = B::Desert;

    // ========== 中部变体生物群系（奇异度>=0时使用）==========
    // 冰冻
    m_middleBiomesVariant[0][0] = B::IceSpikes;
    m_middleBiomesVariant[0][1] = BIOME_NULL;
    m_middleBiomesVariant[0][2] = BIOME_NULL;
    m_middleBiomesVariant[0][3] = B::SnowyTaiga;
    m_middleBiomesVariant[0][4] = BIOME_NULL;
    // 冷
    m_middleBiomesVariant[1][0] = BIOME_NULL;
    m_middleBiomesVariant[1][1] = BIOME_NULL;
    m_middleBiomesVariant[1][2] = BIOME_NULL;
    m_middleBiomesVariant[1][3] = BIOME_NULL;
    m_middleBiomesVariant[1][4] = B::OldGrowthPineTaiga;
    // 温和
    m_middleBiomesVariant[2][0] = B::SunflowerPlains;
    m_middleBiomesVariant[2][1] = BIOME_NULL;
    m_middleBiomesVariant[2][2] = BIOME_NULL;
    m_middleBiomesVariant[2][3] = B::OldGrowthBirchForest;
    m_middleBiomesVariant[2][4] = BIOME_NULL;
    // 暖
    m_middleBiomesVariant[3][0] = BIOME_NULL;
    m_middleBiomesVariant[3][1] = BIOME_NULL;
    m_middleBiomesVariant[3][2] = B::Plains;
    m_middleBiomesVariant[3][3] = B::SparseJungle;
    m_middleBiomesVariant[3][4] = B::BambooJungle;
    // 热
    m_middleBiomesVariant[4][0] = BIOME_NULL;
    m_middleBiomesVariant[4][1] = BIOME_NULL;
    m_middleBiomesVariant[4][2] = BIOME_NULL;
    m_middleBiomesVariant[4][3] = BIOME_NULL;
    m_middleBiomesVariant[4][4] = BIOME_NULL;

    // ========== 高原生物群系 ==========
    // 冰冻
    m_plateauBiomes[0][0] = B::SnowyPlains;
    m_plateauBiomes[0][1] = B::SnowyPlains;
    m_plateauBiomes[0][2] = B::SnowyPlains;
    m_plateauBiomes[0][3] = B::SnowyTaiga;
    m_plateauBiomes[0][4] = B::SnowyTaiga;
    // 冷
    m_plateauBiomes[1][0] = B::Meadow;
    m_plateauBiomes[1][1] = B::Meadow;
    m_plateauBiomes[1][2] = B::Forest;
    m_plateauBiomes[1][3] = B::Taiga;
    m_plateauBiomes[1][4] = B::OldGrowthSpruceTaiga;
    // 温和
    m_plateauBiomes[2][0] = B::Meadow;
    m_plateauBiomes[2][1] = B::Meadow;
    m_plateauBiomes[2][2] = B::Meadow;
    m_plateauBiomes[2][3] = B::Meadow;
    m_plateauBiomes[2][4] = B::PaleGarden;
    // 暖
    m_plateauBiomes[3][0] = B::SavannaPlateau;
    m_plateauBiomes[3][1] = B::SavannaPlateau;
    m_plateauBiomes[3][2] = B::Forest;
    m_plateauBiomes[3][3] = B::Forest;
    m_plateauBiomes[3][4] = B::Jungle;
    // 热
    m_plateauBiomes[4][0] = B::Badlands;
    m_plateauBiomes[4][1] = B::Badlands;
    m_plateauBiomes[4][2] = B::Badlands;
    m_plateauBiomes[4][3] = B::WoodedBadlands;
    m_plateauBiomes[4][4] = B::WoodedBadlands;

    // ========== 高原变体生物群系 ==========
    // 冰冻
    m_plateauBiomesVariant[0][0] = B::IceSpikes;
    m_plateauBiomesVariant[0][1] = BIOME_NULL;
    m_plateauBiomesVariant[0][2] = BIOME_NULL;
    m_plateauBiomesVariant[0][3] = BIOME_NULL;
    m_plateauBiomesVariant[0][4] = BIOME_NULL;
    // 冷
    m_plateauBiomesVariant[1][0] = B::CherryGrove;
    m_plateauBiomesVariant[1][1] = BIOME_NULL;
    m_plateauBiomesVariant[1][2] = B::Meadow;
    m_plateauBiomesVariant[1][3] = B::Meadow;
    m_plateauBiomesVariant[1][4] = B::OldGrowthPineTaiga;
    // 温和
    m_plateauBiomesVariant[2][0] = B::CherryGrove;
    m_plateauBiomesVariant[2][1] = B::CherryGrove;
    m_plateauBiomesVariant[2][2] = B::Forest;
    m_plateauBiomesVariant[2][3] = B::BirchForest;
    m_plateauBiomesVariant[2][4] = BIOME_NULL;
    // 暖
    m_plateauBiomesVariant[3][0] = BIOME_NULL;
    m_plateauBiomesVariant[3][1] = BIOME_NULL;
    m_plateauBiomesVariant[3][2] = BIOME_NULL;
    m_plateauBiomesVariant[3][3] = BIOME_NULL;
    m_plateauBiomesVariant[3][4] = BIOME_NULL;
    // 热
    m_plateauBiomesVariant[4][0] = B::ErodedBadlands;
    m_plateauBiomesVariant[4][1] = B::ErodedBadlands;
    m_plateauBiomesVariant[4][2] = BIOME_NULL;
    m_plateauBiomesVariant[4][3] = BIOME_NULL;
    m_plateauBiomesVariant[4][4] = BIOME_NULL;

    // ========== 破碎生物群系 ==========
    // 冰冻
    m_shatteredBiomes[0][0] = B::WindsweptGravellyHills;
    m_shatteredBiomes[0][1] = B::WindsweptGravellyHills;
    m_shatteredBiomes[0][2] = B::WindsweptHills;
    m_shatteredBiomes[0][3] = B::WindsweptForest;
    m_shatteredBiomes[0][4] = B::WindsweptForest;
    // 冷
    m_shatteredBiomes[1][0] = B::WindsweptGravellyHills;
    m_shatteredBiomes[1][1] = B::WindsweptGravellyHills;
    m_shatteredBiomes[1][2] = B::WindsweptHills;
    m_shatteredBiomes[1][3] = B::WindsweptForest;
    m_shatteredBiomes[1][4] = B::WindsweptForest;
    // 温和
    m_shatteredBiomes[2][0] = B::WindsweptHills;
    m_shatteredBiomes[2][1] = B::WindsweptHills;
    m_shatteredBiomes[2][2] = B::WindsweptHills;
    m_shatteredBiomes[2][3] = B::WindsweptForest;
    m_shatteredBiomes[2][4] = B::WindsweptForest;
    // 暖
    m_shatteredBiomes[3][0] = BIOME_NULL;
    m_shatteredBiomes[3][1] = BIOME_NULL;
    m_shatteredBiomes[3][2] = BIOME_NULL;
    m_shatteredBiomes[3][3] = BIOME_NULL;
    m_shatteredBiomes[3][4] = BIOME_NULL;
    // 热
    m_shatteredBiomes[4][0] = BIOME_NULL;
    m_shatteredBiomes[4][1] = BIOME_NULL;
    m_shatteredBiomes[4][2] = BIOME_NULL;
    m_shatteredBiomes[4][3] = BIOME_NULL;
    m_shatteredBiomes[4][4] = BIOME_NULL;
}

// ============================================================================
// 生物群系选择方法
// ============================================================================

BiomeId OverworldBiomeBuilder::pickMiddleBiome(i32 temperature, i32 humidity, f64 weirdness) const
{
    if (weirdness >= 0.0) {
        const BiomeId variant = m_middleBiomesVariant[temperature][humidity];
        if (variant != BIOME_NULL) {
            return variant;
        }
    }
    return m_middleBiomes[temperature][humidity];
}

BiomeId OverworldBiomeBuilder::pickPlateauBiome(i32 temperature, i32 humidity, f64 weirdness) const
{
    if (weirdness >= 0.0) {
        const BiomeId variant = m_plateauBiomesVariant[temperature][humidity];
        if (variant != BIOME_NULL) {
            return variant;
        }
    }
    return m_plateauBiomes[temperature][humidity];
}

BiomeId OverworldBiomeBuilder::pickPeakBiome(i32 temperature, i32 humidity, f64 weirdness) const
{
    MC_UNUSED(humidity);
    if (temperature <= 2) {
        return weirdness < 0.0 ? B::JaggedPeaks : B::FrozenPeaks;
    }
    if (temperature == 3) {
        return B::StonyPeaks;
    }
    // temperature == 4 (hot)
    return pickBadlandsBiome(humidity, weirdness);
}

BiomeId OverworldBiomeBuilder::pickSlopeBiome(i32 temperature, i32 humidity, f64 weirdness) const
{
    MC_UNUSED(weirdness);
    if (temperature >= 3) {
        return pickPlateauBiome(temperature, humidity, weirdness);
    }
    return humidity <= 1 ? B::SnowySlopes : B::Grove;
}

BiomeId OverworldBiomeBuilder::pickBadlandsBiome(i32 humidity, f64 weirdness) const
{
    if (humidity < 2) {
        return weirdness < 0.0 ? B::Badlands : B::ErodedBadlands;
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

// ============================================================================
// 参数添加辅助方法
// ============================================================================

void OverworldBiomeBuilder::addSurfaceBiome(ParameterList<BiomeId>& list,
    const Parameter& temperature,
    const Parameter& humidity,
    const Parameter& continentalness,
    const Parameter& erosion,
    const Parameter& weirdness,
    f64 offset,
    BiomeId biome) const
{
    // 表面生物群系注册两个参数点：depth=0 和 depth=1
    const Parameter surfaceDepth0 = Parameter::span(-1.0f, 0.0f);
    const Parameter surfaceDepth1 = Parameter::span(0.0f, 1.0f);

    list.add(ParameterPoint{temperature,
                 humidity,
                 continentalness,
                 erosion,
                 surfaceDepth0,
                 weirdness,
                 quantizeCoord(static_cast<f32>(offset))},
        biome);
    list.add(ParameterPoint{temperature,
                 humidity,
                 continentalness,
                 erosion,
                 surfaceDepth1,
                 weirdness,
                 quantizeCoord(static_cast<f32>(offset))},
        biome);
}

void OverworldBiomeBuilder::addUndergroundBiome(ParameterList<BiomeId>& list,
    const Parameter& temperature,
    const Parameter& humidity,
    const Parameter& continentalness,
    const Parameter& erosion,
    const Parameter& depth,
    const Parameter& weirdness,
    f64 offset,
    BiomeId biome) const
{
    list.add(
        ParameterPoint{
            temperature, humidity, continentalness, erosion, depth, weirdness, quantizeCoord(static_cast<f32>(offset))},
        biome);
}

// ============================================================================
// 构建参数列表
// ============================================================================

ParameterList<BiomeId> OverworldBiomeBuilder::buildParameterList() const
{
    ParameterList<BiomeId> list;
    const Parameter fullRange = Parameter::fullRange();

    // ========== 海洋生物群系 ==========
    for (i32 temp = 0; temp < 5; ++temp) {
        // 深海
        addSurfaceBiome(list,
            m_temperatures[temp],
            fullRange,
            m_mushroomFieldsContinentalness,
            fullRange,
            fullRange,
            0.0,
            B::MushroomFields);
        addSurfaceBiome(list,
            m_temperatures[temp],
            fullRange,
            m_deepOceanContinentalness,
            fullRange,
            fullRange,
            0.0,
            m_oceans[temp][0]);
        addSurfaceBiome(list,
            m_temperatures[temp],
            fullRange,
            m_oceanContinentalness,
            fullRange,
            fullRange,
            0.0,
            m_oceans[temp][1]);
    }

    // ========== 海岸生物群系 ==========
    for (i32 temp = 0; temp < 5; ++temp) {
        const BiomeId beachBiome = pickBeachBiome(temp);
        addSurfaceBiome(list,
            m_temperatures[temp],
            fullRange,
            m_coastContinentalness,
            fullRange,
            m_valleyWeirdness,
            0.0,
            beachBiome);
    }

    // ========== 内陆生物群系 ==========
    for (i32 temp = 0; temp < 5; ++temp) {
        for (i32 humid = 0; humid < 5; ++humid) {
            // 近内陆 - 山谷
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_nearInlandContinentalness,
                fullRange,
                m_valleyWeirdness,
                0.0,
                pickMiddleBiome(temp, humid, -1.0));

            // 近内陆 - 低坡度
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_nearInlandContinentalness,
                fullRange,
                m_lowWeirdness,
                0.0,
                pickSlopeBiome(temp, humid, -1.0));

            // 近内陆 - 高坡度
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_nearInlandContinentalness,
                fullRange,
                m_highWeirdness,
                0.0,
                pickSlopeBiome(temp, humid, 1.0));

            // 近内陆 - 山峰
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_nearInlandContinentalness,
                fullRange,
                m_peakWeirdness,
                0.0,
                pickPeakBiome(temp, humid, -1.0));

            // 中内陆 - 山谷
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_midInlandContinentalness,
                fullRange,
                m_valleyWeirdness,
                0.0,
                pickMiddleBiome(temp, humid, -1.0));

            // 中内陆 - 低坡度
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_midInlandContinentalness,
                fullRange,
                m_lowWeirdness,
                0.0,
                pickPlateauBiome(temp, humid, -1.0));

            // 中内陆 - 高坡度
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_midInlandContinentalness,
                fullRange,
                m_highWeirdness,
                0.0,
                pickSlopeBiome(temp, humid, 1.0));

            // 中内陆 - 山峰
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_midInlandContinentalness,
                fullRange,
                m_peakWeirdness,
                0.0,
                pickPeakBiome(temp, humid, -1.0));

            // 远内陆 - 山谷
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_farInlandContinentalness,
                fullRange,
                m_valleyWeirdness,
                0.0,
                pickMiddleBiome(temp, humid, -1.0));

            // 远内陆 - 低坡度
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_farInlandContinentalness,
                fullRange,
                m_lowWeirdness,
                0.0,
                pickPlateauBiome(temp, humid, -1.0));

            // 远内陆 - 中坡度
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_farInlandContinentalness,
                fullRange,
                m_midWeirdness,
                0.0,
                pickPlateauBiome(temp, humid, 1.0));

            // 远内陆 - 高坡度
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_farInlandContinentalness,
                fullRange,
                m_highWeirdness,
                0.0,
                pickSlopeBiome(temp, humid, 1.0));

            // 远内陆 - 山峰
            addSurfaceBiome(list,
                m_temperatures[temp],
                m_humidities[humid],
                m_farInlandContinentalness,
                fullRange,
                m_peakWeirdness,
                0.0,
                pickPeakBiome(temp, humid, -1.0));
        }
    }

    // ========== 地下生物群系 ==========
    // 滴水石洞：高大陆度
    addUndergroundBiome(list,
        fullRange,
        fullRange,
        Parameter::span(0.8f, 1.0f),
        fullRange,
        m_undergroundDepth,
        fullRange,
        0.0,
        B::DripstoneCaves);

    // 繁茂洞穴：高湿度
    addUndergroundBiome(list,
        fullRange,
        Parameter::span(0.7f, 1.0f),
        fullRange,
        fullRange,
        m_undergroundDepth,
        fullRange,
        0.0,
        B::LushCaves);

    // 深暗之域：低侵蚀 + 最深层
    addUndergroundBiome(list,
        fullRange,
        fullRange,
        fullRange,
        Parameter::span(m_erosions[0].min, m_erosions[1].max),
        Parameter::point(1.1f),
        fullRange,
        0.0,
        B::DeepDark);

    return list;
}

} // namespace mc::world::biome::source
