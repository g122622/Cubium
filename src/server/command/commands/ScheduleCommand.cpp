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
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
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
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/FunctionArgument.hpp"
#include "common/command/arguments/TimeArgument.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/FunctionSuggestionProvider.hpp"
#include "server/function/FunctionManager.hpp"
#include "server/function/TimerQueue.hpp"
#include <limits>
#include <memory>
#include <sstream>
#include <string>

namespace mc {
namespace command {

/// 错误：尝试在同 tick 调度函数（time == 0）
static const CommandException ERROR_SAME_TICK =
    CommandException(CommandErrorType::DispatcherUnknownCommand, "Cannot schedule a function to run in the same tick");

/// 错误：尝试清除不存在的调度
static const CommandException ERROR_CANT_REMOVE =
    CommandException(CommandErrorType::DispatcherUnknownCommand, "No scheduled function found");

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
    auto nameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, FunctionArgumentResult>>(
        "function", FunctionArgumentType::functions());
    nameArg->setCustomSuggestions(std::make_shared<FunctionSuggestionProvider>());
    auto timeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("time", TimeArgumentType::time());

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
    auto clearNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, FunctionArgumentResult>>(
        "function", FunctionArgumentType::functions());
    clearNameArg->setCustomSuggestions(std::make_shared<FunctionSuggestionProvider>());
    clearNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _clearSchedule(ctx); });
    clearNode->addChild(clearNameArg);
    scheduleNode->addChild(clearNode);

    dispatcher.registerCommand(scheduleNode);
}

i32 ScheduleCommand::_scheduleFunction(CommandContext<ServerCommandSource>& context, bool append)
{
    auto& source = context.getSource();
    auto funcResult = FunctionArgumentType::getFunctionResult(context, "function");
    const i32 time = context.getArgument<i32>("time");

    // 不允许在同 tick 调度
    if (time == 0) {
        throw ERROR_SAME_TICK;
    }

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("Schedule command requires a server instance");
        return 0;
    }

    // /schedule 仅支持直接函数引用，不支持标签引用
    if (funcResult.isTag()) {
        source.sendError("Cannot schedule a function tag, only individual functions can be scheduled");
        return 0;
    }

    const auto& functionId = funcResult.id();
    auto& functionManager = server->functionManager();
    auto& timerQueue = server->functionTimerQueue();

    // 检查函数是否存在
    if (!functionManager.hasFunction(functionId)) {
        std::ostringstream ss;
        ss << "Unknown function '" << functionId.toString() << "'";
        source.sendError(ss.str());
        return 0;
    }

    // 计算目标 tick
    u64 currentTick = server->currentTick();
    u64 targetTick = currentTick + static_cast<u64>(time);
    std::string eventId = functionId.toString();

    // replace 模式：先移除同名的已有调度
    if (!append) {
        timerQueue.remove(eventId);
    }

    // 添加调度事件
    timerQueue.scheduleFunction(eventId, functionId, targetTick);

    // 反馈
    std::ostringstream ss;
    ss << "Scheduled function '" << functionId.toString() << "' to run in " << time << " ticks";
    if (append) {
        ss << " (append mode)";
    } else {
        ss << " (replace mode)";
    }
    source.sendMessage(ss.str());

    return static_cast<i32>(targetTick % static_cast<u64>(std::numeric_limits<i32>::max()));
}

i32 ScheduleCommand::_clearSchedule(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto funcResult = FunctionArgumentType::getFunctionResult(context, "function");

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("Schedule command requires a server instance");
        return 0;
    }

    auto& timerQueue = server->functionTimerQueue();

    // 使用函数 ID 作为事件名
    std::string eventId = funcResult.id().toString();

    i32 removedCount = timerQueue.remove(eventId);

    if (removedCount == 0) {
        throw ERROR_CANT_REMOVE;
    }

    std::ostringstream ss;
    ss << "Cleared " << removedCount << " scheduled function(s) for '" << funcResult.id().toString() << "'";
    source.sendMessage(ss.str());

    return removedCount;
}

} // namespace command
} // namespace mc
