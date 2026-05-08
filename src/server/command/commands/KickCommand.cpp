#include "KickCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"

#include <sstream>

namespace mc::command {
namespace {

inline constexpr std::string_view DEFAULT_KICK_REASON = "Kicked by an operator";

} // namespace

void KickCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto kickNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("kick");
    kickNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        kickNode,
        support::makeMetadata(
            "Disconnect players from the server.",
            "/kick <targets> [reason]",
            3,
            {},
            true));

    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets",
        EntityArgumentType::players());
    targetsArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return kickPlayers(ctx);
    });

    auto reasonArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "reason",
        StringArgumentType::greedyString());
    reasonArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return kickPlayers(ctx);
    });

    targetsArg->addChild(reasonArg);
    kickNode->addChild(targetsArg);

    dispatcher.registerCommand(kickNode);
}

/**
 * @brief 踢出目标玩家并断开其连接。
 *
 * @param context 命令上下文。
 * @return 成功踢出的玩家数量。
 */
i32 KickCommand::kickPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    const EntitySelector selector = context.getArgument<EntitySelector>("targets");
    const auto playerIds = support::resolvePlayerIds(source, selector);
    const std::string reason = context.hasArgument("reason")
        ? context.getArgument<std::string>("reason")
        : std::string(DEFAULT_KICK_REASON);

    i32 kickedCount = 0;
    for (const PlayerId playerId : playerIds) {
        if (playerId == 0) {
            continue;
        }

        const auto* playerData = server->playerManager().getPlayer(playerId);
        if (playerData == nullptr) {
            continue;
        }

        server->connectionManager().disconnectPlayer(playerId, reason);
        ++kickedCount;
    }

    if (kickedCount == 0) {
        source.sendError("No matching players were found");
        return 0;
    }

    std::ostringstream ss;
    ss << "Kicked " << kickedCount << " player(s): " << reason;
    source.sendMessage(ss.str());
    return kickedCount;
}

} // namespace mc::command
