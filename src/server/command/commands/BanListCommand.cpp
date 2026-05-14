#include "BanListCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/BannedIpList.hpp"
#include "server/core/BannedPlayerList.hpp"

#include <algorithm>
#include <sstream>

namespace mc {
namespace command {

void BanListCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto banlistNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("banlist");
    banlistNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(3); });
    support::applyMetadata(
        banlistNode, support::makeMetadata("Shows the server ban list.", "/banlist [players|ips]", 3, {}, false));

    // /banlist (无参数，显示所有)
    banlistNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return listAll(ctx); });

    // /banlist players
    auto playersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("players");
    playersNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return listPlayers(ctx); });

    // /banlist ips
    auto ipsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("ips");
    ipsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return listIps(ctx); });

    banlistNode->addChild(playersNode);
    banlistNode->addChild(ipsNode);
    dispatcher.registerCommand(banlistNode);
}

i32 BanListCommand::listAll(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.banlist.failed.noServer");
        return 0;
    }

    auto& playerBanList = server->bannedPlayerList();
    auto& ipBanList = server->bannedIpList();

    auto bannedPlayers = playerBanList.getAllBannedNames();
    auto bannedIps = ipBanList.getAllBannedIps();

    size_t total = bannedPlayers.size() + bannedIps.size();

    if (total == 0) {
        source.sendMessage("There are no banned players or IPs");
        return 0;
    }

    std::ostringstream ss;
    ss << "There are " << total << " total ban(s): ";

    // 列出封禁的玩家
    if (!bannedPlayers.empty()) {
        std::sort(bannedPlayers.begin(), bannedPlayers.end());
        ss << "Players [";
        for (size_t i = 0; i < bannedPlayers.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << bannedPlayers[i];
        }
        ss << "]";
    }

    // 列出封禁的 IP
    if (!bannedIps.empty()) {
        if (!bannedPlayers.empty()) ss << ", ";
        std::sort(bannedIps.begin(), bannedIps.end());
        ss << "IPs [";
        for (size_t i = 0; i < bannedIps.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << bannedIps[i];
        }
        ss << "]";
    }

    source.sendMessage(ss.str());
    return static_cast<i32>(total);
}

i32 BanListCommand::listPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.banlist.failed.noServer");
        return 0;
    }

    auto& banList = server->bannedPlayerList();
    auto entries = banList.getAllEntries();

    if (entries.empty()) {
        source.sendMessage("There are no banned players");
        return 0;
    }

    // 按名称排序
    std::sort(entries.begin(),
        entries.end(),
        [](const server::core::BannedPlayerEntry& a, const server::core::BannedPlayerEntry& b) {
            return a.name < b.name;
        });

    std::ostringstream ss;
    ss << "There are " << entries.size() << " banned player(s): ";

    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << entries[i].name;
    }

    source.sendMessage(ss.str());
    return static_cast<i32>(entries.size());
}

i32 BanListCommand::listIps(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.banlist.failed.noServer");
        return 0;
    }

    auto& banList = server->bannedIpList();
    auto entries = banList.getAllEntries();

    if (entries.empty()) {
        source.sendMessage("There are no banned IPs");
        return 0;
    }

    // 按 IP 排序
    std::sort(entries.begin(),
        entries.end(),
        [](const server::core::BannedIpEntry& a, const server::core::BannedIpEntry& b) { return a.ip < b.ip; });

    std::ostringstream ss;
    ss << "There are " << entries.size() << " banned IP(s): ";

    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << entries[i].ip;
    }

    source.sendMessage(ss.str());
    return static_cast<i32>(entries.size());
}

} // namespace command
} // namespace mc
