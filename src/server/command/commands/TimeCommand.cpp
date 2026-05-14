#include "TimeCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/TimeManager.hpp"

#include <sstream>

namespace mc {
namespace command {
namespace {

inline constexpr i32 DAY_TIME = 1000;
inline constexpr i32 NOON_TIME = 6000;
inline constexpr i32 NIGHT_TIME = 13000;
inline constexpr i32 MIDNIGHT_TIME = 18000;
inline constexpr i64 DAY_LENGTH = 24000;

/**
 * @brief 读取并归一化当前昼夜时间。
 *
 * @param timeManager 时间管理器。
 * @return 归一化到 `[0, 23999]` 区间的昼夜时间。
 */
[[nodiscard]] i32 normalizedDayTime(const server::core::TimeManager& timeManager)
{
    return static_cast<i32>(timeManager.dayTime() % DAY_LENGTH);
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
        return setTime(ctx);
    });

    auto noonNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("noon");
    noonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("value", NOON_TIME);
        return setTime(ctx);
    });

    auto nightNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("night");
    nightNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("value", NIGHT_TIME);
        return setTime(ctx);
    });

    auto midnightNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("midnight");
    midnightNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("value", MIDNIGHT_TIME);
        return setTime(ctx);
    });

    auto setArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("value", IntegerArgumentType::integer(0));
    setArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return setTime(ctx); });

    setValueNode->addChild(dayNode);
    setValueNode->addChild(noonNode);
    setValueNode->addChild(nightNode);
    setValueNode->addChild(midnightNode);
    setValueNode->addChild(setArg);

    auto addValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("value", IntegerArgumentType::integer(0));
    addArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return addTime(ctx); });
    addValueNode->addChild(addArg);

    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("query");
    auto queryArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("type", StringArgumentType::word());
    queryArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return queryTime(ctx); });
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

/**
 * @brief 设置世界昼夜时间。
 *
 * @param context 命令上下文。
 * @return 设置后的昼夜时间。
 */
i32 TimeCommand::setTime(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const i32 value = context.getArgument<i32>("value");
    return setTimeValue(source, value);
}

/**
 * @brief 在当前昼夜时间基础上增加 tick。
 *
 * @param context 命令上下文。
 * @return 增加后的昼夜时间。
 */
i32 TimeCommand::addTime(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const i32 value = context.getArgument<i32>("value");
    server->timeManager().addDayTime(value);
    const i32 updatedDayTime = normalizedDayTime(server->timeManager());

    std::ostringstream ss;
    ss << "Set the time to " << updatedDayTime;
    source.sendMessage(ss.str());
    return updatedDayTime;
}

/**
 * @brief 查询当前时间信息。
 *
 * @param context 命令上下文。
 * @return 查询值。
 */
i32 TimeCommand::queryTime(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

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
