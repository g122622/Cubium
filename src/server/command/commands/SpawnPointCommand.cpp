#include "SpawnPointCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void SpawnPointCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto spawnPointNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spawnpoint");
    spawnPointNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        spawnPointNode,
        support::makeMetadata(
            "Sets the spawn point for a player.",
            "/spawnpoint [<player>] [<pos>]",
            2,
            {},
            true));

    // /spawnpoint - 设置自己的重生点到当前位置
    spawnPointNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setSelfSpawnPoint(ctx);
    });

    // /spawnpoint <player>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );
    playerNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setPlayerSpawnPoint(ctx);
    });

    // /spawnpoint <player> <pos>
    auto posNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3()
    );
    posNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setPlayerSpawnPointAtPosition(ctx);
    });

    playerNode->addChild(posNode);
    spawnPointNode->addChild(playerNode);
    dispatcher.registerCommand(spawnPointNode);
}

i32 SpawnPointCommand::setSelfSpawnPoint(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    if (!source.isPlayer()) {
        source.sendMessage("You must be a player to use this command");
        return 0;
    }

    const Vector3d& pos = source.position();
    BlockPos spawnPos(
        static_cast<BlockCoord>(pos.x),
        static_cast<BlockCoord>(pos.y),
        static_cast<BlockCoord>(pos.z)
    );

    // TODO: 需要扩展 ServerPlayerData 或 PlayerManager 来存储玩家重生点

    std::ostringstream ss;
    ss << "Set spawn point to "
       << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z;
    source.sendMessage(ss.str());

    return 1;
}

i32 SpawnPointCommand::setPlayerSpawnPoint(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();

    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendMessage("No matching players were found");
        return 0;
    }

    auto& playerManager = server->playerManager();
    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        auto* playerData = playerManager.getPlayer(playerId);
        if (!playerData) {
            continue;
        }

        // 设置重生点到玩家当前位置
        BlockPos spawnPos(
            static_cast<BlockCoord>(playerData->x),
            static_cast<BlockCoord>(playerData->y),
            static_cast<BlockCoord>(playerData->z)
        );

        // TODO: 需要存储重生点
        successCount++;
    }

    return successCount;
}

i32 SpawnPointCommand::setPlayerSpawnPointAtPosition(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();

    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendMessage("No matching players were found");
        return 0;
    }

    auto& pos = context.getArgument<Vector3d>("pos");
    BlockPos spawnPos(
        static_cast<BlockCoord>(pos.x),
        static_cast<BlockCoord>(pos.y),
        static_cast<BlockCoord>(pos.z)
    );

    // TODO: 需要存储重生点

    std::ostringstream ss;
    ss << "Set spawn point to "
       << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z;
    source.sendMessage(ss.str());

    return static_cast<i32>(playerIds.size());
}

} // namespace command
} // namespace mc
