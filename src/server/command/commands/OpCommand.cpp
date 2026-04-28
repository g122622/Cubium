#include "OpCommand.hpp"

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

void OpCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto opNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("op");
    opNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        opNode,
        support::makeMetadata(
            "Grants operator status to a player.",
            "/op <player>",
            3,
            {},
            false));

    // /op <player>
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return opPlayer(ctx);
    });

    opNode->addChild(playerArg);
    dispatcher.registerCommand(opNode);
}

i32 OpCommand::opPlayer(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 解析目标玩家
    std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendMessage("commands.op.failed.noPlayer");
        return 0;
    }

    // 目前只能操作一个玩家
    PlayerId targetId = playerIds.front();

    // 获取玩家数据
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("commands.op.failed.noServer");
        return 0;
    }

    auto* playerData = server->playerManager().getPlayer(targetId);
    if (playerData == nullptr) {
        source.sendMessage("commands.op.failed.playerNotFound");
        return 0;
    }

    // TODO: 实现权限管理系统
    // 当前简化实现：仅发送消息提示
    // 需要：
    // 1. ServerPlayerData 添加 permissionLevel 字段
    // 2. 保存到配置文件或数据库
    // 3. 通知客户端权限变更

    std::ostringstream ss;
    ss << "Made " << playerData->username << " a server operator";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
