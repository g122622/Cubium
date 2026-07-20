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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file PlayerCanHarmPlayerTest.cpp
 * @brief Player::canHarmPlayer 和 PvP 保护机制单元测试
 *
 * 测试 Player::canHarmPlayer 在不同队伍配置和友伤设置下的行为：
 * - 无队伍玩家之间可以互相伤害
 * - 同队玩家取决于友伤设置
 * - 不同队伍玩家之间可以互相伤害
 */

#include <gtest/gtest.h>

#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/world/gamerule/GameRules.hpp"

using namespace mc;
using namespace mc::world::gamerule;

// ============================================================================
// 测试用 Mock Team 类
// ============================================================================

class CanHarmPlayerMockTeam : public scoreboard::Team {
public:
    explicit CanHarmPlayerMockTeam(const std::string& name, bool allowFriendlyFire = true)
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
// 测试用 Mock Player 类（支持设置队伍）
// ============================================================================

class CanHarmPlayerMockPlayer : public Player {
public:
    explicit CanHarmPlayerMockPlayer(const std::string& name = "TestPlayer")
        : Player(EntityInstanceId(1), name)
    {}

    void setMockTeam(scoreboard::Team* team) { m_mockTeam = team; }

    [[nodiscard]] scoreboard::Team* getTeam() override { return m_mockTeam; }
    [[nodiscard]] const scoreboard::Team* getTeam() const override { return m_mockTeam; }

private:
    scoreboard::Team* m_mockTeam = nullptr;
};

// ============================================================================
// Player::canHarmPlayer 测试
// ============================================================================

class PlayerCanHarmPlayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_teamFriendlyFireOn = std::make_unique<CanHarmPlayerMockTeam>("red", true);
        m_teamFriendlyFireOff = std::make_unique<CanHarmPlayerMockTeam>("blue", false);
        m_teamOther = std::make_unique<CanHarmPlayerMockTeam>("green", true);
        m_attacker = std::make_unique<CanHarmPlayerMockPlayer>("Attacker");
        m_target = std::make_unique<CanHarmPlayerMockPlayer>("Target");
    }

    void TearDown() override
    {
        m_target.reset();
        m_attacker.reset();
        m_teamOther.reset();
        m_teamFriendlyFireOff.reset();
        m_teamFriendlyFireOn.reset();
    }

    std::unique_ptr<CanHarmPlayerMockTeam> m_teamFriendlyFireOn;
    std::unique_ptr<CanHarmPlayerMockTeam> m_teamFriendlyFireOff;
    std::unique_ptr<CanHarmPlayerMockTeam> m_teamOther;
    std::unique_ptr<CanHarmPlayerMockPlayer> m_attacker;
    std::unique_ptr<CanHarmPlayerMockPlayer> m_target;
};

// ============================================================================
// 无队伍情况
// ============================================================================

TEST_F(PlayerCanHarmPlayerTest, NoTeam_CanHarm)
{
    // 攻击者和目标都没有队伍，可以互相伤害
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));
}

TEST_F(PlayerCanHarmPlayerTest, AttackerNoTeam_CanHarmTargetWithTeam)
{
    // 攻击者没有队伍，目标有队伍，可以伤害
    m_target->setMockTeam(m_teamFriendlyFireOn.get());
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));
}

TEST_F(PlayerCanHarmPlayerTest, TargetNoTeam_CanHarmTarget)
{
    // 攻击者有队伍，目标没有队伍，可以伤害
    m_attacker->setMockTeam(m_teamFriendlyFireOn.get());
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));
}

// ============================================================================
// 同队情况
// ============================================================================

TEST_F(PlayerCanHarmPlayerTest, SameTeam_FriendlyFireOn_CanHarm)
{
    // 同队且允许友伤，可以伤害
    m_attacker->setMockTeam(m_teamFriendlyFireOn.get());
    m_target->setMockTeam(m_teamFriendlyFireOn.get());
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));
}

TEST_F(PlayerCanHarmPlayerTest, SameTeam_FriendlyFireOff_CannotHarm)
{
    // 同队且不允许友伤，不可以伤害
    m_attacker->setMockTeam(m_teamFriendlyFireOff.get());
    m_target->setMockTeam(m_teamFriendlyFireOff.get());
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_target));
}

// ============================================================================
// 不同队伍情况
// ============================================================================

TEST_F(PlayerCanHarmPlayerTest, DifferentTeams_CanHarmRegardlessOfFriendlyFire)
{
    // 不同队伍，无论友伤设置如何，都可以伤害
    m_attacker->setMockTeam(m_teamFriendlyFireOn.get());
    m_target->setMockTeam(m_teamFriendlyFireOff.get());
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));
}

TEST_F(PlayerCanHarmPlayerTest, DifferentTeams_BothFriendlyFireOff_CanHarm)
{
    // 不同队伍，即使双方都不允许友伤，也可以伤害
    m_attacker->setMockTeam(m_teamFriendlyFireOff.get());
    m_target->setMockTeam(m_teamOther.get());
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));
}

// ============================================================================
// 友伤设置切换测试
// ============================================================================

TEST_F(PlayerCanHarmPlayerTest, SameTeam_ToggleFriendlyFire)
{
    m_attacker->setMockTeam(m_teamFriendlyFireOff.get());
    m_target->setMockTeam(m_teamFriendlyFireOff.get());

    // 友伤关闭时不可以伤害
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_target));

    // 开启友伤后可以伤害
    m_teamFriendlyFireOff->setAllowFriendlyFire(true);
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_target));

    // 再次关闭友伤
    m_teamFriendlyFireOff->setAllowFriendlyFire(false);
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_target));
}

// ============================================================================
// 自身攻击测试（攻击者和目标相同）
// ============================================================================

TEST_F(PlayerCanHarmPlayerTest, SelfHarm_NoTeam_CanHarm)
{
    // 没有队伍时，自己可以伤害自己
    EXPECT_TRUE(m_attacker->canHarmPlayer(*m_attacker));
}

TEST_F(PlayerCanHarmPlayerTest, SelfHarm_SameTeam_FriendlyFireOff_CannotHarm)
{
    // 同队且友伤关闭时，自己不能伤害自己
    m_attacker->setMockTeam(m_teamFriendlyFireOff.get());
    EXPECT_FALSE(m_attacker->canHarmPlayer(*m_attacker));
}

// ============================================================================
// PVP 游戏规则默认值测试
// ============================================================================

TEST(PvpGameRuleIntegrationTest, PvpRuleKeyAccessibleViaGameRulesContainer)
{
    GameRules rules;
    // 默认值应为 true
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::PVP));

    // 可以修改
    rules.setBoolean(GameRuleKeys::PVP, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::PVP));
}
