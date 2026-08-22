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
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/ConnectionManager.hpp"
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
    setWorldSpawnNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(setWorldSpawnNode,
        support::makeMetadata("Sets the world spawn point.", "/setworldspawn [<pos> [<rotation>]]", 2, {}, true));

    // /setworldspawn - 设置当前位置为世界出生点（BlockPos.containing(source.position())，floor 成整数 BlockPos），
    // 朝向 ZERO_ROTATION（0,0）。对齐 MC 1.21.11: setSpawn(source, BlockPos.containing(getPosition()), ZERO_ROTATION)。
    setWorldSpawnNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setCurrentPosition(ctx); });

    // /setworldspawn <pos> - pos 用 BlockPosArgumentType（整数 floor，对齐 vanilla BlockPosArgument），
    // 非 Vec3ArgumentType（centerCorrect 给绝对整数加 0.5 偏移致出生点偏 0.5，已修）。
    auto posNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    posNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setPosition(ctx); });

    // /setworldspawn <pos> <rotation> - rotation 用 RotationArgumentType（接 yaw pitch，对齐 vanilla
    // RotationArgument）。 yaw 存 ServerWorld::m_spawnAngle；pitch 暂丢弃（Cubium 出生点 pitch 未建模，TODO
    // 完整建模后补）。
    auto rotationNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "rotation", RotationArgumentType::rotation());
    rotationNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setPositionWithRotation(ctx); });

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

    auto* server = source.server();
    DimensionId dimensionId = source.dimensionId();
    auto* dimension = server->dimensionManager().getDimension(dimensionId);

    if (!dimension) {
        source.sendError("Dimension not found");
        return 0;
    }

    // 对齐 MC 1.21.11: BlockPos.containing(source.getPosition()) — 玩家位置 floor 成整数 BlockPos。
    const Vector3d& playerPos = source.position();
    Vector3d pos(static_cast<f64>(static_cast<BlockCoord>(std::floor(playerPos.x))),
        static_cast<f64>(static_cast<BlockCoord>(std::floor(playerPos.y))),
        static_cast<f64>(static_cast<BlockCoord>(std::floor(playerPos.z))));

    // 对齐 MC 1.21.11: 无参 rotation = WorldCoordinates.ZERO_ROTATION（yaw=0, pitch=0），不用玩家朝向。
    f32 angle = 0.0f;
    dimension->setSpawnPoint(pos);

    // 同步更新 ServerWorld 的世界出生点和朝向
    if (dimension->world()) {
        dimension->world()->setWorldSpawnPoint(pos, angle);
    }

    // 广播新的出生点到所有玩家
    _broadcastSpawnPosition(server, pos, angle, dimensionId);

    std::ostringstream ss;
    ss << "Set world spawn point to " << static_cast<BlockCoord>(pos.x) << ", " << static_cast<BlockCoord>(pos.y)
       << ", " << static_cast<BlockCoord>(pos.z) << " (angle: " << angle << ")";
    source.sendMessage(ss.str());

    return 1;
}

i32 SetWorldSpawnCommand::_setPosition(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();

    // 对齐 MC 1.21.11: BlockPosArgument.getSpawnablePos — 取整成 BlockPos（整数），无 centerCorrect 偏移。
    const Vector3i blockPos = BlockPosArgumentType::getBlockPos(context, "pos", source);
    Vector3d pos(static_cast<f64>(blockPos.x), static_cast<f64>(blockPos.y), static_cast<f64>(blockPos.z));

    DimensionId dimensionId = source.dimensionId();
    auto* dimension = server->dimensionManager().getDimension(dimensionId);

    if (!dimension) {
        source.sendError("Dimension not found");
        return 0;
    }

    dimension->setSpawnPoint(pos);

    // 对齐 MC 1.21.11: <pos> 不带 rotation 时 rotation = ZERO_ROTATION（yaw=0, pitch=0）。
    f32 angle = 0.0f;

    // 同步更新 ServerWorld 的世界出生点
    if (dimension->world()) {
        dimension->world()->setWorldSpawnPoint(pos, angle);
    }

    // 广播新的出生点到所有玩家
    _broadcastSpawnPosition(server, pos, angle, dimensionId);

    std::ostringstream ss;
    ss << "Set world spawn point to " << static_cast<BlockCoord>(pos.x) << ", " << static_cast<BlockCoord>(pos.y)
       << ", " << static_cast<BlockCoord>(pos.z);
    source.sendMessage(ss.str());

    return 1;
}

i32 SetWorldSpawnCommand::_setPositionWithRotation(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();

    // 对齐 MC 1.21.11: BlockPosArgument.getSpawnablePos — 取整成 BlockPos（整数）。
    const Vector3i blockPos = BlockPosArgumentType::getBlockPos(context, "pos", source);
    Vector3d pos(static_cast<f64>(blockPos.x), static_cast<f64>(blockPos.y), static_cast<f64>(blockPos.z));

    // 对齐 MC 1.21.11: RotationArgument.getRotation — 返回 (yaw, pitch)。
    // RotationArgumentType::getRotation 返回 Vector2f(yaw, pitch)（x=yaw, y=pitch，绝对分量取原值）。
    const Vector2f rotation = RotationArgumentType::getRotation(context, "rotation", source);
    f32 angle = rotation.x; // yaw
    // pitch = rotation.y — Cubium 出生点 pitch 未建模（仅 yaw 持久化到 level.dat SpawnAngle，对齐 Java level.dat）。
    // TODO: 完整对齐 vanilla LevelData.RespawnData(yaw, pitch) 需在 ServerWorld/Dimension/LevelDatCodec 建模 pitch，
    //       届时将 rotation.y(pitch) 存入并随 SetDefaultSpawnPosition 网络包下发，玩家重生朝向用 pitch。
    f32 pitch = rotation.y;
    (void)pitch; // 暂不使用，保留语义占位

    // 对齐 MC 1.21.11: 将 yaw 归一化到 [-180, 180]（vanilla 在 RespawnData 构造时未显式 wrapDegrees，
    // 但玩家重生应用 angle 时由 Entity 处理；Cubium 此处显式 wrap 保证存储规范化）。
    angle = math::wrapDegrees(angle);

    DimensionId dimensionId = source.dimensionId();
    auto* dimension = server->dimensionManager().getDimension(dimensionId);

    if (!dimension) {
        source.sendError("Dimension not found");
        return 0;
    }

    dimension->setSpawnPoint(pos);

    // 同步更新 ServerWorld 的世界出生点和朝向（yaw）
    if (dimension->world()) {
        dimension->world()->setWorldSpawnPoint(pos, angle);
    }

    // 广播新的出生点到所有玩家
    _broadcastSpawnPosition(server, pos, angle, dimensionId);

    std::ostringstream ss;
    ss << "Set world spawn point to " << static_cast<BlockCoord>(pos.x) << ", " << static_cast<BlockCoord>(pos.y)
       << ", " << static_cast<BlockCoord>(pos.z) << " (angle: " << angle << ")";
    source.sendMessage(ss.str());

    return 1;
}

void SetWorldSpawnCommand::_broadcastSpawnPosition(
    server::IServer* server, const Vector3d& pos, f32 angle, DimensionId dimensionId)
{
    // 1.21.11 SetDefaultSpawnPosition：dimension(ResourceKey) + blockPosPacked + yaw + pitch
    // dimension 取命令执行者所在维度，命令作用于该维度出生点。
    std::string dimensionKey = "minecraft:overworld";
    switch (dimensionId) {
        case -1:
            dimensionKey = "minecraft:the_nether";
            break;
        case 1:
            dimensionKey = "minecraft:the_end";
            break;
        case 0:
        default:
            dimensionKey = "minecraft:overworld";
            break;
    }

    mc::network::ir::play::SetDefaultSpawnPosition pkt;
    pkt.dimension = dimensionKey;
    pkt.blockPosPacked =
        BlockPos(static_cast<BlockCoord>(pos.x), static_cast<BlockCoord>(pos.y), static_cast<BlockCoord>(pos.z))
            .asLong();
    pkt.yaw = angle;
    pkt.pitch = 0.0f;

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };

    // 通过 ConnectionManager 广播给所有玩家
    server->connectionManager().broadcast(packet);
}

} // namespace command
} // namespace mc
