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

#include "TeleportCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/core/TeleportManager.hpp"

#include <memory>
#include <sstream>
#include <vector>

namespace mc {
namespace command {
namespace {

/**
 * @brief 读取命令中的三维坐标参数。
 *
 * @param context 命令上下文。
 * @return 解析出的坐标。
 */
[[nodiscard]] Vector3d readTargetPosition(CommandContext<ServerCommandSource>& context)
{
    return Vector3d(static_cast<f64>(context.getArgument<f32>("x")),
        static_cast<f64>(context.getArgument<f32>("y")),
        static_cast<f64>(context.getArgument<f32>("z")));
}

/**
 * @brief 读取目标玩家当前位置与朝向。
 *
 * @param source 命令源。
 * @param selector 目标选择器。
 * @return 是否读取成功。
 *
 * @note 该辅助函数只接受单个玩家结果，多结果由参数类型约束在解析阶段拦截。
 */
[[nodiscard]] bool tryResolveDestinationPlayer(const ServerCommandSource& source,
    const EntitySelector& selector,
    const server::ServerPlayerData*& destinationPlayer)
{
    destinationPlayer = nullptr;
    const PlayerId destinationPlayerId = support::resolveSinglePlayerId(source, selector);
    if (destinationPlayerId == 0 || source.server() == nullptr) {
        return false;
    }

    destinationPlayer = source.server()->playerManager().getPlayer(destinationPlayerId);
    return destinationPlayer != nullptr;
}

/**
 * @brief 统一执行一组玩家的传送请求。
 *
 * @param source 命令源。
 * @param targetPlayerIds 目标玩家集合。
 * @param position 目标坐标。
 * @param rotation 目标朝向。
 * @return 成功传送的玩家数量。
 */
[[nodiscard]] i32 teleportPlayers(ServerCommandSource& source,
    const std::vector<PlayerId>& targetPlayerIds,
    const Vector3d& position,
    const Vector2f& rotation)
{
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    i32 teleportedCount = 0;
    for (const PlayerId playerId : targetPlayerIds) {
        if (playerId == 0) {
            continue;
        }

        if (server->teleportManager().requestTeleport(
                playerId, position.x, position.y, position.z, rotation.x, rotation.y) != 0) {
            ++teleportedCount;
        }
    }

    return teleportedCount;
}

} // namespace

void TeleportCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto tpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("tp");
    tpNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(tpNode,
        support::makeMetadata("Teleport entities.",
            "/tp <destination>|<x> <y> <z>|<targets> <destination>|<targets> <x> <y> <z>",
            2,
            {"teleport"},
            true));

    auto teleportNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("teleport");
    teleportNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    teleportNode->setRedirect(tpNode);

    auto selfDestinationArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::player());
    selfDestinationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _teleportToEntity(ctx); });

    auto selfXArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("x", FloatArgumentType::floatArg());
    auto selfYArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("y", FloatArgumentType::floatArg());
    auto selfZArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("z", FloatArgumentType::floatArg());
    selfZArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _teleportToPosition(ctx); });
    selfYArg->addChild(selfZArg);
    selfXArg->addChild(selfYArg);

    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());

    auto destinationArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "destination", EntityArgumentType::player());
    destinationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _teleportTargetToEntity(ctx); });

    auto targetXArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("x", FloatArgumentType::floatArg());
    auto targetYArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("y", FloatArgumentType::floatArg());
    auto targetZArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("z", FloatArgumentType::floatArg());
    targetZArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _teleportTargetToPosition(ctx); });
    targetYArg->addChild(targetZArg);
    targetXArg->addChild(targetYArg);

    targetsArg->addChild(destinationArg);
    targetsArg->addChild(targetXArg);

    tpNode->addChild(selfDestinationArg);
    tpNode->addChild(selfXArg);
    tpNode->addChild(targetsArg);

    dispatcher.registerCommand(tpNode);
    dispatcher.registerCommand(teleportNode);
}

/**
 * @brief 将命令源玩家传送到目标玩家位置。
 *
 * @param context 命令上下文。
 * @return 成功时返回 `1`，失败时返回 `0`。
 */
i32 TeleportCommand::_teleportToEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    if (!source.isPlayer()) {
        source.sendError("You must be a player to teleport yourself");
        return 0;
    }

    const EntitySelector selector = context.getArgument<EntitySelector>("target");
    const server::ServerPlayerData* destinationPlayer = nullptr;
    if (!tryResolveDestinationPlayer(source, selector, destinationPlayer)) {
        source.sendError("No matching destination player was found");
        return 0;
    }

    const i32 teleportedCount = teleportPlayers(source,
        {source.playerId()},
        Vector3d(destinationPlayer->x, destinationPlayer->y, destinationPlayer->z),
        Vector2f(destinationPlayer->yaw, destinationPlayer->pitch));
    if (teleportedCount == 0) {
        source.sendMessage("Failed to teleport player");
        return 0;
    }

    std::ostringstream ss;
    ss << "Teleported " << source.name() << " to " << destinationPlayer->username;
    source.sendMessage(ss.str());
    return 1;
}

/**
 * @brief 将命令源玩家传送到指定坐标。
 *
 * @param context 命令上下文。
 * @return 成功时返回 `1`，失败时返回 `0`。
 */
i32 TeleportCommand::_teleportToPosition(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    if (!source.isPlayer()) {
        source.sendError("You must be a player to teleport yourself");
        return 0;
    }

    const Vector3d position = readTargetPosition(context);
    const i32 teleportedCount = teleportPlayers(source, {source.playerId()}, position, source.rotation());
    if (teleportedCount == 0) {
        source.sendMessage("Failed to teleport player");
        return 0;
    }

    std::ostringstream ss;
    ss << "Teleported " << source.name() << " to " << position.x << ", " << position.y << ", " << position.z;
    source.sendMessage(ss.str());
    return 1;
}

/**
 * @brief 将目标玩家集合传送到目标玩家位置。
 *
 * @param context 命令上下文。
 * @return 成功传送的玩家数量。
 */
i32 TeleportCommand::_teleportTargetToEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    const EntitySelector targets = context.getArgument<EntitySelector>("targets");
    const EntitySelector destination = context.getArgument<EntitySelector>("destination");
    const auto targetPlayerIds = support::resolvePlayerIds(source, targets);

    const server::ServerPlayerData* destinationPlayer = nullptr;
    if (!tryResolveDestinationPlayer(source, destination, destinationPlayer)) {
        source.sendError("No matching destination player was found");
        return 0;
    }

    const i32 teleportedCount = teleportPlayers(source,
        targetPlayerIds,
        Vector3d(destinationPlayer->x, destinationPlayer->y, destinationPlayer->z),
        Vector2f(destinationPlayer->yaw, destinationPlayer->pitch));
    if (teleportedCount == 0) {
        source.sendError("No matching players were found");
        return 0;
    }

    std::ostringstream ss;
    ss << "Teleported " << teleportedCount << " player(s) to " << destinationPlayer->username;
    source.sendMessage(ss.str());
    return teleportedCount;
}

/**
 * @brief 将目标玩家集合传送到指定坐标。
 *
 * @param context 命令上下文。
 * @return 成功传送的玩家数量。
 */
i32 TeleportCommand::_teleportTargetToPosition(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector targets = context.getArgument<EntitySelector>("targets");
    const auto targetPlayerIds = support::resolvePlayerIds(source, targets);
    const Vector3d position = readTargetPosition(context);

    const i32 teleportedCount = teleportPlayers(source, targetPlayerIds, position, source.rotation());
    if (teleportedCount == 0) {
        source.sendError("No matching players were found");
        return 0;
    }

    std::ostringstream ss;
    ss << "Teleported " << teleportedCount << " player(s) to " << position.x << ", " << position.y << ", "
       << position.z;
    source.sendMessage(ss.str());
    return teleportedCount;
}

} // namespace command
} // namespace mc
