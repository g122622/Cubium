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
 * IMPLIED, INCLUDING WITHOUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file ServerPlayerPvpTest.cpp
 * @brief ServerPlayer PvP 保护机制单元测试
 *
 * 测试 ServerPlayer 级别的 PvP 保护逻辑：
 * - ServerPlayer::canHarmPlayer() 在 PvP 游戏规则关闭时阻止伤害
 * - ServerPlayer::hurt() 拦截玩家来源伤害时的 PvP 检查
 * - 间接伤害来源（箭矢）的 PvP 检查
 * - 非玩家伤害来源不受 PvP 规则影响
 *
 * 由于 ServerPlayer::getTeam() 依赖服务器记分板系统，
 * 测试使用 TestServerPlayer 子类重写 getTeam() 以注入模拟队伍。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"

using namespace mc;
using namespace mc::world::gamerule;

// ============================================================================
// 测试用 Mock Team 类
// ============================================================================

class PvpMockTeam : public scoreboard::Team {
public:
    explicit PvpMockTeam(const std::string& name, bool allowFriendlyFire = true)
        : m_name(name)
        , m_allowFriendlyFire(allowFriendlyFire)
    {}

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }

    [[nodiscard]] const text::ITextComponent* getDisplayName() const noexcept override { return nullptr; }
    void setDisplayName(std::unique_ptr<text::ITextComponent>) override {}

    [[nodiscard]] const std::set<std::string>& getMembers() const noexcept override
    {
        static std::set<std::string> empty;
        return empty;
    }
    bool addMember(const std::string&) override { return false; }
    bool removeMember(const std::string&) override { return false; }
    [[nodiscard]] bool hasMember(const std::string&) const override { return false; }
    void clearMembers() override {}

    [[nodiscard]] text::TextFormatting getColor() const noexcept override { return text::TextFormatting::White; }
    void setColor(text::TextFormatting) override {}

    [[nodiscard]] const text::ITextComponent* getPrefix() const noexcept override { return nullptr; }
    void setPrefix(std::unique_ptr<text::ITextComponent>) override {}
    [[nodiscard]] const text::ITextComponent* getSuffix() const noexcept override { return nullptr; }
    void setSuffix(std::unique_ptr<text::ITextComponent>) override {}

    [[nodiscard]] bool getAllowFriendlyFire() const noexcept override { return m_allowFriendlyFire; }
    void setAllowFriendlyFire(bool allow) override { m_allowFriendlyFire = allow; }

    [[nodiscard]] bool canSeeFriendlyInvisibles() const noexcept override { return false; }
    void setSeeFriendlyInvisibles(bool) override {}

    [[nodiscard]] scoreboard::TeamVisibility getNameTagVisibility() const noexcept override
    {
        return scoreboard::TeamVisibility::Always;
    }
    void setNameTagVisibility(scoreboard::TeamVisibility) override {}

    [[nodiscard]] scoreboard::TeamVisibility getDeathMessageVisibility() const noexcept override
    {
        return scoreboard::TeamVisibility::Always;
    }
    void setDeathMessageVisibility(scoreboard::TeamVisibility) override {}

    [[nodiscard]] scoreboard::TeamCollisionRule getCollisionRule() const noexcept override
    {
        return scoreboard::TeamCollisionRule::Always;
    }
    void setCollisionRule(scoreboard::TeamCollisionRule) override {}

    [[nodiscard]] std::unique_ptr<text::ITextComponent> formatName(const text::ITextComponent&) const override
    {
        return nullptr;
    }

    [[nodiscard]] std::unique_ptr<text::ITextComponent> getFormattedDisplayName() const override { return nullptr; }

private:
    std::string m_name;
    bool m_allowFriendlyFire;
};

// ============================================================================
// 测试用 TestServerPlayer 类（重写 getTeam 以注入模拟队伍）
// ============================================================================

class TestServerPlayer : public ServerPlayer {
public:
    explicit TestServerPlayer(EntityInstanceId id, const std::string& name)
        : ServerPlayer(id, name, mc::test::testEcsRegistry())
    {}

    void setMockTeam(scoreboard::Team* team) { m_mockTeam = team; }

    [[nodiscard]] scoreboard::Team* getTeam() override { return m_mockTeam; }
    [[nodiscard]] const scoreboard::Team* getTeam() const override { return m_mockTeam; }

private:
    scoreboard::Team* m_mockTeam = nullptr;
};

// ============================================================================
// ServerPlayer::canHarmPlayer 测试
// ============================================================================

class ServerPlayerCanHarmPlayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建 ServerWorld（默认构造，PvP 游戏规则默认为 true）
        m_world = std::make_unique<mc::server::ServerWorld>(mc::server::ServerWorldConfig{});

        // 创建 TestServerPlayer 并设置世界
        m_attacker = std::make_unique<TestServerPlayer>(EntityInstanceId(1), "Attacker");
        m_target = std::make_unique<TestServerPlayer>(EntityInstanceId(2), "Target");
        m_attacker->setWorld(m_world.get());
        m_target->setWorld(m_world.get());

        // 创建队伍
        m_teamFriendlyFireOn = std::make_unique<PvpMockTeam>("red", true);
        m_teamFriendlyFireOff = std::make_unique<PvpMockTeam>("blue", false);
        m_teamOther = std::make_unique<PvpMockTeam>("green", true);
    }

    void TearDown() override
    {
        m_target.reset();
        m_attacker.reset();
        m_teamOther.reset();
        m_teamFriendlyFireOff.reset();
        m_teamFriendlyFireOn.reset();
        m_world.reset();
    }

    /**
     * @brief 设置 PvP 游戏规则
     */
    void setPvpEnabled(bool enabled) { m_world->getGameRules().setBoolean(GameRuleKeys::PVP, enabled, nullptr); }

    std::unique_ptr<mc::server::ServerWorld> m_world;
    std::unique_ptr<TestServerPlayer> m_attacker;
    std::unique_ptr<TestServerPlayer> m_target;
    std::unique_ptr<PvpMockTeam> m_teamFriendlyFireOn;
    std::unique_ptr<PvpMockTeam> m_teamFriendlyFireOff;
    std::unique_ptr<PvpMockTeam> m_teamOther;
};

// ---------- PvP 游戏规则测试 ----------

TEST_F(ServerPlayerCanHarmPlayerTest, PvpEnabled_NoTeam_CanHarm)
{
    // PvP 开启，双方无队伍，可以伤害
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));
}

TEST_F(ServerPlayerCanHarmPlayerTest, PvpDisabled_NoTeam_CannotHarm)
{
    // PvP 关闭，即使双方无队伍，也不可以伤害
    setPvpEnabled(false);
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_target));
}

TEST_F(ServerPlayerCanHarmPlayerTest, PvpEnabled_SameTeam_FriendlyFireOn_CanHarm)
{
    // PvP 开启，同队，友伤开启，可以伤害
    m_attacker->setMockTeam(m_teamFriendlyFireOn.get());
    m_target->setMockTeam(m_teamFriendlyFireOn.get());
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));
}

TEST_F(ServerPlayerCanHarmPlayerTest, PvpDisabled_SameTeam_FriendlyFireOn_CannotHarm)
{
    // PvP 关闭，即使同队友伤开启，也不可以伤害
    m_attacker->setMockTeam(m_teamFriendlyFireOn.get());
    m_target->setMockTeam(m_teamFriendlyFireOn.get());
    setPvpEnabled(false);
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_target));
}

TEST_F(ServerPlayerCanHarmPlayerTest, PvpEnabled_SameTeam_FriendlyFireOff_CannotHarm)
{
    // PvP 开启，同队，友伤关闭，不可以伤害
    m_attacker->setMockTeam(m_teamFriendlyFireOff.get());
    m_target->setMockTeam(m_teamFriendlyFireOff.get());
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_target));
}

TEST_F(ServerPlayerCanHarmPlayerTest, PvpDisabled_SameTeam_FriendlyFireOff_CannotHarm)
{
    // PvP 关闭，同队友伤关闭，当然不可以伤害
    m_attacker->setMockTeam(m_teamFriendlyFireOff.get());
    m_target->setMockTeam(m_teamFriendlyFireOff.get());
    setPvpEnabled(false);
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_target));
}

TEST_F(ServerPlayerCanHarmPlayerTest, PvpEnabled_DifferentTeams_CanHarm)
{
    // PvP 开启，不同队伍，可以伤害
    m_attacker->setMockTeam(m_teamFriendlyFireOn.get());
    m_target->setMockTeam(m_teamOther.get());
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));
}

TEST_F(ServerPlayerCanHarmPlayerTest, PvpDisabled_DifferentTeams_CannotHarm)
{
    // PvP 关闭，即使不同队伍，也不可以伤害
    m_attacker->setMockTeam(m_teamFriendlyFireOn.get());
    m_target->setMockTeam(m_teamOther.get());
    setPvpEnabled(false);
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_target));
}

// ---------- 自身攻击测试 ----------

TEST_F(ServerPlayerCanHarmPlayerTest, SelfHarm_PvpEnabled_NoTeam_CanHarm)
{
    // PvP 开启，无队伍，自己可以伤害自己
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_attacker));
}

TEST_F(ServerPlayerCanHarmPlayerTest, SelfHarm_PvpDisabled_CannotHarm)
{
    // PvP 关闭，自己不可以伤害自己
    setPvpEnabled(false);
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_attacker));
}

TEST_F(ServerPlayerCanHarmPlayerTest, SelfHarm_PvpEnabled_SameTeam_FriendlyFireOff_CannotHarm)
{
    // PvP 开启，同队友伤关闭，自己不能伤害自己
    m_attacker->setMockTeam(m_teamFriendlyFireOff.get());
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_attacker));
}

// ---------- PvP 规则切换测试 ----------

TEST_F(ServerPlayerCanHarmPlayerTest, TogglePvpRule)
{
    // 初始 PvP 开启
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));

    // 关闭 PvP
    setPvpEnabled(false);
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_target));

    // 重新开启 PvP
    setPvpEnabled(true);
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));
}

// ---------- 无世界指针时的行为 ----------

TEST_F(ServerPlayerCanHarmPlayerTest, NullWorld_FallsBackToBaseClass)
{
    // 当世界指针为空时，跳过 PvP 规则检查，委托给基类
    auto player = std::make_unique<TestServerPlayer>(EntityInstanceId(3), "NoWorldPlayer");
    // 不设置世界，m_world 为 nullptr
    // 基类 Player::canHarmPlayer 无队伍时返回 true
    EXPECT_TRUE(player->canHarmPlayer(*m_target));

    // 即使 PvP 关闭，无世界指针时不检查 PvP 规则
    setPvpEnabled(false);
    EXPECT_TRUE(player->canHarmPlayer(*m_target));

    // 同队友伤关闭时仍然受基类队伍检查
    player->setMockTeam(m_teamFriendlyFireOff.get());
    m_target->setMockTeam(m_teamFriendlyFireOff.get());
    EXPECT_FALSE(player->canHarmPlayer(*m_target));
}

// ---------- PvP 关闭 + 队友检查的优先级 ----------

TEST_F(ServerPlayerCanHarmPlayerTest, PvpDisabledTakesPrecedenceOverTeamRules)
{
    // PvP 关闭时，无论队伍配置如何，都不能伤害
    // 攻击者有队伍，目标没有队伍
    setPvpEnabled(false);
    m_attacker->setMockTeam(m_teamFriendlyFireOn.get());
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_target));

    // 两个不同队伍
    m_attacker->setMockTeam(m_teamFriendlyFireOn.get());
    m_target->setMockTeam(m_teamOther.get());
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_target));
}

// ============================================================================
// ServerPlayer::hurt() PvP 拦截测试
// ============================================================================

class ServerPlayerHurtPvpTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<mc::server::ServerWorld>(mc::server::ServerWorldConfig{});

        m_attacker = std::make_unique<TestServerPlayer>(EntityInstanceId(1), "Attacker");
        m_target = std::make_unique<TestServerPlayer>(EntityInstanceId(2), "Target");
        m_attacker->setWorld(m_world.get());
        m_target->setWorld(m_world.get());

        m_teamFriendlyFireOff = std::make_unique<PvpMockTeam>("blue", false);
    }

    void TearDown() override
    {
        m_target.reset();
        m_attacker.reset();
        m_teamFriendlyFireOff.reset();
        m_world.reset();
    }

    void setPvpEnabled(bool enabled) { m_world->getGameRules().setBoolean(GameRuleKeys::PVP, enabled, nullptr); }

    std::unique_ptr<mc::server::ServerWorld> m_world;
    std::unique_ptr<TestServerPlayer> m_attacker;
    std::unique_ptr<TestServerPlayer> m_target;
    std::unique_ptr<PvpMockTeam> m_teamFriendlyFireOff;
};

// ---------- 玩家攻击来源 PvP 检查 ----------

TEST_F(ServerPlayerHurtPvpTest, PlayerAttack_PvpEnabled_AllowsDamage)
{
    // PvP 开启，玩家攻击玩家，允许伤害
    setPvpEnabled(true);
    auto source = DamageSources::playerAttack(m_attacker.get());
    // 注意：Player::hurt() 还有创造模式无敌检查，
    // 在非创造模式下（默认），伤害应该被允许通过 PvP 检查
    bool result = m_target->hurt(source, 5.0f);
    EXPECT_TRUE(result);
}

TEST_F(ServerPlayerHurtPvpTest, PlayerAttack_PvpDisabled_BlocksDamage)
{
    // PvP 关闭，玩家攻击玩家，伤害被拦截
    setPvpEnabled(false);
    auto source = DamageSources::playerAttack(m_attacker.get());
    bool result = m_target->hurt(source, 5.0f);
    EXPECT_FALSE(result);
}

TEST_F(ServerPlayerHurtPvpTest, PlayerAttack_SameTeamFriendlyFireOff_BlocksDamage)
{
    // PvP 开启但同队友伤关闭
    setPvpEnabled(true);
    m_attacker->setMockTeam(m_teamFriendlyFireOff.get());
    m_target->setMockTeam(m_teamFriendlyFireOff.get());
    auto source = DamageSources::playerAttack(m_attacker.get());
    bool result = m_target->hurt(source, 5.0f);
    EXPECT_FALSE(result);
}

TEST_F(ServerPlayerHurtPvpTest, PlayerAttack_SelfDamage_NotBlockedByPvp)
{
    // 玩家攻击自己（self-harm），PvP 检查中 attackingPlayer != this 会跳过
    // ServerPlayer::hurt() 中 self-damage 不受 PvP 保护
    setPvpEnabled(false);
    auto source = DamageSources::playerAttack(m_target.get());
    // attackingPlayer == this，所以 PvP 检查被跳过
    // Player::hurt() 的创造模式检查可能阻止，非创造模式下应通过
    bool result = m_target->hurt(source, 5.0f);
    EXPECT_TRUE(result);
}

// ---------- 非玩家伤害来源不受 PvP 影响 ----------

TEST_F(ServerPlayerHurtPvpTest, EnvironmentalDamage_NotAffectedByPvp)
{
    // 环境伤害（非玩家来源）不受 PvP 规则影响
    setPvpEnabled(false);
    auto source = DamageSources::generic();
    bool result = m_target->hurt(source, 5.0f);
    EXPECT_TRUE(result);
}

TEST_F(ServerPlayerHurtPvpTest, FallDamage_NotAffectedByPvp)
{
    // 摔落伤害不受 PvP 规则影响
    setPvpEnabled(false);
    auto source = DamageSources::fall();
    bool result = m_target->hurt(source, 5.0f);
    EXPECT_TRUE(result);
}

// ---------- 间接玩家伤害来源 PvP 检查 ----------

TEST_F(ServerPlayerHurtPvpTest, IndirectPlayerDamage_PvpDisabled_BlocksDamage)
{
    // 间接伤害（箭矢等），来源是玩家，PvP 关闭时应被拦截
    setPvpEnabled(false);
    // IndirectEntityDamageSource::getEntity() 返回射击者（source）
    auto source = DamageSources::arrow(nullptr, m_attacker.get(), true);
    bool result = m_target->hurt(source, 5.0f);
    EXPECT_FALSE(result);
}

TEST_F(ServerPlayerHurtPvpTest, IndirectPlayerDamage_PvpEnabled_AllowsDamage)
{
    // 间接伤害（箭矢等），来源是玩家，PvP 开启时允许伤害
    setPvpEnabled(true);
    auto source = DamageSources::arrow(nullptr, m_attacker.get(), true);
    bool result = m_target->hurt(source, 5.0f);
    EXPECT_TRUE(result);
}

// ---------- 无世界指针时的 hurt 行为 ----------

TEST_F(ServerPlayerHurtPvpTest, NullWorld_PlayerAttack_AllowsDamage)
{
    // 攻击者无世界指针时，canHarmPlayer 委托给基类（无 PvP 规则检查）
    auto attackerNoWorld = std::make_unique<TestServerPlayer>(EntityInstanceId(3), "NoWorldAttacker");
    // 不设置世界，m_world 为 nullptr
    auto source = DamageSources::playerAttack(attackerNoWorld.get());
    bool result = m_target->hurt(source, 5.0f);
    // 无世界指针时，canHarmPlayer 使用基类逻辑（无队伍=允许）
    EXPECT_TRUE(result);
}

// ---------- Mob 攻击不受 PvP 影响 ----------

TEST_F(ServerPlayerHurtPvpTest, MobAttack_NotAffectedByPvp)
{
    // 非玩家实体攻击（EntityDamageSource 但 source 不是 Player）
    // dynamic_cast<Player*> 会返回 nullptr，不触发 PvP 检查
    setPvpEnabled(false);
    // 使用 nullptr 作为 source 实体，模拟非玩家伤害
    EntityDamageSource mobSource(DamageType::MobAttack, nullptr);
    bool result = m_target->hurt(mobSource, 5.0f);
    // 非 Player 来源不受 PvP 保护
    EXPECT_TRUE(result);
}

// ---------- PvP 关闭 + 友伤关闭双重阻止测试 ----------

TEST_F(ServerPlayerHurtPvpTest, PvpDisabledAndFriendlyFireOff_BothBlockDamage)
{
    // PvP 关闭且友伤关闭，双重阻止
    setPvpEnabled(false);
    m_attacker->setMockTeam(m_teamFriendlyFireOff.get());
    m_target->setMockTeam(m_teamFriendlyFireOff.get());
    auto source = DamageSources::playerAttack(m_attacker.get());
    bool result = m_target->hurt(source, 5.0f);
    EXPECT_FALSE(result);
}

// ---------- 多态调用验证 ----------

TEST_F(ServerPlayerHurtPvpTest, PolymorphicCall_ThroughPlayerPointer)
{
    // 通过 Player* 指针调用 hurt()，验证 ServerPlayer 的重写被正确调用
    setPvpEnabled(false);
    Player* attackerPtr = m_attacker.get();
    Player* targetPtr = m_target.get();
    auto source = DamageSources::playerAttack(attackerPtr);
    bool result = targetPtr->hurt(source, 5.0f);
    // ServerPlayer::hurt() 应该拦截 PvP 伤害
    EXPECT_FALSE(result);
}
