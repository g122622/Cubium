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

#include "SetIdleTimeoutCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"

#include <memory>
#include <sstream>

namespace mc::command {

void SetIdleTimeoutCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto timeoutNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("setidletimeout");
    timeoutNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(3); });
    support::applyMetadata(
        timeoutNode, support::makeMetadata("Set the player idle timeout in minutes.", "/setidletimeout <minutes>", 3));

    auto minutesArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "minutes", IntegerArgumentType::integer(0, 1440));
    minutesArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setTimeout(ctx); });
    timeoutNode->addChild(minutesArg);

    dispatcher.registerCommand(timeoutNode);
}

i32 SetIdleTimeoutCommand::_setTimeout(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const i32 timeout = context.getArgument<i32>("minutes");
    server->setPlayerIdleTimeoutMinutes(timeout);

    std::ostringstream ss;
    ss << "Set idle timeout to " << timeout << " minute(s)";
    source.sendMessage(ss.str());
    return 1;
}

} // namespace mc::command