#include "TimeCommand.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/core/TimeManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void TimeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    // /time set <value>
    auto setValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "value",
        IntegerArgumentType::integer(0)
    );
    setArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setTime(ctx);
    });
    setValueNode->addChild(setArg);

    // /time add <value>
    auto addValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "value",
        IntegerArgumentType::integer(0)
    );
    addArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addTime(ctx);
    });
    addValueNode->addChild(addArg);

    // /time query <day|daytime|gametime>
    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("query");
    auto queryArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "type",
        StringArgumentType::word()
    );
    queryArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return queryTime(ctx);
    });
    queryNode->addChild(queryArg);

    // 创建 time 字面量节点
    auto timeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("time");
    timeNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    timeNode->addChild(setValueNode);
    timeNode->addChild(addValueNode);
    timeNode->addChild(queryNode);

    dispatcher.registerCommand(timeNode);
}

i32 TimeCommand::setTime(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();
    if (!server) {
        source.sendMessage("Server not available");
        return 0;
    }

    i32 value = context.getArgument<i32>("value");
    server->timeManager().setDayTime(value);

    std::ostringstream ss;
    ss << "Set the time to " << value;
    source.sendMessage(ss.str());

    return 1;
}

i32 TimeCommand::addTime(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();
    if (!server) {
        source.sendMessage("Server not available");
        return 0;
    }

    i32 value = context.getArgument<i32>("value");
    server->timeManager().addDayTime(value);

    std::ostringstream ss;
    ss << "Added " << value << " to the time";
    source.sendMessage(ss.str());

    return 1;
}

i32 TimeCommand::queryTime(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();
    if (!server) {
        source.sendMessage("Server not available");
        return 0;
    }

    String type = context.getArgument<String>("type");
    auto& timeMgr = server->timeManager();

    i64 time = 0;

    if (type == "day") {
        time = timeMgr.dayCount();
        std::ostringstream ss;
        ss << "The day is " << time;
        source.sendMessage(ss.str());
    } else if (type == "daytime") {
        time = timeMgr.dayTime();
        std::ostringstream ss;
        ss << "The daytime is " << time;
        source.sendMessage(ss.str());
    } else if (type == "gametime") {
        time = timeMgr.gameTime();
        std::ostringstream ss;
        ss << "The game time is " << time;
        source.sendMessage(ss.str());
    } else {
        source.sendMessage("Unknown query type: " + type);
        return 0;
    }

    return 1;
}

} // namespace command
} // namespace mc
