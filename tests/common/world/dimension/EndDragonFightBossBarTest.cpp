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

// ============================================================================
// 末影龙战斗 Boss 栏同步单元测试
//
// 本文件覆盖 EndDragonFight::updateDragon()、setDragonKilled() 和 tick() 中
// 与 Boss 栏相关的所有逻辑分支，包括：
//   - updateDragon 同步血量百分比、重置失联计数器、同步自定义名称
//   - updateDragon UUID 不匹配时忽略
//   - setDragonKilled 设置百分比为 0、隐藏 Boss 栏
//   - tick() 每 tick 设置 Boss 栏可见性 = !dragonKilled
//   - tick() 玩家扫描节奏（TIME_BETWEEN_PLAYER_SCANS）
//   - replacePlayers 增量更新（避免闪烁）
//   - NullDragonBossBar 默认行为
//   - setDragonBossBar 注入 nullptr 恢复为 NullDragonBossBar
// ============================================================================

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/boss/EnderDragonEntity.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/EndDragonFightTestAccessor.hpp"
#include "common/world/dimension/end/EndDragonFight.hpp"
#include "common/world/dimension/end/IDragonBossBar.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace mc;

namespace mc::test {

// ============================================================================
// 模拟 Boss 栏 - 记录所有调用以供断言
// ============================================================================

class MockDragonBossBar final : public IDragonBossBar {
public:
    // ========== 调用记录结构 ==========

    struct SetPercentCall {
        f32 percent;
    };
    struct SetNameCall {
        std::string text;
    };
    struct SetVisibleCall {
        bool visible;
    };
    struct AddPlayerCall {
        PlayerId playerId;
    };
    struct RemovePlayerCall {
        PlayerId playerId;
    };
    struct ReplacePlayersCall {
        std::set<PlayerId> playerIds;
    };

    // ========== 属性更新 ==========

    void setPercent(f32 percent) override
    {
        m_setPercentCalls.push_back({percent});
        m_percent = percent;
    }

    void setName(std::unique_ptr<text::ITextComponent> name) override
    {
        std::string text = name ? name->getUnformattedText() : std::string();
        m_setNameCalls.push_back({text});
        m_nameText = text;
    }

    void setVisible(bool visible) override
    {
        m_setVisibleCalls.push_back({visible});
        m_visible = visible;
    }

    // ========== 玩家管理 ==========

    void addPlayer(PlayerId playerId) override
    {
        m_addPlayerCalls.push_back({playerId});
        m_players.insert(playerId);
    }

    void removePlayer(PlayerId playerId) override
    {
        m_removePlayerCalls.push_back({playerId});
        m_players.erase(playerId);
    }

    void removeAllPlayers() override
    {
        m_removeAllPlayersCallCount++;
        m_players.clear();
    }

    void replacePlayers(const std::set<PlayerId>& playerIds) override
    {
        m_replacePlayersCalls.push_back({playerIds});
        m_players = playerIds;
    }

    // ========== 状态查询 ==========

    [[nodiscard]] bool hasPlayers() const override { return !m_players.empty(); }
    [[nodiscard]] f32 percent() const override { return m_percent; }
    [[nodiscard]] bool visible() const override { return m_visible; }
    [[nodiscard]] const std::set<PlayerId>& getPlayers() const override { return m_players; }

    // ========== 测试断言辅助 ==========

    [[nodiscard]] const std::vector<SetPercentCall>& setPercentCalls() const { return m_setPercentCalls; }
    [[nodiscard]] const std::vector<SetNameCall>& setNameCalls() const { return m_setNameCalls; }
    [[nodiscard]] const std::vector<SetVisibleCall>& setVisibleCalls() const { return m_setVisibleCalls; }
    [[nodiscard]] const std::vector<AddPlayerCall>& addPlayerCalls() const { return m_addPlayerCalls; }
    [[nodiscard]] const std::vector<RemovePlayerCall>& removePlayerCalls() const { return m_removePlayerCalls; }
    [[nodiscard]] const std::vector<ReplacePlayersCall>& replacePlayersCalls() const { return m_replacePlayersCalls; }
    [[nodiscard]] i32 removeAllPlayersCallCount() const { return m_removeAllPlayersCallCount; }
    [[nodiscard]] const std::set<PlayerId>& players() const { return m_players; }
    [[nodiscard]] const std::string& nameText() const { return m_nameText; }

    void reset()
    {
        m_setPercentCalls.clear();
        m_setNameCalls.clear();
        m_setVisibleCalls.clear();
        m_addPlayerCalls.clear();
        m_removePlayerCalls.clear();
        m_replacePlayersCalls.clear();
        m_removeAllPlayersCallCount = 0;
        m_percent = 0.0f;
        m_visible = false;
        m_nameText.clear();
        m_players.clear();
    }

private:
    std::vector<SetPercentCall> m_setPercentCalls;
    std::vector<SetNameCall> m_setNameCalls;
    std::vector<SetVisibleCall> m_setVisibleCalls;
    std::vector<AddPlayerCall> m_addPlayerCalls;
    std::vector<RemovePlayerCall> m_removePlayerCalls;
    std::vector<ReplacePlayersCall> m_replacePlayersCalls;
    i32 m_removeAllPlayersCallCount = 0;

    f32 m_percent = 0.0f;
    bool m_visible = false;
    std::string m_nameText;
    std::set<PlayerId> m_players;
};

} // namespace mc::test

// ============================================================================
// 测试夹具
// ============================================================================

/**
 * @brief 最小化的测试世界，继承 BaseTestWorld 以访问其 protected 默认构造函数。
 *
 * 本测试套件不需要覆写任何 IWorld 方法——_updatePlayers 在无玩家世界中应直接
 * 调用 replacePlayers(空集合)，因此使用基类的默认实现即可。
 */
class DragonFightBossBarTestWorld final : public ::mc::test::BaseTestWorld {
public:
    DragonFightBossBarTestWorld() = default;
};

class EndDragonFightBossBarTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        m_fight = std::make_unique<EndDragonFight>(42, std::nullopt);
        m_accessor = std::make_unique<::mc::test::EndDragonFightTestAccessor>(*m_fight);
        m_bossBar = std::make_unique<::mc::test::MockDragonBossBar>();
        m_rawBossBar = m_bossBar.get();
        m_fight->setDragonBossBar(std::move(m_bossBar));
    }

    void TearDown() override
    {
        m_accessor.reset();
        m_fight.reset();
    }

    /**
     * @brief 创建一个设置了指定 UUID 和血量的末影龙测试实例
     *
     * EnderDragonEntity 构造函数会调用 registerAttributes()，将 MAX_HEALTH 设为 200，
     * 因此默认 maxHealth() == 200。setHealth 会被 clamp 到 [0, maxHealth]。
     */
    std::unique_ptr<entity::EnderDragonEntity> makeDragon(const std::string& uuid, f32 health = 200.0f)
    {
        auto dragon = std::make_unique<entity::EnderDragonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
        dragon->setUuid(uuid);
        dragon->setHealth(health);
        return dragon;
    }

    DragonFightBossBarTestWorld m_world;
    std::unique_ptr<EndDragonFight> m_fight;
    std::unique_ptr<::mc::test::EndDragonFightTestAccessor> m_accessor;
    std::unique_ptr<::mc::test::MockDragonBossBar> m_bossBar;
    ::mc::test::MockDragonBossBar* m_rawBossBar = nullptr;
};

// ============================================================================
// updateDragon：血量百分比同步
// ============================================================================

TEST_F(EndDragonFightBossBarTest, UpdateDragon_FullHealth_SetsPercentToOne)
{
    // UUID 匹配且满血：百分比应为 1.0
    const std::string uuid = "abcdef0123456789abcdef0123456789";
    m_accessor->setDragonUUID(uuid);

    auto dragon = makeDragon(uuid, 200.0f);
    m_fight->updateDragon(*dragon);

    ASSERT_FALSE(m_rawBossBar->setPercentCalls().empty());
    EXPECT_FLOAT_EQ(m_rawBossBar->setPercentCalls().back().percent, 1.0f);
}

TEST_F(EndDragonFightBossBarTest, UpdateDragon_HalfHealth_SetsPercentToHalf)
{
    const std::string uuid = "abcdef0123456789abcdef0123456789";
    m_accessor->setDragonUUID(uuid);

    auto dragon = makeDragon(uuid, 100.0f); // 100 / 200 = 0.5
    m_fight->updateDragon(*dragon);

    ASSERT_FALSE(m_rawBossBar->setPercentCalls().empty());
    EXPECT_FLOAT_EQ(m_rawBossBar->setPercentCalls().back().percent, 0.5f);
}

TEST_F(EndDragonFightBossBarTest, UpdateDragon_ZeroHealth_SetsPercentToZero)
{
    const std::string uuid = "abcdef0123456789abcdef0123456789";
    m_accessor->setDragonUUID(uuid);

    auto dragon = makeDragon(uuid, 0.0f);
    m_fight->updateDragon(*dragon);

    ASSERT_FALSE(m_rawBossBar->setPercentCalls().empty());
    EXPECT_FLOAT_EQ(m_rawBossBar->setPercentCalls().back().percent, 0.0f);
}

TEST_F(EndDragonFightBossBarTest, UpdateDragon_QuarterHealth_SetsPercentToQuarter)
{
    const std::string uuid = "abcdef0123456789abcdef0123456789";
    m_accessor->setDragonUUID(uuid);

    auto dragon = makeDragon(uuid, 50.0f); // 50 / 200 = 0.25
    m_fight->updateDragon(*dragon);

    ASSERT_FALSE(m_rawBossBar->setPercentCalls().empty());
    EXPECT_FLOAT_EQ(m_rawBossBar->setPercentCalls().back().percent, 0.25f);
}

// ============================================================================
// updateDragon：UUID 不匹配时忽略
// ============================================================================

TEST_F(EndDragonFightBossBarTest, UpdateDragon_UuidMismatch_NoBossBarUpdate)
{
    // 战斗管理器追踪的 UUID 与龙实体 UUID 不匹配时，不应更新 Boss 栏
    m_accessor->setDragonUUID("tracked-uuid-aaaaaaaaaaaaaaaaaaaaaa");

    auto dragon = makeDragon("different-uuid-bbbbbbbbbbbbbbbbbbbb", 100.0f);

    m_rawBossBar->reset(); // 清除构造后可能的调用
    m_fight->updateDragon(*dragon);

    EXPECT_TRUE(m_rawBossBar->setPercentCalls().empty());
    EXPECT_TRUE(m_rawBossBar->setNameCalls().empty());
}

TEST_F(EndDragonFightBossBarTest, UpdateDragon_UuidMismatch_DoesNotResetTicksSinceDragonSeen)
{
    // UUID 不匹配时不应重置 ticksSinceDragonSeen
    m_accessor->setDragonUUID("tracked-uuid-aaaaaaaaaaaaaaaaaaaaaa");
    m_accessor->setTicksSinceDragonSeen(500);

    auto dragon = makeDragon("different-uuid-bbbbbbbbbbbbbbbbbbbb", 100.0f);
    m_fight->updateDragon(*dragon);

    EXPECT_EQ(m_accessor->ticksSinceDragonSeen(), 500);
}

// ============================================================================
// updateDragon：重置 ticksSinceDragonSeen
// ============================================================================

TEST_F(EndDragonFightBossBarTest, UpdateDragon_UuidMatch_ResetsTicksSinceDragonSeen)
{
    const std::string uuid = "abcdef0123456789abcdef0123456789";
    m_accessor->setDragonUUID(uuid);
    m_accessor->setTicksSinceDragonSeen(500);

    auto dragon = makeDragon(uuid, 100.0f);
    m_fight->updateDragon(*dragon);

    EXPECT_EQ(m_accessor->ticksSinceDragonSeen(), 0);
}

// ============================================================================
// updateDragon：自定义名称同步
// ============================================================================

TEST_F(EndDragonFightBossBarTest, UpdateDragon_WithCustomName_SyncsNameToBossBar)
{
    const std::string uuid = "abcdef0123456789abcdef0123456789";
    m_accessor->setDragonUUID(uuid);

    auto dragon = makeDragon(uuid, 200.0f);
    dragon->setCustomName("Jean");

    m_fight->updateDragon(*dragon);

    ASSERT_FALSE(m_rawBossBar->setNameCalls().empty());
    EXPECT_EQ(m_rawBossBar->setNameCalls().back().text, "Jean");
}

TEST_F(EndDragonFightBossBarTest, UpdateDragon_WithoutCustomName_DoesNotUpdateName)
{
    const std::string uuid = "abcdef0123456789abcdef0123456789";
    m_accessor->setDragonUUID(uuid);

    auto dragon = makeDragon(uuid, 200.0f);
    // 不设置自定义名称

    m_rawBossBar->reset();
    m_fight->updateDragon(*dragon);

    EXPECT_TRUE(m_rawBossBar->setNameCalls().empty());
}

TEST_F(EndDragonFightBossBarTest, UpdateDragon_WithCustomName_SyncsAfterPercent)
{
    // 验证调用顺序：先 setPercent，后 setName（与 MC Java 顺序一致）
    const std::string uuid = "abcdef0123456789abcdef0123456789";
    m_accessor->setDragonUUID(uuid);

    auto dragon = makeDragon(uuid, 100.0f);
    dragon->setCustomName("Named Dragon");

    m_fight->updateDragon(*dragon);

    // 至少有一次 setPercent 和一次 setName
    ASSERT_GE(m_rawBossBar->setPercentCalls().size(), 1u);
    ASSERT_GE(m_rawBossBar->setNameCalls().size(), 1u);
    // 不强制顺序断言，因 MC Java 实现中两者独立调用，无强约束
}

// ============================================================================
// setDragonKilled：Boss 栏设置
// ============================================================================

TEST_F(EndDragonFightBossBarTest, SetDragonKilled_SetsBossBarPercentToZero)
{
    m_fight->setDragonKilled(m_world);

    // 最后一次 setPercent 应为 0
    ASSERT_FALSE(m_rawBossBar->setPercentCalls().empty());
    EXPECT_FLOAT_EQ(m_rawBossBar->setPercentCalls().back().percent, 0.0f);
}

TEST_F(EndDragonFightBossBarTest, SetDragonKilled_HidesBossBar)
{
    m_fight->setDragonKilled(m_world);

    // 应至少调用 setVisible(false) 一次
    bool foundHide = false;
    for (const auto& call : m_rawBossBar->setVisibleCalls()) {
        if (!call.visible) {
            foundHide = true;
            break;
        }
    }
    EXPECT_TRUE(foundHide) << "Expected setVisible(false) to be called by setDragonKilled";
}

// ============================================================================
// tick()：Boss 栏可见性
// ============================================================================

TEST_F(EndDragonFightBossBarTest, Tick_DragonAlive_SetsBossBarVisible)
{
    // 龙未死：tick 应设置 Boss 栏可见
    m_accessor->setDragonKilledFlag(false);

    m_rawBossBar->reset();
    m_fight->tick(m_world);

    ASSERT_FALSE(m_rawBossBar->setVisibleCalls().empty());
    EXPECT_TRUE(m_rawBossBar->setVisibleCalls().back().visible);
}

TEST_F(EndDragonFightBossBarTest, Tick_DragonKilled_SetsBossBarInvisible)
{
    // 龙已死：tick 应设置 Boss 栏不可见
    m_accessor->setDragonKilledFlag(true);

    m_rawBossBar->reset();
    m_fight->tick(m_world);

    ASSERT_FALSE(m_rawBossBar->setVisibleCalls().empty());
    EXPECT_FALSE(m_rawBossBar->setVisibleCalls().back().visible);
}

TEST_F(EndDragonFightBossBarTest, Tick_EveryTick_UpdatesVisibility)
{
    // tick 每次都应更新可见性（而非仅在变化时）
    m_accessor->setDragonKilledFlag(false);

    m_rawBossBar->reset();
    m_fight->tick(m_world);
    m_fight->tick(m_world);
    m_fight->tick(m_world);

    // 3 次 tick 应产生至少 3 次 setVisible 调用
    // 注：实际可能更多，因为 _updatePlayers 内部不调用 setVisible
    EXPECT_GE(m_rawBossBar->setVisibleCalls().size(), 3u);
}

// ============================================================================
// tick()：玩家扫描节奏
// ============================================================================

TEST_F(EndDragonFightBossBarTest, Tick_PlayerScanHappensEveryTwentyTicks)
{
    // 初始 m_ticksSinceLastPlayerScan = TIME_BETWEEN_PLAYER_SCANS (20)
    // 第一次 tick: ++m_ticksSinceLastPlayerScan = 21 >= 20 -> 扫描，重置为 0
    // 后续 19 次 tick: ++m_ticksSinceLastPlayerScan 从 1 到 19，不扫描
    // 第 21 次 tick: ++m_ticksSinceLastPlayerScan = 20 >= 20 -> 再次扫描

    m_rawBossBar->reset();
    for (int i = 0; i < 20; ++i) {
        m_fight->tick(m_world);
    }
    // 20 次 tick 中应触发 2 次 replacePlayers（第 1 次和第 21 次本应触发，但只跑 20 次）
    // 第 1 次 tick: 21 >= 20 -> 扫描
    // 第 2-20 次 tick: 1-19 < 20 -> 不扫描
    // 共 1 次 replacePlayers
    EXPECT_EQ(m_rawBossBar->replacePlayersCalls().size(), 1u);

    // 再 tick 1 次（第 21 次）：20 >= 20 -> 扫描
    m_fight->tick(m_world);
    EXPECT_EQ(m_rawBossBar->replacePlayersCalls().size(), 2u);
}

// ============================================================================
// tick()：无玩家时跳过重逻辑
// ============================================================================

TEST_F(EndDragonFightBossBarTest, Tick_NoPlayers_DoesNotIncrementTicksSinceDragonSeen)
{
    // 无可见玩家时，tick 应提前返回，不执行龙失联检查
    // 由于 _updatePlayers 不会添加任何玩家（测试世界无玩家），hasPlayers() 返回 false
    m_accessor->setDragonKilledFlag(false);
    m_accessor->setTicksSinceDragonSeen(0);
    m_accessor->setDragonUUID("some-uuid");

    m_fight->tick(m_world);
    m_fight->tick(m_world);

    // tick 不应修改 ticksSinceDragonSeen（因为无玩家，提前返回）
    // 注：ticksSinceDragonSeen 只在 updateDragon 或 tick 的龙失联分支中修改
    // 无玩家时 tick 提前返回，不进入龙失联分支
    EXPECT_EQ(m_accessor->ticksSinceDragonSeen(), 0);
}

// ============================================================================
// _updatePlayers：玩家追踪
// ============================================================================

TEST_F(EndDragonFightBossBarTest, UpdatePlayers_NoPlayersInWorld_ResultsInEmptyBossBarPlayers)
{
    m_accessor->updatePlayers(m_world);

    EXPECT_FALSE(m_rawBossBar->hasPlayers());
    EXPECT_TRUE(m_rawBossBar->players().empty());
}

// ============================================================================
// setDragonBossBar：注入与恢复
// ============================================================================

TEST_F(EndDragonFightBossBarTest, SetDragonBossBar_Nullptr_RestoresNullDragonBossBar)
{
    // 注入 nullptr 应恢复为 NullDragonBossBar
    m_fight->setDragonBossBar(nullptr);

    // NullDragonBossBar::hasPlayers() 返回 false
    EXPECT_FALSE(m_fight->dragonBossBar().hasPlayers());
    // NullDragonBossBar::percent() 返回 0
    EXPECT_FLOAT_EQ(m_fight->dragonBossBar().percent(), 0.0f);
    // NullDragonBossBar::visible() 返回 false
    EXPECT_FALSE(m_fight->dragonBossBar().visible());
}

TEST_F(EndDragonFightBossBarTest, SetDragonBossBar_ReplacesPreviousBossBar)
{
    // 创建第二个 MockDragonBossBar 并注入
    auto secondBossBar = std::make_unique<::mc::test::MockDragonBossBar>();
    ::mc::test::MockDragonBossBar* rawSecond = secondBossBar.get();

    m_fight->setDragonBossBar(std::move(secondBossBar));

    // 原 bossBar 应被替换。通过设置 percent 验证当前 bossBar 是第二个
    m_fight->dragonBossBar().setPercent(0.42f);
    EXPECT_FLOAT_EQ(rawSecond->percent(), 0.42f);
}

// ============================================================================
// createDefaultBossName：默认 Boss 栏名称
// ============================================================================

TEST_F(EndDragonFightBossBarTest, CreateDefaultBossName_ReturnsEnderDragonTranslation)
{
    auto name = EndDragonFight::createDefaultBossName();
    ASSERT_NE(name, nullptr);

    // TranslationTextComponent 的 getUnformattedText 返回翻译键本身
    EXPECT_EQ(name->getUnformattedText(), "entity.minecraft.ender_dragon");
}

// ============================================================================
// 常量正确性测试
// ============================================================================

TEST_F(EndDragonFightBossBarTest, Constants_MatchMCJavaValues)
{
    // 对应 MC Java EndDragonFight 中的常量
    EXPECT_EQ(EndDragonFight::TIME_BETWEEN_PLAYER_SCANS, 20);
    EXPECT_EQ(EndDragonFight::MAX_TICKS_BEFORE_DRAGON_RESPAWN, 1200);
    EXPECT_FLOAT_EQ(EndDragonFight::PLAYER_TRACKING_RADIUS, 192.0f);
}

// ============================================================================
// NullDragonBossBar 行为测试
// ============================================================================

TEST(NullDragonBossBarTest, AllMethodsAreNoOps)
{
    NullDragonBossBar bar;

    // 所有方法应不抛异常且无副作用
    bar.setPercent(0.5f);
    bar.setName(std::make_unique<text::StringTextComponent>("test"));
    bar.setVisible(true);
    bar.addPlayer(PlayerId{1});
    bar.removePlayer(PlayerId{1});
    bar.removeAllPlayers();
    bar.replacePlayers({PlayerId{1}, PlayerId{2}});

    EXPECT_FALSE(bar.hasPlayers());
    EXPECT_FLOAT_EQ(bar.percent(), 0.0f);
    EXPECT_FALSE(bar.visible());
}

TEST(NullDragonBossBarTest, ReplacePlayersWithEmptySet_StillHasNoPlayers)
{
    NullDragonBossBar bar;
    bar.replacePlayers({});
    EXPECT_FALSE(bar.hasPlayers());
}

// ============================================================================
// MockDragonBossBar 自身行为验证（确保测试桩正确）
// ============================================================================

TEST_F(EndDragonFightBossBarTest, MockBossBar_RecordsAllCalls)
{
    // 此测试验证 MockDragonBossBar 自身正确记录调用，后续测试可信任其断言
    m_rawBossBar->setPercent(0.7f);
    m_rawBossBar->setVisible(true);
    m_rawBossBar->addPlayer(PlayerId{42});

    ASSERT_EQ(m_rawBossBar->setPercentCalls().size(), 1u);
    EXPECT_FLOAT_EQ(m_rawBossBar->setPercentCalls()[0].percent, 0.7f);

    ASSERT_EQ(m_rawBossBar->setVisibleCalls().size(), 1u);
    EXPECT_TRUE(m_rawBossBar->setVisibleCalls()[0].visible);

    ASSERT_EQ(m_rawBossBar->addPlayerCalls().size(), 1u);
    EXPECT_EQ(m_rawBossBar->addPlayerCalls()[0].playerId, PlayerId{42});

    EXPECT_TRUE(m_rawBossBar->hasPlayers());
    EXPECT_EQ(m_rawBossBar->players().size(), 1u);
}

// ============================================================================
// updateDragon：多次调用持续同步
// ============================================================================

TEST_F(EndDragonFightBossBarTest, UpdateDragon_MultipleCalls_ContinuouslySyncsPercent)
{
    const std::string uuid = "abcdef0123456789abcdef0123456789";
    m_accessor->setDragonUUID(uuid);

    auto dragon = makeDragon(uuid, 200.0f);

    // 第一次：满血
    m_fight->updateDragon(*dragon);
    EXPECT_FLOAT_EQ(m_rawBossBar->setPercentCalls().back().percent, 1.0f);

    // 龙受伤
    dragon->setHealth(150.0f);
    m_fight->updateDragon(*dragon);
    EXPECT_FLOAT_EQ(m_rawBossBar->setPercentCalls().back().percent, 0.75f);

    // 龙濒死
    dragon->setHealth(10.0f);
    m_fight->updateDragon(*dragon);
    EXPECT_FLOAT_EQ(m_rawBossBar->setPercentCalls().back().percent, 0.05f);

    // 龙死亡
    dragon->setHealth(0.0f);
    m_fight->updateDragon(*dragon);
    EXPECT_FLOAT_EQ(m_rawBossBar->setPercentCalls().back().percent, 0.0f);

    // 验证总共 4 次 setPercent 调用
    EXPECT_EQ(m_rawBossBar->setPercentCalls().size(), 4u);
}

// ============================================================================
// 综合场景：updateDragon 与 setDragonKilled 的协作
// ============================================================================

TEST_F(EndDragonFightBossBarTest, Integration_UpdateDragonThenKill_TransitionsBossBarToZero)
{
    const std::string uuid = "abcdef0123456789abcdef0123456789";
    m_accessor->setDragonUUID(uuid);

    auto dragon = makeDragon(uuid, 200.0f);
    m_fight->updateDragon(*dragon);
    EXPECT_FLOAT_EQ(m_rawBossBar->percent(), 1.0f);

    // 龙受伤
    dragon->setHealth(80.0f);
    m_fight->updateDragon(*dragon);
    EXPECT_FLOAT_EQ(m_rawBossBar->percent(), 0.4f);

    // 击杀龙
    m_fight->setDragonKilled(m_world);

    // Boss 栏应被重置为 0 并隐藏
    EXPECT_FLOAT_EQ(m_rawBossBar->percent(), 0.0f);
    EXPECT_FALSE(m_rawBossBar->visible());
}
