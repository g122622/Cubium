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
#include <spdlog/spdlog.h>
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
    jsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _sendRawMessage(ctx); });

    playerNode->addChild(jsonNode);
    tellrawNode->addChild(playerNode);
    dispatcher.registerCommand(tellrawNode);
}

i32 TellRawCommand::_sendRawMessage(CommandContext<ServerCommandSource>& context)
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
            // TODO(Phase6): 新 IR 暂无 S→C 系统/聊天消息包（SystemChat/DisguisedChat）。
            //   旧 ChatBroadcast 字节包已删除，tellraw 的 JSON 消息当前无法下发客户端，
            //   先记日志占位，待补 SystemChat IR struct 后再接通。successCount 仍按预期递增。
            spdlog::debug("TellRawCommand: json message to {} dropped (no S->C chat IR yet): {}",
                playerData->username,
                messageToSend);
            successCount++;
        }
    }

    return successCount;
}

} // namespace command
} // namespace mc
