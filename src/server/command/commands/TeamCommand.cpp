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
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include <sstream>

namespace mc {
namespace command {

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
    displayNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return addTeam(ctx); });
    teamNameArg->addChild(displayNameArg);
    teamNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return addTeam(ctx); });
    addNode->addChild(teamNameArg);
    teamNode->addChild(addNode);

    // /team remove <team>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", StringArgumentType::string());
    removeTeamArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return removeTeam(ctx); });
    removeNode->addChild(removeTeamArg);
    teamNode->addChild(removeNode);

    // /team list [team]
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    auto listTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", StringArgumentType::string());
    listTeamArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return listTeams(ctx); });
    listNode->addChild(listTeamArg);
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return listTeams(ctx); });
    teamNode->addChild(listNode);

    // /team empty <team>
    auto emptyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("empty");
    auto emptyTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", StringArgumentType::string());
    emptyTeamArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return emptyTeam(ctx); });
    emptyNode->addChild(emptyTeamArg);
    teamNode->addChild(emptyNode);

    // /team join <team> <members>
    auto joinNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("join");
    auto joinTeamArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("team", StringArgumentType::string());
    auto membersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "members", EntityArgumentType::entities());
    membersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return joinTeam(ctx); });
    joinTeamArg->addChild(membersArg);
    joinNode->addChild(joinTeamArg);
    teamNode->addChild(joinNode);

    // /team leave <members>
    auto leaveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("leave");
    auto leaveMembersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "members", EntityArgumentType::entities());
    leaveMembersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return leaveTeam(ctx); });
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
    colorValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return modifyTeam(ctx); });
    colorNode->addChild(colorValueArg);
    modifyTeamArg->addChild(colorNode);

    // modify <team> friendlyFire <true|false>
    auto friendlyFireNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("friendlyFire");
    auto friendlyFireArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("value", BoolArgumentType::boolArg());
    friendlyFireArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return modifyTeam(ctx); });
    friendlyFireNode->addChild(friendlyFireArg);
    modifyTeamArg->addChild(friendlyFireNode);

    // modify <team> seeFriendlyInvisibles <true|false>
    auto seeInvisNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("seeFriendlyInvisibles");
    auto seeInvisArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("value", BoolArgumentType::boolArg());
    seeInvisArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return modifyTeam(ctx); });
    seeInvisNode->addChild(seeInvisArg);
    modifyTeamArg->addChild(seeInvisNode);

    // modify <team> prefix <prefix>
    auto prefixNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("prefix");
    auto prefixArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "value", StringArgumentType::greedyString());
    prefixArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return modifyTeam(ctx); });
    prefixNode->addChild(prefixArg);
    modifyTeamArg->addChild(prefixNode);

    // modify <team> suffix <suffix>
    auto suffixNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("suffix");
    auto suffixArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "value", StringArgumentType::greedyString());
    suffixArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return modifyTeam(ctx); });
    suffixNode->addChild(suffixArg);
    modifyTeamArg->addChild(suffixNode);

    // modify <team> displayName <displayName>
    auto displayNameNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("displayName");
    auto displayNameValueArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "value", StringArgumentType::greedyString());
    displayNameValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return modifyTeam(ctx); });
    displayNameNode->addChild(displayNameValueArg);
    modifyTeamArg->addChild(displayNameNode);

    modifyNode->addChild(modifyTeamArg);
    teamNode->addChild(modifyNode);

    dispatcher.registerCommand(teamNode);
}

i32 TeamCommand::addTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string teamName = context.getArgument<std::string>("team");

    std::string displayName = teamName;
    if (context.hasArgument("displayName")) {
        displayName = context.getArgument<std::string>("displayName");
    }

    std::ostringstream ss;
    ss << "Created team '" << teamName << "'";
    source.sendMessage(ss.str());

    // TODO: 实现队伍系统

    return 1;
}

i32 TeamCommand::removeTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string teamName = context.getArgument<std::string>("team");

    std::ostringstream ss;
    ss << "Removed team '" << teamName << "'";
    source.sendMessage(ss.str());

    // TODO: 实现队伍系统

    return 1;
}

i32 TeamCommand::listTeams(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    if (context.hasArgument("team")) {
        const std::string teamName = context.getArgument<std::string>("team");
        std::ostringstream ss;
        ss << "Members of team '" << teamName << "': (none)";
        source.sendMessage(ss.str());
    } else {
        // TODO: 实现队伍系统
        source.sendMessage("There are no teams");
    }

    return 1;
}

i32 TeamCommand::emptyTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string teamName = context.getArgument<std::string>("team");

    std::ostringstream ss;
    ss << "Emptied team '" << teamName << "'";
    source.sendMessage(ss.str());

    // TODO: 实现队伍系统

    return 1;
}

i32 TeamCommand::joinTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string teamName = context.getArgument<std::string>("team");
    const EntitySelector& selector = context.getArgument<EntitySelector>("members");

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    std::ostringstream ss;
    ss << "Added " << playerIds.size() << " member(s) to team '" << teamName << "'";
    source.sendMessage(ss.str());

    // TODO: 实现队伍系统

    return static_cast<i32>(playerIds.size());
}

i32 TeamCommand::leaveTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("members");

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    std::ostringstream ss;
    ss << "Removed " << playerIds.size() << " member(s) from their teams";
    source.sendMessage(ss.str());

    // TODO: 实现队伍系统

    return static_cast<i32>(playerIds.size());
}

i32 TeamCommand::modifyTeam(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string teamName = context.getArgument<std::string>("team");

    std::ostringstream ss;
    ss << "Modified team '" << teamName << "'";
    source.sendMessage(ss.str());

    // TODO: 实现队伍系统

    return 1;
}

} // namespace command
} // namespace mc
