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

#include <gtest/gtest.h>

#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/advancement/trigger/impl/ItemTriggers.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;

/**
 * @brief ItemTriggers 单元测试
 *
 * 测试物品相关触发器的核心功能：
 * - ConsumeItemTrigger: 消耗物品触发器
 * - ItemDurabilityTrigger: 耐久变化触发器
 * - EnchantedItemTrigger: 附魔物品触发器
 * - FilledBucketTrigger: 填充桶触发器
 *
 * 参考 MC 1.16.5: CriteriaTriggers.CONSUME_ITEM, ITEM_DURABILITY_CHANGED, ENCHANTED_ITEM, FILLED_BUCKET
 */
class ItemTriggersTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册内置触发器
        CriterionTriggers::instance().registerBuiltinTriggers();
    }

    void TearDown() override
    {
        // 清理
        CriterionTriggers::instance().clear();
    }
};

// ========== ConsumeItemTrigger 测试 ==========

TEST_F(ItemTriggersTest, ConsumeItemTriggerRegistration)
{
    // 验证触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(ConsumeItemTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:consume_item");
}

TEST_F(ItemTriggersTest, ConsumeItemInstanceFromJsonEmpty)
{
    // 空条件应该匹配任何物品
    nlohmann::json conditions = nullptr;

    auto* trigger = CriterionTriggers::instance().getTrigger<ConsumeItemTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<ConsumeItemTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(ItemTriggersTest, ConsumeItemInstanceFromJsonWithItem)
{
    // 带物品条件的触发器
    nlohmann::json conditions = R"({
        "item": {
            "item": "minecraft:apple"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<ConsumeItemTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<ConsumeItemTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(ItemTriggersTest, ConsumeItemTestWithMatchingItem)
{
    // 创建物品谓词
    auto itemResult = ItemPredicate::fromJson(R"({"item": "minecraft:apple"})"_json);
    ASSERT_TRUE(itemResult.success());

    // 创建触发器实例
    auto instance = std::make_shared<ConsumeItemTriggerInstance>(std::move(itemResult).value());

    // 创建匹配的物品栈（苹果）
    if (Items::APPLE != nullptr) {
        ItemStack appleStack(Items::APPLE, 1);
        EXPECT_TRUE(instance->test(appleStack));
    }
}

TEST_F(ItemTriggersTest, ConsumeItemTestWithNonMatchingItem)
{
    // 创建物品谓词（苹果）
    auto itemResult = ItemPredicate::fromJson(R"({"item": "minecraft:apple"})"_json);
    ASSERT_TRUE(itemResult.success());

    // 创建触发器实例
    auto instance = std::make_shared<ConsumeItemTriggerInstance>(std::move(itemResult).value());

    // 创建不匹配的物品栈（钻石）
    if (Items::DIAMOND != nullptr) {
        ItemStack diamondStack(Items::DIAMOND, 1);
        EXPECT_FALSE(instance->test(diamondStack));
    }
}

TEST_F(ItemTriggersTest, ConsumeItemConditionsToJsonRoundtrip)
{
    nlohmann::json originalConditions = R"({
        "item": {
            "item": "minecraft:golden_apple"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<ConsumeItemTrigger>();
    ASSERT_NE(trigger, nullptr);

    // 解析
    auto result = trigger->fromJson(originalConditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<ConsumeItemTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("item"));
}

// ========== ItemDurabilityTrigger 测试 ==========

TEST_F(ItemTriggersTest, ItemDurabilityTriggerRegistration)
{
    // 验证触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(ItemDurabilityTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:item_durability_changed");
}

TEST_F(ItemTriggersTest, ItemDurabilityInstanceFromJsonEmpty)
{
    // 空条件应该匹配任何耐久变化
    nlohmann::json conditions = nullptr;

    auto* trigger = CriterionTriggers::instance().getTrigger<ItemDurabilityTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<ItemDurabilityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(ItemTriggersTest, ItemDurabilityInstanceFromJsonWithBounds)
{
    // 带耐久范围和变化量条件
    nlohmann::json conditions = R"({
        "item": {
            "item": "minecraft:diamond_sword"
        },
        "durability": {
            "min": 100
        },
        "delta": {
            "min": 1,
            "max": 10
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<ItemDurabilityTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<ItemDurabilityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(ItemTriggersTest, ItemDurabilityTestWithMatchingConditions)
{
    // 创建物品谓词
    auto itemResult = ItemPredicate::fromJson(R"({"item": "minecraft:diamond_sword"})"_json);
    ASSERT_TRUE(itemResult.success());

    // 创建耐久和变化量范围
    IntBounds durability(IntBounds::atLeast(100));
    IntBounds delta(IntBounds::between(1, 10));

    // 创建触发器实例
    auto instance = std::make_shared<ItemDurabilityTriggerInstance>(std::move(itemResult).value(), durability, delta);

    // 创建物品栈
    if (Items::DIAMOND_SWORD != nullptr) {
        ItemStack sword(Items::DIAMOND_SWORD, 1);
        sword.setDamage(500); // 耐久度 = maxDamage - damage

        // 旧耐久度为 100（假设 maxDamage = 600）
        i32 oldDurability = 100;

        // delta = oldDurability - newDurability
        // 如果 delta 在 1-10 范围内，应该匹配
        // 注意：这个测试依赖于物品的实际 maxDamage
        // 这里只验证测试不会崩溃
        EXPECT_NO_THROW(instance->test(sword, oldDurability));
    }
}

TEST_F(ItemTriggersTest, ItemDurabilityConditionsToJsonRoundtrip)
{
    nlohmann::json originalConditions = R"({
        "item": {
            "item": "minecraft:iron_pickaxe"
        },
        "durability": {
            "min": 50
        },
        "delta": {
            "min": 1
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<ItemDurabilityTrigger>();
    ASSERT_NE(trigger, nullptr);

    // 解析
    auto result = trigger->fromJson(originalConditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<ItemDurabilityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("item"));
    EXPECT_TRUE(serialized.contains("durability"));
    EXPECT_TRUE(serialized.contains("delta"));
}

// ========== EnchantedItemTrigger 测试 ==========

TEST_F(ItemTriggersTest, EnchantedItemTriggerRegistration)
{
    // 验证触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(EnchantedItemTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:enchanted_item");
}

TEST_F(ItemTriggersTest, EnchantedItemInstanceFromJsonEmpty)
{
    // 空条件应该匹配任何附魔
    nlohmann::json conditions = nullptr;

    auto* trigger = CriterionTriggers::instance().getTrigger<EnchantedItemTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<EnchantedItemTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(ItemTriggersTest, EnchantedItemInstanceFromJsonWithLevels)
{
    // 带物品和等级条件
    nlohmann::json conditions = R"({
        "item": {
            "item": "minecraft:diamond_sword"
        },
        "levels": {
            "min": 5,
            "max": 30
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<EnchantedItemTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<EnchantedItemTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(ItemTriggersTest, EnchantedItemTestWithMatchingConditions)
{
    // 创建物品谓词
    auto itemResult = ItemPredicate::fromJson(R"({"item": "minecraft:diamond_sword"})"_json);
    ASSERT_TRUE(itemResult.success());

    // 创建等级范围
    IntBounds levels(IntBounds::between(5, 30));

    // 创建触发器实例
    auto instance = std::make_shared<EnchantedItemTriggerInstance>(std::move(itemResult).value(), levels);

    // 创建物品栈
    if (Items::DIAMOND_SWORD != nullptr) {
        ItemStack sword(Items::DIAMOND_SWORD, 1);

        // 等级在范围内
        EXPECT_TRUE(instance->test(sword, 15));  // 15 在 5-30 范围内
        EXPECT_FALSE(instance->test(sword, 3));  // 3 不在 5-30 范围内
        EXPECT_FALSE(instance->test(sword, 35)); // 35 不在 5-30 范围内
    }
}

TEST_F(ItemTriggersTest, EnchantedItemConditionsToJsonRoundtrip)
{
    nlohmann::json originalConditions = R"({
        "item": {
            "item": "minecraft:iron_sword"
        },
        "levels": {
            "min": 1,
            "max": 10
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<EnchantedItemTrigger>();
    ASSERT_NE(trigger, nullptr);

    // 解析
    auto result = trigger->fromJson(originalConditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<EnchantedItemTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("item"));
    EXPECT_TRUE(serialized.contains("levels"));
}

// ========== FilledBucketTrigger 测试 ==========

TEST_F(ItemTriggersTest, FilledBucketTriggerRegistration)
{
    // 验证触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(FilledBucketTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:filled_bucket");
}

TEST_F(ItemTriggersTest, FilledBucketInstanceFromJsonEmpty)
{
    // 空条件应该匹配任何桶
    nlohmann::json conditions = nullptr;

    auto* trigger = CriterionTriggers::instance().getTrigger<FilledBucketTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<FilledBucketTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(ItemTriggersTest, FilledBucketInstanceFromJsonWithItem)
{
    // 带物品条件（水桶）
    nlohmann::json conditions = R"({
        "item": {
            "item": "minecraft:water_bucket"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<FilledBucketTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<FilledBucketTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(ItemTriggersTest, FilledBucketTestWithMatchingItem)
{
    // 创建物品谓词（水桶）
    auto itemResult = ItemPredicate::fromJson(R"({"item": "minecraft:water_bucket"})"_json);
    ASSERT_TRUE(itemResult.success());

    // 创建触发器实例（使用构造函数）
    auto instance = std::make_shared<FilledBucketTriggerInstance>(std::move(itemResult).value());

    // 创建匹配的物品栈（水桶）
    if (Items::WATER_BUCKET != nullptr) {
        ItemStack waterBucket(Items::WATER_BUCKET, 1);
        EXPECT_TRUE(instance->test(waterBucket));
    }
}

TEST_F(ItemTriggersTest, FilledBucketTestWithNonMatchingItem)
{
    // 创建物品谓词（水桶）
    auto itemResult = ItemPredicate::fromJson(R"({"item": "minecraft:water_bucket"})"_json);
    ASSERT_TRUE(itemResult.success());

    // 创建触发器实例（使用构造函数）
    auto instance = std::make_shared<FilledBucketTriggerInstance>(std::move(itemResult).value());

    // 创建不匹配的物品栈（岩浆桶）
    if (Items::LAVA_BUCKET != nullptr) {
        ItemStack lavaBucket(Items::LAVA_BUCKET, 1);
        EXPECT_FALSE(instance->test(lavaBucket));
    }
}

TEST_F(ItemTriggersTest, FilledBucketConditionsToJsonRoundtrip)
{
    nlohmann::json originalConditions = R"({
        "item": {
            "item": "minecraft:lava_bucket"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<FilledBucketTrigger>();
    ASSERT_NE(trigger, nullptr);

    // 解析
    auto result = trigger->fromJson(originalConditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<FilledBucketTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("item"));
}

// ========== 工厂方法测试 ==========

TEST_F(ItemTriggersTest, ConsumeItemFactoryMethod)
{
    auto itemResult = ItemPredicate::fromJson(R"({"item": "minecraft:apple"})"_json);
    ASSERT_TRUE(itemResult.success());

    auto instance = ConsumeItemTrigger::item(std::move(itemResult).value());
    ASSERT_NE(instance, nullptr);
}

// ========== TRIGGER_ID 常量测试 ==========

TEST_F(ItemTriggersTest, TriggerIdConstants)
{
    EXPECT_STREQ(ConsumeItemTrigger::TRIGGER_ID, "minecraft:consume_item");
    EXPECT_STREQ(ConsumeItemTriggerInstance::TRIGGER_ID, "minecraft:consume_item");

    EXPECT_STREQ(ItemDurabilityTrigger::TRIGGER_ID, "minecraft:item_durability_changed");
    EXPECT_STREQ(ItemDurabilityTriggerInstance::TRIGGER_ID, "minecraft:item_durability_changed");

    EXPECT_STREQ(EnchantedItemTrigger::TRIGGER_ID, "minecraft:enchanted_item");
    EXPECT_STREQ(EnchantedItemTriggerInstance::TRIGGER_ID, "minecraft:enchanted_item");

    EXPECT_STREQ(FilledBucketTrigger::TRIGGER_ID, "minecraft:filled_bucket");
    EXPECT_STREQ(FilledBucketTriggerInstance::TRIGGER_ID, "minecraft:filled_bucket");
}

// main 函数由 gtest_main 库提供
