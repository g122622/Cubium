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
 * The above copyright notice shall be included in all copies or substantial
 * portions of the Software.
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
 * @file test_pos_rule_test.cpp
 * @brief PosRuleTest 及 ProcessorListLoader 位置谓词解析单元测试
 */

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/template/RuleTest.hpp"
#include "common/world/gen/jigsaw/ProcessorListLoader.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::gen::feature::template_;
using namespace mc::world::gen::jigsaw;
using namespace mc::math;

// ============================================================================
// PosRuleTest 基础类测试
// ============================================================================

class PosRuleTestTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    BlockPos originalPos{0, 0, 0};
    BlockPos seedPos{0, 0, 0};
};

// ============================================================================
// AlwaysTruePosRuleTest 测试
// ============================================================================

TEST_F(PosRuleTestTest, AlwaysTrueReturnsTrue)
{
    Random rng(12345);
    AlwaysTruePosRuleTest test;

    EXPECT_TRUE(test.test(originalPos, BlockPos{10, 20, 30}, seedPos, rng));
    EXPECT_TRUE(test.test(originalPos, BlockPos{-100, 0, 100}, seedPos, rng));
}

TEST_F(PosRuleTestTest, AlwaysTrueClone)
{
    AlwaysTruePosRuleTest test;
    auto clone = test.clone();

    EXPECT_NE(clone, nullptr);
    Random rng(42);
    EXPECT_TRUE(clone->test(originalPos, BlockPos{5, 5, 5}, seedPos, rng));
}

// ============================================================================
// LinearPosRuleTest 测试
// ============================================================================

TEST_F(PosRuleTestTest, LinearPosMinDistanceZeroProbability)
{
    // 在 min_dist 处，概率为 min_probability = 0.0，不应匹配
    Random rng(12345);
    LinearPosRuleTest test(/*minDistance=*/0, /*maxDistance=*/10, /*minProbability=*/0.0f, /*maxProbability=*/1.0f);

    // seedPos 在原点，worldPos 也在原点（距离 0），概率为 0.0
    int matches = 0;
    for (int i = 0; i < 100; ++i) {
        if (test.test(originalPos, BlockPos{0, 0, 0}, seedPos, rng)) {
            matches++;
        }
    }
    // 概率 0.0 应该几乎不匹配（允许极少量误差）
    EXPECT_LT(matches, 5);
}

TEST_F(PosRuleTestTest, LinearPosMaxDistanceFullProbability)
{
    // 在 max_dist 处，概率为 max_probability = 1.0，应总是匹配
    Random rng(12345);
    LinearPosRuleTest test(/*minDistance=*/0, /*maxDistance=*/10, /*minProbability=*/0.0f, /*maxProbability=*/1.0f);

    // 曼哈顿距离 30（超过 max_dist=10），概率 clamp 到 1.0
    int matches = 0;
    for (int i = 0; i < 100; ++i) {
        if (test.test(originalPos, BlockPos{10, 10, 10}, seedPos, rng)) {
            matches++;
        }
    }
    EXPECT_EQ(matches, 100);
}

TEST_F(PosRuleTestTest, LinearPosProbabilityDistribution)
{
    // 测试中间距离的概率分布
    Random rng(12345);
    LinearPosRuleTest test(/*minDistance=*/0, /*maxDistance=*/100, /*minProbability=*/0.0f, /*maxProbability=*/1.0f);

    // 在距离 50 处，概率应约为 0.5
    BlockPos worldPos{50, 0, 0};
    int matches = 0;
    const int trials = 1000;
    for (int i = 0; i < trials; ++i) {
        if (test.test(originalPos, worldPos, seedPos, rng)) {
            matches++;
        }
    }

    f32 actualProbability = static_cast<f32>(matches) / static_cast<f32>(trials);
    EXPECT_NEAR(actualProbability, 0.5f, 0.1f);
}

TEST_F(PosRuleTestTest, LinearPosClone)
{
    LinearPosRuleTest test(0, 100, 0.0f, 1.0f);
    auto clone = test.clone();

    EXPECT_NE(clone, nullptr);

    // 使用确定性概率（1.0）来测试克隆，避免随机数状态差异
    LinearPosRuleTest testFull(/*minDistance=*/0, /*maxDistance=*/10, /*minProbability=*/1.0f, /*maxProbability=*/1.0f);
    auto cloneFull = testFull.clone();

    Random rng(42);
    BlockPos worldPos{5, 0, 0};
    EXPECT_TRUE(testFull.test(originalPos, worldPos, seedPos, rng));
    EXPECT_TRUE(cloneFull->test(originalPos, worldPos, seedPos, rng));
}

TEST_F(PosRuleTestTest, LinearPosManhattanDistance)
{
    // LinearPosRuleTest 使用曼哈顿距离
    Random rng(12345);
    LinearPosRuleTest test(/*minDistance=*/0, /*maxDistance=*/10, /*minProbability=*/1.0f, /*maxProbability=*/1.0f);

    // 曼哈顿距离为 3+4+5=12 > max_dist=10，概率 clamp 到 1.0
    EXPECT_TRUE(test.test(originalPos, BlockPos{3, 4, 5}, seedPos, rng));
}

// ============================================================================
// AxisAlignedLinearPosTest 测试
// ============================================================================

TEST_F(PosRuleTestTest, AxisAlignedLinearYAxis)
{
    // 只测量 Y 轴距离
    Random rng(12345);
    AxisAlignedLinearPosTest test(/*minProbability=*/0.0f,
        /*maxProbability=*/1.0f,
        /*minDistance=*/0,
        /*maxDistance=*/100,
        Axis::Y);

    // Y 距离 50，概率约 0.5
    int matches = 0;
    const int trials = 1000;
    for (int i = 0; i < trials; ++i) {
        if (test.test(originalPos, BlockPos{1000, 50, 1000}, seedPos, rng)) {
            matches++;
        }
    }

    f32 actualProbability = static_cast<f32>(matches) / static_cast<f32>(trials);
    EXPECT_NEAR(actualProbability, 0.5f, 0.1f);
}

TEST_F(PosRuleTestTest, AxisAlignedLinearXAxis)
{
    // 只测量 X 轴距离
    Random rng(12345);
    AxisAlignedLinearPosTest test(/*minProbability=*/0.0f,
        /*maxProbability=*/1.0f,
        /*minDistance=*/0,
        /*maxDistance=*/100,
        Axis::X);

    // X 距离 50，概率约 0.5；Y 和 Z 的距离不影响
    int matches = 0;
    const int trials = 1000;
    for (int i = 0; i < trials; ++i) {
        if (test.test(originalPos, BlockPos{50, 9999, 9999}, seedPos, rng)) {
            matches++;
        }
    }

    f32 actualProbability = static_cast<f32>(matches) / static_cast<f32>(trials);
    EXPECT_NEAR(actualProbability, 0.5f, 0.1f);
}

TEST_F(PosRuleTestTest, AxisAlignedLinearZAxis)
{
    // 只测量 Z 轴距离
    Random rng(12345);
    AxisAlignedLinearPosTest test(/*minProbability=*/0.0f,
        /*maxProbability=*/1.0f,
        /*minDistance=*/0,
        /*maxDistance=*/100,
        Axis::Z);

    // Z 距离 50，概率约 0.5
    int matches = 0;
    const int trials = 1000;
    for (int i = 0; i < trials; ++i) {
        if (test.test(originalPos, BlockPos{0, 0, 50}, seedPos, rng)) {
            matches++;
        }
    }

    f32 actualProbability = static_cast<f32>(matches) / static_cast<f32>(trials);
    EXPECT_NEAR(actualProbability, 0.5f, 0.1f);
}

TEST_F(PosRuleTestTest, AxisAlignedLinearNonZeroSeedPos)
{
    // 测试非零 seedPos 的情况
    Random rng(12345);
    AxisAlignedLinearPosTest test(/*minProbability=*/0.0f,
        /*maxProbability=*/1.0f,
        /*minDistance=*/0,
        /*maxDistance=*/100,
        Axis::Y);

    // seedPos 在 (0, 100, 0)，worldPos 在 (0, 150, 0)
    // Y 轴距离 = |150 - 100| = 50，概率约 0.5
    BlockPos nonZeroSeedPos{0, 100, 0};
    int matches = 0;
    const int trials = 1000;
    for (int i = 0; i < trials; ++i) {
        if (test.test(originalPos, BlockPos{0, 150, 0}, nonZeroSeedPos, rng)) {
            matches++;
        }
    }

    f32 actualProbability = static_cast<f32>(matches) / static_cast<f32>(trials);
    EXPECT_NEAR(actualProbability, 0.5f, 0.1f);
}

TEST_F(PosRuleTestTest, AxisAlignedLinearClone)
{
    AxisAlignedLinearPosTest test(0.0f, 1.0f, 0, 100, Axis::Y);
    auto clone = test.clone();

    EXPECT_NE(clone, nullptr);

    // 使用确定性概率（1.0）来测试克隆，避免随机数状态差异
    AxisAlignedLinearPosTest testFull(/*minProbability=*/1.0f,
        /*maxProbability=*/1.0f,
        /*minDistance=*/0,
        /*maxDistance=*/10,
        Axis::Y);
    auto cloneFull = testFull.clone();

    Random rng(42);
    BlockPos worldPos{0, 5, 0};
    EXPECT_TRUE(testFull.test(originalPos, worldPos, seedPos, rng));
    EXPECT_TRUE(cloneFull->test(originalPos, worldPos, seedPos, rng));
}

// ============================================================================
// ProcessorListLoader 位置谓词 JSON 解析测试
// ============================================================================

class ProcessorListPosPredicateTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(ProcessorListPosPredicateTest, ParseAlwaysTruePosPredicate)
{
    // pos_predicate 为 always_true
    const std::string json = R"({
        "processors": [
            {
                "processor_type": "minecraft:rule",
                "rules": [
                    {
                        "input_predicate": { "predicate_type": "minecraft:always_true" },
                        "location_predicate": { "predicate_type": "minecraft:always_true" },
                        "position_predicate": { "predicate_type": "minecraft:always_true" },
                        "output_state": { "Name": "minecraft:air" }
                    }
                ]
            }
        ]
    })";

    auto result = ProcessorListLoader::loadFromJson(json, ResourceLocation("minecraft", "test/always_true_pos"));
    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
}

TEST_F(ProcessorListPosPredicateTest, ParseLinearPosPredicate)
{
    // pos_predicate 为 linear_pos
    const std::string json = R"({
        "processors": [
            {
                "processor_type": "minecraft:rule",
                "rules": [
                    {
                        "input_predicate": { "predicate_type": "minecraft:always_true" },
                        "location_predicate": { "predicate_type": "minecraft:always_true" },
                        "position_predicate": {
                            "predicate_type": "minecraft:linear_pos",
                            "min_chance": 0.0,
                            "max_chance": 1.0,
                            "min_dist": 0,
                            "max_dist": 10
                        },
                        "output_state": { "Name": "minecraft:air" }
                    }
                ]
            }
        ]
    })";

    auto result = ProcessorListLoader::loadFromJson(json, ResourceLocation("minecraft", "test/linear_pos"));
    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
}

TEST_F(ProcessorListPosPredicateTest, ParseAxisAlignedLinearPosPredicate)
{
    // pos_predicate 为 axis_aligned_linear_pos — 与原版 high_rampart.json 相同的格式
    const std::string json = R"({
        "processors": [
            {
                "processor_type": "minecraft:rule",
                "rules": [
                    {
                        "input_predicate": { "predicate_type": "minecraft:always_true" },
                        "location_predicate": { "predicate_type": "minecraft:always_true" },
                        "output_state": { "Name": "minecraft:air" },
                        "position_predicate": {
                            "predicate_type": "minecraft:axis_aligned_linear_pos",
                            "min_chance": 0.0,
                            "max_chance": 0.05,
                            "min_dist": 0,
                            "max_dist": 100,
                            "axis": "y"
                        }
                    }
                ]
            }
        ]
    })";

    auto result =
        ProcessorListLoader::loadFromJson(json, ResourceLocation("minecraft", "test/axis_aligned_linear_pos"));
    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
}

TEST_F(ProcessorListPosPredicateTest, ParseAxisAlignedLinearPosWithXAxis)
{
    // 测试 X 轴
    const std::string json = R"({
        "processors": [
            {
                "processor_type": "minecraft:rule",
                "rules": [
                    {
                        "input_predicate": { "predicate_type": "minecraft:always_true" },
                        "location_predicate": { "predicate_type": "minecraft:always_true" },
                        "position_predicate": {
                            "predicate_type": "minecraft:axis_aligned_linear_pos",
                            "min_chance": 0.1,
                            "max_chance": 0.9,
                            "min_dist": 5,
                            "max_dist": 50,
                            "axis": "x"
                        },
                        "output_state": { "Name": "minecraft:stone" }
                    }
                ]
            }
        ]
    })";

    auto result =
        ProcessorListLoader::loadFromJson(json, ResourceLocation("minecraft", "test/axis_aligned_linear_pos_x"));
    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
}

TEST_F(ProcessorListPosPredicateTest, ParseAxisAlignedLinearPosWithZAxis)
{
    // 测试 Z 轴
    const std::string json = R"({
        "processors": [
            {
                "processor_type": "minecraft:rule",
                "rules": [
                    {
                        "input_predicate": { "predicate_type": "minecraft:always_true" },
                        "location_predicate": { "predicate_type": "minecraft:always_true" },
                        "position_predicate": {
                            "predicate_type": "minecraft:axis_aligned_linear_pos",
                            "min_chance": 0.2,
                            "max_chance": 0.8,
                            "min_dist": 3,
                            "max_dist": 20,
                            "axis": "z"
                        },
                        "output_state": { "Name": "minecraft:stone" }
                    }
                ]
            }
        ]
    })";

    auto result =
        ProcessorListLoader::loadFromJson(json, ResourceLocation("minecraft", "test/axis_aligned_linear_pos_z"));
    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
}

TEST_F(ProcessorListPosPredicateTest, ParseLinearPosWithDefaultValues)
{
    // 测试 linear_pos 使用默认值（省略 min_chance/max_chance/min_dist/max_dist）
    const std::string json = R"({
        "processors": [
            {
                "processor_type": "minecraft:rule",
                "rules": [
                    {
                        "input_predicate": { "predicate_type": "minecraft:always_true" },
                        "location_predicate": { "predicate_type": "minecraft:always_true" },
                        "position_predicate": {
                            "predicate_type": "minecraft:linear_pos"
                        },
                        "output_state": { "Name": "minecraft:air" }
                    }
                ]
            }
        ]
    })";

    // min_dist=0 和 max_dist=0 时 min_dist >= max_dist，应回退到 always_true
    auto result = ProcessorListLoader::loadFromJson(json, ResourceLocation("minecraft", "test/linear_pos_defaults"));
    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
}

TEST_F(ProcessorListPosPredicateTest, ParseAxisAlignedLinearPosDefaultAxis)
{
    // 测试省略 axis 字段（默认 Y）
    const std::string json = R"({
        "processors": [
            {
                "processor_type": "minecraft:rule",
                "rules": [
                    {
                        "input_predicate": { "predicate_type": "minecraft:always_true" },
                        "location_predicate": { "predicate_type": "minecraft:always_true" },
                        "position_predicate": {
                            "predicate_type": "minecraft:axis_aligned_linear_pos",
                            "min_chance": 0.0,
                            "max_chance": 1.0,
                            "min_dist": 0,
                            "max_dist": 10
                        },
                        "output_state": { "Name": "minecraft:air" }
                    }
                ]
            }
        ]
    })";

    auto result = ProcessorListLoader::loadFromJson(
        json, ResourceLocation("minecraft", "test/axis_aligned_linear_pos_default_axis"));
    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
}

TEST_F(ProcessorListPosPredicateTest, ParseUnknownPosPredicateTypeFallsBackToAlwaysTrue)
{
    // 未知 predicate_type 应回退到 always_true
    const std::string json = R"({
        "processors": [
            {
                "processor_type": "minecraft:rule",
                "rules": [
                    {
                        "input_predicate": { "predicate_type": "minecraft:always_true" },
                        "location_predicate": { "predicate_type": "minecraft:always_true" },
                        "position_predicate": {
                            "predicate_type": "minecraft:unknown_pos_type"
                        },
                        "output_state": { "Name": "minecraft:air" }
                    }
                ]
            }
        ]
    })";

    auto result = ProcessorListLoader::loadFromJson(json, ResourceLocation("minecraft", "test/unknown_pos_predicate"));
    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
}

TEST_F(ProcessorListPosPredicateTest, ParseMissingPredicateTypeFallsBackToAlwaysTrue)
{
    // 缺少 predicate_type 字段应回退到 always_true
    const std::string json = R"({
        "processors": [
            {
                "processor_type": "minecraft:rule",
                "rules": [
                    {
                        "input_predicate": { "predicate_type": "minecraft:always_true" },
                        "location_predicate": { "predicate_type": "minecraft:always_true" },
                        "position_predicate": {
                            "min_chance": 0.5
                        },
                        "output_state": { "Name": "minecraft:air" }
                    }
                ]
            }
        ]
    })";

    auto result =
        ProcessorListLoader::loadFromJson(json, ResourceLocation("minecraft", "test/missing_pos_predicate_type"));
    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
}

TEST_F(ProcessorListPosPredicateTest, ParseNoPosPredicate)
{
    // 不含 position_predicate 字段（应默认为 always_true）
    const std::string json = R"({
        "processors": [
            {
                "processor_type": "minecraft:rule",
                "rules": [
                    {
                        "input_predicate": { "predicate_type": "minecraft:always_true" },
                        "location_predicate": { "predicate_type": "minecraft:always_true" },
                        "output_state": { "Name": "minecraft:air" }
                    }
                ]
            }
        ]
    })";

    auto result = ProcessorListLoader::loadFromJson(json, ResourceLocation("minecraft", "test/no_pos_predicate"));
    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
}

TEST_F(ProcessorListPosPredicateTest, ParseHighRampartProcessorList)
{
    // 使用原版 high_rampart.json 的完整格式测试
    const std::string json = R"({
        "processors": [
            {
                "processor_type": "minecraft:rule",
                "rules": [
                    {
                        "input_predicate": {
                            "block": "minecraft:gold_block",
                            "predicate_type": "minecraft:random_block_match",
                            "probability": 0.3
                        },
                        "location_predicate": {
                            "predicate_type": "minecraft:always_true"
                        },
                        "output_state": {
                            "Name": "minecraft:cracked_polished_blackstone_bricks"
                        }
                    },
                    {
                        "input_predicate": {
                            "predicate_type": "minecraft:always_true"
                        },
                        "location_predicate": {
                            "predicate_type": "minecraft:always_true"
                        },
                        "output_state": {
                            "Name": "minecraft:air"
                        },
                        "position_predicate": {
                            "axis": "y",
                            "max_chance": 0.05,
                            "max_dist": 100,
                            "min_chance": 0.0,
                            "min_dist": 0,
                            "predicate_type": "minecraft:axis_aligned_linear_pos"
                        }
                    },
                    {
                        "input_predicate": {
                            "block": "minecraft:gilded_blackstone",
                            "predicate_type": "minecraft:random_block_match",
                            "probability": 0.5
                        },
                        "location_predicate": {
                            "predicate_type": "minecraft:always_true"
                        },
                        "output_state": {
                            "Name": "minecraft:blackstone"
                        }
                    }
                ]
            }
        ]
    })";

    auto result = ProcessorListLoader::loadFromJson(json, ResourceLocation("minecraft", "test/high_rampart"));
    ASSERT_TRUE(result.success()) << "Failed to load: " << result.error().message();
}
