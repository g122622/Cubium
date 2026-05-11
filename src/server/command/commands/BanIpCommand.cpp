#include "BanIpCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/BannedIpList.hpp"
#include "server/core/ConnectionManager.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/entity/entities/player/Player.hpp"

#include <sstream>
#include <iomanip>
#include <ctime>
#include <regex>

namespace mc {
namespace command {

namespace {

/**
 * @brief 获取当前时间的格式化字符串
 * @return 格式化的时间字符串 (yyyy-MM-dd HH:mm:ss Z)
 */
std::string getCurrentTimeString() {
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
 * @brief 检查字符串是否为有效的 IPv4 地址
 * @param str 字符串
 * @return true 如果是有效的 IPv4 地址
 */
bool isValidIpv4(const std::string& str) {
    // IPv4 正则表达式
    static const std::regex ipv4Regex(
        R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)"
    );
    return std::regex_match(str, ipv4Regex);
}

} // anonymous namespace

void BanIpCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto banIpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("ban-ip");
    banIpNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        banIpNode,
        support::makeMetadata(
            "Bans an IP address from the server.",
            "/ban-ip <target> [reason]",
            3,
            {},
            false));

    // /ban-ip <target>
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "target",
        StringArgumentType::string()
    );
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return banIp(ctx);
    });

    // /ban-ip <target> <reason>
    auto reasonArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "reason",
        StringArgumentType::greedyString()
    );
    reasonArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return banIp(ctx);
    });

    targetArg->addChild(reasonArg);
    banIpNode->addChild(targetArg);
    dispatcher.registerCommand(banIpNode);
}

i32 BanIpCommand::banIp(CommandContext<ServerCommandSource>& context) {
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
    } else {
        // 目标是玩家名，需要查找其 IP 地址
        EntitySelector selector;
        // 尝试从名称解析玩家
        auto playerIds = support::resolvePlayerIds(source, EntitySelector::byUsername(target));

        if (playerIds.empty()) {
            // 玩家不在线，无法获取 IP 地址
            std::ostringstream ss;
            ss << "commands.banip.failed.invalidIp";
            source.sendError(ss.str());
            return 0;
        }

        // 获取玩家的 IP 地址
        auto* playerData = server->playerManager().getPlayer(playerIds.front());
        if (playerData == nullptr || !playerData->hasConnection()) {
            source.sendError("commands.banip.failed.playerNotFound");
            return 0;
        }

        // 从连接获取 IP 地址（这里简化处理，实际需要从连接获取）
        // 暂时使用 placeholder，实际实现需要从 TcpSession 获取
        // TODO: 需要在 ServerPlayerData 或 Connection 中存储 IP 地址
        ipAddress = "127.0.0.1"; // placeholder
        playersToKick = playerIds;

        spdlog::warn("BanIpCommand: IP address retrieval from connection not fully implemented");
    }

    auto& banList = server->bannedIpList();

    // 检查是否已被封禁
    if (banList.isBanned(ipAddress)) {
        source.sendError("commands.banip.failed.ipAlreadyBanned");
        return 0;
    }

    // 创建封禁条目
    server::core::BannedIpEntry entry(
        ipAddress,
        getCurrentTimeString(),
        bannedBy,
        "forever",  // 永久封禁
        reason
    );

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
    if (isIp) {
        // 需要查找该 IP 的所有玩家
        // TODO: 需要在 PlayerManager 中实现按 IP 查找玩家的功能
        spdlog::warn("BanIpCommand: Kicking all players from IP {} not fully implemented", ipAddress);
    } else {
        // 踢出已知玩家
        for (PlayerId playerId : playersToKick) {
            std::ostringstream kickReason;
            kickReason << "Your IP has been banned!\nReason: " << reason;
            server->connectionManager().disconnectPlayer(playerId, kickReason.str());
            spdlog::info("Player kicked due to IP ban (playerId={})", playerId);
        }
    }

    return 1;
}

} // namespace command
} // namespace mc
