#include "BanCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/TimeUtils.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/BannedPlayerList.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace mc {
namespace command {

namespace {

/**
 * @brief 获取当前时间的格式化字符串
 * @return 格式化的时间字符串 (yyyy-MM-dd HH:mm:ss Z)
 */
std::string getCurrentTimeString()
{
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);

    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &now_time);
#else
    localtime_r(&now_time, &tm);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S %z");
    return ss.str();
}

/**
 * @brief 从用户名生成临时 UUID
 * @param name 用户名
 * @return UUID 字符串
 */
std::string generateUuidFromName(const std::string& name)
{
    std::hash<std::string> hasher;
    size_t hash = hasher(name);

    std::ostringstream ss;
    ss << std::hex;

    ss << ((hash >> 0) & 0xFFFFFFFF);
    ss << "-";
    ss << ((hash >> 32) & 0xFFFF);
    ss << "-";
    ss << ((hash >> 48) & 0xFFFF);
    ss << "-";
    ss << ((hasher(name + "salt1") >> 0) & 0xFFFF);
    ss << "-";
    ss << ((hasher(name + "salt2") >> 0) & 0xFFFFFFFFFFFF);

    return ss.str();
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
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return banPlayer(ctx); });

    // /ban <player> <reason>
    auto reasonArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "reason", StringArgumentType::greedyString());
    reasonArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return banPlayer(ctx); });

    playerArg->addChild(reasonArg);
    banNode->addChild(playerArg);
    dispatcher.registerCommand(banNode);
}

i32 BanCommand::banPlayer(CommandContext<ServerCommandSource>& context)
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
                playerUuid = playerData->uuid;
            }
        }
    }

    // 如果玩家不在线，尝试从选择器获取用户名
    if (playerName.empty() && selector.hasUsername()) {
        playerName = selector.username();
        // 离线玩家使用生成的 UUID
        playerUuid = generateUuidFromName(playerName);
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
