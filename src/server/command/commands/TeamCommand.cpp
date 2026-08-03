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

#include "TeamCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/scoreboard/core/TeamEnums.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace mc {
namespace command {

// 使用 mc::server 命名空间中的 ServerScoreboard
using ::mc::server::ServerScoreboard;

// 辅助函数：获取服务端记分板
static ServerScoreboard* getScoreboard(ServerCommandSource& source)
{
    auto* server = source.server();
    if (!server) {
        return nullptr;
    }
    return &server->scoreboard();
}

// 辅助函数：从字符串解析颜色
static text::TextFormatting parseColor(const std::string& name)
{
    return text::fromName(name);
}

// 辅助函数：从字符串解析可见性
static scoreboard::TeamVisibility parseVisibility(const std::string& name)
{
    return scoreboard::teamVisibilityFromString(name);
}

// 辅助函数：从字符串解析碰撞规则
static scoreboard::TeamCollisionRule parseCollisionRule(const std::string& name)
{
    return scoreboard::teamCollisionRuleFromString(name);
}

void TeamCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto teamNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("team");
    teamNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(teamNode,
        support::makeMetadata("Manages teams.", "/team <add|remove|list|empty|join|leave|modify> ...", 2, {}, true));

    // /team add <team> [displayName]
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto teamNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", StringArgumentType::string());
    auto displayNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "displayName", StringArgumentType::greedyString());
    displayNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addTeam(ctx); });
    teamNameArg->addChild(displayNameArg);
    teamNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addTeam(ctx); });
    addNode->addChild(teamNameArg);
    teamNode->addChild(addNode);

    // /team remove <team>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", StringArgumentType::string());
    removeTeamArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeTeam(ctx); });
    removeNode->addChild(removeTeamArg);
    teamNode->addChild(removeNode);

    // /team list [team]
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    auto listTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", StringArgumentType::string());
    listTeamArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _listTeams(ctx); });
    listNode->addChild(listTeamArg);
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _listTeams(ctx); });
    teamNode->addChild(listNode);

    // /team empty <team>
    auto emptyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("empty");
    auto emptyTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", StringArgumentType::string());
    emptyTeamArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _emptyTeam(ctx); });
    emptyNode->addChild(emptyTeamArg);
    teamNode->addChild(emptyNode);

    // /team join <team> <members>
    auto joinNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("join");
    auto joinTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", StringArgumentType::string());
    auto membersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "members", EntityArgumentType::entities());
    membersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _joinTeam(ctx); });
    joinTeamArg->addChild(membersArg);
    joinNode->addChild(joinTeamArg);
    teamNode->addChild(joinNode);

    // /team leave <members>
    auto leaveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("leave");
    auto leaveMembersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "members", EntityArgumentType::entities());
    leaveMembersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _leaveTeam(ctx); });
    leaveNode->addChild(leaveMembersArg);
    teamNode->addChild(leaveNode);

    // /team modify <team> <property> <value>
    auto modifyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("modify");
    auto modifyTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", StringArgumentType::string());

    // modify <team> color <color>
    auto colorNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("color");
    auto colorValueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("value", StringArgumentType::string());
    colorValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _modifyTeam(ctx); });
    colorNode->addChild(colorValueArg);
    modifyTeamArg->addChild(colorNode);

    // modify <team> friendlyFire <true|false>
    auto friendlyFireNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("friendlyFire");
    auto friendlyFireArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("value", BoolArgumentType::boolArg());
    friendlyFireArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _modifyTeam(ctx); });
    friendlyFireNode->addChild(friendlyFireArg);
    modifyTeamArg->addChild(friendlyFireNode);

    // modify <team> seeFriendlyInvisibles <true|false>
    auto seeInvisNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("seeFriendlyInvisibles");
    auto seeInvisArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("value", BoolArgumentType::boolArg());
    seeInvisArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _modifyTeam(ctx); });
    seeInvisNode->addChild(seeInvisArg);
    modifyTeamArg->addChild(seeInvisNode);

    // modify <team> prefix <prefix>
    auto prefixNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("prefix");
    auto prefixArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "value", StringArgumentType::greedyString());
    prefixArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _modifyTeam(ctx); });
    prefixNode->addChild(prefixArg);
    modifyTeamArg->addChild(prefixNode);

    // modify <team> suffix <suffix>
    auto suffixNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("suffix");
    auto suffixArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "value", StringArgumentType::greedyString());
    suffixArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _modifyTeam(ctx); });
    suffixNode->addChild(suffixArg);
    modifyTeamArg->addChild(suffixNode);

    // modify <team> displayName <displayName>
    auto displayNameNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("displayName");
    auto displayNameValueArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "value", StringArgumentType::greedyString());
    displayNameValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _modifyTeam(ctx); });
    displayNameNode->addChild(displayNameValueArg);
    modifyTeamArg->addChild(displayNameNode);

    // modify <team> nametagVisibility <visibility>
    auto nametagVisNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("nametagVisibility");
    auto nametagVisArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("value", StringArgumentType::string());
    nametagVisArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _modifyTeam(ctx); });
    nametagVisNode->addChild(nametagVisArg);
    modifyTeamArg->addChild(nametagVisNode);

    // modify <team> deathMessageVisibility <visibility>
    auto deathMsgVisNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("deathMessageVisibility");
    auto deathMsgVisArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("value", StringArgumentType::string());
    deathMsgVisArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _modifyTeam(ctx); });
    deathMsgVisNode->addChild(deathMsgVisArg);
    modifyTeamArg->addChild(deathMsgVisNode);

    // modify <team> collisionRule <rule>
    auto collisionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("collisionRule");
    auto collisionArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("value", StringArgumentType::string());
    collisionArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _modifyTeam(ctx); });
    collisionNode->addChild(collisionArg);
    modifyTeamArg->addChild(collisionNode);

    modifyNode->addChild(modifyTeamArg);
    teamNode->addChild(modifyNode);

    dispatcher.registerCommand(teamNode);
}

i32 TeamCommand::_addTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const std::string teamName = context.getArgument<std::string>("team");

    // 检查名称长度
    if (teamName.length() > scoreboard::ScorePlayerTeam::MAX_NAME_LENGTH) {
        std::ostringstream ss;
        ss << "Team name '" << teamName << "' is too long (max " << scoreboard::ScorePlayerTeam::MAX_NAME_LENGTH
           << " characters)";
        source.sendError(ss.str());
        return 0;
    }

    // 检查是否已存在
    if (scoreboard->hasTeam(teamName)) {
        std::ostringstream ss;
        ss << "A team with the name '" << teamName << "' already exists";
        source.sendError(ss.str());
        return 0;
    }

    // 创建队伍
    auto* team = scoreboard->createTeam(teamName);
    if (!team) {
        source.sendError("Failed to create team");
        return 0;
    }

    // 设置显示名称（如果提供）
    if (context.hasArgument("displayName")) {
        const std::string displayName = context.getArgument<std::string>("displayName");
        team->setDisplayName(std::make_unique<text::StringTextComponent>(displayName));
    }

    std::ostringstream ss;
    ss << "Created team '" << teamName << "'";
    source.sendMessage(ss.str());

    return 1;
}

i32 TeamCommand::_removeTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const std::string teamName = context.getArgument<std::string>("team");

    // 查找队伍
    auto* team = scoreboard->getTeam(teamName);
    if (!team) {
        std::ostringstream ss;
        ss << "Unknown team '" << teamName << "'";
        source.sendError(ss.str());
        return 0;
    }

    // 移除队伍
    scoreboard->removeTeam(*team);

    std::ostringstream ss;
    ss << "Removed team '" << teamName << "'";
    source.sendMessage(ss.str());

    return static_cast<i32>(scoreboard->getTeams().size());
}

i32 TeamCommand::_listTeams(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    if (context.hasArgument("team")) {
        // 列出指定队伍的成员
        const std::string teamName = context.getArgument<std::string>("team");
        auto* team = scoreboard->getTeam(teamName);
        if (!team) {
            std::ostringstream ss;
            ss << "Unknown team '" << teamName << "'";
            source.sendError(ss.str());
            return 0;
        }

        const auto& members = team->getMembers();
        if (members.empty()) {
            std::ostringstream ss;
            ss << "Team '" << teamName << "' has no members";
            source.sendMessage(ss.str());
            return 0;
        }

        std::ostringstream ss;
        ss << "Members of team '" << teamName << "': ";
        bool first = true;
        for (const auto& member : members) {
            if (!first) {
                ss << ", ";
            }
            ss << member;
            first = false;
        }
        source.sendMessage(ss.str());
        return static_cast<i32>(members.size());
    } else {
        // 列出所有队伍
        auto teams = scoreboard->getTeams();
        if (teams.empty()) {
            source.sendMessage("There are no teams");
            return 0;
        }

        std::ostringstream ss;
        ss << "There are " << teams.size() << " teams: ";
        bool first = true;
        for (const auto* team : teams) {
            if (!first) {
                ss << ", ";
            }
            ss << team->getName();
            first = false;
        }
        source.sendMessage(ss.str());
        return static_cast<i32>(teams.size());
    }
}

i32 TeamCommand::_emptyTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const std::string teamName = context.getArgument<std::string>("team");

    // 查找队伍
    auto* team = scoreboard->getTeam(teamName);
    if (!team) {
        std::ostringstream ss;
        ss << "Unknown team '" << teamName << "'";
        source.sendError(ss.str());
        return 0;
    }

    // 复制成员列表（因为我们会修改它）
    const auto& members = team->getMembers();
    std::vector<std::string> membersCopy(members.begin(), members.end());

    if (membersCopy.empty()) {
        std::ostringstream ss;
        ss << "Team '" << teamName << "' is already empty";
        source.sendMessage(ss.str());
        return 0;
    }

    // 移除所有成员
    for (const auto& member : membersCopy) {
        scoreboard->removePlayerFromTeam(member, *team);
    }

    std::ostringstream ss;
    ss << "Emptied team '" << teamName << "' (removed " << membersCopy.size() << " member(s))";
    source.sendMessage(ss.str());

    return static_cast<i32>(membersCopy.size());
}

i32 TeamCommand::_joinTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const std::string teamName = context.getArgument<std::string>("team");
    const EntitySelector& selector = context.getArgument<EntitySelector>("members");

    // 查找队伍
    auto* team = scoreboard->getTeam(teamName);
    if (!team) {
        std::ostringstream ss;
        ss << "Unknown team '" << teamName << "'";
        source.sendError(ss.str());
        return 0;
    }

    // 解析玩家
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    // 添加成员
    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        std::string playerName = support::resolvePlayerName(source, playerId);
        if (scoreboard->addPlayerToTeam(playerName, *team)) {
            successCount++;
        }
    }

    if (successCount == 1) {
        std::ostringstream ss;
        ss << "Added 1 member to team '" << teamName << "'";
        source.sendMessage(ss.str());
    } else {
        std::ostringstream ss;
        ss << "Added " << successCount << " members to team '" << teamName << "'";
        source.sendMessage(ss.str());
    }

    return successCount;
}

i32 TeamCommand::_leaveTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const EntitySelector& selector = context.getArgument<EntitySelector>("members");

    // 解析玩家
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    // 移除成员（从所有队伍移除）
    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        std::string playerName = support::resolvePlayerName(source, playerId);
        auto* currentTeam = scoreboard->getPlayersTeam(playerName);
        if (currentTeam) {
            if (scoreboard->removePlayerFromTeam(playerName, *currentTeam)) {
                successCount++;
            }
        }
    }

    if (successCount == 0) {
        source.sendMessage("No members were removed from any team");
        return 0;
    }

    std::ostringstream ss;
    ss << "Removed " << successCount << " member(s) from their teams";
    source.sendMessage(ss.str());

    return successCount;
}

i32 TeamCommand::_modifyTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const std::string teamName = context.getArgument<std::string>("team");

    // 查找队伍
    auto* team = scoreboard->getTeam(teamName);
    if (!team) {
        std::ostringstream ss;
        ss << "Unknown team '" << teamName << "'";
        source.sendError(ss.str());
        return 0;
    }

    // 判断修改类型
    if (context.hasArgument("color")) {
        // 修改颜色
        const std::string colorStr = context.getArgument<std::string>("color");
        text::TextFormatting color = parseColor(colorStr);

        if (color == text::TextFormatting::None) {
            std::ostringstream ss;
            ss << "Invalid color '" << colorStr << "'";
            source.sendError(ss.str());
            return 0;
        }

        if (team->getColor() == color) {
            std::ostringstream ss;
            ss << "Team '" << teamName << "' already has color '" << colorStr << "'";
            source.sendError(ss.str());
            return 0;
        }

        team->setColor(color);
        std::ostringstream ss;
        ss << "Set color of team '" << teamName << "' to '" << colorStr << "'";
        source.sendMessage(ss.str());
        return 1;
    }

    if (context.hasArgument("friendlyFire")) {
        // 修改友军伤害
        const bool value = context.getArgument<bool>("friendlyFire");

        if (team->getAllowFriendlyFire() == value) {
            std::ostringstream ss;
            ss << "Team '" << teamName << "' already has friendly fire " << (value ? "enabled" : "disabled");
            source.sendError(ss.str());
            return 0;
        }

        team->setAllowFriendlyFire(value);
        std::ostringstream ss;
        ss << "Set friendly fire of team '" << teamName << "' to " << (value ? "true" : "false");
        source.sendMessage(ss.str());
        return 1;
    }

    if (context.hasArgument("seeFriendlyInvisibles")) {
        // 修改是否能看到隐身队友
        const bool value = context.getArgument<bool>("seeFriendlyInvisibles");

        if (team->canSeeFriendlyInvisibles() == value) {
            std::ostringstream ss;
            ss << "Team '" << teamName << "' already has see friendly invisibles " << (value ? "enabled" : "disabled");
            source.sendError(ss.str());
            return 0;
        }

        team->setSeeFriendlyInvisibles(value);
        std::ostringstream ss;
        ss << "Set see friendly invisibles of team '" << teamName << "' to " << (value ? "true" : "false");
        source.sendMessage(ss.str());
        return 1;
    }

    if (context.hasArgument("prefix")) {
        // 修改前缀
        const std::string prefix = context.getArgument<std::string>("prefix");
        team->setPrefix(std::make_unique<text::StringTextComponent>(prefix));

        std::ostringstream ss;
        ss << "Set prefix of team '" << teamName << "'";
        source.sendMessage(ss.str());
        return 1;
    }

    if (context.hasArgument("suffix")) {
        // 修改后缀
        const std::string suffix = context.getArgument<std::string>("suffix");
        team->setSuffix(std::make_unique<text::StringTextComponent>(suffix));

        std::ostringstream ss;
        ss << "Set suffix of team '" << teamName << "'";
        source.sendMessage(ss.str());
        return 1;
    }

    if (context.hasArgument("displayName")) {
        // 修改显示名称
        const std::string displayName = context.getArgument<std::string>("displayName");
        team->setDisplayName(std::make_unique<text::StringTextComponent>(displayName));

        std::ostringstream ss;
        ss << "Set display name of team '" << teamName << "'";
        source.sendMessage(ss.str());
        return 1;
    }

    if (context.hasArgument("nametagVisibility")) {
        // 修改名称标签可见性
        const std::string visStr = context.getArgument<std::string>("nametagVisibility");
        scoreboard::TeamVisibility visibility = parseVisibility(visStr);

        if (visibility == scoreboard::TeamVisibility::Always && visStr != "always") {
            // 解析失败
            std::ostringstream ss;
            ss << "Invalid visibility '" << visStr << "'. Valid values: always, never, hideForOtherTeams, "
               << "hideForOwnTeam";
            source.sendError(ss.str());
            return 0;
        }

        if (team->getNameTagVisibility() == visibility) {
            std::ostringstream ss;
            ss << "Team '" << teamName << "' already has nametag visibility '" << visStr << "'";
            source.sendError(ss.str());
            return 0;
        }

        team->setNameTagVisibility(visibility);
        std::ostringstream ss;
        ss << "Set nametag visibility of team '" << teamName << "' to '" << visStr << "'";
        source.sendMessage(ss.str());
        return 1;
    }

    if (context.hasArgument("deathMessageVisibility")) {
        // 修改死亡消息可见性
        const std::string visStr = context.getArgument<std::string>("deathMessageVisibility");
        scoreboard::TeamVisibility visibility = parseVisibility(visStr);

        if (visibility == scoreboard::TeamVisibility::Always && visStr != "always") {
            // 解析失败
            std::ostringstream ss;
            ss << "Invalid visibility '" << visStr << "'. Valid values: always, never, hideForOtherTeams, "
               << "hideForOwnTeam";
            source.sendError(ss.str());
            return 0;
        }

        if (team->getDeathMessageVisibility() == visibility) {
            std::ostringstream ss;
            ss << "Team '" << teamName << "' already has death message visibility '" << visStr << "'";
            source.sendError(ss.str());
            return 0;
        }

        team->setDeathMessageVisibility(visibility);
        std::ostringstream ss;
        ss << "Set death message visibility of team '" << teamName << "' to '" << visStr << "'";
        source.sendMessage(ss.str());
        return 1;
    }

    if (context.hasArgument("collisionRule")) {
        // 修改碰撞规则
        const std::string ruleStr = context.getArgument<std::string>("collisionRule");
        scoreboard::TeamCollisionRule rule = parseCollisionRule(ruleStr);

        if (rule == scoreboard::TeamCollisionRule::Always && ruleStr != "always") {
            // 解析失败
            std::ostringstream ss;
            ss << "Invalid collision rule '" << ruleStr << "'. Valid values: always, never, pushOtherTeams, "
               << "pushOwnTeam";
            source.sendError(ss.str());
            return 0;
        }

        if (team->getCollisionRule() == rule) {
            std::ostringstream ss;
            ss << "Team '" << teamName << "' already has collision rule '" << ruleStr << "'";
            source.sendError(ss.str());
            return 0;
        }

        team->setCollisionRule(rule);
        std::ostringstream ss;
        ss << "Set collision rule of team '" << teamName << "' to '" << ruleStr << "'";
        source.sendMessage(ss.str());
        return 1;
    }

    std::ostringstream ss;
    ss << "Modified team '" << teamName << "'";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
