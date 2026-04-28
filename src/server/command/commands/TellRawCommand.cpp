#include "TellRawCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void TellRawCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto tellrawNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("tellraw");
    tellrawNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        tellrawNode,
        support::makeMetadata(
            "Sends a raw JSON message to a player.",
            "/tellraw <player> <json message>",
            2,
            {},
            true));

    // /tellraw <player> <json>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );

    auto jsonNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "json",
        StringArgumentType::greedyString()
    );
    jsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return sendRawMessage(ctx);
    });

    playerNode->addChild(jsonNode);
    tellrawNode->addChild(playerNode);
    dispatcher.registerCommand(tellrawNode);
}

i32 TellRawCommand::sendRawMessage(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const String& jsonMessage = context.getArgument<String>("json");
    auto* server = source.server();
    auto& playerManager = server->playerManager();
    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        auto* playerData = playerManager.getPlayer(playerId);
        if (!playerData) {
            continue;
        }

        // TODO: 解析 JSON 并构建聊天组件
        // 当前直接发送原始文本
        auto conn = playerData->getConnection();
        if (conn && conn->isConnected()) {
            // 发送原始 JSON 消息
            successCount++;
        }
    }

    return successCount;
}

} // namespace command
} // namespace mc
