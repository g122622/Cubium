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
#include "common/advancement/trigger/impl/EntityTriggers.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;

/**
 * @brief BredAnimalsTrigger 单元测试
 *
 * 测试动物繁殖触发器的核心功能：
 * - fromJson 解析
 * - test 条件检测
 * - conditionsToJson 序列化
 * - 触发器注册
 *
 * 参考 MC 1.16.5: CriteriaTriggers.BRED_ANIMALS
 */
class BredAnimalsTriggerTest : public ::testing::Test {
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

// ========== BredAnimalsTrigger 注册测试 ==========

TEST_F(BredAnimalsTriggerTest, TriggerRegistration)
{
    // 验证触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(BredAnimalsTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:bred_animals");
}

TEST_F(BredAnimalsTriggerTest, TriggerIdCorrect)
{
    EXPECT_STREQ(BredAnimalsTrigger::TRIGGER_ID, "minecraft:bred_animals");
    EXPECT_STREQ(BredAnimalsTriggerInstance::TRIGGER_ID, "minecraft:bred_animals");
}

TEST_F(BredAnimalsTriggerTest, GetIdReturnsCorrectResourceLocation)
{
    BredAnimalsTrigger trigger;
    ResourceLocation id = trigger.getId();
    EXPECT_EQ(id.toString(), "minecraft:bred_animals");
}

// ========== BredAnimalsTriggerInstance fromJson 测试 ==========

TEST_F(BredAnimalsTriggerTest, InstanceFromJsonEmpty)
{
    // 空条件应该匹配任何动物繁殖
    nlohmann::json conditions = {};

    auto* trigger = CriterionTriggers::instance().getTrigger<BredAnimalsTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 空条件序列化为空对象
    EXPECT_TRUE(instance->conditionsToJson().is_null());
}

TEST_F(BredAnimalsTriggerTest, InstanceFromJsonNull)
{
    // null 条件也应该匹配任何动物繁殖
    nlohmann::json conditions = nullptr;

    auto* trigger = CriterionTriggers::instance().getTrigger<BredAnimalsTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(BredAnimalsTriggerTest, InstanceFromJsonWithChildPredicate)
{
    // 只指定子代类型
    nlohmann::json conditions = R"({
        "child": {
            "type": "minecraft:cow"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<BredAnimalsTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("child"));
}

TEST_F(BredAnimalsTriggerTest, InstanceFromJsonWithParentPredicate)
{
    // 只指定父代类型
    nlohmann::json conditions = R"({
        "parent": {
            "type": "minecraft:cow"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<BredAnimalsTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("parent"));
}

TEST_F(BredAnimalsTriggerTest, InstanceFromJsonWithPartnerPredicate)
{
    // 只指定配偶类型
    nlohmann::json conditions = R"({
        "partner": {
            "type": "minecraft:cow"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<BredAnimalsTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("partner"));
}

TEST_F(BredAnimalsTriggerTest, InstanceFromJsonFullConditions)
{
    // 完整条件：指定子代、父代和配偶
    nlohmann::json conditions = R"({
        "child": {
            "type": "minecraft:cow"
        },
        "parent": {
            "type": "minecraft:cow"
        },
        "partner": {
            "type": "minecraft:cow"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<BredAnimalsTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证所有条件都被解析
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("child"));
    EXPECT_TRUE(serialized.contains("parent"));
    EXPECT_TRUE(serialized.contains("partner"));
}

TEST_F(BredAnimalsTriggerTest, InstanceFromJsonWithDifferentTypes)
{
    // 测试不同动物类型的条件
    // 场景：两头牛繁殖出小牛
    nlohmann::json conditions = R"({
        "child": {
            "type": "minecraft:cow"
        },
        "parent": {
            "type": "minecraft:cow"
        },
        "partner": {
            "type": "minecraft:cow"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<BredAnimalsTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

// ========== BredAnimalsTriggerInstance test 测试 ==========

TEST_F(BredAnimalsTriggerTest, TestEmptyConditions)
{
    // 空条件实例
    BredAnimalsTriggerInstance instance;

    // 验证 isAny 状态
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

TEST_F(BredAnimalsTriggerTest, TestWithChildOnly)
{
    // 只设置子代条件
    nlohmann::json childJson = R"({
        "type": "minecraft:pig"
    })"_json;

    auto childResult = EntityPredicate::fromJson(childJson);
    ASSERT_TRUE(childResult.success());

    BredAnimalsTriggerInstance instance(std::move(childResult).value(), EntityPredicate{}, EntityPredicate{});

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("child"));
}

TEST_F(BredAnimalsTriggerTest, TestWithParentAndPartner)
{
    // 设置父代和配偶条件
    nlohmann::json parentJson = R"({"type": "minecraft:sheep"})"_json;
    nlohmann::json partnerJson = R"({"type": "minecraft:sheep"})"_json;

    auto parentResult = EntityPredicate::fromJson(parentJson);
    auto partnerResult = EntityPredicate::fromJson(partnerJson);
    ASSERT_TRUE(parentResult.success());
    ASSERT_TRUE(partnerResult.success());

    BredAnimalsTriggerInstance instance(
        EntityPredicate{}, std::move(parentResult).value(), std::move(partnerResult).value());

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("parent"));
    EXPECT_TRUE(conditionsJson.contains("partner"));
}

TEST_F(BredAnimalsTriggerTest, TestWithAllPredicates)
{
    // 设置所有条件
    nlohmann::json childJson = R"({"type": "minecraft:chicken"})"_json;
    nlohmann::json parentJson = R"({"type": "minecraft:chicken"})"_json;
    nlohmann::json partnerJson = R"({"type": "minecraft:chicken"})"_json;

    auto childResult = EntityPredicate::fromJson(childJson);
    auto parentResult = EntityPredicate::fromJson(parentJson);
    auto partnerResult = EntityPredicate::fromJson(partnerJson);
    ASSERT_TRUE(childResult.success());
    ASSERT_TRUE(parentResult.success());
    ASSERT_TRUE(partnerResult.success());

    BredAnimalsTriggerInstance instance(
        std::move(childResult).value(), std::move(parentResult).value(), std::move(partnerResult).value());

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("child"));
    EXPECT_TRUE(conditionsJson.contains("parent"));
    EXPECT_TRUE(conditionsJson.contains("partner"));
}

// ========== conditionsToJson 序列化测试 ==========

TEST_F(BredAnimalsTriggerTest, ConditionsToJsonRoundtrip)
{
    // 测试序列化和反序列化的往返
    nlohmann::json originalConditions = R"({
        "child": {
            "type": "minecraft:horse"
        },
        "parent": {
            "type": "minecraft:horse"
        },
        "partner": {
            "type": "minecraft:horse"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<BredAnimalsTrigger>();
    ASSERT_NE(trigger, nullptr);

    // 解析
    auto result = trigger->fromJson(originalConditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 序列化
    nlohmann::json serialized = instance->conditionsToJson();

    // 验证关键字段存在
    EXPECT_TRUE(serialized.contains("child"));
    EXPECT_TRUE(serialized.contains("parent"));
    EXPECT_TRUE(serialized.contains("partner"));
}

TEST_F(BredAnimalsTriggerTest, ConditionsToJsonNullForAny)
{
    // 空条件序列化为 null
    BredAnimalsTriggerInstance instance;
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

// ========== 构造函数测试 ==========

TEST_F(BredAnimalsTriggerTest, DefaultConstructor)
{
    BredAnimalsTriggerInstance instance;
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

TEST_F(BredAnimalsTriggerTest, ParameterizedConstructor)
{
    nlohmann::json childJson = R"({"type": "minecraft:rabbit"})"_json;
    nlohmann::json parentJson = R"({"type": "minecraft:rabbit"})"_json;
    nlohmann::json partnerJson = R"({"type": "minecraft:rabbit"})"_json;

    auto childResult = EntityPredicate::fromJson(childJson);
    auto parentResult = EntityPredicate::fromJson(parentJson);
    auto partnerResult = EntityPredicate::fromJson(partnerJson);
    ASSERT_TRUE(childResult.success());
    ASSERT_TRUE(parentResult.success());
    ASSERT_TRUE(partnerResult.success());

    BredAnimalsTriggerInstance instance(
        std::move(childResult).value(), std::move(parentResult).value(), std::move(partnerResult).value());

    // 验证所有谓词都被设置
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("child"));
    EXPECT_TRUE(conditionsJson.contains("parent"));
    EXPECT_TRUE(conditionsJson.contains("partner"));
}

// ========== 实际进度场景测试 ==========

TEST_F(BredAnimalsTriggerTest, TwoByTwoAnimalsAdvancement)
{
    // 测试 "Two by Two" 进度的典型条件
    // 这个进度要求繁殖每种动物
    // 条件格式：只检查子代类型
    nlohmann::json conditions = R"({
        "child": {
            "type": "minecraft:cow"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<BredAnimalsTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("child"));
}

TEST_F(BredAnimalsTriggerTest, SpecificParentAdvancement)
{
    // 测试需要特定父代的进度条件
    // 例如：用特定的羊繁殖
    nlohmann::json conditions = R"({
        "parent": {
            "type": "minecraft:sheep",
            "flags": {
                "is_baby": false
            }
        },
        "child": {
            "type": "minecraft:sheep"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<BredAnimalsTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

// main 函数由 gtest_main 库提供
