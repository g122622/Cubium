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

#include "common/TestWorldHelper.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::biome;

// ============================================================================
// Biome::getPrecipitationAt 测试夹具
// ============================================================================

class BiomeGetPrecipitationAtTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }

    static constexpr i32 SEA_LEVEL = 63;
};

// ============================================================================
// 无降水生物群系测试
// ============================================================================

TEST_F(BiomeGetPrecipitationAtTest, DesertBiome_ReturnsNone)
{
    // 沙漠生物群系：没有降水
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Desert);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.getPrecipitationAt(0, 64, 0, SEA_LEVEL), BiomeClimate::Precipitation::None);
}

// ============================================================================
// 温暖生物群系（降水类型为 Rain，温度 >= 0.15）测试
// ============================================================================

TEST_F(BiomeGetPrecipitationAtTest, PlainsBiome_AtSeaLevel_ReturnsRain)
{
    // 平原生物群系：有降水，温度 > 0.15
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.getPrecipitationAt(0, 64, 0, SEA_LEVEL), BiomeClimate::Precipitation::Rain);
}

TEST_F(BiomeGetPrecipitationAtTest, WarmBiome_AlwaysReturnsRainRegardlessOfAltitude)
{
    // 平原温度较高（0.8），即使在高处也不会降雪
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    EXPECT_EQ(biome.getPrecipitationAt(0, 200, 0, SEA_LEVEL), BiomeClimate::Precipitation::Rain);
}

// ============================================================================
// 寒冷生物群系（降水类型为 Rain 但温度 < 0.15）测试
// ============================================================================

TEST_F(BiomeGetPrecipitationAtTest, SnowyPlainsBiome_AtSeaLevel_ReturnsSnow)
{
    // 积雪平原生物群系：降水类型为 Rain 但温度极低
    const Biome& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    EXPECT_EQ(biome.getPrecipitationAt(0, 64, 0, SEA_LEVEL), BiomeClimate::Precipitation::Snow);
}

// ============================================================================
// 高度对降水类型的影响测试
// ============================================================================

TEST_F(BiomeGetPrecipitationAtTest, MildBiome_AtHighAltitude_ReturnsSnow)
{
    // 构造一个温度略高于阈值的生物群系，测试高海拔降温使其变为降雪
    Biome biome(Biomes::Plains, "test_mild");
    BiomeClimate climate(true, 0.2f, BiomeClimate::TemperatureModifier::None, 0.5f, 0.5f, 0.0f, 0.0f);
    biome.setClimate(climate);

    // 海平面处：温度 >= 0.15，应返回 Rain
    EXPECT_EQ(biome.getPrecipitationAt(0, 64, 0, SEA_LEVEL), BiomeClimate::Precipitation::Rain);

    // 极高处：温度降低到 < 0.15，应返回 Snow
    EXPECT_EQ(biome.getPrecipitationAt(0, 500, 0, SEA_LEVEL), BiomeClimate::Precipitation::Snow);
}

TEST_F(BiomeGetPrecipitationAtTest, NoneBiome_AtHighAltitude_StillReturnsNone)
{
    // 没有降水的生物群系，无论高度如何，都返回 None
    Biome biome(Biomes::Desert, "test_none");
    BiomeClimate climate(false, 0.2f, BiomeClimate::TemperatureModifier::None, 0.5f, 0.5f, 0.0f, 0.0f);
    biome.setClimate(climate);

    EXPECT_EQ(biome.getPrecipitationAt(0, 64, 0, SEA_LEVEL), BiomeClimate::Precipitation::None);
    EXPECT_EQ(biome.getPrecipitationAt(0, 500, 0, SEA_LEVEL), BiomeClimate::Precipitation::None);
}

// ============================================================================
// 边界值测试
// ============================================================================

TEST_F(BiomeGetPrecipitationAtTest, TemperatureAtExactThreshold_ReturnsRain)
{
    // 温度恰好为 0.15 时，>= 0.15 应返回 Rain
    Biome biome(Biomes::Plains, "test_threshold");
    BiomeClimate climate(true, 0.15f, BiomeClimate::TemperatureModifier::None, 0.5f, 0.5f, 0.0f, 0.0f);
    biome.setClimate(climate);

    EXPECT_EQ(biome.getPrecipitationAt(0, 64, 0, SEA_LEVEL), BiomeClimate::Precipitation::Rain);
}

TEST_F(BiomeGetPrecipitationAtTest, TemperatureJustBelowThreshold_ReturnsSnow)
{
    // 温度略低于 0.15 时，应返回 Snow
    Biome biome(Biomes::Plains, "test_below_threshold");
    BiomeClimate climate(true, 0.14f, BiomeClimate::TemperatureModifier::None, 0.5f, 0.5f, 0.0f, 0.0f);
    biome.setClimate(climate);

    EXPECT_EQ(biome.getPrecipitationAt(0, 64, 0, SEA_LEVEL), BiomeClimate::Precipitation::Snow);
}

// ============================================================================
// 与 coldEnoughToSnow / warmEnoughToRain 一致性测试
// ============================================================================

TEST_F(BiomeGetPrecipitationAtTest, ConsistentWithColdEnoughToSnow)
{
    const Biome& coldBiome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    const Biome& warmBiome = BiomeRegistry::instance().get(Biomes::Plains);

    // 如果 coldEnoughToSnow 返回 true，getPrecipitationAt 应返回 Snow
    if (coldBiome.coldEnoughToSnow(0, 64, 0, SEA_LEVEL)) {
        EXPECT_EQ(coldBiome.getPrecipitationAt(0, 64, 0, SEA_LEVEL), BiomeClimate::Precipitation::Snow);
    }

    // 如果 warmEnoughToRain 返回 true 且生物群系有降水，getPrecipitationAt 应返回 Rain
    if (warmBiome.warmEnoughToRain(0, 64, 0, SEA_LEVEL) && warmBiome.hasPrecipitation()) {
        EXPECT_EQ(warmBiome.getPrecipitationAt(0, 64, 0, SEA_LEVEL), BiomeClimate::Precipitation::Rain);
    }
}
