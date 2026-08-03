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

#include "WhitelistCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/WhitelistManager.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

void WhitelistCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto whitelistNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("whitelist");
    whitelistNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(3); });
    support::applyMetadata(whitelistNode,
        support::makeMetadata(
            "Manages the server whitelist.", "/whitelist <on|off|list|add|remove|reload>", 3, {}, false));

    // /whitelist on
    auto onNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("on");
    onNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _whitelistOn(ctx); });

    // /whitelist off
    auto offNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("off");
    offNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _whitelistOff(ctx); });

    // /whitelist list
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _whitelistList(ctx); });

    // /whitelist add <player>
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addPlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());
    addPlayerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _whitelistAdd(ctx); });
    addNode->addChild(addPlayerArg);

    // /whitelist remove <player>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removePlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());
    removePlayerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _whitelistRemove(ctx); });
    removeNode->addChild(removePlayerArg);

    // /whitelist reload
    auto reloadNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reload");
    reloadNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _whitelistReload(ctx); });

    whitelistNode->addChild(onNode);
    whitelistNode->addChild(offNode);
    whitelistNode->addChild(listNode);
    whitelistNode->addChild(addNode);
    whitelistNode->addChild(removeNode);
    whitelistNode->addChild(reloadNode);

    dispatcher.registerCommand(whitelistNode);
}

i32 WhitelistCommand::_whitelistOn(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.whitelist.failed");
        return 0;
    }

    auto& whitelistManager = server->whitelistManager();

    // 检查是否已启用
    if (whitelistManager.isEnabled()) {
        source.sendError("commands.whitelist.alreadyOn");
        return 0;
    }

    // 启用白名单
    whitelistManager.setEnabled(true);
    source.sendMessage("commands.whitelist.enabled");

    // 踢出不在白名单的玩家
    _kickNonWhitelistedPlayers(source);

    return 1;
}

i32 WhitelistCommand::_whitelistOff(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.whitelist.failed");
        return 0;
    }

    auto& whitelistManager = server->whitelistManager();

    // 检查是否已禁用
    if (!whitelistManager.isEnabled()) {
        source.sendError("commands.whitelist.alreadyOff");
        return 0;
    }

    // 禁用白名单
    whitelistManager.setEnabled(false);
    source.sendMessage("commands.whitelist.disabled");

    return 1;
}

i32 WhitelistCommand::_whitelistList(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.whitelist.failed");
        return 0;
    }

    auto& whitelistManager = server->whitelistManager();
    auto names = whitelistManager.getAllNames();

    if (names.empty()) {
        source.sendMessage("commands.whitelist.list.none");
        return 1;
    }

    std::ostringstream ss;
    ss << "There are " << names.size() << " whitelisted players: ";

    // 按名称排序
    std::sort(names.begin(), names.end());

    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) {
            ss << ", ";
        }
        ss << names[i];
    }

    source.sendMessage(ss.str());
    return static_cast<i32>(names.size());
}

i32 WhitelistCommand::_whitelistAdd(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.whitelist.failed");
        return 0;
    }

    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 获取玩家名
    std::string playerName;
    std::string playerUuid;

    if (selector.hasUsername()) {
        playerName = selector.username();
        // 离线玩家无法获取真实 UUID，使用 MC 原版离线模式 UUID 算法
        // 算法：UUID.nameUUIDFromBytes(("OfflinePlayer:" + name).getBytes(UTF_8))
        // 参考: net.minecraft.core.UUIDUtil.createOfflinePlayerUUID
        playerUuid = util::uuidToStringWithDashes(util::generateOfflineUuid(playerName));
    } else {
        std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
        if (!playerIds.empty()) {
            auto* server = source.server();
            if (server != nullptr) {
                auto* playerData = server->playerManager().getPlayer(playerIds.front());
                if (playerData != nullptr) {
                    playerName = playerData->username;
                    // 在线玩家使用其真实 UUID
                    // ServerPlayerData::uuid 存储为 32 字符无连字符格式，
                    // 白名单 JSON 使用带连字符的标准格式，需要转换
                    const auto& rawUuid = playerData->uuid;
                    if (!rawUuid.empty()) {
                        Uuid parsed = util::uuidFromString(rawUuid);
                        playerUuid = util::uuidToStringWithDashes(parsed);
                    }
                    if (playerUuid.empty()) {
                        // UUID 为空或解析失败时回退到离线模式 UUID
                        playerUuid = util::uuidToStringWithDashes(util::generateOfflineUuid(playerName));
                    }
                }
            }
        }
    }

    if (playerName.empty()) {
        source.sendError("commands.whitelist.add.failed");
        return 0;
    }

    auto& whitelistManager = server->whitelistManager();

    // 检查是否已在白名单中
    if (whitelistManager.isNameWhitelisted(playerName)) {
        source.sendError("commands.whitelist.add.failed");
        return 0;
    }

    // 添加到白名单
    mc::server::core::WhitelistEntry entry(playerUuid, playerName);
    if (whitelistManager.addEntry(entry)) {
        // 保存白名单
        auto saveResult = whitelistManager.save();
        if (saveResult.failed()) {
            spdlog::error("Failed to save whitelist: {}", saveResult.error().message());
        }

        std::ostringstream ss;
        ss << "Added " << playerName << " to the whitelist";
        source.sendMessage(ss.str());

        return 1;
    }

    source.sendError("commands.whitelist.add.failed");
    return 0;
}

i32 WhitelistCommand::_whitelistRemove(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.whitelist.failed");
        return 0;
    }

    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 获取玩家名
    std::string playerName;

    if (selector.hasUsername()) {
        playerName = selector.username();
    } else {
        std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
        if (!playerIds.empty()) {
            auto* server = source.server();
            if (server != nullptr) {
                auto* playerData = server->playerManager().getPlayer(playerIds.front());
                if (playerData != nullptr) {
                    playerName = playerData->username;
                }
            }
        }
    }

    if (playerName.empty()) {
        source.sendError("commands.whitelist.remove.failed");
        return 0;
    }

    auto& whitelistManager = server->whitelistManager();

    // 检查是否在白名单中
    if (!whitelistManager.isNameWhitelisted(playerName)) {
        source.sendError("commands.whitelist.remove.failed");
        return 0;
    }

    // 从白名单移除
    if (whitelistManager.removeEntryByName(playerName)) {
        // 保存白名单
        auto saveResult = whitelistManager.save();
        if (saveResult.failed()) {
            spdlog::error("Failed to save whitelist: {}", saveResult.error().message());
        }

        std::ostringstream ss;
        ss << "Removed " << playerName << " from the whitelist";
        source.sendMessage(ss.str());

        // 踢出不在白名单的玩家（如果白名单已启用）
        if (whitelistManager.isEnabled()) {
            _kickNonWhitelistedPlayers(source);
        }

        return 1;
    }

    source.sendError("commands.whitelist.remove.failed");
    return 0;
}

i32 WhitelistCommand::_whitelistReload(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.whitelist.failed");
        return 0;
    }

    auto& whitelistManager = server->whitelistManager();

    // 重新加载白名单
    auto loadResult = whitelistManager.reload();
    if (loadResult.failed()) {
        spdlog::error("Failed to reload whitelist: {}", loadResult.error().message());
        source.sendError("commands.whitelist.reloaded.failed");
        return 0;
    }

    source.sendMessage("commands.whitelist.reloaded");

    // 踢出不在白名单的玩家（如果白名单已启用）
    if (whitelistManager.isEnabled()) {
        _kickNonWhitelistedPlayers(source);
    }

    return 1;
}

void WhitelistCommand::_kickNonWhitelistedPlayers(ServerCommandSource& source)
{
    auto* server = source.server();
    if (server == nullptr) {
        return;
    }

    auto& whitelistManager = server->whitelistManager();
    auto& playerManager = server->playerManager();
    auto& connectionManager = server->connectionManager();

    // 收集需要踢出的玩家
    std::vector<PlayerId> playersToKick;

    playerManager.forEachPlayer([&](const mc::server::ServerPlayerData& player) {
        // 检查玩家是否在白名单中
        if (!whitelistManager.isNameWhitelisted(player.username)) {
            playersToKick.push_back(player.playerId);
        }
    });

    // 踢出玩家
    for (PlayerId playerId : playersToKick) {
        connectionManager.disconnectPlayer(playerId, "You are not white-listed on this server!");
    }

    if (!playersToKick.empty()) {
        spdlog::info("Kicked {} player(s) not on the whitelist", playersToKick.size());
    }
}

} // namespace command
} // namespace mc
