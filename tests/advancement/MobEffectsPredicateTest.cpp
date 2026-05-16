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
#include "common/advancement/trigger/conditions/MobEffectsPredicate.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/impl/EffectTriggers.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;
using namespace mc::entity::effect;

/**
 * @brief MobEffectsPredicate 单元测试
 *
 * 测试效果谓词的功能：
 * - EffectInstancePredicate 效果实例匹配
 * - MobEffectsPredicate 效果组合匹配
 * - JSON 解析和序列化
 */
class MobEffectsPredicateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置测试环境
    }

    void TearDown() override
    {
        // 清理
    }
};

// ========== EffectInstancePredicate 测试 ==========

TEST_F(MobEffectsPredicateTest, EffectInstancePredicate_DefaultMatchesAny)
{
    EffectInstancePredicate predicate;

    // 默认谓词应该匹配任何效果实例
    EffectInstance speed(EffectType::Speed, 600, 0);
    EXPECT_TRUE(predicate.test(&speed));

    EffectInstance regen(EffectType::Regeneration, 200, 2);
    EXPECT_TRUE(predicate.test(&regen));
}

TEST_F(MobEffectsPredicateTest, EffectInstancePredicate_DefaultReturnsFalseForNullptr)
{
    EffectInstancePredicate predicate;

    // 默认谓词对 nullptr 应该返回 false
    EXPECT_FALSE(predicate.test(nullptr));
}

TEST_F(MobEffectsPredicateTest, EffectInstancePredicate_AmplifierBounds)
{
    // 等级 1-3 (amplifier 0-2)
    IntBounds amplifierBounds = IntBounds::between(0, 2);
    EffectInstancePredicate predicate(amplifierBounds, IntBounds{}, std::nullopt, std::nullopt);

    // 等级 I (amplifier 0) 应该匹配
    EffectInstance speed1(EffectType::Speed, 600, 0);
    EXPECT_TRUE(predicate.test(&speed1));

    // 等级 II (amplifier 1) 应该匹配
    EffectInstance speed2(EffectType::Speed, 600, 1);
    EXPECT_TRUE(predicate.test(&speed2));

    // 等级 III (amplifier 2) 应该匹配
    EffectInstance speed3(EffectType::Speed, 600, 2);
    EXPECT_TRUE(predicate.test(&speed3));

    // 等级 IV (amplifier 3) 不应该匹配
    EffectInstance speed4(EffectType::Speed, 600, 3);
    EXPECT_FALSE(predicate.test(&speed4));
}

TEST_F(MobEffectsPredicateTest, EffectInstancePredicate_DurationBounds)
{
    // 持续时间至少 100 tick
    IntBounds durationBounds = IntBounds::atLeast(100);
    EffectInstancePredicate predicate(IntBounds{}, durationBounds, std::nullopt, std::nullopt);

    // 持续时间 200 应该匹配
    EffectInstance effect1(EffectType::Speed, 200, 0);
    EXPECT_TRUE(predicate.test(&effect1));

    // 持续时间 100 应该匹配
    EffectInstance effect2(EffectType::Speed, 100, 0);
    EXPECT_TRUE(predicate.test(&effect2));

    // 持续时间 50 不应该匹配
    EffectInstance effect3(EffectType::Speed, 50, 0);
    EXPECT_FALSE(predicate.test(&effect3));
}

TEST_F(MobEffectsPredicateTest, EffectInstancePredicate_AmbientCheck)
{
    // 要求环境效果
    EffectInstancePredicate predicate(IntBounds{}, IntBounds{}, true, std::nullopt);

    // 环境效果应该匹配
    EffectInstance ambientEffect(EffectType::Speed, 600, 0, true, true, true);
    EXPECT_TRUE(predicate.test(&ambientEffect));

    // 非环境效果不应该匹配
    EffectInstance normalEffect(EffectType::Speed, 600, 0, false, true, true);
    EXPECT_FALSE(predicate.test(&normalEffect));
}

TEST_F(MobEffectsPredicateTest, EffectInstancePredicate_VisibleCheck)
{
    // 要求显示粒子
    EffectInstancePredicate predicate(IntBounds{}, IntBounds{}, std::nullopt, true);

    // 显示粒子的效果应该匹配
    EffectInstance visibleEffect(EffectType::Speed, 600, 0, false, true, true);
    EXPECT_TRUE(predicate.test(&visibleEffect));

    // 隐藏粒子的效果不应该匹配
    EffectInstance invisibleEffect(EffectType::Speed, 600, 0, false, false, true);
    EXPECT_FALSE(predicate.test(&invisibleEffect));
}

TEST_F(MobEffectsPredicateTest, EffectInstancePredicate_CombinedConditions)
{
    // 组合条件：等级 II+ (amplifier >= 1)，持续时间 100+ tick，非环境效果
    IntBounds amplifierBounds = IntBounds::atLeast(1);
    IntBounds durationBounds = IntBounds::atLeast(100);
    EffectInstancePredicate predicate(amplifierBounds, durationBounds, false, std::nullopt);

    // 等级 II，持续时间 200，非环境 - 应该匹配
    EffectInstance good(EffectType::Speed, 200, 1, false, true, true);
    EXPECT_TRUE(predicate.test(&good));

    // 等级 I，持续时间 200，非环境 - 不应该匹配（等级不够）
    EffectInstance lowLevel(EffectType::Speed, 200, 0, false, true, true);
    EXPECT_FALSE(predicate.test(&lowLevel));

    // 等级 II，持续时间 50，非环境 - 不应该匹配（持续时间不够）
    EffectInstance shortDuration(EffectType::Speed, 50, 1, false, true, true);
    EXPECT_FALSE(predicate.test(&shortDuration));

    // 等级 II，持续时间 200，环境效果 - 不应该匹配
    EffectInstance ambient(EffectType::Speed, 200, 1, true, true, true);
    EXPECT_FALSE(predicate.test(&ambient));
}

TEST_F(MobEffectsPredicateTest, EffectInstancePredicate_FromJson)
{
    // 测试 JSON 解析
    nlohmann::json json = R"({
        "amplifier": {"min": 1, "max": 3},
        "duration": {"min": 100},
        "ambient": false,
        "visible": true
    })"_json;

    auto result = EffectInstancePredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    EffectInstancePredicate predicate = result.value();

    // 验证解析结果
    EXPECT_TRUE(predicate.getAmplifier().test(1));
    EXPECT_TRUE(predicate.getAmplifier().test(2));
    EXPECT_TRUE(predicate.getAmplifier().test(3));
    EXPECT_FALSE(predicate.getAmplifier().test(0));
    EXPECT_FALSE(predicate.getAmplifier().test(4));

    EXPECT_TRUE(predicate.getDuration().test(100));
    EXPECT_TRUE(predicate.getDuration().test(200));
    EXPECT_FALSE(predicate.getDuration().test(50));

    EXPECT_TRUE(predicate.getAmbient().has_value());
    EXPECT_FALSE(predicate.getAmbient().value());

    EXPECT_TRUE(predicate.getVisible().has_value());
    EXPECT_TRUE(predicate.getVisible().value());
}

TEST_F(MobEffectsPredicateTest, EffectInstancePredicate_FromJson_Empty)
{
    // 空 JSON 应该返回默认谓词
    nlohmann::json json = nullptr;
    auto result = EffectInstancePredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    EffectInstancePredicate predicate = result.value();
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(MobEffectsPredicateTest, EffectInstancePredicate_ToJson)
{
    // 创建谓词
    IntBounds amplifierBounds = IntBounds::exactly(2);
    IntBounds durationBounds = IntBounds::atLeast(100);
    EffectInstancePredicate predicate(amplifierBounds, durationBounds, true, false);

    // 序列化为 JSON
    nlohmann::json json = predicate.toJson();

    // 验证 JSON
    EXPECT_TRUE(json.contains("amplifier"));
    EXPECT_TRUE(json.contains("duration"));
    EXPECT_TRUE(json.contains("ambient"));
    EXPECT_TRUE(json.contains("visible"));
    EXPECT_EQ(json["amplifier"], 2);
    EXPECT_TRUE(json["duration"].contains("min"));
    EXPECT_EQ(json["duration"]["min"], 100);
    EXPECT_EQ(json["ambient"], true);
    EXPECT_EQ(json["visible"], false);
}

// ========== MobEffectsPredicate 测试 ==========

TEST_F(MobEffectsPredicateTest, EmptyPredicateMatchesAny)
{
    MobEffectsPredicate predicate;

    // 空谓词应该匹配任何实体
    // 注意：需要 LivingEntity 来测试效果，这里测试 isAny() 方法
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(MobEffectsPredicateTest, FromJson_SingleEffect)
{
    // 解析单个效果条件
    nlohmann::json json = R"({
        "minecraft:speed": {
            "amplifier": {"min": 0, "max": 2}
        }
    })"_json;

    auto result = MobEffectsPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    MobEffectsPredicate predicate = result.value();
    EXPECT_FALSE(predicate.isAny());
    EXPECT_EQ(predicate.getEffects().size(), 1u);
    EXPECT_TRUE(predicate.getEffects().count(EffectType::Speed) > 0);
}

TEST_F(MobEffectsPredicateTest, FromJson_MultipleEffects)
{
    // 解析多个效果条件
    nlohmann::json json = R"({
        "minecraft:speed": {
            "amplifier": {"min": 0, "max": 2}
        },
        "minecraft:regeneration": {
            "duration": {"min": 200}
        }
    })"_json;

    auto result = MobEffectsPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    MobEffectsPredicate predicate = result.value();
    EXPECT_FALSE(predicate.isAny());
    EXPECT_EQ(predicate.getEffects().size(), 2u);
    EXPECT_TRUE(predicate.getEffects().count(EffectType::Speed) > 0);
    EXPECT_TRUE(predicate.getEffects().count(EffectType::Regeneration) > 0);
}

TEST_F(MobEffectsPredicateTest, FromJson_UnknownEffect)
{
    // 未知效果类型应该被跳过
    nlohmann::json json = R"({
        "minecraft:unknown_effect": {},
        "minecraft:speed": {}
    })"_json;

    auto result = MobEffectsPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    MobEffectsPredicate predicate = result.value();
    // 只有 speed 应该被解析
    EXPECT_EQ(predicate.getEffects().size(), 1u);
    EXPECT_TRUE(predicate.getEffects().count(EffectType::Speed) > 0);
}

TEST_F(MobEffectsPredicateTest, FromJson_Empty)
{
    // 空 JSON 应该返回默认谓词
    nlohmann::json json = nullptr;
    auto result = MobEffectsPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    MobEffectsPredicate predicate = result.value();
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(MobEffectsPredicateTest, ToJson)
{
    // 创建谓词
    std::unordered_map<EffectType, EffectInstancePredicate> effects;
    effects[EffectType::Speed] = EffectInstancePredicate(IntBounds::exactly(1), IntBounds{}, std::nullopt, std::nullopt);

    MobEffectsPredicate predicate(std::move(effects));

    // 序列化为 JSON
    nlohmann::json json = predicate.toJson();

    // 验证 JSON
    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("minecraft:speed"));
    EXPECT_TRUE(json["minecraft:speed"].contains("amplifier"));
    EXPECT_EQ(json["minecraft:speed"]["amplifier"], 1);
}

TEST_F(MobEffectsPredicateTest, RoundTrip_JsonSerialization)
{
    // 测试 JSON 序列化/反序列化往返
    nlohmann::json originalJson = R"({
        "minecraft:speed": {
            "amplifier": {"min": 0, "max": 3},
            "duration": {"min": 100, "max": 600}
        },
        "minecraft:strength": {
            "amplifier": 2
        }
    })"_json;

    // 解析
    auto parseResult = MobEffectsPredicate::fromJson(originalJson);
    ASSERT_TRUE(parseResult.success());

    // 序列化
    nlohmann::json serializedJson = parseResult.value().toJson();

    // 再次解析
    auto reparseResult = MobEffectsPredicate::fromJson(serializedJson);
    ASSERT_TRUE(reparseResult.success());

    MobEffectsPredicate reparsed = reparseResult.value();

    // 验证效果数量相同
    EXPECT_EQ(parseResult.value().getEffects().size(), reparsed.getEffects().size());

    // 验证效果类型相同
    for (const auto& [type, predicate] : parseResult.value().getEffects()) {
        EXPECT_TRUE(reparsed.getEffects().count(type) > 0);
    }
}

// ========== 集成测试 ==========

TEST_F(MobEffectsPredicateTest, Integration_WithEntityPredicate)
{
    // 测试 EntityPredicate 集成
    nlohmann::json json = R"({
        "type": "minecraft:player",
        "effects": {
            "minecraft:speed": {
                "amplifier": {"min": 1}
            }
        }
    })"_json;

    auto result = EntityPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    EntityPredicate predicate = result.value();
    EXPECT_TRUE(predicate.getType().has_value());
    EXPECT_EQ(predicate.getType().value().toString(), "minecraft:player");
    EXPECT_FALSE(predicate.getEffects().isAny());
}

TEST_F(MobEffectsPredicateTest, Integration_EffectsChangedTrigger)
{
    // 测试 EffectsChangedTrigger 集成
    nlohmann::json json = R"({
        "effects": {
            "minecraft:regeneration": {
                "amplifier": 2
            }
        }
    })"_json;

    // 测试 EffectsChangedTrigger 解析
    auto trigger = std::make_shared<EffectsChangedTriggerInstance>();
    auto result = trigger->fromJson(json);
    ASSERT_TRUE(result.success());

    // 验证条件可以正确序列化
    nlohmann::json serialized = trigger->conditionsToJson();
    EXPECT_TRUE(serialized.contains("effects"));
    EXPECT_TRUE(serialized["effects"].contains("minecraft:regeneration"));
}
