#include <gtest/gtest.h>

#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/scoreboard/core/TeamEnums.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"

using namespace mc;
using namespace mc::scoreboard;
using namespace mc::text;

/**
 * @brief 队伍系统测试套件
 *
 * 测试 ScorePlayerTeam 的功能：
 * - 队伍创建和基本属性
 * - 成员管理
 * - 队伍属性设置（颜色、前缀、后缀、可见性等）
 * - 友军设置
 * - 格式化名称
 */
class TeamTest : public ::testing::Test {
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

// ========== 队伍创建测试 ==========

TEST_F(TeamTest, CreateTeam) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);
    EXPECT_EQ(team->getName(), "red");
    EXPECT_TRUE(scoreboard.hasTeam("red"));
}

TEST_F(TeamTest, CreateDuplicateTeam) {
    Scoreboard scoreboard;
    scoreboard.createTeam("red");
    auto* duplicate = scoreboard.createTeam("red");
    EXPECT_EQ(duplicate, nullptr);
}

TEST_F(TeamTest, CreateTeamWithInvalidName) {
    Scoreboard scoreboard;

    // 空名称
    EXPECT_EQ(scoreboard.createTeam(""), nullptr);

    // 包含空格
    EXPECT_EQ(scoreboard.createTeam("invalid name"), nullptr);

    // 超长名称
    std::string longName(17, 'a');
    EXPECT_EQ(scoreboard.createTeam(longName), nullptr);
}

// ========== 显示名称测试 ==========

TEST_F(TeamTest, DisplayName) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认显示名称为队伍名称
    auto* displayName = team->getDisplayName();
    ASSERT_NE(displayName, nullptr);
    EXPECT_EQ(displayName->getUnformattedText(), "red");

    // 设置显示名称
    team->setDisplayName(std::make_unique<StringTextComponent>("Red Team"));
    displayName = team->getDisplayName();
    ASSERT_NE(displayName, nullptr);
    EXPECT_EQ(displayName->getUnformattedText(), "Red Team");
}

// ========== 成员管理测试 ==========

TEST_F(TeamTest, AddMember) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 添加成员
    EXPECT_TRUE(team->addMember("Steve"));
    EXPECT_TRUE(team->hasMember("Steve"));
    EXPECT_EQ(team->getMembers().size(), 1);

    // 重复添加
    EXPECT_FALSE(team->addMember("Steve"));
    EXPECT_EQ(team->getMembers().size(), 1);

    // 添加多个成员
    EXPECT_TRUE(team->addMember("Alex"));
    EXPECT_TRUE(team->addMember("Bob"));
    EXPECT_EQ(team->getMembers().size(), 3);
}

TEST_F(TeamTest, RemoveMember) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    team->addMember("Steve");
    team->addMember("Alex");

    // 移除成员
    EXPECT_TRUE(team->removeMember("Steve"));
    EXPECT_FALSE(team->hasMember("Steve"));
    EXPECT_EQ(team->getMembers().size(), 1);

    // 重复移除
    EXPECT_FALSE(team->removeMember("Steve"));

    // 移除不存在的成员
    EXPECT_FALSE(team->removeMember("NonExistent"));
}

TEST_F(TeamTest, ClearMembers) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    team->addMember("Steve");
    team->addMember("Alex");
    team->addMember("Bob");

    // 清空成员
    team->clearMembers();
    EXPECT_EQ(team->getMembers().size(), 0);
    EXPECT_FALSE(team->hasMember("Steve"));
    EXPECT_FALSE(team->hasMember("Alex"));
    EXPECT_FALSE(team->hasMember("Bob"));
}

TEST_F(TeamTest, GetMembers) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    team->addMember("Steve");
    team->addMember("Alex");
    team->addMember("Bob");

    const auto& members = team->getMembers();
    EXPECT_EQ(members.size(), 3);
    EXPECT_TRUE(members.count("Steve") > 0);
    EXPECT_TRUE(members.count("Alex") > 0);
    EXPECT_TRUE(members.count("Bob") > 0);
}

// ========== 队伍颜色测试 ==========

TEST_F(TeamTest, TeamColor) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认颜色为白色
    EXPECT_EQ(team->getColor(), TextFormatting::White);

    // 设置颜色
    team->setColor(TextFormatting::Red);
    EXPECT_EQ(team->getColor(), TextFormatting::Red);

    team->setColor(TextFormatting::Blue);
    EXPECT_EQ(team->getColor(), TextFormatting::Blue);
}

// ========== 前缀和后缀测试 ==========

TEST_F(TeamTest, PrefixAndSuffix) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认前缀和后缀为空
    auto* prefix = team->getPrefix();
    auto* suffix = team->getSuffix();
    ASSERT_NE(prefix, nullptr);
    ASSERT_NE(suffix, nullptr);
    EXPECT_EQ(prefix->getUnformattedText(), "");
    EXPECT_EQ(suffix->getUnformattedText(), "");

    // 设置前缀
    team->setPrefix(std::make_unique<StringTextComponent>("[RED] "));
    prefix = team->getPrefix();
    ASSERT_NE(prefix, nullptr);
    EXPECT_EQ(prefix->getUnformattedText(), "[RED] ");

    // 设置后缀
    team->setSuffix(std::make_unique<StringTextComponent>("!"));
    suffix = team->getSuffix();
    ASSERT_NE(suffix, nullptr);
    EXPECT_EQ(suffix->getUnformattedText(), "!");

    // 设置空前缀（应使用空字符串）
    team->setPrefix(nullptr);
    prefix = team->getPrefix();
    ASSERT_NE(prefix, nullptr);
    EXPECT_EQ(prefix->getUnformattedText(), "");
}

// ========== 友军设置测试 ==========

TEST_F(TeamTest, AllowFriendlyFire) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认允许友军伤害
    EXPECT_TRUE(team->getAllowFriendlyFire());

    // 设置不允许友军伤害
    team->setAllowFriendlyFire(false);
    EXPECT_FALSE(team->getAllowFriendlyFire());

    team->setAllowFriendlyFire(true);
    EXPECT_TRUE(team->getAllowFriendlyFire());
}

TEST_F(TeamTest, SeeFriendlyInvisibles) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认可以看到隐身的友军
    EXPECT_TRUE(team->canSeeFriendlyInvisibles());

    // 设置不能看到隐身的友军
    team->setSeeFriendlyInvisibles(false);
    EXPECT_FALSE(team->canSeeFriendlyInvisibles());

    team->setSeeFriendlyInvisibles(true);
    EXPECT_TRUE(team->canSeeFriendlyInvisibles());
}

TEST_F(TeamTest, FriendlyFlags) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认: 允许友军伤害 + 可以看到隐身友军
    // flags = 0x01 (friendlyFire) | 0x02 (seeFriendlyInvisibles) = 0x03
    EXPECT_EQ(team->getFriendlyFlags(), 0x03);

    // 设置不允许友军伤害
    team->setAllowFriendlyFire(false);
    EXPECT_EQ(team->getFriendlyFlags(), 0x02);  // 只有 seeFriendlyInvisibles

    // 设置不能看到隐身友军
    team->setSeeFriendlyInvisibles(false);
    EXPECT_EQ(team->getFriendlyFlags(), 0x00);  // 两者都关闭

    // 通过 flags 设置
    team->setFriendlyFlags(0x01);
    EXPECT_TRUE(team->getAllowFriendlyFire());
    EXPECT_FALSE(team->canSeeFriendlyInvisibles());
}

// ========== 可见性设置测试 ==========

TEST_F(TeamTest, NameTagVisibility) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认总是显示名称标签
    EXPECT_EQ(team->getNameTagVisibility(), TeamVisibility::Always);

    // 设置不同可见性
    team->setNameTagVisibility(TeamVisibility::Never);
    EXPECT_EQ(team->getNameTagVisibility(), TeamVisibility::Never);

    team->setNameTagVisibility(TeamVisibility::HideForOtherTeams);
    EXPECT_EQ(team->getNameTagVisibility(), TeamVisibility::HideForOtherTeams);

    team->setNameTagVisibility(TeamVisibility::HideForOwnTeam);
    EXPECT_EQ(team->getNameTagVisibility(), TeamVisibility::HideForOwnTeam);
}

TEST_F(TeamTest, DeathMessageVisibility) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认总是显示死亡消息
    EXPECT_EQ(team->getDeathMessageVisibility(), TeamVisibility::Always);

    // 设置不同可见性
    team->setDeathMessageVisibility(TeamVisibility::Never);
    EXPECT_EQ(team->getDeathMessageVisibility(), TeamVisibility::Never);

    team->setDeathMessageVisibility(TeamVisibility::HideForOtherTeams);
    EXPECT_EQ(team->getDeathMessageVisibility(), TeamVisibility::HideForOtherTeams);
}

// ========== 碰撞规则测试 ==========

TEST_F(TeamTest, CollisionRule) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认总是碰撞
    EXPECT_EQ(team->getCollisionRule(), TeamCollisionRule::Always);

    // 设置不同碰撞规则
    team->setCollisionRule(TeamCollisionRule::Never);
    EXPECT_EQ(team->getCollisionRule(), TeamCollisionRule::Never);

    team->setCollisionRule(TeamCollisionRule::PushOtherTeams);
    EXPECT_EQ(team->getCollisionRule(), TeamCollisionRule::PushOtherTeams);

    team->setCollisionRule(TeamCollisionRule::PushOwnTeam);
    EXPECT_EQ(team->getCollisionRule(), TeamCollisionRule::PushOwnTeam);
}

// ========== 格式化名称测试 ==========

TEST_F(TeamTest, FormatName) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 设置队伍颜色
    team->setColor(TextFormatting::Red);

    // 格式化名称
    StringTextComponent playerName("Steve");
    auto formatted = team->formatName(playerName);
    ASSERT_NE(formatted, nullptr);

    // 验证颜色已应用
    const auto& style = formatted->getStyle();
    EXPECT_EQ(style.getColor(), TextFormatting::Red);

    // 验证文本内容（无前缀后缀时只有名称）
    EXPECT_EQ(formatted->getUnformattedText(), "Steve");
}

TEST_F(TeamTest, FormatNameWithPrefixAndSuffix) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("admin");
    ASSERT_NE(team, nullptr);

    // 设置队伍颜色（金色）
    team->setColor(TextFormatting::Gold);

    // 设置前缀 [ADMIN]（绿色粗体）
    auto prefix = std::make_unique<StringTextComponent>("[ADMIN] ");
    Style prefixStyle;
    prefixStyle.setColor(TextFormatting::Green);
    prefixStyle.setBold(true);
    prefix->setStyle(prefixStyle);
    team->setPrefix(std::move(prefix));

    // 设置后缀 ★
    auto suffix = std::make_unique<StringTextComponent>(" ★");
    Style suffixStyle;
    suffixStyle.setColor(TextFormatting::Yellow);
    suffix->setStyle(suffixStyle);
    team->setSuffix(std::move(suffix));

    // 格式化名称
    StringTextComponent playerName("Steve");
    auto formatted = team->formatName(playerName);
    ASSERT_NE(formatted, nullptr);

    // 验证文本内容：前缀 + 名称 + 后缀
    EXPECT_EQ(formatted->getUnformattedText(), "[ADMIN] Steve ★");

    // 验证队伍颜色应用到根组件（金色）
    const auto& rootStyle = formatted->getStyle();
    EXPECT_EQ(rootStyle.getColor(), TextFormatting::Gold);

    // 验证有两个子组件（前缀、名称、后缀）
    const auto& siblings = formatted->getSiblings();
    EXPECT_EQ(siblings.size(), 3);  // prefix + name + suffix

    // 验证前缀的样式（绿色粗体）
    const auto& prefixComponent = siblings[0];
    EXPECT_EQ(prefixComponent->getUnformattedText(), "[ADMIN] ");
    EXPECT_EQ(prefixComponent->getStyle().getColor(), TextFormatting::Green);
    EXPECT_TRUE(prefixComponent->getStyle().isBold());

    // 验证后缀的样式（黄色）
    const auto& suffixComponent = siblings[2];
    EXPECT_EQ(suffixComponent->getUnformattedText(), " ★");
    EXPECT_EQ(suffixComponent->getStyle().getColor(), TextFormatting::Yellow);
}

TEST_F(TeamTest, FormatNameWithOnlyPrefix) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("vip");
    ASSERT_NE(team, nullptr);

    // 设置队伍颜色
    team->setColor(TextFormatting::Aqua);

    // 只设置前缀
    auto prefix = std::make_unique<StringTextComponent>("[VIP] ");
    team->setPrefix(std::move(prefix));

    // 格式化名称
    StringTextComponent playerName("Alex");
    auto formatted = team->formatName(playerName);
    ASSERT_NE(formatted, nullptr);

    // 验证文本内容
    EXPECT_EQ(formatted->getUnformattedText(), "[VIP] Alex");

    // 验证队伍颜色
    EXPECT_EQ(formatted->getStyle().getColor(), TextFormatting::Aqua);
}

TEST_F(TeamTest, FormatNameWithOnlySuffix) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("mod");
    ASSERT_NE(team, nullptr);

    // 设置队伍颜色
    team->setColor(TextFormatting::LightPurple);

    // 只设置后缀
    auto suffix = std::make_unique<StringTextComponent>(" [MOD]");
    team->setSuffix(std::move(suffix));

    // 格式化名称
    StringTextComponent playerName("Bob");
    auto formatted = team->formatName(playerName);
    ASSERT_NE(formatted, nullptr);

    // 验证文本内容
    EXPECT_EQ(formatted->getUnformattedText(), "Bob [MOD]");

    // 验证队伍颜色
    EXPECT_EQ(formatted->getStyle().getColor(), TextFormatting::LightPurple);
}

TEST_F(TeamTest, FormatNameWithResetColor) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("default");
    ASSERT_NE(team, nullptr);

    // 设置颜色为 Reset（不应应用到组件）
    team->setColor(TextFormatting::Reset);

    // 设置前缀和后缀
    team->setPrefix(std::make_unique<StringTextComponent>("[D] "));
    team->setSuffix(std::make_unique<StringTextComponent>(" [D]"));

    // 格式化名称
    StringTextComponent playerName("Player");
    auto formatted = team->formatName(playerName);
    ASSERT_NE(formatted, nullptr);

    // 验证文本内容
    EXPECT_EQ(formatted->getUnformattedText(), "[D] Player [D]");

    // 验证颜色未应用（Reset 不设置颜色）
    EXPECT_FALSE(formatted->getStyle().getColor().has_value());
}

// ========== 队伍移除测试 ==========

TEST_F(TeamTest, RemoveTeam) {
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    team->addMember("Steve");
    team->addMember("Alex");

    // 移除队伍
    scoreboard.removeTeam(*team);
    EXPECT_FALSE(scoreboard.hasTeam("red"));

    // 成员应被清空
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), nullptr);
    EXPECT_EQ(scoreboard.getPlayersTeam("Alex"), nullptr);
}

// ========== 多队伍测试 ==========

TEST_F(TeamTest, MultipleTeams) {
    Scoreboard scoreboard;
    auto* red = scoreboard.createTeam("red");
    auto* blue = scoreboard.createTeam("blue");
    auto* green = scoreboard.createTeam("green");

    ASSERT_NE(red, nullptr);
    ASSERT_NE(blue, nullptr);
    ASSERT_NE(green, nullptr);

    // 设置不同颜色
    red->setColor(TextFormatting::Red);
    blue->setColor(TextFormatting::Blue);
    green->setColor(TextFormatting::Green);

    // 添加成员
    scoreboard.addPlayerToTeam("Steve", *red);
    scoreboard.addPlayerToTeam("Alex", *blue);
    scoreboard.addPlayerToTeam("Bob", *green);

    // 验证队伍归属
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), red);
    EXPECT_EQ(scoreboard.getPlayersTeam("Alex"), blue);
    EXPECT_EQ(scoreboard.getPlayersTeam("Bob"), green);

    // 获取所有队伍
    auto teams = scoreboard.getTeams();
    EXPECT_EQ(teams.size(), 3);
}

// ========== 队伍切换测试 ==========

TEST_F(TeamTest, SwitchTeam) {
    Scoreboard scoreboard;
    auto* red = scoreboard.createTeam("red");
    auto* blue = scoreboard.createTeam("blue");

    // 加入红队
    scoreboard.addPlayerToTeam("Steve", *red);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), red);
    EXPECT_TRUE(red->hasMember("Steve"));
    EXPECT_EQ(red->getMembers().size(), 1);

    // 切换到蓝队
    scoreboard.addPlayerToTeam("Steve", *blue);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), blue);
    EXPECT_FALSE(red->hasMember("Steve"));
    EXPECT_TRUE(blue->hasMember("Steve"));
    EXPECT_EQ(red->getMembers().size(), 0);
    EXPECT_EQ(blue->getMembers().size(), 1);
}

// ========== 集成测试 ==========

TEST_F(TeamTest, FullTeamWorkflow) {
    Scoreboard scoreboard;

    // 1. 创建队伍
    auto* red = scoreboard.createTeam("red");
    red->setDisplayName(std::make_unique<StringTextComponent>("Red Team"));
    red->setColor(TextFormatting::Red);
    red->setPrefix(std::make_unique<StringTextComponent>("[R] "));
    red->setAllowFriendlyFire(false);
    red->setSeeFriendlyInvisibles(true);
    red->setNameTagVisibility(TeamVisibility::HideForOtherTeams);
    red->setCollisionRule(TeamCollisionRule::PushOwnTeam);

    // 2. 添加成员
    scoreboard.addPlayerToTeam("Steve", *red);
    scoreboard.addPlayerToTeam("Alex", *red);
    scoreboard.addPlayerToTeam("Bob", *red);

    // 3. 验证属性
    EXPECT_EQ(red->getName(), "red");
    EXPECT_EQ(red->getDisplayName()->getUnformattedText(), "Red Team");
    EXPECT_EQ(red->getColor(), TextFormatting::Red);
    EXPECT_EQ(red->getPrefix()->getUnformattedText(), "[R] ");
    EXPECT_FALSE(red->getAllowFriendlyFire());
    EXPECT_TRUE(red->canSeeFriendlyInvisibles());
    EXPECT_EQ(red->getNameTagVisibility(), TeamVisibility::HideForOtherTeams);
    EXPECT_EQ(red->getCollisionRule(), TeamCollisionRule::PushOwnTeam);
    EXPECT_EQ(red->getMembers().size(), 3);

    // 4. 验证成员归属
    for (const auto& member : red->getMembers()) {
        EXPECT_EQ(scoreboard.getPlayersTeam(member), red);
    }

    // 5. 移除成员
    scoreboard.removePlayerFromTeam("Bob", *red);
    EXPECT_EQ(red->getMembers().size(), 2);
    EXPECT_EQ(scoreboard.getPlayersTeam("Bob"), nullptr);

    // 6. 清理
    scoreboard.removeTeam(*red);
    EXPECT_FALSE(scoreboard.hasTeam("red"));
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), nullptr);
    EXPECT_EQ(scoreboard.getPlayersTeam("Alex"), nullptr);
}

// main 函数由 gtest_main 库提供
