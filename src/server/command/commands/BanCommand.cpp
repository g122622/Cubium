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

#include "BanCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/BannedPlayerList.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "common/util/DateTimeUtils.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>
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

} // anonymous namespace

void BanCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto banNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("ban");
    banNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(3); });
    support::applyMetadata(
        banNode, support::makeMetadata("Bans a player from the server.", "/ban <player> [reason]", 3, {}, false));

    // /ban <player>
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _banPlayer(ctx); });

    // /ban <player> <reason>
    auto reasonArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "reason", StringArgumentType::greedyString());
    reasonArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _banPlayer(ctx); });

    playerArg->addChild(reasonArg);
    banNode->addChild(playerArg);
    dispatcher.registerCommand(banNode);
}

i32 BanCommand::_banPlayer(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 获取可选原因
    std::string reason = "Banned by an operator";
    if (context.hasArgument("reason")) {
        reason = context.getArgument<std::string>("reason");
    }

    // 获取封禁者名称
    std::string bannedBy = source.name().empty() ? "Server" : source.name();

    // 解析目标玩家
    std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);

    std::string playerName;
    std::string playerUuid;
    PlayerId playerId = 0;

    if (!playerIds.empty()) {
        playerId = playerIds.front();
        auto* server = source.server();
        if (server != nullptr) {
            auto* playerData = server->playerManager().getPlayer(playerId);
            if (playerData != nullptr) {
                playerName = playerData->username;
                // 在线玩家使用其真实 UUID
                // ServerPlayerData::uuid 存储为 32 字符无连字符格式，
                // 封禁列表 JSON 使用带连字符的标准格式，需要转换
                const auto& rawUuid = playerData->uuid;
                if (!rawUuid.empty()) {
                    Uuid parsed = util::uuidFromString(rawUuid);
                    playerUuid = util::uuidToStringWithDashes(parsed);
                }
            }
        }
    }

    // 如果玩家不在线，尝试从选择器获取用户名
    if (playerName.empty() && selector.hasUsername()) {
        playerName = selector.username();
        // 离线玩家使用 MC 原版离线模式 UUID 算法
        // 算法：UUID.nameUUIDFromBytes(("OfflinePlayer:" + name).getBytes(UTF_8))
        // 参考: net.minecraft.core.UUIDUtil.createOfflinePlayerUUID
        playerUuid = util::uuidToStringWithDashes(util::generateOfflineUuid(playerName));
    }

    if (playerName.empty()) {
        source.sendError("commands.ban.failed.noPlayer");
        return 0;
    }

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.ban.failed.noServer");
        return 0;
    }

    auto& banList = server->bannedPlayerList();

    // 检查是否已被封禁
    if (banList.isNameBanned(playerName)) {
        source.sendError("commands.ban.failed.playerAlreadyBanned");
        return 0;
    }

    // 创建封禁条目
    server::core::BannedPlayerEntry entry(playerUuid,
        playerName,
        getCurrentTimeString(),
        bannedBy,
        "forever", // 永久封禁
        reason);

    // 添加到封禁列表
    if (!banList.addEntry(entry)) {
        source.sendError("commands.ban.failed.addEntry");
        return 0;
    }

    // 保存封禁列表
    auto saveResult = banList.save();
    if (saveResult.failed()) {
        spdlog::error("Failed to save banned players list: {}", saveResult.error().message());
    }

    // 发送成功消息
    std::ostringstream ss;
    ss << "Banned " << playerName << ": " << reason;
    source.sendMessage(ss.str());

    // 如果玩家在线，踢出玩家
    if (playerId != 0) {
        std::ostringstream kickReason;
        kickReason << "You are banned from this server!\nReason: " << reason;
        server->connectionManager().disconnectPlayer(playerId, kickReason.str());
        spdlog::info("Player {} ({}) kicked due to ban", playerName, playerId);
    }

    return 1;
}

} // namespace command
} // namespace mc
