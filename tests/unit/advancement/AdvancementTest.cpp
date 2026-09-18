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

#include "common/advancement/Advancement.hpp"
#include "common/advancement/AdvancementList.hpp"
#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/AdvancementProgress.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/advancement/trigger/impl/ImpossibleTrigger.hpp"
#include "common/advancement/trigger/impl/InventoryChangedTrigger.hpp"
#include "common/advancement/trigger/impl/TickTrigger.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include <vector>
#include <nlohmann/json.hpp>

// Undef Windows macros that may conflict with method names
#ifdef parent
#undef parent
#endif

using namespace mc;
using namespace mc::advancement;

/**
 * @brief 成就系统测试套件
 *
 * 测试成就系统的核心功能：
 * - Advancement 解析和序列化
 * - AdvancementProgress 进度追踪
 * - AdvancementManager 注册和管理
 * - CriterionTrigger 触发器系统
 */
class AdvancementTest : public ::testing::Test {
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

    // 创建一个简单的测试成就
    Advancement::Ptr createTestAdvancement(
        const std::string& id, const std::string& parentId = "", bool hasDisplay = true)
    {
        Advancement::Builder builder{ResourceLocation(id)};

        if (!parentId.empty()) {
            builder.parent(ResourceLocation(parentId));
        }

        if (hasDisplay) {
            AdvancementDisplay display(ItemStack(), // 图标
                std::make_unique<mc::text::StringTextComponent>("Test Title"),
                std::make_unique<mc::text::StringTextComponent>("Test Description"),
                AdvancementFrame::Task,
                true, // showToast
                true, // announceToChat
                false // hidden
            );
            builder.display(std::move(display));
        }

        auto buildResult = builder.build();
        if (buildResult.success()) {
            return std::make_shared<Advancement>(std::move(buildResult).value());
        }
        return nullptr;
    }
};

// ========== Advancement 测试 ==========

TEST_F(AdvancementTest, BasicAdvancement)
{
    auto advancement = createTestAdvancement("minecraft:test/advancement");
    ASSERT_NE(advancement, nullptr);

    EXPECT_EQ(advancement->getId().toString(), "minecraft:test/advancement");
    EXPECT_TRUE(advancement->isRoot());
    EXPECT_TRUE(advancement->hasDisplay());
    EXPECT_FALSE(advancement->getParent().has_value());
}

TEST_F(AdvancementTest, AdvancementWithParent)
{
    auto parentAdv = createTestAdvancement("minecraft:test/parent");
    auto child = createTestAdvancement("minecraft:test/child", "minecraft:test/parent");

    ASSERT_NE(child, nullptr);
    EXPECT_FALSE(child->isRoot());
    ASSERT_TRUE(child->getParent().has_value());
    EXPECT_EQ(child->getParent()->toString(), "minecraft:test/parent");
}

TEST_F(AdvancementTest, AdvancementFromJson)
{
    nlohmann::json json = R"({
        "parent": "minecraft:story/root",
        "display": {
            "icon": {"item": "minecraft:diamond"},
            "title": "Diamonds!",
            "description": "Acquire diamonds",
            "frame": "task",
            "show_toast": true,
            "announce_to_chat": true
        },
        "criteria": {
            "diamond": {
                "trigger": "minecraft:inventory_changed",
                "conditions": {
                    "items": [{"item": "minecraft:diamond"}]
                }
            }
        }
    })"_json;

    auto result = Advancement::fromJson(ResourceLocation("minecraft:test/diamond"), json);
    ASSERT_TRUE(result.success());

    auto advancementPtr = std::make_shared<Advancement>(std::move(result).value());
    EXPECT_EQ(advancementPtr->getId().toString(), "minecraft:test/diamond");
    EXPECT_TRUE(advancementPtr->hasDisplay());
    EXPECT_EQ(advancementPtr->getCriteria().size(), 1);
    EXPECT_TRUE(advancementPtr->getParent().has_value());
    EXPECT_EQ(advancementPtr->getParent()->toString(), "minecraft:story/root");
}

TEST_F(AdvancementTest, AdvancementToJson)
{
    auto advancement = createTestAdvancement("minecraft:test/advancement");
    ASSERT_NE(advancement, nullptr);

    nlohmann::json json = advancement->toJson();
    EXPECT_TRUE(json.contains("display"));
    EXPECT_TRUE(json["display"].contains("title"));
    EXPECT_TRUE(json["display"].contains("description"));
}

// ========== AdvancementProgress 测试 ==========

TEST_F(AdvancementTest, CriterionProgress)
{
    CriterionProgress progress;

    // 初始状态
    EXPECT_FALSE(progress.isObtained());
    EXPECT_FALSE(progress.getObtainedTime().has_value());

    // 完成
    progress.obtain();
    EXPECT_TRUE(progress.isObtained());
    EXPECT_TRUE(progress.getObtainedTime().has_value());

    // 重置
    progress.reset();
    EXPECT_FALSE(progress.isObtained());
    EXPECT_FALSE(progress.getObtainedTime().has_value());
}

TEST_F(AdvancementTest, AdvancementProgressGrantRevoke)
{
    // 创建带条件的成就
    Advancement::Builder builder(ResourceLocation("minecraft:test/progress"));
    auto trigger = std::make_shared<ImpossibleTriggerInstance>();
    builder.criterion("criterion1", trigger);
    builder.criterion("criterion2", trigger);

    auto result = builder.build();
    ASSERT_TRUE(result.success());

    auto advancementPtr = std::make_shared<Advancement>(std::move(result).value());
    AdvancementProgress progress(advancementPtr);

    // 初始状态
    EXPECT_FALSE(progress.isDone());
    EXPECT_FALSE(progress.hasProgress());
    EXPECT_EQ(progress.countCompletedCriteria(), 0);

    // 授予第一个条件
    EXPECT_TRUE(progress.grantCriterion("criterion1"));
    EXPECT_FALSE(progress.isDone());
    EXPECT_TRUE(progress.hasProgress());
    EXPECT_EQ(progress.countCompletedCriteria(), 1);

    // 重复授予
    EXPECT_FALSE(progress.grantCriterion("criterion1"));

    // 授予第二个条件
    EXPECT_TRUE(progress.grantCriterion("criterion2"));
    EXPECT_TRUE(progress.isDone());
    EXPECT_EQ(progress.countCompletedCriteria(), 2);

    // 撤销
    EXPECT_TRUE(progress.revokeCriterion("criterion1"));
    EXPECT_FALSE(progress.isDone());
    EXPECT_EQ(progress.countCompletedCriteria(), 1);
}

TEST_F(AdvancementTest, AdvancementProgressRequirements)
{
    // 创建带需求的成就（OR关系）
    Advancement::Builder builder(ResourceLocation("minecraft:test/requirements"));
    auto trigger = std::make_shared<ImpossibleTriggerInstance>();
    builder.criterion("a", trigger);
    builder.criterion("b", trigger);
    builder.requirements({{"a", "b"}}); // OR: 任一满足

    auto result = builder.build();
    ASSERT_TRUE(result.success());

    auto advancementPtr = std::make_shared<Advancement>(std::move(result).value());
    AdvancementProgress progress(advancementPtr);

    // 只需满足一个条件
    progress.grantCriterion("a");
    EXPECT_TRUE(progress.isDone());

    // OR关系测试 - 创建新的进度对象测试另一个分支
    AdvancementProgress progress2(advancementPtr);
    progress2.grantCriterion("b");
    EXPECT_TRUE(progress2.isDone());
}

TEST_F(AdvancementTest, AdvancementProgressSerialization)
{
    Advancement::Builder builder(ResourceLocation("minecraft:test/serialization"));
    auto trigger = std::make_shared<ImpossibleTriggerInstance>();
    builder.criterion("criterion1", trigger);
    builder.criterion("criterion2", trigger);

    auto result = builder.build();
    ASSERT_TRUE(result.success());

    auto advancementPtr = std::make_shared<Advancement>(std::move(result).value());
    AdvancementProgress progress(advancementPtr);

    progress.grantCriterion("criterion1");
    progress.grantCriterion("criterion2");

    // 序列化
    nlohmann::json json = progress.toJson();
    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("criteria"));
    EXPECT_TRUE(json["criteria"].contains("criterion1"));
    EXPECT_TRUE(json["criteria"].contains("criterion2"));

    // 反序列化
    auto result2 = AdvancementProgress::fromJson(json, advancementPtr);
    ASSERT_TRUE(result2.success());
    auto progress2 = std::move(result2).value();

    EXPECT_TRUE(progress2.isDone());
    EXPECT_TRUE(progress2.getCriterion("criterion1")->isObtained());
    EXPECT_TRUE(progress2.getCriterion("criterion2")->isObtained());
}

// ========== CriterionProgress 日期时间格式测试 ==========

TEST_F(AdvancementTest, CriterionProgressFromJsonMcJavaStringFormat)
{
    // MC Java 版格式：时间字符串 "2024-06-15 14:30:00 +0800"
    // 对应 Java 的 DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss Z")
    nlohmann::json jsonStr = "2024-06-15 14:30:00 +0800";
    auto progress = CriterionProgress::fromJson(jsonStr);

    // 应正确解析为已完成状态（非 nullopt 时间戳）
    EXPECT_TRUE(progress.isObtained());
    EXPECT_TRUE(progress.getObtainedTime().has_value());

    // 时间戳应为合理的毫秒值（2024 年的时间戳约 1.7 × 10^12 毫秒）
    i64 timeValue = progress.getObtainedTime().value();
    EXPECT_GT(timeValue, 1700000000000LL); // 2023 年之后
    EXPECT_LT(timeValue, 1800000000000LL); // 2027 年之前
}

TEST_F(AdvancementTest, CriterionProgressFromJsonUtcString)
{
    // UTC 时间字符串
    nlohmann::json jsonStr = "2024-01-15 10:30:00 +0000";
    auto progress = CriterionProgress::fromJson(jsonStr);

    EXPECT_TRUE(progress.isObtained());
    EXPECT_TRUE(progress.getObtainedTime().has_value());

    // 验证 UTC+0 时间戳精度
    i64 timeValue = progress.getObtainedTime().value();
    i64 expected = 1705314600000LL;                    // 2024-01-15T10:30:00Z
    EXPECT_LE(std::abs(timeValue - expected), 1000LL); // 允许 1 秒误差
}

TEST_F(AdvancementTest, CriterionProgressFromJsonTimeStringTimezoneConsistency)
{
    // UTC 和 UTC+8 的同一时刻应解析为相同的时间戳
    nlohmann::json utcJson = "2024-01-15 10:30:00 +0000";
    nlohmann::json utc8Json = "2024-01-15 18:30:00 +0800";

    auto utcProgress = CriterionProgress::fromJson(utcJson);
    auto utc8Progress = CriterionProgress::fromJson(utc8Json);

    ASSERT_TRUE(utcProgress.getObtainedTime().has_value());
    ASSERT_TRUE(utc8Progress.getObtainedTime().has_value());

    // 两个时间戳应相等（时区差异已补偿），允许 ±2 秒误差
    i64 diff = std::abs(utcProgress.getObtainedTime().value() - utc8Progress.getObtainedTime().value());
    EXPECT_LE(diff, 2000LL);
}

TEST_F(AdvancementTest, CriterionProgressFromJsonInvalidString)
{
    // 无效的时间字符串应标记为已完成（时间戳为 0，保持向后兼容）
    nlohmann::json invalidJson = "not-a-date";
    auto progress = CriterionProgress::fromJson(invalidJson);

    // 解析失败时回退到时间戳 0，标记为已完成
    EXPECT_TRUE(progress.isObtained());
    EXPECT_TRUE(progress.getObtainedTime().has_value());
    EXPECT_EQ(progress.getObtainedTime().value(), 0);
}

TEST_F(AdvancementTest, CriterionProgressFromJsonNumberFormat)
{
    // 数字格式（毫秒时间戳）：项目内部格式
    nlohmann::json jsonNum = 1705314600000LL;
    auto progress = CriterionProgress::fromJson(jsonNum);

    EXPECT_TRUE(progress.isObtained());
    ASSERT_TRUE(progress.getObtainedTime().has_value());
    EXPECT_EQ(progress.getObtainedTime().value(), 1705314600000LL);
}

TEST_F(AdvancementTest, CriterionProgressFromJsonObjectFormat)
{
    // 对象格式：{"obtainedTime": 毫秒时间戳}
    nlohmann::json jsonObj = {{"obtainedTime", 1705314600000LL}};
    auto progress = CriterionProgress::fromJson(jsonObj);

    EXPECT_TRUE(progress.isObtained());
    ASSERT_TRUE(progress.getObtainedTime().has_value());
    EXPECT_EQ(progress.getObtainedTime().value(), 1705314600000LL);
}

TEST_F(AdvancementTest, CriterionProgressFromJsonNullFormat)
{
    // null 格式：未完成
    nlohmann::json jsonNull = nullptr;
    auto progress = CriterionProgress::fromJson(jsonNull);

    EXPECT_FALSE(progress.isObtained());
    EXPECT_FALSE(progress.getObtainedTime().has_value());
}

TEST_F(AdvancementTest, CriterionProgressToJsonMcJavaStringFormat)
{
    // toJson 应输出 MC Java 版兼容的日期时间字符串
    CriterionProgress progress;
    progress.obtain();

    nlohmann::json json = progress.toJson();

    // 应为字符串类型（MC Java 版格式），不是数字
    ASSERT_TRUE(json.is_string());

    std::string timeStr = json.get<std::string>();
    // 验证格式结构：yyyy-MM-dd HH:mm:ss ZZZZ
    EXPECT_NE(timeStr.find('-'), std::string::npos); // 日期分隔符
    EXPECT_NE(timeStr.find(':'), std::string::npos); // 时间分隔符
    EXPECT_NE(timeStr.find(' '), std::string::npos); // 日期与时间之间的空格
    // 时区偏移：格式为 "yyyy-MM-dd HH:mm:ss +HHMM" 或 "yyyy-MM-dd HH:mm:ss -HHMM"
    // 位置 19 为空格，位置 20 为 '+' 或 '-'（负时区机器上为 '-'，不能断言 '+'）
    ASSERT_GE(timeStr.size(), 25u);
    EXPECT_EQ(timeStr[19], ' ');
    EXPECT_TRUE(timeStr[20] == '+' || timeStr[20] == '-');
}

TEST_F(AdvancementTest, CriterionProgressRoundTripStringFormat)
{
    // 往返测试：toJson 输出字符串 → fromJson 解析回来
    CriterionProgress original;
    original.obtain();

    nlohmann::json json = original.toJson();
    auto restored = CriterionProgress::fromJson(json);

    EXPECT_TRUE(restored.isObtained());
    ASSERT_TRUE(restored.getObtainedTime().has_value());
    ASSERT_TRUE(original.getObtainedTime().has_value());

    // 往返后时间戳应一致（允许 ±1 秒误差）
    i64 diff = std::abs(restored.getObtainedTime().value() - original.getObtainedTime().value());
    EXPECT_LE(diff, 1000LL);
}

TEST_F(AdvancementTest, CriterionProgressUnobtainedToJson)
{
    // 未完成条件序列化应为 null
    CriterionProgress progress;
    nlohmann::json json = progress.toJson();

    EXPECT_TRUE(json.is_null());
}

TEST_F(AdvancementTest, AdvancementProgressRoundTripWithDateTimeString)
{
    // 完整往返测试：授予条件 → 序列化 → 反序列化 → 验证
    Advancement::Builder builder(ResourceLocation("minecraft:test/datetime_roundtrip"));
    auto trigger = std::make_shared<ImpossibleTriggerInstance>();
    builder.criterion("enter_nether", trigger);
    builder.criterion("find_fortress", trigger);

    auto result = builder.build();
    ASSERT_TRUE(result.success());

    auto advancementPtr = std::make_shared<Advancement>(std::move(result).value());
    AdvancementProgress progress(advancementPtr);

    progress.grantCriterion("enter_nether");

    // 序列化
    nlohmann::json json = progress.toJson();
    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("criteria"));

    // 条件值应为字符串（日期时间格式）
    EXPECT_TRUE(json["criteria"]["enter_nether"].is_string());

    // 反序列化
    auto result2 = AdvancementProgress::fromJson(json, advancementPtr);
    ASSERT_TRUE(result2.success());
    auto progress2 = std::move(result2).value();

    EXPECT_FALSE(progress2.isDone()); // 只完成了一个条件
    EXPECT_TRUE(progress2.getCriterion("enter_nether")->isObtained());
    EXPECT_FALSE(progress2.getCriterion("find_fortress")->isObtained());
}

// ========== AdvancementManager 测试 ==========

TEST_F(AdvancementTest, ManagerRegisterGet)
{
    auto& manager = AdvancementManager::instance();
    manager.clear();

    auto advancement = createTestAdvancement("minecraft:test/manager");
    ASSERT_NE(advancement, nullptr);

    EXPECT_TRUE(manager.registerAdvancement(advancement));
    EXPECT_TRUE(manager.contains(ResourceLocation("minecraft:test/manager")));

    auto retrieved = manager.get(ResourceLocation("minecraft:test/manager"));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getId().toString(), "minecraft:test/manager");

    // 重复注册
    EXPECT_FALSE(manager.registerAdvancement(advancement));

    // 移除
    EXPECT_TRUE(manager.removeAdvancement(ResourceLocation("minecraft:test/manager")));
    EXPECT_FALSE(manager.contains(ResourceLocation("minecraft:test/manager")));

    manager.clear();
}

TEST_F(AdvancementTest, ManagerHierarchy)
{
    auto& manager = AdvancementManager::instance();
    manager.clear();

    // 创建父子成就
    auto root = createTestAdvancement("minecraft:test/root");
    auto child1 = createTestAdvancement("minecraft:test/child1", "minecraft:test/root");
    auto child2 = createTestAdvancement("minecraft:test/child2", "minecraft:test/root");
    auto grandchild = createTestAdvancement("minecraft:test/grandchild", "minecraft:test/child1");

    manager.registerAdvancement(root);
    manager.registerAdvancement(child1);
    manager.registerAdvancement(child2);
    manager.registerAdvancement(grandchild);

    // 检查根成就
    const auto& roots = manager.getRoots();
    EXPECT_EQ(roots.size(), 1);
    EXPECT_EQ(roots[0]->getId().toString(), "minecraft:test/root");

    // 检查子成就
    auto rootRetrieved = manager.get(ResourceLocation("minecraft:test/root"));
    ASSERT_NE(rootRetrieved, nullptr);
    EXPECT_EQ(rootRetrieved->getChildren().size(), 2);

    manager.clear();
}

// ========== CriterionTrigger 测试 ==========

TEST_F(AdvancementTest, ImpossibleTrigger)
{
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(ImpossibleTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    // Impossible 触发器不能从 JSON 创建实例
    nlohmann::json conditions = {};
    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());
    EXPECT_NE(result.value(), nullptr);
}

TEST_F(AdvancementTest, TickTrigger)
{
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(TickTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    // Tick 触发器无条件
    nlohmann::json conditions = {};
    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<TickTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->getId().toString(), "minecraft:tick");
}

TEST_F(AdvancementTest, InventoryChangedTrigger)
{
    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(InventoryChangedTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    // 创建带条件的实例
    nlohmann::json conditions = R"({
        "items": [{"item": "minecraft:diamond", "count": {"min": 1}}],
        "slots": {"occupied": {"min": 1}}
    })"_json;

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<InventoryChangedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->getId().toString(), "minecraft:inventory_changed");
}

// ========== ItemPredicate 测试 ==========

TEST_F(AdvancementTest, ItemPredicateBasic)
{
    nlohmann::json json = R"({
        "item": "minecraft:diamond",
        "count": {"min": 1, "max": 10}
    })"_json;

    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    ItemPredicate predicate = std::move(result).value();

    // 测试匹配（需要 ItemStack 支持）
    // ItemStack diamond(ItemRegistry::get("minecraft:diamond"), 5);
    // EXPECT_TRUE(predicate.test(diamond));
}

TEST_F(AdvancementTest, ItemPredicateAny)
{
    ItemPredicate any;
    EXPECT_TRUE(any.isAny());

    nlohmann::json json = {};
    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

// ========== AdvancementList 测试 ==========

TEST_F(AdvancementTest, AdvancementListOperations)
{
    AdvancementList list;

    auto root = createTestAdvancement("minecraft:test/root");
    auto child = createTestAdvancement("minecraft:test/child", "minecraft:test/root");

    list.add(root);
    list.add(child);

    EXPECT_EQ(list.size(), 2);
    EXPECT_TRUE(list.contains(ResourceLocation("minecraft:test/root")));
    EXPECT_TRUE(list.contains(ResourceLocation("minecraft:test/child")));

    // 根成就
    const auto& roots = list.getRoots();
    EXPECT_EQ(roots.size(), 1);

    // 移除
    list.remove(ResourceLocation("minecraft:test/child"));
    EXPECT_EQ(list.size(), 1);
    EXPECT_FALSE(list.contains(ResourceLocation("minecraft:test/child")));
}

// ========== 集成测试 ==========

TEST_F(AdvancementTest, FullWorkflow)
{
    auto& manager = AdvancementManager::instance();
    manager.clear();

    // 1. 创建成就树
    auto root = createTestAdvancement("minecraft:story/root");
    auto mineStone = createTestAdvancement("minecraft:story/mine_stone", "minecraft:story/root");

    manager.registerAdvancement(root);
    manager.registerAdvancement(mineStone);

    // 2. 验证层次结构
    auto rootRetrieved = manager.get(ResourceLocation("minecraft:story/root"));
    ASSERT_NE(rootRetrieved, nullptr);
    EXPECT_EQ(rootRetrieved->getChildren().size(), 1);

    // 3. 创建进度
    AdvancementProgress progress(mineStone);
    EXPECT_FALSE(progress.isDone());

    // 4. 完成条件（假设有条件）
    // progress.grantCriterion("mine_stone");
    // EXPECT_TRUE(progress.isDone());

    // 5. 清理
    manager.clear();
}

// ========== InventoryChangedTriggerInstance 测试 ==========

TEST_F(AdvancementTest, InventoryChangedTriggerInstance_SlotCounting)
{
    // 创建槽位条件测试
    nlohmann::json conditions = R"({
        "slots": {
            "occupied": {"min": 1, "max": 5},
            "full": {"min": 0, "max": 2},
            "empty": {"min": 10, "max": 20}
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(InventoryChangedTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<InventoryChangedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证条件解析正确
    EXPECT_FALSE(instance->getSlotsOccupied().isUnbounded());
    EXPECT_FALSE(instance->getSlotsFull().isUnbounded());
    EXPECT_FALSE(instance->getSlotsEmpty().isUnbounded());
    EXPECT_TRUE(instance->getItems().empty());
}

TEST_F(AdvancementTest, InventoryChangedTriggerInstance_ItemPredicateParsing)
{
    // 创建物品条件测试
    nlohmann::json conditions = R"({
        "items": [
            {"item": "minecraft:diamond"},
            {"item": "minecraft:iron_ingot", "count": 10}
        ]
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(InventoryChangedTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<InventoryChangedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证物品谓词解析正确
    const auto& items = instance->getItems();
    EXPECT_EQ(items.size(), 2);
}

TEST_F(AdvancementTest, InventoryChangedTriggerInstance_Serialization)
{
    // 测试序列化和反序列化
    nlohmann::json conditions = R"({
        "slots": {
            "occupied": {"min": 5},
            "empty": {"max": 10}
        },
        "items": [{"item": "minecraft:gold_ingot"}]
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(InventoryChangedTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<InventoryChangedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 序列化
    nlohmann::json json = instance->conditionsToJson();
    EXPECT_TRUE(json.contains("slots"));
    EXPECT_TRUE(json.contains("items"));
}

TEST_F(AdvancementTest, InventoryChangedTrigger_HasItemsFactory)
{
    // 使用 JSON 解析来创建 ItemPredicate
    nlohmann::json diamondJson = R"({"item": "minecraft:diamond"})"_json;
    auto result1 = ItemPredicate::fromJson(diamondJson);
    ASSERT_TRUE(result1.success());
    ItemPredicate diamondPred = std::move(result1).value();

    auto instance = InventoryChangedTrigger::hasItem(diamondPred);
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->getItems().size(), 1);

    // 创建多个物品谓词
    nlohmann::json ironJson = R"({"item": "minecraft:iron_ingot", "count": 5})"_json;
    auto result2 = ItemPredicate::fromJson(ironJson);
    ASSERT_TRUE(result2.success());
    ItemPredicate ironPred = std::move(result2).value();

    std::vector<ItemPredicate> items;
    items.push_back(diamondPred);
    items.push_back(ironPred);

    auto instance2 = InventoryChangedTrigger::hasItems(std::move(items));
    ASSERT_NE(instance2, nullptr);
    EXPECT_EQ(instance2->getItems().size(), 2);
}

TEST_F(AdvancementTest, InventoryChangedTriggerInstance_AnyMatch)
{
    // 空条件应该匹配任何情况
    nlohmann::json conditions = {};

    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(InventoryChangedTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<InventoryChangedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 创建模拟物品栏（空）
    std::vector<ItemStack> emptyInventory(41); // 41 slots

    // 应该匹配任何情况
    EXPECT_TRUE(instance->testWithInventory(41,
        [&emptyInventory](i32 slot) -> const ItemStack& { return emptyInventory[static_cast<std::size_t>(slot)]; }));
}

TEST_F(AdvancementTest, InventoryChangedTriggerInstance_SlotConditions)
{
    // 测试槽位条件：占用槽位数、满槽位数、空槽位数
    nlohmann::json conditions = R"({
        "slots": {
            "occupied": {"min": 3, "max": 5},
            "full": {"min": 1},
            "empty": {"min": 30}
        }
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(InventoryChangedTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<InventoryChangedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 创建模拟物品栏：4个占用槽位，其中2个满
    std::vector<ItemStack> inventory(41);
    // 槽位 0: 数量 64 (满)
    inventory[0] = ItemStack();
    // 槽位 1: 数量 32 (不满)
    inventory[1] = ItemStack();
    // 槽位 2: 数量 64 (满)
    inventory[2] = ItemStack();
    // 槽位 3: 数量 16 (不满)
    inventory[3] = ItemStack();
    // 槽位 4-40: 空

    // 注意：这里测试的是槽位计数逻辑，ItemStack::isEmpty() 和 getMaxStackSize() 的实际行为
    // 取决于具体的 ItemStack 实现
    // 由于我们使用空的 ItemStack，它们实际上是空的，所以 occupied = 0
    // 这个测试主要验证条件解析是否正确
    EXPECT_FALSE(instance->getSlotsOccupied().isUnbounded());
    EXPECT_FALSE(instance->getSlotsFull().isUnbounded());
    EXPECT_FALSE(instance->getSlotsEmpty().isUnbounded());
}

TEST_F(AdvancementTest, InventoryChangedTriggerInstance_MultipleItemPredicates)
{
    // 测试多个物品谓词：所有谓词都必须匹配
    nlohmann::json conditions = R"({
        "items": [
            {"item": "minecraft:diamond"},
            {"item": "minecraft:iron_ingot"}
        ]
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(InventoryChangedTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<InventoryChangedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证两个物品谓词都被解析
    const auto& items = instance->getItems();
    EXPECT_EQ(items.size(), 2);

    // 测试匹配逻辑
    // 创建模拟物品栏：全部为空
    std::vector<ItemStack> emptyInventory(41);

    // 由于物品栏为空，不应该匹配（需要两个物品都有）
    EXPECT_FALSE(instance->testWithInventory(41,
        [&emptyInventory](i32 slot) -> const ItemStack& { return emptyInventory[static_cast<std::size_t>(slot)]; }));
}

TEST_F(AdvancementTest, InventoryChangedTriggerInstance_SingleItemPredicate)
{
    // 测试单个物品谓词：只需要有一个匹配
    nlohmann::json conditions = R"({
        "items": [
            {"item": "minecraft:diamond"}
        ]
    })"_json;

    auto* trigger = CriterionTriggers::instance().getTrigger(ResourceLocation(InventoryChangedTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<InventoryChangedTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);

    // 验证只有一个物品谓词
    const auto& items = instance->getItems();
    EXPECT_EQ(items.size(), 1);

    // 创建模拟物品栏：全部为空
    std::vector<ItemStack> emptyInventory(41);

    // 由于物品栏为空，不应该匹配
    EXPECT_FALSE(instance->testWithInventory(41,
        [&emptyInventory](i32 slot) -> const ItemStack& { return emptyInventory[static_cast<std::size_t>(slot)]; }));
}

// main 函数由 gtest_main 库提供
