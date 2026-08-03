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

#include "SpawnPointCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <cmath>
#include <memory>
#include <sstream>

namespace mc {
namespace command {

void SpawnPointCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto spawnPointNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spawnpoint");
    spawnPointNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(spawnPointNode,
        support::makeMetadata("Sets the spawn point for a player.", "/spawnpoint [<player>] [<pos>]", 2, {}, true));

    // /spawnpoint - 设置自己的重生点到当前位置
    spawnPointNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setSelfSpawnPoint(ctx); });

    // /spawnpoint <player>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());
    playerNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setPlayerSpawnPoint(ctx); });

    // /spawnpoint <player> <pos>
    auto posNode =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec3ArgumentType::vec3());
    posNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setPlayerSpawnPointAtPosition(ctx); });

    playerNode->addChild(posNode);
    spawnPointNode->addChild(playerNode);
    dispatcher.registerCommand(spawnPointNode);
}

i32 SpawnPointCommand::_setSelfSpawnPoint(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    ServerPlayer* player = source.player();
    if (player == nullptr) {
        source.sendError("You must be a player to use this command");
        return 0;
    }

    const Vector3d& pos = source.position();
    BlockPos spawnPos(static_cast<BlockCoord>(std::floor(pos.x)),
        static_cast<BlockCoord>(std::floor(pos.y)),
        static_cast<BlockCoord>(std::floor(pos.z)));

    // 获取当前维度，默认为主世界
    DimensionId dimensionId = DimensionManager::OVERWORLD;
    if (source.world() != nullptr) {
        dimensionId = source.world()->dimension();
    }

    // 设置玩家的重生点
    player->setSpawnPoint(dimensionId, spawnPos, false);

    std::ostringstream ss;
    ss << "Set spawn point to " << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z;
    source.sendMessage(ss.str());

    return 1;
}

i32 SpawnPointCommand::_setPlayerSpawnPoint(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("Server not available");
        return 0;
    }

    auto* world = source.world();
    DimensionId dimensionId = DimensionManager::OVERWORLD;
    if (world != nullptr) {
        dimensionId = world->dimension();
    }

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        // 通过 ServerPlayerEntityManager 获取玩家实体
        Player* player = server->playerEntityManager().getPlayerEntity(playerId, *world);
        if (player == nullptr) {
            continue;
        }

        // 使用玩家的当前位置作为重生点
        const Vector3& pos = player->position();
        BlockPos spawnPos(static_cast<BlockCoord>(std::floor(pos.x)),
            static_cast<BlockCoord>(std::floor(pos.y)),
            static_cast<BlockCoord>(std::floor(pos.z)));

        player->setSpawnPoint(dimensionId, spawnPos, false);

        std::ostringstream ss;
        ss << "Set spawn point for " << player->username() << " to " << spawnPos.x << ", " << spawnPos.y << ", "
           << spawnPos.z;
        source.sendMessage(ss.str());
        successCount++;
    }

    return successCount;
}

i32 SpawnPointCommand::_setPlayerSpawnPointAtPosition(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("Server not available");
        return 0;
    }

    auto* world = source.world();
    DimensionId dimensionId = DimensionManager::OVERWORLD;
    if (world != nullptr) {
        dimensionId = world->dimension();
    }

    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", source);
    BlockPos spawnPos(static_cast<BlockCoord>(std::floor(pos.x)),
        static_cast<BlockCoord>(std::floor(pos.y)),
        static_cast<BlockCoord>(std::floor(pos.z)));

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        // 通过 ServerPlayerEntityManager 获取玩家实体
        Player* player = server->playerEntityManager().getPlayerEntity(playerId, *world);
        if (player == nullptr) {
            continue;
        }

        player->setSpawnPoint(dimensionId, spawnPos, false);

        std::ostringstream ss;
        ss << "Set spawn point for " << player->username() << " to " << spawnPos.x << ", " << spawnPos.y << ", "
           << spawnPos.z;
        source.sendMessage(ss.str());
        successCount++;
    }

    return successCount;
}

} // namespace command
} // namespace mc
