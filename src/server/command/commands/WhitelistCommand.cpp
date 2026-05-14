#include "WhitelistCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/WhitelistManager.hpp"

#include <algorithm>
#include <sstream>
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
    onNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return whitelistOn(ctx); });

    // /whitelist off
    auto offNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("off");
    offNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return whitelistOff(ctx); });

    // /whitelist list
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return whitelistList(ctx); });

    // /whitelist add <player>
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addPlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());
    addPlayerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return whitelistAdd(ctx); });
    addNode->addChild(addPlayerArg);

    // /whitelist remove <player>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removePlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());
    removePlayerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return whitelistRemove(ctx); });
    removeNode->addChild(removePlayerArg);

    // /whitelist reload
    auto reloadNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reload");
    reloadNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return whitelistReload(ctx); });

    whitelistNode->addChild(onNode);
    whitelistNode->addChild(offNode);
    whitelistNode->addChild(listNode);
    whitelistNode->addChild(addNode);
    whitelistNode->addChild(removeNode);
    whitelistNode->addChild(reloadNode);

    dispatcher.registerCommand(whitelistNode);
}

i32 WhitelistCommand::whitelistOn(CommandContext<ServerCommandSource>& context)
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
    kickNonWhitelistedPlayers(source);

    return 1;
}

i32 WhitelistCommand::whitelistOff(CommandContext<ServerCommandSource>& context)
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

i32 WhitelistCommand::whitelistList(CommandContext<ServerCommandSource>& context)
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

    // 按 MC 1.16.5 格式输出
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

i32 WhitelistCommand::whitelistAdd(CommandContext<ServerCommandSource>& context)
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
        // UUID 暂时使用玩家名生成（实际应从 Mojang API 获取）
        // 在真实服务器中，这需要查询 Mojang API
        playerUuid = generateUuidFromName(playerName);
    } else {
        std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
        if (!playerIds.empty()) {
            auto* server = source.server();
            if (server != nullptr) {
                auto* playerData = server->playerManager().getPlayer(playerIds.front());
                if (playerData != nullptr) {
                    playerName = playerData->username;
                    // 使用玩家 ID 作为临时 UUID
                    playerUuid = std::to_string(playerIds.front());
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

        // 如果白名单已启用，踢出不在白名单的玩家
        if (whitelistManager.isEnabled()) {
            // 注意：不需要在这里踢人，因为新添加的玩家已经在白名单中
        }

        return 1;
    }

    source.sendError("commands.whitelist.add.failed");
    return 0;
}

i32 WhitelistCommand::whitelistRemove(CommandContext<ServerCommandSource>& context)
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
            kickNonWhitelistedPlayers(source);
        }

        return 1;
    }

    source.sendError("commands.whitelist.remove.failed");
    return 0;
}

i32 WhitelistCommand::whitelistReload(CommandContext<ServerCommandSource>& context)
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
        kickNonWhitelistedPlayers(source);
    }

    return 1;
}

void WhitelistCommand::kickNonWhitelistedPlayers(ServerCommandSource& source)
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
        // 首先尝试通过名称检查（MC 1.16.5 行为）
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

std::string WhitelistCommand::generateUuidFromName(const std::string& name)
{
    // 生成基于名称的临时 UUID
    // 实际服务器应从 Mojang API 获取真实 UUID
    // 这里使用简单的哈希算法生成伪 UUID
    std::hash<std::string> hasher;
    size_t hash = hasher(name);

    // 格式化为 UUID 格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    std::ostringstream ss;
    ss << std::hex;

    // 第一段 (8 字符)
    ss << ((hash >> 0) & 0xFFFFFFFF);
    ss << "-";
    // 第二段 (4 字符)
    ss << ((hash >> 32) & 0xFFFF);
    ss << "-";
    // 第三段 (4 字符)
    ss << ((hash >> 48) & 0xFFFF);
    ss << "-";
    // 第四段 (4 字符)
    ss << ((hasher(name + "salt1") >> 0) & 0xFFFF);
    ss << "-";
    // 第五段 (12 字符)
    ss << ((hasher(name + "salt2") >> 0) & 0xFFFFFFFFFFFF);

    return ss.str();
}

} // namespace command
} // namespace mc
