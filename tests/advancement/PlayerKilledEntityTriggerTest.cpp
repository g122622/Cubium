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

#include "common/TestWorldHelper.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/impl/PlayerKilledEntityTrigger.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;

/**
 * @brief PlayerKilledEntityTrigger 单元测试
 *
 * 测试玩家击杀实体触发器的核心功能：
 * - fromJson 解析
 * - test 条件检测
 * - conditionsToJson 序列化
 */
class PlayerKilledEntityTriggerTest : public ::testing::Test {
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

// ========== PlayerKilledEntityTrigger 注册测试 ==========

TEST_F(PlayerKilledEntityTriggerTest, TriggerRegistration)
{
    // 验证触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(PlayerKilledEntityTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:player_killed_entity");
}

TEST_F(PlayerKilledEntityTriggerTest, EntityKilledPlayerTriggerRegistration)
{
    // 验证反向触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(EntityKilledPlayerTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:entity_killed_player");
}

// ========== PlayerKilledEntityTriggerInstance fromJson 测试 ==========

TEST_F(PlayerKilledEntityTriggerTest, InstanceFromJsonEmpty)
{
    // 空条件应该匹配任何击杀
    nlohmann::json conditions = {};

    auto* trigger = CriterionTriggers::instance().getTrigger<PlayerKilledEntityTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlayerKilledEntityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlayerKilledEntityTriggerTest, InstanceFromJsonWithEntityPredicate)
{
    // 带实体谓词的条件
    nlohmann::json conditions = R"({
        "entity": {
            "type": "minecraft:zombie"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlayerKilledEntityTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlayerKilledEntityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlayerKilledEntityTriggerTest, InstanceFromJsonWithKillingBlow)
{
    // 带伤害源谓词的条件
    nlohmann::json conditions = R"({
        "killing_blow": {
            "is_projectile": true
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlayerKilledEntityTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlayerKilledEntityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlayerKilledEntityTriggerTest, InstanceFromJsonFullConditions)
{
    // 完整条件
    nlohmann::json conditions = R"({
        "entity": {
            "type": "minecraft:skeleton"
        },
        "killing_blow": {
            "is_projectile": true,
            "direct_entity": {
                "type": "minecraft:arrow"
            }
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlayerKilledEntityTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlayerKilledEntityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

// ========== PlayerKilledEntityTriggerInstance test 测试 ==========

TEST_F(PlayerKilledEntityTriggerTest, TestEmptyConditions)
{
    // 空条件应该匹配任何实体和伤害源
    PlayerKilledEntityTriggerInstance instance;

    // 创建模拟实体和伤害源
    // 由于 Entity 和 DamageSource 需要完整的世界环境，
    // 这里主要测试 isAny() 状态
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

TEST_F(PlayerKilledEntityTriggerTest, TestWithEntityPredicate)
{
    // 带实体类型的条件
    nlohmann::json entityJson = R"({
        "type": "minecraft:zombie"
    })"_json;

    auto entityResult = EntityPredicate::fromJson(entityJson);
    ASSERT_TRUE(entityResult.success());

    PlayerKilledEntityTriggerInstance instance(std::move(entityResult).value(), DamageSourcePredicate{});

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("entity"));
}

TEST_F(PlayerKilledEntityTriggerTest, TestWithDamageSourcePredicate)
{
    // 带伤害源的条件
    // 注意：DamageSourcePredicate 的 fromJson/toJson 尚未完全实现
    // 所以这里只测试构造函数，不测试序列化
    DamageSourcePredicate damagePred;

    PlayerKilledEntityTriggerInstance instance(EntityPredicate{}, damagePred);

    // 验证条件不匹配任何谓词（因为都是空的）
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

// ========== conditionsToJson 序列化测试 ==========

TEST_F(PlayerKilledEntityTriggerTest, ConditionsToJsonRoundtrip)
{
    // 测试序列化和反序列化的往返
    // 注意：DamageSourcePredicate 的序列化尚未完全实现
    // 所以只测试 entity 部分
    nlohmann::json originalConditions = R"({
        "entity": {
            "type": "minecraft:creeper"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlayerKilledEntityTrigger>();
    ASSERT_NE(trigger, nullptr);

    // 解析
    auto result = trigger->fromJson(originalConditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlayerKilledEntityTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 序列化
    nlohmann::json serialized = instance->conditionsToJson();

    // 验证关键字段存在
    EXPECT_TRUE(serialized.contains("entity"));
}

// ========== 静态工厂方法测试 ==========

TEST_F(PlayerKilledEntityTriggerTest, FactoryEntityKilled)
{
    // entityKilled() 工厂方法
    auto instance = PlayerKilledEntityTrigger::entityKilled();
    ASSERT_NE(instance, nullptr);
    EXPECT_TRUE(instance->conditionsToJson().is_null()); // 空条件
}

TEST_F(PlayerKilledEntityTriggerTest, FactoryEntityKilledWithPredicate)
{
    // entityKilled(predicate) 工厂方法
    EntityPredicate predicate;
    auto instance = PlayerKilledEntityTrigger::entityKilled(predicate);
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlayerKilledEntityTriggerTest, FactoryKilledByEntity)
{
    // killedByEntity() 工厂方法
    EntityPredicate predicate;
    auto instance = PlayerKilledEntityTrigger::killedByEntity(predicate);
    ASSERT_NE(instance, nullptr);
}

// ========== EntityKilledPlayerTrigger 测试 ==========

TEST_F(PlayerKilledEntityTriggerTest, EntityKilledPlayerFromJson)
{
    nlohmann::json conditions = R"({
        "entity": {
            "type": "minecraft:zombie"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<EntityKilledPlayerTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<EntityKilledPlayerTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlayerKilledEntityTriggerTest, EntityKilledPlayerTestEmptyConditions)
{
    EntityKilledPlayerTriggerInstance instance;
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

TEST_F(PlayerKilledEntityTriggerTest, EntityKilledPlayerConditionsToJson)
{
    // 注意：DamageSourcePredicate 的序列化尚未完全实现
    EntityKilledPlayerTriggerInstance instance;

    // 空条件应该返回 null
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

// main 函数由 gtest_main 库提供
