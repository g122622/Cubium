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
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include "common/world/spawn/RespawnConfig.hpp"
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
#include <utility>
#include <vector>

namespace mc {
namespace command {

namespace {

/**
 * @brief 把"命令源位置 + 旋转"应用到目标玩家列表
 *
 * 位置取整为方块坐标，yaw 归一到 [-180, 180]，pitch 夹紧到 [-90, 90]；配置恒为 forced
 * （强制重生点不要求目标是床或重生锚）。维度取命令源所在维度。
 */
i32 _applySpawnPoint(CommandContext<ServerCommandSource>& context,
    const std::vector<PlayerId>& playerIds,
    const Vector3d& position,
    const Vector2f& rotation)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("Server not available");
        return 0;
    }

    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("No world available");
        return 0;
    }

    const BlockPos spawnPos(static_cast<BlockCoord>(std::floor(position.x)),
        static_cast<BlockCoord>(std::floor(position.y)),
        static_cast<BlockCoord>(std::floor(position.z)));
    const DimensionId dimensionId = world->dimension();
    const RespawnConfig config(RespawnData::of(dimensionId, spawnPos, rotation.x, rotation.y), true);

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        Player* player = server->playerEntityManager().getPlayerEntity(playerId, *world);
        if (player == nullptr) {
            continue;
        }
        player->setRespawnPosition(config, false);

        std::ostringstream ss;
        ss << "Set spawn point for " << player->username() << " to " << spawnPos.x << ", " << spawnPos.y << ", "
           << spawnPos.z << ", " << config.respawnData.yaw << ", " << config.respawnData.pitch << " in "
           << dimensionIdToString(dimensionId);
        source.sendMessage(ss.str());
        ++successCount;
    }

    return successCount;
}

} // namespace

void SpawnPointCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto spawnPointNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spawnpoint");
    spawnPointNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(spawnPointNode,
        support::makeMetadata(
            "Sets the spawn point for a player.", "/spawnpoint [<targets> [<pos> [<rotation>]]]", 2, {}, true));

    // /spawnpoint - 把自己设置在命令源位置，朝向全零
    spawnPointNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setSelfSpawnPoint(ctx);
    });

    // /spawnpoint <targets>
    auto targetsNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());
    targetsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setPlayerSpawnPoint(ctx);
    });

    // /spawnpoint <targets> <pos>
    auto posNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    posNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setPlayerSpawnPointAtPosition(ctx);
    });

    // /spawnpoint <targets> <pos> <rotation>
    auto rotationNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "rotation", RotationArgumentType::rotation());
    rotationNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setPlayerSpawnPointAtRotation(ctx);
    });

    posNode->addChild(rotationNode);
    targetsNode->addChild(posNode);
    spawnPointNode->addChild(targetsNode);
    dispatcher.registerCommand(spawnPointNode);
}

i32 SpawnPointCommand::_setSelfSpawnPoint(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    Player* player = source.player();
    if (player == nullptr) {
        source.sendError("You must be a player to use this command");
        return 0;
    }

    const BlockPos spawnPos(static_cast<BlockCoord>(std::floor(source.position().x)),
        static_cast<BlockCoord>(std::floor(source.position().y)),
        static_cast<BlockCoord>(std::floor(source.position().z)));
    const RespawnConfig config(RespawnData::of(source.dimensionId(), spawnPos, 0.0f, 0.0f), true);
    player->setRespawnPosition(config, false);

    std::ostringstream ss;
    ss << "Set spawn point to " << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ", 0.0, 0.0 in "
       << dimensionIdToString(source.dimensionId());
    source.sendMessage(ss.str());
    return 1;
}

i32 SpawnPointCommand::_setPlayerSpawnPoint(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("targets");
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 无位置参数：目标位置与朝向都取命令源（不是目标玩家自身）。
    return _applySpawnPoint(context, playerIds, source.position(), Vector2f(0.0f, 0.0f));
}

i32 SpawnPointCommand::_setPlayerSpawnPointAtPosition(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("targets");
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const Vector3d pos = [&] {
        const Vector3i p = BlockPosArgumentType::getBlockPos(context, "pos", source);
        return Vector3d(static_cast<f64>(p.x), static_cast<f64>(p.y), static_cast<f64>(p.z));
    }();
    return _applySpawnPoint(context, playerIds, pos, Vector2f(0.0f, 0.0f));
}

i32 SpawnPointCommand::_setPlayerSpawnPointAtRotation(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const auto& selector = context.getArgument<EntitySelector>("targets");
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const Vector3d pos = [&] {
        const Vector3i p = BlockPosArgumentType::getBlockPos(context, "pos", source);
        return Vector3d(static_cast<f64>(p.x), static_cast<f64>(p.y), static_cast<f64>(p.z));
    }();
    const Vector2f rotation = RotationArgumentType::getRotation(context, "rotation", source);
    return _applySpawnPoint(context, playerIds, pos, rotation);
}

} // namespace command
} // namespace mc
