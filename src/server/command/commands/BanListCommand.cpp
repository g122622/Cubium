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

#include "BanListCommand.hpp"
#include "server/access/AccessManager.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "server/access/base/BannedIpListTypes.hpp"
#include "server/access/base/BannedPlayerListTypes.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>

namespace mc {
namespace command {

void BanListCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto banlistNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("banlist");
    banlistNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        banlistNode, support::makeMetadata("Shows the server ban list.", "/banlist [players|ips]", 3, {}, false));

    // /banlist (无参数，显示所有)
    banlistNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return listAll(ctx);
    });

    // /banlist players
    auto playersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("players");
    playersNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return listPlayers(ctx);
    });

    // /banlist ips
    auto ipsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("ips");
    ipsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return listIps(ctx);
    });

    banlistNode->addChild(playersNode);
    banlistNode->addChild(ipsNode);
    dispatcher.registerCommand(banlistNode);
}

i32 BanListCommand::listAll(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendFailure(::mc::text::literal("Server not available"));
        return 0;
    }

    auto& playerBanList = server->accessManager();
    auto& ipBanList = server->accessManager();

    auto bannedPlayers = playerBanList.playerBanGetAllBannedNames();
    auto bannedIps = ipBanList.ipBanGetAllBannedIps();

    size_t total = bannedPlayers.size() + bannedIps.size();

    if (total == 0) {
        source.sendSuccess(::mc::text::literal("There are no banned players or IPs"), false);
        return 0;
    }

    std::ostringstream ss;
    ss << "There are " << total << " total ban(s): ";

    // 列出封禁的玩家
    if (!bannedPlayers.empty()) {
        std::sort(bannedPlayers.begin(), bannedPlayers.end());
        ss << "Players [";
        for (size_t i = 0; i < bannedPlayers.size(); ++i) {
            if (i > 0) {
                ss << ", ";
            }
            ss << bannedPlayers[i];
        }
        ss << "]";
    }

    // 列出封禁的 IP
    if (!bannedIps.empty()) {
        if (!bannedPlayers.empty()) {
            ss << ", ";
        }
        std::sort(bannedIps.begin(), bannedIps.end());
        ss << "IPs [";
        for (size_t i = 0; i < bannedIps.size(); ++i) {
            if (i > 0) {
                ss << ", ";
            }
            ss << bannedIps[i];
        }
        ss << "]";
    }

    source.sendSuccess(::mc::text::literal(ss.str()), false);
    return static_cast<i32>(total);
}

i32 BanListCommand::listPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendFailure(::mc::text::literal("Server not available"));
        return 0;
    }

    auto& banList = server->accessManager();
    auto entries = banList.playerBanGetAllEntries();

    if (entries.empty()) {
        source.sendSuccess(::mc::text::literal("There are no banned players"), false);
        return 0;
    }

    // 按名称排序
    std::sort(
        entries.begin(), entries.end(), [](const server::BannedPlayerEntry& a, const server::BannedPlayerEntry& b) {
            return a.name < b.name;
        });

    std::ostringstream ss;
    ss << "There are " << entries.size() << " banned player(s): ";

    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) {
            ss << ", ";
        }
        ss << entries[i].name;
    }

    source.sendSuccess(::mc::text::literal(ss.str()), false);
    return static_cast<i32>(entries.size());
}

i32 BanListCommand::listIps(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendFailure(::mc::text::literal("Server not available"));
        return 0;
    }

    auto& banList = server->accessManager();
    auto entries = banList.ipBanGetAllEntries();

    if (entries.empty()) {
        source.sendSuccess(::mc::text::literal("There are no banned IPs"), false);
        return 0;
    }

    // 按 IP 排序
    std::sort(entries.begin(), entries.end(), [](const server::BannedIpEntry& a, const server::BannedIpEntry& b) {
        return a.ip < b.ip;
    });

    std::ostringstream ss;
    ss << "There are " << entries.size() << " banned IP(s): ";

    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) {
            ss << ", ";
        }
        ss << entries[i].ip;
    }

    source.sendSuccess(::mc::text::literal(ss.str()), false);
    return static_cast<i32>(entries.size());
}

} // namespace command
} // namespace mc
