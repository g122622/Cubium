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

#include "ReturnCommand.hpp"

#include "ExecuteCommand.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/command/support/CommandMetadata.hpp"

#include <memory>
#include <string>

namespace mc::command {

void ReturnCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    // 原版形状：/return <value> | /return run <command> | /return fail
    auto returnNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("return");
    returnNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        returnNode, support::makeMetadata("Return a value from the current function.", "/return <value>", 2, {}, true));

    auto valueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("value", IntegerArgumentType::integer());
    valueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _returnValue(ctx);
    });
    returnNode->addChild(valueArg);

    auto runNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto runArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    runArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _returnRun(ctx);
    });
    runNode->addChild(runArg);
    returnNode->addChild(runNode);

    auto failNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("fail");
    failNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _returnFail(ctx);
    });
    returnNode->addChild(failNode);

    dispatcher.registerCommand(returnNode);
}

i32 ReturnCommand::_returnValue(CommandContext<ServerCommandSource>& context)
{
    // 原版：以给定值作为命令结果返回。
    return context.getArgument<i32>("value");
}

i32 ReturnCommand::_returnRun(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    // 与 /execute run 共用同一套嵌套执行路径（CommandRegistry::execute 返回命令结果）。
    return ExecuteCommand::executeNestedCommand(source, context.getArgument<std::string>("command"));
}

i32 ReturnCommand::_returnFail(CommandContext<ServerCommandSource>& context)
{
    // 原版 /return fail：以失败结束，函数链随之停止。
    return 0;
}

} // namespace mc::command
