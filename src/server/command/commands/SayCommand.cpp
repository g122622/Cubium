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

#include "SayCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <memory>
#include <string>
#include <spdlog/spdlog.h>

namespace mc::command {

void SayCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto sayNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("say");
    sayNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(sayNode, support::makeMetadata("Broadcast a server message.", "/say <message>", 2));

    auto messageArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "message", StringArgumentType::greedyString());
    messageArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _say(ctx); });
    sayNode->addChild(messageArg);

    dispatcher.registerCommand(sayNode);
}

i32 SayCommand::_say(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const std::string message = context.getArgument<std::string>("message");
    // 系统消息仅记日志（原 IServer::broadcastServerMessage 实现即只 spdlog，不发包）。
    // 批5b 已从 IServer 删除该纯虚，命令直接打日志。
    spdlog::info("[System] [{}] {}", source.name(), message);
    source.sendMessage("Broadcasted message");
    return 1;
}

} // namespace mc::command