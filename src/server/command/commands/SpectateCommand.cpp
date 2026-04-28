#include "SpectateCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void SpectateCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto spectateNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spectate");
    spectateNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        spectateNode,
        support::makeMetadata(
            "Makes a player in spectator mode spectate another entity.",
            "/spectate <target> [player]",
            2,
            {},
            true));

    // /spectate <target> [player]
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target",
        EntityArgumentType::entity());
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player());
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return startSpectating(ctx);
    });
    targetArg->addChild(playerArg);
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return startSpectating(ctx);
    });
    spectateNode->addChild(targetArg);

    // /spectate stop [player]
    auto stopNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("stop");
    auto stopPlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player());
    stopPlayerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return stopSpectating(ctx);
    });
    stopNode->addChild(stopPlayerArg);
    stopNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return stopSpectating(ctx);
    });
    spectateNode->addChild(stopNode);

    dispatcher.registerCommand(spectateNode);
}

i32 SpectateCommand::startSpectating(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& targetSelector = context.getArgument<EntitySelector>("target");

    // 解析目标实体
    auto targetIds = support::resolvePlayerIds(source, targetSelector);
    if (targetIds.empty()) {
        source.sendError("No entity matched the target selector");
        return 0;
    }

    // 获取玩家（命令发送者或指定玩家）
    PlayerId spectatorId;
    if (context.hasArgument("player")) {
        const EntitySelector& playerSelector = context.getArgument<EntitySelector>("player");
        auto playerIds = support::resolvePlayerIds(source, playerSelector);
        if (playerIds.empty()) {
            source.sendError("No player matched the player selector");
            return 0;
        }
        spectatorId = playerIds[0];
    } else {
        if (!source.isPlayer()) {
            source.sendError("Only players can use this command");
            return 0;
        }
        spectatorId = source.playerId();
    }

    auto spectator = source.server()->playerManager().getPlayer(spectatorId);
    auto target = source.server()->playerManager().getPlayer(targetIds[0]);

    if (!spectator) {
        source.sendError("Spectator not found");
        return 0;
    }

    // 检查旁观者模式
    // TODO: 实现游戏模式检查
    // if (spectator->gameMode() != GameMode::Spectator) {
    //     source.sendError("Player must be in spectator mode");
    //     return 0;
    // }

    std::ostringstream ss;
    ss << spectator->username << " is now spectating ";
    if (target) {
        ss << target->username;
    } else {
        ss << "entity " << targetIds[0];
    }
    source.sendMessage(ss.str());

    // TODO: 实现旁观者跟踪系统
    // 1. 设置玩家的旁观目标
    // 2. 同步给客户端

    return 1;
}

i32 SpectateCommand::stopSpectating(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 获取玩家（命令发送者或指定玩家）
    PlayerId spectatorId;
    if (context.hasArgument("player")) {
        const EntitySelector& playerSelector = context.getArgument<EntitySelector>("player");
        auto playerIds = support::resolvePlayerIds(source, playerSelector);
        if (playerIds.empty()) {
            source.sendError("No player matched the player selector");
            return 0;
        }
        spectatorId = playerIds[0];
    } else {
        if (!source.isPlayer()) {
            source.sendError("Only players can use this command");
            return 0;
        }
        spectatorId = source.playerId();
    }

    auto spectator = source.server()->playerManager().getPlayer(spectatorId);
    if (!spectator) {
        source.sendError("Player not found");
        return 0;
    }

    std::ostringstream ss;
    ss << spectator->username << " is no longer spectating";
    source.sendMessage(ss.str());

    // TODO: 实现旁观者跟踪系统
    // 1. 清除玩家的旁观目标
    // 2. 同步给客户端

    return 1;
}

} // namespace command
} // namespace mc
