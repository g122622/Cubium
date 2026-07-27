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

#include "common/TempDirHelper.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/scoreboard/storage/ScoreboardDataManager.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include <filesystem>

using namespace mc;
using namespace mc::scoreboard;
using namespace mc::world::storage;

/**
 * @brief ScoreboardDataManager 测试套件
 *
 * 测试 ScoreboardDataManager 的持久化操作，重点关注 deleteObjective 的级联删除行为：
 * - 删除目标时级联删除关联分数
 * - 删除不存在的目标不应出错
 * - 目标名前缀冲突边界（abc vs abcdef）不应误删
 * - 删除一个目标不影响其他目标的分数
 * - 持久化验证：重新打开数据库后确认数据一致性
 */
class ScoreboardDataManagerTest : public ::testing::Test {
protected:
    std::filesystem::path testDir;
    std::unique_ptr<SingleLevelStorageManager> storage;
    ScoreboardDataManager* dataManager{nullptr};

    void SetUp() override
    {
        // PID + 纳秒 + 计数器组合，跨进程唯一，避免 CTest -j16 同名目录撞锁
        testDir = mc::test::makeUniqueTestDir("mc_sbdm_test");

        storage = std::make_unique<SingleLevelStorageManager>();
        SingleLevelStorageConfig config;
        config.consistencyMode = ConsistencyMode::Strong;
        auto result = storage->open(testDir, config);
        ASSERT_TRUE(result.success()) << "Failed to open storage: " << result.error().message();

        dataManager = storage->scoreboardDataManager();
        ASSERT_NE(dataManager, nullptr) << "ScoreboardDataManager should be initialized after open()";
    }

    void TearDown() override
    {
        storage->close();
        storage.reset();
        dataManager = nullptr;

        mc::test::removeTestDir(testDir);
    }

    // 辅助方法：保存一个目标
    void saveObjective(const std::string& name,
        const std::string& criteria = "dummy",
        const std::string& displayName = "",
        const std::string& renderType = "integer")
    {
        ScoreboardSaveData::ObjectiveData data;
        data.name = name;
        data.criteriaName = criteria;
        data.displayName = displayName.empty() ? name : displayName;
        data.renderType = renderType;

        auto result = dataManager->saveObjective(data);
        ASSERT_TRUE(result.success()) << "saveObjective failed: " << result.error().message();
    }

    // 辅助方法：保存一个分数
    void saveScore(const std::string& objectiveName, const std::string& playerName, i32 score, bool locked = false)
    {
        auto result = dataManager->saveScore(objectiveName, playerName, score, locked);
        ASSERT_TRUE(result.success()) << "saveScore failed: " << result.error().message();
    }

    // 辅助方法：验证目标存在
    void expectObjectiveExists(const std::string& name)
    {
        auto result = dataManager->loadObjective(name);
        ASSERT_TRUE(result.success()) << "loadObjective failed: " << result.error().message();
        EXPECT_TRUE(result.value().has_value()) << "Objective '" << name << "' should exist";
    }

    // 辅助方法：验证目标不存在
    void expectObjectiveNotExists(const std::string& name)
    {
        auto result = dataManager->loadObjective(name);
        ASSERT_TRUE(result.success()) << "loadObjective failed: " << result.error().message();
        EXPECT_FALSE(result.value().has_value()) << "Objective '" << name << "' should not exist";
    }

    // 辅助方法：验证分数存在且值正确
    void expectScoreExists(const std::string& objectiveName, const std::string& playerName, i32 expectedScore)
    {
        auto result = dataManager->loadScore(objectiveName, playerName);
        ASSERT_TRUE(result.success()) << "loadScore failed: " << result.error().message();
        ASSERT_TRUE(result.value().has_value())
            << "Score for objective '" << objectiveName << "' player '" << playerName << "' should exist";
        EXPECT_EQ(result.value()->score, expectedScore);
    }

    // 辅助方法：验证分数不存在
    void expectScoreNotExists(const std::string& objectiveName, const std::string& playerName)
    {
        auto result = dataManager->loadScore(objectiveName, playerName);
        ASSERT_TRUE(result.success()) << "loadScore failed: " << result.error().message();
        EXPECT_FALSE(result.value().has_value())
            << "Score for objective '" << objectiveName << "' player '" << playerName << "' should not exist";
    }

    // 辅助方法：验证目标的所有分数都不存在
    void expectNoScoresForObjective(const std::string& objectiveName)
    {
        auto result = dataManager->loadScoresForObjective(objectiveName);
        ASSERT_TRUE(result.success()) << "loadScoresForObjective failed: " << result.error().message();
        EXPECT_TRUE(result.value().empty()) << "There should be no scores for objective '" << objectiveName << "'";
    }
};

// ========== 基础操作测试 ==========

TEST_F(ScoreboardDataManagerTest, SaveAndLoadObjective)
{
    saveObjective("kills", "dummy", "Player Kills", "integer");
    expectObjectiveExists("kills");

    auto result = dataManager->loadObjective("kills");
    auto& data = result.value().value();
    EXPECT_EQ(data.name, "kills");
    EXPECT_EQ(data.criteriaName, "dummy");
    EXPECT_EQ(data.displayName, "Player Kills");
    EXPECT_EQ(data.renderType, "integer");
}

TEST_F(ScoreboardDataManagerTest, SaveAndLoadScore)
{
    saveObjective("points");
    saveScore("points", "Steve", 100);
    saveScore("points", "Alex", 50);

    expectScoreExists("points", "Steve", 100);
    expectScoreExists("points", "Alex", 50);
}

TEST_F(ScoreboardDataManagerTest, LoadNonexistentObjective)
{
    auto result = dataManager->loadObjective("nonexistent");
    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().has_value());
}

TEST_F(ScoreboardDataManagerTest, LoadNonexistentScore)
{
    auto result = dataManager->loadScore("nonexistent", "Steve");
    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().has_value());
}

// ========== deleteObjective 级联删除测试 ==========

TEST_F(ScoreboardDataManagerTest, DeleteObjective_CascadesScores)
{
    // 创建目标并添加多个玩家的分数
    saveObjective("kills");
    saveScore("kills", "Steve", 100);
    saveScore("kills", "Alex", 50);
    saveScore("kills", "Bob", 25);

    // 验证数据已写入
    expectObjectiveExists("kills");
    expectScoreExists("kills", "Steve", 100);
    expectScoreExists("kills", "Alex", 50);
    expectScoreExists("kills", "Bob", 25);

    // 删除目标
    auto result = dataManager->deleteObjective("kills");
    ASSERT_TRUE(result.success()) << "deleteObjective failed: " << result.error().message();

    // 验证目标已被删除
    expectObjectiveNotExists("kills");

    // 验证所有关联分数已被级联删除
    expectScoreNotExists("kills", "Steve");
    expectScoreNotExists("kills", "Alex");
    expectScoreNotExists("kills", "Bob");

    // 使用 loadScoresForObjective 验证
    expectNoScoresForObjective("kills");
}

TEST_F(ScoreboardDataManagerTest, DeleteObjective_DoesNotAffectOtherObjectives)
{
    // 创建两个目标
    saveObjective("kills");
    saveObjective("deaths");

    // 为两个目标添加分数（包含同名玩家）
    saveScore("kills", "Steve", 100);
    saveScore("kills", "Alex", 50);
    saveScore("deaths", "Steve", 10);
    saveScore("deaths", "Alex", 5);

    // 删除 kills 目标
    auto result = dataManager->deleteObjective("kills");
    ASSERT_TRUE(result.success()) << "deleteObjective failed: " << result.error().message();

    // kills 目标及其分数应被删除
    expectObjectiveNotExists("kills");
    expectScoreNotExists("kills", "Steve");
    expectScoreNotExists("kills", "Alex");

    // deaths 目标及其分数应保持不变
    expectObjectiveExists("deaths");
    expectScoreExists("deaths", "Steve", 10);
    expectScoreExists("deaths", "Alex", 5);
}

TEST_F(ScoreboardDataManagerTest, DeleteObjective_ObjectiveNamePrefix_NoCrossDeletion)
{
    // 测试目标名前缀冲突：abc vs abcdef
    // 确保删除 "abc" 不会误删 "abcdef" 的分数
    saveObjective("abc");
    saveObjective("abcdef");

    saveScore("abc", "Player1", 10);
    saveScore("abcdef", "Player1", 20);

    // 删除 "abc" 目标
    auto result = dataManager->deleteObjective("abc");
    ASSERT_TRUE(result.success()) << "deleteObjective failed: " << result.error().message();

    // abc 目标及其分数应被删除
    expectObjectiveNotExists("abc");
    expectScoreNotExists("abc", "Player1");

    // abcdef 目标及其分数应保持不变
    expectObjectiveExists("abcdef");
    expectScoreExists("abcdef", "Player1", 20);
}

TEST_F(ScoreboardDataManagerTest, DeleteObjective_EmptyObjective_NoScoresToDelete)
{
    // 删除没有分数的目标
    saveObjective("empty_obj");

    auto result = dataManager->deleteObjective("empty_obj");
    ASSERT_TRUE(result.success()) << "deleteObjective failed: " << result.error().message();

    expectObjectiveNotExists("empty_obj");
}

TEST_F(ScoreboardDataManagerTest, DeleteObjective_NonexistentObjective_NoError)
{
    // 删除不存在的目标应成功（幂等）
    auto result = dataManager->deleteObjective("nonexistent");
    EXPECT_TRUE(result.success()) << "Deleting nonexistent objective should succeed";
}

TEST_F(ScoreboardDataManagerTest, DeleteObjective_MultipleObjectivesWithScores)
{
    // 创建三个目标，各有不同数量的分数
    saveObjective("obj1");
    saveObjective("obj2");
    saveObjective("obj3");

    saveScore("obj1", "P1", 1);
    saveScore("obj1", "P2", 2);
    saveScore("obj1", "P3", 3);

    saveScore("obj2", "P1", 10);
    saveScore("obj2", "P4", 40);

    saveScore("obj3", "P5", 50);

    // 删除 obj2
    auto result = dataManager->deleteObjective("obj2");
    ASSERT_TRUE(result.success()) << "deleteObjective failed: " << result.error().message();

    // obj2 及其分数应被删除
    expectObjectiveNotExists("obj2");
    expectNoScoresForObjective("obj2");
    expectScoreNotExists("obj2", "P1");
    expectScoreNotExists("obj2", "P4");

    // obj1 和 obj3 的数据应保持不变
    expectObjectiveExists("obj1");
    expectScoreExists("obj1", "P1", 1);
    expectScoreExists("obj1", "P2", 2);
    expectScoreExists("obj1", "P3", 3);

    expectObjectiveExists("obj3");
    expectScoreExists("obj3", "P5", 50);
}

// ========== 持久化验证测试 ==========

TEST_F(ScoreboardDataManagerTest, DeleteObjective_PersistenceAfterReopen)
{
    // 创建目标并添加分数
    saveObjective("kills");
    saveScore("kills", "Steve", 100);
    saveScore("kills", "Alex", 50);

    saveObjective("deaths");
    saveScore("deaths", "Steve", 10);

    // 删除 kills 目标
    auto result = dataManager->deleteObjective("kills");
    ASSERT_TRUE(result.success());

    // 关闭并重新打开数据库
    storage->close();
    storage = std::make_unique<SingleLevelStorageManager>();
    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Strong;
    auto openResult = storage->open(testDir, config);
    ASSERT_TRUE(openResult.success()) << "Failed to reopen storage: " << openResult.error().message();

    dataManager = storage->scoreboardDataManager();
    ASSERT_NE(dataManager, nullptr);

    // 验证 kills 目标及其分数仍然不存在（持久化后）
    expectObjectiveNotExists("kills");
    expectScoreNotExists("kills", "Steve");
    expectScoreNotExists("kills", "Alex");
    expectNoScoresForObjective("kills");

    // 验证 deaths 目标及其分数仍然存在
    expectObjectiveExists("deaths");
    expectScoreExists("deaths", "Steve", 10);
}

// ========== deleteScore 独立测试 ==========

TEST_F(ScoreboardDataManagerTest, DeleteScore_SingleScore)
{
    saveObjective("points");
    saveScore("points", "Steve", 100);
    saveScore("points", "Alex", 50);

    // 删除单个分数
    auto result = dataManager->deleteScore("points", "Steve");
    ASSERT_TRUE(result.success());

    expectScoreNotExists("points", "Steve");
    expectScoreExists("points", "Alex", 50);
}

// ========== deletePlayerScores 测试 ==========

TEST_F(ScoreboardDataManagerTest, DeletePlayerScores_AllObjectives)
{
    saveObjective("kills");
    saveObjective("deaths");

    saveScore("kills", "Steve", 100);
    saveScore("kills", "Alex", 50);
    saveScore("deaths", "Steve", 10);
    saveScore("deaths", "Alex", 5);

    // 删除 Steve 的所有分数
    auto result = dataManager->deletePlayerScores("Steve");
    ASSERT_TRUE(result.success()) << "deletePlayerScores failed: " << result.error().message();

    // Steve 的分数应被删除
    expectScoreNotExists("kills", "Steve");
    expectScoreNotExists("deaths", "Steve");

    // Alex 的分数应保持不变
    expectScoreExists("kills", "Alex", 50);
    expectScoreExists("deaths", "Alex", 5);
}

// ========== saveScoreboard / loadScoreboard 集成测试 ==========

TEST_F(ScoreboardDataManagerTest, SaveAndLoadScoreboard_RoundTrip)
{
    // 注册判据（Scoreboard::addObjective 需要）
    ScoreCriteriaRegistry::instance().registerBuiltinCriteria();

    // 创建记分板并添加数据
    Scoreboard scoreboard;
    auto* dummy = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* kills = scoreboard.addObjective("kills", *dummy);
    auto* deaths = scoreboard.addObjective("deaths", *dummy);

    scoreboard.getOrCreateScore("Steve", *kills)->setScorePoints(100);
    scoreboard.getOrCreateScore("Alex", *kills)->setScorePoints(50);
    scoreboard.getOrCreateScore("Steve", *deaths)->setScorePoints(10);
    scoreboard.getOrCreateScore("Alex", *deaths)->setScorePoints(5);

    // 保存
    auto saveResult = dataManager->saveScoreboard(scoreboard);
    ASSERT_TRUE(saveResult.success()) << "saveScoreboard failed: " << saveResult.error().message();

    // 加载到新记分板
    Scoreboard restored;
    ScoreCriteriaRegistry::instance().registerBuiltinCriteria();
    auto loadResult = dataManager->loadScoreboard(restored);
    ASSERT_TRUE(loadResult.success()) << "loadScoreboard failed: " << loadResult.error().message();

    // 验证目标
    EXPECT_TRUE(restored.hasObjective("kills"));
    EXPECT_TRUE(restored.hasObjective("deaths"));

    // 验证分数
    auto* restoredKills = restored.getObjective("kills");
    ASSERT_NE(restoredKills, nullptr);
    auto* steveKills = restored.getScore("Steve", *restoredKills);
    ASSERT_NE(steveKills, nullptr);
    EXPECT_EQ(steveKills->getScorePoints(), 100);

    auto* alexKills = restored.getScore("Alex", *restoredKills);
    ASSERT_NE(alexKills, nullptr);
    EXPECT_EQ(alexKills->getScorePoints(), 50);

    // 清理判据注册表
    ScoreCriteriaRegistry::instance().clear();
}

TEST_F(ScoreboardDataManagerTest, DeleteObjective_AfterScoreboardSaveAndLoad)
{
    // 注册判据
    ScoreCriteriaRegistry::instance().registerBuiltinCriteria();

    // 创建记分板并添加数据
    Scoreboard scoreboard;
    auto* dummy = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* kills = scoreboard.addObjective("kills", *dummy);
    auto* deaths = scoreboard.addObjective("deaths", *dummy);

    scoreboard.getOrCreateScore("Steve", *kills)->setScorePoints(100);
    scoreboard.getOrCreateScore("Alex", *kills)->setScorePoints(50);
    scoreboard.getOrCreateScore("Steve", *deaths)->setScorePoints(10);

    // 保存到数据库
    auto saveResult = dataManager->saveScoreboard(scoreboard);
    ASSERT_TRUE(saveResult.success());

    // 删除 kills 目标（应级联删除其所有分数）
    auto deleteResult = dataManager->deleteObjective("kills");
    ASSERT_TRUE(deleteResult.success());

    // 验证删除
    expectObjectiveNotExists("kills");
    expectScoreNotExists("kills", "Steve");
    expectScoreNotExists("kills", "Alex");
    expectNoScoresForObjective("kills");

    // deaths 目标及其分数应保持不变
    expectObjectiveExists("deaths");
    expectScoreExists("deaths", "Steve", 10);

    // 加载到新记分板，验证数据库状态一致
    Scoreboard restored;
    auto loadResult = dataManager->loadScoreboard(restored);
    ASSERT_TRUE(loadResult.success());

    EXPECT_FALSE(restored.hasObjective("kills"));
    EXPECT_TRUE(restored.hasObjective("deaths"));

    // 清理判据注册表
    ScoreCriteriaRegistry::instance().clear();
}

// ========== 缓存一致性测试 ==========

TEST_F(ScoreboardDataManagerTest, DeleteObjective_CacheConsistency)
{
    // 先加载目标到缓存
    saveObjective("obj1");
    saveScore("obj1", "P1", 10);
    saveScore("obj1", "P2", 20);

    // 加载到缓存
    expectObjectiveExists("obj1");
    expectScoreExists("obj1", "P1", 10);

    // 删除目标
    auto result = dataManager->deleteObjective("obj1");
    ASSERT_TRUE(result.success());

    // 再次加载应返回空（验证缓存已清除）
    expectObjectiveNotExists("obj1");
    expectScoreNotExists("obj1", "P1");
    expectScoreNotExists("obj1", "P2");
}

TEST_F(ScoreboardDataManagerTest, DeleteObjective_DirtyScoresCleared)
{
    // 创建目标并添加分数
    saveObjective("obj1");
    saveScore("obj1", "P1", 10);

    // 删除目标（应清除关联的脏分数条目）
    auto result = dataManager->deleteObjective("obj1");
    ASSERT_TRUE(result.success());

    // 验证目标及其分数已被删除
    expectObjectiveNotExists("obj1");
    expectScoreNotExists("obj1", "P1");
}

// ========== loadScoresForObjective 测试 ==========

TEST_F(ScoreboardDataManagerTest, LoadScoresForObjective)
{
    saveObjective("points");
    saveScore("points", "Steve", 100);
    saveScore("points", "Alex", 50);
    saveScore("points", "Bob", 25);

    auto result = dataManager->loadScoresForObjective("points");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 3);

    // 验证分数值
    bool foundSteve = false, foundAlex = false, foundBob = false;
    for (const auto& score : result.value()) {
        if (score.playerName == "Steve") {
            EXPECT_EQ(score.score, 100);
            foundSteve = true;
        } else if (score.playerName == "Alex") {
            EXPECT_EQ(score.score, 50);
            foundAlex = true;
        } else if (score.playerName == "Bob") {
            EXPECT_EQ(score.score, 25);
            foundBob = true;
        }
    }
    EXPECT_TRUE(foundSteve);
    EXPECT_TRUE(foundAlex);
    EXPECT_TRUE(foundBob);
}

TEST_F(ScoreboardDataManagerTest, LoadScoresForObjective_AfterDeleteObjective)
{
    saveObjective("points");
    saveScore("points", "Steve", 100);
    saveScore("points", "Alex", 50);

    // 删除目标
    auto deleteResult = dataManager->deleteObjective("points");
    ASSERT_TRUE(deleteResult.success());

    // 加载该目标的分数应返回空
    auto result = dataManager->loadScoresForObjective("points");
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().empty());
}

// ========== 队伍操作测试（确保 deleteObjective 不影响队伍）==========

TEST_F(ScoreboardDataManagerTest, DeleteObjective_DoesNotAffectTeams)
{
    // 保存目标和队伍
    saveObjective("kills");
    saveScore("kills", "Steve", 100);

    ScoreboardSaveData::TeamData teamData;
    teamData.name = "red";
    teamData.displayName = "Red Team";
    teamData.color = "red";
    teamData.nameTagVisibility = "always";
    teamData.deathMessageVisibility = "always";
    teamData.collisionRule = "always";
    teamData.allowFriendlyFire = true;
    teamData.seeFriendlyInvisibles = false;
    teamData.members = {"Steve"};

    auto teamResult = dataManager->saveTeam(teamData);
    ASSERT_TRUE(teamResult.success());

    // 删除目标
    auto result = dataManager->deleteObjective("kills");
    ASSERT_TRUE(result.success());

    // 队伍应保持不变
    auto loadResult = dataManager->loadTeam("red");
    ASSERT_TRUE(loadResult.success());
    ASSERT_TRUE(loadResult.value().has_value());
    EXPECT_EQ(loadResult.value()->name, "red");
    EXPECT_EQ(loadResult.value()->members.size(), 1);
    EXPECT_EQ(loadResult.value()->members[0], "Steve");
}
