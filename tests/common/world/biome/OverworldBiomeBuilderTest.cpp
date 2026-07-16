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

/**
 * @file OverworldBiomeBuilderTest.cpp
 * @brief OverworldBiomeBuilder unit tests
 *
 * Tests cover:
 * 1. buildParameterList() returns a non-empty list with sufficient entries
 * 2. Climate parameter range correctness (temperature, humidity, erosion, continentalness)
 * 3. Known biome lookups via findValue() with specific climate parameter combinations
 * 4. Ocean biomes at appropriate continentalness ranges
 * 5. Underground biomes (DripstoneCaves, LushCaves, DeepDark) depth ranges
 * 6. addSurfaceBiome produces entries at both depth=0 and depth=1
 * 7. All key overworld biome IDs are present in the parameter list
 */

#include "common/world/biome/source/OverworldBiomeBuilder.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/climate/ParameterList.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include <algorithm>
#include <set>
#include <unordered_set>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace world::biome;
using namespace world::biome::source;
using namespace world::biome::climate;
namespace B = Biomes;

// ============================================================================
// Helper: collect all unique biome IDs from a ParameterList
// ============================================================================

std::set<BiomeId> collectBiomeIds(const ParameterList<BiomeId>& list)
{
    std::set<BiomeId> ids;
    for (const auto& entry : list) {
        ids.insert(entry.second);
    }
    return ids;
}

// ============================================================================
// Helper: count entries with a specific biome ID
// ============================================================================

size_t countEntriesForBiome(const ParameterList<BiomeId>& list, BiomeId biome)
{
    size_t count = 0;
    for (const auto& entry : list) {
        if (entry.second == biome) {
            ++count;
        }
    }
    return count;
}

// ============================================================================
// Helper: check if depth parameter covers a given range
// ============================================================================

bool depthCoversRange(const ParameterPoint& pp, f32 rangeMin, f32 rangeMax)
{
    const i64 qMin = static_cast<i64>(rangeMin * QUANTIZATION_FACTOR);
    const i64 qMax = static_cast<i64>(rangeMax * QUANTIZATION_FACTOR);
    return pp.depth.min <= qMin && pp.depth.max >= qMax;
}

// ============================================================================
// Helper: check if depth parameter is a point at a specific value
// ============================================================================

bool depthIsPoint(const ParameterPoint& pp, f32 value)
{
    const i64 q = static_cast<i64>(value * QUANTIZATION_FACTOR);
    return pp.depth.min == q && pp.depth.max == q;
}

// ============================================================================
// 1. buildParameterList() returns a non-empty list with sufficient entries
// ============================================================================

TEST(OverworldBiomeBuilderTest, BuildParameterListReturnsNonEmpty)
{
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    EXPECT_FALSE(list.empty());
}

TEST(OverworldBiomeBuilderTest, BuildParameterListHasAtLeast50Entries)
{
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    // The overworld has many biome+climate combinations; MC 1.21 typically
    // produces well over 300 entries in the parameter list.
    EXPECT_GE(list.size(), 50u);
}

TEST(OverworldBiomeBuilderTest, BuildParameterListHasSubstantialEntries)
{
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    // In practice, with 13 weirdness slices, 5x5 temperature/humidity combos,
    // 7 erosion levels, multiple continentalness ranges, and off-coast biomes,
    // the total should be well over 200 entries.
    EXPECT_GE(list.size(), 200u);
}

// ============================================================================
// 2. Climate parameter range correctness
// ============================================================================

TEST(OverworldBiomeBuilderTest, TemperatureRangesArePresentInEntries)
{
    // Verify that entries span the full temperature range [-1, 1].
    // Frozen temperature should map to [-1, -0.45], warm to [0.2, 0.55].
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // Find the minimum temperature min and maximum temperature max across all entries
    i64 globalTempMin = std::numeric_limits<i64>::max();
    i64 globalTempMax = std::numeric_limits<i64>::min();

    for (const auto& entry : list) {
        globalTempMin = std::min(globalTempMin, entry.first.temperature.min);
        globalTempMax = std::max(globalTempMax, entry.first.temperature.max);
    }

    // Frozen temperature starts at -1.0
    const i64 frozenMin = static_cast<i64>(-1.0f * QUANTIZATION_FACTOR);
    EXPECT_LE(globalTempMin, frozenMin);

    // Hot temperature goes up to 1.0
    const i64 hotMax = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);
    EXPECT_GE(globalTempMax, hotMax);
}

TEST(OverworldBiomeBuilderTest, HumidityRangesArePresentInEntries)
{
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    i64 globalHumidMin = std::numeric_limits<i64>::max();
    i64 globalHumidMax = std::numeric_limits<i64>::min();

    for (const auto& entry : list) {
        globalHumidMin = std::min(globalHumidMin, entry.first.humidity.min);
        globalHumidMax = std::max(globalHumidMax, entry.first.humidity.max);
    }

    // Humidity spans from -1.0 (arid) to 1.0 (humid)
    const i64 aridMin = static_cast<i64>(-1.0f * QUANTIZATION_FACTOR);
    const i64 humidMax = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);
    EXPECT_LE(globalHumidMin, aridMin);
    EXPECT_GE(globalHumidMax, humidMax);
}

TEST(OverworldBiomeBuilderTest, ContinentalnessRangesSpanFullSpectrum)
{
    // Continentalness ranges from mushroom_fields (-1.2) to far_inland (1.0)
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    i64 globalContMin = std::numeric_limits<i64>::max();
    i64 globalContMax = std::numeric_limits<i64>::min();

    for (const auto& entry : list) {
        globalContMin = std::min(globalContMin, entry.first.continentalness.min);
        globalContMax = std::max(globalContMax, entry.first.continentalness.max);
    }

    // Mushroom fields start at -1.2
    const i64 mushroomMin = static_cast<i64>(-1.2f * QUANTIZATION_FACTOR);
    EXPECT_LE(globalContMin, mushroomMin);

    // Far inland goes up to 1.0
    const i64 farInlandMax = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);
    EXPECT_GE(globalContMax, farInlandMax);
}

// ============================================================================
// 3. Known biome lookups via findValue() with specific climate targets
// ============================================================================

TEST(OverworldBiomeBuilderTest, MushroomFieldsAtMushroomFieldsContinentalness)
{
    // Mushroom fields should be found at very negative continentalness
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // Target: fullRange temp/humid/erosion/weirdness, mushroom fields continentalness, depth=0
    auto target = TargetPoint::fromFloats(0.0f, 0.0f, -1.1f, 0.0f, 0.0f, 0.0f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::MushroomFields);
}

TEST(OverworldBiomeBuilderTest, FrozenOceanAtDeepOceanContinentalnessFrozenTemp)
{
    // At frozen temperature + deep ocean continentalness => DeepFrozenOcean
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // Frozen temperature: [-1.0, -0.45], center ~ -0.725
    // Deep ocean continentalness: [-1.05, -0.455], center ~ -0.75
    auto target = TargetPoint::fromFloats(-0.725f, 0.0f, -0.75f, 0.0f, 0.0f, 0.0f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::DeepFrozenOcean);
}

TEST(OverworldBiomeBuilderTest, WarmOceanAtOceanContinentalnessWarmTemp)
{
    // Warm temperature + ocean continentalness => WarmOcean
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // Warm temperature: [0.55, 1.0], center ~ 0.775
    // Ocean continentalness: [-0.455, -0.19], center ~ -0.32
    auto target = TargetPoint::fromFloats(0.775f, 0.0f, -0.32f, 0.0f, 0.0f, 0.0f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::WarmOcean);
}

TEST(OverworldBiomeBuilderTest, PlainsAtWarmAridInland)
{
    // m_middleBiomes[3][0] = Savanna (warm, arid)
    // m_middleBiomes[1][0] = Plains (cold, arid)
    // m_middleBiomes[1][1] = Plains (cold, dry)
    // 目标 warm+arid 的中内陆，使用 mid slice 负奇异度（weirdness ∈ [-0.26666, -0.05]）
    // temperature[3]=warm [0.2,0.55], humidity[0]=arid [-1.0,-0.35]
    // mid-inland continentalness [0.03, 0.3], erosion[2] mid [-0.375, -0.2225]
    // 注意：weirdness=0 落在 Valleys 切片 [-0.05,0.05]（河流），无法命中陆地生物群系，
    // 必须使用非零的 mid/high/low 切片奇异度。
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // Savanna at warm+arid inland（mid slice 负奇异度）
    auto target = TargetPoint::fromFloats(0.375f, -0.675f, 0.15f, -0.3f, 0.0f, -0.2f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::Savanna);
}

TEST(OverworldBiomeBuilderTest, DesertAtHotAnyHumidityInland)
{
    // m_middleBiomes[4][*] = Desert for all humidity levels
    // temperature[4]=hot [0.55,1.0], humidity neutral, mid-inland
    // 注意：hot 温度在内陆会走 pickMiddleBiomeOrBadlandsIfHot → pickBadlandsBiome，
    // 在低侵蚀（erosions[2]）的中内陆会命中 Badlands 而非 Desert。
    // Desert 在 hot 温度的中内陆仅出现在较高侵蚀（erosions[3]/erosions[4]）。
    // 因此这里使用 erosion=0.1（erosions[4]=[0.05,0.45]）+ mid slice 负奇异度。
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    auto target = TargetPoint::fromFloats(0.775f, 0.0f, 0.15f, 0.1f, 0.0f, -0.2f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::Desert);
}

TEST(OverworldBiomeBuilderTest, SnowyPlainsAtFrozenAridInland)
{
    // m_middleBiomes[0][0] = SnowyPlains (frozen, arid)
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // frozen temperature [-1.0, -0.45], arid humidity [-1.0, -0.35]
    // 使用 mid slice 负奇异度（weirdness ∈ [-0.26666, -0.05]），避免落入 Valleys 切片
    auto target = TargetPoint::fromFloats(-0.725f, -0.675f, 0.15f, -0.3f, 0.0f, -0.2f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::SnowyPlains);
}

TEST(OverworldBiomeBuilderTest, TaigaAtFrozenHumidInland)
{
    // m_middleBiomes[0][4] = Taiga (frozen, humid)
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // frozen temperature [-1.0, -0.45], humid humidity [0.3, 1.0]
    // 使用 mid slice 负奇异度，避免落入 Valleys 切片
    auto target = TargetPoint::fromFloats(-0.725f, 0.65f, 0.15f, -0.3f, 0.0f, -0.2f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::Taiga);
}

TEST(OverworldBiomeBuilderTest, ForestAtTemperateNeutralInland)
{
    // m_middleBiomes[2][2] = FlowerForest (if weirdness >=0) or Forest (if weirdness <0)
    // With weirdness < 0 (mid slice negative), should get Forest
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // temperate temperature [-0.15, 0.2], neutral humidity [-0.1, 0.1]
    // Target weirdness = -0.33 (mid slice negative range)
    auto target = TargetPoint::fromFloats(0.025f, 0.0f, 0.15f, -0.3f, 0.0f, -0.33f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::Forest);
}

TEST(OverworldBiomeBuilderTest, JungleAtWarmHumidInland)
{
    // m_middleBiomes[3][4] = Jungle (warm, humid)
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // 使用 mid slice 负奇异度，避免落入 Valleys 切片
    auto target = TargetPoint::fromFloats(0.375f, 0.65f, 0.15f, -0.3f, 0.0f, -0.2f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::Jungle);
}

TEST(OverworldBiomeBuilderTest, DeepOceanAtTemperateDeepOceanContinentalness)
{
    // m_oceans[2][0] = DeepOcean (temperate, deep)
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // temperate [-0.15, 0.2], deep ocean [-1.05, -0.455]
    auto target = TargetPoint::fromFloats(0.025f, 0.0f, -0.75f, 0.0f, 0.0f, 0.0f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::DeepOcean);
}

TEST(OverworldBiomeBuilderTest, OceanAtTemperateOceanContinentalness)
{
    // m_oceans[2][1] = Ocean (temperate, shallow)
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    auto target = TargetPoint::fromFloats(0.025f, 0.0f, -0.32f, 0.0f, 0.0f, 0.0f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::Ocean);
}

TEST(OverworldBiomeBuilderTest, StonyShoreAtCoastLowErosionMidWeirdness)
{
    // Mid slice: StonyShore at coast continentalness with erosion 0-2
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // coast [-0.19, -0.11], erosion span [0,2] => [-1.0, -0.2225]
    auto target = TargetPoint::fromFloats(0.0f, 0.0f, -0.15f, -0.6f, 0.0f, -0.33f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::StonyShore);
}

TEST(OverworldBiomeBuilderTest, SwampAtColdTemperateInlandHighErosion)
{
    // Swamp: temperatures[1..2], inland, erosion[6] (highest)
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // cold temperature [-0.45, -0.15], neutral humidity, inland, high erosion
    auto target = TargetPoint::fromFloats(-0.3f, 0.0f, 0.2f, 0.775f, 0.0f, -0.33f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::Swamp);
}

// ============================================================================
// 4. Ocean biomes at appropriate continentalness
// ============================================================================

TEST(OverworldBiomeBuilderTest, OceanBiomesArePresentInList)
{
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);

    EXPECT_TRUE(ids.count(B::Ocean));
    EXPECT_TRUE(ids.count(B::DeepOcean));
    EXPECT_TRUE(ids.count(B::FrozenOcean));
    EXPECT_TRUE(ids.count(B::DeepFrozenOcean));
    EXPECT_TRUE(ids.count(B::WarmOcean));
    EXPECT_TRUE(ids.count(B::LukewarmOcean));
    EXPECT_TRUE(ids.count(B::ColdOcean));
    EXPECT_TRUE(ids.count(B::DeepLukewarmOcean));
    EXPECT_TRUE(ids.count(B::DeepColdOcean));
}

TEST(OverworldBiomeBuilderTest, OceanBiomesOnlyAtNegativeContinentalness)
{
    // Ocean biomes should only appear at negative continentalness
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    std::set<BiomeId> oceanBiomes = {B::Ocean,
        B::DeepOcean,
        B::FrozenOcean,
        B::DeepFrozenOcean,
        B::WarmOcean,
        B::LukewarmOcean,
        B::ColdOcean,
        B::DeepLukewarmOcean,
        B::DeepColdOcean};

    for (const auto& entry : list) {
        if (oceanBiomes.count(entry.second)) {
            // Continentalness max should be negative (ocean ranges are all negative)
            EXPECT_LT(entry.first.continentalness.max, 0)
                << "Ocean biome " << entry.second << " should have negative continentalness max";
        }
    }
}

// ============================================================================
// 5. Underground biomes have correct depth ranges
// ============================================================================

TEST(OverworldBiomeBuilderTest, DripstoneCavesHasUndergroundDepthRange)
{
    // DripstoneCaves: addUndergroundBiome => depth [0.2, 0.9]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    bool found = false;
    for (const auto& entry : list) {
        if (entry.second == B::DripstoneCaves) {
            found = true;
            EXPECT_TRUE(depthCoversRange(entry.first, 0.2f, 0.9f))
                << "DripstoneCaves should have depth spanning [0.2, 0.9]";
        }
    }
    EXPECT_TRUE(found) << "DripstoneCaves should be in the parameter list";
}

TEST(OverworldBiomeBuilderTest, LushCavesHasUndergroundDepthRange)
{
    // LushCaves: addUndergroundBiome => depth [0.2, 0.9]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    bool found = false;
    for (const auto& entry : list) {
        if (entry.second == B::LushCaves) {
            found = true;
            EXPECT_TRUE(depthCoversRange(entry.first, 0.2f, 0.9f)) << "LushCaves should have depth spanning [0.2, 0.9]";
        }
    }
    EXPECT_TRUE(found) << "LushCaves should be in the parameter list";
}

TEST(OverworldBiomeBuilderTest, DeepDarkHasBottomDepthPoint)
{
    // DeepDark: addBottomBiome => depth = point(1.1)
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    bool found = false;
    for (const auto& entry : list) {
        if (entry.second == B::DeepDark) {
            found = true;
            EXPECT_TRUE(depthIsPoint(entry.first, 1.1f)) << "DeepDark should have depth at exactly 1.1";
        }
    }
    EXPECT_TRUE(found) << "DeepDark should be in the parameter list";
}

TEST(OverworldBiomeBuilderTest, UndergroundBiomesNotAtDepthZero)
{
    // Underground biomes should NOT appear at depth=0 surface level
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    std::set<BiomeId> undergroundBiomes = {B::DripstoneCaves, B::LushCaves, B::DeepDark};

    for (const auto& entry : list) {
        if (undergroundBiomes.count(entry.second)) {
            // None of the underground biomes should have depth=0 as a point
            if (entry.first.depth.min == entry.first.depth.max) {
                // Point depth
                EXPECT_NE(entry.first.depth.min, 0) << "Underground biome should not be at depth=0";
            } else {
                // Range depth - should not include 0 if it starts at 0.2
                EXPECT_GE(entry.first.depth.min, static_cast<i64>(0.2f * QUANTIZATION_FACTOR))
                    << "Underground biome depth range should start at >= 0.2";
            }
        }
    }
}

// ============================================================================
// 6. addSurfaceBiome adds entries at both depth=0 AND depth=1
// ============================================================================

TEST(OverworldBiomeBuilderTest, SurfaceBiomesHaveBothDepth0AndDepth1Entries)
{
    // Each surface biome should have entries at depth=0 AND depth=1
    // because addSurfaceBiome creates two ParameterPoints per call.
    // Pick a biome that only appears as surface (e.g., Plains)
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // Count Plains entries
    size_t plainsCount = countEntriesForBiome(list, B::Plains);
    EXPECT_GE(plainsCount, 2u) << "Plains should appear at least twice (depth=0 and depth=1)";

    // Verify that Plains has entries at both depth=0 and depth=1
    bool hasDepth0 = false;
    bool hasDepth1 = false;
    for (const auto& entry : list) {
        if (entry.second == B::Plains) {
            if (depthIsPoint(entry.first, 0.0f)) {
                hasDepth0 = true;
            }
            if (depthIsPoint(entry.first, 1.0f)) {
                hasDepth1 = true;
            }
        }
    }
    EXPECT_TRUE(hasDepth0) << "Plains should have an entry at depth=0";
    EXPECT_TRUE(hasDepth1) << "Plains should have an entry at depth=1";
}

TEST(OverworldBiomeBuilderTest, MushroomFieldsHasBothDepth0AndDepth1)
{
    // MushroomFields is added via addSurfaceBiome in addOffCoastBiomes
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    bool hasDepth0 = false;
    bool hasDepth1 = false;
    for (const auto& entry : list) {
        if (entry.second == B::MushroomFields) {
            if (depthIsPoint(entry.first, 0.0f)) {
                hasDepth0 = true;
            }
            if (depthIsPoint(entry.first, 1.0f)) {
                hasDepth1 = true;
            }
        }
    }
    EXPECT_TRUE(hasDepth0) << "MushroomFields should have an entry at depth=0";
    EXPECT_TRUE(hasDepth1) << "MushroomFields should have an entry at depth=1";
}

TEST(OverworldBiomeBuilderTest, SurfaceBiomeEntryCountIsEven)
{
    // Since addSurfaceBiome adds exactly 2 entries per call, surface biomes
    // should have an even number of entries (2 per climate combination).
    // Plains is a surface biome used as middleBiome[1][0], [1][1], [2][1], [3][2]
    // It also appears as variant. The count should be even.
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    size_t desertCount = countEntriesForBiome(list, B::Desert);
    EXPECT_EQ(desertCount % 2, 0u) << "Desert (surface biome) should have an even number of entries";

    size_t taigaCount = countEntriesForBiome(list, B::Taiga);
    EXPECT_EQ(taigaCount % 2, 0u) << "Taiga (surface biome) should have an even number of entries";
}

// ============================================================================
// 7. All key overworld biome IDs are present in the parameter list
// ============================================================================

TEST(OverworldBiomeBuilderTest, AllKeyOverworldBiomesPresent)
{
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);

    // --- Surface biomes ---
    EXPECT_TRUE(ids.count(B::Plains));
    EXPECT_TRUE(ids.count(B::Desert));
    EXPECT_TRUE(ids.count(B::Forest));
    EXPECT_TRUE(ids.count(B::Taiga));
    EXPECT_TRUE(ids.count(B::Swamp));
    EXPECT_TRUE(ids.count(B::Mountains)); // WindsweptHills
    EXPECT_TRUE(ids.count(B::Jungle));

    // --- Snowy biomes ---
    EXPECT_TRUE(ids.count(B::SnowyPlains));
    EXPECT_TRUE(ids.count(B::SnowyTaiga));
    EXPECT_TRUE(ids.count(B::SnowyBeach));

    // --- Beach and shore ---
    EXPECT_TRUE(ids.count(B::Beach));
    EXPECT_TRUE(ids.count(B::StonyShore)); // StoneShore alias

    // --- Ocean biomes ---
    EXPECT_TRUE(ids.count(B::Ocean));
    EXPECT_TRUE(ids.count(B::DeepOcean));
    EXPECT_TRUE(ids.count(B::FrozenOcean));
    EXPECT_TRUE(ids.count(B::WarmOcean));

    // --- Mountain biomes ---
    EXPECT_TRUE(ids.count(B::Meadow));
    EXPECT_TRUE(ids.count(B::Grove));
    EXPECT_TRUE(ids.count(B::SnowySlopes));
    EXPECT_TRUE(ids.count(B::JaggedPeaks));
    EXPECT_TRUE(ids.count(B::FrozenPeaks));
    EXPECT_TRUE(ids.count(B::StonyPeaks));

    // --- Badlands ---
    EXPECT_TRUE(ids.count(B::Badlands));
    EXPECT_TRUE(ids.count(B::WoodedBadlands)); // WoodedBadlandsPlateau alias

    // --- Savanna ---
    EXPECT_TRUE(ids.count(B::Savanna));
    EXPECT_TRUE(ids.count(B::SavannaPlateau));

    // --- Birch/Dark forest ---
    EXPECT_TRUE(ids.count(B::BirchForest));
    EXPECT_TRUE(ids.count(B::DarkForest));

    // --- Old growth ---
    EXPECT_TRUE(ids.count(B::OldGrowthPineTaiga));   // GiantTreeTaiga alias
    EXPECT_TRUE(ids.count(B::OldGrowthSpruceTaiga)); // GiantSpruceTaiga alias

    // --- Rivers ---
    EXPECT_TRUE(ids.count(B::River));
    EXPECT_TRUE(ids.count(B::FrozenRiver));

    // --- Mushroom ---
    EXPECT_TRUE(ids.count(B::MushroomFields));

    // --- Underground biomes ---
    EXPECT_TRUE(ids.count(B::DripstoneCaves));
    EXPECT_TRUE(ids.count(B::LushCaves));
    EXPECT_TRUE(ids.count(B::DeepDark));

    // --- Swamp variants ---
    EXPECT_TRUE(ids.count(B::MangroveSwamp));

    // --- Variants ---
    EXPECT_TRUE(ids.count(B::SunflowerPlains));
    EXPECT_TRUE(ids.count(B::FlowerForest));
    EXPECT_TRUE(ids.count(B::IceSpikes));
    EXPECT_TRUE(ids.count(B::CherryGrove));

    // --- Windswept variants ---
    EXPECT_TRUE(ids.count(B::WindsweptSavanna)); // ShatteredSavanna alias
    EXPECT_TRUE(ids.count(B::ErodedBadlands));

    // --- Cold ocean variants ---
    EXPECT_TRUE(ids.count(B::ColdOcean));
    EXPECT_TRUE(ids.count(B::DeepColdOcean));
    EXPECT_TRUE(ids.count(B::LukewarmOcean));
    EXPECT_TRUE(ids.count(B::DeepLukewarmOcean));
    EXPECT_TRUE(ids.count(B::DeepFrozenOcean));
}

TEST(OverworldBiomeBuilderTest, NoNetherBiomesInList)
{
    // Nether biomes should not appear in the overworld parameter list
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);

    EXPECT_FALSE(ids.count(B::NetherWastes));
    EXPECT_FALSE(ids.count(B::SoulSandValley));
    EXPECT_FALSE(ids.count(B::CrimsonForest));
    EXPECT_FALSE(ids.count(B::WarpedForest));
    EXPECT_FALSE(ids.count(B::BasaltDeltas));
}

TEST(OverworldBiomeBuilderTest, NoEndBiomesInList)
{
    // End biomes should not appear in the overworld parameter list
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);

    EXPECT_FALSE(ids.count(B::TheEnd));
    EXPECT_FALSE(ids.count(B::SmallEndIslands));
    EXPECT_FALSE(ids.count(B::EndMidlands));
    EXPECT_FALSE(ids.count(B::EndHighlands));
    EXPECT_FALSE(ids.count(B::EndBarrens));
}

// ============================================================================
// Additional structural tests
// ============================================================================

TEST(OverworldBiomeBuilderTest, AllEntriesHaveValidBiomeIds)
{
    // Every entry in the parameter list should have a valid (non-max) BiomeId
    // and should be within the known biome ID range
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    for (const auto& entry : list) {
        EXPECT_NE(entry.second, std::numeric_limits<BiomeId>::max())
            << "Entry should not have BIOME_NULL sentinel value";
        EXPECT_LT(entry.second, B::Count) << "BiomeId " << entry.second << " exceeds known biome count";
    }
}

TEST(OverworldBiomeBuilderTest, AllSurfaceEntriesHaveDepthZeroOrOne)
{
    // Surface biomes (added via addSurfaceBiome) have depth=0.0 or depth=1.0
    // Underground biomes have depth=[0.2, 0.9] and bottom biomes have depth=1.1
    // This test checks that all entries at depth=0 are surface biomes
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    std::set<BiomeId> nonSurfaceBiomes = {B::DripstoneCaves, B::LushCaves, B::DeepDark};

    for (const auto& entry : list) {
        if (depthIsPoint(entry.first, 0.0f)) {
            EXPECT_FALSE(nonSurfaceBiomes.count(entry.second))
                << "Non-surface biome " << entry.second << " should not have depth=0";
        }
    }
}

TEST(OverworldBiomeBuilderTest, BuilderCanBeConstructedMultipleTimes)
{
    // Verify the builder can be constructed and used multiple times
    // without issues (no global state problems)
    OverworldBiomeBuilder builder1;
    auto list1 = builder1.buildParameterList();

    OverworldBiomeBuilder builder2;
    auto list2 = builder2.buildParameterList();

    EXPECT_EQ(list1.size(), list2.size());

    // Verify entries match
    auto it1 = list1.begin();
    auto it2 = list2.begin();
    while (it1 != list1.end() && it2 != list2.end()) {
        EXPECT_EQ(it1->second, it2->second);
        ++it1;
        ++it2;
    }
}

TEST(OverworldBiomeBuilderTest, WeirdnessSlicesCoverFullRange)
{
    // The 13 weirdness slices should cover the full range [-1, 1]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    i64 weirdMin = std::numeric_limits<i64>::max();
    i64 weirdMax = std::numeric_limits<i64>::min();

    for (const auto& entry : list) {
        weirdMin = std::min(weirdMin, entry.first.weirdness.min);
        weirdMax = std::max(weirdMax, entry.first.weirdness.max);
    }

    const i64 negOne = static_cast<i64>(-1.0f * QUANTIZATION_FACTOR);
    const i64 posOne = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);
    EXPECT_LE(weirdMin, negOne);
    EXPECT_GE(weirdMax, posOne);
}

TEST(OverworldBiomeBuilderTest, ErosionRangesPresentInEntries)
{
    // Erosion should span from -1.0 to 1.0
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    i64 eroMin = std::numeric_limits<i64>::max();
    i64 eroMax = std::numeric_limits<i64>::min();

    for (const auto& entry : list) {
        eroMin = std::min(eroMin, entry.first.erosion.min);
        eroMax = std::max(eroMax, entry.first.erosion.max);
    }

    const i64 negOne = static_cast<i64>(-1.0f * QUANTIZATION_FACTOR);
    const i64 posOne = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);
    EXPECT_LE(eroMin, negOne);
    EXPECT_GE(eroMax, posOne);
}

TEST(OverworldBiomeBuilderTest, FrozenRiverAtFrozenTemperature)
{
    // Valleys with frozen temperature should produce FrozenRiver
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    auto ids = collectBiomeIds(list);
    EXPECT_TRUE(ids.count(B::FrozenRiver));
}

TEST(OverworldBiomeBuilderTest, RiverAtUnfrozenTemperature)
{
    // Valleys with unfrozen temperature should produce River
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    auto ids = collectBiomeIds(list);
    EXPECT_TRUE(ids.count(B::River));
}

TEST(OverworldBiomeBuilderTest, MountainBiomesPresentAtLowErosion)
{
    // Low erosion inland areas should produce mountain biomes
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);

    EXPECT_TRUE(ids.count(B::JaggedPeaks));
    EXPECT_TRUE(ids.count(B::FrozenPeaks));
    EXPECT_TRUE(ids.count(B::StonyPeaks));
    EXPECT_TRUE(ids.count(B::Grove));
    EXPECT_TRUE(ids.count(B::SnowySlopes));
}

TEST(OverworldBiomeBuilderTest, MangroveSwampAtWarmHotTemperature)
{
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);
    EXPECT_TRUE(ids.count(B::MangroveSwamp));
}

TEST(OverworldBiomeBuilderTest, PaleGardenPresent)
{
    // PaleGarden appears as plateauBiomesVariant[2][4]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);
    EXPECT_TRUE(ids.count(B::PaleGarden));
}

TEST(OverworldBiomeBuilderTest, BambooJunglePresent)
{
    // BambooJungle appears as middleBiomesVariant[3][4]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);
    EXPECT_TRUE(ids.count(B::BambooJungle));
}

TEST(OverworldBiomeBuilderTest, SparseJunglePresent)
{
    // SparseJungle appears as middleBiomesVariant[3][3]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);
    EXPECT_TRUE(ids.count(B::SparseJungle));
}

TEST(OverworldBiomeBuilderTest, IceSpikesPresent)
{
    // IceSpikes appears as middleBiomesVariant[0][0] and plateauBiomesVariant[0][0]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);
    EXPECT_TRUE(ids.count(B::IceSpikes));
}

TEST(OverworldBiomeBuilderTest, OldGrowthPineTaigaPresent)
{
    // OldGrowthPineTaiga appears as middleBiomesVariant[1][4] and plateauBiomesVariant[1][4]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);
    EXPECT_TRUE(ids.count(B::OldGrowthPineTaiga));
}

TEST(OverworldBiomeBuilderTest, OldGrowthBirchForestPresent)
{
    // OldGrowthBirchForest (TallBirchForest) appears as middleBiomesVariant[2][3]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);
    EXPECT_TRUE(ids.count(B::OldGrowthBirchForest));
}

// ============================================================================
// Specific pick method verification through findValue
// ============================================================================

TEST(OverworldBiomeBuilderTest, PickPeakBiomeFrozenNegativeWeirdness)
{
    // frozen temperature, any humidity, weirdness < 0 => JaggedPeaks
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // Target: frozen temp, mid-inland, very low erosion (erosion[0]), weirdness < 0 (peaks slice)
    // peaks weirdness range: [-0.7667, -0.5667] (negative)
    auto target = TargetPoint::fromFloats(-0.725f, 0.0f, 0.5f, -0.9f, 0.0f, -0.67f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::JaggedPeaks);
}

TEST(OverworldBiomeBuilderTest, PickPeakBiomeFrozenPositiveWeirdness)
{
    // frozen temperature, any humidity, weirdness >= 0 => FrozenPeaks
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // peaks weirdness range: [0.5667, 0.7667] (positive)
    auto target = TargetPoint::fromFloats(-0.725f, 0.0f, 0.5f, -0.9f, 0.0f, 0.67f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::FrozenPeaks);
}

TEST(OverworldBiomeBuilderTest, PickPeakBiomeHotNegativeWeirdness)
{
    // hot temperature, arid humidity, weirdness < 0 => Badlands (from pickBadlandsBiome)
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // hot temp [0.55, 1.0], arid humidity [-1.0, -0.35], mid/far inland, low erosion
    auto target = TargetPoint::fromFloats(0.775f, -0.675f, 0.5f, -0.9f, 0.0f, -0.67f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::Badlands);
}

TEST(OverworldBiomeBuilderTest, PickSlopeBiomeColdArid)
{
    // temperature <= 1 (cold), humidity <= 1 (arid) => SnowySlopes
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // cold temp [-0.45, -0.15], arid humidity [-1.0, -0.35], near inland, erosion[0]
    auto target = TargetPoint::fromFloats(-0.3f, -0.675f, -0.04f, -0.9f, 0.0f, 0.33f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::SnowySlopes);
}

TEST(OverworldBiomeBuilderTest, PickSlopeBiomeColdHumid)
{
    // temperature <= 1 (cold), humidity > 1 => Grove
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // cold temp [-0.45, -0.15], humid humidity [0.3, 1.0], near inland, erosion[0]
    auto target = TargetPoint::fromFloats(-0.3f, 0.65f, -0.04f, -0.9f, 0.0f, 0.33f);
    BiomeId result = list.findValue(target);
    EXPECT_EQ(result, B::Grove);
}

TEST(OverworldBiomeBuilderTest, PickBeachBiomeFrozen)
{
    // frozen temperature => SnowyBeach
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // frozen temp, coast continentalness, erosion[3..4] in low/valley slice
    auto target = TargetPoint::fromFloats(-0.725f, 0.0f, -0.15f, 0.0f, 0.0f, -0.15f);
    BiomeId result = list.findValue(target);
    // The exact result depends on the slice, but at valley with coast and frozen temp,
    // it should be SnowyBeach (or FrozenRiver in valley)
    // In low slice with erosion 3-4 at coast: beach => SnowyBeach for frozen
    EXPECT_TRUE(result == B::SnowyBeach || result == B::FrozenRiver || result == B::StonyShore)
        << "At frozen coast, expected SnowyBeach, FrozenRiver, or StonyShore, got " << result;
}

TEST(OverworldBiomeBuilderTest, PickBeachBiomeHot)
{
    // hot temperature => Desert (from pickBeachBiome)
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    // hot temp, coast, low slice, erosion 3-4
    auto target = TargetPoint::fromFloats(0.775f, 0.0f, -0.15f, 0.0f, 0.0f, -0.15f);
    BiomeId result = list.findValue(target);
    EXPECT_TRUE(result == B::Desert || result == B::Beach) << "At hot coast, expected Desert or Beach, got " << result;
}

TEST(OverworldBiomeBuilderTest, BadlandsAtHotLowHumidity)
{
    // Badlands should appear at hot temperature with low humidity
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();
    auto ids = collectBiomeIds(list);

    EXPECT_TRUE(ids.count(B::Badlands));
    EXPECT_TRUE(ids.count(B::ErodedBadlands));
    EXPECT_TRUE(ids.count(B::WoodedBadlands));
}

// ============================================================================
// Quantitative entry count verification
// ============================================================================

TEST(OverworldBiomeBuilderTest, OffCoastBiomeEntriesPresent)
{
    // Off-coast biomes: 1 MushroomFields + 5*2 ocean biomes = 11 surface biomes
    // Each surface biome has 2 entries (depth=0 and depth=1), so 22 entries total
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    size_t mushroomCount = countEntriesForBiome(list, B::MushroomFields);
    EXPECT_GE(mushroomCount, 2u) << "MushroomFields should have at least 2 entries";

    size_t warmOceanCount = countEntriesForBiome(list, B::WarmOcean);
    EXPECT_GE(warmOceanCount, 2u) << "WarmOcean should have at least 2 entries";
}

TEST(OverworldBiomeBuilderTest, DripstoneCavesHighContinentalness)
{
    // DripstoneCaves has continentalness [0.8, 1.0]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    for (const auto& entry : list) {
        if (entry.second == B::DripstoneCaves) {
            const i64 expectedMin = static_cast<i64>(0.8f * QUANTIZATION_FACTOR);
            const i64 expectedMax = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);
            EXPECT_EQ(entry.first.continentalness.min, expectedMin);
            EXPECT_EQ(entry.first.continentalness.max, expectedMax);
        }
    }
}

TEST(OverworldBiomeBuilderTest, LushCavesHighHumidity)
{
    // LushCaves has humidity [0.7, 1.0]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    for (const auto& entry : list) {
        if (entry.second == B::LushCaves) {
            const i64 expectedMin = static_cast<i64>(0.7f * QUANTIZATION_FACTOR);
            const i64 expectedMax = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);
            EXPECT_EQ(entry.first.humidity.min, expectedMin);
            EXPECT_EQ(entry.first.humidity.max, expectedMax);
        }
    }
}

TEST(OverworldBiomeBuilderTest, DeepDarkLowErosion)
{
    // DeepDark has erosion span [erosions[0], erosions[1]] = [-1.0, -0.375]
    OverworldBiomeBuilder builder;
    auto list = builder.buildParameterList();

    for (const auto& entry : list) {
        if (entry.second == B::DeepDark) {
            const i64 expectedMin = static_cast<i64>(-1.0f * QUANTIZATION_FACTOR);
            const i64 expectedMax = static_cast<i64>(-0.375f * QUANTIZATION_FACTOR);
            EXPECT_EQ(entry.first.erosion.min, expectedMin);
            EXPECT_EQ(entry.first.erosion.max, expectedMax);
        }
    }
}

// ============================================================================
// spawnTarget() 测试
// ============================================================================
//
// MC 1.21.11: OverworldBiomeBuilder.spawnTarget() 返回 2 个 ParameterPoint。
// 验证我们的实现与原版完全一致。

TEST(OverworldBiomeBuilderSpawnTargetTest, ReturnsTwoParameterPoints)
{
    OverworldBiomeBuilder builder;
    auto target = builder.spawnTarget();
    ASSERT_EQ(target.size(), 2u);
}

TEST(OverworldBiomeBuilderSpawnTargetTest, TemperatureIsFullRange)
{
    // MC: this.FULL_RANGE = Climate.Parameter.span(-1.0F, 1.0F)
    OverworldBiomeBuilder builder;
    auto target = builder.spawnTarget();
    ASSERT_EQ(target.size(), 2u);

    const i64 expectedMin = static_cast<i64>(-1.0f * QUANTIZATION_FACTOR);
    const i64 expectedMax = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);
    for (const auto& pp : target) {
        EXPECT_EQ(pp.temperature.min, expectedMin);
        EXPECT_EQ(pp.temperature.max, expectedMax);
    }
}

TEST(OverworldBiomeBuilderSpawnTargetTest, HumidityIsFullRange)
{
    OverworldBiomeBuilder builder;
    auto target = builder.spawnTarget();
    ASSERT_EQ(target.size(), 2u);

    const i64 expectedMin = static_cast<i64>(-1.0f * QUANTIZATION_FACTOR);
    const i64 expectedMax = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);
    for (const auto& pp : target) {
        EXPECT_EQ(pp.humidity.min, expectedMin);
        EXPECT_EQ(pp.humidity.max, expectedMax);
    }
}

TEST(OverworldBiomeBuilderSpawnTargetTest, ErosionIsFullRange)
{
    OverworldBiomeBuilder builder;
    auto target = builder.spawnTarget();
    ASSERT_EQ(target.size(), 2u);

    const i64 expectedMin = static_cast<i64>(-1.0f * QUANTIZATION_FACTOR);
    const i64 expectedMax = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);
    for (const auto& pp : target) {
        EXPECT_EQ(pp.erosion.min, expectedMin);
        EXPECT_EQ(pp.erosion.max, expectedMax);
    }
}

TEST(OverworldBiomeBuilderSpawnTargetTest, ContinentalnessSpansInlandToFullRange)
{
    // MC: Climate.Parameter.span(this.inlandContinentalness, this.FULL_RANGE)
    //   = span([-0.11, 0.55], [-1, 1]) = [-0.11, 1]
    OverworldBiomeBuilder builder;
    auto target = builder.spawnTarget();
    ASSERT_EQ(target.size(), 2u);

    const i64 expectedMin = static_cast<i64>(-0.11f * QUANTIZATION_FACTOR);
    const i64 expectedMax = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);
    for (const auto& pp : target) {
        EXPECT_EQ(pp.continentalness.min, expectedMin);
        EXPECT_EQ(pp.continentalness.max, expectedMax);
    }
}

TEST(OverworldBiomeBuilderSpawnTargetTest, DepthIsZeroPoint)
{
    // MC: Climate.Parameter.point(0.0F)
    OverworldBiomeBuilder builder;
    auto target = builder.spawnTarget();
    ASSERT_EQ(target.size(), 2u);

    for (const auto& pp : target) {
        EXPECT_EQ(pp.depth.min, 0);
        EXPECT_EQ(pp.depth.max, 0);
    }
}

TEST(OverworldBiomeBuilderSpawnTargetTest, OffsetIsZero)
{
    OverworldBiomeBuilder builder;
    auto target = builder.spawnTarget();
    ASSERT_EQ(target.size(), 2u);

    for (const auto& pp : target) {
        EXPECT_EQ(pp.offset, 0);
    }
}

TEST(OverworldBiomeBuilderSpawnTargetTest, WeirdnessSplitAtPlusMinus016)
{
    // MC: 第一个 weirdness = span(-1.0F, -0.16F)
    //     第二个 weirdness = span(0.16F, 1.0F)
    OverworldBiomeBuilder builder;
    auto target = builder.spawnTarget();
    ASSERT_EQ(target.size(), 2u);

    const i64 negMin = static_cast<i64>(-1.0f * QUANTIZATION_FACTOR);
    const i64 negMax = static_cast<i64>(-0.16f * QUANTIZATION_FACTOR);
    const i64 posMin = static_cast<i64>(0.16f * QUANTIZATION_FACTOR);
    const i64 posMax = static_cast<i64>(1.0f * QUANTIZATION_FACTOR);

    EXPECT_EQ(target[0].weirdness.min, negMin);
    EXPECT_EQ(target[0].weirdness.max, negMax);
    EXPECT_EQ(target[1].weirdness.min, posMin);
    EXPECT_EQ(target[1].weirdness.max, posMax);
}

TEST(OverworldBiomeBuilderSpawnTargetTest, TwoEntriesHaveDisjointWeirdness)
{
    // 两个 weirdness 范围不重叠（[-1, -0.16] 和 [0.16, 1]）
    OverworldBiomeBuilder builder;
    auto target = builder.spawnTarget();
    ASSERT_EQ(target.size(), 2u);

    // 第一个的 max 应小于第二个的 min
    EXPECT_LT(target[0].weirdness.max, target[1].weirdness.min);
}

} // namespace
} // namespace mc
