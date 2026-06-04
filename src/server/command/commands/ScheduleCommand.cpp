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

#include "ScheduleCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <sstream>

namespace mc {
namespace command {

void ScheduleCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto scheduleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("schedule");
    scheduleNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(scheduleNode,
        support::makeMetadata("Schedules a function to run at a later time.",
            "/schedule function <function> <time> [append|replace]",
            2,
            {},
            true));

    // /schedule function <function> <time> [append|replace]
    auto functionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("function");
    auto nameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "function", StringArgumentType::string());
    auto timeArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("time", IntegerArgumentType::integer(1));

    // append 模式
    auto appendNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("append");
    appendNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _scheduleFunction(ctx, true); });
    timeArg->addChild(appendNode);

    // replace 模式
    auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");
    replaceNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _scheduleFunction(ctx, false); });
    timeArg->addChild(replaceNode);

    // 默认为 replace
    timeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _scheduleFunction(ctx, false); });

    nameArg->addChild(timeArg);
    functionNode->addChild(nameArg);
    scheduleNode->addChild(functionNode);

    // /schedule clear <function>
    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    auto clearNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "function", StringArgumentType::string());
    clearNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _clearSchedule(ctx); });
    clearNode->addChild(clearNameArg);
    scheduleNode->addChild(clearNode);

    dispatcher.registerCommand(scheduleNode);
}

i32 ScheduleCommand::_scheduleFunction(CommandContext<ServerCommandSource>& context, bool append)
{
    auto& source = context.getSource();
    const std::string functionName = context.getArgument<std::string>("function");
    const i32 time = context.getArgument<i32>("time");

    std::ostringstream ss;
    ss << "Scheduled function '" << functionName << "' to run in " << time << " ticks";
    if (append) {
        ss << " (append mode)";
    } else {
        ss << " (replace mode)";
    }
    source.sendMessage(ss.str());

    // TODO: 实现函数调度系统
    // 1. 将函数添加到调度队列
    // 2. 设置执行时间
    // 3. 在 tick 系统中检查和执行到期的函数

    return 1;
}

i32 ScheduleCommand::_clearSchedule(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string functionName = context.getArgument<std::string>("function");

    std::ostringstream ss;
    ss << "Cleared scheduled function '" << functionName << "'";
    source.sendMessage(ss.str());

    // TODO: 实现调度清除
    // 从调度队列中移除指定函数的所有待执行实例

    return 1;
}

} // namespace command
} // namespace mc
