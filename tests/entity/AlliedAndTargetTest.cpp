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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "entity/core/Entity.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::entity::ai::goal;

// ============================================================================
// 辅助 Mock 类
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

class MockEntityWithTeam : public Entity {
public:
    MockEntityWithTeam()
        : Entity(EntityId(1))
    {}

    void setTeam(scoreboard::Team* team) { m_team = team; }

    [[nodiscard]] scoreboard::Team* getTeam() override { return m_team; }
    [[nodiscard]] const scoreboard::Team* getTeam() const override { return m_team; }

private:
    scoreboard::Team* m_team = nullptr;
};

// ============================================================================
// isAlliedTo 双向性测试
// ============================================================================

class AlliedTest : public ::testing::Test {
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

TEST_F(AlliedTest, SelfCheck_AlwaysTrue)
{
    // 即使没有队伍，实体自身也是盟友
    EXPECT_TRUE(m_entityNoTeam->isAlliedTo(*m_entityNoTeam));
    EXPECT_TRUE(m_entity1->isAlliedTo(*m_entity1));
}

TEST_F(AlliedTest, SameTeam_BothDirections)
{
    m_entity1->setTeam(m_team1.get());
    m_entity2->setTeam(m_team1.get());

    EXPECT_TRUE(m_entity1->isAlliedTo(*m_entity2));
    EXPECT_TRUE(m_entity2->isAlliedTo(*m_entity1));
}

TEST_F(AlliedTest, DifferentTeams_NotAllied)
{
    m_entity1->setTeam(m_team1.get());
    m_entity2->setTeam(m_team2.get());

    EXPECT_FALSE(m_entity1->isAlliedTo(*m_entity2));
    EXPECT_FALSE(m_entity2->isAlliedTo(*m_entity1));
}

TEST_F(AlliedTest, OneWithoutTeam_NotAllied)
{
    m_entity1->setTeam(m_team1.get());

    EXPECT_FALSE(m_entity1->isAlliedTo(*m_entityNoTeam));
    EXPECT_FALSE(m_entityNoTeam->isAlliedTo(*m_entity1));
}

TEST_F(AlliedTest, BothWithoutTeam_NotAlliedUnlessSelf)
{
    EXPECT_FALSE(m_entityNoTeam->isAlliedTo(*m_entity1));
}

TEST_F(AlliedTest, ConsidersEntityAsAlly_OverrideBidirectionalCheck)
{
    // isAlliedTo 是双向的：如果任一方向认为对方是盟友，结果为 true
    // 这在 TameableEntity 继承主人队伍时很重要
    // 例如：驯服的狼（无队伍但继承主人队伍） vs 主人（有队伍）
    // 狼的 considersEntityAsAlly(主人) 返回 true（因为狼继承主人队伍）
    // 主人的 considersEntityAsAlly(狼) 可能返回 false（主人不知道狼继承了自己队伍）
    // 但 isAlliedTo 会检查两个方向，所以结果为 true

    // 这里验证默认行为：同队则双向 true
    m_entity1->setTeam(m_team1.get());
    m_entity2->setTeam(m_team1.get());
    EXPECT_TRUE(m_entity1->considersEntityAsAlly(*m_entity2));
    EXPECT_TRUE(m_entity2->considersEntityAsAlly(*m_entity1));
}

TEST_F(AlliedTest, IsAlliedTo_SweepAttackScenario_TeammatesNotHit)
{
    m_entity1->setTeam(m_team1.get());
    m_entity2->setTeam(m_team1.get());

    // 横扫攻击不应击中盟友
    EXPECT_TRUE(m_entity1->isAlliedTo(*m_entity2));
}

TEST_F(AlliedTest, IsAlliedTo_SweepAttackScenario_EnemiesHit)
{
    m_entity1->setTeam(m_team1.get());
    m_entity2->setTeam(m_team2.get());

    // 横扫攻击应击中敌人
    EXPECT_FALSE(m_entity1->isAlliedTo(*m_entity2));
}
