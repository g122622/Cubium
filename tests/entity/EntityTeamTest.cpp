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

#include "common/scoreboard/core/Team.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/MobEntity.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试用 Mock Team 类
// ============================================================================

class MockTeam : public scoreboard::Team {
public:
    explicit MockTeam(const std::string& name)
        : m_name(name)
        , m_allowFriendlyFire(true)
        , m_seeFriendlyInvisibles(false)
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

    [[nodiscard]] bool canSeeFriendlyInvisibles() const noexcept override { return m_seeFriendlyInvisibles; }
    void setSeeFriendlyInvisibles(bool see) override { m_seeFriendlyInvisibles = see; }

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
    bool m_seeFriendlyInvisibles;
};

// ============================================================================
// 测试用 Mock Entity 类（支持队伍）
// ============================================================================

class MockEntityWithTeam : public Entity {
public:
    MockEntityWithTeam()
        : Entity(EntityInstanceId(1))
    {}

    void setTeam(scoreboard::Team* team) { m_team = team; }

    [[nodiscard]] scoreboard::Team* getTeam() override { return m_team; }
    [[nodiscard]] const scoreboard::Team* getTeam() const override { return m_team; }

private:
    scoreboard::Team* m_team = nullptr;
};

// ============================================================================
// Entity::isOnScoreboardTeam 测试
// ============================================================================

class EntityTeamTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_team1 = std::make_unique<MockTeam>("red");
        m_team2 = std::make_unique<MockTeam>("blue");
        m_entity1 = std::make_unique<MockEntityWithTeam>();
        m_entity2 = std::make_unique<MockEntityWithTeam>();
        m_entityNoTeam = std::make_unique<MockEntityWithTeam>();
    }

    void TearDown() override
    {
        m_entityNoTeam.reset();
        m_entity2.reset();
        m_entity1.reset();
        m_team2.reset();
        m_team1.reset();
    }

    std::unique_ptr<MockTeam> m_team1;
    std::unique_ptr<MockTeam> m_team2;
    std::unique_ptr<MockEntityWithTeam> m_entity1;
    std::unique_ptr<MockEntityWithTeam> m_entity2;
    std::unique_ptr<MockEntityWithTeam> m_entityNoTeam;
};

// ============================================================================
// isOnScoreboardTeam 测试
// ============================================================================

TEST_F(EntityTeamTest, IsOnScoreboardTeam_EntityWithTeam_ReturnsTrueForSameTeam)
{
    // 设置实体所属队伍
    m_entity1->setTeam(m_team1.get());

    // 检查实体是否在指定队伍
    EXPECT_TRUE(m_entity1->isOnScoreboardTeam(m_team1.get()));
}

TEST_F(EntityTeamTest, IsOnScoreboardTeam_EntityWithTeam_ReturnsFalseForDifferentTeam)
{
    // 设置实体所属队伍
    m_entity1->setTeam(m_team1.get());

    // 检查实体是否在不同队伍
    EXPECT_FALSE(m_entity1->isOnScoreboardTeam(m_team2.get()));
}

TEST_F(EntityTeamTest, IsOnScoreboardTeam_EntityWithoutTeam_ReturnsFalseForAnyTeam)
{
    // 实体没有队伍
    EXPECT_FALSE(m_entityNoTeam->isOnScoreboardTeam(m_team1.get()));
    EXPECT_FALSE(m_entityNoTeam->isOnScoreboardTeam(m_team2.get()));
}

TEST_F(EntityTeamTest, IsOnScoreboardTeam_EntityWithTeam_ReturnsFalseForNullTeam)
{
    // 设置实体所属队伍
    m_entity1->setTeam(m_team1.get());

    // 检查 null 队伍
    EXPECT_FALSE(m_entity1->isOnScoreboardTeam(nullptr));
}

TEST_F(EntityTeamTest, IsOnScoreboardTeam_EntityWithoutTeam_ReturnsFalseForNullTeam)
{
    // 实体没有队伍，检查 null
    EXPECT_FALSE(m_entityNoTeam->isOnScoreboardTeam(nullptr));
}

// ============================================================================
// isOnSameTeam 测试
// ============================================================================

TEST_F(EntityTeamTest, IsOnSameTeam_SameTeam_ReturnsTrue)
{
    // 两个实体在同一队伍
    m_entity1->setTeam(m_team1.get());
    m_entity2->setTeam(m_team1.get());

    EXPECT_TRUE(m_entity1->isOnSameTeam(*m_entity2));
    EXPECT_TRUE(m_entity2->isOnSameTeam(*m_entity1));
}

TEST_F(EntityTeamTest, IsOnSameTeam_DifferentTeams_ReturnsFalse)
{
    // 两个实体在不同队伍
    m_entity1->setTeam(m_team1.get());
    m_entity2->setTeam(m_team2.get());

    EXPECT_FALSE(m_entity1->isOnSameTeam(*m_entity2));
    EXPECT_FALSE(m_entity2->isOnSameTeam(*m_entity1));
}

TEST_F(EntityTeamTest, IsOnSameTeam_OneWithTeamOneWithout_ReturnsFalse)
{
    // 一个实体有队伍，另一个没有
    m_entity1->setTeam(m_team1.get());
    // entityNoTeam 没有队伍

    EXPECT_FALSE(m_entity1->isOnSameTeam(*m_entityNoTeam));
    EXPECT_FALSE(m_entityNoTeam->isOnSameTeam(*m_entity1));
}

TEST_F(EntityTeamTest, IsOnSameTeam_BothWithoutTeam_ReturnsFalse)
{
    // 两个实体都没有队伍
    EXPECT_FALSE(m_entityNoTeam->isOnSameTeam(*m_entityNoTeam));
}

TEST_F(EntityTeamTest, IsOnSameTeam_SelfCheck_ReturnsTrue)
{
    // 检查自己（同一队伍对象）
    m_entity1->setTeam(m_team1.get());

    // 同一实体检查自己
    EXPECT_TRUE(m_entity1->isOnSameTeam(*m_entity1));
}

TEST_F(EntityTeamTest, IsOnSameTeam_SelfCheckNoTeam_ReturnsFalse)
{
    // 检查自己（没有队伍）
    EXPECT_FALSE(m_entityNoTeam->isOnSameTeam(*m_entityNoTeam));
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(EntityTeamTest, IsOnSameTeam_TeamComparisonByPointer)
{
    // MC 1.16.5: Team.isSameTeam() 使用对象指针相等性判断
    // 即使两个 Team 有相同的名称，它们也不是同一队伍

    // 创建两个名称相同的不同 Team 对象
    auto teamA = std::make_unique<MockTeam>("same_name");
    auto teamB = std::make_unique<MockTeam>("same_name");

    m_entity1->setTeam(teamA.get());
    m_entity2->setTeam(teamB.get());

    // 不同指针，即使名称相同，也不算同一队伍
    EXPECT_FALSE(m_entity1->isOnSameTeam(*m_entity2));
}

TEST_F(EntityTeamTest, IsOnScoreboardTeam_SameTeamObject_ReturnsTrue)
{
    // 使用相同的 Team 指针
    m_entity1->setTeam(m_team1.get());
    m_entity2->setTeam(m_team1.get());

    // 使用 entity2 的队伍检查 entity1
    EXPECT_TRUE(m_entity1->isOnScoreboardTeam(m_entity2->getTeam()));
    EXPECT_TRUE(m_entity2->isOnScoreboardTeam(m_entity1->getTeam()));
}

// ============================================================================
// 横扫攻击场景模拟测试
// ============================================================================

TEST_F(EntityTeamTest, IsOnSameTeam_SweepAttackScenario_TeammatesNotHit)
{
    // 模拟横扫攻击场景：队友不应被攻击
    m_entity1->setTeam(m_team1.get()); // 攻击者
    m_entity2->setTeam(m_team1.get()); // 队友

    // isOnSameTeam 返回 true，表示是队友，不应被攻击
    EXPECT_TRUE(m_entity1->isOnSameTeam(*m_entity2));
}

TEST_F(EntityTeamTest, IsOnSameTeam_SweepAttackScenario_EnemiesHit)
{
    // 模拟横扫攻击场景：敌人应被攻击
    m_entity1->setTeam(m_team1.get()); // 攻击者（红队）
    m_entity2->setTeam(m_team2.get()); // 敌人（蓝队）

    // isOnSameTeam 返回 false，表示不是队友，可以被攻击
    EXPECT_FALSE(m_entity1->isOnSameTeam(*m_entity2));
}

TEST_F(EntityTeamTest, IsOnSameTeam_SweepAttackScenario_NoTeamEntityCanBeHit)
{
    // 模拟横扫攻击场景：没有队伍的实体可以被攻击
    m_entity1->setTeam(m_team1.get()); // 攻击者（红队）
    // entityNoTeam 没有队伍

    // 没有队伍的实体不算队友，可以被攻击
    EXPECT_FALSE(m_entity1->isOnSameTeam(*m_entityNoTeam));
    EXPECT_FALSE(m_entityNoTeam->isOnSameTeam(*m_entity1));
}

// ============================================================================
// Entity::isAlliedTo 测试
// ============================================================================

TEST_F(EntityTeamTest, IsAlliedTo_SelfCheck_AlwaysTrue)
{
    // 实体自身视为盟友，即使没有队伍
    EXPECT_TRUE(m_entityNoTeam->isAlliedTo(*m_entityNoTeam));
    EXPECT_TRUE(m_entity1->isAlliedTo(*m_entity1));
}

TEST_F(EntityTeamTest, IsAlliedTo_SameTeam_ReturnsTrue)
{
    m_entity1->setTeam(m_team1.get());
    m_entity2->setTeam(m_team1.get());

    // 同一队伍双向都是盟友
    EXPECT_TRUE(m_entity1->isAlliedTo(*m_entity2));
    EXPECT_TRUE(m_entity2->isAlliedTo(*m_entity1));
}

TEST_F(EntityTeamTest, IsAlliedTo_DifferentTeams_ReturnsFalse)
{
    m_entity1->setTeam(m_team1.get());
    m_entity2->setTeam(m_team2.get());

    EXPECT_FALSE(m_entity1->isAlliedTo(*m_entity2));
    EXPECT_FALSE(m_entity2->isAlliedTo(*m_entity1));
}

TEST_F(EntityTeamTest, IsAlliedTo_OneWithoutTeam_ReturnsFalse)
{
    m_entity1->setTeam(m_team1.get());

    // 有队伍实体与无队伍实体不是盟友
    EXPECT_FALSE(m_entity1->isAlliedTo(*m_entityNoTeam));
    EXPECT_FALSE(m_entityNoTeam->isAlliedTo(*m_entity1));
}

TEST_F(EntityTeamTest, IsAlliedTo_BothWithoutTeam_ReturnsFalse)
{
    // 两个没有队伍的实体不是盟友（除非自身）
    EXPECT_FALSE(m_entityNoTeam->isAlliedTo(*m_entity1));
}

TEST_F(EntityTeamTest, IsAlliedTo_ConsidersEntityAsAlly_OverrideWorks)
{
    // 验证 considersEntityAsAlly 虚方法可被子类重写
    // 模拟驯服动物继承主人队伍的场景：
    // 狼没有自己的队伍，但 considersEntityAsAlly 检查主人的队伍
    class MockTameableWithOwnerTeam : public MockEntityWithTeam {
    public:
        MockTameableWithOwnerTeam()
            : MockEntityWithTeam()
        {
            // 给一个不同的 EntityInstanceId 以避免与 m_entityNoTeam 冲突
        }

        // 模拟：已驯服的动物继承主人的队伍
        [[nodiscard]] scoreboard::Team* getTeam() override { return m_ownerTeam; }
        [[nodiscard]] const scoreboard::Team* getTeam() const override { return m_ownerTeam; }

        void setOwnerTeam(scoreboard::Team* team) { m_ownerTeam = team; }

    private:
        scoreboard::Team* m_ownerTeam = nullptr;
    };

    auto tameable = std::make_unique<MockTameableWithOwnerTeam>();
    m_entity1->setTeam(m_team1.get());     // 主人在 red 队
    tameable->setOwnerTeam(m_team1.get()); // 驯服动物继承主人的队伍

    // 驯服动物与主人在同一队伍 → 盟友
    EXPECT_TRUE(tameable->isAlliedTo(*m_entity1));
    EXPECT_TRUE(m_entity1->isAlliedTo(*tameable));

    // 驯服动物与敌队不是盟友
    m_entity2->setTeam(m_team2.get());
    EXPECT_FALSE(tameable->isAlliedTo(*m_entity2));
    EXPECT_FALSE(m_entity2->isAlliedTo(*tameable));
}
