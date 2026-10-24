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

#include "JfrCommand.hpp"

#include "common/command/CommandNode.hpp"
#include "server/command/support/CommandMetadata.hpp"

#include <memory>

namespace mc::command {

void JfrCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    // 原版形状：/jfr start | /jfr stop。Cubium 未内嵌 JFR（这是 JVM 自带机制），
    // 命令保留为可识别的运维入口，执行时明确告知不支持，而不是假装成功。
    auto jfrNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("jfr");
    jfrNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(4);
    });
    support::applyMetadata(jfrNode, support::makeMetadata("Manage JFR profiling.", "/jfr <start|stop>", 4, {}, true));

    auto startNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("start");
    startNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _start(ctx);
    });
    jfrNode->addChild(startNode);

    auto stopNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("stop");
    stopNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _stop(ctx);
    });
    jfrNode->addChild(stopNode);

    dispatcher.registerCommand(jfrNode);
}

i32 JfrCommand::_start(CommandContext<ServerCommandSource>& context)
{
    context.getSource().sendError("JFR profiling is not supported by this server");
    return 0;
}

i32 JfrCommand::_stop(CommandContext<ServerCommandSource>& context)
{
    context.getSource().sendError("JFR profiling is not supported by this server");
    return 0;
}

} // namespace mc::command
