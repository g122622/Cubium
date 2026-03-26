#include "WeatherCommand.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include "common/world/weather/WeatherState.hpp"
#include <sstream>

namespace mc {
namespace command {

void WeatherCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    // /weather clear [duration]
    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    auto clearDurationArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "duration",
        IntegerArgumentType::integer(0)
    );
    clearDurationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setClear(ctx);
    });
    clearNode->addChild(clearDurationArg);
    // 不带参数时使用默认值
    clearNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        // 使用默认值 0（内部会使用 DEFAULT_COMMAND_DURATION）
        ctx.setArgument("duration", 0);
        return setClear(ctx);
    });

    // /weather rain [duration]
    auto rainNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("rain");
    auto rainDurationArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "duration",
        IntegerArgumentType::integer(0)
    );
    rainDurationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setRain(ctx);
    });
    rainNode->addChild(rainDurationArg);
    rainNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("duration", 0);
        return setRain(ctx);
    });

    // /weather thunder [duration]
    auto thunderNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("thunder");
    auto thunderDurationArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "duration",
        IntegerArgumentType::integer(0)
    );
    thunderDurationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setThunder(ctx);
    });
    thunderNode->addChild(thunderDurationArg);
    thunderNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        ctx.setArgument("duration", 0);
        return setThunder(ctx);
    });

    // /weather query
    auto queryNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("query");
    queryNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return query(ctx);
    });

    // 创建 weather 字面量节点
    auto weatherNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("weather");
    weatherNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    weatherNode->addChild(clearNode);
    weatherNode->addChild(rainNode);
    weatherNode->addChild(thunderNode);
    weatherNode->addChild(queryNode);

    dispatcher.registerCommand(weatherNode);
}

i32 WeatherCommand::setClear(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();
    if (!server) {
        source.sendMessage("Server not available");
        return 0;
    }

    i32 duration = context.getArgument<i32>("duration");
    server->weatherManager().setClear(duration);

    // 转换为秒显示（1秒 = 20 ticks）
    i32 seconds = duration > 0 ? duration / 20 : 300; // 默认 5 分钟
    std::ostringstream ss;
    ss << "Changing weather to clear for " << seconds << " seconds";
    source.sendMessage(ss.str());

    return 1;
}

i32 WeatherCommand::setRain(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();
    if (!server) {
        source.sendMessage("Server not available");
        return 0;
    }

    i32 duration = context.getArgument<i32>("duration");
    server->weatherManager().setRain(duration);

    i32 seconds = duration > 0 ? duration / 20 : 300;
    std::ostringstream ss;
    ss << "Changing weather to rain for " << seconds << " seconds";
    source.sendMessage(ss.str());

    return 1;
}

i32 WeatherCommand::setThunder(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();
    if (!server) {
        source.sendMessage("Server not available");
        return 0;
    }

    i32 duration = context.getArgument<i32>("duration");
    server->weatherManager().setThunder(duration);

    i32 seconds = duration > 0 ? duration / 20 : 300;
    std::ostringstream ss;
    ss << "Changing weather to thunder for " << seconds << " seconds";
    source.sendMessage(ss.str());

    return 1;
}

i32 WeatherCommand::query(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();
    if (!server) {
        source.sendMessage("Server not available");
        return 0;
    }

    auto& weatherMgr = server->weatherManager();
    auto type = static_cast<i32>(weatherMgr.weatherType());
    f32 rainStrength = weatherMgr.rainStrength();
    f32 thunderStrength = weatherMgr.thunderStrength();

    String typeStr;
    switch (type) {
        case 1:  // Rain
            typeStr = "rain";
            break;
        case 2:  // Thunder
            typeStr = "thunder";
            break;
        default:  // Clear
            typeStr = "clear";
            break;
    }

    std::ostringstream ss;
    ss << "The weather is " << typeStr;
    ss << " (rain strength: " << rainStrength;
    ss << ", thunder strength: " << thunderStrength << ")";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
