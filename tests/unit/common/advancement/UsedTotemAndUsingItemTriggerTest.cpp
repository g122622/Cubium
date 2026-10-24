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
 * @file UsedTotemAndUsingItemTriggerTest.cpp
 * @brief used_totem / using_item 触发器（vanilla UsedTotemTrigger / UsingItemTrigger）单元测试
 *
 * 数据包 story/use_totem 与 nether/obtain_ancient_debris 等进度依赖这两个触发器，
 * 此前二者均未实现也未注册，相关进度恒不可达。
 */

#include "common/advancement/Criterion.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/EffectTriggers.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

namespace mc {
namespace {

class UsedTotemAndUsingItemTriggerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::ensureInitialized();
        advancement::CriterionTriggers::instance().registerBuiltinTriggers();
    }

    void TearDown() override
    {
        advancement::CriterionTriggers::instance().clear();
    }
};

TEST_F(UsedTotemAndUsingItemTriggerTest, BothTriggersRegistered)
{
    EXPECT_TRUE(advancement::CriterionTriggers::instance().hasTrigger(ResourceLocation("minecraft:used_totem")));
    EXPECT_TRUE(advancement::CriterionTriggers::instance().hasTrigger(ResourceLocation("minecraft:using_item")));
}

TEST_F(UsedTotemAndUsingItemTriggerTest, UsedTotem_MatchesConfiguredItem)
{
    ASSERT_NE(Items::TOTEM_OF_UNDYING, nullptr);
    auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::UsedTotemTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(nlohmann::json{{"item", {{"items", "minecraft:totem_of_undying"}}}});
    ASSERT_TRUE(result.success());
    auto instance = std::dynamic_pointer_cast<advancement::UsedTotemTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    EXPECT_TRUE(instance->test(ItemStack(*Items::TOTEM_OF_UNDYING, 1)));
    // 其他物品不匹配
    ASSERT_NE(Items::DIAMOND, nullptr);
    EXPECT_FALSE(instance->test(ItemStack(*Items::DIAMOND, 1)));
}

TEST_F(UsedTotemAndUsingItemTriggerTest, UsedTotem_WithoutItemMatchesAny)
{
    // data 包 story/use_totem 的写法：conditions 为 {"item": {"items": "minecraft:totem_of_undying"}}；
    // 缺省 item 时按 vanilla matches() 语义匹配任意物品
    auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::UsedTotemTrigger>();
    ASSERT_NE(trigger, nullptr);
    auto result = trigger->fromJson(nlohmann::json::object());
    ASSERT_TRUE(result.success());
    auto instance = std::dynamic_pointer_cast<advancement::UsedTotemTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
    ASSERT_NE(Items::DIAMOND, nullptr);
    EXPECT_TRUE(instance->test(ItemStack(*Items::DIAMOND, 1)));
}

TEST_F(UsedTotemAndUsingItemTriggerTest, UsingItem_MatchesBowPredicate)
{
    auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::UsingItemTrigger>();
    ASSERT_NE(trigger, nullptr);

    // 对齐数据包：{"items": ["minecraft:bow", "minecraft:crossbow"]}
    const nlohmann::json json = {
        {"trigger", "minecraft:using_item"},
        {"conditions", {{"item", {{"items", nlohmann::json::array({"minecraft:bow", "minecraft:crossbow"})}}}}},
    };
    auto criterion = advancement::Criterion::fromJson("using_bow", json);
    ASSERT_TRUE(criterion.success());
    auto instance =
        std::dynamic_pointer_cast<advancement::UsingItemTriggerInstance>(criterion.value().getTriggerInstance());
    ASSERT_NE(instance, nullptr);

    ASSERT_NE(Items::BOW, nullptr);
    EXPECT_TRUE(instance->test(ItemStack(*Items::BOW, 1)));
    ASSERT_NE(Items::DIAMOND, nullptr);
    EXPECT_FALSE(instance->test(ItemStack(*Items::DIAMOND, 1)));
}

TEST_F(UsedTotemAndUsingItemTriggerTest, UsingItem_WithoutItemMatchesAny)
{
    auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::UsingItemTrigger>();
    ASSERT_NE(trigger, nullptr);
    auto result = trigger->fromJson(nlohmann::json::object());
    ASSERT_TRUE(result.success());
    auto instance = std::dynamic_pointer_cast<advancement::UsingItemTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
    ASSERT_NE(Items::SHIELD, nullptr);
    EXPECT_TRUE(instance->test(ItemStack(*Items::SHIELD, 1)));
}

TEST_F(UsedTotemAndUsingItemTriggerTest, ConditionsToJsonRoundTrip)
{
    auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::UsingItemTrigger>();
    ASSERT_NE(trigger, nullptr);
    auto result = trigger->fromJson(nlohmann::json{{"item", {{"items", "minecraft:bow"}}}});
    ASSERT_TRUE(result.success());
    auto instance = std::dynamic_pointer_cast<advancement::UsingItemTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
    const nlohmann::json roundTrip = instance->conditionsToJson();
    ASSERT_TRUE(roundTrip.contains("item"));
}

} // namespace
} // namespace mc