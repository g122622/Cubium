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
#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/AdvancementProgress.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/ImpossibleTrigger.hpp"
#include "common/advancement/trigger/impl/TickTrigger.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include <nlohmann/json.hpp>

// Undef Windows macros that may conflict with method names
#ifdef parent
#undef parent
#endif

using namespace mc;
using namespace mc::advancement;
using namespace mc::server;

/**
 * @brief PlayerAdvancements 单元测试
 *
 * 测试 PlayerAdvancements 的核心功能：
 * - grantCriterion 授予条件时注册监听器
 * - revokeCriterion 撤销条件时重新注册监听器
 * - 可见性计算
 * - 序列化和反序列化
 * - 监听器生命周期管理
 */
class PlayerAdvancementsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册内置触发器
        CriterionTriggers::instance().registerBuiltinTriggers();
        playerAdvancements = std::make_unique<PlayerAdvancements>(PlayerId(1));
    }

    void TearDown() override
    {
        playerAdvancements.reset();
        CriterionTriggers::instance().clear();
        AdvancementManager::instance().clear();
    }

    /**
     * @brief 创建测试成就
     * @param id 成就ID
     * @param criterionNames 条件名称列表
     * @param triggerId 触发器ID（默认为 minecraft:tick）
     * @return 成就指针
     */
    Advancement::Ptr createTestAdvancement(const std::string& id,
        const std::vector<std::string>& criterionNames,
        const std::string& triggerId = "minecraft:tick")
    {
        Advancement::Builder builder{ResourceLocation(id)};

        // 添加显示信息
        AdvancementDisplay display(ItemStack(), // 图标
            std::make_unique<mc::text::StringTextComponent>("Test Title"),
            std::make_unique<mc::text::StringTextComponent>("Test Description"),
            AdvancementFrame::Task,
            true, // showToast
            true, // announceToChat
            false // hidden
        );
        builder.display(std::move(display));

        // 添加条件
        for (const auto& name : criterionNames) {
            auto trigger = std::make_shared<TickTriggerInstance>();
            builder.criterion(name, trigger);
        }

        auto result = builder.build();
        EXPECT_TRUE(result.success());
        if (!result.success()) {
            return nullptr;
        }
        return std::make_shared<Advancement>(std::move(result).value());
    }

    /**
     * @brief 创建不可触发成就（使用 ImpossibleTrigger）
     */
    Advancement::Ptr createImpossibleAdvancement(const std::string& id, const std::vector<std::string>& criterionNames)
    {
        Advancement::Builder builder{ResourceLocation(id)};

        AdvancementDisplay display(ItemStack(),
            std::make_unique<mc::text::StringTextComponent>("Test Title"),
            std::make_unique<mc::text::StringTextComponent>("Test Description"),
            AdvancementFrame::Task,
            true,
            true,
            false);
        builder.display(std::move(display));

        for (const auto& name : criterionNames) {
            auto trigger = std::make_shared<ImpossibleTriggerInstance>();
            builder.criterion(name, trigger);
        }

        auto result = builder.build();
        EXPECT_TRUE(result.success());
        if (!result.success()) {
            return nullptr;
        }
        return std::make_shared<Advancement>(std::move(result).value());
    }

    std::unique_ptr<PlayerAdvancements> playerAdvancements;
};

// ========== grantCriterion 测试 ==========

TEST_F(PlayerAdvancementsTest, GrantCriterionCreatesProgress)
{
    auto adv = createTestAdvancement("minecraft:test/grant_creates", {"criterion1"});
    ASSERT_NE(adv, nullptr);

    // 授予条件应创建进度条目
    bool result = playerAdvancements->grantCriterion(adv, "criterion1");
    EXPECT_TRUE(result);
    EXPECT_TRUE(playerAdvancements->hasProgress(adv));
    EXPECT_TRUE(playerAdvancements->isDone(adv));
}

TEST_F(PlayerAdvancementsTest, GrantCriterionReturnsFalseForSameCriterion)
{
    auto adv = createTestAdvancement("minecraft:test/grant_twice", {"criterion1"});
    ASSERT_NE(adv, nullptr);

    EXPECT_TRUE(playerAdvancements->grantCriterion(adv, "criterion1"));
    // 再次授予同一条件应返回 false
    EXPECT_FALSE(playerAdvancements->grantCriterion(adv, "criterion1"));
}

TEST_F(PlayerAdvancementsTest, GrantCriterionMultiCriteriaAdvancement)
{
    auto adv = createTestAdvancement("minecraft:test/multi_criteria", {"criterion1", "criterion2", "criterion3"});
    ASSERT_NE(adv, nullptr);

    EXPECT_TRUE(playerAdvancements->grantCriterion(adv, "criterion1"));
    EXPECT_TRUE(playerAdvancements->hasProgress(adv));
    EXPECT_FALSE(playerAdvancements->isDone(adv)); // 只有一个条件完成

    EXPECT_TRUE(playerAdvancements->grantCriterion(adv, "criterion2"));
    EXPECT_FALSE(playerAdvancements->isDone(adv)); // 两个条件完成

    EXPECT_TRUE(playerAdvancements->grantCriterion(adv, "criterion3"));
    EXPECT_TRUE(playerAdvancements->isDone(adv)); // 全部完成
}

TEST_F(PlayerAdvancementsTest, GrantCriterionNullAdvancement)
{
    bool result = playerAdvancements->grantCriterion(nullptr, "criterion1");
    EXPECT_FALSE(result);
}

TEST_F(PlayerAdvancementsTest, GrantCriterionUpdatesVisibility)
{
    auto adv = createTestAdvancement("minecraft:test/grant_visibility", {"criterion1"});
    ASSERT_NE(adv, nullptr);

    // 有进度的成就应可见
    EXPECT_FALSE(playerAdvancements->isVisible(adv));
    playerAdvancements->grantCriterion(adv, "criterion1");
    EXPECT_TRUE(playerAdvancements->isVisible(adv));
}

TEST_F(PlayerAdvancementsTest, GrantAllCriteriaCompletesAdvancement)
{
    auto adv = createTestAdvancement("minecraft:test/grant_all", {"c1", "c2", "c3"});
    ASSERT_NE(adv, nullptr);

    bool result = playerAdvancements->grantAllCriteria(adv);
    EXPECT_TRUE(result);
    EXPECT_TRUE(playerAdvancements->isDone(adv));
}

// ========== revokeCriterion 测试 ==========

TEST_F(PlayerAdvancementsTest, RevokeCriterionRemovesProgress)
{
    auto adv = createTestAdvancement("minecraft:test/revoke", {"criterion1"});
    ASSERT_NE(adv, nullptr);

    playerAdvancements->grantCriterion(adv, "criterion1");
    EXPECT_TRUE(playerAdvancements->isDone(adv));

    bool result = playerAdvancements->revokeCriterion(adv, "criterion1");
    EXPECT_TRUE(result);
    EXPECT_FALSE(playerAdvancements->isDone(adv));
}

TEST_F(PlayerAdvancementsTest, RevokeCriterionReturnsFalseForNonexistent)
{
    auto adv = createTestAdvancement("minecraft:test/revoke_nonexist", {"criterion1"});
    ASSERT_NE(adv, nullptr);

    // 撤销未授予的条件应返回 false
    EXPECT_FALSE(playerAdvancements->revokeCriterion(adv, "criterion1"));
}

TEST_F(PlayerAdvancementsTest, RevokeAllCriteriaRemovesAllProgress)
{
    auto adv = createTestAdvancement("minecraft:test/revoke_all", {"c1", "c2"});
    ASSERT_NE(adv, nullptr);

    playerAdvancements->grantAllCriteria(adv);
    EXPECT_TRUE(playerAdvancements->isDone(adv));

    bool result = playerAdvancements->revokeAllCriteria(adv);
    EXPECT_TRUE(result);
    EXPECT_FALSE(playerAdvancements->isDone(adv));
}

TEST_F(PlayerAdvancementsTest, RevokeCriterionUpdatesProgressChanged)
{
    auto adv = createTestAdvancement("minecraft:test/revoke_changed", {"c1", "c2"});
    ASSERT_NE(adv, nullptr);

    playerAdvancements->grantAllCriteria(adv);
    playerAdvancements->clearProgressChanged();

    playerAdvancements->revokeCriterion(adv, "c1");
    const auto& changed = playerAdvancements->getProgressChangedAdvancements();
    EXPECT_EQ(changed.count(adv), 1u);
}

// ========== 进度查询测试 ==========

TEST_F(PlayerAdvancementsTest, GetProgressReturnsNullForUnknown)
{
    auto adv = createTestAdvancement("minecraft:test/unknown", {"c1"});
    ASSERT_NE(adv, nullptr);

    EXPECT_EQ(playerAdvancements->getProgress(adv), nullptr);
}

TEST_F(PlayerAdvancementsTest, GetProgressReturnsNonNullAfterGrant)
{
    auto adv = createTestAdvancement("minecraft:test/get_progress", {"c1"});
    ASSERT_NE(adv, nullptr);

    playerAdvancements->grantCriterion(adv, "c1");
    EXPECT_NE(playerAdvancements->getProgress(adv), nullptr);
}

TEST_F(PlayerAdvancementsTest, IsDoneReturnsFalseForNoProgress)
{
    auto adv = createTestAdvancement("minecraft:test/not_done", {"c1"});
    ASSERT_NE(adv, nullptr);

    EXPECT_FALSE(playerAdvancements->isDone(adv));
}

TEST_F(PlayerAdvancementsTest, HasProgressReturnsFalseForNoProgress)
{
    auto adv = createTestAdvancement("minecraft:test/no_progress", {"c1"});
    ASSERT_NE(adv, nullptr);

    EXPECT_FALSE(playerAdvancements->hasProgress(adv));
}

// ========== 可见性测试 ==========

TEST_F(PlayerAdvancementsTest, AdvancementNotVisibleByDefault)
{
    auto adv = createTestAdvancement("minecraft:test/not_visible", {"c1"});
    ASSERT_NE(adv, nullptr);

    // 没有进度的成就不应自动可见（需要通过触发器系统注册/触发才能获得进度）
    EXPECT_FALSE(playerAdvancements->isVisible(adv));
}

TEST_F(PlayerAdvancementsTest, AdvancementVisibleAfterProgress)
{
    auto adv = createTestAdvancement("minecraft:test/visible_progress", {"c1"});
    ASSERT_NE(adv, nullptr);

    playerAdvancements->grantCriterion(adv, "c1");
    EXPECT_TRUE(playerAdvancements->isVisible(adv));
}

TEST_F(PlayerAdvancementsTest, HiddenAdvancementNotVisibleWithoutProgress)
{
    Advancement::Builder builder{ResourceLocation("minecraft:test/hidden")};
    AdvancementDisplay display(ItemStack(),
        std::make_unique<mc::text::StringTextComponent>("Hidden"),
        std::make_unique<mc::text::StringTextComponent>("Hidden Advancement"),
        AdvancementFrame::Task,
        true,
        true,
        true // hidden
    );
    builder.display(std::move(display));
    auto trigger = std::make_shared<TickTriggerInstance>();
    builder.criterion("c1", trigger);
    auto result = builder.build();
    ASSERT_TRUE(result.success());
    auto adv = std::make_shared<Advancement>(std::move(result).value());

    // 隐藏成就没有进度时不应可见
    EXPECT_FALSE(playerAdvancements->isVisible(adv));

    // 有进度后应可见
    playerAdvancements->grantCriterion(adv, "c1");
    EXPECT_TRUE(playerAdvancements->isVisible(adv));
}

TEST_F(PlayerAdvancementsTest, AdvancementWithNoDisplayNotVisible)
{
    Advancement::Builder builder{ResourceLocation("minecraft:test/no_display")};
    auto trigger = std::make_shared<TickTriggerInstance>();
    builder.criterion("c1", trigger);
    // 不添加 display 信息
    auto result = builder.build();
    ASSERT_TRUE(result.success());
    auto adv = std::make_shared<Advancement>(std::move(result).value());

    // 没有显示信息的成就不可见
    EXPECT_FALSE(playerAdvancements->isVisible(adv));
}

// ========== 序列化测试 ==========

TEST_F(PlayerAdvancementsTest, ToJsonEmptyProgress)
{
    nlohmann::json json = playerAdvancements->toJson();
    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("advancements"));
}

TEST_F(PlayerAdvancementsTest, ToJsonWithProgress)
{
    auto adv = createTestAdvancement("minecraft:test/serialization", {"c1", "c2"});
    ASSERT_NE(adv, nullptr);

    playerAdvancements->grantCriterion(adv, "c1");

    nlohmann::json json = playerAdvancements->toJson();
    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("advancements"));
    EXPECT_TRUE(json["advancements"].contains("minecraft:test/serialization"));

    const auto& progressJson = json["advancements"]["minecraft:test/serialization"];
    EXPECT_TRUE(progressJson.contains("criteria"));
    EXPECT_TRUE(progressJson["criteria"].contains("c1"));
}

TEST_F(PlayerAdvancementsTest, LoadFromJsonRestoresProgress)
{
    auto adv = createTestAdvancement("minecraft:test/load", {"c1", "c2"});
    ASSERT_NE(adv, nullptr);

    // 先注册到 Manager，loadFromJson 需要通过 Manager 查找成就
    auto& manager = AdvancementManager::instance();
    manager.registerAdvancement(adv);

    // 创建 JSON 数据
    nlohmann::json json;
    json["advancements"]["minecraft:test/load"]["criteria"]["c1"] = 0; // 时间戳 0 表示已完成
    json["advancements"]["minecraft:test/load"]["done"] = false;

    bool result = playerAdvancements->loadFromJson(json, manager);
    EXPECT_TRUE(result);
    EXPECT_TRUE(playerAdvancements->hasProgress(adv));
    EXPECT_FALSE(playerAdvancements->isDone(adv)); // c2 未授予

    // 验证进度详情
    auto* progress = playerAdvancements->getProgress(adv);
    ASSERT_NE(progress, nullptr);
    EXPECT_TRUE(progress->getCriterion("c1")->isObtained());
    EXPECT_FALSE(progress->getCriterion("c2")->isObtained());
}

TEST_F(PlayerAdvancementsTest, LoadFromJsonIgnoresUnknownAdvancement)
{
    auto& manager = AdvancementManager::instance();

    nlohmann::json json;
    json["advancements"]["minecraft:nonexistent"]["criteria"]["c1"] = 0;

    // 不应崩溃，未知成就应被忽略
    bool result = playerAdvancements->loadFromJson(json, manager);
    EXPECT_TRUE(result);
}

TEST_F(PlayerAdvancementsTest, RoundTripSerialization)
{
    auto adv = createTestAdvancement("minecraft:test/roundtrip", {"c1", "c2"});
    ASSERT_NE(adv, nullptr);

    auto& manager = AdvancementManager::instance();
    manager.registerAdvancement(adv);

    // 授予条件
    playerAdvancements->grantCriterion(adv, "c1");

    // 序列化
    nlohmann::json json = playerAdvancements->toJson();

    // 反序列化到新的 PlayerAdvancements
    auto restored = std::make_unique<PlayerAdvancements>(PlayerId(1));
    bool result = restored->loadFromJson(json, manager);
    EXPECT_TRUE(result);

    // 验证进度已恢复
    auto* progress = restored->getProgress(adv);
    ASSERT_NE(progress, nullptr);
    EXPECT_TRUE(progress->getCriterion("c1")->isObtained());
    EXPECT_FALSE(progress->getCriterion("c2")->isObtained());
}

// ========== 监听器注册/注销测试 ==========

TEST_F(PlayerAdvancementsTest, RegisterListenersForIncompleteAdvancement)
{
    auto adv = createTestAdvancement("minecraft:test/register", {"c1", "c2"});
    ASSERT_NE(adv, nullptr);

    // 手动创建进度但不完成
    playerAdvancements->grantCriterion(adv, "c1");

    // 进度存在且未完成，应能注册监听器
    playerAdvancements->registerListeners(adv);

    // 验证：进度应存在且 c2 未完成
    auto* progress = playerAdvancements->getProgress(adv);
    ASSERT_NE(progress, nullptr);
    EXPECT_TRUE(progress->getCriterion("c1")->isObtained());
    EXPECT_FALSE(progress->getCriterion("c2")->isObtained());
}

TEST_F(PlayerAdvancementsTest, RegisterListenersSkipsCompletedAdvancement)
{
    auto adv = createTestAdvancement("minecraft:test/register_done", {"c1"});
    ASSERT_NE(adv, nullptr);

    // 完成整个成就
    playerAdvancements->grantAllCriteria(adv);
    EXPECT_TRUE(playerAdvancements->isDone(adv));

    // 对已完成的成就注册监听器应安全（跳过）
    playerAdvancements->registerListeners(adv);
}

TEST_F(PlayerAdvancementsTest, UnregisterListenersForCompletedAdvancement)
{
    auto adv = createTestAdvancement("minecraft:test/unregister_done", {"c1", "c2"});
    ASSERT_NE(adv, nullptr);

    playerAdvancements->grantAllCriteria(adv);

    // 注销监听器应安全
    playerAdvancements->unregisterListeners(adv);
}

TEST_F(PlayerAdvancementsTest, UnregisterListenersNullAdvancement)
{
    // 注销空成就应安全
    playerAdvancements->unregisterListeners(nullptr);
}

TEST_F(PlayerAdvancementsTest, RegisterListenersNullAdvancement)
{
    // 注册空成就应安全
    playerAdvancements->registerListeners(nullptr);
}

// ========== grantCriterion 自动注册监听器测试 ==========

TEST_F(PlayerAdvancementsTest, GrantCriterionRegistersListenersForNewAdvancement)
{
    // 使用 TickTrigger，因为它是内置触发器
    auto adv = createTestAdvancement("minecraft:test/grant_registers", {"c1", "c2"});
    ASSERT_NE(adv, nullptr);

    // 首次授予条件应自动注册监听器
    playerAdvancements->grantCriterion(adv, "c1");

    // 验证进度条目已创建
    auto* progress = playerAdvancements->getProgress(adv);
    ASSERT_NE(progress, nullptr);
    EXPECT_TRUE(progress->getCriterion("c1")->isObtained());
    EXPECT_FALSE(progress->getCriterion("c2")->isObtained());

    // 验证：通过 TickTrigger 触发第二个条件
    auto* tickTrigger = CriterionTriggers::instance().getTrigger<TickTrigger>();
    ASSERT_NE(tickTrigger, nullptr);

    // TickTrigger 应该有监听器注册在这个玩家的 PlayerAdvancements 上
    EXPECT_TRUE(tickTrigger->hasListeners(*playerAdvancements));
}

TEST_F(PlayerAdvancementsTest, GrantAllCriteriaUnregistersListenersWhenComplete)
{
    auto adv = createTestAdvancement("minecraft:test/grant_all_unregisters", {"c1"});
    ASSERT_NE(adv, nullptr);

    playerAdvancements->grantAllCriteria(adv);
    EXPECT_TRUE(playerAdvancements->isDone(adv));

    // 完成后监听器应被注销
    auto* tickTrigger = CriterionTriggers::instance().getTrigger<TickTrigger>();
    ASSERT_NE(tickTrigger, nullptr);
    EXPECT_FALSE(tickTrigger->hasListeners(*playerAdvancements));
}

// ========== loadFromJson 监听器注册测试 ==========

TEST_F(PlayerAdvancementsTest, LoadFromJsonRegistersListenersForIncomplete)
{
    auto adv = createTestAdvancement("minecraft:test/load_listeners", {"c1", "c2"});
    ASSERT_NE(adv, nullptr);

    auto& manager = AdvancementManager::instance();
    manager.registerAdvancement(adv);

    // 创建部分完成的 JSON 数据
    nlohmann::json json;
    json["advancements"]["minecraft:test/load_listeners"]["criteria"]["c1"] = 0;
    json["advancements"]["minecraft:test/load_listeners"]["done"] = false;

    bool result = playerAdvancements->loadFromJson(json, manager);
    EXPECT_TRUE(result);

    // 不完整的成就应有监听器
    auto* tickTrigger = CriterionTriggers::instance().getTrigger<TickTrigger>();
    ASSERT_NE(tickTrigger, nullptr);
    EXPECT_TRUE(tickTrigger->hasListeners(*playerAdvancements));
}

// ========== clearProgressChanged 测试 ==========

TEST_F(PlayerAdvancementsTest, ClearProgressChanged)
{
    auto adv = createTestAdvancement("minecraft:test/clear_changed", {"c1"});
    ASSERT_NE(adv, nullptr);

    playerAdvancements->grantCriterion(adv, "c1");
    EXPECT_EQ(playerAdvancements->getProgressChangedAdvancements().count(adv), 1u);

    playerAdvancements->clearProgressChanged();
    EXPECT_TRUE(playerAdvancements->getProgressChangedAdvancements().empty());
}

// ========== getPlayerId 测试 ==========

TEST_F(PlayerAdvancementsTest, GetPlayerId)
{
    EXPECT_EQ(playerAdvancements->getPlayerId(), PlayerId(1));
}

// ========== 边界条件测试 ==========

TEST_F(PlayerAdvancementsTest, GrantCriterionWithUnknownCriterionName)
{
    auto adv = createTestAdvancement("minecraft:test/unknown_criterion", {"c1"});
    ASSERT_NE(adv, nullptr);

    // 授予成就中不存在的条件名称
    bool result = playerAdvancements->grantCriterion(adv, "nonexistent");
    EXPECT_FALSE(result);
}

TEST_F(PlayerAdvancementsTest, RevokeCriterionWithNoProgress)
{
    auto adv = createTestAdvancement("minecraft:test/revoke_no_progress", {"c1"});
    ASSERT_NE(adv, nullptr);

    // 没有进度时撤销条件
    bool result = playerAdvancements->revokeCriterion(adv, "c1");
    EXPECT_FALSE(result);
}

TEST_F(PlayerAdvancementsTest, LoadFromJsonInvalidInput)
{
    auto& manager = AdvancementManager::instance();

    // 非对象 JSON
    nlohmann::json json = "not an object";
    bool result = playerAdvancements->loadFromJson(json, manager);
    EXPECT_FALSE(result);
}

TEST_F(PlayerAdvancementsTest, OnAdvancementsReloadedClearsState)
{
    auto adv = createTestAdvancement("minecraft:test/reload_clears", {"c1"});
    ASSERT_NE(adv, nullptr);

    playerAdvancements->grantCriterion(adv, "c1");
    EXPECT_TRUE(playerAdvancements->hasProgress(adv));

    auto& manager = AdvancementManager::instance();
    playerAdvancements->onAdvancementsReloaded(manager);

    // 重载后所有进度应被清除
    EXPECT_FALSE(playerAdvancements->hasProgress(adv));
    EXPECT_FALSE(playerAdvancements->isDone(adv));
    EXPECT_TRUE(playerAdvancements->getVisibleAdvancements().empty());
}
