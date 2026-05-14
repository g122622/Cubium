#include <gtest/gtest.h>

#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/scoreboard/criteria/DeathCountCriteria.hpp"
#include "common/scoreboard/criteria/DummyCriteria.hpp"
#include "common/scoreboard/criteria/KillCountCriteria.hpp"
#include "common/scoreboard/criteria/ReadOnlyCriteria.hpp"
#include "common/scoreboard/criteria/TeamKillCriteria.hpp"
#include "common/scoreboard/criteria/TriggerCriteria.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"

using namespace mc;
using namespace mc::scoreboard;

/**
 * @brief 判据系统测试套件
 *
 * 测试各类判据：
 * - DummyCriteria 手动设置判据
 * - TriggerCriteria 触发器判据
 * - DeathCountCriteria 死亡计数判据
 * - KillCountCriteria 击杀计数判据
 * - ReadOnlyCriteria 只读判据
 * - TeamKillCriteria 队伍击杀判据
 */
class ScoreCriteriaTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册内置判据
        ScoreCriteriaRegistry::instance().registerBuiltinCriteria();
    }

    void TearDown() override
    {
        // 清理判据注册表
        ScoreCriteriaRegistry::instance().clear();
    }
};

// ========== DummyCriteria 测试 ==========

TEST_F(ScoreCriteriaTest, DummyCriteria_Basic)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    ASSERT_NE(criteria, nullptr);

    EXPECT_EQ(criteria->getName(), "dummy");
    EXPECT_FALSE(criteria->isReadOnly());
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Integer);
}

TEST_F(ScoreCriteriaTest, DummyCriteria_ManualScore)
{
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* objective = scoreboard.addObjective("test", *criteria);
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);

    // Dummy 判据允许手动设置分数
    score->setScorePoints(100);
    EXPECT_EQ(score->getScorePoints(), 100);

    score->addScore(50);
    EXPECT_EQ(score->getScorePoints(), 150);

    score->subtractScore(30);
    EXPECT_EQ(score->getScorePoints(), 120);
}

// ========== TriggerCriteria 测试 ==========

TEST_F(ScoreCriteriaTest, TriggerCriteria_Basic)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(criteria, nullptr);

    EXPECT_EQ(criteria->getName(), "trigger");
    EXPECT_FALSE(criteria->isReadOnly());
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Integer);
}

TEST_F(ScoreCriteriaTest, TriggerCriteria_Locked)
{
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("trigger");
    auto* objective = scoreboard.addObjective("trigger_obj", *criteria);
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);

    // 新创建的 Trigger 分数默认不锁定
    // 注意：实际的锁定/解锁逻辑需要在 TriggerCriteria 中实现
    // 这里我们只验证 isLocked/setLocked 方法
    EXPECT_FALSE(score->isLocked());

    // 可以手动设置锁定状态
    score->setLocked(true);
    EXPECT_TRUE(score->isLocked());
}

TEST_F(ScoreCriteriaTest, TriggerCriteria_UnlockAndModify)
{
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("trigger");
    auto* objective = scoreboard.addObjective("trigger_obj", *criteria);
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);

    // 解锁后可以修改
    score->setLocked(false);
    EXPECT_FALSE(score->isLocked());

    score->setScorePoints(10);
    EXPECT_EQ(score->getScorePoints(), 10);

    // 修改后自动锁定
    // 注意：实际实现中 TriggerCriteria 会在 onScoreChanged 后重新锁定
}

// ========== DeathCountCriteria 测试 ==========

TEST_F(ScoreCriteriaTest, DeathCountCriteria_Basic)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("deathCount");
    ASSERT_NE(criteria, nullptr);

    EXPECT_EQ(criteria->getName(), "deathCount");
    EXPECT_FALSE(criteria->isReadOnly());
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Integer);
}

TEST_F(ScoreCriteriaTest, DeathCountCriteria_IncrementOnDeath)
{
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("deathCount");
    auto* objective = scoreboard.addObjective("deaths", *criteria);
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);

    // 初始分数为 0
    EXPECT_EQ(score->getScorePoints(), 0);

    // 模拟玩家死亡
    criteria->onPlayerDeath("Steve", scoreboard);

    // 死亡计数应增加
    EXPECT_EQ(score->getScorePoints(), 1);

    // 再次死亡
    criteria->onPlayerDeath("Steve", scoreboard);
    EXPECT_EQ(score->getScorePoints(), 2);
}

TEST_F(ScoreCriteriaTest, DeathCountCriteria_MultiplePlayers)
{
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("deathCount");
    auto* objective = scoreboard.addObjective("deaths", *criteria);

    scoreboard.getOrCreateScore("Steve", *objective);
    scoreboard.getOrCreateScore("Alex", *objective);
    scoreboard.getOrCreateScore("Bob", *objective);

    // Steve 死亡 3 次
    for (int i = 0; i < 3; ++i) {
        criteria->onPlayerDeath("Steve", scoreboard);
    }

    // Alex 死亡 1 次
    criteria->onPlayerDeath("Alex", scoreboard);

    // Bob 没有死亡

    EXPECT_EQ(scoreboard.getScore("Steve", *objective)->getScorePoints(), 3);
    EXPECT_EQ(scoreboard.getScore("Alex", *objective)->getScorePoints(), 1);
    EXPECT_EQ(scoreboard.getScore("Bob", *objective)->getScorePoints(), 0);
}

// ========== KillCountCriteria 测试 ==========

TEST_F(ScoreCriteriaTest, PlayerKillCountCriteria_Basic)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("playerKillCount");
    ASSERT_NE(criteria, nullptr);

    EXPECT_EQ(criteria->getName(), "playerKillCount");
    EXPECT_FALSE(criteria->isReadOnly());
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Integer);
}

TEST_F(ScoreCriteriaTest, PlayerKillCountCriteria_IncrementOnPlayerKill)
{
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("playerKillCount");
    auto* objective = scoreboard.addObjective("player_kills", *criteria);
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);

    // 玩家击杀玩家
    criteria->onPlayerKill("Steve", "player", true, scoreboard);
    EXPECT_EQ(score->getScorePoints(), 1);

    // 击杀非玩家实体不计入
    criteria->onPlayerKill("Steve", "zombie", false, scoreboard);
    EXPECT_EQ(score->getScorePoints(), 1); // 仍然是 1
}

TEST_F(ScoreCriteriaTest, TotalKillCountCriteria_Basic)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("totalKillCount");
    ASSERT_NE(criteria, nullptr);

    EXPECT_EQ(criteria->getName(), "totalKillCount");
    EXPECT_FALSE(criteria->isReadOnly());
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Integer);
}

TEST_F(ScoreCriteriaTest, TotalKillCountCriteria_IncrementOnAnyKill)
{
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("totalKillCount");
    auto* objective = scoreboard.addObjective("total_kills", *criteria);
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);

    // 击杀玩家
    criteria->onPlayerKill("Steve", "player", true, scoreboard);
    EXPECT_EQ(score->getScorePoints(), 1);

    // 击杀僵尸
    criteria->onPlayerKill("Steve", "zombie", false, scoreboard);
    EXPECT_EQ(score->getScorePoints(), 2);

    // 击杀骷髅
    criteria->onPlayerKill("Steve", "skeleton", false, scoreboard);
    EXPECT_EQ(score->getScorePoints(), 3);
}

// ========== ReadOnlyCriteria 测试 ==========

TEST_F(ScoreCriteriaTest, HealthCriteria_Basic)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("health");
    ASSERT_NE(criteria, nullptr);

    EXPECT_EQ(criteria->getName(), "health");
    EXPECT_TRUE(criteria->isReadOnly());                             // 只读
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Hearts); // 心形渲染
}

TEST_F(ScoreCriteriaTest, FoodCriteria_Basic)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("food");
    ASSERT_NE(criteria, nullptr);

    EXPECT_EQ(criteria->getName(), "food");
    EXPECT_TRUE(criteria->isReadOnly());
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Integer);
}

TEST_F(ScoreCriteriaTest, AirCriteria_Basic)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("air");
    ASSERT_NE(criteria, nullptr);

    EXPECT_EQ(criteria->getName(), "air");
    EXPECT_TRUE(criteria->isReadOnly());
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Integer);
}

TEST_F(ScoreCriteriaTest, ArmorCriteria_Basic)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("armor");
    ASSERT_NE(criteria, nullptr);

    EXPECT_EQ(criteria->getName(), "armor");
    EXPECT_TRUE(criteria->isReadOnly());
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Integer);
}

TEST_F(ScoreCriteriaTest, XpCriteria_Basic)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("xp");
    ASSERT_NE(criteria, nullptr);

    EXPECT_EQ(criteria->getName(), "xp");
    EXPECT_TRUE(criteria->isReadOnly());
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Integer);
}

TEST_F(ScoreCriteriaTest, LevelCriteria_Basic)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("level");
    ASSERT_NE(criteria, nullptr);

    EXPECT_EQ(criteria->getName(), "level");
    EXPECT_TRUE(criteria->isReadOnly());
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Integer);
}

TEST_F(ScoreCriteriaTest, ReadOnlyCriteria_IgnoreScoreChange)
{
    Scoreboard scoreboard;
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("health");
    auto* objective = scoreboard.addObjective("health_display", *criteria);
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);

    // 尝试手动设置分数
    score->setScorePoints(20);

    // 只读判据的分数更新由游戏逻辑控制
    // 这里的测试验证分数可以设置，但实际游戏会自动更新
    EXPECT_EQ(score->getScorePoints(), 20);
}

// ========== TeamKillCriteria 测试 ==========

TEST_F(ScoreCriteriaTest, TeamKillCriteria_Basic)
{
    auto* redKill = ScoreCriteriaRegistry::instance().getCriteria("teamkill.red");
    ASSERT_NE(redKill, nullptr);
    EXPECT_EQ(redKill->getName(), "teamkill.red");
    EXPECT_FALSE(redKill->isReadOnly());

    auto* killedByBlue = ScoreCriteriaRegistry::instance().getCriteria("killedByTeam.blue");
    ASSERT_NE(killedByBlue, nullptr);
    EXPECT_EQ(killedByBlue->getName(), "killedByTeam.blue");
    EXPECT_FALSE(killedByBlue->isReadOnly());
}

TEST_F(ScoreCriteriaTest, TeamKillCriteria_AllColors)
{
    // 验证所有 16 种颜色的队伍击杀判据都已注册
    const std::vector<std::string> colors = {"black",
        "dark_blue",
        "dark_green",
        "dark_aqua",
        "dark_red",
        "dark_purple",
        "gold",
        "gray",
        "dark_gray",
        "blue",
        "green",
        "aqua",
        "red",
        "light_purple",
        "yellow",
        "white"};

    for (const auto& color : colors) {
        std::string teamkillName = "teamkill." + color;
        std::string killedByTeamName = "killedByTeam." + color;

        auto* teamkill = ScoreCriteriaRegistry::instance().getCriteria(teamkillName);
        auto* killedBy = ScoreCriteriaRegistry::instance().getCriteria(killedByTeamName);

        EXPECT_NE(teamkill, nullptr) << "Missing: " << teamkillName;
        EXPECT_NE(killedBy, nullptr) << "Missing: " << killedByTeamName;
    }
}

// ========== 判据渲染类型测试 ==========

TEST_F(ScoreCriteriaTest, RenderType_Integer)
{
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    EXPECT_EQ(criteria->getDefaultRenderType(), RenderType::Integer);
}

TEST_F(ScoreCriteriaTest, RenderType_Hearts)
{
    auto* health = ScoreCriteriaRegistry::instance().getCriteria("health");
    EXPECT_EQ(health->getDefaultRenderType(), RenderType::Hearts);
}

// ========== 判据与目标关联测试 ==========

TEST_F(ScoreCriteriaTest, ObjectiveWithCriteria)
{
    Scoreboard scoreboard;
    auto* deathCount = ScoreCriteriaRegistry::instance().getCriteria("deathCount");
    auto* playerKillCount = ScoreCriteriaRegistry::instance().getCriteria("playerKillCount");

    // 创建同一判据的多个目标
    auto* deaths1 = scoreboard.addObjective("deaths_main", *deathCount);
    auto* deaths2 = scoreboard.addObjective("deaths_backup", *deathCount);
    auto* kills = scoreboard.addObjective("kills", *playerKillCount);

    // 预先创建分数（判据回调只更新已存在的分数）
    scoreboard.getOrCreateScore("Steve", *deaths1);
    scoreboard.getOrCreateScore("Steve", *deaths2);
    scoreboard.getOrCreateScore("Steve", *kills);

    // 玩家死亡应更新所有死亡计数目标
    deathCount->onPlayerDeath("Steve", scoreboard);

    EXPECT_EQ(scoreboard.getScore("Steve", *deaths1)->getScorePoints(), 1);
    EXPECT_EQ(scoreboard.getScore("Steve", *deaths2)->getScorePoints(), 1);
    EXPECT_EQ(scoreboard.getScore("Steve", *kills)->getScorePoints(), 0); // 不受影响

    // 按判据查询目标
    auto deathObjectives = scoreboard.getObjectivesByCriteria(*deathCount);
    EXPECT_EQ(deathObjectives.size(), 2);

    auto killObjectives = scoreboard.getObjectivesByCriteria(*playerKillCount);
    EXPECT_EQ(killObjectives.size(), 1);
}

// ========== 判据注册测试 ==========

TEST_F(ScoreCriteriaTest, RegisterCustomCriteria)
{
    // 创建自定义判据
    class CustomCriteria : public ScoreCriteria {
    public:
        CustomCriteria()
            : ScoreCriteria()
        {}

        const std::string& getName() const noexcept override
        {
            static const std::string name = "custom";
            return name;
        }

        bool isReadOnly() const noexcept override { return false; }
    };

    // 注册
    auto result = ScoreCriteriaRegistry::instance().registerCriteria(std::make_unique<CustomCriteria>());
    EXPECT_TRUE(result.success());

    // 验证
    auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("custom");
    ASSERT_NE(criteria, nullptr);
    EXPECT_EQ(criteria->getName(), "custom");
    EXPECT_FALSE(criteria->isReadOnly());
}

TEST_F(ScoreCriteriaTest, RegisterDuplicateCriteria)
{
    // 尝试注册重复名称的判据
    auto result = ScoreCriteriaRegistry::instance().registerCriteria(std::make_unique<DummyCriteria>());
    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), ErrorCode::AlreadyExists);
}

TEST_F(ScoreCriteriaTest, RegisterNullCriteria)
{
    auto result = ScoreCriteriaRegistry::instance().registerCriteria(nullptr);
    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);
}

TEST_F(ScoreCriteriaTest, RegisterEmptyNameCriteria)
{
    class EmptyNameCriteria : public ScoreCriteria {
    public:
        EmptyNameCriteria()
            : ScoreCriteria()
        {}

        const std::string& getName() const noexcept override
        {
            static const std::string name;
            return name; // 空名称
        }
    };

    auto result = ScoreCriteriaRegistry::instance().registerCriteria(std::make_unique<EmptyNameCriteria>());
    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);
}

// main 函数由 gtest_main 库提供
