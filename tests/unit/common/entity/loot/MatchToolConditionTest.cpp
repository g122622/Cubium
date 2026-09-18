/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/conditions/EnchantmentPredicate.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootPool.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/conditions/LootConditions.hpp"
#include "common/item/loot/conditions/MatchToolCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::loot;

// Test implementation of IWorld for loot testing
class MatchToolConditionTestWorld : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("MatchToolConditionTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("MatchToolConditionTestWorld::tickManager not implemented");
    }
};

class MatchToolConditionTest : public ::testing::Test {
protected:
    MatchToolConditionTestWorld m_world;

    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

// ============================================================================
// 基本功能测试
// ============================================================================

TEST_F(MatchToolConditionTest, NoPredicate_NoTool_ReturnsFalse)
{
    // 无谓词时，只要存在非空工具即满足条件
    // 没有工具时应返回 false
    MatchToolCondition condition;
    math::Random random(12345);
    auto context = LootContextBuilder(m_world).withRandom(random).build();

    EXPECT_FALSE(condition.test(*context));
}

TEST_F(MatchToolConditionTest, NoPredicate_WithTool_ReturnsTrue)
{
    // 无谓词时，只要存在非空工具即满足条件
    MatchToolCondition condition;
    math::Random random(12345);

    // 创建一个物品堆作为工具
    ItemStack tool(Items::DIAMOND_PICKAXE, 1);

    auto context =
        LootContextBuilder(m_world).withRandom(random).withParameter<ItemStack>(LootParams::TOOL, &tool).build();

    EXPECT_TRUE(condition.test(*context));
}

TEST_F(MatchToolConditionTest, NoPredicate_WithEmptyTool_ReturnsFalse)
{
    // 空物品堆应视为无工具
    MatchToolCondition condition;
    math::Random random(12345);

    ItemStack emptyTool;

    auto context =
        LootContextBuilder(m_world).withRandom(random).withParameter<ItemStack>(LootParams::TOOL, &emptyTool).build();

    EXPECT_FALSE(condition.test(*context));
}

TEST_F(MatchToolConditionTest, GetType_ReturnsMatchTool)
{
    MatchToolCondition condition;
    EXPECT_EQ(condition.getType(), "match_tool");
}

TEST_F(MatchToolConditionTest, Clone_NoPredicate)
{
    MatchToolCondition original;
    auto cloned = original.clone();

    EXPECT_EQ(cloned->getType(), "match_tool");
    math::Random random(12345);
    auto context = LootContextBuilder(m_world).withRandom(random).build();
    EXPECT_FALSE(cloned->test(*context));
}

TEST_F(MatchToolConditionTest, Clone_WithPredicate)
{
    advancement::ItemPredicate predicate;
    predicate = advancement::ItemPredicate(ResourceLocation("minecraft:diamond_pickaxe"),
        advancement::IntBounds(),
        advancement::IntBounds(),
        std::nullopt,
        {},
        {},
        advancement::NBTPredicate());

    MatchToolCondition original(std::move(predicate));
    auto cloned = original.clone();

    EXPECT_EQ(cloned->getType(), "match_tool");
}

// ============================================================================
// ItemPredicate 匹配测试
// ============================================================================

TEST_F(MatchToolConditionTest, WithItemPredicate_MatchingItem_ReturnsTrue)
{
    // 创建匹配钻石镐的谓词
    advancement::ItemPredicate predicate(ResourceLocation("minecraft:diamond_pickaxe"),
        advancement::IntBounds(),
        advancement::IntBounds(),
        std::nullopt,
        {},
        {},
        advancement::NBTPredicate());

    MatchToolCondition condition(std::move(predicate));
    math::Random random(12345);

    ItemStack tool(Items::DIAMOND_PICKAXE, 1);
    auto context =
        LootContextBuilder(m_world).withRandom(random).withParameter<ItemStack>(LootParams::TOOL, &tool).build();

    EXPECT_TRUE(condition.test(*context));
}

TEST_F(MatchToolConditionTest, WithItemPredicate_NonMatchingItem_ReturnsFalse)
{
    // 创建匹配钻石镐的谓词
    advancement::ItemPredicate predicate(ResourceLocation("minecraft:diamond_pickaxe"),
        advancement::IntBounds(),
        advancement::IntBounds(),
        std::nullopt,
        {},
        {},
        advancement::NBTPredicate());

    MatchToolCondition condition(std::move(predicate));
    math::Random random(12345);

    // 使用铁镐（不匹配）
    ItemStack tool(Items::IRON_PICKAXE, 1);
    auto context =
        LootContextBuilder(m_world).withRandom(random).withParameter<ItemStack>(LootParams::TOOL, &tool).build();

    EXPECT_FALSE(condition.test(*context));
}

TEST_F(MatchToolConditionTest, WithAnyPredicate_WithTool_ReturnsTrue)
{
    // 创建匹配任意物品的谓词（默认构造）
    advancement::ItemPredicate anyPredicate;

    MatchToolCondition condition(std::move(anyPredicate));
    math::Random random(12345);

    ItemStack tool(Items::IRON_PICKAXE, 1);
    auto context =
        LootContextBuilder(m_world).withRandom(random).withParameter<ItemStack>(LootParams::TOOL, &tool).build();

    // ItemPredicate::isAny() 为 true 时，任何非空物品都匹配
    EXPECT_TRUE(condition.test(*context));
}

TEST_F(MatchToolConditionTest, WithItemPredicate_NoTool_ReturnsFalse)
{
    // 创建匹配钻石镐的谓词，但没有工具
    advancement::ItemPredicate predicate(ResourceLocation("minecraft:diamond_pickaxe"),
        advancement::IntBounds(),
        advancement::IntBounds(),
        std::nullopt,
        {},
        {},
        advancement::NBTPredicate());

    MatchToolCondition condition(std::move(predicate));
    math::Random random(12345);

    auto context = LootContextBuilder(m_world).withRandom(random).build();
    EXPECT_FALSE(condition.test(*context));
}

TEST_F(MatchToolConditionTest, WithEnchantmentPredicate_MatchingEnchantment_ReturnsTrue)
{
    // 创建带有精准采集附魔谓词的 ItemPredicate
    advancement::EnchantmentPredicate silkTouch(
        ResourceLocation("minecraft:silk_touch"), advancement::IntBounds::atLeast(1));

    std::vector<advancement::EnchantmentPredicate> enchantments;
    enchantments.push_back(std::move(silkTouch));

    advancement::ItemPredicate predicate(std::nullopt,
        std::nullopt,
        advancement::IntBounds(),
        advancement::IntBounds(),
        std::nullopt,
        std::move(enchantments),
        {},
        advancement::NBTPredicate());

    MatchToolCondition condition(std::move(predicate));
    math::Random random(12345);

    // 创建带有精准采集的工具
    ItemStack tool(Items::DIAMOND_PICKAXE, 1);
    tool.addEnchantment("minecraft:silk_touch", 1);

    auto context =
        LootContextBuilder(m_world).withRandom(random).withParameter<ItemStack>(LootParams::TOOL, &tool).build();

    EXPECT_TRUE(condition.test(*context));
}

TEST_F(MatchToolConditionTest, WithEnchantmentPredicate_NoMatchingEnchantment_ReturnsFalse)
{
    // 创建带有精准采集附魔谓词的 ItemPredicate
    advancement::EnchantmentPredicate silkTouch(
        ResourceLocation("minecraft:silk_touch"), advancement::IntBounds::atLeast(1));

    std::vector<advancement::EnchantmentPredicate> enchantments;
    enchantments.push_back(std::move(silkTouch));

    advancement::ItemPredicate predicate(std::nullopt,
        std::nullopt,
        advancement::IntBounds(),
        advancement::IntBounds(),
        std::nullopt,
        std::move(enchantments),
        {},
        advancement::NBTPredicate());

    MatchToolCondition condition(std::move(predicate));
    math::Random random(12345);

    // 创建不带精准采集的工具
    ItemStack tool(Items::DIAMOND_PICKAXE, 1);
    tool.addEnchantment("minecraft:fortune", 3);

    auto context =
        LootContextBuilder(m_world).withRandom(random).withParameter<ItemStack>(LootParams::TOOL, &tool).build();

    EXPECT_FALSE(condition.test(*context));
}

TEST_F(MatchToolConditionTest, GetPredicate_NoPredicate)
{
    MatchToolCondition condition;
    EXPECT_FALSE(condition.getPredicate().has_value());
}

TEST_F(MatchToolConditionTest, GetPredicate_WithPredicate)
{
    advancement::ItemPredicate predicate(ResourceLocation("minecraft:diamond_pickaxe"),
        advancement::IntBounds(),
        advancement::IntBounds(),
        std::nullopt,
        {},
        {},
        advancement::NBTPredicate());

    MatchToolCondition condition(std::move(predicate));
    EXPECT_TRUE(condition.getPredicate().has_value());
}

// ============================================================================
// LootConditionBuilder 集成测试
// ============================================================================

TEST_F(MatchToolConditionTest, BuilderMatchTool_NoPredicate)
{
    auto condition = LootConditionBuilder::matchTool();
    ASSERT_NE(condition, nullptr);
    EXPECT_EQ(condition->getType(), "match_tool");
}

TEST_F(MatchToolConditionTest, BuilderMatchTool_WithPredicate)
{
    advancement::ItemPredicate predicate(ResourceLocation("minecraft:diamond_pickaxe"),
        advancement::IntBounds(),
        advancement::IntBounds(),
        std::nullopt,
        {},
        {},
        advancement::NBTPredicate());

    auto condition = LootConditionBuilder::matchTool(std::move(predicate));
    ASSERT_NE(condition, nullptr);
    EXPECT_EQ(condition->getType(), "match_tool");
}
