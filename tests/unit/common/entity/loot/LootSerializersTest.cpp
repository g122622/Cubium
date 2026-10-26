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
#include "item/enchantment/EnchantmentRegistry.hpp"
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
    void SetUp() override
    {
        Items::ensureInitialized();
        // set_enchantments 在加载期校验附魔注册表
        item::enchant::EnchantmentRegistry::initialize();
    }

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

TEST_F(LootSerializersTest, ParseBinomialRange_DoubleEncodedFields)
{
    // 原版数据包以 double 编码 n/p（如 {"type":"minecraft:binomial","n":3.0,"p":0.53333336}），
    // 此前要求 n 为整数导致 blocks/melon_stem 等 4 张表加载失败
    nlohmann::json json = {{"type", "minecraft:binomial"}, {"n", 3.0}, {"p", 0.53333336}};

    auto result = LootSerializers::parseBinomialRange(json);
    ASSERT_TRUE(result.success()) << result.error().message();
    BinomialRange range = result.value();
    EXPECT_EQ(3, range.getN());
    EXPECT_NEAR(0.53333336f, range.getP(), 1e-6f);
}

TEST_F(LootSerializersTest, ParseBinomialRange_RejectsNegativeN)
{
    nlohmann::json json = {{"type", "minecraft:binomial"}, {"n", -1.0}, {"p", 0.5}};
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

TEST_F(LootSerializersTest, ParseCondition_PrecisionToolUsesMatchTool)
{
    // 原版没有 `silk_touch` 条件：精准采集靠 `match_tool` 的 `predicates.enchantments`
    // 表达。这里验证同一条路径能解析。
    nlohmann::json json = {{"condition", "minecraft:match_tool"},
        {"predicates", {{"enchantments", {{{"enchantment", "minecraft:silk_touch"}, {"levels", {{"min", 1}}}}}}}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());

    auto condition = result.value();
    EXPECT_EQ("match_tool", condition->getType());
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
    // 原版 `enchanted_count_increase` 必须指明附魔（数据包里的 `enchantment` 字段）。
    nlohmann::json json = {{"function", "minecraft:enchanted_count_increase"},
        {"count", {{"min", 0}, {"max", 1}}},
        {"limit", 3},
        {"enchantment", "minecraft:looting"}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto function = result.value();
    EXPECT_EQ("enchanted_count_increase", function->getType());
}

TEST_F(LootSerializersTest, ParseFunction_SetCustomDataRejectsMalformedTag)
{
    // 自定义数据在加载期解析：tag 非法时整张战利品表解析失败，而不是掉落时静默忽略
    nlohmann::json json = {{"function", "minecraft:set_custom_data"}, {"tag", "{invalid nbt string"}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_FALSE(result.success());
    EXPECT_EQ(ErrorCode::InvalidData, result.error().code());

    nlohmann::json emptyTag = {{"function", "minecraft:set_custom_data"}, {"tag", ""}};
    auto emptyResult = LootSerializers::parseFunction(emptyTag);
    EXPECT_FALSE(emptyResult.success());
}

TEST_F(LootSerializersTest, ParseFunction_SetCustomDataParsesTagAtLoad)
{
    nlohmann::json json = {{"function", "minecraft:set_custom_data"}, {"tag", "{CustomKey:42}"}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success()) << (!result.success() ? result.error().message() : "");

    auto function = result.value();
    ASSERT_NE(function, nullptr);
    EXPECT_EQ("set_custom_data", function->getType());

    // 加载期已解析：掉落期内直接合并，无需再次解析 Mojangson
    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();
    ItemStack applied = function->apply(stack, *context);
    ASSERT_TRUE(applied.hasTag());
    const nlohmann::json* tag = applied.getTag();
    ASSERT_NE(tag, nullptr);
    auto it = tag->find("CustomKey");
    ASSERT_NE(it, tag->end());
    EXPECT_EQ(42, it->get<i32>());
}

TEST_F(LootSerializersTest, ParseFunction_AllVanillaTypesSupported)
{
    // 原版 LootItemFunctions 注册的全部类型均须有解析分支（不再有"未实现即跳过"的清单）
    const std::vector<std::string> allTypes = {"set_count",
        "apply_bonus",
        "enchanted_count_increase",
        "set_damage",
        "set_name",
        "set_lore",
        "limit_count",
        "furnace_smelt",
        "enchant_with_levels",
        "enchant_randomly",
        "explosion_decay",
        "set_custom_data",
        "copy_name",
        "copy_state",
        "copy_custom_data",
        "fill_player_head",
        "set_attributes",
        "set_contents",
        "set_loot_table",
        "exploration_map",
        "set_stew_effect",
        "set_enchantments",
        "set_potion",
        "set_instrument",
        "copy_components",
        "set_components",
        "set_ominous_bottle_amplifier",
        "set_item",
        "filtered",
        "sequence",
        "set_fireworks",
        "set_firework_explosion",
        "toggle_tooltips",
        "set_custom_model_data",
        "set_written_book_pages",
        "set_writable_book_pages",
        "set_book_cover",
        "set_banner_pattern",
        "discard"};
    for (const auto& type : allTypes) {
        nlohmann::json json = {{"function", "minecraft:" + type}};
        // 必填字段缺失时允许解析失败，但不能报"Unknown function type"
        auto result = LootSerializers::parseFunction(json);
        if (!result.success()) {
            EXPECT_EQ(std::string::npos, result.error().message().find("Unknown function type"))
                << type << ": " << result.error().message();
        }
    }

    // 真正未注册的类型仍按原版语义让整张表加载失败
    nlohmann::json unknown = {{"function", "minecraft:not_a_real_function"}};
    auto unknownResult = LootSerializers::parseFunction(unknown);
    ASSERT_FALSE(unknownResult.success());
    EXPECT_EQ(ErrorCode::InvalidData, unknownResult.error().code());
}

TEST_F(LootSerializersTest, ParsePool_AllImplementedFunctionsKeepWorking)
{
    // 同一函数列表中的多种已实现类型按顺序全部生效
    const nlohmann::json entry = {{"type", "minecraft:item"},
        {"name", "minecraft:diamond"},
        {"functions",
            {{{"function", "minecraft:set_count"}, {"count", 3}},
                {{"function", "minecraft:set_enchantments"}, {"enchantments", {{"minecraft:fortune", 2}}}},
                {{"function", "minecraft:set_name"}, {"name", "Test"}}}}};
    const nlohmann::json json = {{"rolls", 1}, {"entries", {entry}}};
    auto result = LootSerializers::parsePool(json);
    ASSERT_TRUE(result.success()) << (!result.success() ? result.error().message() : "");
    auto pool = result.value();
    ASSERT_NE(pool, nullptr);
    const auto& entries = pool->getEntries();
    ASSERT_EQ(entries.size(), 1u);
    const auto& functions = entries[0]->getFunctions();
    ASSERT_EQ(functions.size(), 3u);
    EXPECT_EQ("set_count", functions[0]->getType());
    EXPECT_EQ("set_enchantments", functions[1]->getType());
    EXPECT_EQ("set_name", functions[2]->getType());
}

TEST_F(LootSerializersTest, ParseLootTable_VanillaDatapackStyleFunctions)
{
    // 回归：原版数据包 chests/buried_treasure.json 使用 set_potion；
    // chests/trial_chambers 使用 set_components/copy_components，按标签搜索的 exploration_map 等。
    const nlohmann::json entry = {{"type", "minecraft:item"},
        {"name", "minecraft:potion"},
        {"functions", {{{"function", "minecraft:set_potion"}, {"id", "minecraft:water_breathing"}}}}};
    const nlohmann::json pool = {{"rolls", 1}, {"entries", {entry}}};
    const nlohmann::json json = {{"type", "minecraft:chest"}, {"pools", {pool}}};
    auto result = LootSerializers::parseLootTable(json.dump());
    ASSERT_TRUE(result.success()) << (!result.success() ? result.error().message() : "");
    auto table = result.value();
    ASSERT_NE(table, nullptr);
    ASSERT_EQ(table->getPools().size(), 1u);
    const auto& entries = table->getPools()[0]->getEntries();
    ASSERT_EQ(entries.size(), 1u);
    ASSERT_EQ(entries[0]->getFunctions().size(), 1u);
    EXPECT_EQ("set_potion", entries[0]->getFunctions()[0]->getType());
}

TEST_F(LootSerializersTest, ParsePool_RejectsEntryWithUnknownFunction)
{
    // 掉落项函数列表中含未注册函数：整体解析失败（不允许静默丢弃该函数）
    const nlohmann::json entry = {{"type", "minecraft:item"},
        {"name", "minecraft:diamond"},
        {"functions", {{{"function", "minecraft:not_a_real_function"}}}}};
    const nlohmann::json json = {{"rolls", 1}, {"entries", {entry}}};
    auto result = LootSerializers::parsePool(json);
    ASSERT_FALSE(result.success());
    EXPECT_EQ(ErrorCode::InvalidData, result.error().code());
}

TEST_F(LootSerializersTest, ParseLootTable_RejectsPoolWithMalformedFunction)
{
    // 池级函数非法（set_custom_data 的 tag 不是合法 Mojangson）：整张表加载失败
    const nlohmann::json entry = {{"type", "minecraft:item"}, {"name", "minecraft:stone"}};
    const nlohmann::json pool = {{"rolls", 1},
        {"entries", {entry}},
        {"functions", {{{"function", "minecraft:set_custom_data"}, {"tag", "{broken"}}}}};
    const nlohmann::json json = {{"type", "minecraft:chest"}, {"pools", {pool}}};
    auto result = LootSerializers::parseLootTable(json.dump());
    ASSERT_FALSE(result.success());
    EXPECT_EQ(ErrorCode::InvalidData, result.error().code());
}

TEST_F(LootSerializersTest, ParseLootTable_AcceptsValidEntryFunctions)
{
    const nlohmann::json entry = {{"type", "minecraft:item"},
        {"name", "minecraft:stone"},
        {"functions", {{{"function", "minecraft:set_count"}, {"count", 3}}}}};
    const nlohmann::json pool = {{"rolls", 1}, {"entries", {entry}}};
    const nlohmann::json json = {{"type", "minecraft:chest"}, {"pools", {pool}}};
    auto result = LootSerializers::parseLootTable(json.dump());
    ASSERT_TRUE(result.success()) << (!result.success() ? result.error().message() : "");
    // value() 为转移所有权语义：只能取一次
    auto table = result.value();
    ASSERT_NE(table, nullptr);
    ASSERT_EQ(table->getPools().size(), 1u);
    const auto& entries = table->getPools()[0]->getEntries();
    ASSERT_EQ(entries.size(), 1u);
    ASSERT_EQ(entries[0]->getFunctions().size(), 1u);
    EXPECT_EQ("set_count", entries[0]->getFunctions()[0]->getType());
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
    nlohmann::json json = {{"function", "minecraft:copy_custom_data"}, {"source", "block_entity"}};

    auto result = LootSerializers::parseFunction(json);
    // 源码 LootSerializers 已实现 minecraft:copy_custom_data 函数解析（_parseCopyCustomDataFunction），
    // ops 数组可选；此前测试期望 Unsupported，是源码实现前的过时期望。
    // 仅断言解析成功与类型标识，避免对实现细节（getSource）过度耦合。
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value()->getType(), "copy_custom_data");
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap)
{
    // 显式 destination 是结构标签；未指定 decoration 时使用默认 woodland_mansion，
    // 不再由 destination 推导
    nlohmann::json json = {{"function", "minecraft:exploration_map"}, {"destination", "minecraft:mansion"}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    // 注意：Result<std::unique_ptr<T>>::value() 按值返回并转移所有权，
    // 必须用局部变量接住该 unique_ptr，否则对象在语句结束时即被析构。
    auto function = result.value();
    auto* func = dynamic_cast<ExplorationMapFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ResourceLocation("minecraft:mansion"), func->getDestination());
    EXPECT_EQ(ExplorationMapFunction::DEFAULT_ZOOM, func->getZoom());
    EXPECT_EQ(ExplorationMapFunction::DEFAULT_SEARCH_RADIUS, func->getSearchRadius());
    EXPECT_TRUE(func->shouldSearchSkipLoadedChunks());
    EXPECT_EQ(world::map::DecorationType::MANSION, func->getDecoration());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_Defaults)
{
    // 空 JSON：destination=on_treasure_maps、decoration=woodland_mansion、zoom=2、
    // search_radius=50、skip_existing_chunks=true
    nlohmann::json json = {{"function", "minecraft:exploration_map"}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto function = result.value();
    auto* func = dynamic_cast<ExplorationMapFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ExplorationMapFunction::defaultDestination(), func->getDestination());
    EXPECT_EQ(ResourceLocation("minecraft:on_treasure_maps"), func->getDestination());
    EXPECT_EQ(world::map::DecorationType::MANSION, func->getDecoration());
    EXPECT_EQ(2, func->getZoom());
    EXPECT_EQ(50, func->getSearchRadius());
    EXPECT_TRUE(func->shouldSearchSkipLoadedChunks());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_AllFields)
{
    nlohmann::json json = {
        {"function", "minecraft:exploration_map"},
        {"destination", "minecraft:on_woodland_explorer_maps"},
        {"decoration", "minecraft:red_x"},
        {"zoom", 1},
        {"search_radius", 100},
        {"skip_existing_chunks", false},
    };

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto function = result.value();
    auto* func = dynamic_cast<ExplorationMapFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ResourceLocation("minecraft:on_woodland_explorer_maps"), func->getDestination());
    EXPECT_EQ(world::map::DecorationType::RED_X, func->getDecoration());
    EXPECT_EQ(1, func->getZoom());
    EXPECT_EQ(100, func->getSearchRadius());
    EXPECT_FALSE(func->shouldSearchSkipLoadedChunks());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_VanillaDatapack)
{
    // 原版数据包 shipwreck_map / underwater_ruin 使用：只指定 decoration 与 zoom，
    // destination 使用默认的 on_treasure_maps
    nlohmann::json json = {
        {"function", "minecraft:exploration_map"},
        {"decoration", "minecraft:red_x"},
        {"skip_existing_chunks", false},
        {"zoom", 1},
    };

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto function = result.value();
    auto* func = dynamic_cast<ExplorationMapFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ResourceLocation("minecraft:on_treasure_maps"), func->getDestination());
    EXPECT_EQ(world::map::DecorationType::RED_X, func->getDecoration());
    EXPECT_EQ(1, func->getZoom());
    EXPECT_EQ(50, func->getSearchRadius());
    EXPECT_FALSE(func->shouldSearchSkipLoadedChunks());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_DestinationWithoutNamespace)
{
    // 不带命名空间前缀时按资源位置默认命名空间解析为 minecraft
    nlohmann::json json = {{"function", "minecraft:exploration_map"}, {"destination", "on_ocean_explorer_maps"}};

    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success());

    auto function = result.value();
    auto* func = dynamic_cast<ExplorationMapFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ResourceLocation("minecraft:on_ocean_explorer_maps"), func->getDestination());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_InvalidDestinationFails)
{
    // destination 是资源位置：语法非法（空路径）时整张战利品表加载失败
    for (const char* value : {"", "minecraft:", ":"}) {
        nlohmann::json json = {{"function", "minecraft:exploration_map"}, {"destination", value}};
        auto result = LootSerializers::parseFunction(json);
        ASSERT_FALSE(result.success()) << value;
        EXPECT_EQ(ErrorCode::InvalidData, result.error().code()) << value;
    }

    // 非字符串类型同样失败（不再静默回退默认值）
    nlohmann::json wrongType = {{"function", "minecraft:exploration_map"}, {"destination", 42}};
    auto wrongTypeResult = LootSerializers::parseFunction(wrongType);
    ASSERT_FALSE(wrongTypeResult.success());
    EXPECT_EQ(ErrorCode::InvalidData, wrongTypeResult.error().code());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_InvalidDecorationFails)
{
    // decoration 是注册表项：无法识别时整张战利品表加载失败
    for (const char* value : {"nonexistent_type", "minecraft:not_a_decoration"}) {
        nlohmann::json json = {{"function", "minecraft:exploration_map"}, {"decoration", value}};
        auto result = LootSerializers::parseFunction(json);
        ASSERT_FALSE(result.success()) << value;
        EXPECT_EQ(ErrorCode::InvalidData, result.error().code()) << value;
    }

    nlohmann::json wrongType = {{"function", "minecraft:exploration_map"}, {"decoration", 7}};
    auto wrongTypeResult = LootSerializers::parseFunction(wrongType);
    ASSERT_FALSE(wrongTypeResult.success());
    EXPECT_EQ(ErrorCode::InvalidData, wrongTypeResult.error().code());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_ZoomMustFitByte)
{
    // zoom 按字节编码：-128..127 合法，越界或非整数失败
    for (const char* value : {"128", "-129", "100000", "1.5", "\"2\""}) {
        nlohmann::json json =
            nlohmann::json::parse(std::string("{\"function\":\"minecraft:exploration_map\",\"zoom\":") + value + "}");
        auto result = LootSerializers::parseFunction(json);
        ASSERT_FALSE(result.success()) << value;
        EXPECT_EQ(ErrorCode::InvalidData, result.error().code()) << value;
    }

    nlohmann::json maxZoom = {{"function", "minecraft:exploration_map"}, {"zoom", 127}};
    auto maxResult = LootSerializers::parseFunction(maxZoom);
    ASSERT_TRUE(maxResult.success());
    auto maxFunc = maxResult.value();
    auto* func = dynamic_cast<ExplorationMapFunction*>(maxFunc.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(127, func->getZoom());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_NegativeSearchRadiusFails)
{
    nlohmann::json json = {{"function", "minecraft:exploration_map"}, {"search_radius", -1}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_FALSE(result.success());
    EXPECT_EQ(ErrorCode::InvalidData, result.error().code());

    nlohmann::json zero = {{"function", "minecraft:exploration_map"}, {"search_radius", 0}};
    auto zeroResult = LootSerializers::parseFunction(zero);
    ASSERT_TRUE(zeroResult.success());
    auto function = zeroResult.value();
    auto* func = dynamic_cast<ExplorationMapFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(0, func->getSearchRadius());
}

TEST_F(LootSerializersTest, ParseFunction_ExplorationMap_SkipExistingMustBeBoolean)
{
    nlohmann::json json = {{"function", "minecraft:exploration_map"}, {"skip_existing_chunks", "false"}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_FALSE(result.success());
    EXPECT_EQ(ErrorCode::InvalidData, result.error().code());
}

TEST_F(LootSerializersTest, ParseFunction_SetEnchantments)
{
    nlohmann::json json = {
        {"function", "minecraft:set_enchantments"}, {"enchantments", {{"minecraft:fortune", 3}, {"unbreaking", 1}}}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success()) << result.error().message();
    auto function = result.value();
    auto* func = dynamic_cast<SetEnchantmentsFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->getEnchantments().size(), 2u);
    EXPECT_EQ(ResourceLocation("minecraft:fortune"), func->getEnchantments()[0].enchantmentId);
    EXPECT_EQ(ResourceLocation("minecraft:unbreaking"), func->getEnchantments()[1].enchantmentId);
    EXPECT_FALSE(func->isAdd());

    // unknown enchantment -> load failure
    nlohmann::json bad = {
        {"function", "minecraft:set_enchantments"}, {"enchantments", {{"minecraft:not_an_enchantment", 1}}}};
    EXPECT_FALSE(LootSerializers::parseFunction(bad).success());
}

TEST_F(LootSerializersTest, ParseFunction_SetPotion)
{
    nlohmann::json json = {{"function", "minecraft:set_potion"}, {"id", "minecraft:strong_regeneration"}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success()) << result.error().message();
    auto function = result.value();
    EXPECT_EQ("set_potion", function->getType());

    nlohmann::json bad = {{"function", "minecraft:set_potion"}, {"id", "minecraft:not_a_potion"}};
    EXPECT_FALSE(LootSerializers::parseFunction(bad).success());
}

TEST_F(LootSerializersTest, ParseFunction_SetInstrument)
{
    nlohmann::json json = {{"function", "minecraft:set_instrument"}, {"options", "#minecraft:regular_goat_horns"}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success()) << result.error().message();
    auto function = result.value();
    auto* func = dynamic_cast<SetInstrumentFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ResourceLocation("minecraft:regular_goat_horns"), func->getOptionsTag());
}

TEST_F(LootSerializersTest, ParseFunction_CopyComponents)
{
    nlohmann::json json = {
        {"function", "minecraft:copy_components"}, {"source", "block_entity"}, {"include", {"minecraft:custom_name"}}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success()) << result.error().message();
    auto function = result.value();
    auto* func = dynamic_cast<CopyComponentsFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(CopyComponentsFunction::Source::BlockEntity, func->getSource());
    ASSERT_EQ(func->getInclude().size(), 1u);
    EXPECT_EQ(item::component::DataComponentType::CustomName, func->getInclude()[0]);

    nlohmann::json bad = {{"function", "minecraft:copy_components"}, {"include", {"minecraft:not_a_component"}}};
    EXPECT_FALSE(LootSerializers::parseFunction(bad).success());
}

TEST_F(LootSerializersTest, ParseFunction_SetComponents)
{
    nlohmann::json json = {{"function", "minecraft:set_components"},
        {"components",
            {{"minecraft:enchantment_glint_override", true}, {"minecraft:custom_model_data", {{"floats", {1.5}}}}}}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success()) << result.error().message();
    auto function = result.value();
    auto* func = dynamic_cast<SetComponentsFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getComponents().added().size(), 2u);

    // 未知组件 -> 加载失败
    nlohmann::json bad = {
        {"function", "minecraft:set_components"}, {"components", {{"minecraft:no_such_component", 1}}}};
    EXPECT_FALSE(LootSerializers::parseFunction(bad).success());
}

TEST_F(LootSerializersTest, ParseFunction_SetOminousBottleAmplifier)
{
    nlohmann::json json = {{"function", "minecraft:set_ominous_bottle_amplifier"},
        {"amplifier", {{"type", "minecraft:uniform"}, {"min", 2}, {"max", 4}}}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success()) << result.error().message();
    auto function = result.value();
    auto* func = dynamic_cast<SetOminousBottleAmplifierFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_FLOAT_EQ(2.0f, func->getAmplifier().getMin());
    EXPECT_FLOAT_EQ(4.0f, func->getAmplifier().getMax());
}

TEST_F(LootSerializersTest, ParseFunction_SetItem)
{
    nlohmann::json json = {{"function", "minecraft:set_item"}, {"item", "diamond"}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success()) << result.error().message();
    auto function = result.value();
    auto* func = dynamic_cast<SetItemFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(ResourceLocation("minecraft:diamond"), func->getItemId());

    nlohmann::json bad = {{"function", "minecraft:set_item"}, {"item", "minecraft:not_an_item"}};
    EXPECT_FALSE(LootSerializers::parseFunction(bad).success());
}

TEST_F(LootSerializersTest, ParseFunction_FilteredAndSequenceAndDiscard)
{
    nlohmann::json filtered = {{"function", "minecraft:filtered"},
        {"item_filter", {{"items", "minecraft:diamond"}}},
        {"on_pass", {{"function", "minecraft:set_count"}, {"count", 2}}}};
    auto filteredResult = LootSerializers::parseFunction(filtered);
    ASSERT_TRUE(filteredResult.success()) << filteredResult.error().message();
    auto filteredFunction = filteredResult.value();
    auto* filteredFunc = dynamic_cast<FilteredFunction*>(filteredFunction.get());
    ASSERT_NE(filteredFunc, nullptr);
    EXPECT_TRUE(filteredFunc->getOnPass().has_value());
    EXPECT_FALSE(filteredFunc->getOnFail().has_value());

    nlohmann::json sequence = {{"function", "minecraft:sequence"},
        {"functions", {{{"function", "minecraft:set_count"}, {"count", 1}}, {{"function", "minecraft:set_name"}}}}};
    auto sequenceResult = LootSerializers::parseFunction(sequence);
    ASSERT_TRUE(sequenceResult.success()) << sequenceResult.error().message();
    auto sequenceFunction = sequenceResult.value();
    auto* sequenceFunc = dynamic_cast<SequenceFunction*>(sequenceFunction.get());
    ASSERT_NE(sequenceFunc, nullptr);
    EXPECT_EQ(sequenceFunc->getFunctions().size(), 2u);

    nlohmann::json discard = {{"function", "minecraft:discard"}};
    auto discardResult = LootSerializers::parseFunction(discard);
    ASSERT_TRUE(discardResult.success());
    EXPECT_EQ("discard", discardResult.value()->getType());
}

TEST_F(LootSerializersTest, ParseFunction_SetFireworksAndExplosion)
{
    nlohmann::json json = {{"function", "minecraft:set_fireworks"},
        {"flight_duration", 2},
        {"explosions", {{"mode", "append"}, {"explosions", {{{"shape", "creeper"}, {"colors", {0xFF0000}}}}}}}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success()) << result.error().message();
    auto function = result.value();
    auto* func = dynamic_cast<SetFireworksFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_TRUE(func->getFlightDuration().has_value());
    EXPECT_EQ(2, *func->getFlightDuration());
    ASSERT_TRUE(func->getExplosions().has_value());
    EXPECT_EQ(ListOperationKind::Append, func->getExplosions()->kind);
    ASSERT_EQ(func->getExplosions()->explosions.size(), 1u);
    EXPECT_EQ(item::component::FireworkShape::Creeper, func->getExplosions()->explosions[0].shape);

    nlohmann::json explosion = {
        {"function", "minecraft:set_firework_explosion"}, {"shape", "star"}, {"colors", {0x00FF00}}, {"trail", true}};
    auto explosionResult = LootSerializers::parseFunction(explosion);
    ASSERT_TRUE(explosionResult.success()) << explosionResult.error().message();
    auto explosionFunction = explosionResult.value();
    auto* explosionFunc = dynamic_cast<SetFireworkExplosionFunction*>(explosionFunction.get());
    ASSERT_NE(explosionFunc, nullptr);
    ASSERT_TRUE(explosionFunc->getShape().has_value());
    EXPECT_EQ(item::component::FireworkShape::Star, *explosionFunc->getShape());
}

TEST_F(LootSerializersTest, ParseFunction_BookAndBannerFunctions)
{
    nlohmann::json written = {{"function", "minecraft:set_written_book_pages"}, {"pages", {"page1", "page2"}}};
    auto writtenResult = LootSerializers::parseFunction(written);
    ASSERT_TRUE(writtenResult.success()) << writtenResult.error().message();
    auto writtenFunction = writtenResult.value();
    auto* writtenFunc = dynamic_cast<SetWrittenBookPagesFunction*>(writtenFunction.get());
    ASSERT_NE(writtenFunc, nullptr);
    EXPECT_EQ(writtenFunc->getPages().size(), 2u);

    nlohmann::json writable = {{"function", "minecraft:set_writable_book_pages"}, {"pages", {"a"}}};
    auto writableResult = LootSerializers::parseFunction(writable);
    ASSERT_TRUE(writableResult.success()) << writableResult.error().message();
    EXPECT_EQ("set_writable_book_pages", writableResult.value()->getType());

    nlohmann::json cover = {
        {"function", "minecraft:set_book_cover"}, {"title", "T"}, {"author", "A"}, {"generation", 2}};
    auto coverResult = LootSerializers::parseFunction(cover);
    ASSERT_TRUE(coverResult.success()) << coverResult.error().message();
    EXPECT_EQ("set_book_cover", coverResult.value()->getType());

    nlohmann::json banner = {{"function", "minecraft:set_banner_pattern"},
        {"patterns", {{{"pattern", "minecraft:stripe_bottom"}, {"color", "red"}}}},
        {"append", true}};
    auto bannerResult = LootSerializers::parseFunction(banner);
    ASSERT_TRUE(bannerResult.success()) << bannerResult.error().message();
    auto bannerFunction = bannerResult.value();
    auto* bannerFunc = dynamic_cast<SetBannerPatternFunction*>(bannerFunction.get());
    ASSERT_NE(bannerFunc, nullptr);
    EXPECT_TRUE(bannerFunc->isAppend());

    nlohmann::json badBanner = {{"function", "minecraft:set_banner_pattern"},
        {"patterns", {{{"pattern", "minecraft:stripe_bottom"}, {"color", "not_a_color"}}}}};
    EXPECT_FALSE(LootSerializers::parseFunction(badBanner).success());
}

TEST_F(LootSerializersTest, ParseFunction_ToggleTooltipsAndCustomModelData)
{
    nlohmann::json tooltips = {{"function", "minecraft:toggle_tooltips"},
        {"toggles", {{"minecraft:enchantments", false}, {"minecraft:custom_name", true}}}};
    auto tooltipsResult = LootSerializers::parseFunction(tooltips);
    ASSERT_TRUE(tooltipsResult.success()) << tooltipsResult.error().message();
    auto tooltipsFunction = tooltipsResult.value();
    auto* tooltipsFunc = dynamic_cast<ToggleTooltipsFunction*>(tooltipsFunction.get());
    ASSERT_NE(tooltipsFunc, nullptr);
    EXPECT_EQ(tooltipsFunc->getToggles().size(), 2u);

    nlohmann::json modelData = {{"function", "minecraft:set_custom_model_data"},
        {"floats", {{"mode", "replace_all"}, {"values", {1.0, 2.0}}}},
        {"strings", {{"mode", "append"}, {"values", {"a"}}}}};
    auto modelResult = LootSerializers::parseFunction(modelData);
    ASSERT_TRUE(modelResult.success()) << modelResult.error().message();
    EXPECT_EQ("set_custom_model_data", modelResult.value()->getType());
}

TEST_F(LootSerializersTest, ParseFunction_ModifyContents)
{
    nlohmann::json json = {{"function", "minecraft:modify_contents"},
        {"component", "minecraft:container"},
        {"modifier", {{"function", "minecraft:set_count"}, {"count", 1}}}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success()) << result.error().message();
    auto function = result.value();
    auto* func = dynamic_cast<ModifyContentsFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ("minecraft:container", func->getComponent());
    ASSERT_NE(func->getModifier(), nullptr);
    EXPECT_EQ("set_count", func->getModifier()->getType());

    nlohmann::json bad = {{"function", "minecraft:modify_contents"},
        {"component", "minecraft:not_a_container"},
        {"modifier", {{"function", "minecraft:set_count"}, {"count", 1}}}};
    EXPECT_FALSE(LootSerializers::parseFunction(bad).success());
}

TEST_F(LootSerializersTest, ParseFunction_Reference)
{
    // 未注册的命名函数 -> 加载失败（原版 MissingReferenceProblem）
    nlohmann::json missing = {{"function", "minecraft:reference"}, {"name", "minecraft:missing_function"}};
    EXPECT_FALSE(LootSerializers::parseFunction(missing).success());

    // 注册后引用可正常解析
    LootFunctionManager::instance().registerFunction(
        "minecraft:test_function", std::make_unique<SetCountFunction>(RandomValueRange(1.0f, 1.0f), false));
    nlohmann::json json = {{"function", "minecraft:reference"}, {"name", "minecraft:test_function"}};
    auto result = LootSerializers::parseFunction(json);
    ASSERT_TRUE(result.success()) << result.error().message();
    auto function = result.value();
    auto* func = dynamic_cast<FunctionReferenceFunction*>(function.get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ("minecraft:test_function", func->getName());
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
    entry->addCondition(std::make_unique<TableBonusCondition>("minecraft:fortune", std::vector<f32>{0.1f, 0.2f}));
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
    EXPECT_EQ("table_bonus", parsedEntry->getConditions()[0]->getType());
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

// ============================================================================
// entity_properties 目标名称（对齐 vanilla LootContext.EntityTarget）
// ============================================================================

TEST_F(LootSerializersTest, ParseCondition_EntityProperties_DirectAttackerTarget)
{
    // 数据包"熔炼掉落"用 direct_attacker 指代直接来源（投射物本身或近战攻击者）
    nlohmann::json json = {{"condition", "minecraft:entity_properties"},
        {"entity", "direct_attacker"},
        {"predicate", {{"equipment", {{"mainhand", "minecraft:iron_sword"}}}}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());
    auto conditionPtr = result.value(); // value() 是转移所有权语义，必须只取一次
    auto* condition = dynamic_cast<loot::EntityPropertiesCondition*>(conditionPtr.get());
    ASSERT_NE(condition, nullptr);
    EXPECT_EQ(condition->getTarget(), loot::EntityPropertiesCondition::EntityTarget::DirectAttacker);
}

TEST_F(LootSerializersTest, ParseCondition_EntityProperties_AttackerTarget)
{
    nlohmann::json json = {{"condition", "minecraft:entity_properties"},
        {"entity", "attacker"},
        {"predicate", {{"type", "minecraft:player"}}}};

    auto result = LootSerializers::parseCondition(json);
    ASSERT_TRUE(result.success());
    auto conditionPtr = result.value(); // value() 是转移所有权语义，必须只取一次
    auto* condition = dynamic_cast<loot::EntityPropertiesCondition*>(conditionPtr.get());
    ASSERT_NE(condition, nullptr);
    EXPECT_EQ(condition->getTarget(), loot::EntityPropertiesCondition::EntityTarget::Attacker);
}
