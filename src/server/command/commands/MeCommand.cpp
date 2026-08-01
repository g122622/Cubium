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

#include "MeCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

void MeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto meNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("me");
    meNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(0); });
    support::applyMetadata(
        meNode, support::makeMetadata("Displays a message about yourself in chat.", "/me <action>", 0, {}, true));

    auto actionArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "action", StringArgumentType::greedyString());
    actionArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _performAction(ctx); });
    meNode->addChild(actionArg);

    dispatcher.registerCommand(meNode);
}

i32 MeCommand::_performAction(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const std::string action = context.getArgument<std::string>("action");
    const std::string& name = source.name();

    // 广播 "* playername action" 格式的消息（原 IServer::broadcastServerMessage
    // 实现即只 spdlog，不发包；批5b 已从 IServer 删除该纯虚，命令直接打日志）。
    spdlog::info("[System] * {} {}", name, action);
    return 1;
}

} // namespace command
} // namespace mc
