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

#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/advancement/trigger/impl/EntityTriggers.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;

/**
 * @brief PlayerInteractedWithEntityTrigger 单元测试
 *
 * 测试玩家与实体交互触发器的核心功能：
 * - fromJson 解析
 * - test 条件检测
 * - conditionsToJson 序列化
 */
class PlayerInteractedWithEntityTriggerTest : public ::testing::Test {
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

// ========== PlayerInteractedWithEntityTrigger 注册测试 ==========

TEST_F(PlayerInteractedWithEntityTriggerTest, TriggerRegistration)
{
    // 验证触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(
        ResourceLocation(PlayerInteractedWithEntityTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:player_interacted_with_entity");
}

// ========== PlayerInteractedWithEntityTriggerInstance fromJson 测试 ==========

TEST_F(PlayerInteractedWithEntityTriggerTest, InstanceFromJsonEmpty)
{
    // 空条件应该匹配任何交互
    nlohmann::json conditions = {};

    auto* trigger = CriterionTriggers::instance().getTrigger<PlayerInteractedWithEntityTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlayerInteractedWithEntityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlayerInteractedWithEntityTriggerTest, InstanceFromJsonWithItemPredicate)
{
    // 带物品谓词的条件
    nlohmann::json conditions = R"({
        "item": {
            "item": "minecraft:diamond"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlayerInteractedWithEntityTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlayerInteractedWithEntityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlayerInteractedWithEntityTriggerTest, InstanceFromJsonWithEntityPredicate)
{
    // 带实体谓词的条件
    nlohmann::json conditions = R"({
        "entity": {
            "type": "minecraft:piglin"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlayerInteractedWithEntityTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlayerInteractedWithEntityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlayerInteractedWithEntityTriggerTest, InstanceFromJsonFullConditions)
{
    // 完整条件（物品 + 实体）
    nlohmann::json conditions = R"({
        "item": {
            "item": "minecraft:gold_ingot"
        },
        "entity": {
            "type": "minecraft:piglin"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlayerInteractedWithEntityTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlayerInteractedWithEntityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

// ========== PlayerInteractedWithEntityTriggerInstance test 测试 ==========

TEST_F(PlayerInteractedWithEntityTriggerTest, TestEmptyConditions)
{
    // 空条件应该匹配任何物品和实体
    PlayerInteractedWithEntityTriggerInstance instance;

    // 验证空条件的序列化结果
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

TEST_F(PlayerInteractedWithEntityTriggerTest, TestWithItemPredicate)
{
    // 带物品类型的条件
    nlohmann::json itemJson = R"({
        "item": "minecraft:diamond"
    })"_json;

    auto itemResult = ItemPredicate::fromJson(itemJson);
    ASSERT_TRUE(itemResult.success());

    PlayerInteractedWithEntityTriggerInstance instance(std::move(itemResult).value(), EntityPredicate{});

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("item"));
}

TEST_F(PlayerInteractedWithEntityTriggerTest, TestWithEntityPredicate)
{
    // 带实体类型的条件
    nlohmann::json entityJson = R"({
        "type": "minecraft:zombie"
    })"_json;

    auto entityResult = EntityPredicate::fromJson(entityJson);
    ASSERT_TRUE(entityResult.success());

    PlayerInteractedWithEntityTriggerInstance instance(ItemPredicate{}, std::move(entityResult).value());

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("entity"));
}

TEST_F(PlayerInteractedWithEntityTriggerTest, TestWithBothPredicates)
{
    // 同时带物品和实体的条件
    nlohmann::json itemJson = R"({
        "item": "minecraft:gold_ingot"
    })"_json;

    nlohmann::json entityJson = R"({
        "type": "minecraft:villager"
    })"_json;

    auto itemResult = ItemPredicate::fromJson(itemJson);
    ASSERT_TRUE(itemResult.success());

    auto entityResult = EntityPredicate::fromJson(entityJson);
    ASSERT_TRUE(entityResult.success());

    PlayerInteractedWithEntityTriggerInstance instance(std::move(itemResult).value(), std::move(entityResult).value());

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("item"));
    EXPECT_TRUE(conditionsJson.contains("entity"));
}

// ========== conditionsToJson 序列化测试 ==========

TEST_F(PlayerInteractedWithEntityTriggerTest, ConditionsToJsonRoundtrip)
{
    // 测试序列化和反序列化的往返
    nlohmann::json originalConditions = R"({
        "item": {
            "item": "minecraft:emerald"
        },
        "entity": {
            "type": "minecraft:villager"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlayerInteractedWithEntityTrigger>();
    ASSERT_NE(trigger, nullptr);

    // 解析
    auto result = trigger->fromJson(originalConditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlayerInteractedWithEntityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 序列化
    nlohmann::json serialized = instance->conditionsToJson();

    // 验证关键字段存在
    EXPECT_TRUE(serialized.contains("item"));
    EXPECT_TRUE(serialized.contains("entity"));
}

// ========== TRIGGER_ID 常量测试 ==========

TEST_F(PlayerInteractedWithEntityTriggerTest, TriggerIdConstant)
{
    // 验证 TRIGGER_ID 常量
    EXPECT_STREQ(PlayerInteractedWithEntityTrigger::TRIGGER_ID, "minecraft:player_interacted_with_entity");
    EXPECT_STREQ(PlayerInteractedWithEntityTriggerInstance::TRIGGER_ID, "minecraft:player_interacted_with_entity");
}

// main 函数由 gtest_main 库提供
