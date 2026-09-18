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
 * @file LootSerializersTest.cpp
 * @brief 掉落表 JSON 序列化器测试
 */

#include "item/loot/LootSerializers.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/loot/LootPool.hpp"
#include "item/loot/LootTable.hpp"
#include "item/loot/StatePropertiesPredicate.hpp"
#include "item/loot/conditions/LootConditions.hpp"
#include "item/loot/conditions/MatchToolCondition.hpp"
#include "item/loot/entries/ItemLootEntry.hpp"
#include "item/loot/entries/LootEntry.hpp"
#include "item/loot/entries/TableLootEntry.hpp"
#include "item/loot/functions/LootFunctions.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/chunk/data/ChunkData.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::loot;

// Test implementation of IWorld for loot testing
class LootSerializersTestWorld : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("LootSerializersTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("LootSerializersTestWorld::tickManager not implemented");
    }
};

class LootSerializersTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }

    LootSerializersTestWorld m_world;
};

// ============================================================================
// RandomValueRange Parsing Tests
// ============================================================================

TEST_F(LootSerializersTest, ParseRandomValueRange_FixedInteger)
{
    nlohmann::json json = 5;

    auto result = LootSerializers::parseRandomValueRange(json);
    ASSERT_TRUE(result.success());

    RandomValueRange range = result.value();
    EXPECT_TRUE(range.isFixed());
    EXPECT_FLOAT_EQ(5.0f, range.getMin());
    EXPECT_FLOAT_EQ(5.0f, range.getMax());
}

TEST_F(LootSerializersTest, ParseRandomValueRange_FixedFloat)
{
    nlohmann::json json = 3.5f;

    auto result = LootSerializers::parseRandomValueRange(json);
    ASSERT_TRUE(result.success());

    RandomValueRange range = result.value();
    EXPECT_TRUE(range.isFixed());
    EXPECT_FLOAT_EQ(3.5f, range.getMin());
    EXPECT_FLOAT_EQ(3.5f, range.getMax());
}

TEST_F(LootSerializersTest, ParseRandomValueRange_RangeObject)
{
    nlohmann::json json = {{"min", 1}, {"max", 5}};

    auto result = LootSerializers::parseRandomValueRange(json);
    ASSERT_TRUE(result.success());

    RandomValueRange range = result.value();
    EXPECT_FALSE(range.isFixed());
    EXPECT_FLOAT_EQ(1.0f, range.getMin());
    EXPECT_FLOAT_EQ(5.0f, range.getMax());
}

TEST_F(LootSerializersTest, ParseRandomValueRange_WithType)
{
    nlohmann::json json = {{"type", "minecraft:uniform"}, {"min", 2.0f}, {"max", 8.0f}};

    auto result = LootSerializers::parseRandomValueRange(json);
    ASSERT_TRUE(result.success());

    RandomValueRange range = result.value();
    EXPECT_FLOAT_EQ(2.0f, range.getMin());
    EXPECT_FLOAT_EQ(8.0f, range.getMax());
}

TEST_F(LootSerializersTest, ParseRandomValueRange_InvalidType)
{
    nlohmann::json json = "invalid";

    auto result = LootSerializers::parseRandomValueRange(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// BinomialRange Parsing Tests
// ============================================================================

TEST_F(LootSerializersTest, ParseBinomialRange_Basic)
{
    nlohmann::json json = {{"type", "minecraft:binomial"}, {"n", 10}, {"p", 0.5f}};

    auto result = LootSerializers::parseBinomialRange(json);
    ASSERT_TRUE(result.success());

    BinomialRange range = result.value();
    EXPECT_EQ(10, range.getN());
    EXPECT_FLOAT_EQ(0.5f, range.getP());
}

TEST_F(LootSerializersTest, ParseBinomialRange_MissingN)
{
    nlohmann::json json = {{"type", "minecraft:binomial"}, {"p", 0.5f}};

    auto result = LootSerializers::parseBinomialRange(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, ParseBinomialRange_MissingP)
{
    nlohmann::json json = {{"type", "minecraft:binomial"}, {"n", 10}};

    auto result = LootSerializers::parseBinomialRange(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// ConstantRange Parsing Tests
// ============================================================================

TEST_F(LootSerializersTest, ParseConstantRange_FromInteger)
{
    nlohmann::json json = 7;

    auto result = LootSerializers::parseConstantRange(json);
    ASSERT_TRUE(result.success());

    ConstantRange range = result.value();
    EXPECT_EQ(7, range.getValue());
}

TEST_F(LootSerializersTest, ParseConstantRange_FromObject)
{
    nlohmann::json json = {{"type", "minecraft:constant"}, {"value", 42}};

    auto result = LootSerializers::parseConstantRange(json);
    ASSERT_TRUE(result.success());

    ConstantRange range = result.value();
    EXPECT_EQ(42, range.getValue());
}

// ============================================================================
// IRandomRange Parsing Tests
// ============================================================================

TEST_F(LootSerializersTest, ParseRandomRange_ConstantFromNumber)
{
    nlohmann::json json = 15;

    auto result = LootSerializers::parseRandomRange(json);
    ASSERT_TRUE(result.success());

    auto range = result.value();
    math::Random rng(12345);
    EXPECT_EQ(15, range->generateInt(rng));
}

TEST_F(LootSerializersTest, ParseRandomRange_UniformType)
{
    nlohmann::json json = {{"type", "minecraft:uniform"}, {"min", 1}, {"max", 10}};

    auto result = LootSerializers::parseRandomRange(json);
    ASSERT_TRUE(result.success());

    auto range = result.value();
    math::Random rng(12345);
    i32 value = range->generateInt(rng);
    EXPECT_GE(value, 1);
    EXPECT_LE(value, 10);
}

TEST_F(LootSerializersTest, ParseRandomRange_BinomialType)
{
    nlohmann::json json = {{"type", "minecraft:binomial"}, {"n", 5}, {"p", 0.3f}};

    auto result = LootSerializers::parseRandomRange(json);
    ASSERT_TRUE(result.success());

    auto range = result.value();
    math::Random rng(12345);
    i32 value = range->generateInt(rng);
    EXPECT_GE(value, 0);
    EXPECT_LE(value, 5);
}

TEST_F(LootSerializersTest, ParseRandomRange_ConstantType)
{
    nlohmann::json json = {{"type", "minecraft:constant"}, {"value", 100}};

    auto result = LootSerializers::parseRandomRange(json);
    ASSERT_TRUE(result.success());

    auto range = result.value();
    math::Random rng(12345);
    EXPECT_EQ(100, range->generateInt(rng));
}

TEST_F(LootSerializersTest, ParseRandomRange_DefaultIsUniform)
{
    nlohmann::json json = {{"min", 5}, {"max", 15}};

    auto result = LootSerializers::parseRandomRange(json);
    ASSERT_TRUE(result.success());

    auto range = result.value();
    math::Random rng(12345);
    i32 value = range->generateInt(rng);
    EXPECT_GE(value, 5);
    EXPECT_LE(value, 15);
}

// ============================================================================
// LootCondition Parsing Tests
// ============================================================================

TEST_F(LootSerializersTest, ParseCondition_RandomChance)
{
    nlohmann::json json = {{"condition", "minecraft:random_chance"}, {"chance", 0.5f}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("random_chance", condition->getType());
}

TEST_F(LootSerializersTest, ParseCondition_SilkTouch)
{
    nlohmann::json json = {{"condition", "minecraft:silk_touch"}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("silk_touch", condition->getType());
}

TEST_F(LootSerializersTest, ParseCondition_Inverted)
{
    nlohmann::json json = {
        {"condition", "minecraft:inverted"}, {"term", {{"condition", "minecraft:random_chance"}, {"chance", 0.3f}}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("inverted", condition->getType());
}

TEST_F(LootSerializersTest, ParseCondition_Alternative)
{
    nlohmann::json json = {{"condition", "minecraft:alternative"},
        {"terms",
            {{{"condition", "minecraft:random_chance"}, {"chance", 0.3f}},
                {{"condition", "minecraft:random_chance"}, {"chance", 0.5f}}}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("or", condition->getType()); // alternative -> OrCondition
}

TEST_F(LootSerializersTest, ParseCondition_MissingType)
{
    nlohmann::json json = {{"chance", 0.5f}};

    auto result = LootSerializers::parseCondition(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, ParseCondition_UnknownType)
{
    nlohmann::json json = {{"condition", "minecraft:unknown_condition"}};

    auto result = LootSerializers::parseCondition(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, ParseCondition_ReferenceSupported)
{
    nlohmann::json json = {{"condition", "minecraft:reference"}, {"name", "minecraft:test"}};

    auto result = LootSerializers::parseCondition(json);
    // 源码 LootSerializers 已实现 minecraft:reference 条件解析（_parseReferenceCondition），
    // 此前测试期望 Unsupported，是源码实现前的过时期望。
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value()->getType(), "reference");
}

TEST_F(LootSerializersTest, ParseCondition_TableBonus)
{
    nlohmann::json json = {{"condition", "minecraft:table_bonus"},
        {"enchantment", "minecraft:fortune"},
        {"chances", {0.1, 0.2, 0.3, 0.4}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());
    auto condition = std::move(result).value();
    EXPECT_EQ(condition->getType(), "table_bonus");

    // 验证序列化还原
    auto toJson = LootSerializers::toJson(*condition);
    EXPECT_EQ(toJson["condition"], "minecraft:table_bonus");
    EXPECT_EQ(toJson["enchantment"], "minecraft:fortune");
    EXPECT_EQ(toJson["chances"].size(), 4u);
    EXPECT_FLOAT_EQ(toJson["chances"][0].get<f32>(), 0.1f);
    EXPECT_FLOAT_EQ(toJson["chances"][1].get<f32>(), 0.2f);
    EXPECT_FLOAT_EQ(toJson["chances"][2].get<f32>(), 0.3f);
    EXPECT_FLOAT_EQ(toJson["chances"][3].get<f32>(), 0.4f);
}

TEST_F(LootSerializersTest, ParseCondition_TableBonus_MissingEnchantment)
{
    nlohmann::json json = {{"condition", "minecraft:table_bonus"}, {"chances", {0.1, 0.2}}};

    auto result = LootSerializers::parseCondition(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, ParseCondition_TableBonus_MissingChances)
{
    nlohmann::json json = {{"condition", "minecraft:table_bonus"}, {"enchantment", "minecraft:fortune"}};

    auto result = LootSerializers::parseCondition(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, ParseCondition_TableBonus_EmptyChances)
{
    nlohmann::json json = {{"condition", "minecraft:table_bonus"},
        {"enchantment", "minecraft:fortune"},
        {"chances", nlohmann::json::array()}};

    auto result = LootSerializers::parseCondition(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, ParseCondition_BlockStateProperty_BlockOnly)
{
    // 仅检查方块 ID
    nlohmann::json json = {{"condition", "minecraft:block_state_property"}, {"block", "minecraft:stone"}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("block_state_property", condition->getType());

    auto* blockStateCond = dynamic_cast<BlockStateCondition*>(condition.get());
    ASSERT_NE(blockStateCond, nullptr);
    EXPECT_EQ(blockStateCond->getBlockId(), "minecraft:stone");
    EXPECT_TRUE(blockStateCond->getProperties().isEmpty());
}

TEST_F(LootSerializersTest, ParseCondition_BlockStateProperty_WithExactProperty)
{
    // 检查方块 ID + 精确属性匹配
    nlohmann::json json = {{"condition", "minecraft:block_state_property"},
        {"block", "minecraft:beetroots"},
        {"properties", {{"age", "3"}}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("block_state_property", condition->getType());

    auto* blockStateCond = dynamic_cast<BlockStateCondition*>(condition.get());
    ASSERT_NE(blockStateCond, nullptr);
    EXPECT_EQ(blockStateCond->getBlockId(), "minecraft:beetroots");
    EXPECT_EQ(blockStateCond->getProperties().matcherCount(), 1);
}

TEST_F(LootSerializersTest, ParseCondition_BlockStateProperty_WithRangeProperty)
{
    // 检查方块 ID + 范围属性匹配
    nlohmann::json json = {{"condition", "minecraft:block_state_property"},
        {"block", "minecraft:wheat"},
        {"properties", {{"age", {{"min", "5"}, {"max", "7"}}}}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("block_state_property", condition->getType());

    auto* blockStateCond = dynamic_cast<BlockStateCondition*>(condition.get());
    ASSERT_NE(blockStateCond, nullptr);
    EXPECT_EQ(blockStateCond->getBlockId(), "minecraft:wheat");
    EXPECT_EQ(blockStateCond->getProperties().matcherCount(), 1);
}

TEST_F(LootSerializersTest, ParseCondition_BlockStateProperty_WithMultipleProperties)
{
    // 多属性匹配
    nlohmann::json json = {{"condition", "minecraft:block_state_property"},
        {"block", "minecraft:oak_door"},
        {"properties", {{"facing", "north"}, {"open", "true"}}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("block_state_property", condition->getType());

    auto* blockStateCond = dynamic_cast<BlockStateCondition*>(condition.get());
    ASSERT_NE(blockStateCond, nullptr);
    EXPECT_EQ(blockStateCond->getBlockId(), "minecraft:oak_door");
    EXPECT_EQ(blockStateCond->getProperties().matcherCount(), 2);
}

TEST_F(LootSerializersTest, ParseCondition_BlockStateProperty_MissingBlock)
{
    // 缺少 block 字段
    nlohmann::json json = {{"condition", "minecraft:block_state_property"}, {"properties", {{"age", "3"}}}};

    auto result = LootSerializers::parseCondition(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, BlockStateCondition_Serialization)
{
    // 测试序列化
    StatePropertiesPredicate properties;
    properties.addExactMatch("age", "3");

    BlockStateCondition condition("minecraft:wheat", std::move(properties));

    nlohmann::json json = LootSerializers::toJson(condition);
    EXPECT_EQ(json["condition"], "minecraft:block_state_property");
    EXPECT_EQ(json["block"], "minecraft:wheat");
    EXPECT_TRUE(json.contains("properties"));
    EXPECT_TRUE(json["properties"].contains("age"));
    EXPECT_EQ(json["properties"]["age"], "3");
}

// ============================================================================
// LootFunction Parsing Tests
// ============================================================================

TEST_F(LootSerializersTest, ParseFunction_SetCount)
{
    nlohmann::json json = {{"function", "minecraft:set_count"}, {"count", {{"min", 1}, {"max", 5}}}, {"add", false}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto function = result.value();
    EXPECT_EQ("set_count", function->getType());
}

TEST_F(LootSerializersTest, ParseFunction_LootingEnchant)
{
    nlohmann::json json = {
        {"function", "minecraft:looting_enchant"}, {"count", {{"min", 0}, {"max", 1}}}, {"limit", 3}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto function = result.value();
    EXPECT_EQ("looting_enchant", function->getType());
}

TEST_F(LootSerializersTest, ParseFunction_SetDamage)
{
    nlohmann::json json = {
        {"function", "minecraft:set_damage"}, {"damage", {{"min", 0.5f}, {"max", 1.0f}}}, {"add", true}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto function = result.value();
    EXPECT_EQ("set_damage", function->getType());
}

TEST_F(LootSerializersTest, ParseFunction_EnchantWithLevels)
{
    nlohmann::json json = {
        {"function", "minecraft:enchant_with_levels"}, {"levels", {{"min", 10}, {"max", 30}}}, {"treasure", true}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto function = result.value();
    EXPECT_EQ("enchant_with_levels", function->getType());
}

TEST_F(LootSerializersTest, ParseFunction_FurnaceSmelt)
{
    nlohmann::json json = {{"function", "minecraft:furnace_smelt"}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto function = result.value();
    EXPECT_EQ("furnace_smelt", function->getType());
}

TEST_F(LootSerializersTest, ParseFunction_MissingFunctionField)
{
    nlohmann::json json = {{"count", 5}};

    auto result = LootSerializers::parseFunction(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, ParseFunction_UnknownType)
{
    nlohmann::json json = {{"function", "minecraft:unknown_function"}};

    auto result = LootSerializers::parseFunction(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, ParseFunction_CopyNbtSupported)
{
    nlohmann::json json = {{"function", "minecraft:copy_nbt"}, {"source", "block_entity"}};

    auto result = LootSerializers::parseFunction(json);
    // 源码 LootSerializers 已实现 minecraft:copy_nbt 函数解析（_parseCopyNbtFunction），
    // ops 数组可选；此前测试期望 Unsupported，是源码实现前的过时期望。
    // 仅断言解析成功与类型标识，避免对实现细节（getSource）过度耦合。
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value()->getType(), "copy_nbt");
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap)
{
    // 测试解析带有 destination 字段的 exploration_map 函数
    nlohmann::json json = {{"function", "minecraft:exploration_map"}, {"destination", "minecraft:mansion"}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto* func = dynamic_cast<ExplorationMapFunction*>(result.value().get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ExplorationMapFunction::Destination::Mansion, func->getDestination());
    EXPECT_EQ(2, func->getZoom());                                                  // 默认值
    EXPECT_EQ(50, func->getSearchRadius());                                         // 默认值
    EXPECT_TRUE(func->shouldSkipKnownStructures());                                 // 默认值
    EXPECT_FALSE(func->getDecoration().has_value());                                // 未指定 decoration
    EXPECT_EQ(world::map::DecorationType::MANSION, func->getEffectiveDecoration()); // 从 destination 推导
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_AllFields)
{
    // 测试解析所有字段的 exploration_map 函数
    nlohmann::json json = {
        {"function", "minecraft:exploration_map"},
        {"destination", "minecraft:buried_treasure"},
        {"decoration", "minecraft:red_x"},
        {"zoom", 1},
        {"search_radius", 100},
        {"skip_existing_chunks", false},
    };

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto* func = dynamic_cast<ExplorationMapFunction*>(result.value().get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ExplorationMapFunction::Destination::BuriedTreasure, func->getDestination());
    EXPECT_EQ(1, func->getZoom());
    EXPECT_EQ(100, func->getSearchRadius());
    EXPECT_FALSE(func->shouldSkipKnownStructures());
    EXPECT_TRUE(func->getDecoration().has_value());
    EXPECT_EQ(world::map::DecorationType::RED_X, func->getDecoration().value());
    EXPECT_EQ(world::map::DecorationType::RED_X, func->getEffectiveDecoration());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_VanillaDatapack)
{
    // 测试解析原版数据包中实际使用的格式（与 shipwreck_map.json 等一致）
    nlohmann::json json = {
        {"function", "minecraft:exploration_map"},
        {"decoration", "minecraft:red_x"},
        {"skip_existing_chunks", false},
        {"zoom", 1},
    };

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto* func = dynamic_cast<ExplorationMapFunction*>(result.value().get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ExplorationMapFunction::Destination::BuriedTreasure, func->getDestination()); // 默认值
    EXPECT_EQ(1, func->getZoom());
    EXPECT_EQ(50, func->getSearchRadius()); // 默认值
    EXPECT_FALSE(func->shouldSkipKnownStructures());
    EXPECT_TRUE(func->getDecoration().has_value());
    EXPECT_EQ(world::map::DecorationType::RED_X, func->getDecoration().value());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_DecorationWithoutNamespace)
{
    // 测试不带命名空间前缀的 decoration 字段（MC 1.16.5 兼容格式）
    nlohmann::json json = {
        {"function", "minecraft:exploration_map"},
        {"decoration", "monument"},
    };

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto* func = dynamic_cast<ExplorationMapFunction*>(result.value().get());
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getDecoration().has_value());
    EXPECT_EQ(world::map::DecorationType::MONUMENT, func->getDecoration().value());
    EXPECT_EQ(world::map::DecorationType::MONUMENT, func->getEffectiveDecoration());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_EmptyJson)
{
    // 测试空 JSON（全部使用默认值）
    nlohmann::json json = {{"function", "minecraft:exploration_map"}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto* func = dynamic_cast<ExplorationMapFunction*>(result.value().get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ExplorationMapFunction::Destination::BuriedTreasure, func->getDestination()); // 默认
    EXPECT_EQ(2, func->getZoom());                                                          // 默认
    EXPECT_EQ(50, func->getSearchRadius());                                                 // 默认
    EXPECT_TRUE(func->shouldSkipKnownStructures());                                         // 默认
    EXPECT_FALSE(func->getDecoration().has_value()); // 未指定，从 destination 推导
    EXPECT_EQ(world::map::DecorationType::RED_X, func->getEffectiveDecoration());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_InvalidDestination)
{
    // 测试无效的 destination 字符串，应回退到默认值 BuriedTreasure
    nlohmann::json json = {{"function", "minecraft:exploration_map"}, {"destination", "minecraft:unknown_structure"}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto* func = dynamic_cast<ExplorationMapFunction*>(result.value().get());
    ASSERT_NE(func, nullptr);
    // 无法识别的 destination 应回退到默认值
    EXPECT_EQ(ExplorationMapFunction::Destination::BuriedTreasure, func->getDestination());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_InvalidDecoration)
{
    // 测试无效的 decoration 字符串，应忽略并回退到从 destination 推导
    nlohmann::json json = {{"function", "minecraft:exploration_map"}, {"decoration", "nonexistent_type"}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto* func = dynamic_cast<ExplorationMapFunction*>(result.value().get());
    ASSERT_NE(func, nullptr);
    // 无法识别的 decoration 应忽略，decoration 保持 nullopt
    EXPECT_FALSE(func->getDecoration().has_value());
    // 从默认 destination (BuriedTreasure) 推导
    EXPECT_EQ(world::map::DecorationType::RED_X, func->getEffectiveDecoration());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_DestinationWithoutNamespace)
{
    // 测试不带命名空间前缀的 destination（MC 1.16.5 兼容格式）
    nlohmann::json json = {{"function", "minecraft:exploration_map"}, {"destination", "monument"}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto* func = dynamic_cast<ExplorationMapFunction*>(result.value().get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ExplorationMapFunction::Destination::Monument, func->getDestination());
}

TEST_F(LootSerializersTest, DecorationTypeStringConversion)
{
    // 测试 decorationTypeFromString 和 decorationTypeToString 的双向转换
    using world::map::DecorationType;

    // 带命名空间前缀
    EXPECT_EQ(DecorationType::MANSION, world::map::decorationTypeFromString("minecraft:mansion").value());
    EXPECT_EQ(DecorationType::MONUMENT, world::map::decorationTypeFromString("minecraft:monument").value());
    EXPECT_EQ(DecorationType::RED_X, world::map::decorationTypeFromString("minecraft:red_x").value());
    EXPECT_EQ(DecorationType::TARGET_X, world::map::decorationTypeFromString("minecraft:target_x").value());
    EXPECT_EQ(DecorationType::BANNER_WHITE, world::map::decorationTypeFromString("minecraft:banner_white").value());
    EXPECT_EQ(DecorationType::BANNER_BLACK, world::map::decorationTypeFromString("minecraft:banner_black").value());

    // 不带命名空间前缀（MC 1.16.5 格式）
    EXPECT_EQ(DecorationType::MANSION, world::map::decorationTypeFromString("mansion").value());
    EXPECT_EQ(DecorationType::RED_X, world::map::decorationTypeFromString("red_x").value());
    EXPECT_EQ(DecorationType::PLAYER, world::map::decorationTypeFromString("player").value());
    EXPECT_EQ(DecorationType::FRAME, world::map::decorationTypeFromString("frame").value());

    // 无效字符串
    EXPECT_FALSE(world::map::decorationTypeFromString("invalid_type").has_value());
    EXPECT_FALSE(world::map::decorationTypeFromString("").has_value());

    // decorationTypeToString 反向转换
    EXPECT_STREQ("mansion", world::map::decorationTypeToString(DecorationType::MANSION));
    EXPECT_STREQ("monument", world::map::decorationTypeToString(DecorationType::MONUMENT));
    EXPECT_STREQ("red_x", world::map::decorationTypeToString(DecorationType::RED_X));
    EXPECT_STREQ("banner_white", world::map::decorationTypeToString(DecorationType::BANNER_WHITE));
}

// ============================================================================
// LootEntry Parsing Tests
// ============================================================================

TEST_F(LootSerializersTest, ParseEntry_Item)
{
    nlohmann::json json = {{"type", "minecraft:item"}, {"name", "minecraft:diamond"}, {"weight", 10}, {"quality", 2}};

    auto result = LootSerializers::parseEntry(json);
    ASSERT_TRUE(result.success());

    auto entry = result.value();
    EXPECT_EQ(LootEntryType::Item, entry->getType());
    EXPECT_EQ(10, entry->getWeight());
    EXPECT_EQ(2, entry->getQuality());
}

TEST_F(LootSerializersTest, ParseEntry_Empty)
{
    nlohmann::json json = {{"type", "minecraft:empty"}, {"weight", 1}};

    auto result = LootSerializers::parseEntry(json);
    ASSERT_TRUE(result.success());

    auto entry = result.value();
    EXPECT_EQ(LootEntryType::Empty, entry->getType());
}

TEST_F(LootSerializersTest, ParseEntry_LootTable_Reference)
{
    // MC 1.21+：minecraft:loot_table entry 用 value 字段（字符串=外部引用）
    nlohmann::json json = {{"type", "minecraft:loot_table"}, {"value", "minecraft:blocks/diamond_ore"}};

    auto result = LootSerializers::parseEntry(json);
    ASSERT_TRUE(result.success());

    auto entry = result.value();
    EXPECT_EQ(LootEntryType::Table, entry->getType());

    auto* tableEntry = dynamic_cast<TableLootEntry*>(entry.get());
    ASSERT_NE(nullptr, tableEntry);
    EXPECT_FALSE(tableEntry->isInline());
    EXPECT_EQ("minecraft:blocks/diamond_ore", tableEntry->getTableId());
    EXPECT_EQ(nullptr, tableEntry->getInlineTable());
}

TEST_F(LootSerializersTest, ParseEntry_LootTable_Inline)
{
    // MC 1.21+：value 为对象时是内联完整掉落表
    nlohmann::json inlineTable = {
        {"pools", {{{"rolls", 1.0f}, {"entries", {{{"type", "minecraft:item"}, {"name", "minecraft:diamond"}}}}}}}};
    nlohmann::json json = {{"type", "minecraft:loot_table"}, {"value", inlineTable}, {"weight", 4}};

    auto result = LootSerializers::parseEntry(json);
    ASSERT_TRUE(result.success());

    auto entry = result.value();
    EXPECT_EQ(LootEntryType::Table, entry->getType());
    EXPECT_EQ(4, entry->getWeight());

    auto* tableEntry = dynamic_cast<TableLootEntry*>(entry.get());
    ASSERT_NE(nullptr, tableEntry);
    EXPECT_TRUE(tableEntry->isInline());
    EXPECT_TRUE(tableEntry->getTableId().empty());
    EXPECT_NE(nullptr, tableEntry->getInlineTable());
    EXPECT_EQ(1u, tableEntry->getInlineTable()->poolCount());
}

TEST_F(LootSerializersTest, ParseEntry_LootTable_MissingValue)
{
    // 缺 value 字段应解析失败
    nlohmann::json json = {{"type", "minecraft:loot_table"}};

    auto result = LootSerializers::parseEntry(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, ParseEntry_LootTable_ToJsonRoundTrip)
{
    nlohmann::json json = {{"type", "minecraft:loot_table"}, {"value", "minecraft:blocks/diamond_ore"}};

    auto result = LootSerializers::parseEntry(json);
    ASSERT_TRUE(result.success());

    nlohmann::json serialized = LootSerializers::toJson(*result.value());
    EXPECT_EQ("minecraft:loot_table", serialized["type"].get<std::string>());
    EXPECT_EQ("minecraft:blocks/diamond_ore", serialized["value"].get<std::string>());
    EXPECT_FALSE(serialized.contains("name"));
}

TEST_F(LootSerializersTest, ParseEntry_Alternatives)
{
    nlohmann::json json = {{"type", "minecraft:alternatives"},
        {"children",
            {{{"type", "minecraft:item"}, {"name", "minecraft:diamond"}},
                {{"type", "minecraft:item"}, {"name", "minecraft:emerald"}}}}};

    auto result = LootSerializers::parseEntry(json);
    ASSERT_TRUE(result.success());

    auto entry = result.value();
    EXPECT_EQ(LootEntryType::Alternatives, entry->getType());
}

TEST_F(LootSerializersTest, ParseEntry_Group)
{
    nlohmann::json json = {{"type", "minecraft:group"},
        {"children",
            {{{"type", "minecraft:item"}, {"name", "minecraft:diamond"}},
                {{"type", "minecraft:item"}, {"name", "minecraft:emerald"}}}}};

    auto result = LootSerializers::parseEntry(json);
    ASSERT_TRUE(result.success());

    auto entry = result.value();
    EXPECT_EQ(LootEntryType::Group, entry->getType());
}

TEST_F(LootSerializersTest, ParseEntry_Sequence)
{
    nlohmann::json json = {{"type", "minecraft:sequence"},
        {"children",
            {{{"type", "minecraft:item"}, {"name", "minecraft:diamond"}},
                {{"type", "minecraft:item"}, {"name", "minecraft:emerald"}}}}};

    auto result = LootSerializers::parseEntry(json);
    ASSERT_TRUE(result.success());

    auto entry = result.value();
    EXPECT_EQ(LootEntryType::Sequence, entry->getType());
}

TEST_F(LootSerializersTest, ParseEntry_WithConditions)
{
    nlohmann::json json = {{"type", "minecraft:item"},
        {"name", "minecraft:diamond"},
        {"conditions", {{{"condition", "minecraft:random_chance"}, {"chance", 0.5f}}}}};

    auto result = LootSerializers::parseEntry(json);
    ASSERT_TRUE(result.success());

    auto entry = result.value();
    EXPECT_EQ(LootEntryType::Item, entry->getType());
    EXPECT_EQ(1, entry->getConditions().size());
}

TEST_F(LootSerializersTest, ParseEntry_MissingName)
{
    nlohmann::json json = {{"type", "minecraft:item"}};

    auto result = LootSerializers::parseEntry(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, ParseEntry_UnknownType)
{
    nlohmann::json json = {{"type", "minecraft:unknown_type"}};

    auto result = LootSerializers::parseEntry(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// LootPool Parsing Tests
// ============================================================================

TEST_F(LootSerializersTest, ParsePool_Basic)
{
    nlohmann::json json = {{"rolls", 2}, {"entries", {{{"type", "minecraft:item"}, {"name", "minecraft:diamond"}}}}};

    auto result = LootSerializers::parsePool(json);
    ASSERT_TRUE(result.success());

    auto pool = result.value();
    EXPECT_FLOAT_EQ(2.0f, pool->getRolls().getMin());
    EXPECT_FLOAT_EQ(2.0f, pool->getRolls().getMax());
    EXPECT_EQ(1u, pool->getEntries().size());
}

TEST_F(LootSerializersTest, ParsePool_WithBonusRolls)
{
    nlohmann::json json = {{"rolls", {{"min", 1}, {"max", 3}}},
        {"bonus_rolls", {{"min", 0}, {"max", 1}}},
        {"entries", {{{"type", "minecraft:item"}, {"name", "minecraft:diamond"}}}}};

    auto result = LootSerializers::parsePool(json);
    ASSERT_TRUE(result.success());

    auto pool = result.value();
    EXPECT_FLOAT_EQ(1.0f, pool->getRolls().getMin());
    EXPECT_FLOAT_EQ(3.0f, pool->getRolls().getMax());
    EXPECT_FLOAT_EQ(0.0f, pool->getBonusRolls().getMin());
    EXPECT_FLOAT_EQ(1.0f, pool->getBonusRolls().getMax());
}

TEST_F(LootSerializersTest, ParsePool_WithMultipleEntries)
{
    nlohmann::json json = {{"rolls", 1},
        {"entries",
            {{{"type", "minecraft:item"}, {"name", "minecraft:diamond"}, {"weight", 10}},
                {{"type", "minecraft:item"}, {"name", "minecraft:emerald"}, {"weight", 5}},
                {{"type", "minecraft:empty"}, {"weight", 85}}}}};

    auto result = LootSerializers::parsePool(json);
    ASSERT_TRUE(result.success());

    auto pool = result.value();
    EXPECT_EQ(3u, pool->getEntries().size());
}

TEST_F(LootSerializersTest, ParsePool_MissingRolls)
{
    nlohmann::json json = {{"entries", {{{"type", "minecraft:item"}, {"name", "minecraft:diamond"}}}}};

    auto result = LootSerializers::parsePool(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// LootTable Parsing Tests
// ============================================================================

TEST_F(LootSerializersTest, ParseLootTable_Empty)
{
    nlohmann::json json = nlohmann::json::object();

    auto result = LootSerializers::parseLootTable(json);
    ASSERT_TRUE(result.success());

    auto table = result.value();
    EXPECT_EQ(0u, table->getPools().size());
}

TEST_F(LootSerializersTest, ParseLootTable_SinglePool)
{
    nlohmann::json json = {
        {"pools", {{{"rolls", 1}, {"entries", {{{"type", "minecraft:item"}, {"name", "minecraft:diamond"}}}}}}}};

    auto result = LootSerializers::parseLootTable(json);
    ASSERT_TRUE(result.success());

    auto table = result.value();
    EXPECT_EQ(1u, table->getPools().size());
}

TEST_F(LootSerializersTest, ParseLootTable_MultiplePools)
{
    nlohmann::json json = {{"pools",
        {{{"rolls", 1}, {"entries", {{{"type", "minecraft:item"}, {"name", "minecraft:diamond"}}}}},
            {{"rolls", 2}, {"entries", {{{"type", "minecraft:item"}, {"name", "minecraft:emerald"}}}}}}}};

    auto result = LootSerializers::parseLootTable(json);
    ASSERT_TRUE(result.success());

    auto table = result.value();
    EXPECT_EQ(2u, table->getPools().size());
}

TEST_F(LootSerializersTest, ParseLootTable_ComplexExample)
{
    // 典型的方块掉落表 JSON
    nlohmann::json json = {{"type", "minecraft:block"},
        {"pools",
            {{{"rolls", 1},
                {"entries",
                    {{{"type", "minecraft:alternatives"},
                        {"children",
                            {{{"type", "minecraft:item"},
                                 {"name", "minecraft:diamond_ore"},
                                 {"conditions",
                                     {{{"condition", "minecraft:match_tool"},
                                         {"predicate",
                                             {{"enchantments", {{{"enchantment", "minecraft:silk_touch"}}}}}}}}}},
                                {{"type", "minecraft:item"},
                                    {"name", "minecraft:diamond"},
                                    {"functions",
                                        {{{"function", "minecraft:apply_bonus"},
                                             {"enchantment", "minecraft:fortune"},
                                             {"formula", "minecraft:ore_drops"}},
                                            {{"function", "minecraft:explosion_decay"}}}}}}}}}}}}}};

    auto result = LootSerializers::parseLootTable(json);
    ASSERT_TRUE(result.success());

    auto table = result.value();
    EXPECT_EQ(1u, table->getPools().size());
}

TEST_F(LootSerializersTest, ParseLootTable_FromString)
{
    std::string jsonStr = R"({
        "pools": [
            {
                "rolls": 1,
                "entries": [
                    {"type": "minecraft:item", "name": "minecraft:diamond"}
                ]
            }
        ]
    })";

    auto result = LootSerializers::parseLootTable(jsonStr);
    ASSERT_TRUE(result.success());

    auto table = result.value();
    EXPECT_EQ(1u, table->getPools().size());
}

TEST_F(LootSerializersTest, ParseLootTable_InvalidJson)
{
    std::string jsonStr = "{ invalid json }";

    auto result = LootSerializers::parseLootTable(jsonStr);
    EXPECT_FALSE(result.success());
}

TEST_F(LootSerializersTest, ParseLootTable_NotAnObject)
{
    nlohmann::json json = "not an object";

    auto result = LootSerializers::parseLootTable(json);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// Serialization Tests
// ============================================================================

TEST_F(LootSerializersTest, ToJson_RandomValueRange)
{
    RandomValueRange range(1.0f, 5.0f);
    nlohmann::json json = LootSerializers::toJson(range);

    EXPECT_TRUE(json.is_object());
    EXPECT_FLOAT_EQ(1.0f, json["min"].get<f32>());
    EXPECT_FLOAT_EQ(5.0f, json["max"].get<f32>());
}

TEST_F(LootSerializersTest, ToJson_RandomValueRange_Fixed)
{
    RandomValueRange range(3.0f);
    nlohmann::json json = LootSerializers::toJson(range);

    EXPECT_TRUE(json.is_number_integer());
    EXPECT_EQ(3, json.get<i32>());
}

TEST_F(LootSerializersTest, ToJson_BinomialRange)
{
    BinomialRange range(10, 0.5f);
    nlohmann::json json = LootSerializers::toJson(range);

    EXPECT_TRUE(json.is_object());
    EXPECT_EQ("minecraft:binomial", json["type"].get<std::string>());
    EXPECT_EQ(10, json["n"].get<i32>());
    EXPECT_FLOAT_EQ(0.5f, json["p"].get<f32>());
}

TEST_F(LootSerializersTest, ToJson_ConstantRange)
{
    ConstantRange range(42);
    nlohmann::json json = LootSerializers::toJson(range);

    EXPECT_TRUE(json.is_number_integer());
    EXPECT_EQ(42, json.get<i32>());
}

TEST_F(LootSerializersTest, ToJson_LootTable)
{
    auto table = std::make_unique<LootTable>();
    auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 3.0f));
    pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f, 1.0f), 1, 0));
    table->addPool(std::move(pool));

    nlohmann::json json = LootSerializers::toJson(*table);

    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("pools"));
    EXPECT_TRUE(json["pools"].is_array());
    EXPECT_EQ(1u, json["pools"].size());
}

TEST_F(LootSerializersTest, ToJsonString_Pretty)
{
    auto table = std::make_unique<LootTable>();
    auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f));
    pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0));
    table->addPool(std::move(pool));

    std::string jsonStr = LootSerializers::toJsonString(*table, 2);

    EXPECT_TRUE(jsonStr.find("\"pools\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"entries\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"minecraft:diamond\"") != std::string::npos);
}

TEST_F(LootSerializersTest, ToJsonString_Compact)
{
    auto table = std::make_unique<LootTable>();
    auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f));
    pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0));
    table->addPool(std::move(pool));

    std::string jsonStr = LootSerializers::toJsonString(*table); // 默认紧凑格式

    EXPECT_TRUE(jsonStr.find("\"pools\"") != std::string::npos);
    // 紧凑格式不应该有缩进
    EXPECT_TRUE(jsonStr.find("\n  ") == std::string::npos);
}

// ============================================================================
// Round-trip Tests
// ============================================================================

TEST_F(LootSerializersTest, RoundTrip_SimpleTable)
{
    // 创建原始掉落表
    auto originalTable = std::make_unique<LootTable>();
    auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 3.0f));
    pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f, 2.0f), 10, 2));
    pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:emerald", RandomValueRange(1.0f, 1.0f), 5, 1));
    originalTable->addPool(std::move(pool));

    // 序列化
    std::string jsonStr = LootSerializers::toJsonString(*originalTable, 2);

    // 反序列化
    auto parseResult = LootSerializers::parseLootTable(jsonStr);
    ASSERT_TRUE(parseResult.success());

    auto parsedTable = parseResult.value();
    ASSERT_EQ(1u, parsedTable->getPools().size());

    const auto& parsedPool = parsedTable->getPools()[0];
    EXPECT_FLOAT_EQ(1.0f, parsedPool->getRolls().getMin());
    EXPECT_FLOAT_EQ(3.0f, parsedPool->getRolls().getMax());
    EXPECT_EQ(2u, parsedPool->getEntries().size());
}

TEST_F(LootSerializersTest, RoundTrip_TableWithConditions)
{
    // 创建带条件的掉落表
    auto originalTable = std::make_unique<LootTable>();
    auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f));

    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0);
    entry->addCondition(std::make_unique<SilkTouchCondition>());
    pool->addEntry(std::move(entry));

    originalTable->addPool(std::move(pool));

    // 序列化
    std::string jsonStr = LootSerializers::toJsonString(*originalTable, 2);

    // 反序列化
    auto parseResult = LootSerializers::parseLootTable(jsonStr);
    ASSERT_TRUE(parseResult.success());

    auto parsedTable = parseResult.value();
    ASSERT_EQ(1u, parsedTable->getPools().size());
    ASSERT_EQ(1u, parsedTable->getPools()[0]->getEntries().size());

    const auto& parsedEntry = parsedTable->getPools()[0]->getEntries()[0];
    EXPECT_EQ(1u, parsedEntry->getConditions().size());
    EXPECT_EQ("silk_touch", parsedEntry->getConditions()[0]->getType());
}

// ============================================================================
// MatchTool Condition Parsing Tests (端到端集成测试)
// ============================================================================

TEST_F(LootSerializersTest, ParseCondition_MatchTool_NoPredicate)
{
    // 无谓词的 match_tool 条件：仅检查是否存在非空工具
    nlohmann::json json = {{"condition", "minecraft:match_tool"}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("match_tool", condition->getType());

    // 验证条件可以通过 dynamic_cast 转换为 MatchToolCondition
    auto* matchTool = dynamic_cast<MatchToolCondition*>(condition.get());
    ASSERT_NE(matchTool, nullptr);
    EXPECT_FALSE(matchTool->getPredicate().has_value());
}

TEST_F(LootSerializersTest, ParseCondition_MatchTool_WithItemPredicate)
{
    // 带物品谓词的 match_tool 条件：匹配特定物品ID
    nlohmann::json json = {
        {"condition", "minecraft:match_tool"}, {"predicate", {{"item", "minecraft:diamond_pickaxe"}}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("match_tool", condition->getType());

    auto* matchTool = dynamic_cast<MatchToolCondition*>(condition.get());
    ASSERT_NE(matchTool, nullptr);
    EXPECT_TRUE(matchTool->getPredicate().has_value());
}

TEST_F(LootSerializersTest, ParseCondition_MatchTool_WithEnchantments)
{
    // 带附魔谓词的 match_tool 条件：匹配精准采集附魔
    nlohmann::json json = {{"condition", "minecraft:match_tool"},
        {"predicate", {{"enchantments", {{{"enchantment", "minecraft:silk_touch"}, {"levels", {{"min", 1}}}}}}}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("match_tool", condition->getType());

    auto* matchTool = dynamic_cast<MatchToolCondition*>(condition.get());
    ASSERT_NE(matchTool, nullptr);
    EXPECT_TRUE(matchTool->getPredicate().has_value());
}

TEST_F(LootSerializersTest, ParseCondition_MatchTool_WithEmptyPredicate)
{
    // 空 predicate 对象（无任何匹配条件，等同于 isAny）
    nlohmann::json json = {{"condition", "minecraft:match_tool"}, {"predicate", nlohmann::json::object()}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("match_tool", condition->getType());

    auto* matchTool = dynamic_cast<MatchToolCondition*>(condition.get());
    ASSERT_NE(matchTool, nullptr);
    EXPECT_TRUE(matchTool->getPredicate().has_value());
    // 空 predicate 对象解析为 isAny() == true
    EXPECT_TRUE(matchTool->getPredicate()->isAny());
}

TEST_F(LootSerializersTest, ParseCondition_MatchTool_WithItemAndTag)
{
    // 带标签谓词的 match_tool 条件
    nlohmann::json json = {
        {"condition", "minecraft:match_tool"}, {"predicate", {{"tag", "minecraft:mineable/pickaxe"}}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("match_tool", condition->getType());
}
