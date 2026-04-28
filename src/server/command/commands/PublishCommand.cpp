#include "PublishCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <sstream>

namespace mc {
namespace command {

void PublishCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto publishNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("publish");
    publishNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(4);
    });
    support::applyMetadata(
        publishNode,
        support::makeMetadata(
            "Opens the singleplayer world to the LAN.",
            "/publish [port] [allowCheats]",
            4,
            {},
            true));

    // /publish [port] [allowCheats]
    auto portArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "port",
        IntegerArgumentType::integer(1, 65535));
    auto cheatsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>(
        "allowCheats",
        BoolArgumentType::boolArg());
    cheatsArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return publishToWorld(ctx);
    });
    portArg->addChild(cheatsArg);
    portArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return publishToWorld(ctx);
    });
    publishNode->addChild(portArg);

    // 默认参数
    publishNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return publishToWorld(ctx);
    });

    dispatcher.registerCommand(publishNode);
}

i32 PublishCommand::publishToWorld(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 默认值
    i32 port = 25565;
    bool allowCheats = false;

    if (context.hasArgument("port")) {
        port = context.getArgument<i32>("port");
    }
    if (context.hasArgument("allowCheats")) {
        allowCheats = context.getArgument<bool>("allowCheats");
    }

    // 检查是否为集成服务器
    // TODO: 实现 isIntegrated() 方法
    // auto server = source.server();
    // if (!server->isIntegrated()) {
    //     source.sendError("This command can only be used in singleplayer");
    //     return 0;
    // }

    std::ostringstream ss;
    ss << "Published world to LAN on port " << port;
    if (allowCheats) {
        ss << " with cheats enabled";
    }
    source.sendMessage(ss.str());

    // TODO: 实现局域网发布功能
    // 1. 启动局域网监听
    // 2. 设置是否允许作弊
    // 3. 广播到局域网

    return 1;
}

} // namespace command
} // namespace mc
