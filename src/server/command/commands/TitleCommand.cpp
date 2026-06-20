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

#include "TitleCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/TimeArgument.hpp"
#include "common/network/packet/TitlePacket.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include <sstream>

namespace mc {
namespace command {

namespace {
/**
 * @brief 发送 TitlePacket 给指定玩家
 *
 * @param connMgr 连接管理器
 * @param playerId 目标玩家ID
 * @param packet 标题包
 * @return 是否发送成功
 */
bool sendTitlePacket(server::core::ConnectionManager& connMgr, PlayerId playerId, const network::TitlePacket& packet)
{
    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize TitlePacket: {}", result.error().message());
        return false;
    }

    return connMgr.sendPacketToPlayer(playerId, network::PacketType::Title, result.value());
}

/**
 * @brief 广播 TitlePacket 给所有指定玩家
 *
 * @param source 命令源
 * @param playerIds 目标玩家ID列表
 * @param packet 标题包
 * @return 成功发送的玩家数量
 */
i32 broadcastTitlePacket(
    ServerCommandSource& source, const std::vector<PlayerId>& playerIds, const network::TitlePacket& packet)
{
    auto* server = source.server();
    if (!server) {
        return 0;
    }

    auto& connMgr = server->connectionManager();
    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        if (sendTitlePacket(connMgr, playerId, packet)) {
            successCount++;
        }
    }

    return successCount;
}
} // namespace

void TitleCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto titleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("title");
    titleNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(titleNode,
        support::makeMetadata("Controls screen title display.",
            "/title <player> (clear|reset|title|subtitle|actionbar|times) ...",
            2,
            {},
            true));

    // /title <player>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::players());

    // clear 子命令
    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    clearNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _clearTitle(ctx); });

    // reset 子命令
    auto resetNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reset");
    resetNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _resetTitle(ctx); });

    // title <json> 子命令
    auto titleTextNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("title");
    auto titleJsonNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "json", StringArgumentType::greedyString());
    titleJsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setTitle(ctx); });
    titleTextNode->addChild(titleJsonNode);

    // subtitle <json> 子命令
    auto subtitleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("subtitle");
    auto subtitleJsonNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "json", StringArgumentType::greedyString());
    subtitleJsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setSubtitle(ctx); });
    subtitleNode->addChild(subtitleJsonNode);

    // actionbar <json> 子命令
    auto actionbarNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("actionbar");
    auto actionbarJsonNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "json", StringArgumentType::greedyString());
    actionbarJsonNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setActionbar(ctx); });
    actionbarNode->addChild(actionbarJsonNode);

    // times <fadeIn> <stay> <fadeOut> 子命令
    auto timesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("times");
    auto fadeInNode =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("fadeIn", TimeArgumentType::time());
    auto stayNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("stay", TimeArgumentType::time());
    auto fadeOutNode =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("fadeOut", TimeArgumentType::time());
    fadeOutNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setTimes(ctx); });

    fadeInNode->addChild(stayNode);
    stayNode->addChild(fadeOutNode);
    timesNode->addChild(fadeInNode);

    playerNode->addChild(clearNode);
    playerNode->addChild(resetNode);
    playerNode->addChild(titleTextNode);
    playerNode->addChild(subtitleNode);
    playerNode->addChild(actionbarNode);
    playerNode->addChild(timesNode);
    titleNode->addChild(playerNode);

    dispatcher.registerCommand(titleNode);
}

i32 TitleCommand::_clearTitle(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 创建并广播清除标题包
    auto packet = network::TitlePacket::createClear();
    return broadcastTitlePacket(source, playerIds, packet);
}

i32 TitleCommand::_resetTitle(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 创建并广播重置标题包
    auto packet = network::TitlePacket::createReset();
    return broadcastTitlePacket(source, playerIds, packet);
}

i32 TitleCommand::_setTitle(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 获取JSON文本参数
    const std::string& jsonText = context.getArgument<std::string>("json");

    // 创建并广播主标题包
    auto packet = network::TitlePacket::createTitle(jsonText);
    return broadcastTitlePacket(source, playerIds, packet);
}

i32 TitleCommand::_setSubtitle(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 获取JSON文本参数
    const std::string& jsonText = context.getArgument<std::string>("json");

    // 创建并广播副标题包
    auto packet = network::TitlePacket::createSubtitle(jsonText);
    return broadcastTitlePacket(source, playerIds, packet);
}

i32 TitleCommand::_setActionbar(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 获取JSON文本参数
    const std::string& jsonText = context.getArgument<std::string>("json");

    // 创建并广播动作栏包
    auto packet = network::TitlePacket::createActionbar(jsonText);
    return broadcastTitlePacket(source, playerIds, packet);
}

i32 TitleCommand::_setTimes(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 获取时间参数（单位：tick）
    i32 fadeIn = context.getArgument<i32>("fadeIn");
    i32 stay = context.getArgument<i32>("stay");
    i32 fadeOut = context.getArgument<i32>("fadeOut");

    // 创建并广播时间设置包
    auto packet = network::TitlePacket::createTimes(fadeIn, stay, fadeOut);
    return broadcastTitlePacket(source, playerIds, packet);
}

} // namespace command
} // namespace mc
