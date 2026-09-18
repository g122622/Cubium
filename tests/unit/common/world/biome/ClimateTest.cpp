/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

/**
 * @file ClimateTest.cpp
 * @brief Climate 参数系统单元测试
 *
 * 测试覆盖：
 * 1. Parameter 构造、量化、距离计算
 * 2. TargetPoint 量化/反量化
 * 3. ParameterPoint fitness 计算
 * 4. ParameterList 最近邻搜索
 * 5. quantizeCoord/unquantizeCoord 精度
 * 6. 辅助函数 parameters() / pointParameters()
 */

#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/climate/ParameterList.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include <limits>
#include <gtest/gtest.h>

namespace mc {
namespace world::biome::climate {
namespace {

// ============================================================================
// Parameter 测试
// ============================================================================

TEST(ClimateParameterTest, PointCreatesSingleValueRange)
{
    auto p = Parameter::point(0.5f);
    EXPECT_EQ(p.min, p.max);
    EXPECT_EQ(p.min, quantizeCoord(0.5f));
}

TEST(ClimateParameterTest, PointAtZero)
{
    auto p = Parameter::point(0.0f);
    EXPECT_EQ(p.min, 0);
    EXPECT_EQ(p.max, 0);
}

TEST(ClimateParameterTest, PointAtNegativeValue)
{
    auto p = Parameter::point(-1.0f);
    EXPECT_EQ(p.min, p.max);
    EXPECT_EQ(p.min, quantizeCoord(-1.0f));
}

TEST(ClimateParameterTest, SpanCreatesRange)
{
    auto p = Parameter::span(-1.0f, 1.0f);
    EXPECT_EQ(p.min, quantizeCoord(-1.0f));
    EXPECT_EQ(p.max, quantizeCoord(1.0f));
}

TEST(ClimateParameterTest, SpanFromTwoParameters)
{
    auto first = Parameter::span(-1.0f, 0.0f);
    auto second = Parameter::span(0.5f, 1.5f);
    auto merged = Parameter::span(first, second);
    EXPECT_EQ(merged.min, first.min);
    EXPECT_EQ(merged.max, second.max);
}

TEST(ClimateParameterTest, FullRangeIsMinus2To2)
{
    auto fr = Parameter::fullRange();
    EXPECT_EQ(fr.min, quantizeCoord(-2.0f));
    EXPECT_EQ(fr.max, quantizeCoord(2.0f));
}

TEST(ClimateParameterTest, DistanceValueInRangeReturnsZero)
{
    auto p = Parameter::span(-1.0f, 1.0f);
    // 0.0 量化后在 [-10000, 10000] 范围内
    EXPECT_EQ(p.distance(quantizeCoord(0.0f)), 0);
    EXPECT_EQ(p.distance(quantizeCoord(-0.5f)), 0);
    EXPECT_EQ(p.distance(quantizeCoord(0.99f)), 0);
}

TEST(ClimateParameterTest, DistanceValueBelowRangeReturnsPositive)
{
    auto p = Parameter::span(-1.0f, 1.0f);
    // -2.0 量化后 < -10000（p.min），距离应为 p.min - quantizeCoord(-2.0f)
    i64 dist = p.distance(quantizeCoord(-2.0f));
    EXPECT_GT(dist, 0);
    // 验证：p.min = quantizeCoord(-1.0f) = -10000, value = quantizeCoord(-2.0f) = -20000
    // dist = p.min - value = -10000 - (-20000) = 10000
    EXPECT_EQ(dist, p.min - quantizeCoord(-2.0f));
}

TEST(ClimateParameterTest, DistanceValueAboveRangeReturnsPositive)
{
    auto p = Parameter::span(-1.0f, 1.0f);
    i64 dist = p.distance(quantizeCoord(2.0f));
    EXPECT_GT(dist, 0);
    EXPECT_EQ(dist, quantizeCoord(2.0f) - p.max);
}

TEST(ClimateParameterTest, DistanceAtExactBoundary)
{
    auto p = Parameter::span(-1.0f, 1.0f);
    EXPECT_EQ(p.distance(p.min), 0);
    EXPECT_EQ(p.distance(p.max), 0);
}

TEST(ClimateParameterTest, PointDistanceIsAlwaysZeroAtPoint)
{
    auto p = Parameter::point(0.5f);
    EXPECT_EQ(p.distance(quantizeCoord(0.5f)), 0);
}

TEST(ClimateParameterTest, PointDistanceNonZeroAwayFromPoint)
{
    auto p = Parameter::point(0.5f);
    i64 dist = p.distance(quantizeCoord(0.6f));
    EXPECT_GT(dist, 0);
}

TEST(ClimateParameterTest, Equality)
{
    auto a = Parameter::span(-1.0f, 1.0f);
    auto b = Parameter::span(-1.0f, 1.0f);
    auto c = Parameter::span(-0.5f, 0.5f);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

// ============================================================================
// quantizeCoord / unquantizeCoord 测试
// ============================================================================

TEST(ClimateQuantizeTest, QuantizeZeroIsZero)
{
    EXPECT_EQ(quantizeCoord(0.0f), 0);
}

TEST(ClimateQuantizeTest, QuantizeOneIsFactor)
{
    EXPECT_EQ(quantizeCoord(1.0f), 10000);
}

TEST(ClimateQuantizeTest, QuantizeNegativeOne)
{
    EXPECT_EQ(quantizeCoord(-1.0f), -10000);
}

TEST(ClimateQuantizeTest, QuantizeTwoIsFactorTimesTwo)
{
    EXPECT_EQ(quantizeCoord(2.0f), 20000);
}

TEST(ClimateQuantizeTest, QuantizeRoundTrip)
{
    // 量化后反量化应在浮点精度内一致
    for (f32 val : {-2.0f, -1.0f, -0.5f, 0.0f, 0.25f, 0.5f, 1.0f, 1.5f, 2.0f}) {
        i64 q = quantizeCoord(val);
        f32 restored = unquantizeCoord(q);
        EXPECT_NEAR(restored, val, 0.001f) << "Round-trip failed for " << val;
    }
}

TEST(ClimateQuantizeTest, UnquantizeZeroIsZero)
{
    EXPECT_FLOAT_EQ(unquantizeCoord(0), 0.0f);
}

TEST(ClimateQuantizeTest, QuantizationFactorIs10000)
{
    EXPECT_FLOAT_EQ(QUANTIZATION_FACTOR, 10000.0f);
}

// ============================================================================
// TargetPoint 测试
// ============================================================================

TEST(ClimateTargetPointTest, FromFloatsQuantizesValues)
{
    auto tp = TargetPoint::fromFloats(1.0f, 0.5f, -1.0f, 0.0f, 0.25f, -0.5f);
    EXPECT_EQ(tp.temperature, quantizeCoord(1.0f));
    EXPECT_EQ(tp.humidity, quantizeCoord(0.5f));
    EXPECT_EQ(tp.continentalness, quantizeCoord(-1.0f));
    EXPECT_EQ(tp.erosion, quantizeCoord(0.0f));
    EXPECT_EQ(tp.depth, quantizeCoord(0.25f));
    EXPECT_EQ(tp.weirdness, quantizeCoord(-0.5f));
}

TEST(ClimateTargetPointTest, ToParameterArrayIncludesOffset)
{
    auto tp = TargetPoint::fromFloats(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    auto arr = tp.toParameterArray();
    EXPECT_EQ(arr.size(), 7u);
    EXPECT_EQ(arr[0], quantizeCoord(1.0f));
    EXPECT_EQ(arr[6], 0); // offset 始终为 0
}

// ============================================================================
// ParameterPoint 测试
// ============================================================================

TEST(ClimateParameterPointTest, FitnessExactMatchIsZero)
{
    // 当 TargetPoint 完全在 ParameterPoint 范围内时，所有 distance 为 0
    auto pp = ParameterPoint{
        Parameter::span(-1.0f, 1.0f), // temperature
        Parameter::span(-1.0f, 1.0f), // humidity
        Parameter::span(-1.0f, 1.0f), // continentalness
        Parameter::span(-1.0f, 1.0f), // erosion
        Parameter::span(-1.0f, 1.0f), // depth
        Parameter::span(-1.0f, 1.0f), // weirdness
        0                             // offset
    };

    auto tp = TargetPoint::fromFloats(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(pp.fitness(tp), 0);
}

TEST(ClimateParameterPointTest, FitnessNonZeroWhenOutOfRange)
{
    auto pp = ParameterPoint{Parameter::point(0.0f), // temperature 必须精确为 0
        Parameter::fullRange(),                      // 其他参数全范围
        Parameter::fullRange(),
        Parameter::fullRange(),
        Parameter::fullRange(),
        Parameter::fullRange(),
        0};

    // temperature = 1.0 应产生非零距离
    auto tp = TargetPoint::fromFloats(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    i64 fitness = pp.fitness(tp);
    EXPECT_GT(fitness, 0);

    // fitness 应为 temperature 距离的平方
    i64 dist = quantizeCoord(1.0f) - quantizeCoord(0.0f); // 10000
    EXPECT_EQ(fitness, dist * dist);
}

TEST(ClimateParameterPointTest, FitnessOffsetContributesToResult)
{
    auto pp1 = ParameterPoint{
        Parameter::fullRange(),
        Parameter::fullRange(),
        Parameter::fullRange(),
        Parameter::fullRange(),
        Parameter::fullRange(),
        Parameter::fullRange(),
        0 // offset = 0
    };

    auto pp2 = ParameterPoint{
        Parameter::fullRange(),
        Parameter::fullRange(),
        Parameter::fullRange(),
        Parameter::fullRange(),
        Parameter::fullRange(),
        Parameter::fullRange(),
        quantizeCoord(0.5f) // offset = 0.5
    };

    auto tp = TargetPoint::fromFloats(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    // pp1 完全匹配（offset=0，target offset 也为 0）
    EXPECT_EQ(pp1.fitness(tp), 0);

    // pp2 有 offset 贡献
    i64 fitness2 = pp2.fitness(tp);
    EXPECT_GT(fitness2, 0);
    EXPECT_EQ(fitness2, quantizeCoord(0.5f) * quantizeCoord(0.5f));
}

TEST(ClimateParameterPointTest, FitnessIsSumOfSquaredDistances)
{
    // 精确验证 fitness = sum(distance^2) for all 6 params + offset^2
    auto pp = ParameterPoint{
        Parameter::point(1.0f), // temperature: target 0.0 距离 = 10000
        Parameter::point(0.5f), // humidity: target 0.0 距离 = 5000
        Parameter::point(0.0f), // continentalness: 距离 = 0
        Parameter::point(0.0f), // erosion: 距离 = 0
        Parameter::point(0.0f), // depth: 距离 = 0
        Parameter::point(0.0f), // weirdness: 距离 = 0
        quantizeCoord(0.25f)    // offset = 2500
    };

    auto tp = TargetPoint::fromFloats(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    i64 fitness = pp.fitness(tp);

    i64 d_temp = 10000;
    i64 d_humid = 5000;
    i64 d_offset = 2500;
    i64 expected = d_temp * d_temp + d_humid * d_humid + d_offset * d_offset;
    EXPECT_EQ(fitness, expected);
}

TEST(ClimateParameterPointTest, Equality)
{
    auto pp1 = pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    auto pp2 = pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    auto pp3 = pointParameters(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(pp1, pp2);
    EXPECT_NE(pp1, pp3);
}

// ============================================================================
// parameters() / pointParameters() 辅助函数测试
// ============================================================================

TEST(ClimateHelpersTest, ParametersCreatesParameterPointWithRanges)
{
    auto pp = parameters(Parameter::span(-1.0f, 1.0f),
        Parameter::span(-1.0f, 1.0f),
        Parameter::span(-1.0f, 1.0f),
        Parameter::span(-1.0f, 1.0f),
        Parameter::span(-1.0f, 1.0f),
        Parameter::span(-1.0f, 1.0f),
        0.0f);

    EXPECT_EQ(pp.temperature.min, quantizeCoord(-1.0f));
    EXPECT_EQ(pp.temperature.max, quantizeCoord(1.0f));
    EXPECT_EQ(pp.offset, 0);
}

TEST(ClimateHelpersTest, PointParametersCreatesParameterPointWithPoints)
{
    auto pp = pointParameters(0.5f, -0.5f, 0.0f, 0.25f, -0.75f, 1.0f, 0.1f);

    EXPECT_EQ(pp.temperature.min, pp.temperature.max);
    EXPECT_EQ(pp.temperature.min, quantizeCoord(0.5f));
    EXPECT_EQ(pp.humidity.min, pp.humidity.max);
    EXPECT_EQ(pp.humidity.min, quantizeCoord(-0.5f));
    EXPECT_EQ(pp.offset, quantizeCoord(0.1f));
}

TEST(ClimateHelpersTest, PointParametersDefaultOffsetIsZero)
{
    auto pp = pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(pp.offset, 0);
}

// ============================================================================
// ParameterList 测试
// ============================================================================

TEST(ClimateParameterListTest, EmptyListAsserts)
{
    ParameterList<i32> list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0u);
    // findValue on empty list would assert, so we don't test it here
}

TEST(ClimateParameterListTest, SingleEntryFindValue)
{
    ParameterList<i32> list;
    list.add(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 42);

    auto target = TargetPoint::fromFloats(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(target), 42);
}

TEST(ClimateParameterListTest, FindNearestEntry)
{
    ParameterList<i32> list;
    list.add(pointParameters(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 1); // 远
    list.add(pointParameters(0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 2);  // 近
    list.add(pointParameters(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 3);  // 最近

    auto target = TargetPoint::fromFloats(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(target), 3);
}

TEST(ClimateParameterListTest, FindWithRangeParameters)
{
    ParameterList<i32> list;
    // 入口 1：温度范围 [-1, -0.5]
    list.add(parameters(Parameter::span(-1.0f, -0.5f),
                 Parameter::fullRange(),
                 Parameter::fullRange(),
                 Parameter::fullRange(),
                 Parameter::fullRange(),
                 Parameter::fullRange(),
                 0.0f),
        1);
    // 入口 2：温度范围 [0.5, 1.0]
    list.add(parameters(Parameter::span(0.5f, 1.0f),
                 Parameter::fullRange(),
                 Parameter::fullRange(),
                 Parameter::fullRange(),
                 Parameter::fullRange(),
                 Parameter::fullRange(),
                 0.0f),
        2);

    // 目标温度 = 0.75，应匹配入口 2
    auto target = TargetPoint::fromFloats(0.75f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(target), 2);

    // 目标温度 = -0.75，应匹配入口 1
    auto target2 = TargetPoint::fromFloats(-0.75f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(target2), 1);
}

TEST(ClimateParameterListTest, OffsetAffectsFindValue)
{
    ParameterList<i32> list;

    // 两个入口在其他参数上完全匹配，但 offset 不同
    list.add(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 1); // offset = 0
    list.add(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f), 2); // offset = 0.5

    // 目标 offset 为 0，应匹配入口 1（offset 更小）
    auto target = TargetPoint::fromFloats(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(target), 1);
}

TEST(ClimateParameterListTest, MultipleDimensionsAffectFindValue)
{
    ParameterList<i32> list;

    // 入口 1：temperature=0, humidity=0
    list.add(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 1);
    // 入口 2：temperature=1, humidity=0
    list.add(pointParameters(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 2);
    // 入口 3：temperature=0, humidity=1
    list.add(pointParameters(0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f), 3);

    // 目标 (0.1, 0.1) — 距离入口 1 最近
    auto target = TargetPoint::fromFloats(0.1f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(target), 1);

    // 目标 (0.9, 0.1) — 距离入口 2 最近
    auto target2 = TargetPoint::fromFloats(0.9f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(target2), 2);

    // 目标 (0.1, 0.9) — 距离入口 3 最近
    auto target3 = TargetPoint::fromFloats(0.1f, 0.9f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(target3), 3);
}

TEST(ClimateParameterListTest, NetherBiomeParameterValues)
{
    // 验证下界生物群系参数值与 MC 1.21.11 一致
    ParameterList<BiomeId> list;

    // NetherWastes: temp=0.0, humid=0.0, offset=0.0
    list.add(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::NetherWastes);

    // SoulSandValley: temp=0.0, humid=-0.5, offset=0.0
    list.add(pointParameters(0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::SoulSandValley);

    // CrimsonForest: temp=0.4, humid=0.0, offset=0.0
    list.add(pointParameters(0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::CrimsonForest);

    // WarpedForest: temp=0.0, humid=0.5, offset=0.375
    list.add(pointParameters(0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.375f), Biomes::WarpedForest);

    // BasaltDeltas: temp=-0.5, humid=0.0, offset=0.175
    list.add(pointParameters(-0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.175f), Biomes::BasaltDeltas);

    // 验证在下界参数空间中的最近邻匹配
    // 温度高、湿度中 -> CrimsonForest
    auto hotNeutral = TargetPoint::fromFloats(0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(hotNeutral), Biomes::CrimsonForest);

    // 温度低、湿度低 -> BasaltDeltas
    auto coldDry = TargetPoint::fromFloats(-0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(coldDry), Biomes::BasaltDeltas);

    // 温度中性、湿度高 -> WarpedForest (offset 使其更近)
    auto neutralWet = TargetPoint::fromFloats(0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(neutralWet), Biomes::WarpedForest);

    // 温度中性、湿度低 -> SoulSandValley
    auto neutralDry = TargetPoint::fromFloats(0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(neutralDry), Biomes::SoulSandValley);

    // 温度中性、湿度中性 -> NetherWastes
    auto neutralNeutral = TargetPoint::fromFloats(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(neutralNeutral), Biomes::NetherWastes);
}

TEST(ClimateParameterListTest, SizeAndEntries)
{
    ParameterList<i32> list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0u);

    list.add(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 1);
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.size(), 1u);

    list.add(pointParameters(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 2);
    EXPECT_EQ(list.size(), 2u);
}

TEST(ClimateParameterListTest, IterateEntries)
{
    ParameterList<i32> list;
    list.add(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 10);
    list.add(pointParameters(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 20);

    int count = 0;
    for (const auto& [pp, value] : list) {
        (void)pp;
        EXPECT_TRUE(value == 10 || value == 20);
        ++count;
    }
    EXPECT_EQ(count, 2);
}

// ============================================================================
// Fitness 与 MC 一致性验证测试
// ============================================================================

TEST(ClimateFitnessTest, FitnessMatchesManualCalculation)
{
    // 验证 fitness 计算与手动计算一致
    // 创建两个点参数：A(0.5, 0, 0, 0, 0, 0) 和 B(-0.5, 0, 0, 0, 0, 0)
    // 目标点 T(0.3, 0, 0, 0, 0, 0)
    // fitness(A, T) = (5000-3000)^2 = 4,000,000 (因为 A 是 point，距离 = |5000-3000| = 2000)
    // fitness(B, T) = (5000+3000)^2 = 64,000,000 (因为 B 是 point，距离 = |-5000-3000| = 8000)

    auto ppA = pointParameters(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    auto ppB = pointParameters(-0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    auto target = TargetPoint::fromFloats(0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    i64 fitnessA = ppA.fitness(target);
    i64 fitnessB = ppB.fitness(target);

    // A 更近
    EXPECT_LT(fitnessA, fitnessB);

    // 验证 fitness 值
    i64 distA = quantizeCoord(0.5f) - quantizeCoord(0.3f); // 2000
    EXPECT_EQ(fitnessA, distA * distA);                    // 4,000,000
}

TEST(ClimateFitnessTest, FullRangeParameterHasZeroDistanceForAllValues)
{
    auto fullRange = Parameter::fullRange();
    // fullRange 是 [-2.0, 2.0]，任何在范围内的量化值距离都为 0
    EXPECT_EQ(fullRange.distance(quantizeCoord(-1.5f)), 0);
    EXPECT_EQ(fullRange.distance(quantizeCoord(0.0f)), 0);
    EXPECT_EQ(fullRange.distance(quantizeCoord(1.9f)), 0);

    // 超出范围的值才有距离
    EXPECT_GT(fullRange.distance(quantizeCoord(3.0f)), 0);
    EXPECT_GT(fullRange.distance(quantizeCoord(-3.0f)), 0);
}

// ============================================================================
// 量化边界条件测试
// ============================================================================

TEST(ClimateQuantizeBoundaryTest, QuantizeNegativeTwo)
{
    // -2.0 是 fullRange 的最小值
    EXPECT_EQ(quantizeCoord(-2.0f), -20000);
}

TEST(ClimateQuantizeBoundaryTest, QuantizeTwo)
{
    // 2.0 是 fullRange 的最大值
    EXPECT_EQ(quantizeCoord(2.0f), 20000);
}

TEST(ClimateQuantizeBoundaryTest, SmallValueQuantization)
{
    // 确保小值量化不会丢失精度
    EXPECT_EQ(quantizeCoord(0.0001f), 1);
    EXPECT_EQ(quantizeCoord(-0.0001f), -1);
}

TEST(ClimateQuantizeBoundaryTest, ZeroValueQuantization)
{
    EXPECT_EQ(quantizeCoord(0.0f), 0);
    EXPECT_FLOAT_EQ(unquantizeCoord(0), 0.0f);
}

} // namespace
} // namespace world::biome::climate
} // namespace mc
