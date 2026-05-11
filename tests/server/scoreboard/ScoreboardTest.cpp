#include <gtest/gtest.h>

#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/scoreboard/core/TeamEnums.hpp"
#include "common/scoreboard/criteria/DummyCriteria.hpp"
#include "common/scoreboard/criteria/TriggerCriteria.hpp"
#include "common/scoreboard/criteria/DeathCountCriteria.hpp"
#include "common/scoreboard/criteria/KillCountCriteria.hpp"
#include "common/scoreboard/criteria/ReadOnlyCriteria.hpp"
#include "common/util/text/StringTextComponent.hpp"

using namespace mc;
using namespace mc::scoreboard;

/**
 * @brief 记分板核心逻辑测试套件
 *
 * 测试记分板系统的核心功能：
 * - ScoreCriteriaRegistry 判据注册和查询
 * - Scoreboard 目标创建、删除、查询
 * - Scoreboard 分数操作
 * - ScorePlayerTeam 成员管理
 * - 显示槽位管理
 */
class ScoreboardTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 注册内置判据
        ScoreCriteriaRegistry::instance().registerBuiltinCriteria();
    }

    void TearDown() override {
        // 清理判据注册表
        ScoreCriteriaRegistry::instance().clear();
    }

    // 创建测试用记分板
    Scoreboard createTestScoreboard() {
        return Scoreboard();
    }
};

// ========== ScoreCriteriaRegistry 测试 ==========

TEST_F(ScoreboardTest, CriteriaRegistry_RegisterAndGet) {
    // 获取已注册的判据
    auto* dummy = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    ASSERT_NE(dummy, nullptr);
    EXPECT_EQ(dummy->getName(), "dummy");
    EXPECT_FALSE(dummy->isReadOnly());

    auto* health = ScoreCriteriaRegistry::instance().getCriteria("health");
    ASSERT_NE(health, nullptr);
    EXPECT_EQ(health->getName(), "health");
    EXPECT_TRUE(health->isReadOnly());
}

TEST_F(ScoreboardTest, CriteriaRegistry_HasCriteria) {
    EXPECT_TRUE(ScoreCriteriaRegistry::instance().hasCriteria("dummy"));
    EXPECT_TRUE(ScoreCriteriaRegistry::instance().hasCriteria("deathCount"));
    EXPECT_TRUE(ScoreCriteriaRegistry::instance().hasCriteria("playerKillCount"));
    EXPECT_TRUE(ScoreCriteriaRegistry::instance().hasCriteria("totalKillCount"));
    EXPECT_TRUE(ScoreCriteriaRegistry::instance().hasCriteria("health"));
    EXPECT_FALSE(ScoreCriteriaRegistry::instance().hasCriteria("nonexistent"));
}

TEST_F(ScoreboardTest, CriteriaRegistry_GetAllCriteriaNames) {
    auto names = ScoreCriteriaRegistry::instance().getAllCriteriaNames();
    EXPECT_GE(names.size(), 11);  // 至少 11 个内置判据

    // 检查关键判据存在
    EXPECT_NE(std::find(names.begin(), names.end(), "dummy"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "trigger"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "deathCount"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "health"), names.end());
}

TEST_F(ScoreboardTest, CriteriaRegistry_TeamKillCriteria) {
    // 检查队伍击杀判据
    auto* teamkillRed = ScoreCriteriaRegistry::instance().getCriteria("teamkill.red");
    ASSERT_NE(teamkillRed, nullptr);
    EXPECT_EQ(teamkillRed->getName(), "teamkill.red");

    auto* killedByTeamBlue = ScoreCriteriaRegistry::instance().getCriteria("killedByTeam.blue");
    ASSERT_NE(killedByTeamBlue, nullptr);
    EXPECT_EQ(killedByTeamBlue->getName(), "killedByTeam.blue");
}

// ========== Scoreboard 目标管理测试 ==========

TEST_F(ScoreboardTest, Objective_CreateAndGet) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    ASSERT_NE(criteria, nullptr);

    // 创建目标
    auto* objective = scoreboard.addObjective("kills", *criteria);
    ASSERT_NE(objective, nullptr);
    EXPECT_EQ(objective->getName(), "kills");
    EXPECT_EQ(&objective->getCriteria(), criteria);
    EXPECT_TRUE(scoreboard.hasObjective("kills"));

    // 获取目标
    auto* retrieved = scoreboard.getObjective("kills");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved, objective);

    // 重复创建返回 nullptr
    auto* duplicate = scoreboard.addObjective("kills", *criteria);
    EXPECT_EQ(duplicate, nullptr);
}

TEST_F(ScoreboardTest, Objective_WithDisplayName) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    ASSERT_NE(criteria, nullptr);

    auto displayName = std::make_unique<text::StringTextComponent>("Player Kills");
    auto* objective = scoreboard.addObjective("kills", *criteria, std::move(displayName));
    ASSERT_NE(objective, nullptr);

    auto* display = objective->getDisplayName();
    ASSERT_NE(display, nullptr);
    EXPECT_EQ(display->getUnformattedText(), "Player Kills");
}

TEST_F(ScoreboardTest, Objective_Remove) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* objective = scoreboard.addObjective("kills", *criteria);
    ASSERT_NE(objective, nullptr);

    // 创建分数
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setScorePoints(10);

    // 移除目标
    scoreboard.removeObjective(*objective);
    EXPECT_FALSE(scoreboard.hasObjective("kills"));

    // 分数应被移除
    auto* retrievedScore = scoreboard.getScore("Steve", *objective);
    EXPECT_EQ(retrievedScore, nullptr);
}

TEST_F(ScoreboardTest, Objective_GetObjectives) {
    Scoreboard scoreboard;
    auto* dummy = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* deathCount = ScoreCriteriaRegistry::instance().getCriteria("deathCount");

    scoreboard.addObjective("kills", *dummy);
    scoreboard.addObjective("deaths", *deathCount);
    scoreboard.addObjective("points", *dummy);

    auto objectives = scoreboard.getObjectives();
    EXPECT_EQ(objectives.size(), 3);

    // 按判据筛选
    auto dummyObjectives = scoreboard.getObjectivesByCriteria(*dummy);
    EXPECT_EQ(dummyObjectives.size(), 2);

    auto deathObjectives = scoreboard.getObjectivesByCriteria(*deathCount);
    EXPECT_EQ(deathObjectives.size(), 1);
}

TEST_F(ScoreboardTest, Objective_NameValidation) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");

    // 有效名称
    EXPECT_NE(scoreboard.addObjective("valid_name", *criteria), nullptr);
    EXPECT_NE(scoreboard.addObjective("valid-name", *criteria), nullptr);
    EXPECT_NE(scoreboard.addObjective("valid123", *criteria), nullptr);

    // 无效名称
    EXPECT_EQ(scoreboard.addObjective("", *criteria), nullptr);  // 空
    EXPECT_EQ(scoreboard.addObjective("invalid name", *criteria), nullptr);  // 空格
    EXPECT_EQ(scoreboard.addObjective("invalid@name", *criteria), nullptr);  // 特殊字符

    // 超长名称
    std::string longName(17, 'a');
    EXPECT_EQ(scoreboard.addObjective(longName, *criteria), nullptr);
}

// ========== Scoreboard 分数管理测试 ==========

TEST_F(ScoreboardTest, Score_CreateAndGet) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* objective = scoreboard.addObjective("kills", *criteria);
    ASSERT_NE(objective, nullptr);

    // 创建分数
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    EXPECT_EQ(score->getPlayerName(), "Steve");
    EXPECT_EQ(&score->getObjective(), objective);
    EXPECT_EQ(score->getScorePoints(), 0);  // 初始分数为 0

    // 重复获取返回同一分数
    auto* same = scoreboard.getOrCreateScore("Steve", *objective);
    EXPECT_EQ(same, score);
}

TEST_F(ScoreboardTest, Score_SetAndGet) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* objective = scoreboard.addObjective("kills", *criteria);
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);

    // 设置分数
    score->setScorePoints(10);
    EXPECT_EQ(score->getScorePoints(), 10);

    // 增加分数
    score->addScore(5);
    EXPECT_EQ(score->getScorePoints(), 15);

    // 减少分数
    score->subtractScore(3);
    EXPECT_EQ(score->getScorePoints(), 12);

    // 重置
    score->reset();
    EXPECT_EQ(score->getScorePoints(), 0);
}

TEST_F(ScoreboardTest, Score_Boundary) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* objective = scoreboard.addObjective("kills", *criteria);
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);

    // 最大值
    score->setScorePoints(Score::MAX_SCORE);
    EXPECT_EQ(score->getScorePoints(), Score::MAX_SCORE);

    // 尝试超出最大值
    score->addScore(100);
    EXPECT_EQ(score->getScorePoints(), Score::MAX_SCORE);

    // 最小值
    score->setScorePoints(Score::MIN_SCORE);
    EXPECT_EQ(score->getScorePoints(), Score::MIN_SCORE);

    // 尝试超出最小值
    score->subtractScore(100);
    EXPECT_EQ(score->getScorePoints(), Score::MIN_SCORE);
}

TEST_F(ScoreboardTest, Score_Remove) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* objective = scoreboard.addObjective("kills", *criteria);
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setScorePoints(10);

    // 移除特定目标的分数
    scoreboard.removeScore("Steve", objective);
    auto* retrieved = scoreboard.getScore("Steve", *objective);
    EXPECT_EQ(retrieved, nullptr);

    // 添加多个目标
    auto* objective2 = scoreboard.addObjective("deaths", *criteria);
    scoreboard.getOrCreateScore("Steve", *objective2);
    scoreboard.getOrCreateScore("Steve", *objective);

    // 移除玩家的所有分数
    scoreboard.removeScore("Steve", nullptr);
    EXPECT_EQ(scoreboard.getScore("Steve", *objective), nullptr);
    EXPECT_EQ(scoreboard.getScore("Steve", *objective2), nullptr);
}

TEST_F(ScoreboardTest, Score_GetSorted) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* objective = scoreboard.addObjective("kills", *criteria);

    // 创建多个分数
    scoreboard.getOrCreateScore("Steve", *objective)->setScorePoints(10);
    scoreboard.getOrCreateScore("Alex", *objective)->setScorePoints(20);
    scoreboard.getOrCreateScore("Bob", *objective)->setScorePoints(15);
    scoreboard.getOrCreateScore("Charlie", *objective)->setScorePoints(20);

    // 获取排序后的分数
    auto sorted = scoreboard.getSortedScores(*objective);
    ASSERT_EQ(sorted.size(), 4);

    // 验证排序：分数降序，名称升序
    EXPECT_EQ(sorted[0]->getScorePoints(), 20);
    EXPECT_EQ(sorted[0]->getPlayerName(), "Alex");  // 同分数按名称排序
    EXPECT_EQ(sorted[1]->getScorePoints(), 20);
    EXPECT_EQ(sorted[1]->getPlayerName(), "Charlie");
    EXPECT_EQ(sorted[2]->getScorePoints(), 15);
    EXPECT_EQ(sorted[2]->getPlayerName(), "Bob");
    EXPECT_EQ(sorted[3]->getScorePoints(), 10);
    EXPECT_EQ(sorted[3]->getPlayerName(), "Steve");
}

TEST_F(ScoreboardTest, Score_PlayerObjectives) {
    Scoreboard scoreboard;
    auto* dummy = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* deathCount = ScoreCriteriaRegistry::instance().getCriteria("deathCount");

    auto* kills = scoreboard.addObjective("kills", *dummy);
    auto* deaths = scoreboard.addObjective("deaths", *deathCount);

    scoreboard.getOrCreateScore("Steve", *kills);
    scoreboard.getOrCreateScore("Steve", *deaths);

    auto objectives = scoreboard.getPlayerObjectives("Steve");
    EXPECT_EQ(objectives.size(), 2);
    EXPECT_NE(std::find(objectives.begin(), objectives.end(), "kills"), objectives.end());
    EXPECT_NE(std::find(objectives.begin(), objectives.end(), "deaths"), objectives.end());
}

TEST_F(ScoreboardTest, Score_PlayerNameValidation) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* objective = scoreboard.addObjective("kills", *criteria);

    // 有效名称
    EXPECT_NE(scoreboard.getOrCreateScore("Steve", *objective), nullptr);
    EXPECT_NE(scoreboard.getOrCreateScore("Player123", *objective), nullptr);

    // 空名称
    EXPECT_EQ(scoreboard.getOrCreateScore("", *objective), nullptr);

    // 超长名称
    std::string longName(41, 'a');
    EXPECT_EQ(scoreboard.getOrCreateScore(longName, *objective), nullptr);
}

// ========== 显示槽位测试 ==========

TEST_F(ScoreboardTest, DisplaySlot_SetAndGet) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* objective = scoreboard.addObjective("kills", *criteria);

    // 设置显示槽位
    scoreboard.setObjectiveInDisplaySlot(DisplaySlot::Sidebar, objective);
    auto* retrieved = scoreboard.getObjectiveInDisplaySlot(DisplaySlot::Sidebar);
    EXPECT_EQ(retrieved, objective);

    // 清除显示槽位
    scoreboard.setObjectiveInDisplaySlot(DisplaySlot::Sidebar, nullptr);
    EXPECT_EQ(scoreboard.getObjectiveInDisplaySlot(DisplaySlot::Sidebar), nullptr);
}

TEST_F(ScoreboardTest, DisplaySlot_GetDisplaySlotsForObject) {
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* objective = scoreboard.addObjective("kills", *criteria);

    // 设置多个槽位
    scoreboard.setObjectiveInDisplaySlot(DisplaySlot::Sidebar, objective);
    scoreboard.setObjectiveInDisplaySlot(DisplaySlot::List, objective);

    auto slots = scoreboard.getDisplaySlotsForObject(*objective);
    EXPECT_EQ(slots.size(), 2);

    // 移除目标时清除槽位
    scoreboard.removeObjective(*objective);
    EXPECT_EQ(scoreboard.getObjectiveInDisplaySlot(DisplaySlot::Sidebar), nullptr);
    EXPECT_EQ(scoreboard.getObjectiveInDisplaySlot(DisplaySlot::List), nullptr);
}

// ========== 队伍管理测试 ==========

TEST_F(ScoreboardTest, Team_CreateAndGet) {
    Scoreboard scoreboard;

    // 创建队伍
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);
    EXPECT_EQ(team->getName(), "red");
    EXPECT_TRUE(scoreboard.hasTeam("red"));

    // 获取队伍
    auto* retrieved = scoreboard.getTeam("red");
    EXPECT_EQ(retrieved, team);

    // 重复创建返回 nullptr
    auto* duplicate = scoreboard.createTeam("red");
    EXPECT_EQ(duplicate, nullptr);
}

TEST_F(ScoreboardTest, Team_Remove) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 添加成员
    team->addMember("Steve");
    team->addMember("Alex");

    // 移除队伍
    scoreboard.removeTeam(*team);
    EXPECT_FALSE(scoreboard.hasTeam("red"));

    // 成员应被清空
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), nullptr);
    EXPECT_EQ(scoreboard.getPlayersTeam("Alex"), nullptr);
}

TEST_F(ScoreboardTest, Team_MemberManagement) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");

    // 添加成员
    EXPECT_TRUE(scoreboard.addPlayerToTeam("Steve", *team));
    EXPECT_TRUE(team->hasMember("Steve"));
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), team);

    // 重复添加
    EXPECT_FALSE(scoreboard.addPlayerToTeam("Steve", *team));

    // 添加更多成员
    scoreboard.addPlayerToTeam("Alex", *team);
    EXPECT_EQ(team->getMembers().size(), 2);

    // 移除成员
    EXPECT_TRUE(scoreboard.removePlayerFromTeam("Steve", *team));
    EXPECT_FALSE(team->hasMember("Steve"));
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), nullptr);

    // 重复移除
    EXPECT_FALSE(scoreboard.removePlayerFromTeam("Steve", *team));
}

TEST_F(ScoreboardTest, Team_SwitchTeam) {
    Scoreboard scoreboard;
    auto* red = scoreboard.createTeam("red");
    auto* blue = scoreboard.createTeam("blue");

    // 加入红队
    scoreboard.addPlayerToTeam("Steve", *red);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), red);

    // 切换到蓝队
    scoreboard.addPlayerToTeam("Steve", *blue);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), blue);
    EXPECT_FALSE(red->hasMember("Steve"));
    EXPECT_TRUE(blue->hasMember("Steve"));
}

TEST_F(ScoreboardTest, Team_GetTeams) {
    Scoreboard scoreboard;
    scoreboard.createTeam("red");
    scoreboard.createTeam("blue");
    scoreboard.createTeam("green");

    auto teams = scoreboard.getTeams();
    EXPECT_EQ(teams.size(), 3);
}

TEST_F(ScoreboardTest, Team_NameValidation) {
    Scoreboard scoreboard;

    // 有效名称
    EXPECT_NE(scoreboard.createTeam("valid_name"), nullptr);
    EXPECT_NE(scoreboard.createTeam("valid-name"), nullptr);

    // 无效名称
    EXPECT_EQ(scoreboard.createTeam(""), nullptr);  // 空
    EXPECT_EQ(scoreboard.createTeam("invalid name"), nullptr);  // 空格

    // 超长名称
    std::string longName(17, 'a');
    EXPECT_EQ(scoreboard.createTeam(longName), nullptr);
}

// ========== 集成测试 ==========

TEST_F(ScoreboardTest, FullWorkflow) {
    Scoreboard scoreboard;

    // 1. 创建目标
    auto* dummy = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* kills = scoreboard.addObjective("kills", *dummy);
    auto* deaths = scoreboard.addObjective("deaths", *dummy);

    // 2. 创建队伍
    auto* red = scoreboard.createTeam("red");
    auto* blue = scoreboard.createTeam("blue");

    // 3. 添加玩家到队伍
    scoreboard.addPlayerToTeam("Steve", *red);
    scoreboard.addPlayerToTeam("Alex", *blue);
    scoreboard.addPlayerToTeam("Bob", *red);

    // 4. 设置分数
    scoreboard.getOrCreateScore("Steve", *kills)->setScorePoints(10);
    scoreboard.getOrCreateScore("Alex", *kills)->setScorePoints(20);
    scoreboard.getOrCreateScore("Bob", *kills)->setScorePoints(15);

    scoreboard.getOrCreateScore("Steve", *deaths)->setScorePoints(3);
    scoreboard.getOrCreateScore("Alex", *deaths)->setScorePoints(5);
    scoreboard.getOrCreateScore("Bob", *deaths)->setScorePoints(2);

    // 5. 设置显示槽位
    scoreboard.setObjectiveInDisplaySlot(DisplaySlot::Sidebar, kills);
    scoreboard.setObjectiveInDisplaySlot(DisplaySlot::List, deaths);

    // 6. 验证
    EXPECT_EQ(scoreboard.getObjectives().size(), 2);
    EXPECT_EQ(scoreboard.getTeams().size(), 2);
    EXPECT_EQ(scoreboard.getObjectiveInDisplaySlot(DisplaySlot::Sidebar), kills);

    auto sortedKills = scoreboard.getSortedScores(*kills);
    EXPECT_EQ(sortedKills[0]->getPlayerName(), "Alex");  // 20 kills
    EXPECT_EQ(sortedKills[1]->getPlayerName(), "Bob");   // 15 kills
    EXPECT_EQ(sortedKills[2]->getPlayerName(), "Steve"); // 10 kills

    // 7. 清理
    scoreboard.removeObjective(*kills);
    EXPECT_EQ(scoreboard.getObjectives().size(), 1);
    EXPECT_EQ(scoreboard.getObjectiveInDisplaySlot(DisplaySlot::Sidebar), nullptr);
}

// ========== forAllObjectives 测试 ==========

TEST_F(ScoreboardTest, ForAllObjectives) {
    Scoreboard scoreboard;
    auto* deathCount = ScoreCriteriaRegistry::instance().getCriteria("deathCount");
    auto* dummy = ScoreCriteriaRegistry::instance().getCriteria("dummy");

    // 创建多个死亡计数目标
    auto* deaths1 = scoreboard.addObjective("deaths1", *deathCount);
    auto* deaths2 = scoreboard.addObjective("deaths2", *deathCount);
    auto* points = scoreboard.addObjective("points", *dummy);  // 不同判据

    scoreboard.getOrCreateScore("Steve", *deaths1)->setScorePoints(10);
    scoreboard.getOrCreateScore("Steve", *deaths2)->setScorePoints(20);
    scoreboard.getOrCreateScore("Steve", *points)->setScorePoints(100);

    // 统计所有死亡计数目标的分数
    i32 totalDeaths = 0;
    scoreboard.forAllObjectives(*deathCount, "Steve", [&totalDeaths](Score& score) {
        totalDeaths += score.getScorePoints();
    });

    EXPECT_EQ(totalDeaths, 30);  // 10 + 20
}

// main 函数由 gtest_main 库提供
