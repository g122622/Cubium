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
 * @file LootTableTriggerTest.cpp
 * @brief 容器战利品生成触发器（vanilla LootTableTrigger）单元测试
 *
 * 覆盖数据包 adventure/salvage_sherd.json、nether/loot_bastion.json 等 10 处进度所依赖的
 * `minecraft:player_generates_container_loot` 条件：
 *   {"loot_table": "minecraft:archaeology/desert_pyramid"}
 * 此前该触发器只存在常量、未注册也未实现，所有依赖它的进度恒不可达。
 */

#include "common/advancement/Criterion.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/EffectTriggers.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

namespace mc {
namespace {

class LootTableTriggerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        advancement::CriterionTriggers::instance().registerBuiltinTriggers();
    }

    void TearDown() override
    {
        advancement::CriterionTriggers::instance().clear();
    }
};

TEST_F(LootTableTriggerTest, IsRegistered)
{
    auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::LootTableTrigger>();
    ASSERT_NE(trigger, nullptr);
    EXPECT_TRUE(advancement::CriterionTriggers::instance().hasTrigger(
        ResourceLocation("minecraft:player_generates_container_loot")));
}

TEST_F(LootTableTriggerTest, ConditionParsesLootTable)
{
    const nlohmann::json json = {{"loot_table", "minecraft:chests/bastion_treasure"}};
    advancement::LootTableTriggerInstance instance;
    ASSERT_TRUE(instance.fromJson(json).success());
    EXPECT_TRUE(instance.test(ResourceLocation("minecraft:chests/bastion_treasure")));
    EXPECT_FALSE(instance.test(ResourceLocation("minecraft:chests/bastion_bridge")));
}

TEST_F(LootTableTriggerTest, MissingLootTableIsRejected)
{
    // vanilla 中 loot_table 为必填字段，缺失时整个进度文件应判为解析失败
    advancement::LootTableTriggerInstance instance;
    const auto result = instance.fromJson(nlohmann::json::object());
    EXPECT_TRUE(result.failed());
}

TEST_F(LootTableTriggerTest, CriterionRoundTripFromAdvancementJson)
{
    // 完整走一遍进度文件里的写法：Criterion 解析 trigger 并从 conditions 取 loot_table
    const nlohmann::json json = {
        {"trigger", "minecraft:player_generates_container_loot"},
        {"conditions", {{"loot_table", "minecraft:archaeology/desert_pyramid"}}},
    };
    auto criterion = advancement::Criterion::fromJson("desert_pyramid", json);
    ASSERT_TRUE(criterion.success());

    auto instance =
        std::dynamic_pointer_cast<advancement::LootTableTriggerInstance>(criterion.value().getTriggerInstance());
    ASSERT_NE(instance, nullptr);
    EXPECT_TRUE(instance->test(ResourceLocation("minecraft:archaeology/desert_pyramid")));
    EXPECT_FALSE(instance->test(ResourceLocation("minecraft:archaeology/desert_well")));
}

TEST_F(LootTableTriggerTest, ConditionsToJsonPreservesLootTable)
{
    advancement::LootTableTriggerInstance instance;
    ASSERT_TRUE(instance.fromJson(nlohmann::json{{"loot_table", "minecraft:chests/simple_dungeon"}}).success());
    const nlohmann::json roundTrip = instance.conditionsToJson();
    ASSERT_TRUE(roundTrip.contains("loot_table"));
    EXPECT_EQ(roundTrip["loot_table"].get<std::string>(), "minecraft:chests/simple_dungeon");
}

} // namespace
} // namespace mc