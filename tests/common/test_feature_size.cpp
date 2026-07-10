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

#include "common/world/gen/feature/parser/FeatureSizeParser.hpp"
#include "common/world/gen/feature/tree/TreeFeature.hpp"
#include "common/world/gen/feature/tree/featuresize/FeatureSize.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::world::gen::feature::parser;

// ============================================================================
// TwoLayersFeatureSize 测试
// ============================================================================

TEST(TwoLayersFeatureSizeTest, ReturnsLowerSizeBelowLimit)
{
    TwoLayersFeatureSize size(/*limit=*/1, /*lowerSize=*/0, /*upperSize=*/2);
    // y < limit (1) → lowerSize
    EXPECT_EQ(size.getSizeAtHeight(/*trunkHeight=*/6, /*y=*/0), 0);
}

TEST(TwoLayersFeatureSizeTest, ReturnsUpperSizeAtAndAboveLimit)
{
    TwoLayersFeatureSize size(/*limit=*/1, /*lowerSize=*/0, /*upperSize=*/2);
    // y >= limit (1) → upperSize
    EXPECT_EQ(size.getSizeAtHeight(/*trunkHeight=*/6, /*y=*/1), 2);
    EXPECT_EQ(size.getSizeAtHeight(/*trunkHeight=*/6, /*y=*/2), 2);
    EXPECT_EQ(size.getSizeAtHeight(/*trunkHeight=*/6, /*y=*/6), 2);
}

TEST(TwoLayersFeatureSizeTest, TypeIsTwoLayers)
{
    TwoLayersFeatureSize size(1, 0, 2);
    EXPECT_EQ(size.type(), FeatureSizeType::TwoLayers);
}

TEST(TwoLayersFeatureSizeTest, AccessorsReturnCorrectValues)
{
    TwoLayersFeatureSize size(/*limit=*/1, /*lowerSize=*/0, /*upperSize=*/2);
    EXPECT_EQ(size.limit(), 1);
    EXPECT_EQ(size.lowerSize(), 0);
    EXPECT_EQ(size.upperSize(), 2);
}

TEST(TwoLayersFeatureSizeTest, MinClippedHeightDefaultsToNullopt)
{
    TwoLayersFeatureSize size(1, 0, 2);
    EXPECT_FALSE(size.minClippedHeight().has_value());
}

TEST(TwoLayersFeatureSizeTest, MinClippedHeightPreservedWhenProvided)
{
    TwoLayersFeatureSize size(1, 0, 2, std::optional<i32>{4});
    ASSERT_TRUE(size.minClippedHeight().has_value());
    EXPECT_EQ(*size.minClippedHeight(), 4);
}

TEST(TwoLayersFeatureSizeTest, CloneProducesEqualObject)
{
    TwoLayersFeatureSize size(2, 1, 3, std::optional<i32>{5});
    auto cloned = size.clone();

    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->type(), FeatureSizeType::TwoLayers);
    auto* typed = dynamic_cast<TwoLayersFeatureSize*>(cloned.get());
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->limit(), 2);
    EXPECT_EQ(typed->lowerSize(), 1);
    EXPECT_EQ(typed->upperSize(), 3);
    ASSERT_TRUE(typed->minClippedHeight().has_value());
    EXPECT_EQ(*typed->minClippedHeight(), 5);
}

// ============================================================================
// ThreeLayersFeatureSize 测试
// ============================================================================

TEST(ThreeLayersFeatureSizeTest, ReturnsLowerSizeBelowLimit)
{
    ThreeLayersFeatureSize size(/*limit=*/1,
        /*upperLimit=*/1,
        /*lowerSize=*/0,
        /*middleSize=*/1,
        /*upperSize=*/2);
    EXPECT_EQ(size.getSizeAtHeight(/*trunkHeight=*/6, /*y=*/0), 0);
}

TEST(ThreeLayersFeatureSizeTest, ReturnsMiddleSizeBetweenLimitAndUpperThreshold)
{
    ThreeLayersFeatureSize size(/*limit=*/1,
        /*upperLimit=*/1,
        /*lowerSize=*/0,
        /*middleSize=*/1,
        /*upperSize=*/2);
    // trunkHeight=6, upperLimit=1 → upper threshold = 6 - 1 = 5
    // y in [1, 4] → middleSize
    EXPECT_EQ(size.getSizeAtHeight(6, 1), 1);
    EXPECT_EQ(size.getSizeAtHeight(6, 4), 1);
}

TEST(ThreeLayersFeatureSizeTest, ReturnsUpperSizeAboveThreshold)
{
    ThreeLayersFeatureSize size(/*limit=*/1,
        /*upperLimit=*/1,
        /*lowerSize=*/0,
        /*middleSize=*/1,
        /*upperSize=*/2);
    // y >= trunkHeight - upperLimit = 6 - 1 = 5 → upperSize
    EXPECT_EQ(size.getSizeAtHeight(6, 5), 2);
    EXPECT_EQ(size.getSizeAtHeight(6, 6), 2);
}

TEST(ThreeLayersFeatureSizeTest, TypeIsThreeLayers)
{
    ThreeLayersFeatureSize size(1, 1, 0, 1, 2);
    EXPECT_EQ(size.type(), FeatureSizeType::ThreeLayers);
}

TEST(ThreeLayersFeatureSizeTest, AccessorsReturnCorrectValues)
{
    ThreeLayersFeatureSize size(/*limit=*/1,
        /*upperLimit=*/2,
        /*lowerSize=*/0,
        /*middleSize=*/1,
        /*upperSize=*/3,
        std::optional<i32>{4});
    EXPECT_EQ(size.limit(), 1);
    EXPECT_EQ(size.upperLimit(), 2);
    EXPECT_EQ(size.lowerSize(), 0);
    EXPECT_EQ(size.middleSize(), 1);
    EXPECT_EQ(size.upperSize(), 3);
    ASSERT_TRUE(size.minClippedHeight().has_value());
    EXPECT_EQ(*size.minClippedHeight(), 4);
}

TEST(ThreeLayersFeatureSizeTest, CloneProducesEqualObject)
{
    ThreeLayersFeatureSize size(1, 2, 0, 1, 3, std::optional<i32>{5});
    auto cloned = size.clone();

    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->type(), FeatureSizeType::ThreeLayers);
    auto* typed = dynamic_cast<ThreeLayersFeatureSize*>(cloned.get());
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->limit(), 1);
    EXPECT_EQ(typed->upperLimit(), 2);
    EXPECT_EQ(typed->lowerSize(), 0);
    EXPECT_EQ(typed->middleSize(), 1);
    EXPECT_EQ(typed->upperSize(), 3);
    ASSERT_TRUE(typed->minClippedHeight().has_value());
    EXPECT_EQ(*typed->minClippedHeight(), 5);
}

// ============================================================================
// FeatureSizeParser 测试
// ============================================================================

TEST(FeatureSizeParserTest, ParsesTwoLayersFeatureSize)
{
    nlohmann::json j = R"({
        "type": "minecraft:two_layers_feature_size",
        "limit": 1,
        "lower_size": 0,
        "upper_size": 2
    })"_json;

    auto result = FeatureSizeParser::parse(j);
    ASSERT_TRUE(result.success());
    auto size = std::move(result).value();
    ASSERT_NE(size, nullptr);
    EXPECT_EQ(size->type(), FeatureSizeType::TwoLayers);
    auto* typed = dynamic_cast<TwoLayersFeatureSize*>(size.get());
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->limit(), 1);
    EXPECT_EQ(typed->lowerSize(), 0);
    EXPECT_EQ(typed->upperSize(), 2);
    EXPECT_FALSE(typed->minClippedHeight().has_value());
}

TEST(FeatureSizeParserTest, ParsesTwoLayersFeatureSizeWithMinClippedHeight)
{
    nlohmann::json j = R"({
        "type": "minecraft:two_layers_feature_size",
        "limit": 0,
        "lower_size": 0,
        "upper_size": 0,
        "min_clipped_height": 4
    })"_json;

    auto result = FeatureSizeParser::parse(j);
    ASSERT_TRUE(result.success());
    auto size = std::move(result).value();
    auto* typed = dynamic_cast<TwoLayersFeatureSize*>(size.get());
    ASSERT_NE(typed, nullptr);
    ASSERT_TRUE(typed->minClippedHeight().has_value());
    EXPECT_EQ(*typed->minClippedHeight(), 4);
}

TEST(FeatureSizeParserTest, ParsesThreeLayersFeatureSize)
{
    nlohmann::json j = R"({
        "type": "minecraft:three_layers_feature_size",
        "limit": 1,
        "lower_size": 0,
        "middle_size": 1,
        "upper_limit": 1,
        "upper_size": 2
    })"_json;

    auto result = FeatureSizeParser::parse(j);
    ASSERT_TRUE(result.success());
    auto size = std::move(result).value();
    ASSERT_NE(size, nullptr);
    EXPECT_EQ(size->type(), FeatureSizeType::ThreeLayers);
    auto* typed = dynamic_cast<ThreeLayersFeatureSize*>(size.get());
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->limit(), 1);
    EXPECT_EQ(typed->upperLimit(), 1);
    EXPECT_EQ(typed->lowerSize(), 0);
    EXPECT_EQ(typed->middleSize(), 1);
    EXPECT_EQ(typed->upperSize(), 2);
}

TEST(FeatureSizeParserTest, ParsesWithoutNamespacePrefix)
{
    nlohmann::json j = R"({
        "type": "two_layers_feature_size",
        "limit": 1,
        "lower_size": 0,
        "upper_size": 2
    })"_json;

    auto result = FeatureSizeParser::parse(j);
    ASSERT_TRUE(result.success());
    auto size = std::move(result).value();
    EXPECT_EQ(size->type(), FeatureSizeType::TwoLayers);
}

TEST(FeatureSizeParserTest, FailsOnMissingType)
{
    nlohmann::json j = R"({"limit": 1})"_json;
    auto result = FeatureSizeParser::parse(j);
    EXPECT_FALSE(result.success());
}

TEST(FeatureSizeParserTest, FailsOnMissingRequiredField)
{
    nlohmann::json j = R"({
        "type": "minecraft:two_layers_feature_size",
        "limit": 1,
        "upper_size": 2
    })"_json;
    auto result = FeatureSizeParser::parse(j);
    EXPECT_FALSE(result.success());
}

TEST(FeatureSizeParserTest, FailsOnOutOfRangeValue)
{
    nlohmann::json j = R"({
        "type": "minecraft:two_layers_feature_size",
        "limit": 100,
        "lower_size": 0,
        "upper_size": 2
    })"_json;
    auto result = FeatureSizeParser::parse(j);
    EXPECT_FALSE(result.success());
}

TEST(FeatureSizeParserTest, FailsOnUnknownType)
{
    nlohmann::json j = R"({
        "type": "minecraft:unknown_feature_size",
        "limit": 1
    })"_json;
    auto result = FeatureSizeParser::parse(j);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// TreeFeatureConfig 深拷贝 minimumSize 测试
// ============================================================================

TEST(TreeFeatureConfigMinimumSizeTest, CopyConstructorDeepCopiesMinimumSize)
{
    TwoLayersFeatureSize originalSize(1, 0, 2, std::optional<i32>{4});

    TreeFeatureConfig config;
    config.minimumSize = originalSize.clone();

    TreeFeatureConfig copy(config);
    ASSERT_NE(copy.minimumSize, nullptr);
    EXPECT_NE(copy.minimumSize.get(), config.minimumSize.get()); // 不同指针
    EXPECT_EQ(copy.minimumSize->type(), FeatureSizeType::TwoLayers);
    auto* typed = dynamic_cast<TwoLayersFeatureSize*>(copy.minimumSize.get());
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->limit(), 1);
    EXPECT_EQ(typed->lowerSize(), 0);
    EXPECT_EQ(typed->upperSize(), 2);
    ASSERT_TRUE(typed->minClippedHeight().has_value());
    EXPECT_EQ(*typed->minClippedHeight(), 4);
}

TEST(TreeFeatureConfigMinimumSizeTest, CopyAssignmentDeepCopiesMinimumSize)
{
    TreeFeatureConfig config;
    config.minimumSize = std::make_unique<TwoLayersFeatureSize>(2, 1, 3);

    TreeFeatureConfig assigned;
    assigned = config;
    ASSERT_NE(assigned.minimumSize, nullptr);
    EXPECT_NE(assigned.minimumSize.get(), config.minimumSize.get());
    EXPECT_EQ(assigned.minimumSize->type(), FeatureSizeType::TwoLayers);
}

TEST(TreeFeatureConfigMinimumSizeTest, CopyAssignmentWithNullSourceResetsDestination)
{
    TreeFeatureConfig source; // minimumSize 为空
    TreeFeatureConfig dest;
    dest.minimumSize = std::make_unique<TwoLayersFeatureSize>(1, 0, 2);

    dest = source;
    EXPECT_EQ(dest.minimumSize, nullptr);
}

TEST(TreeFeatureConfigMinimumSizeTest, MoveConstructorTransfersOwnership)
{
    TreeFeatureConfig config;
    config.minimumSize = std::make_unique<TwoLayersFeatureSize>(1, 0, 2);
    auto* originalPtr = config.minimumSize.get();

    TreeFeatureConfig moved(std::move(config));
    ASSERT_NE(moved.minimumSize, nullptr);
    EXPECT_EQ(moved.minimumSize.get(), originalPtr); // 指针相同
    EXPECT_EQ(config.minimumSize, nullptr);          // 源被清空
}

TEST(TreeFeatureConfigMinimumSizeTest, MoveAssignmentTransfersOwnership)
{
    TreeFeatureConfig config;
    config.minimumSize = std::make_unique<TwoLayersFeatureSize>(1, 0, 2);
    auto* originalPtr = config.minimumSize.get();

    TreeFeatureConfig moved;
    moved = std::move(config);
    ASSERT_NE(moved.minimumSize, nullptr);
    EXPECT_EQ(moved.minimumSize.get(), originalPtr);
    EXPECT_EQ(config.minimumSize, nullptr);
}
