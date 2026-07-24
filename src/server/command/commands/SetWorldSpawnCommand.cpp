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
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/block/BlockPos.hpp"
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
    support::applyMetadata(setWorldSpawnNode,
        support::makeMetadata("Sets the world spawn point.", "/setworldspawn [<pos> [<angle>]]", 2, {}, true));

    // /setworldspawn - 设置当前位置为世界出生点（使用玩家的朝向）
    setWorldSpawnNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setCurrentPosition(ctx); });

    // /setworldspawn <pos>
    auto posNode =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec3ArgumentType::vec3());
    posNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setPosition(ctx); });

    // /setworldspawn <pos> <angle> - 设置世界出生点到指定位置和朝向
    // 参考 MC 1.21.11: SetWorldSpawnCommand 支持可选的 rotation 参数
    auto angleNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "angle", FloatArgumentType::floatArg(-180.0f, 180.0f));
    angleNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setPositionWithAngle(ctx); });

    posNode->addChild(angleNode);
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
    DimensionId dimensionId = DimensionId(0); // 默认主世界
    auto* dimension = server->dimensionManager().getDimension(dimensionId);

    if (!dimension) {
        source.sendError("Dimension not found");
        return 0;
    }

    const Vector3d& pos = source.position();
    // 参考 MC 1.21.11: 不指定位置时使用玩家的朝向（rotation.y 为 yaw）
    f32 angle = source.rotation().y;
    dimension->setSpawnPoint(pos);

    // 同步更新 ServerWorld 的世界出生点和朝向
    if (dimension->world()) {
        dimension->world()->setWorldSpawnPoint(pos, angle);
    }

    // 广播新的出生点到所有玩家
    _broadcastSpawnPosition(server, pos, angle);

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

    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", source);

    DimensionId dimensionId = DimensionId(0); // 默认主世界
    auto* dimension = server->dimensionManager().getDimension(dimensionId);

    if (!dimension) {
        source.sendError("Dimension not found");
        return 0;
    }

    dimension->setSpawnPoint(pos);

    // 不指定朝向时默认为 0.0f
    f32 angle = 0.0f;

    // 同步更新 ServerWorld 的世界出生点
    if (dimension->world()) {
        dimension->world()->setWorldSpawnPoint(pos, angle);
    }

    // 广播新的出生点到所有玩家
    _broadcastSpawnPosition(server, pos, angle);

    std::ostringstream ss;
    ss << "Set world spawn point to " << static_cast<BlockCoord>(pos.x) << ", " << static_cast<BlockCoord>(pos.y)
       << ", " << static_cast<BlockCoord>(pos.z);
    source.sendMessage(ss.str());

    return 1;
}

i32 SetWorldSpawnCommand::_setPositionWithAngle(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();

    const Vector3d pos = Vec3ArgumentType::getVec3(context, "pos", source);
    f32 angle = context.getArgument<f32>("angle");
    // 将角度归一化到 [-180, 180] 范围，与 MC 原版行为一致
    angle = math::wrapDegrees(angle);

    DimensionId dimensionId = DimensionId(0); // 默认主世界
    auto* dimension = server->dimensionManager().getDimension(dimensionId);

    if (!dimension) {
        source.sendError("Dimension not found");
        return 0;
    }

    dimension->setSpawnPoint(pos);

    // 同步更新 ServerWorld 的世界出生点和朝向
    if (dimension->world()) {
        dimension->world()->setWorldSpawnPoint(pos, angle);
    }

    // 广播新的出生点到所有玩家
    _broadcastSpawnPosition(server, pos, angle);

    std::ostringstream ss;
    ss << "Set world spawn point to " << static_cast<BlockCoord>(pos.x) << ", " << static_cast<BlockCoord>(pos.y)
       << ", " << static_cast<BlockCoord>(pos.z) << " (angle: " << angle << ")";
    source.sendMessage(ss.str());

    return 1;
}

void SetWorldSpawnCommand::_broadcastSpawnPosition(server::IServer* server, const Vector3d& pos, f32 angle)
{
    // 1.21.11 SetDefaultSpawnPosition：dimension(ResourceKey) + blockPosPacked + yaw + pitch
    // TODO(Phase6): dimension 当前固定主世界，命令只作用于主世界出生点；多维度出生点需补。
    mc::network::ir::play::SetDefaultSpawnPosition pkt;
    pkt.dimension = "minecraft:overworld";
    pkt.blockPosPacked = BlockPos(static_cast<BlockCoord>(pos.x), static_cast<BlockCoord>(pos.y),
                                static_cast<BlockCoord>(pos.z))
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
