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
#include "common/advancement/trigger/impl/AvoidVibrationTrigger.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;

/**
 * @brief AvoidVibrationTrigger 单元测试
 *
 * 测试避免振动触发器的核心功能：
 * - fromJson 解析（无条件的触发器）
 * - conditionsToJson 序列化
 * - 触发器注册与 ID 验证
 *
 * 参考 MC 1.21.11: CriteriaTriggers.AVOID_VIBRATION
 * 此触发器无条件谓词，任何成功的避免振动都会触发。
 */
class AvoidVibrationTriggerTest : public ::testing::Test {
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

// ========== AvoidVibrationTrigger 注册测试 ==========

TEST_F(AvoidVibrationTriggerTest, TriggerRegistration)
{
    // 验证触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(AvoidVibrationTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:avoid_vibration");
}

TEST_F(AvoidVibrationTriggerTest, GetTriggerTemplate)
{
    // 验证模板版本的 getTrigger 可用
    auto* trigger = CriterionTriggers::instance().getTrigger<AvoidVibrationTrigger>();
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:avoid_vibration");
}

// ========== TRIGGER_ID 常量测试 ==========

TEST_F(AvoidVibrationTriggerTest, TriggerIdConstant)
{
    // 验证 TRIGGER_ID 常量
    EXPECT_STREQ(AvoidVibrationTrigger::TRIGGER_ID, "minecraft:avoid_vibration");
    EXPECT_STREQ(AvoidVibrationTriggerInstance::TRIGGER_ID, "minecraft:avoid_vibration");
}

// ========== AvoidVibrationTriggerInstance fromJson 测试 ==========

TEST_F(AvoidVibrationTriggerTest, InstanceFromJsonEmpty)
{
    // 空条件应该成功解析（避免振动触发器无条件）
    nlohmann::json conditions = {};

    auto* trigger = CriterionTriggers::instance().getTrigger<AvoidVibrationTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<AvoidVibrationTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(AvoidVibrationTriggerTest, InstanceFromJsonNull)
{
    // null 条件也应该成功解析
    nlohmann::json conditions = nullptr;

    auto* trigger = CriterionTriggers::instance().getTrigger<AvoidVibrationTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<AvoidVibrationTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(AvoidVibrationTriggerTest, InstanceFromJsonArbitraryData)
{
    // 任意 JSON 数据也应该成功解析（避免振动触发器忽略所有条件）
    nlohmann::json conditions = R"({
        "player": [{ "type": "minecraft:player" }],
        "some_random_field": 42
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<AvoidVibrationTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<AvoidVibrationTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

// ========== conditionsToJson 序列化测试 ==========

TEST_F(AvoidVibrationTriggerTest, ConditionsToJsonEmpty)
{
    // 默认实例序列化为 null（无条件）
    AvoidVibrationTriggerInstance instance;
    nlohmann::json serialized = instance.conditionsToJson();
    EXPECT_TRUE(serialized.is_null());
}

TEST_F(AvoidVibrationTriggerTest, ConditionsToJsonRoundtrip)
{
    // 解析后重新序列化，应该始终为 null
    nlohmann::json conditions = {};

    auto* trigger = CriterionTriggers::instance().getTrigger<AvoidVibrationTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<AvoidVibrationTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.is_null());
}

TEST_F(AvoidVibrationTriggerTest, ConditionsToJsonRoundtripWithArbitraryInput)
{
    // 即使输入了任意 JSON，序列化结果也应该是 null
    nlohmann::json conditions = R"({
        "foo": "bar"
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<AvoidVibrationTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<AvoidVibrationTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.is_null());
}

// ========== 构造函数测试 ==========

TEST_F(AvoidVibrationTriggerTest, DefaultConstructor)
{
    // 默认构造的实例应无条件匹配
    AvoidVibrationTriggerInstance instance;
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

// ========== getId 测试 ==========

TEST_F(AvoidVibrationTriggerTest, GetIdReturnsCorrectResourceLocation)
{
    auto* trigger = CriterionTriggers::instance().getTrigger<AvoidVibrationTrigger>();
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId(), ResourceLocation("minecraft:avoid_vibration"));
}

// ========== CriterionTriggers 注册验证 ==========

TEST_F(AvoidVibrationTriggerTest, CriterionTriggersNamespaceConstant)
{
    // 验证 triggers 命名空间中的常量
    EXPECT_STREQ(triggers::AVOID_VIBRATION, "minecraft:avoid_vibration");
    EXPECT_STREQ(triggers::AVOID_VIBRATION, AvoidVibrationTrigger::TRIGGER_ID);
}

// main 函数由 gtest_main 库提供
