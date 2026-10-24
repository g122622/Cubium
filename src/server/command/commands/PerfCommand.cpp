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

#include "PerfCommand.hpp"

#include "common/command/CommandNode.hpp"
#include "server/command/support/CommandMetadata.hpp"

#include <memory>

namespace mc::command {

void PerfCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    // 原版形状：/perf start | /perf stop。采样器尚未实现，命令保留并在执行时明确拒绝。
    auto perfNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("perf");
    perfNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(4); });
    support::applyMetadata(perfNode, support::makeMetadata("Profile the server.", "/perf <start|stop>", 4, {}, true));

    auto startNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("start");
    startNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _start(ctx); });
    perfNode->addChild(startNode);

    auto stopNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("stop");
    stopNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _stop(ctx); });
    perfNode->addChild(stopNode);

    dispatcher.registerCommand(perfNode);
}

i32 PerfCommand::_start(CommandContext<ServerCommandSource>& context)
{
    context.getSource().sendError("Profiling is not supported by this server");
    return 0;
}

i32 PerfCommand::_stop(CommandContext<ServerCommandSource>& context)
{
    context.getSource().sendError("Profiling is not supported by this server");
    return 0;
}

} // namespace mc::command
