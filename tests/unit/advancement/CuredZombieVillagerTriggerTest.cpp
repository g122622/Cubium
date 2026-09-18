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
 * @brief CuredZombieVillagerTrigger 单元测试
 *
 * 测试治愈僵尸村民触发器的核心功能：
 * - fromJson 解析
 * - test 条件检测
 * - conditionsToJson 序列化
 * - 触发器注册
 */
class CuredZombieVillagerTriggerTest : public ::testing::Test {
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

// ========== CuredZombieVillagerTrigger 注册测试 ==========

TEST_F(CuredZombieVillagerTriggerTest, TriggerRegistration)
{
    // 验证触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(CuredZombieVillagerTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:cured_zombie_villager");
}

// ========== CuredZombieVillagerTriggerInstance fromJson 测试 ==========

TEST_F(CuredZombieVillagerTriggerTest, InstanceFromJsonEmpty)
{
    // 空条件应该匹配任何僵尸村民治愈
    nlohmann::json conditions = {};

    auto* trigger = CriterionTriggers::instance().getTrigger<CuredZombieVillagerTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<CuredZombieVillagerTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(CuredZombieVillagerTriggerTest, InstanceFromJsonWithZombiePredicate)
{
    // 带僵尸谓词的条件
    nlohmann::json conditions = R"({
        "zombie": {
            "type": "minecraft:zombie_villager"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<CuredZombieVillagerTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<CuredZombieVillagerTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("zombie"));
}

TEST_F(CuredZombieVillagerTriggerTest, InstanceFromJsonWithVillagerPredicate)
{
    // 带村民谓词的条件
    nlohmann::json conditions = R"({
        "villager": {
            "type": "minecraft:villager"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<CuredZombieVillagerTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<CuredZombieVillagerTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("villager"));
}

TEST_F(CuredZombieVillagerTriggerTest, InstanceFromJsonFullConditions)
{
    // 完整条件
    nlohmann::json conditions = R"({
        "zombie": {
            "type": "minecraft:zombie_villager"
        },
        "villager": {
            "type": "minecraft:villager"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<CuredZombieVillagerTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<CuredZombieVillagerTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证所有条件都被解析
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("zombie"));
    EXPECT_TRUE(serialized.contains("villager"));
}

// ========== CuredZombieVillagerTriggerInstance test 测试 ==========

TEST_F(CuredZombieVillagerTriggerTest, TestEmptyConditions)
{
    // 空条件应该匹配任何僵尸村民治愈
    CuredZombieVillagerTriggerInstance instance;

    // 验证 isAny 状态
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

TEST_F(CuredZombieVillagerTriggerTest, TestWithZombiePredicate)
{
    // 带僵尸类型的条件
    nlohmann::json zombieJson = R"({
        "type": "minecraft:zombie_villager"
    })"_json;

    auto zombieResult = EntityPredicate::fromJson(zombieJson);
    ASSERT_TRUE(zombieResult.success());

    CuredZombieVillagerTriggerInstance instance(std::move(zombieResult).value(), EntityPredicate{});

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("zombie"));
}

TEST_F(CuredZombieVillagerTriggerTest, TestWithVillagerPredicate)
{
    // 带村民类型的条件
    nlohmann::json villagerJson = R"({
        "type": "minecraft:villager"
    })"_json;

    auto villagerResult = EntityPredicate::fromJson(villagerJson);
    ASSERT_TRUE(villagerResult.success());

    CuredZombieVillagerTriggerInstance instance(EntityPredicate{}, std::move(villagerResult).value());

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("villager"));
}

TEST_F(CuredZombieVillagerTriggerTest, TestWithBothPredicates)
{
    // 带僵尸和村民类型的条件
    nlohmann::json zombieJson = R"({"type": "minecraft:zombie_villager"})"_json;
    nlohmann::json villagerJson = R"({"type": "minecraft:villager"})"_json;

    auto zombieResult = EntityPredicate::fromJson(zombieJson);
    auto villagerResult = EntityPredicate::fromJson(villagerJson);
    ASSERT_TRUE(zombieResult.success());
    ASSERT_TRUE(villagerResult.success());

    CuredZombieVillagerTriggerInstance instance(std::move(zombieResult).value(), std::move(villagerResult).value());

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("zombie"));
    EXPECT_TRUE(conditionsJson.contains("villager"));
}

// ========== conditionsToJson 序列化测试 ==========

TEST_F(CuredZombieVillagerTriggerTest, ConditionsToJsonRoundtrip)
{
    // 测试序列化和反序列化的往返
    nlohmann::json originalConditions = R"({
        "zombie": {
            "type": "minecraft:zombie_villager"
        },
        "villager": {
            "type": "minecraft:villager"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<CuredZombieVillagerTrigger>();
    ASSERT_NE(trigger, nullptr);

    // 解析
    auto result = trigger->fromJson(originalConditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<CuredZombieVillagerTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 序列化
    nlohmann::json serialized = instance->conditionsToJson();

    // 验证关键字段存在
    EXPECT_TRUE(serialized.contains("zombie"));
    EXPECT_TRUE(serialized.contains("villager"));
}

TEST_F(CuredZombieVillagerTriggerTest, ConditionsToJsonNullForAny)
{
    // 空条件序列化为 null
    CuredZombieVillagerTriggerInstance instance;
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

// ========== 构造函数测试 ==========

TEST_F(CuredZombieVillagerTriggerTest, DefaultConstructor)
{
    CuredZombieVillagerTriggerInstance instance;
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

TEST_F(CuredZombieVillagerTriggerTest, ParameterizedConstructor)
{
    nlohmann::json zombieJson = R"({"type": "minecraft:zombie_villager"})"_json;
    nlohmann::json villagerJson = R"({"type": "minecraft:villager"})"_json;

    auto zombieResult = EntityPredicate::fromJson(zombieJson);
    auto villagerResult = EntityPredicate::fromJson(villagerJson);
    ASSERT_TRUE(zombieResult.success());
    ASSERT_TRUE(villagerResult.success());

    CuredZombieVillagerTriggerInstance instance(std::move(zombieResult).value(), std::move(villagerResult).value());

    // 验证两个谓词都被设置
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("zombie"));
    EXPECT_TRUE(conditionsJson.contains("villager"));
}

// ========== 触发器 ID 测试 ==========

TEST_F(CuredZombieVillagerTriggerTest, TriggerIdCorrect)
{
    EXPECT_STREQ(CuredZombieVillagerTrigger::TRIGGER_ID, "minecraft:cured_zombie_villager");
    EXPECT_STREQ(CuredZombieVillagerTriggerInstance::TRIGGER_ID, "minecraft:cured_zombie_villager");
}

TEST_F(CuredZombieVillagerTriggerTest, GetIdReturnsCorrectResourceLocation)
{
    CuredZombieVillagerTrigger trigger;
    ResourceLocation id = trigger.getId();
    EXPECT_EQ(id.toString(), "minecraft:cured_zombie_villager");
}

// ========== 其他实体触发器注册测试 ==========

TEST_F(CuredZombieVillagerTriggerTest, TameAnimalTriggerRegistration)
{
    // 验证驯服动物触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(TameAnimalTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:tame_animal");
}

// ========== VillagerTradeTrigger 测试 ==========

TEST_F(CuredZombieVillagerTriggerTest, VillagerTradeTriggerFromJson)
{
    // 测试村民交易触发器的 JSON 解析
    nlohmann::json conditions = R"({
        "villager": {
            "type": "minecraft:villager"
        },
        "item": {
            "item": "minecraft:emerald"
        }
    })"_json;

    VillagerTradeTrigger trigger;
    auto result = trigger.fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<VillagerTradeTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(CuredZombieVillagerTriggerTest, VillagerTradeTriggerEmptyConditions)
{
    // 空条件
    nlohmann::json conditions = {};

    VillagerTradeTrigger trigger;
    auto result = trigger.fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<VillagerTradeTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 空条件序列化为 null
    EXPECT_TRUE(instance->conditionsToJson().is_null());
}

// ========== BredAnimalsTrigger 测试 ==========

TEST_F(CuredZombieVillagerTriggerTest, BredAnimalsTriggerFromJson)
{
    // 测试繁殖动物触发器的 JSON 解析
    nlohmann::json conditions = R"({
        "child": {
            "type": "minecraft:cow"
        }
    })"_json;

    BredAnimalsTrigger trigger;
    auto result = trigger.fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(CuredZombieVillagerTriggerTest, BredAnimalsTriggerFullConditions)
{
    // 完整条件
    nlohmann::json conditions = R"({
        "parent": {
            "type": "minecraft:cow"
        },
        "partner": {
            "type": "minecraft:cow"
        },
        "child": {
            "type": "minecraft:cow"
        }
    })"_json;

    BredAnimalsTrigger trigger;
    auto result = trigger.fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BredAnimalsTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("parent"));
    EXPECT_TRUE(serialized.contains("partner"));
    EXPECT_TRUE(serialized.contains("child"));
}

// main 函数由 gtest_main 库提供
