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
 * @file BlockPredicateTest.cpp
 * @brief BlockPredicate、FluidPredicate 和 StatePropertiesPredicate 单元测试
 *
 * 测试内容：
 * 1. StatePropertiesPredicate 的 fromJson 方法
 * 2. BlockPredicate 的方块ID和标签检查
 * 3. FluidPredicate 的流体匹配检查
 * 4. StatePropertiesPredicate 复用验证
 */

#include "advancement/trigger/conditions/BlockPredicate.hpp"
#include "entity/loot/StatePropertiesPredicate.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "util/property/Properties.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;

// ============================================================================
// StatePropertiesPredicate fromJson 测试
// ============================================================================

class StatePropertiesPredicateFromJsonTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保方块已初始化
        VanillaBlocks::initialize();
    }
};

TEST_F(StatePropertiesPredicateFromJsonTest, EmptyJson)
{
    // 空JSON返回空谓词
    auto result = StatePropertiesPredicate::fromJson(nlohmann::json::object());
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isEmpty());
}

TEST_F(StatePropertiesPredicateFromJsonTest, NullJson)
{
    // null 返回空谓词
    auto result = StatePropertiesPredicate::fromJson(nullptr);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isEmpty());
}

TEST_F(StatePropertiesPredicateFromJsonTest, ExactMatch)
{
    // 精确匹配: { "age": "3" }
    nlohmann::json json = {{"age", "3"}};

    auto result = StatePropertiesPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().matcherCount(), 1);
}

TEST_F(StatePropertiesPredicateFromJsonTest, RangeMatchMinOnly)
{
    // 范围匹配（仅最小值）: { "power": { "min": "5" } }
    nlohmann::json json = {{"power", {{"min", "5"}}}};

    auto result = StatePropertiesPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().matcherCount(), 1);
}

TEST_F(StatePropertiesPredicateFromJsonTest, RangeMatchMaxOnly)
{
    // 范围匹配（仅最大值）: { "level": { "max": "10" } }
    nlohmann::json json = {{"level", {{"max", "10"}}}};

    auto result = StatePropertiesPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().matcherCount(), 1);
}

TEST_F(StatePropertiesPredicateFromJsonTest, RangeMatchBoth)
{
    // 范围匹配（最小和最大）: { "power": { "min": "5", "max": "10" } }
    nlohmann::json json = {{"power", {{"min", "5"}, {"max", "10"}}}};

    auto result = StatePropertiesPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().matcherCount(), 1);
}

TEST_F(StatePropertiesPredicateFromJsonTest, RangeMatchSameValue)
{
    // 范围匹配（min == max，优化为精确匹配）
    nlohmann::json json = {{"age", {{"min", "3"}, {"max", "3"}}}};

    auto result = StatePropertiesPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().matcherCount(), 1);

    // 验证是精确匹配器
    const auto& matchers = result.value().matchers();
    EXPECT_NE(dynamic_cast<const StatePropertiesPredicate::ExactMatcher*>(matchers[0].get()), nullptr);
}

TEST_F(StatePropertiesPredicateFromJsonTest, MultipleProperties)
{
    // 多属性匹配
    nlohmann::json json = {{"facing", "north"}, {"lit", "true"}, {"age", {{"min", "5"}, {"max", "7"}}}};

    auto result = StatePropertiesPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().matcherCount(), 3);
}

// ============================================================================
// StatePropertiesPredicate toJsonValue 测试
// ============================================================================

TEST_F(StatePropertiesPredicateFromJsonTest, ToJsonValueRoundTrip)
{
    // 创建谓词
    StatePropertiesPredicate original;
    original.addExactMatch("age", "3");
    original.addRangeMatch("power", "5", "10");

    // 序列化
    nlohmann::json json = original.toJsonValue();

    // 反序列化
    auto result = StatePropertiesPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().matcherCount(), 2);
}

TEST_F(StatePropertiesPredicateFromJsonTest, ToJsonValueEmpty)
{
    StatePropertiesPredicate empty;
    nlohmann::json json = empty.toJsonValue();
    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.empty());
}

// ============================================================================
// BlockPredicate 测试
// ============================================================================

class BlockPredicateTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(BlockPredicateTest, DefaultPredicateMatchesAll)
{
    BlockPredicate predicate;
    EXPECT_TRUE(predicate.isAny());

    // 获取一个方块状态
    const BlockState* stone = BlockRegistry::instance().get(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_TRUE(predicate.test(*stone));
}

TEST_F(BlockPredicateTest, MatchesByBlockId)
{
    // 创建一个匹配石头的谓词
    BlockPredicate predicate(ResourceLocation("minecraft:stone"), std::nullopt, StatePropertiesPredicate{});
    EXPECT_FALSE(predicate.isAny());

    // 测试石头
    const BlockState* stone = BlockRegistry::instance().get(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_TRUE(predicate.test(*stone));

    // 测试其他方块
    const BlockState* dirt = BlockRegistry::instance().get(ResourceLocation("minecraft:dirt"));
    ASSERT_NE(dirt, nullptr);
    EXPECT_FALSE(predicate.test(*dirt));
}

TEST_F(BlockPredicateTest, MatchesByTag)
{
    // 创建一个匹配原木标签的谓词
    auto* logsTag = BlockTags::getTag(ResourceLocation("minecraft:logs"));
    if (logsTag == nullptr) {
        GTEST_SKIP() << "minecraft:logs tag not available";
    }

    // 使用第一个原木方块测试
    const BlockState* oakLog = BlockRegistry::instance().get(ResourceLocation("minecraft:oak_log"));
    if (oakLog == nullptr) {
        GTEST_SKIP() << "minecraft:oak_log not available";
    }

    BlockPredicate predicate(std::nullopt, ResourceLocation("minecraft:logs"), StatePropertiesPredicate{});
    EXPECT_FALSE(predicate.isAny());
    EXPECT_TRUE(predicate.test(*oakLog));

    // 测试非原木方块
    const BlockState* stone = BlockRegistry::instance().get(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_FALSE(predicate.test(*stone));
}

TEST_F(BlockPredicateTest, MatchesWithStateProperties)
{
    // 创建带状态属性的谓词
    StatePropertiesPredicate statePred;
    statePred.addExactMatch("lit", "true");

    BlockPredicate predicate(ResourceLocation("minecraft:redstone_lamp"), std::nullopt, std::move(statePred));

    // 获取红石灯
    const BlockState* lamp = BlockRegistry::instance().get(ResourceLocation("minecraft:redstone_lamp"));
    if (lamp == nullptr) {
        GTEST_SKIP() << "minecraft:redstone_lamp not available";
    }

    // 未点亮的红石灯不应该匹配（默认状态 lit=false）
    EXPECT_FALSE(predicate.test(*lamp));
}

TEST_F(BlockPredicateTest, FromJsonBlockId)
{
    nlohmann::json json = {{"block", "minecraft:stone"}};

    auto result = BlockPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_TRUE(result.value().getBlock().has_value());
    EXPECT_EQ(result.value().getBlock()->toString(), "minecraft:stone");
}

TEST_F(BlockPredicateTest, FromJsonTag)
{
    nlohmann::json json = {{"tag", "minecraft:logs"}};

    auto result = BlockPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_TRUE(result.value().getTag().has_value());
    EXPECT_EQ(result.value().getTag()->toString(), "minecraft:logs");
}

TEST_F(BlockPredicateTest, FromJsonWithState)
{
    nlohmann::json json = {{"block", "minecraft:redstone_lamp"}, {"state", {{"lit", "true"}}}};

    auto result = BlockPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_TRUE(result.value().getBlock().has_value());
    EXPECT_FALSE(result.value().getState().isEmpty());
}

TEST_F(BlockPredicateTest, ToJsonRoundTrip)
{
    // 创建谓词
    StatePropertiesPredicate statePred;
    statePred.addExactMatch("lit", "true");

    BlockPredicate original(ResourceLocation("minecraft:redstone_lamp"), std::nullopt, std::move(statePred));

    // 序列化
    nlohmann::json json = original.toJson();

    // 反序列化
    auto result = BlockPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_TRUE(result.value().getBlock().has_value());
    EXPECT_EQ(result.value().getBlock()->toString(), "minecraft:redstone_lamp");
}

TEST_F(BlockPredicateTest, ToJsonAny)
{
    BlockPredicate any;
    nlohmann::json json = any.toJson();
    EXPECT_TRUE(json.is_null());
}

// ============================================================================
// FluidPredicate 测试
// ============================================================================

TEST_F(BlockPredicateTest, FluidPredicateDefaultMatchesAll)
{
    FluidPredicate predicate;
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(BlockPredicateTest, FluidPredicateFromJson)
{
    nlohmann::json json = {{"fluid", "minecraft:water"}};

    auto result = FluidPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
}

TEST_F(BlockPredicateTest, FluidPredicateToJson)
{
    FluidPredicate any;
    nlohmann::json json = any.toJson();
    EXPECT_TRUE(json.is_null());
}

// ============================================================================
// FluidPredicate 流体匹配测试
// ============================================================================

class FluidPredicateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保方块和流体已初始化
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

TEST_F(FluidPredicateTest, DefaultPredicateMatchesAll)
{
    FluidPredicate predicate;
    EXPECT_TRUE(predicate.isAny());

    // 获取一个方块状态
    const BlockState* stone = BlockRegistry::instance().get(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_TRUE(predicate.test(*stone));
}

TEST_F(FluidPredicateTest, MatchesWaterSource)
{
    // 获取水源方块
    const BlockState* water = BlockRegistry::instance().get(ResourceLocation("minecraft:water"));
    if (water == nullptr) {
        GTEST_SKIP() << "minecraft:water not available";
    }

    // 创建匹配水的谓词
    nlohmann::json json = {{"fluid", "minecraft:water"}};
    auto result = FluidPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    const FluidPredicate& predicate = result.value();
    EXPECT_FALSE(predicate.isAny());

    // 水源方块应该匹配
    EXPECT_TRUE(predicate.test(*water));
}

TEST_F(FluidPredicateTest, MatchesFlowingWater)
{
    // 获取水方块
    const BlockState* waterSource = BlockRegistry::instance().get(ResourceLocation("minecraft:water"));
    if (waterSource == nullptr) {
        GTEST_SKIP() << "minecraft:water not available";
    }

    // 创建匹配水的谓词（使用 minecraft:water）
    nlohmann::json json = {{"fluid", "minecraft:water"}};
    auto result = FluidPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    const FluidPredicate& predicate = result.value();
    EXPECT_FALSE(predicate.isAny());

    // 水源方块（level=0）应该匹配
    EXPECT_TRUE(predicate.test(*waterSource));

    // 流动水（level=1-7）也应该匹配
    // 通过修改 BlockState 的 level 属性来获取流动水状态
    auto& levelProp = BlockStateProperties::LEVEL_0_15();
    const BlockState* flowingWater = &waterSource->with(levelProp, 4); // level=4 表示流动水
    EXPECT_TRUE(predicate.test(*flowingWater));
}

TEST_F(FluidPredicateTest, MatchesLavaSource)
{
    // 获取岩浆方块
    const BlockState* lava = BlockRegistry::instance().get(ResourceLocation("minecraft:lava"));
    if (lava == nullptr) {
        GTEST_SKIP() << "minecraft:lava not available";
    }

    // 创建匹配岩浆的谓词
    nlohmann::json json = {{"fluid", "minecraft:lava"}};
    auto result = FluidPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    const FluidPredicate& predicate = result.value();
    EXPECT_FALSE(predicate.isAny());

    // 岩浆应该匹配
    EXPECT_TRUE(predicate.test(*lava));
}

TEST_F(FluidPredicateTest, DoesNotMatchDifferentFluid)
{
    // 获取水方块
    const BlockState* water = BlockRegistry::instance().get(ResourceLocation("minecraft:water"));
    if (water == nullptr) {
        GTEST_SKIP() << "minecraft:water not available";
    }

    // 创建匹配岩浆的谓词
    nlohmann::json json = {{"fluid", "minecraft:lava"}};
    auto result = FluidPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    const FluidPredicate& predicate = result.value();
    EXPECT_FALSE(predicate.isAny());

    // 水不应该匹配岩浆谓词
    EXPECT_FALSE(predicate.test(*water));
}

TEST_F(FluidPredicateTest, DoesNotMatchNonFluidBlock)
{
    // 获取石头方块（无流体）
    const BlockState* stone = BlockRegistry::instance().get(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);

    // 创建匹配水的谓词
    nlohmann::json json = {{"fluid", "minecraft:water"}};
    auto result = FluidPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    const FluidPredicate& predicate = result.value();
    EXPECT_FALSE(predicate.isAny());

    // 石头没有流体，应该不匹配
    EXPECT_FALSE(predicate.test(*stone));
}

TEST_F(FluidPredicateTest, UnknownFluidIdDoesNotMatch)
{
    // 获取水方块
    const BlockState* water = BlockRegistry::instance().get(ResourceLocation("minecraft:water"));
    if (water == nullptr) {
        GTEST_SKIP() << "minecraft:water not available";
    }

    // 创建匹配未知流体的谓词
    nlohmann::json json = {{"fluid", "minecraft:unknown_fluid"}};
    auto result = FluidPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    const FluidPredicate& predicate = result.value();
    EXPECT_FALSE(predicate.isAny());

    // 未知流体ID应该不匹配任何东西
    EXPECT_FALSE(predicate.test(*water));
}

TEST_F(FluidPredicateTest, RoundTripSerialization)
{
    // 创建谓词
    nlohmann::json originalJson = {{"fluid", "minecraft:water"}};
    auto result = FluidPredicate::fromJson(originalJson);
    ASSERT_TRUE(result.success());

    // 序列化
    nlohmann::json serialized = result.value().toJson();
    EXPECT_TRUE(serialized.is_object());
    EXPECT_TRUE(serialized.contains("fluid"));
    EXPECT_EQ(serialized["fluid"], "minecraft:water");

    // 反序列化
    auto result2 = FluidPredicate::fromJson(serialized);
    EXPECT_TRUE(result2.success());
    EXPECT_FALSE(result2.value().isAny());
}

TEST_F(FluidPredicateTest, WaterMatchesFlowingWater)
{
    // 这个测试验证 isEquivalentTo 的正确行为：
    // minecraft:water 谓词应该同时匹配水源和流动水

    const BlockState* waterSource = BlockRegistry::instance().get(ResourceLocation("minecraft:water"));

    if (waterSource == nullptr) {
        GTEST_SKIP() << "minecraft:water not available";
    }

    // 创建匹配 minecraft:water 的谓词
    nlohmann::json json = {{"fluid", "minecraft:water"}};
    auto result = FluidPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    const FluidPredicate& predicate = result.value();

    // 应该匹配水源（level=0）
    EXPECT_TRUE(predicate.test(*waterSource));

    // 也应该匹配流动水（level=1-7，因为 isEquivalentTo）
    auto& levelProp = BlockStateProperties::LEVEL_0_15();
    const BlockState* flowingWater = &waterSource->with(levelProp, 3); // level=3 表示流动水
    EXPECT_TRUE(predicate.test(*flowingWater));

    // 也应该匹配下落的水（level>=8）
    const BlockState* fallingWater = &waterSource->with(levelProp, 10); // level=10 表示下落的水
    EXPECT_TRUE(predicate.test(*fallingWater));
}
