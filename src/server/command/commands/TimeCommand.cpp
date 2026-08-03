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

#include "TimeCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/TimeArgument.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/TimeManager.hpp"

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

namespace mc {
namespace command {
namespace {

// 时间预设常量
inline constexpr i32 DAY_TIME = 1000;       // 日出后不久
inline constexpr i32 NOON_TIME = 6000;      // 正午
inline constexpr i32 NIGHT_TIME = 13000;    // 日落后不久
inline constexpr i32 MIDNIGHT_TIME = 18000; // 午夜

/**
 * @brief 读取并归一化当前昼夜时间。
 *
 * @param timeManager 时间管理器。
 * @return 归一化到 `[0, 23999]` 区间的昼夜时间。
 */
[[nodiscard]] i32 normalizedDayTime(const server::core::TimeManager& timeManager) noexcept
{
    return static_cast<i32>(timeManager.dayTimeOfDay());
}

/**
 * @brief 统一执行设置时间逻辑。
 *
 * @param source 命令源。
 * @param time 目标时间。
 * @return 设置后的昼夜时间。
 */
[[nodiscard]] i32 setTimeValue(ServerCommandSource& source, i32 time)
{
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    server->timeManager().setDayTime(time);
    const i32 updatedDayTime = normalizedDayTime(server->timeManager());

    std::ostringstream ss;
    ss << "Set the time to " << updatedDayTime;
    source.sendMessage(ss.str());
    return updatedDayTime;
}

} // namespace

void TimeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto setValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");

    auto dayNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("day");
    dayNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("value", DAY_TIME);
        return _setTime(ctx);
    });

    auto noonNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("noon");
    noonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("value", NOON_TIME);
        return _setTime(ctx);
    });

    auto nightNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("night");
    nightNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("value", NIGHT_TIME);
        return _setTime(ctx);
    });

    auto midnightNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("midnight");
    midnightNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("value", MIDNIGHT_TIME);
        return _setTime(ctx);
    });

    auto setArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("value", TimeArgumentType::time());
    setArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setTime(ctx); });

    setValueNode->addChild(dayNode);
    setValueNode->addChild(noonNode);
    setValueNode->addChild(nightNode);
    setValueNode->addChild(midnightNode);
    setValueNode->addChild(setArg);

    auto addValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("value", TimeArgumentType::time());
    addArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addTime(ctx); });
    addValueNode->addChild(addArg);

    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("query");
    auto queryArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("type", StringArgumentType::word());
    queryArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _queryTime(ctx); });
    queryNode->addChild(queryArg);

    auto timeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("time");
    timeNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(
        timeNode, support::makeMetadata("Change or query the world time.", "/time <set|add|query> ...", 2));
    timeNode->addChild(setValueNode);
    timeNode->addChild(addValueNode);
    timeNode->addChild(queryNode);

    dispatcher.registerCommand(timeNode);
}

i32 TimeCommand::_setTime(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    const i32 value = context.getArgument<i32>("value");
    return setTimeValue(source, value);
}

i32 TimeCommand::_addTime(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    const i32 value = context.getArgument<i32>("value");
    server->timeManager().addDayTime(value);
    const i32 updatedDayTime = normalizedDayTime(server->timeManager());

    std::ostringstream ss;
    ss << "Set the time to " << updatedDayTime;
    source.sendMessage(ss.str());
    return updatedDayTime;
}

i32 TimeCommand::_queryTime(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    const std::string type = context.getArgument<std::string>("type");
    const auto& timeManager = server->timeManager();

    i32 value = 0;
    if (type == "day") {
        value = static_cast<i32>(timeManager.dayCount() % INT32_MAX);
    } else if (type == "daytime") {
        value = normalizedDayTime(timeManager);
    } else if (type == "gametime") {
        value = static_cast<i32>(timeManager.gameTime() % INT32_MAX);
    } else {
        source.sendError("Unknown query type: " + type);
        return 0;
    }

    std::ostringstream ss;
    ss << "The time query returned " << value;
    source.sendMessage(ss.str());
    return value;
}

} // namespace command
} // namespace mc
