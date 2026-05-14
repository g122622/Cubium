/**
 * @file BlockPredicateTest.cpp
 * @brief BlockPredicate 和 StatePropertiesPredicate 单元测试
 *
 * 测试内容：
 * 1. StatePropertiesPredicate 的 fromJson 方法
 * 2. BlockPredicate 的方块ID和标签检查
 * 3. StatePropertiesPredicate 复用验证
 */

#include "advancement/trigger/conditions/BlockPredicate.hpp"
#include "entity/loot/StatePropertiesPredicate.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/VanillaBlocks.hpp"
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
