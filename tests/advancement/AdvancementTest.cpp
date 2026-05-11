#include <gtest/gtest.h>

#include "common/advancement/Advancement.hpp"
#include "common/advancement/AdvancementProgress.hpp"
#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/AdvancementList.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/ImpossibleTrigger.hpp"
#include "common/advancement/trigger/impl/TickTrigger.hpp"
#include "common/advancement/trigger/impl/InventoryChangedTrigger.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/text/StringTextComponent.hpp"
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
    void SetUp() override {
        // 注册内置触发器
        CriterionTriggers::instance().registerBuiltinTriggers();
    }

    void TearDown() override {
        // 清理
        CriterionTriggers::instance().clear();
    }

    // 创建一个简单的测试成就
    Advancement::Ptr createTestAdvancement(
        const std::string& id,
        const std::string& parentId = "",
        bool hasDisplay = true)
    {
        Advancement::Builder builder(ResourceLocation(id));

        if (!parentId.empty()) {
            builder.parent(ResourceLocation(parentId));
        }

        if (hasDisplay) {
            AdvancementDisplay display(
                ItemStack(),  // 图标
                std::make_unique<mc::text::StringTextComponent>("Test Title"),
                std::make_unique<mc::text::StringTextComponent>("Test Description"),
                AdvancementFrame::Task,
                true,  // showToast
                true,  // announceToChat
                false  // hidden
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

TEST_F(AdvancementTest, BasicAdvancement) {
    auto advancement = createTestAdvancement("minecraft:test/advancement");
    ASSERT_NE(advancement, nullptr);

    EXPECT_EQ(advancement->getId().toString(), "minecraft:test/advancement");
    EXPECT_TRUE(advancement->isRoot());
    EXPECT_TRUE(advancement->hasDisplay());
    EXPECT_FALSE(advancement->getParent().has_value());
}

TEST_F(AdvancementTest, AdvancementWithParent) {
    auto parentAdv = createTestAdvancement("minecraft:test/parent");
    auto child = createTestAdvancement("minecraft:test/child", "minecraft:test/parent");

    ASSERT_NE(child, nullptr);
    EXPECT_FALSE(child->isRoot());
    ASSERT_TRUE(child->getParent().has_value());
    EXPECT_EQ(child->getParent()->toString(), "minecraft:test/parent");
}

TEST_F(AdvancementTest, AdvancementFromJson) {
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

TEST_F(AdvancementTest, AdvancementToJson) {
    auto advancement = createTestAdvancement("minecraft:test/advancement");
    ASSERT_NE(advancement, nullptr);

    nlohmann::json json = advancement->toJson();
    EXPECT_TRUE(json.contains("display"));
    EXPECT_TRUE(json["display"].contains("title"));
    EXPECT_TRUE(json["display"].contains("description"));
}

// ========== AdvancementProgress 测试 ==========

TEST_F(AdvancementTest, CriterionProgress) {
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

TEST_F(AdvancementTest, AdvancementProgressGrantRevoke) {
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

TEST_F(AdvancementTest, AdvancementProgressRequirements) {
    // 创建带需求的成就（OR关系）
    Advancement::Builder builder(ResourceLocation("minecraft:test/requirements"));
    auto trigger = std::make_shared<ImpossibleTriggerInstance>();
    builder.criterion("a", trigger);
    builder.criterion("b", trigger);
    builder.requirements({{"a", "b"}});  // OR: 任一满足

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

TEST_F(AdvancementTest, AdvancementProgressSerialization) {
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
    EXPECT_TRUE(json.contains("criterion1"));
    EXPECT_TRUE(json.contains("criterion2"));

    // 反序列化
    auto result2 = AdvancementProgress::fromJson(json, advancementPtr);
    ASSERT_TRUE(result2.success());
    auto progress2 = std::move(result2).value();

    EXPECT_TRUE(progress2.isDone());
    EXPECT_TRUE(progress2.getCriterion("criterion1")->isObtained());
    EXPECT_TRUE(progress2.getCriterion("criterion2")->isObtained());
}

// ========== AdvancementManager 测试 ==========

TEST_F(AdvancementTest, ManagerRegisterGet) {
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

TEST_F(AdvancementTest, ManagerHierarchy) {
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

TEST_F(AdvancementTest, ImpossibleTrigger) {
    auto* trigger = CriterionTriggers::instance().getTrigger(
        ResourceLocation(ImpossibleTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    // Impossible 触发器不能从 JSON 创建实例
    nlohmann::json conditions = {};
    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());
    EXPECT_NE(result.value(), nullptr);
}

TEST_F(AdvancementTest, TickTrigger) {
    auto* trigger = CriterionTriggers::instance().getTrigger(
        ResourceLocation(TickTrigger::TRIGGER_ID));
    ASSERT_NE(trigger, nullptr);

    // Tick 触发器无条件
    nlohmann::json conditions = {};
    auto result = trigger->fromJson(conditions);
    ASSERT_TRUE(result.success());

    auto instance = std::dynamic_pointer_cast<TickTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->getId().toString(), "minecraft:tick");
}

TEST_F(AdvancementTest, InventoryChangedTrigger) {
    auto* trigger = CriterionTriggers::instance().getTrigger(
        ResourceLocation(InventoryChangedTrigger::TRIGGER_ID));
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

TEST_F(AdvancementTest, ItemPredicateBasic) {
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

TEST_F(AdvancementTest, ItemPredicateAny) {
    ItemPredicate any;
    EXPECT_TRUE(any.isAny());

    nlohmann::json json = {};
    auto result = ItemPredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

// ========== AdvancementList 测试 ==========

TEST_F(AdvancementTest, AdvancementListOperations) {
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

TEST_F(AdvancementTest, FullWorkflow) {
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
