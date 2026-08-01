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

#include "MessageCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

void MessageCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    // 主命令 /msg
    auto msgNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("msg");
    support::applyMetadata(msgNode,
        support::makeMetadata("Sends a private message to another player.", "/msg <player> <message>", 0, {}, true));

    // 别名 /tell
    auto tellNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("tell");
    tellNode->setRedirect(msgNode);

    // 别名 /w (whisper)
    auto wNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("w");
    wNode->setRedirect(msgNode);

    // /msg <player> <message>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());

    auto messageNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "message", StringArgumentType::greedyString());
    messageNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _sendMessage(ctx); });

    playerNode->addChild(messageNode);
    msgNode->addChild(playerNode);

    dispatcher.registerCommand(msgNode);
    dispatcher.registerCommand(tellNode);
    dispatcher.registerCommand(wNode);
}

i32 MessageCommand::_sendMessage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const std::string& message = context.getArgument<std::string>("message");
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("Server not available");
        return 0;
    }

    auto& playerManager = server->playerManager();

    // 获取发送者名称
    std::string senderName = source.name();

    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        auto* targetData = playerManager.getPlayer(playerId);
        if (!targetData) {
            continue;
        }

        // 构建私聊消息给接收者
        // 格式: "<sender> whispers to you: <message>" (灰色斜体)
        std::ostringstream incomingMsg;
        incomingMsg << "§7§o" << senderName << " whispers to you: " << message;

        // 1.21.11 SystemChat(overlay=false)：私聊文本含 §格式码，按纯文本折叠为 StringTag。
        // §格式码由客户端在渲染时解析（vanilla Component.literal 同样保留 §）。
        auto conn = targetData->getConnection();
        if (conn && conn->isConnected()) {
            mc::network::ir::play::SystemChat pkt;
            pkt.content = ::mc::text::plainTextToNbtBytes(incomingMsg.str());
            pkt.overlay = false;
            auto sendResult = conn->send(mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}});
            if (sendResult.failed()) {
                spdlog::warn("MessageCommand: failed to send private message to {} ({})",
                    targetData->username,
                    sendResult.error().message());
            }
        }
        successCount++;
    }

    // 给发送者确认
    if (successCount > 0) {
        if (playerIds.size() == 1) {
            auto* targetData = playerManager.getPlayer(playerIds[0]);
            if (targetData) {
                // 格式: "You whisper to <target>: <message>" (灰色斜体)
                std::ostringstream outgoingMsg;
                outgoingMsg << "§7§oYou whisper to " << targetData->username << ": " << message;
                source.sendMessage(outgoingMsg.str());
            }
        } else {
            // 多个接收者
            std::ostringstream outgoingMsg;
            outgoingMsg << "§7§oYou whisper to " << successCount << " players: " << message;
            source.sendMessage(outgoingMsg.str());
        }
    }

    return successCount;
}

} // namespace command
} // namespace mc
