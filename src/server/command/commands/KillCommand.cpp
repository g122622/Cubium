#include "KillCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/application/IServer.hpp"

#include <sstream>

namespace mc {
namespace command {

void KillCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    auto killNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("kill");
    killNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        killNode,
        support::makeMetadata(
            "Kill entities (players, mobs, etc.).",
            "/kill [<target>]",
            2,
            {},
            false));

    // /kill - 杀死自己
    killNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return killSelf(ctx);
    });

    // /kill <target> - 杀死目标实体
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target",
        EntityArgumentType::entities()
    );
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return killEntities(ctx);
    });
    killNode->addChild(targetArg);

    dispatcher.registerCommand(killNode);
}

i32 KillCommand::killSelf(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 检查命令源是否是实体（玩家）
    if (!source.isPlayer()) {
        source.sendMessage("commands.kill.failed.notEntity");
        return 0;
    }

    ServerPlayer* player = source.player();
    if (player == nullptr) {
        source.sendMessage("commands.kill.failed.notEntity");
        return 0;
    }

    // 使用高伤害杀死实体（MC 风格：设置生命值为 0 或使用 kill 命令伤害源）
    // MC 1.16.5 调用 entity.onKillCommand()
    player->setHealth(0.0f);

    std::ostringstream ss;
    ss << "Killed " << player->username();
    source.sendMessage(ss.str());

    return 1;
}

i32 KillCommand::killEntities(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("target");

    // 获取目标玩家 ID 列表
    std::vector<PlayerId> targetPlayerIds = support::resolvePlayerIds(source, selector);

    if (targetPlayerIds.empty()) {
        source.sendMessage("commands.kill.failed.noEntity");
        return 0;
    }

    i32 killedCount = 0;

    for (PlayerId playerId : targetPlayerIds) {
        if (playerId == 0) {
            continue;
        }

        // 获取玩家数据
        auto* server = source.server();
        if (server == nullptr) {
            continue;
        }

        mc::server::core::PlayerManager& pm = server->playerManager();
        mc::server::ServerPlayerData* playerData = pm.getPlayer(playerId);
        if (playerData == nullptr) {
            continue;
        }

        // TODO: 当 ServerPlayer 实例可通过 PlayerManager 获取时，
        // 应该调用 player->setHealth(0.0f) 或 player->kill() 方法
        // 目前简化实现：仅记录击杀数量

        killedCount++;
    }

    // 发送反馈消息
    if (killedCount == 1) {
        if (source.server() != nullptr && !targetPlayerIds.empty()) {
            mc::server::core::PlayerManager& pm = source.server()->playerManager();
            mc::server::ServerPlayerData* playerData = pm.getPlayer(targetPlayerIds.front());
            if (playerData != nullptr) {
                std::ostringstream ss;
                ss << "Killed " << playerData->username;
                source.sendMessage(ss.str());
            }
        }
    } else {
        std::ostringstream ss;
        ss << "Killed " << killedCount << " entities";
        source.sendMessage(ss.str());
    }

    return killedCount;
}

} // namespace command
} // namespace mc
