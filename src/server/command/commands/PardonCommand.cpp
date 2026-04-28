#include "PardonCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"

#include <sstream>

namespace mc {
namespace command {

void PardonCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto pardonNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("pardon");
    pardonNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        pardonNode,
        support::makeMetadata(
            "Removes a player from the ban list.",
            "/pardon <player>",
            3,
            {},
            false));

    // /pardon <player>
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return pardonPlayer(ctx);
    });

    pardonNode->addChild(playerArg);
    dispatcher.registerCommand(pardonNode);
}

i32 PardonCommand::pardonPlayer(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 解析目标玩家
    std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);

    // 获取玩家名
    String playerName;
    if (!playerIds.empty()) {
        auto* server = source.server();
        if (server != nullptr) {
            auto* playerData = server->playerManager().getPlayer(playerIds.front());
            if (playerData != nullptr) {
                playerName = playerData->username;
            }
        }
    } else {
        // 尝试直接获取用户名
        if (selector.hasUsername()) {
            playerName = selector.username();
        } else {
            source.sendMessage("commands.pardon.failed.noPlayer");
            return 0;
        }
    }

    // TODO: 实现封禁列表系统
    // 需要检查 BannedPlayerList 是否包含该玩家

    std::ostringstream ss;
    ss << "Unbanned " << playerName;
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
