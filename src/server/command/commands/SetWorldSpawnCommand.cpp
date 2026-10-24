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
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp" // BlockPosArgumentType/RotationArgumentType（对齐 vanilla BlockPosArgument/RotationArgument）
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include "common/world/spawn/RespawnData.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace mc {
namespace command {

void SetWorldSpawnCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto setWorldSpawnNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("setworldspawn");
    setWorldSpawnNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(setWorldSpawnNode,
        support::makeMetadata("Sets the world spawn point.", "/setworldspawn [<pos> [<rotation>]]", 2, {}, true));

    // /setworldspawn - 设置当前位置为世界出生点（BlockPos.containing(source.position())，floor 成整数 BlockPos），
    // 朝向 ZERO_ROTATION（0,0）。对齐 MC 1.21.11: setSpawn(source, BlockPos.containing(getPosition()), ZERO_ROTATION)。
    setWorldSpawnNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setCurrentPosition(ctx);
    });

    // /setworldspawn <pos> - pos 用 BlockPosArgumentType（整数 floor，对齐 vanilla BlockPosArgument），
    // 非 Vec3ArgumentType（centerCorrect 给绝对整数加 0.5 偏移致出生点偏 0.5，已修）。
    auto posNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    posNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setPosition(ctx);
    });

    // /setworldspawn <pos> <rotation> - rotation 用 RotationArgumentType（接 yaw pitch，对齐 vanilla
    // RotationArgument）。yaw/pitch 随 spawn 复合标签一起持久化。
    // 完整建模后补）。
    auto rotationNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "rotation", RotationArgumentType::rotation());
    rotationNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setPositionWithRotation(ctx);
    });

    posNode->addChild(rotationNode);
    setWorldSpawnNode->addChild(posNode);
    dispatcher.registerCommand(setWorldSpawnNode);
}

i32 SetWorldSpawnCommand::_setCurrentPosition(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    if (!source.isPlayer()) {
        source.sendError("You must be a player to use this command");
        return 0;
    }
    // 无参：位置取命令源所在方块，朝向取全零（不使用玩家朝向）。
    return _applyWorldSpawn(context, source.position(), Vector2f(0.0f, 0.0f));
}

i32 SetWorldSpawnCommand::_setPosition(CommandContext<ServerCommandSource>& context)
{
    // <pos> 不带 rotation 时 rotation 为全零。
    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", context.getSource());
    return _applyWorldSpawn(context, pos, Vector2f(0.0f, 0.0f));
}

i32 SetWorldSpawnCommand::_setPositionWithRotation(CommandContext<ServerCommandSource>& context)
{
    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", context.getSource());
    const Vector2f rotation = RotationArgumentType::getRotation(context, "rotation", context.getSource());
    return _applyWorldSpawn(context, pos, rotation);
}

i32 SetWorldSpawnCommand::_applyWorldSpawn(
    CommandContext<ServerCommandSource>& context, const Vector3d& position, const Vector2f& rotation)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("Server not available");
        return 0;
    }

    ServerDimension* dimension = server->dimensionManager().getOverworld();
    if (dimension == nullptr || dimension->world() == nullptr) {
        source.sendError("Overworld not available");
        return 0;
    }

    // 出生方块取命令源位置的整数方块坐标（等价 BlockPosArgument.getSpawnablePos 的取整语义）。
    const BlockPos spawnPos(static_cast<BlockCoord>(std::floor(position.x)),
        static_cast<BlockCoord>(std::floor(position.y)),
        static_cast<BlockCoord>(std::floor(position.z)));
    const RespawnData spawnData = RespawnData::of(DimensionManager::OVERWORLD, spawnPos, rotation.x, rotation.y);

    // 整个存档只有一份世界出生数据，保存于主世界 level.dat；重复设置同一位置不重复广播。
    server::ServerWorld& world = *dimension->world();
    if (world.respawnData() == spawnData) {
        return 0;
    }
    world.setRespawnData(spawnData);

    mc::network::ir::play::SetDefaultSpawnPosition pkt;
    pkt.dimension = std::string(dimensionIdToString(spawnData.dimension()));
    pkt.blockPosPacked = spawnData.pos().asLong();
    pkt.yaw = spawnData.yaw;
    pkt.pitch = spawnData.pitch;
    server->connectionManager().broadcast(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pkt)}});

    std::ostringstream ss;
    ss << "Set the world spawn point to " << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << ", "
       << spawnData.yaw << ", " << spawnData.pitch << " in " << dimensionIdToString(spawnData.dimension());
    source.sendMessage(ss.str());
    return 1;
}

} // namespace command
} // namespace mc
