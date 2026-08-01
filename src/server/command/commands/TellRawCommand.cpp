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
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

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

    // 把 tellraw 的 JSON 文本解析为 1.21.11 Component NBT wire 字节。
    // 合法 JSON（含纯文本字符串）→ 复杂/折叠 Component NBT；解析失败回退为纯文本 StringTag。
    std::vector<u8> contentNbt;
    bool jsonValid = false;
    try {
        nlohmann::json json = nlohmann::json::parse(jsonMessage);
        jsonValid = true;
        contentNbt = ::mc::text::parseJsonComponentToNbtBytes(jsonMessage);
    }
    catch (const nlohmann::json::exception&) {
        // 非合法 JSON：按纯文本发送（对齐 vanilla tellraw 对非 JSON 输入的宽容处理）
        contentNbt = ::mc::text::plainTextToNbtBytes(jsonMessage);
    }
    (void)jsonValid;

    for (PlayerId playerId : playerIds) {
        auto* playerData = playerManager.getPlayer(playerId);
        if (!playerData) {
            continue;
        }

        auto conn = playerData->getConnection();
        if (conn && conn->isConnected()) {
            // 1.21.11 SystemChat(overlay=false)：content 为 Component NBT，显示在聊天窗口。
            mc::network::ir::play::SystemChat pkt;
            pkt.content = contentNbt;
            pkt.overlay = false;
            auto sendResult = conn->send(mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}});
            if (sendResult.failed()) {
                spdlog::warn("TellRawCommand: failed to send system chat to {} ({})",
                    playerData->username,
                    sendResult.error().message());
            }
            successCount++;
        }
    }

    return successCount;
}

} // namespace command
} // namespace mc
