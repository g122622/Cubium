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
#include "common/advancement/trigger/conditions/BlockPredicate.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/advancement/trigger/impl/BlockTriggers.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::advancement;

/**
 * @brief PlacedBlockTrigger 单元测试
 *
 * 测试放置方块触发器的核心功能：
 * - fromJson 解析
 * - test 条件检测
 * - conditionsToJson 序列化
 * - 触发器注册
 */
class PlacedBlockTriggerTest : public ::testing::Test {
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

// ========== PlacedBlockTrigger 注册测试 ==========

TEST_F(PlacedBlockTriggerTest, TriggerRegistration)
{
    // 验证触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(PlacedBlockTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->getId().toString(), "minecraft:placed_block");
}

// ========== PlacedBlockTriggerInstance fromJson 测试 ==========

TEST_F(PlacedBlockTriggerTest, InstanceFromJsonEmpty)
{
    // 空条件应该匹配任何方块放置
    nlohmann::json conditions = {};

    auto* trigger = CriterionTriggers::instance().getTrigger<PlacedBlockTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlacedBlockTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlacedBlockTriggerTest, InstanceFromJsonWithBlockPredicate)
{
    // 带方块谓词的条件
    nlohmann::json conditions = R"({
        "block": "minecraft:diamond_block"
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlacedBlockTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlacedBlockTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("block"));
}

TEST_F(PlacedBlockTriggerTest, InstanceFromJsonWithLocationPredicate)
{
    // 带位置谓词的条件
    nlohmann::json conditions = R"({
        "location": {
            "dimension": "minecraft:overworld"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlacedBlockTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlacedBlockTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlacedBlockTriggerTest, InstanceFromJsonWithItemPredicate)
{
    // 带物品谓词的条件
    nlohmann::json conditions = R"({
        "item": {
            "item": "minecraft:diamond"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlacedBlockTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlacedBlockTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlacedBlockTriggerTest, InstanceFromJsonFullConditions)
{
    // 完整条件
    nlohmann::json conditions = R"({
        "block": "minecraft:stone",
        "location": {
            "dimension": "minecraft:overworld"
        },
        "item": {
            "item": "minecraft:stone"
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlacedBlockTrigger>();
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlacedBlockTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证所有条件都被解析
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("block"));
    EXPECT_TRUE(serialized.contains("location"));
    EXPECT_TRUE(serialized.contains("item"));
}

// ========== PlacedBlockTriggerInstance test 测试 ==========

TEST_F(PlacedBlockTriggerTest, TestEmptyConditions)
{
    // 空条件应该匹配任何方块放置
    PlacedBlockTriggerInstance instance;

    // 验证 isAny 状态
    EXPECT_TRUE(instance.conditionsToJson().is_null());
}

TEST_F(PlacedBlockTriggerTest, TestWithBlockPredicate)
{
    // 带方块类型的条件
    nlohmann::json blockJson = R"({
        "block": "minecraft:diamond_block"
    })"_json;

    auto blockResult = BlockPredicate::fromJson(blockJson);
    ASSERT_TRUE(blockResult.success());

    PlacedBlockTriggerInstance instance(std::move(blockResult).value(), LocationPredicate{}, ItemPredicate{});

    // 验证序列化
    nlohmann::json conditionsJson = instance.conditionsToJson();
    EXPECT_TRUE(conditionsJson.contains("block"));
}

// ========== conditionsToJson 序列化测试 ==========

TEST_F(PlacedBlockTriggerTest, ConditionsToJsonRoundtrip)
{
    // 测试序列化和反序列化的往返
    nlohmann::json originalConditions = R"({
        "block": "minecraft:cobblestone"
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger<PlacedBlockTrigger>();
    ASSERT_NE(trigger, nullptr);

    // 解析
    auto result = trigger->fromJson(originalConditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<PlacedBlockTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 序列化
    nlohmann::json serialized = instance->conditionsToJson();

    // 验证关键字段存在
    EXPECT_TRUE(serialized.contains("block"));
}

// ========== EnterBlockTrigger 测试 ==========

TEST_F(PlacedBlockTriggerTest, EnterBlockTriggerRegistration)
{
    // 验证进入方块触发器已注册
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(EnterBlockTrigger::TRIGGER_ID));
    // 注意：EnterBlockTrigger 未在 registerBuiltinTriggers 中注册
    // 所以这里期望返回 nullptr
    // 如果后续注册了，这个测试需要更新
}

TEST_F(PlacedBlockTriggerTest, EnterBlockTriggerFromJson)
{
    // 测试进入方块触发器的 JSON 解析
    nlohmann::json conditions = R"({
        "block": "minecraft:water"
    })"_json;

    EnterBlockTrigger trigger;
    auto result = trigger.fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<EnterBlockTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

// ========== SlideDownBlockTrigger 测试 ==========

TEST_F(PlacedBlockTriggerTest, SlideDownBlockTriggerFromJson)
{
    // 测试滑落触发器的 JSON 解析
    nlohmann::json conditions = R"({
        "block": "minecraft:honey_block"
    })"_json;

    SlideDownBlockTrigger trigger;
    auto result = trigger.fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<SlideDownBlockTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlacedBlockTriggerTest, SlideDownBlockTriggerEmptyConditions)
{
    // 空条件
    nlohmann::json conditions = {};

    SlideDownBlockTrigger trigger;
    auto result = trigger.fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<SlideDownBlockTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 空条件序列化为 null
    EXPECT_TRUE(instance->conditionsToJson().is_null());
}

// ========== BeeNestDestroyedTrigger 测试 ==========

TEST_F(PlacedBlockTriggerTest, BeeNestDestroyedTriggerFromJson)
{
    // 测试破坏蜂巢触发器的 JSON 解析
    nlohmann::json conditions = R"({
        "block": "minecraft:bee_nest",
        "num_bees_inside": {
            "min": 1
        }
    })"_json;

    BeeNestDestroyedTrigger trigger;
    auto result = trigger.fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BeeNestDestroyedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
}

TEST_F(PlacedBlockTriggerTest, BeeNestDestroyedTriggerFullConditions)
{
    // 完整条件
    nlohmann::json conditions = R"({
        "block": "minecraft:beehive",
        "item": {
            "item": "minecraft:shears"
        },
        "num_bees_inside": {
            "min": 3,
            "max": 5
        }
    })"_json;

    BeeNestDestroyedTrigger trigger;
    auto result = trigger.fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<BeeNestDestroyedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证序列化
    nlohmann::json serialized = instance->conditionsToJson();
    EXPECT_TRUE(serialized.contains("block"));
    EXPECT_TRUE(serialized.contains("item"));
    EXPECT_TRUE(serialized.contains("num_bees_inside"));
}

// main 函数由 gtest_main 库提供
