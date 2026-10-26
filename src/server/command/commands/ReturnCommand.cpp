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
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/command/CommandRegistry.hpp"
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
    // 原版给 run 挂上一个「占位」命令：调用方（如 /execute if function）只要看到 run 节点
    // 被选中，就知道这是一条转发命令，转而执行 run 的兄弟子节点。节点链的实际执行放到
    // _returnRun 里，按解析好的 token 直接调用兄弟节点的命令，而不是把原始文本再解析一遍。
    runNode->setCommand([](CommandContext<ServerCommandSource>&) -> i32 {
        throw CommandException(CommandErrorType::Unknown, "commands.return.forward");
    });
    auto runArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    runArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _returnRun(ctx);
    });
    // 原版的 /return run 是一条 ForwardNode（重定向到根），树上不带 command 子节点；
    // 这里的 command 参数只为本实现复用一次派发，不算协议里的命令树。
    runArg->setTreeHidden(true);
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
    const std::string& command = context.getArgument<std::string>("command");

    // 原版 ForwardingCommand：命令文本先按命令语法解析一次，执行时由转发节点接管——run 节点
    // 本身只有占位命令，真正的执行挂在它的兄弟子节点上（例如 /return run execute as @s run ...）。
    if (command.empty()) {
        // 原版这里是一条 dispatchCommand 的转发命令：run 后面没内容时实体本就存在，
        // 只是没有可执行命令，结果按失败返回而不是报错。
        return ExecuteCommand::executeNestedCommand(source, "");
    }
    const std::string fullCommand = command[0] == '/' ? command : "/" + command;
    auto& registry = CommandRegistry::getGlobal();
    auto parseResult = registry.dispatcher().parse(fullCommand, source);
    if (!parseResult.isSuccess()) {
        const auto& error = parseResult.getError();
        throw CommandException(error.has_value() ? error->type() : CommandErrorType::Unknown,
            error.has_value() ? error->message() : "Invalid command: " + command,
            error.has_value() ? error->cursor() : 0);
    }
    if (parseResult.getContext() == nullptr) {
        source.sendError("commands.return.failed.notFound");
        return 0;
    }
    auto result = registry.dispatcher().execute(parseResult);
    if (result.failed()) {
        source.sendError(result.error().message());
        return 0;
    }
    return result.value().result();
}

i32 ReturnCommand::_returnFail(CommandContext<ServerCommandSource>& context)
{
    // 原版 /return fail：以失败结束，函数链随之停止。
    return 0;
}

} // namespace mc::command
