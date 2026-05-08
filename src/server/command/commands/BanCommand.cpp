#include "BanCommand.hpp"

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

void BanCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto banNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("ban");
    banNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        banNode,
        support::makeMetadata(
            "Bans a player from the server.",
            "/ban <player> [reason]",
            3,
            {},
            false));

    // /ban <player>
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return banPlayer(ctx);
    });

    // /ban <player> <reason>
    auto reasonArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "reason",
        StringArgumentType::greedyString()
    );
    reasonArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return banPlayer(ctx);
    });

    playerArg->addChild(reasonArg);
    banNode->addChild(playerArg);
    dispatcher.registerCommand(banNode);
}

i32 BanCommand::banPlayer(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 获取可选原因
    std::string reason = "Banned by an operator";
    if (context.hasArgument("reason")) {
        reason = context.getArgument<std::string>("reason");
    }

    // 解析目标玩家
    std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        // 尝试通过用户名查找
        // TODO: 需要从用户名查找玩家（即使不在线）
        source.sendError("commands.ban.failed.noPlayer");
        return 0;
    }

    PlayerId targetId = playerIds.front();

    // 获取玩家数据
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.ban.failed.noServer");
        return 0;
    }

    auto* playerData = server->playerManager().getPlayer(targetId);
    if (playerData == nullptr) {
        source.sendError("commands.ban.failed.playerNotFound");
        return 0;
    }

    // TODO: 实现封禁列表系统
    // 需要：
    // 1. BannedPlayerList 类
    // 2. BannedPlayerEntry 类
    // 3. 保存到 banned-players.json
    // 4. 断开玩家连接

    std::ostringstream ss;
    ss << "Banned " << playerData->username << ": " << reason;
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
