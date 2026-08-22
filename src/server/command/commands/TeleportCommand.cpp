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
#include "common/entity/entities/player/Player.hpp" // Player::setPosition/setRotation（实体旁路传送）
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
#include "server/world/player/ServerPlayerEntityManager.hpp"

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
 * @brief 目的地玩家解析结果（位置 + 朝向 + 名字）。
 *
 * 真实玩家从 ServerPlayerData 取，SimulatedPlayer（不在 PlayerManager）从 Player 实体回退取。
 * valid=false 表示两者都查不到。
 */
struct DestinationInfo {
    Vector3d position{};
    Vector2f rotation{};
    std::string username;
    bool valid = false;
};

/**
 * @brief 读取目标玩家（目的地）当前位置与朝向。
 *
 * @param source 命令源。
 * @param selector 目标选择器。
 * @param info 输出目的地信息。
 * @return 是否读取成功。
 *
 * @note 该辅助函数只接受单个玩家结果，多结果由参数类型约束在解析阶段拦截。
 *       经 resolveSinglePlayerId 解析 PlayerId（支持 SimulatedPlayer 名字/选择器），再取位置/朝向：
 *       PlayerManager 优先（真实玩家 ServerPlayerData），回退 ServerPlayerEntityManager 的 Player 实体
 *       （SimulatedPlayer 路径）。原先仅经 playerManager().getPlayer(id) 取数，SimulatedPlayer 返 nullptr
 *       即判失败，导致 /tp <targets> <destPlayer> 当 dest 是 SimulatedPlayer 时整个传送不执行。
 */
[[nodiscard]] bool tryResolveDestinationPlayer(
    const ServerCommandSource& source, const EntitySelector& selector, DestinationInfo& info)
{
    info = DestinationInfo{};
    const PlayerId destinationPlayerId = support::resolveSinglePlayerId(source, selector);
    if (destinationPlayerId == 0 || source.server() == nullptr) {
        return false;
    }

    // 真实玩家路径：PlayerManager 持有 ServerPlayerData（含位置/朝向/名字）。
    const server::ServerPlayerData* data = source.server()->playerManager().getPlayer(destinationPlayerId);
    if (data != nullptr) {
        info.position = Vector3d(static_cast<f64>(data->x), static_cast<f64>(data->y), static_cast<f64>(data->z));
        info.rotation = Vector2f(data->yaw, data->pitch);
        info.username = data->username;
        info.valid = true;
        return true;
    }

    // SimulatedPlayer 回退：经 ServerPlayerEntityManager 取 Player 实体的位置/朝向/名字。
    auto* world = source.world();
    if (world == nullptr) {
        return false;
    }
    mc::Player* entity = source.server()->playerEntityManager().getPlayerEntity(destinationPlayerId, *world);
    if (entity == nullptr) {
        return false;
    }
    const auto pos = entity->position();
    info.position = Vector3d(static_cast<f64>(pos.x), static_cast<f64>(pos.y), static_cast<f64>(pos.z));
    info.rotation = Vector2f(entity->yaw(), entity->pitch());
    info.username = entity->username();
    info.valid = true;
    return true;
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

        // 真实玩家路径：经 TeleportManager 改 ServerPlayerData + 发传送包，客户端回移动包后实体收敛。
        if (server->teleportManager().requestTeleport(
                playerId, position.x, position.y, position.z, rotation.x, rotation.y) != 0) {
            ++teleportedCount;
            continue;
        }

        // 回退路径：PlayerManager 查不到该 PlayerId（SimulatedPlayer 不进 PlayerManager，仅有
        // ServerPlayerEntityManager 映射）。经实体管理器解析 ServerPlayer 实体直接 setPosition，
        // 立即改变实体位置（Entity::setPosition 非虚、无网络副作用）。对齐 vanilla 服务端 /tp 立即
        // 移动实体的语义。SimulatedPlayer 无连接，setRotation 写实体朝向即可。
        auto* world = source.world();
        if (world == nullptr) {
            continue;
        }
        mc::Player* playerEntity = server->playerEntityManager().getPlayerEntity(playerId, *world);
        if (playerEntity == nullptr) {
            continue;
        }
        playerEntity->setPosition(
            static_cast<f32>(position.x), static_cast<f32>(position.y), static_cast<f32>(position.z));
        playerEntity->setRotation(rotation.x, rotation.y);
        ++teleportedCount;
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
    DestinationInfo destination;
    if (!tryResolveDestinationPlayer(source, selector, destination)) {
        source.sendError("No matching destination player was found");
        return 0;
    }

    const i32 teleportedCount =
        teleportPlayers(source, {source.playerId()}, destination.position, destination.rotation);
    if (teleportedCount == 0) {
        source.sendMessage("Failed to teleport player");
        return 0;
    }

    std::ostringstream ss;
    ss << "Teleported " << source.name() << " to " << destination.username;
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
    const EntitySelector destinationSelector = context.getArgument<EntitySelector>("destination");
    const auto targetPlayerIds = support::resolvePlayerIds(source, targets);

    DestinationInfo destination;
    if (!tryResolveDestinationPlayer(source, destinationSelector, destination)) {
        source.sendError("No matching destination player was found");
        return 0;
    }

    const i32 teleportedCount = teleportPlayers(source, targetPlayerIds, destination.position, destination.rotation);
    if (teleportedCount == 0) {
        source.sendError("No matching players were found");
        return 0;
    }

    std::ostringstream ss;
    ss << "Teleported " << teleportedCount << " player(s) to " << destination.username;
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
