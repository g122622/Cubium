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
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/core/Types.hpp"
#include "common/resource/LanguageManager.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/scoreboard/core/TeamEnums.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/CommandSuggestionProviders.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace command {

// 使用 mc::server 命名空间中的 ServerScoreboard
using ::mc::server::ServerScoreboard;

namespace {

/// 语言文件加载器（反馈文本复用原版 commands.team.* / team.visibility.* / team.collision.* 键）
const LanguageManager& _lang()
{
    return LanguageManager::instance();
}

/// 获取服务端记分板
ServerScoreboard* _scoreboard(ServerCommandSource& source)
{
    auto* server = source.server();
    return server != nullptr ? &server->scoreboard() : nullptr;
}

/**
 * @brief 队伍格式化显示名（对齐 vanilla PlayerTeam.getFormattedDisplayName）
 *
 * 形如 "[显示名]"，已带队伍颜色的格式码，用于反馈文本中的 %s 参数。
 */
std::string _formattedTeamName(const scoreboard::ScorePlayerTeam& team)
{
    return team.getFormattedDisplayName()->getFormattedText();
}

/// 可见性显示名（对齐 vanilla Team.Visibility.getDisplayName）
std::string _visibilityDisplayName(scoreboard::TeamVisibility visibility)
{
    return _lang().get(std::string("team.visibility.") + scoreboard::teamVisibilityToString(visibility));
}

/// 碰撞规则显示名（对齐 vanilla Team.CollisionRule.getDisplayName）
std::string _collisionDisplayName(scoreboard::TeamCollisionRule rule)
{
    return _lang().get(std::string("team.collision.") + scoreboard::teamCollisionRuleToString(rule));
}

/// 列表格式化（对齐 vanilla ComponentUtils.formatList：以 ", " 连接）
std::string _joinList(const std::vector<std::string>& items)
{
    std::string joined;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i != 0) {
            joined += ", ";
        }
        joined += items[i];
    }
    return joined;
}

/**
 * @brief 解析队伍；不存在时发送未知队伍错误
 */
scoreboard::ScorePlayerTeam* _findTeam(
    ServerCommandSource& source, ServerScoreboard& scoreboard, const std::string& name)
{
    auto* team = scoreboard.getTeam(name);
    if (team == nullptr) {
        source.sendError("Unknown team '" + name + "'");
    }
    return team;
}

} // namespace

void TeamCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto teamNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("team");
    teamNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(teamNode,
        support::makeMetadata("Manages teams.", "/team <add|remove|list|empty|join|leave|modify> ...", 2, {}, true));

    // /team add <team> [displayName]
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    // 原版 /team add 的队伍名是普通 string（新建队伍时用），其余子命令才用 TeamArgument。
    auto teamNameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", StringArgumentType::word());
    // 原版 /team add 的显示名是聊天组件。
    auto displayNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
        "displayName", ComponentArgumentType::component());
    displayNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addTeam(ctx);
    });
    teamNameArg->addChild(displayNameArg);
    teamNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addTeam(ctx);
    });
    addNode->addChild(teamNameArg);
    teamNode->addChild(addNode);

    // /team remove <team>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", TeamNameArgumentType::team());
    removeTeamArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _removeTeam(ctx);
    });
    removeNode->addChild(removeTeamArg);
    teamNode->addChild(removeNode);

    // /team list [team]
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    auto listTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", TeamNameArgumentType::team());
    listTeamArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listTeams(ctx);
    });
    listNode->addChild(listTeamArg);
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listTeams(ctx);
    });
    teamNode->addChild(listNode);

    // /team empty <team>
    auto emptyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("empty");
    auto emptyTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", TeamNameArgumentType::team());
    emptyTeamArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _emptyTeam(ctx);
    });
    emptyNode->addChild(emptyTeamArg);
    teamNode->addChild(emptyNode);

    // /team join <team> <members>
    auto joinNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("join");
    auto joinTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", TeamNameArgumentType::team());
    // 原版 /team join 的 members 是 ScoreHolderArgument（只收名字），与 _joinTeam 读取 string 一致；
    // 若登记成 EntitySelector，getArgument<std::string> 会抛 bad any_cast。
    auto membersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "members", ScoreHolderArgumentType::scoreHolder(true));
    membersArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    membersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _joinTeam(ctx);
    });
    // 原版把命令挂在 team 参数本身：`/team join <team>` 不带成员即视为「自己加入」。
    joinTeamArg->setCommand(membersArg->getCommand());
    joinTeamArg->addChild(membersArg);
    joinNode->addChild(joinTeamArg);
    teamNode->addChild(joinNode);

    // /team leave <members>
    auto leaveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("leave");
    // 原版 /team leave 的 members 是 ScoreHolderArgument（score_holder，只收名字）。
    auto leaveMembersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "members", ScoreHolderArgumentType::scoreHolder(true));
    leaveMembersArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    leaveMembersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _leaveTeam(ctx);
    });
    leaveNode->addChild(leaveMembersArg);
    teamNode->addChild(leaveNode);

    // /team modify <team> <option> <value>
    auto modifyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("modify");
    auto modifyTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", TeamNameArgumentType::team());
    // 选项值参数：原版是 StringArgumentType.string()（类型名 string，允许引号）。
    // 每个选项把自己的名字写进 teamOption 后再交给 _modifyTeam（原版是每个选项注册时
    // 自带判定函数；这里用同一个执行入口按名字分派）。
    auto makeOptionValueArg = [](const char* optionName) {
        auto node = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
            "value", StringArgumentType::string());
        const std::string option = optionName;
        node->setCommand([option](CommandContext<ServerCommandSource>& ctx) {
            ctx.setArgument("teamOption", option);
            return _modifyTeam(ctx);
        });
        return node;
    };

    // 原版 /team modify <team> <选项> <value>：每个选项是字面量，下面只挂一个 value 参数
    // （StringArgumentType.string()），选项名由执行入口按字面量分派。
    // 选项值类型（原版 genericOption 的 withValues）：
    // displayName / prefix / suffix 是聊天组件；friendlyFire / seeFriendlyInvisibles 是 bool；
    // 其余是普通字符串。
    const auto addComponentOption = [&](const char* optionName) {
        auto optionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(optionName);
        auto valueNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
            "value", ComponentArgumentType::component());
        const std::string option = optionName;
        valueNode->setCommand([option](CommandContext<ServerCommandSource>& ctx) {
            ctx.setArgument("teamOption", option);
            return _modifyTeam(ctx);
        });
        optionNode->addChild(valueNode);
        modifyTeamArg->addChild(optionNode);
    };
    const auto addBoolOption = [&](const char* optionName) {
        auto optionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(optionName);
        auto valueNode =
            std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("allowed", BoolArgumentType::boolArg());
        const std::string option = optionName;
        valueNode->setCommand([option](CommandContext<ServerCommandSource>& ctx) {
            ctx.setArgument("teamOption", option);
            return _modifyTeam(ctx);
        });
        optionNode->addChild(valueNode);
        modifyTeamArg->addChild(optionNode);
    };
    // 原版的 nametagVisibility / deathMessageVisibility / collisionRule 是小写开头的固定词表，
    // 在该位置是字面量子节点（不是字符串参数）。
    const auto addLiteralOption = [&](const char* optionName, std::initializer_list<const char*> choices) {
        auto optionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(optionName);
        const std::string option = optionName;
        for (const char* choice : choices) {
            auto choiceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(choice);
            choiceNode->setCommand([option, choice](CommandContext<ServerCommandSource>& ctx) {
                ctx.setArgument("teamOption", option);
                ctx.setArgument("teamOptionValue", std::string(choice));
                return _modifyTeam(ctx);
            });
            optionNode->addChild(choiceNode);
        }
        modifyTeamArg->addChild(optionNode);
    };
    const auto addWordOption = [&](const char* optionName) {
        auto optionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(optionName);
        const std::string option = optionName;
        std::shared_ptr<ArgumentType<std::string>> type = StringArgumentType::string();
        if (option == "color") {
            type = KeywordArgumentType::word("color");
        }
        auto valueNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("value", type);
        valueNode->setCommand([option](CommandContext<ServerCommandSource>& ctx) {
            ctx.setArgument("teamOption", option);
            return _modifyTeam(ctx);
        });
        optionNode->addChild(valueNode);
        modifyTeamArg->addChild(optionNode);
    };
    // 原版这三个选项的参数名与选项同名（displayName / prefix / suffix）。
    const auto addNamedComponentOption = [&](const char* optionName) {
        auto optionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(optionName);
        auto valueNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
            optionName, ComponentArgumentType::component());
        const std::string option = optionName;
        valueNode->setCommand([option](CommandContext<ServerCommandSource>& ctx) {
            ctx.setArgument("teamOption", option);
            return _modifyTeam(ctx);
        });
        optionNode->addChild(valueNode);
        modifyTeamArg->addChild(optionNode);
    };
    for (const char* option : {"displayName", "prefix", "suffix"}) {
        addNamedComponentOption(option);
    }
    for (const char* option : {"friendlyFire", "seeFriendlyInvisibles"}) {
        addBoolOption(option);
    }
    for (const char* option : {"nametagVisibility", "deathMessageVisibility"}) {
        addLiteralOption(option, {"always", "never", "hideForOtherTeams", "hideForOwnTeam"});
    }
    addLiteralOption("collisionRule", {"always", "never", "pushOtherTeams", "pushOwnTeam"});
    for (const char* option : {"color"}) {
        addWordOption(option);
    }

    modifyNode->addChild(modifyTeamArg);
    teamNode->addChild(modifyNode);

    dispatcher.registerCommand(teamNode);
}

i32 TeamCommand::_addTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = _scoreboard(source);
    if (scoreboard == nullptr) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const std::string teamName = context.getArgument<std::string>("team");
    if (teamName.length() > scoreboard::ScorePlayerTeam::MAX_NAME_LENGTH) {
        source.sendError("Team name '" + teamName + "' is too long (max " +
            std::to_string(scoreboard::ScorePlayerTeam::MAX_NAME_LENGTH) + " characters)");
        return 0;
    }

    // 对齐 vanilla：同名队伍已存在时抛出 duplicate 错误。
    if (scoreboard->hasTeam(teamName)) {
        source.sendError(_lang().get("commands.team.add.duplicate"));
        return 0;
    }

    auto* team = scoreboard->createTeam(teamName);
    if (team == nullptr) {
        source.sendError(_lang().get("commands.team.add.duplicate"));
        return 0;
    }

    // 显示名缺省为队伍名本身（vanilla createTeam 的二参版本传 Component.literal(name)）。
    if (context.hasArgument("displayName")) {
        team->setDisplayName(
            std::make_unique<text::StringTextComponent>(context.getArgument<ComponentTextPtr>("displayName")->text));
    }

    source.sendMessage(_lang().get("commands.team.add.success", {_formattedTeamName(*team)}));
    return static_cast<i32>(scoreboard->getTeams().size());
}

i32 TeamCommand::_removeTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = _scoreboard(source);
    if (scoreboard == nullptr) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const std::string teamName = context.getArgument<std::string>("team");
    auto* team = _findTeam(source, *scoreboard, teamName);
    if (team == nullptr) {
        return 0;
    }

    const std::string displayName = _formattedTeamName(*team);
    scoreboard->removeTeam(*team);
    source.sendMessage(_lang().get("commands.team.remove.success", {displayName}));
    return static_cast<i32>(scoreboard->getTeams().size());
}

i32 TeamCommand::_listTeams(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = _scoreboard(source);
    if (scoreboard == nullptr) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    if (context.hasArgument("team")) {
        // /team list <team>：列出该队伍成员
        const std::string teamName = context.getArgument<std::string>("team");
        auto* team = _findTeam(source, *scoreboard, teamName);
        if (team == nullptr) {
            return 0;
        }

        const auto& members = team->getMembers();
        const std::string displayName = _formattedTeamName(*team);
        if (members.empty()) {
            source.sendMessage(_lang().get("commands.team.list.members.empty", {displayName}));
            return 0;
        }

        const std::vector<std::string> memberNames(members.begin(), members.end());
        source.sendMessage(_lang().get("commands.team.list.members.success",
            {displayName, std::to_string(members.size()), _joinList(memberNames)}));
        return static_cast<i32>(members.size());
    }

    // /team list：列出全部队伍
    const auto teams = scoreboard->getTeams();
    if (teams.empty()) {
        source.sendMessage(_lang().get("commands.team.list.teams.empty"));
        return 0;
    }

    std::vector<std::string> teamNames;
    teamNames.reserve(teams.size());
    for (const auto* team : teams) {
        if (team != nullptr) {
            teamNames.push_back(_formattedTeamName(*team));
        }
    }
    source.sendMessage(
        _lang().get("commands.team.list.teams.success", {std::to_string(teams.size()), _joinList(teamNames)}));
    return static_cast<i32>(teams.size());
}

i32 TeamCommand::_emptyTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = _scoreboard(source);
    if (scoreboard == nullptr) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const std::string teamName = context.getArgument<std::string>("team");
    auto* team = _findTeam(source, *scoreboard, teamName);
    if (team == nullptr) {
        return 0;
    }

    const std::string displayName = _formattedTeamName(*team);
    // 复制成员列表，避免在遍历中修改成员集合。
    const std::vector<std::string> members(team->getMembers().begin(), team->getMembers().end());
    if (members.empty()) {
        source.sendError(_lang().get("commands.team.empty.unchanged"));
        return 0;
    }

    for (const auto& member : members) {
        scoreboard->removePlayerFromTeam(member, *team);
    }

    source.sendMessage(_lang().get("commands.team.empty.success", {std::to_string(members.size()), displayName}));
    return static_cast<i32>(members.size());
}

i32 TeamCommand::_joinTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = _scoreboard(source);
    if (scoreboard == nullptr) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const std::string teamName = context.getArgument<std::string>("team");

    auto* team = _findTeam(source, *scoreboard, teamName);
    if (team == nullptr) {
        return 0;
    }

    // 不带 members 时由命令源自己加入（原版 `team.join` 的隐式选择器是 @s）。
    std::vector<PlayerId> playerIds;
    if (context.hasArgument("members")) {
        playerIds = support::resolvePlayerIds(
            source, support::gameProfileSelector(context.getArgument<std::string>("members")));
    } else if (source.isPlayer()) {
        playerIds.push_back(source.playerId());
    }
    if (playerIds.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    std::vector<std::string> names;
    names.reserve(playerIds.size());
    for (PlayerId playerId : playerIds) {
        const std::string playerName = support::resolvePlayerName(source, playerId);
        scoreboard->addPlayerToTeam(playerName, *team);
        names.push_back(playerName);
    }

    const std::string displayName = _formattedTeamName(*team);
    if (playerIds.size() == 1) {
        source.sendMessage(_lang().get("commands.team.join.success.single", {names.front(), displayName}));
    } else {
        source.sendMessage(
            _lang().get("commands.team.join.success.multiple", {std::to_string(playerIds.size()), displayName}));
    }
    return static_cast<i32>(playerIds.size());
}

i32 TeamCommand::_leaveTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = _scoreboard(source);
    if (scoreboard == nullptr) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const EntitySelector selector = support::gameProfileSelector(context.getArgument<std::string>("members"));
    // 不带 members 时由命令源自己加入（原版 `team.join` 的隐式选择器是 @s）。
    std::vector<PlayerId> playerIds;
    if (context.hasArgument("members")) {
        playerIds = support::resolvePlayerIds(
            source, support::gameProfileSelector(context.getArgument<std::string>("members")));
    } else if (source.isPlayer()) {
        playerIds.push_back(source.playerId());
    }
    if (playerIds.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    // 对齐 vanilla leaveTeam：从各自队伍移除（不在任何队伍中的玩家不算失败）。
    std::vector<std::string> names;
    names.reserve(playerIds.size());
    for (PlayerId playerId : playerIds) {
        const std::string playerName = support::resolvePlayerName(source, playerId);
        scoreboard->removePlayerFromTeam(playerName);
        names.push_back(playerName);
    }

    if (playerIds.size() == 1) {
        source.sendMessage(_lang().get("commands.team.leave.success.single", {names.front()}));
    } else {
        source.sendMessage(_lang().get("commands.team.leave.success.multiple", {std::to_string(playerIds.size())}));
    }
    return static_cast<i32>(playerIds.size());
}

i32 TeamCommand::_modifyTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = _scoreboard(source);
    if (scoreboard == nullptr) {
        source.sendError("Scoreboard is not available");
        return 0;
    }

    const std::string teamName = context.getArgument<std::string>("team");
    auto* team = _findTeam(source, *scoreboard, teamName);
    if (team == nullptr) {
        return 0;
    }
    const std::string displayName = _formattedTeamName(*team);

    const std::string option = context.getArgumentOr<std::string>("teamOption", "");
    if (option == "color") {
        const std::string colorStr = context.getArgument<std::string>("value");
        const text::TextFormatting color = text::fromName(colorStr);
        if (color == text::TextFormatting::None) {
            source.sendError("Unknown color '" + colorStr + "'");
            return 0;
        }
        if (team->getColor() == color) {
            source.sendError(_lang().get("commands.team.option.color.unchanged"));
            return 0;
        }
        team->setColor(color);
        source.sendMessage(_lang().get("commands.team.option.color.success", {displayName, text::toName(color)}));
        return 0;
    }

    if (option == "friendlyFire") {
        const bool enabled = context.getArgument<bool>("allowed");
        if (team->getAllowFriendlyFire() == enabled) {
            source.sendError(_lang().get(enabled ? "commands.team.option.friendlyfire.alreadyEnabled"
                                                 : "commands.team.option.friendlyfire.alreadyDisabled"));
            return 0;
        }
        team->setAllowFriendlyFire(enabled);
        source.sendMessage(_lang().get(
            enabled ? "commands.team.option.friendlyfire.enabled" : "commands.team.option.friendlyfire.disabled",
            {displayName}));
        return 0;
    }

    if (option == "seeFriendlyInvisibles") {
        const bool enabled = context.getArgument<bool>("allowed");
        if (team->canSeeFriendlyInvisibles() == enabled) {
            source.sendError(_lang().get(enabled ? "commands.team.option.seeFriendlyInvisibles.alreadyEnabled"
                                                 : "commands.team.option.seeFriendlyInvisibles.alreadyDisabled"));
            return 0;
        }
        team->setSeeFriendlyInvisibles(enabled);
        source.sendMessage(_lang().get(enabled ? "commands.team.option.seeFriendlyInvisibles.enabled"
                                               : "commands.team.option.seeFriendlyInvisibles.disabled",
            {displayName}));
        return 0;
    }

    if (option == "nametagVisibility") {
        const std::string visStr = context.getArgument<std::string>("teamOptionValue");
        const auto visibility = scoreboard::teamVisibilityFromString(visStr);
        // teamVisibilityFromString 对未知值回退 Always，用原字符串核对以拒绝非法输入。
        if (scoreboard::teamVisibilityToString(visibility) != visStr) {
            source.sendError(
                "Unknown visibility '" + visStr + "'. Valid values: always, never, hideForOtherTeams, hideForOwnTeam");
            return 0;
        }
        if (team->getNameTagVisibility() == visibility) {
            source.sendError(_lang().get("commands.team.option.nametagVisibility.unchanged"));
            return 0;
        }
        team->setNameTagVisibility(visibility);
        source.sendMessage(_lang().get(
            "commands.team.option.nametagVisibility.success", {displayName, _visibilityDisplayName(visibility)}));
        return 0;
    }

    if (option == "deathMessageVisibility") {
        const std::string visStr = context.getArgument<std::string>("teamOptionValue");
        const auto visibility = scoreboard::teamVisibilityFromString(visStr);
        if (scoreboard::teamVisibilityToString(visibility) != visStr) {
            source.sendError(
                "Unknown visibility '" + visStr + "'. Valid values: always, never, hideForOtherTeams, hideForOwnTeam");
            return 0;
        }
        if (team->getDeathMessageVisibility() == visibility) {
            source.sendError(_lang().get("commands.team.option.deathMessageVisibility.unchanged"));
            return 0;
        }
        team->setDeathMessageVisibility(visibility);
        source.sendMessage(_lang().get(
            "commands.team.option.deathMessageVisibility.success", {displayName, _visibilityDisplayName(visibility)}));
        return 0;
    }

    if (option == "collisionRule") {
        const std::string ruleStr = context.getArgument<std::string>("teamOptionValue");
        const auto rule = scoreboard::teamCollisionRuleFromString(ruleStr);
        if (scoreboard::teamCollisionRuleToString(rule) != ruleStr) {
            source.sendError(
                "Unknown collision rule '" + ruleStr + "'. Valid values: always, never, pushOtherTeams, pushOwnTeam");
            return 0;
        }
        if (team->getCollisionRule() == rule) {
            source.sendError(_lang().get("commands.team.option.collisionRule.unchanged"));
            return 0;
        }
        team->setCollisionRule(rule);
        source.sendMessage(
            _lang().get("commands.team.option.collisionRule.success", {displayName, _collisionDisplayName(rule)}));
        return 0;
    }

    if (option == "prefix") {
        const std::string prefix = context.getArgument<ComponentTextPtr>(option)->text;
        team->setPrefix(std::make_unique<text::StringTextComponent>(prefix));
        source.sendMessage(_lang().get("commands.team.option.prefix.success", {prefix}));
        return 1;
    }

    if (option == "suffix") {
        const std::string suffix = context.getArgument<ComponentTextPtr>(option)->text;
        team->setSuffix(std::make_unique<text::StringTextComponent>(suffix));
        source.sendMessage(_lang().get("commands.team.option.suffix.success", {suffix}));
        return 1;
    }

    if (option == "displayName") {
        const std::string newDisplayName = context.getArgument<ComponentTextPtr>(option)->text;
        const text::ITextComponent* current = team->getDisplayName();
        if (current != nullptr && current->getFormattedText() == newDisplayName) {
            source.sendError(_lang().get("commands.team.option.name.unchanged"));
            return 0;
        }
        team->setDisplayName(std::make_unique<text::StringTextComponent>(newDisplayName));
        source.sendMessage(_lang().get("commands.team.option.name.success", {displayName}));
        return 0;
    }

    source.sendError("No team option was specified");
    return 0;
}

} // namespace command
} // namespace mc
