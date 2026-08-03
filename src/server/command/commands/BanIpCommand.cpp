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

#include "BanIpCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/BannedIpList.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "common/util/DateTimeUtils.hpp"
#include "server/command/ServerCommandSource.hpp"

#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

namespace {

/**
 * @brief 获取当前时间的格式化字符串
 * @return 格式化的时间字符串 (yyyy-MM-dd HH:mm:ss Z)
 */
std::string getCurrentTimeString()
{
    return util::DateTimeUtils::getCurrentDateTimeString();
}

/**
 * @brief 检查字符串是否为有效的 IPv4 地址
 * @param str 字符串
 * @return true 如果是有效的 IPv4 地址
 */
bool isValidIpv4(const std::string& str)
{
    // IPv4 正则表达式
    static const std::regex ipv4Regex(
        R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    return std::regex_match(str, ipv4Regex);
}

} // anonymous namespace

void BanIpCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto banIpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("ban-ip");
    banIpNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(3); });
    support::applyMetadata(banIpNode,
        support::makeMetadata("Bans an IP address from the server.", "/ban-ip <target> [reason]", 3, {}, false));

    // /ban-ip <target>
    auto targetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _banIp(ctx); });

    // /ban-ip <target> <reason>
    auto reasonArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "reason", StringArgumentType::greedyString());
    reasonArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _banIp(ctx); });

    targetArg->addChild(reasonArg);
    banIpNode->addChild(targetArg);
    dispatcher.registerCommand(banIpNode);
}

i32 BanIpCommand::_banIp(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    std::string target = context.getArgument<std::string>("target");

    // 获取可选原因
    std::string reason = "Banned by an operator";
    if (context.hasArgument("reason")) {
        reason = context.getArgument<std::string>("reason");
    }

    // 获取封禁者名称
    std::string bannedBy = source.name().empty() ? "Server" : source.name();

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.banip.failed.noServer");
        return 0;
    }

    // 判断目标是否为 IP 地址
    bool isIp = isValidIpv4(target);
    std::string ipAddress;
    std::vector<PlayerId> playersToKick;

    if (isIp) {
        // 直接封禁 IP
        ipAddress = target;
        // 查找该 IP 的所有在线玩家
        playersToKick = server->playerManager().getPlayerIdsByAddress(ipAddress);
    } else {
        // 目标是玩家名，需要查找其 IP 地址
        // 使用 PlayerManager::findByUsername 查找玩家
        auto* playerData = server->playerManager().findByUsername(target);

        if (playerData == nullptr) {
            // 玩家不在线，无法获取 IP 地址
            source.sendError("commands.banip.failed.playerNotFound");
            return 0;
        }

        // 获取玩家的 IP 地址
        ipAddress = playerData->ipAddress;
        if (ipAddress.empty()) {
            // 本地连接的玩家（单人游戏），无法封禁 IP
            source.sendError("commands.banip.failed.localPlayer");
            return 0;
        }
        playersToKick.push_back(playerData->playerId);
    }

    auto& banList = server->bannedIpList();

    // 检查是否已被封禁
    if (banList.isBanned(ipAddress)) {
        source.sendError("commands.banip.failed.ipAlreadyBanned");
        return 0;
    }

    // 创建封禁条目
    server::core::BannedIpEntry entry(ipAddress,
        getCurrentTimeString(),
        bannedBy,
        "forever", // 永久封禁
        reason);

    // 添加到封禁列表
    if (!banList.addEntry(entry)) {
        source.sendError("commands.banip.failed.addEntry");
        return 0;
    }

    // 保存封禁列表
    auto saveResult = banList.save();
    if (saveResult.failed()) {
        spdlog::error("Failed to save banned IPs list: {}", saveResult.error().message());
    }

    // 发送成功消息
    std::ostringstream ss;
    ss << "Banned IP " << ipAddress << ": " << reason;
    source.sendMessage(ss.str());

    // 踢出该 IP 的所有在线玩家
    for (PlayerId playerId : playersToKick) {
        std::ostringstream kickReason;
        kickReason << "Your IP has been banned!\nReason: " << reason;
        server->connectionManager().disconnectPlayer(playerId, kickReason.str());
        spdlog::info("Player kicked due to IP ban (playerId={})", playerId);
    }

    return 1;
}

} // namespace command
} // namespace mc
