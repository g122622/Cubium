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

#include "DebugCommand.hpp"

#include "common/command/CommandNode.hpp"
#include "common/command/arguments/FunctionArgument.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/CommandSuggestionProviders.hpp"

#include <memory>

namespace mc::command {
namespace {

/// 函数调试开关（原版 /debug function 控制函数执行时的调试输出）。
bool& _functionDebugEnabled()
{
    static bool s_enabled = false;
    return s_enabled;
}

} // namespace

void DebugCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    // 原版形状：/debug function <name> | /debug start | /debug stop。
    auto debugNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("debug");
    debugNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        debugNode, support::makeMetadata("Toggles debug options.", "/debug <start|stop|function <name>>", 3, {}, true));

    auto functionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("function");
    auto nameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, FunctionArgumentResult>>(
        "name", FunctionArgumentType::functions());
    nameArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _function(ctx);
    });
    functionNode->addChild(nameArg);
    debugNode->addChild(functionNode);

    auto startNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("start");
    startNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _start(ctx);
    });
    debugNode->addChild(startNode);

    auto stopNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("stop");
    stopNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _stop(ctx);
    });
    debugNode->addChild(stopNode);

    dispatcher.registerCommand(debugNode);
}

i32 DebugCommand::_start(CommandContext<ServerCommandSource>& context)
{
    context.getSource().sendError("Debug profiling is not supported by this server");
    return 0;
}

i32 DebugCommand::_stop(CommandContext<ServerCommandSource>& context)
{
    context.getSource().sendError("Debug profiling is not supported by this server");
    return 0;
}

i32 DebugCommand::_function(CommandContext<ServerCommandSource>& context)
{
    auto& enabled = _functionDebugEnabled();
    enabled = !enabled;
    context.getSource().sendMessage(enabled ? "Function debug output enabled" : "Function debug output disabled");
    return 1;
}

} // namespace mc::command
