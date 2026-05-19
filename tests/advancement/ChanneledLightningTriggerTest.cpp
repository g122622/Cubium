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
#include "common/advancement/trigger/impl/ChanneledLightningTrigger.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;

/**
 * @brief ChanneledLightningTrigger 单元测试
 *
 * 测试引雷附魔触发器的核心功能：
 * - fromJson 解析
 * - test 条件检测
 * - conditionsToJson 序列化
 *
 * 参考 MC 1.16.5: CriteriaTriggers.CHANNELED_LIGHTNING
 */
class ChanneledLightningTriggerTest : public ::testing::Test {
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

// ========== ChanneledLightningTrigger 注册测试 ==========

TEST_F(ChanneledLightningTriggerTest, TriggerRegistration)
{
    // 验证触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(ChanneledLightningTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:channeled_lightning");
}

// ========== ChanneledLightningTriggerInstance fromJson 测试 ==========

TEST_F(ChanneledLightningTriggerTest, InstanceFromJsonEmpty)
{
    // 空条件应该匹配任何引雷击中
    nlohmann::json conditions = {};

    auto* trigger = CriterionTriggers::instance().getTrigger<ChanneledLightningTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<ChanneledLightningTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(ChanneledLightningTriggerTest, InstanceFromJsonNull)
{
    // null 条件也应该匹配任何引雷击中
    nlohmann::json conditions = nullptr;

    auto* trigger = CriterionTriggers::instance().getTrigger<ChanneledLightningTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<ChanneledLightningTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(ChanneledLightningTriggerTest, InstanceFromJsonWithSingleVictim)
{
    // 单个受害者条件
    nlohmann::json conditions = R"({
        "victims": [
            { "type": "minecraft:zombie" }
        ]
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<ChanneledLightningTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<ChanneledLightningTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->getVictims().size(), 1u);
}

TEST_F(ChanneledLightningTriggerTest, InstanceFromJsonWithMultipleVictims)
{
    // 多个受害者条件
    nlohmann::json conditions = R"({
        "victims": [
            { "type": "minecraft:zombie" },
            { "type": "minecraft:skeleton" }
        ]
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<ChanneledLightningTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<ChanneledLightningTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->getVictims().size(), 2u);
}

TEST_F(ChanneledLightningTriggerTest, InstanceFromJsonInvalidVictimsNotArray)
{
    // victims 不是数组应该失败
    nlohmann::json conditions = R"({
        "victims": "invalid"
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<ChanneledLightningTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    EXPECT_FALSE(result.success());
}

// ========== ChanneledLightningTriggerInstance test 测试 ==========

TEST_F(ChanneledLightningTriggerTest, TestEmptyConditions)
{
    // 空条件应该匹配任何受害者列表
    ChanneledLightningTriggerInstance instance;

    // 空受害者列表
    std::vector<const Entity*> emptyVictims;
    EXPECT_TRUE(instance.test(emptyVictims));

    // null 指针列表（应该被过滤）
    std::vector<const Entity*> nullVictims{nullptr, nullptr};
    EXPECT_TRUE(instance.test(nullVictims));
}

TEST_F(ChanneledLightningTriggerTest, TestWithSingleEntityPredicate)
{
    // 带单个实体类型的条件
    nlohmann::json entityJson = R"({
        "type": "minecraft:zombie"
    })"_json;

    auto entityResult = EntityPredicate::fromJson(entityJson);
    ASSERT_TRUE(entityResult.success());

    std::vector<EntityPredicate> victims;
    victims.push_back(std::move(entityResult).value());

    ChanneledLightningTriggerInstance instance(std::move(victims));

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("victims"));
    EXPECT_TRUE(conditionsJson["victims"].is_array());
    EXPECT_EQ(conditionsJson["victims"].size(), 1u);
}

TEST_F(ChanneledLightningTriggerTest, TestWithMultipleEntityPredicates)
{
    // 带多个实体类型的条件
    nlohmann::json entityJson1 = R"({
        "type": "minecraft:zombie"
    })"_json;

    nlohmann::json entityJson2 = R"({
        "type": "minecraft:skeleton"
    })"_json;

    auto entityResult1 = EntityPredicate::fromJson(entityJson1);
    auto entityResult2 = EntityPredicate::fromJson(entityJson2);
    ASSERT_TRUE(entityResult1.success());
    ASSERT_TRUE(entityResult2.success());

    std::vector<EntityPredicate> victims;
    victims.push_back(std::move(entityResult1).value());
    victims.push_back(std::move(entityResult2).value());

    ChanneledLightningTriggerInstance instance(std::move(victims));

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("victims"));
    EXPECT_TRUE(conditionsJson["victims"].is_array());
    EXPECT_EQ(conditionsJson["victims"].size(), 2u);
}

// ========== conditionsToJson 序列化测试 ==========

TEST_F(ChanneledLightningTriggerTest, ConditionsToJsonRoundtrip)
{
    // 测试序列化和反序列化的往返
    nlohmann::json originalConditions = R"({
        "victims": [
            { "type": "minecraft:creeper" },
            { "type": "minecraft:skeleton" }
        ]
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<ChanneledLightningTrigger>();
    ASSERT_NE(trigger, nullptr);

    // 解析
    auto result = trigger->fromJson(originalConditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<ChanneledLightningTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 序列化
    nlohmann::json serialized = instance->conditionsToJson();

    // 验证关键字段存在
    EXPECT_TRUE(serialized.contains("victims"));
    EXPECT_TRUE(serialized["victims"].is_array());
    EXPECT_EQ(serialized["victims"].size(), 2u);
}

TEST_F(ChanneledLightningTriggerTest, ConditionsToJsonEmpty)
{
    // 空实例序列化为 null
    ChanneledLightningTriggerInstance instance;
    nlohmann::json serialized = instance.conditionsToJson();
    EXPECT_TRUE(serialized.is_null());
}

// ========== TRIGGER_ID 常量测试 ==========

TEST_F(ChanneledLightningTriggerTest, TriggerIdConstant)
{
    // 验证 TRIGGER_ID 常量
    EXPECT_STREQ(ChanneledLightningTrigger::TRIGGER_ID, "minecraft:channeled_lightning");
    EXPECT_STREQ(ChanneledLightningTriggerInstance::TRIGGER_ID, "minecraft:channeled_lightning");
}

// ========== 构造函数测试 ==========

TEST_F(ChanneledLightningTriggerTest, DefaultConstructor)
{
    // 默认构造函数
    ChanneledLightningTriggerInstance instance;
    EXPECT_TRUE(instance.getVictims().empty());
}

TEST_F(ChanneledLightningTriggerTest, ParameterizedConstructor)
{
    // 带参数的构造函数
    std::vector<EntityPredicate> victims;
    victims.emplace_back(); // 添加一个默认谓词

    ChanneledLightningTriggerInstance instance(std::move(victims));
    EXPECT_EQ(instance.getVictims().size(), 1u);
}

// main 函数由 gtest_main 库提供
