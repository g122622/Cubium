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
 * @file RTreeTest.cpp
 * @brief RTree 空间索引单元测试
 *
 * 测试覆盖：
 * 1. RTree 构建与搜索正确性
 * 2. RTree 搜索结果与暴力搜索一致性
 * 3. 单条目、少量条目、大量条目场景
 * 4. ParameterList 集成测试（findValue 与 findValueBruteForce 一致性）
 * 5. RTree 缓存优化验证
 */

#include "common/world/biome/climate/RTree.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/climate/ParameterList.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace world::biome::climate {
namespace {

// ============================================================================
// RTree 构建与搜索测试
// ============================================================================

TEST(RTreeTest, SingleEntryReturnsCorrectValue)
{
    std::vector<ParameterList<i32>::Entry> entries;
    entries.emplace_back(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 42);

    RTree<i32> tree = RTree<i32>::create(entries);

    auto target = TargetPoint::fromFloats(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target), 42);

    auto target2 = TargetPoint::fromFloats(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target2), 42);
}

TEST(RTreeTest, FewEntriesFindNearest)
{
    std::vector<ParameterList<i32>::Entry> entries;
    entries.emplace_back(pointParameters(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 1);
    entries.emplace_back(pointParameters(0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 2);
    entries.emplace_back(pointParameters(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 3);

    RTree<i32> tree = RTree<i32>::create(entries);

    // 目标 (0.5, 0, 0, 0, 0, 0) 最近的是 entry 3
    auto target = TargetPoint::fromFloats(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target), 3);

    // 目标 (-1.0, 0, 0, 0, 0, 0) 最近的是 entry 1
    auto target2 = TargetPoint::fromFloats(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target2), 1);

    // 目标 (0.8, 0, 0, 0, 0, 0) 最近的是 entry 2
    auto target3 = TargetPoint::fromFloats(0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target3), 2);
}

TEST(RTreeTest, RangeParametersMatch)
{
    std::vector<ParameterList<i32>::Entry> entries;
    // 温度范围 [-1, -0.5]
    entries.emplace_back(parameters(Parameter::span(-1.0f, -0.5f),
                             Parameter::fullRange(),
                             Parameter::fullRange(),
                             Parameter::fullRange(),
                             Parameter::fullRange(),
                             Parameter::fullRange(),
                             0.0f),
        1);
    // 温度范围 [0.5, 1.0]
    entries.emplace_back(parameters(Parameter::span(0.5f, 1.0f),
                             Parameter::fullRange(),
                             Parameter::fullRange(),
                             Parameter::fullRange(),
                             Parameter::fullRange(),
                             Parameter::fullRange(),
                             0.0f),
        2);

    RTree<i32> tree = RTree<i32>::create(entries);

    // 目标温度 = 0.75，应匹配 entry 2
    auto target = TargetPoint::fromFloats(0.75f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target), 2);

    // 目标温度 = -0.75，应匹配 entry 1
    auto target2 = TargetPoint::fromFloats(-0.75f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target2), 1);
}

TEST(RTreeTest, OffsetAffectsResult)
{
    std::vector<ParameterList<i32>::Entry> entries;
    // offset = 0
    entries.emplace_back(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 1);
    // offset = 0.5
    entries.emplace_back(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f), 2);

    RTree<i32> tree = RTree<i32>::create(entries);

    // 目标 offset 为 0，应匹配 entry 1（offset 更小）
    auto target = TargetPoint::fromFloats(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target), 1);
}

TEST(RTreeTest, MultipleDimensionsAffectResult)
{
    std::vector<ParameterList<i32>::Entry> entries;
    entries.emplace_back(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 1);
    entries.emplace_back(pointParameters(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 2);
    entries.emplace_back(pointParameters(0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f), 3);

    RTree<i32> tree = RTree<i32>::create(entries);

    // 目标 (0.1, 0.1) — 距离 entry 1 最近
    auto target = TargetPoint::fromFloats(0.1f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target), 1);

    // 目标 (0.9, 0.1) — 距离 entry 2 最近
    auto target2 = TargetPoint::fromFloats(0.9f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target2), 2);

    // 目标 (0.1, 0.9) — 距离 entry 3 最近
    auto target3 = TargetPoint::fromFloats(0.1f, 0.9f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target3), 3);
}

// ============================================================================
// RTree 与暴力搜索一致性测试
// ============================================================================

TEST(RTreeTest, MatchesBruteForceWithNetherBiomes)
{
    // 使用下界生物群系参数测试
    ParameterList<BiomeId> list;
    list.add(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::NetherWastes);
    list.add(pointParameters(0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::SoulSandValley);
    list.add(pointParameters(0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::CrimsonForest);
    list.add(pointParameters(0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.375f), Biomes::WarpedForest);
    list.add(pointParameters(-0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.175f), Biomes::BasaltDeltas);

    // 测试多个目标点，避开可能产生平局的区域
    // 平局时 RTree 和暴力搜索可能返回不同条目（均为合法最近邻）
    auto hotNeutral = TargetPoint::fromFloats(0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(hotNeutral), list.findValueBruteForce(hotNeutral));
    EXPECT_EQ(list.findValue(hotNeutral), Biomes::CrimsonForest);

    auto coldDry = TargetPoint::fromFloats(-0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(coldDry), list.findValueBruteForce(coldDry));
    EXPECT_EQ(list.findValue(coldDry), Biomes::BasaltDeltas);

    auto neutralWet = TargetPoint::fromFloats(0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(neutralWet), list.findValueBruteForce(neutralWet));
    EXPECT_EQ(list.findValue(neutralWet), Biomes::WarpedForest);

    auto neutralDry = TargetPoint::fromFloats(0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(neutralDry), list.findValueBruteForce(neutralDry));
    EXPECT_EQ(list.findValue(neutralDry), Biomes::SoulSandValley);

    auto neutralNeutral = TargetPoint::fromFloats(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(neutralNeutral), list.findValueBruteForce(neutralNeutral));
    EXPECT_EQ(list.findValue(neutralNeutral), Biomes::NetherWastes);
}

TEST(RTreeTest, MatchesBruteForceWithManyEntries)
{
    // 创建大量条目以测试大列表构建
    // 使用奇数偏移避免平局
    std::vector<ParameterList<i32>::Entry> entries;
    for (i32 i = 0; i < 50; ++i) {
        f32 temp = -1.0f + i * 0.04f + 0.01f; // 加小偏移避免平局
        f32 humid = -1.0f + (i % 10) * 0.2f + 0.01f;
        entries.emplace_back(pointParameters(temp, humid, 0.0f, 0.0f, 0.0f, 0.0f), i);
    }

    ParameterList<i32> list(std::move(entries));

    // 测试多个目标点
    for (f32 temp = -1.0f; temp <= 1.0f; temp += 0.3f) {
        for (f32 humid = -1.0f; humid <= 1.0f; humid += 0.3f) {
            auto target = TargetPoint::fromFloats(temp, humid, 0.0f, 0.0f, 0.0f, 0.0f);
            EXPECT_EQ(list.findValue(target), list.findValueBruteForce(target))
                << "Mismatch at temp=" << temp << " humid=" << humid;
        }
    }
}

TEST(RTreeTest, MatchesBruteForceWithRangeParameters)
{
    // 使用范围参数而非点参数
    std::vector<ParameterList<i32>::Entry> entries;
    for (i32 i = 0; i < 20; ++i) {
        f32 tempMin = -1.0f + i * 0.1f;
        f32 tempMax = tempMin + 0.1f;
        entries.emplace_back(parameters(Parameter::span(tempMin, tempMax),
                                 Parameter::fullRange(),
                                 Parameter::fullRange(),
                                 Parameter::fullRange(),
                                 Parameter::fullRange(),
                                 Parameter::fullRange(),
                                 static_cast<f32>(i) * 0.01f),
            i);
    }

    ParameterList<i32> list(std::move(entries));

    for (f32 temp = -1.0f; temp <= 1.0f; temp += 0.2f) {
        auto target = TargetPoint::fromFloats(temp, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        EXPECT_EQ(list.findValue(target), list.findValueBruteForce(target)) << "Mismatch at temp=" << temp;
    }
}

// ============================================================================
// ParameterList 集成测试
// ============================================================================

TEST(RTreeParameterListTest, SingleEntryFindValue)
{
    ParameterList<i32> list;
    list.add(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 42);

    auto target = TargetPoint::fromFloats(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(target), 42);
    EXPECT_EQ(list.findValueBruteForce(target), 42);
}

TEST(RTreeParameterListTest, FindNearestEntry)
{
    ParameterList<i32> list;
    list.add(pointParameters(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 1);
    list.add(pointParameters(0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 2);
    list.add(pointParameters(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 3);

    auto target = TargetPoint::fromFloats(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(target), 3);
    EXPECT_EQ(list.findValueBruteForce(target), 3);
}

TEST(RTreeParameterListTest, ConstructorWithEntries)
{
    std::vector<ParameterList<i32>::Entry> entries;
    entries.emplace_back(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 1);
    entries.emplace_back(pointParameters(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 2);

    ParameterList<i32> list(std::move(entries));

    auto target = TargetPoint::fromFloats(0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(target), 2);
    EXPECT_EQ(list.findValueBruteForce(target), 2);
}

TEST(RTreeParameterListTest, NetherBiomeValues)
{
    ParameterList<BiomeId> list;
    list.add(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::NetherWastes);
    list.add(pointParameters(0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::SoulSandValley);
    list.add(pointParameters(0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::CrimsonForest);
    list.add(pointParameters(0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.375f), Biomes::WarpedForest);
    list.add(pointParameters(-0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.175f), Biomes::BasaltDeltas);

    // 验证 RTree 结果与暴力搜索一致
    auto hotNeutral = TargetPoint::fromFloats(0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(hotNeutral), list.findValueBruteForce(hotNeutral));
    EXPECT_EQ(list.findValue(hotNeutral), Biomes::CrimsonForest);

    auto coldDry = TargetPoint::fromFloats(-0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(coldDry), list.findValueBruteForce(coldDry));
    EXPECT_EQ(list.findValue(coldDry), Biomes::BasaltDeltas);

    auto neutralWet = TargetPoint::fromFloats(0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(neutralWet), list.findValueBruteForce(neutralWet));
    EXPECT_EQ(list.findValue(neutralWet), Biomes::WarpedForest);

    auto neutralDry = TargetPoint::fromFloats(0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(neutralDry), list.findValueBruteForce(neutralDry));
    EXPECT_EQ(list.findValue(neutralDry), Biomes::SoulSandValley);

    auto neutralNeutral = TargetPoint::fromFloats(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(list.findValue(neutralNeutral), list.findValueBruteForce(neutralNeutral));
    EXPECT_EQ(list.findValue(neutralNeutral), Biomes::NetherWastes);
}

// ============================================================================
// RTree 缓存优化测试
// ============================================================================

TEST(RTreeTest, CacheImprovesRepeatedSearch)
{
    std::vector<ParameterList<i32>::Entry> entries;
    entries.emplace_back(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 1);
    entries.emplace_back(pointParameters(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 2);

    RTree<i32> tree = RTree<i32>::create(entries);

    // 多次搜索同一目标（0.8 远离 entry 1，明显最近 entry 2），验证缓存不影响结果
    auto target = TargetPoint::fromFloats(0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    for (i32 i = 0; i < 10; ++i) {
        EXPECT_EQ(tree.search(target), 2);
    }

    // 搜索另一个明显最近的目标
    auto target2 = TargetPoint::fromFloats(-0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(tree.search(target2), 1);

    // 再搜索原始目标
    EXPECT_EQ(tree.search(target), 2);
}

// ============================================================================
// 大规模压力测试
// ============================================================================

TEST(RTreeTest, LargeScaleMatchesBruteForce)
{
    // 创建 100 个随机分布的条目
    std::vector<ParameterList<i32>::Entry> entries;
    for (i32 i = 0; i < 100; ++i) {
        f32 temp = -1.0f + (i % 10) * 0.2f;
        f32 humid = -1.0f + (i / 10) * 0.2f;
        f32 cont = -1.0f + (i % 5) * 0.4f;
        f32 ero = -1.0f + (i % 4) * 0.5f;
        f32 depth = -1.0f + (i % 3) * 0.7f;
        f32 weird = -1.0f + (i % 7) * 0.3f;
        entries.emplace_back(pointParameters(temp, humid, cont, ero, depth, weird), i);
    }

    ParameterList<i32> list(std::move(entries));

    // 测试多个目标点
    for (f32 temp = -1.0f; temp <= 1.0f; temp += 0.5f) {
        for (f32 humid = -1.0f; humid <= 1.0f; humid += 0.5f) {
            auto target = TargetPoint::fromFloats(temp, humid, 0.0f, 0.0f, 0.0f, 0.0f);
            EXPECT_EQ(list.findValue(target), list.findValueBruteForce(target))
                << "Mismatch at temp=" << temp << " humid=" << humid;
        }
    }
}

// ============================================================================
// Fitness 一致性验证
// ============================================================================

TEST(RTreeTest, RTreeSearchReturnsOptimalFitness)
{
    // 验证 RTree 搜索结果与最小 fitness 一致
    std::vector<ParameterList<i32>::Entry> entries;
    entries.emplace_back(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 1);
    entries.emplace_back(pointParameters(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 2);
    entries.emplace_back(pointParameters(0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 3);
    entries.emplace_back(pointParameters(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), 4);

    ParameterList<i32> list(std::move(entries));

    // 对多个目标点验证 RTree 搜索结果的 fitness 不大于暴力搜索的 fitness
    for (f32 temp = -1.0f; temp <= 1.0f; temp += 0.4f) {
        for (f32 humid = -1.0f; humid <= 1.0f; humid += 0.4f) {
            auto target = TargetPoint::fromFloats(temp, humid, 0.0f, 0.0f, 0.0f, 0.0f);
            const auto& rtreeResult = list.findValue(target);
            const auto& bruteResult = list.findValueBruteForce(target);

            // 找到对应条目的 fitness
            i64 rtreeFitness = std::numeric_limits<i64>::max();
            i64 bruteFitness = std::numeric_limits<i64>::max();
            for (const auto& entry : list.entries()) {
                i64 fitness = entry.first.fitness(target);
                if (entry.second == rtreeResult) {
                    rtreeFitness = fitness;
                }
                if (entry.second == bruteResult) {
                    bruteFitness = fitness;
                }
            }

            // RTree 结果的 fitness 应该等于暴力搜索的 fitness（都是最近邻）
            EXPECT_EQ(rtreeFitness, bruteFitness) << "Fitness mismatch at temp=" << temp << " humid=" << humid
                                                  << " rtree=" << rtreeResult << " brute=" << bruteResult;
        }
    }
}

} // namespace
} // namespace world::biome::climate
} // namespace mc
