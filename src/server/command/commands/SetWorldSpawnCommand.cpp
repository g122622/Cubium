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

#include "SetWorldSpawnCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/network/packet/SpawnPositionPacket.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <sstream>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

void SetWorldSpawnCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto setWorldSpawnNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("setworldspawn");
    setWorldSpawnNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(
        setWorldSpawnNode, support::makeMetadata("Sets the world spawn point.", "/setworldspawn [<pos>]", 2, {}, true));

    // /setworldspawn - 设置当前位置为世界出生点
    setWorldSpawnNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return setCurrentPosition(ctx); });

    // /setworldspawn <pos>
    auto posNode =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>("pos", Vec3ArgumentType::vec3());
    posNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return setPosition(ctx); });

    setWorldSpawnNode->addChild(posNode);
    dispatcher.registerCommand(setWorldSpawnNode);
}

i32 SetWorldSpawnCommand::setCurrentPosition(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    if (!source.isPlayer()) {
        source.sendError("You must be a player to use this command");
        return 0;
    }

    auto* server = source.server();
    DimensionId dimensionId = DimensionId(0); // 默认主世界
    auto* dimension = server->dimensionManager().getDimension(dimensionId);

    if (!dimension) {
        source.sendError("Dimension not found");
        return 0;
    }

    const Vector3d& pos = source.position();
    dimension->setSpawnPoint(pos);

    // 同步更新 ServerWorld 的世界出生点
    server->world().setWorldSpawnPoint(pos);

    // 广播新的出生点到所有玩家
    broadcastSpawnPosition(server, pos);

    std::ostringstream ss;
    ss << "Set world spawn point to " << static_cast<BlockCoord>(pos.x) << ", " << static_cast<BlockCoord>(pos.y)
       << ", " << static_cast<BlockCoord>(pos.z);
    source.sendMessage(ss.str());

    return 1;
}

i32 SetWorldSpawnCommand::setPosition(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();

    const auto& pos = context.getArgument<Vector3d>("pos");

    DimensionId dimensionId = DimensionId(0); // 默认主世界
    auto* dimension = server->dimensionManager().getDimension(dimensionId);

    if (!dimension) {
        source.sendError("Dimension not found");
        return 0;
    }

    dimension->setSpawnPoint(pos);

    // 同步更新 ServerWorld 的世界出生点
    server->world().setWorldSpawnPoint(pos);

    // 广播新的出生点到所有玩家
    broadcastSpawnPosition(server, pos);

    std::ostringstream ss;
    ss << "Set world spawn point to " << static_cast<BlockCoord>(pos.x) << ", " << static_cast<BlockCoord>(pos.y)
       << ", " << static_cast<BlockCoord>(pos.z);
    source.sendMessage(ss.str());

    return 1;
}

void SetWorldSpawnCommand::broadcastSpawnPosition(server::IServer* server, const Vector3d& pos)
{
    // 创建出生点数据包
    network::SpawnPositionPacket spawnPosPacket(
        BlockPos(static_cast<BlockCoord>(pos.x), static_cast<BlockCoord>(pos.y), static_cast<BlockCoord>(pos.z)));

    auto spawnPosResult = spawnPosPacket.serialize();
    if (spawnPosResult.failed()) {
        spdlog::warn("SetWorldSpawnCommand: Failed to serialize SpawnPositionPacket");
        return;
    }

    // 通过 ConnectionManager 广播给所有玩家
    server->connectionManager().broadcastPacket(network::PacketType::SpawnPosition, spawnPosResult.value());
}

} // namespace command
} // namespace mc
