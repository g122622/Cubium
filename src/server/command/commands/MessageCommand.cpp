#include "MessageCommand.hpp"

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

void MessageCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    // 主命令 /msg
    auto msgNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("msg");
    support::applyMetadata(
        msgNode,
        support::makeMetadata(
            "Sends a private message to another player.",
            "/msg <player> <message>",
            0,
            {},
            true));

    // 别名 /tell
    auto tellNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("tell");
    tellNode->setRedirect(msgNode);

    // 别名 /w (whisper)
    auto wNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("w");
    wNode->setRedirect(msgNode);

    // /msg <player> <message>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );

    auto messageNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "message",
        StringArgumentType::greedyString()
    );
    messageNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return sendMessage(ctx);
    });

    playerNode->addChild(messageNode);
    msgNode->addChild(playerNode);

    dispatcher.registerCommand(msgNode);
    dispatcher.registerCommand(tellNode);
    dispatcher.registerCommand(wNode);
}

i32 MessageCommand::sendMessage(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const String& message = context.getArgument<String>("message");
    auto* server = source.server();
    auto& playerManager = server->playerManager();

    // 获取发送者名称
    String senderName = source.name();

    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        auto* targetData = playerManager.getPlayer(playerId);
        if (!targetData) {
            continue;
        }

        // 发送私聊消息给目标玩家
        std::ostringstream ss;
        ss << senderName << " whispers to you: " << message;

        // 通过连接发送消息
        auto conn = targetData->getConnection();
        if (conn && conn->isConnected()) {
            // TODO: 使用专用的私聊消息包
            successCount++;
        }
    }

    // 给发送者确认
    if (successCount > 0 && playerIds.size() == 1) {
        auto* targetData = playerManager.getPlayer(playerIds[0]);
        if (targetData) {
            std::ostringstream ss;
            ss << "You whisper to " << targetData->username << ": " << message;
            source.sendMessage(ss.str());
        }
    }

    return successCount;
}

} // namespace command
} // namespace mc
