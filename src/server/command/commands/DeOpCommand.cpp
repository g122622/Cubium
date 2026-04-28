#include "DeOpCommand.hpp"

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

void DeOpCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto deopNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("deop");
    deopNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        deopNode,
        support::makeMetadata(
            "Revokes operator status from a player.",
            "/deop <player>",
            3,
            {},
            false));

    // /deop <player>
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return deopPlayer(ctx);
    });

    deopNode->addChild(playerArg);
    dispatcher.registerCommand(deopNode);
}

i32 DeOpCommand::deopPlayer(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 解析目标玩家
    std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendMessage("commands.deop.failed.noPlayer");
        return 0;
    }

    // 只操作一个玩家
    PlayerId targetId = playerIds.front();

    // 获取玩家数据
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("commands.deop.failed.noServer");
        return 0;
    }

    auto* playerData = server->playerManager().getPlayer(targetId);
    if (playerData == nullptr) {
        source.sendMessage("commands.deop.failed.playerNotFound");
        return 0;
    }

    // TODO: 实现权限管理系统
    // 当前简化实现：仅发送消息提示

    std::ostringstream ss;
    ss << "Made " << playerData->username << " no longer a server operator";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
