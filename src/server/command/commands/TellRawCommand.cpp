#include "TellRawCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>
#include <nlohmann/json.hpp>

namespace mc {
namespace command {

void TellRawCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto tellrawNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("tellraw");
    tellrawNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(tellrawNode,
        support::makeMetadata(
            "Sends a raw JSON message to a player.", "/tellraw <player> <json message>", 2, {}, true));

    // /tellraw <player> <json>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());

    auto jsonNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "json", StringArgumentType::greedyString());
    jsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return sendRawMessage(ctx); });

    playerNode->addChild(jsonNode);
    tellrawNode->addChild(playerNode);
    dispatcher.registerCommand(tellrawNode);
}

i32 TellRawCommand::sendRawMessage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const std::string& jsonMessage = context.getArgument<std::string>("json");
    auto* server = source.server();
    auto& playerManager = server->playerManager();
    i32 successCount = 0;

    // 解析 JSON 并构建聊天组件
    std::string messageToSend;
    try {
        nlohmann::json json = nlohmann::json::parse(jsonMessage);
        // 成功解析 JSON，直接使用原始 JSON 字符串发送
        // 客户端会自行解析 JSON 格式的聊天消息
        messageToSend = jsonMessage;
    }
    catch (const nlohmann::json::exception&) {
        // JSON 解析失败，将其作为纯文本发送
        // 先尝试作为 JSON 字符串解析（带引号的字符串）
        try {
            nlohmann::json json = nlohmann::json::parse("\"" + jsonMessage + "\"");
            messageToSend = jsonMessage; // 纯文本，直接发送
        }
        catch (...) {
            // 完全无法解析，发送错误信息
            source.sendError("Invalid JSON: " + jsonMessage);
            return 0;
        }
    }

    for (PlayerId playerId : playerIds) {
        auto* playerData = playerManager.getPlayer(playerId);
        if (!playerData) {
            continue;
        }

        auto conn = playerData->getConnection();
        if (conn && conn->isConnected()) {
            // 发送 JSON 格式的聊天消息
            network::ChatMessagePacket chatPacket(messageToSend, 0);
            network::PacketSerializer payload;
            chatPacket.serialize(payload);

            if (server->connectionManager().sendPacketToPlayer(
                    playerId, network::PacketType::ChatBroadcast, payload.buffer())) {
                successCount++;
            }
        }
    }

    return successCount;
}

} // namespace command
} // namespace mc
