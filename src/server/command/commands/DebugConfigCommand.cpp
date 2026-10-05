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

#include "DebugConfigCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"

#include <memory>
#include <sstream>
#include <string>

namespace mc::command {

void DebugConfigCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto debugConfigNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("debugconfig");
    debugConfigNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(3); });
    support::applyMetadata(debugConfigNode,
        support::makeMetadata("Debug configuration of a player's connection.",
            "/debugconfig config <player> | /debugconfig unconfig <uuid> | /debugconfig dialog <uuid> <dialog>",
            3,
            {},
            false));

    // /debugconfig config <player>
    {
        auto configNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("config");
        auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "target", EntityArgumentType::players());
        playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _config(ctx); });
        configNode->addChild(playerArg);
        debugConfigNode->addChild(configNode);
    }

    // /debugconfig unconfig <uuid>
    {
        auto unconfigNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("unconfig");
        auto uuidArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
            "target", StringArgumentType::word());
        uuidArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _unconfig(ctx); });
        unconfigNode->addChild(uuidArg);
        debugConfigNode->addChild(unconfigNode);
    }

    // /debugconfig dialog <uuid> <dialog>
    {
        auto dialogNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("dialog");
        auto uuidArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
            "target", StringArgumentType::word());
        auto dialogArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
            "dialog", StringArgumentType::word());
        dialogArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _dialog(ctx); });
        uuidArg->addChild(dialogArg);
        dialogNode->addChild(uuidArg);
        debugConfigNode->addChild(dialogNode);
    }

    dispatcher.registerCommand(debugConfigNode);
}

i32 DebugConfigCommand::_config(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("Server not available");
        return 0;
    }

    EntitySelector selector = context.getArgument<EntitySelector>("target");
    const std::vector<PlayerId> targetPlayerIds = support::resolvePlayerIds(source, selector);
    if (targetPlayerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    i32 switched = 0;
    for (PlayerId playerId : targetPlayerIds) {
        if (server->startConfigurationForPlayer(playerId)) {
            ++switched;
            std::ostringstream ss;
            ss << "Switched player " << playerId << " to config mode";
            source.sendMessage(ss.str());
        } else {
            std::ostringstream ss;
            ss << "Failed to switch player " << playerId << " to config mode (no remote session)";
            source.sendError(ss.str());
        }
    }
    return switched;
}

i32 DebugConfigCommand::_unconfig(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    // TODO(debugconfig_unconfig): vanilla 的 unconfig 需要「按 UUID 查找处于配置阶段的连接」
    //   （ServerConfigurationPacketListenerImpl）。本项目的会话（ClientSession）不按 UUID 索引，
    //   且配置阶段的连接未建立玩家记录，故暂无法按 UUID 定位。待会话层补充 UUID→session 索引后实现。
    (void)context;
    source.sendError("debugconfig unconfig is not implemented (no UUID-indexed session lookup)");
    return 0;
}

i32 DebugConfigCommand::_dialog(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    // TODO(debugconfig_dialog): 依赖 1.21 的 show_dialog(cb:138) 包与对话系统（Dialog），
    //   二者本项目均未实现；且同 unconfig 需要按 UUID 定位配置阶段连接。待对话系统落地后实现。
    (void)context;
    source.sendError("debugconfig dialog is not implemented (dialog system missing)");
    return 0;
}

} // namespace mc::command
