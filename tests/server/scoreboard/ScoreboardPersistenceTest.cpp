#include <gtest/gtest.h>

#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/scoreboard/core/TeamEnums.hpp"
#include "common/scoreboard/storage/ScoreboardSaveData.hpp"
#include "common/scoreboard/criteria/DummyCriteria.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/util/nbt/Nbt.hpp"

using namespace mc;
using namespace mc::scoreboard;

/**
 * @brief 记分板持久化测试套件
 *
 * 测试 ScoreboardSaveData 的 NBT 序列化和反序列化：
 * - 目标数据的序列化/反序列化
 * - 分数数据的序列化/反序列化
 * - 队伍数据的序列化/反序列化
 * - 显示槽数据的序列化/反序列化
 * - 完整记分板的保存和加载
 */
class ScoreboardPersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 注册内置判据
        ScoreCriteriaRegistry::instance().registerBuiltinCriteria();
    }

    void TearDown() override {
        // 清理判据注册表
        ScoreCriteriaRegistry::instance().clear();
    }
};

// ========== 目标数据序列化测试 ==========

TEST_F(ScoreboardPersistenceTest, ObjectiveData_Serialize) {
    ScoreboardSaveData::ObjectiveData data;
    data.name = "kills";
    data.criteriaName = "dummy";
    data.displayName = "Player Kills";
    data.renderType = "integer";

    nbt::tags::compound_tag tag = data.toNbt();

    // 验证 NBT 结构
    auto* nameTag = dynamic_cast<const nbt::tags::string_tag*>(tag.value.find("Name")->second.get());
    ASSERT_NE(nameTag, nullptr);
    EXPECT_EQ(nameTag->value, "kills");

    auto* criteriaTag = dynamic_cast<const nbt::tags::string_tag*>(tag.value.find("CriteriaName")->second.get());
    ASSERT_NE(criteriaTag, nullptr);
    EXPECT_EQ(criteriaTag->value, "dummy");

    auto* displayTag = dynamic_cast<const nbt::tags::string_tag*>(tag.value.find("DisplayName")->second.get());
    ASSERT_NE(displayTag, nullptr);
    EXPECT_EQ(displayTag->value, "Player Kills");

    auto* renderTag = dynamic_cast<const nbt::tags::string_tag*>(tag.value.find("RenderType")->second.get());
    ASSERT_NE(renderTag, nullptr);
    EXPECT_EQ(renderTag->value, "integer");
}

TEST_F(ScoreboardPersistenceTest, ObjectiveData_Deserialize) {
    nbt::tags::compound_tag tag;
    tag.put("Name", std::string("deaths"));
    tag.put("CriteriaName", std::string("deathCount"));
    tag.put("DisplayName", std::string("Death Count"));
    tag.put("RenderType", std::string("hearts"));

    auto result = ScoreboardSaveData::ObjectiveData::fromNbt(tag);
    ASSERT_TRUE(result.success());

    auto data = std::move(result).value();
    EXPECT_EQ(data.name, "deaths");
    EXPECT_EQ(data.criteriaName, "deathCount");
    EXPECT_EQ(data.displayName, "Death Count");
    EXPECT_EQ(data.renderType, "hearts");
}

TEST_F(ScoreboardPersistenceTest, ObjectiveData_MissingFields) {
    // 缺少 Name 字段
    nbt::tags::compound_tag tag1;
    tag1.put("CriteriaName", std::string("dummy"));
    auto result1 = ScoreboardSaveData::ObjectiveData::fromNbt(tag1);
    EXPECT_FALSE(result1.success());

    // 缺少 CriteriaName 字段
    nbt::tags::compound_tag tag2;
    tag2.put("Name", std::string("test"));
    auto result2 = ScoreboardSaveData::ObjectiveData::fromNbt(tag2);
    EXPECT_FALSE(result2.success());
}

TEST_F(ScoreboardPersistenceTest, ObjectiveData_Defaults) {
    nbt::tags::compound_tag tag;
    tag.put("Name", std::string("test"));
    tag.put("CriteriaName", std::string("dummy"));
    // 不提供 DisplayName 和 RenderType

    auto result = ScoreboardSaveData::ObjectiveData::fromNbt(tag);
    ASSERT_TRUE(result.success());

    auto data = std::move(result).value();
    EXPECT_EQ(data.displayName, "test");  // 默认使用 name
    EXPECT_EQ(data.renderType, "integer");  // 默认 integer
}

// ========== 分数数据序列化测试 ==========

TEST_F(ScoreboardPersistenceTest, ScoreData_Serialize) {
    ScoreboardSaveData::ScoreData data;
    data.playerName = "Steve";
    data.objectiveName = "kills";
    data.score = 42;
    data.locked = true;

    nbt::tags::compound_tag tag = data.toNbt();

    auto* nameTag = dynamic_cast<const nbt::tags::string_tag*>(tag.value.find("Name")->second.get());
    ASSERT_NE(nameTag, nullptr);
    EXPECT_EQ(nameTag->value, "Steve");

    auto* objectiveTag = dynamic_cast<const nbt::tags::string_tag*>(tag.value.find("Objective")->second.get());
    ASSERT_NE(objectiveTag, nullptr);
    EXPECT_EQ(objectiveTag->value, "kills");

    auto* scoreTag = dynamic_cast<const nbt::tags::int_tag*>(tag.value.find("Score")->second.get());
    ASSERT_NE(scoreTag, nullptr);
    EXPECT_EQ(scoreTag->value, 42);

    auto* lockedTag = dynamic_cast<const nbt::tags::byte_tag*>(tag.value.find("Locked")->second.get());
    ASSERT_NE(lockedTag, nullptr);
    EXPECT_EQ(lockedTag->value, 1);
}

TEST_F(ScoreboardPersistenceTest, ScoreData_Deserialize) {
    nbt::tags::compound_tag tag;
    tag.put("Name", std::string("Alex"));
    tag.put("Objective", std::string("deaths"));
    tag.put("Score", static_cast<i32>(10));
    auto lockedTag = std::make_unique<nbt::tags::byte_tag>();
    lockedTag->value = 0;
    tag.value.emplace("Locked", std::move(lockedTag));

    auto result = ScoreboardSaveData::ScoreData::fromNbt(tag);
    ASSERT_TRUE(result.success());

    auto data = std::move(result).value();
    EXPECT_EQ(data.playerName, "Alex");
    EXPECT_EQ(data.objectiveName, "deaths");
    EXPECT_EQ(data.score, 10);
    EXPECT_FALSE(data.locked);
}

// ========== 队伍数据序列化测试 ==========

TEST_F(ScoreboardPersistenceTest, TeamData_Serialize) {
    ScoreboardSaveData::TeamData data;
    data.name = "red";
    data.displayName = "Red Team";
    data.prefix = "[RED]";
    data.suffix = "!";
    data.color = "red";
    data.nameTagVisibility = "always";
    data.deathMessageVisibility = "never";
    data.collisionRule = "pushOtherTeams";
    data.allowFriendlyFire = false;
    data.seeFriendlyInvisibles = true;
    data.members = {"Steve", "Alex", "Bob"};

    nbt::tags::compound_tag tag = data.toNbt();

    // 验证关键字段
    auto* nameTag = dynamic_cast<const nbt::tags::string_tag*>(tag.value.find("TeamName")->second.get());
    ASSERT_NE(nameTag, nullptr);
    EXPECT_EQ(nameTag->value, "red");

    auto* colorTag = dynamic_cast<const nbt::tags::string_tag*>(tag.value.find("TeamColor")->second.get());
    ASSERT_NE(colorTag, nullptr);
    EXPECT_EQ(colorTag->value, "red");

    auto* membersTag = dynamic_cast<const nbt::tags::string_list_tag*>(tag.value.find("Members")->second.get());
    ASSERT_NE(membersTag, nullptr);
    EXPECT_EQ(membersTag->value.size(), 3);
    EXPECT_EQ(membersTag->value[0], "Steve");
    EXPECT_EQ(membersTag->value[1], "Alex");
    EXPECT_EQ(membersTag->value[2], "Bob");
}

TEST_F(ScoreboardPersistenceTest, TeamData_Deserialize) {
    nbt::tags::compound_tag tag;
    tag.put("TeamName", std::string("blue"));
    tag.put("DisplayName", std::string("Blue Team"));
    tag.put("TeamColor", std::string("blue"));
    tag.put("NameTagVisibility", std::string("hideForOtherTeams"));
    tag.put("DeathMessageVisibility", std::string("always"));
    tag.put("CollisionRule", std::string("never"));

    auto friendlyFireTag = std::make_unique<nbt::tags::byte_tag>();
    friendlyFireTag->value = 1;
    tag.value.emplace("AllowFriendlyFire", std::move(friendlyFireTag));

    auto seeInvisTag = std::make_unique<nbt::tags::byte_tag>();
    seeInvisTag->value = 0;
    tag.value.emplace("SeeFriendlyInvisibles", std::move(seeInvisTag));

    auto membersTag = std::make_unique<nbt::tags::string_list_tag>();
    membersTag->value.push_back("Player1");
    membersTag->value.push_back("Player2");
    tag.value.emplace("Members", std::move(membersTag));

    auto result = ScoreboardSaveData::TeamData::fromNbt(tag);
    ASSERT_TRUE(result.success());

    auto data = std::move(result).value();
    EXPECT_EQ(data.name, "blue");
    EXPECT_EQ(data.displayName, "Blue Team");
    EXPECT_EQ(data.color, "blue");
    EXPECT_EQ(data.nameTagVisibility, "hideForOtherTeams");
    EXPECT_TRUE(data.allowFriendlyFire);
    EXPECT_FALSE(data.seeFriendlyInvisibles);
    EXPECT_EQ(data.members.size(), 2);
    EXPECT_EQ(data.members[0], "Player1");
    EXPECT_EQ(data.members[1], "Player2");
}

// ========== 显示槽数据序列化测试 ==========

TEST_F(ScoreboardPersistenceTest, DisplaySlotData_Serialize) {
    ScoreboardSaveData::DisplaySlotData data;
    data.slot = 1;  // Sidebar
    data.objectiveName = "kills";

    nbt::tags::compound_tag tag = data.toNbt();

    auto* slotTag = dynamic_cast<const nbt::tags::int_tag*>(tag.value.find("Slot")->second.get());
    ASSERT_NE(slotTag, nullptr);
    EXPECT_EQ(slotTag->value, 1);

    auto* objectiveTag = dynamic_cast<const nbt::tags::string_tag*>(tag.value.find("Objective")->second.get());
    ASSERT_NE(objectiveTag, nullptr);
    EXPECT_EQ(objectiveTag->value, "kills");
}

TEST_F(ScoreboardPersistenceTest, DisplaySlotData_Deserialize) {
    nbt::tags::compound_tag tag;
    tag.put("Slot", static_cast<i32>(0));  // List
    tag.put("Objective", std::string("deaths"));

    auto result = ScoreboardSaveData::DisplaySlotData::fromNbt(tag);
    ASSERT_TRUE(result.success());

    auto data = std::move(result).value();
    EXPECT_EQ(data.slot, 0);
    EXPECT_EQ(data.objectiveName, "deaths");
}

// ========== 完整记分板序列化测试 ==========

TEST_F(ScoreboardPersistenceTest, Scoreboard_RoundTrip) {
    // 创建原始记分板
    Scoreboard original;
    auto* dummy = ScoreCriteriaRegistry::instance().getCriteria("dummy");
    auto* deathCount = ScoreCriteriaRegistry::instance().getCriteria("deathCount");

    // 创建目标
    auto* kills = original.addObjective("kills", *dummy);
    auto* deaths = original.addObjective("deaths", *deathCount);
    kills->setRenderType(RenderType::Integer);

    // 设置分数
    original.getOrCreateScore("Steve", *kills)->setScorePoints(100);
    original.getOrCreateScore("Alex", *kills)->setScorePoints(50);
    original.getOrCreateScore("Steve", *deaths)->setScorePoints(10);
    original.getOrCreateScore("Alex", *deaths)->setScorePoints(5);

    // 创建队伍
    auto* red = original.createTeam("red");
    red->setColor(TextFormatting::Red);
    red->setAllowFriendlyFire(false);
    original.addPlayerToTeam("Steve", *red);
    original.addPlayerToTeam("Bob", *red);

    // 设置显示槽位
    original.setObjectiveInDisplaySlot(DisplaySlot::Sidebar, kills);
    original.setObjectiveInDisplaySlot(DisplaySlot::List, deaths);

    // 序列化
    ScoreboardSaveData saveData = ScoreboardSaveData::fromScoreboard(original);
    nbt::tags::compound_tag nbt = saveData.toNbt();

    // 验证 NBT 数据
    EXPECT_TRUE(nbt.value.count("Objectives") > 0);
    EXPECT_TRUE(nbt.value.count("PlayerScores") > 0);
    EXPECT_TRUE(nbt.value.count("Teams") > 0);
    EXPECT_TRUE(nbt.value.count("DisplaySlots") > 0);

    // 反序列化到新记分板
    auto loadResult = ScoreboardSaveData::fromNbt(nbt);
    ASSERT_TRUE(loadResult.success());

    Scoreboard restored;
    auto applyResult = loadResult.value().applyToScoreboard(restored);
    EXPECT_TRUE(applyResult.success());

    // 验证目标
    EXPECT_TRUE(restored.hasObjective("kills"));
    EXPECT_TRUE(restored.hasObjective("deaths"));

    auto* restoredKills = restored.getObjective("kills");
    ASSERT_NE(restoredKills, nullptr);
    EXPECT_EQ(restoredKills->getCriteria().getName(), "dummy");
    EXPECT_EQ(restoredKills->getRenderType(), RenderType::Integer);

    auto* restoredDeaths = restored.getObjective("deaths");
    ASSERT_NE(restoredDeaths, nullptr);
    EXPECT_EQ(restoredDeaths->getCriteria().getName(), "deathCount");

    // 验证分数
    auto* steveKills = restored.getScore("Steve", *restoredKills);
    ASSERT_NE(steveKills, nullptr);
    EXPECT_EQ(steveKills->getScorePoints(), 100);

    auto* alexKills = restored.getScore("Alex", *restoredKills);
    ASSERT_NE(alexKills, nullptr);
    EXPECT_EQ(alexKills->getScorePoints(), 50);

    auto* steveDeaths = restored.getScore("Steve", *restoredDeaths);
    ASSERT_NE(steveDeaths, nullptr);
    EXPECT_EQ(steveDeaths->getScorePoints(), 10);

    // 验证队伍
    EXPECT_TRUE(restored.hasTeam("red"));
    auto* restoredRed = restored.getTeam("red");
    ASSERT_NE(restoredRed, nullptr);
    EXPECT_EQ(restoredRed->getColor(), TextFormatting::Red);
    EXPECT_FALSE(restoredRed->getAllowFriendlyFire());
    EXPECT_TRUE(restoredRed->hasMember("Steve"));
    EXPECT_TRUE(restoredRed->hasMember("Bob"));

    // 验证显示槽位
    EXPECT_EQ(restored.getObjectiveInDisplaySlot(DisplaySlot::Sidebar), restoredKills);
    EXPECT_EQ(restored.getObjectiveInDisplaySlot(DisplaySlot::List), restoredDeaths);
}

// ========== 二进制序列化测试 ==========

TEST_F(ScoreboardPersistenceTest, BinarySerialization) {
    // 创建保存数据
    ScoreboardSaveData::ObjectiveData obj;
    obj.name = "test";
    obj.criteriaName = "dummy";
    obj.displayName = "Test Objective";
    obj.renderType = "integer";

    ScoreboardSaveData::ScoreData score;
    score.playerName = "Steve";
    score.objectiveName = "test";
    score.score = 100;
    score.locked = false;

    ScoreboardSaveData saveData;
    saveData.addObjective(std::move(obj));
    saveData.addScore(std::move(score));

    // 序列化为二进制
    auto bytesResult = saveData.serialize();
    ASSERT_TRUE(bytesResult.success());
    auto bytes = std::move(bytesResult).value();

    EXPECT_FALSE(bytes.empty());

    // 反序列化
    auto loadResult = ScoreboardSaveData::deserialize(bytes);
    ASSERT_TRUE(loadResult.success());

    auto& loaded = loadResult.value();
    EXPECT_EQ(loaded.objectives().size(), 1);
    EXPECT_EQ(loaded.objectives()[0].name, "test");
    EXPECT_EQ(loaded.scores().size(), 1);
    EXPECT_EQ(loaded.scores()[0].playerName, "Steve");
    EXPECT_EQ(loaded.scores()[0].score, 100);
}

// ========== 边界情况测试 ==========

TEST_F(ScoreboardPersistenceTest, EmptyScoreboard) {
    Scoreboard empty;
    ScoreboardSaveData saveData = ScoreboardSaveData::fromScoreboard(empty);

    nbt::tags::compound_tag nbt = saveData.toNbt();

    auto loadResult = ScoreboardSaveData::fromNbt(nbt);
    ASSERT_TRUE(loadResult.success());

    Scoreboard restored;
    auto applyResult = loadResult.value().applyToScoreboard(restored);
    EXPECT_TRUE(applyResult.success());

    EXPECT_EQ(restored.getObjectives().size(), 0);
    EXPECT_EQ(restored.getTeams().size(), 0);
}

TEST_F(ScoreboardPersistenceTest, MissingCriteria) {
    // 创建包含未知判据的数据
    ScoreboardSaveData::ObjectiveData obj;
    obj.name = "test";
    obj.criteriaName = "nonexistent_criteria";  // 不存在的判据
    obj.displayName = "Test";
    obj.renderType = "integer";

    ScoreboardSaveData saveData;
    saveData.addObjective(std::move(obj));

    nbt::tags::compound_tag nbt = saveData.toNbt();
    auto loadResult = ScoreboardSaveData::fromNbt(nbt);
    ASSERT_TRUE(loadResult.success());

    // 应用到记分板（应使用 dummy 判据）
    Scoreboard scoreboard;
    auto applyResult = loadResult.value().applyToScoreboard(scoreboard);
    EXPECT_TRUE(applyResult.success());

    // 目标应使用 dummy 判据创建
    auto* objective = scoreboard.getObjective("test");
    ASSERT_NE(objective, nullptr);
    EXPECT_EQ(objective->getCriteria().getName(), "dummy");
}

// ========== 完整工作流测试 ==========

TEST_F(ScoreboardPersistenceTest, FullWorkflow) {
    // 1. 创建记分板并添加数据
    Scoreboard scoreboard;
    auto* dummy = ScoreCriteriaRegistry::instance().getCriteria("dummy");

    // 创建多个目标
    auto* points = scoreboard.addObjective("points", *dummy);
    auto* wins = scoreboard.addObjective("wins", *dummy);
    auto* losses = scoreboard.addObjective("losses", *dummy);

    // 设置分数
    for (int i = 0; i < 10; ++i) {
        std::string player = "Player" + std::to_string(i);
        scoreboard.getOrCreateScore(player, *points)->setScorePoints(i * 100);
        scoreboard.getOrCreateScore(player, *wins)->setScorePoints(i % 3);
        scoreboard.getOrCreateScore(player, *losses)->setScorePoints(i % 5);
    }

    // 创建多个队伍
    auto* red = scoreboard.createTeam("red");
    red->setColor(TextFormatting::Red);
    red->setAllowFriendlyFire(false);

    auto* blue = scoreboard.createTeam("blue");
    blue->setColor(TextFormatting::Blue);
    blue->setNameTagVisibility(TeamVisibility::HideForOtherTeams);

    // 分配玩家到队伍
    for (int i = 0; i < 10; ++i) {
        std::string player = "Player" + std::to_string(i);
        if (i % 2 == 0) {
            scoreboard.addPlayerToTeam(player, *red);
        } else {
            scoreboard.addPlayerToTeam(player, *blue);
        }
    }

    // 设置显示槽位
    scoreboard.setObjectiveInDisplaySlot(DisplaySlot::Sidebar, points);

    // 2. 保存
    ScoreboardSaveData saveData = ScoreboardSaveData::fromScoreboard(scoreboard);
    auto bytesResult = saveData.serialize();
    ASSERT_TRUE(bytesResult.success());

    // 3. 清空原记分板
    for (auto* obj : scoreboard.getObjectives()) {
        scoreboard.removeObjective(*obj);
    }
    for (auto* team : scoreboard.getTeams()) {
        scoreboard.removeTeam(*team);
    }

    EXPECT_EQ(scoreboard.getObjectives().size(), 0);
    EXPECT_EQ(scoreboard.getTeams().size(), 0);

    // 4. 加载
    auto loadResult = ScoreboardSaveData::deserialize(bytesResult.value());
    ASSERT_TRUE(loadResult.success());

    Scoreboard restored;
    auto applyResult = loadResult.value().applyToScoreboard(restored);
    EXPECT_TRUE(applyResult.success());

    // 5. 验证数据完整性
    EXPECT_EQ(restored.getObjectives().size(), 3);
    EXPECT_EQ(restored.getTeams().size(), 2);

    // 验证分数
    auto* restoredPoints = restored.getObjective("points");
    ASSERT_NE(restoredPoints, nullptr);
    EXPECT_EQ(restored.getSortedScores(*restoredPoints).size(), 10);

    // 验证队伍成员
    auto* restoredRed = restored.getTeam("red");
    ASSERT_NE(restoredRed, nullptr);
    EXPECT_EQ(restoredRed->getMembers().size(), 5);  // Player0, 2, 4, 6, 8

    auto* restoredBlue = restored.getTeam("blue");
    ASSERT_NE(restoredBlue, nullptr);
    EXPECT_EQ(restoredBlue->getMembers().size(), 5);  // Player1, 3, 5, 7, 9
}

// main 函数由 gtest_main 库提供
