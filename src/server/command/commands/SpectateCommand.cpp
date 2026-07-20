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

#include "SpectateCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include <sstream>

namespace mc {
namespace command {

void SpectateCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto spectateNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("spectate");
    spectateNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(spectateNode,
        support::makeMetadata(
            "Makes a player in spectator mode spectate another entity.", "/spectate <target> [player]", 2, {}, true));

    // /spectate <target> [player]
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _startSpectating(ctx); });
    targetArg->addChild(playerArg);
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _startSpectating(ctx); });
    spectateNode->addChild(targetArg);

    // /spectate stop [player]
    auto stopNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("stop");
    auto stopPlayerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());
    stopPlayerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _stopSpectating(ctx); });
    stopNode->addChild(stopPlayerArg);
    stopNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _stopSpectating(ctx); });
    spectateNode->addChild(stopNode);

    dispatcher.registerCommand(spectateNode);
}

i32 SpectateCommand::_startSpectating(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& targetSelector = context.getArgument<EntitySelector>("target");

    // 解析目标实体（使用 EntityResolver 以支持任意实体，不仅仅是玩家）
    Entity* targetEntity = support::EntityResolver::resolveSingle(source, targetSelector);
    if (targetEntity == nullptr) {
        source.sendError("No entity matched the target selector");
        return 0;
    }

    // 获取旁观者（命令发送者或指定玩家）
    PlayerId spectatorId;
    if (context.hasArgument("player")) {
        const EntitySelector& playerSelector = context.getArgument<EntitySelector>("player");
        auto playerIds = support::resolvePlayerIds(source, playerSelector);
        if (playerIds.empty()) {
            source.sendError("No player matched the player selector");
            return 0;
        }
        spectatorId = playerIds[0];
    } else {
        if (!source.isPlayer()) {
            source.sendError("Only players can use this command");
            return 0;
        }
        spectatorId = source.playerId();
    }

    // 获取旁观者的 ServerPlayerData 和 ServerPlayer 实体
    auto* playerData = source.server()->playerManager().getPlayer(spectatorId);
    if (playerData == nullptr) {
        source.sendError("Spectator not found");
        return 0;
    }

    // 检查旁观者模式
    if (playerData->gameMode != GameMode::Spectator) {
        source.sendError("Player must be in spectator mode to spectate");
        return 0;
    }

    // 不能旁观自己
    if (static_cast<EntityInstanceId>(spectatorId) == targetEntity->id()) {
        source.sendError("Cannot spectate yourself");
        return 0;
    }

    // 获取 ServerPlayer 实体以调用 setCamera
    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("World not available");
        return 0;
    }

    auto* serverWorld = dynamic_cast<server::ServerWorld*>(world);
    if (serverWorld == nullptr) {
        source.sendError("Server world not available");
        return 0;
    }

    Entity* spectatorEntity = serverWorld->getEntity(static_cast<EntityInstanceId>(spectatorId));
    if (spectatorEntity == nullptr) {
        source.sendError("Spectator entity not found");
        return 0;
    }

    auto* serverPlayer = dynamic_cast<ServerPlayer*>(spectatorEntity);
    if (serverPlayer == nullptr) {
        source.sendError("Spectator is not a server player");
        return 0;
    }

    // 设置旁观目标
    if (!serverPlayer->setCamera(targetEntity)) {
        source.sendError("Failed to set spectate target");
        return 0;
    }

    std::ostringstream ss;
    ss << playerData->username << " is now spectating " << targetEntity->id();
    source.sendMessage(ss.str());

    return 1;
}

i32 SpectateCommand::_stopSpectating(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 获取玩家（命令发送者或指定玩家）
    PlayerId spectatorId;
    if (context.hasArgument("player")) {
        const EntitySelector& playerSelector = context.getArgument<EntitySelector>("player");
        auto playerIds = support::resolvePlayerIds(source, playerSelector);
        if (playerIds.empty()) {
            source.sendError("No player matched the player selector");
            return 0;
        }
        spectatorId = playerIds[0];
    } else {
        if (!source.isPlayer()) {
            source.sendError("Only players can use this command");
            return 0;
        }
        spectatorId = source.playerId();
    }

    auto* playerData = source.server()->playerManager().getPlayer(spectatorId);
    if (playerData == nullptr) {
        source.sendError("Player not found");
        return 0;
    }

    // 获取 ServerPlayer 实体
    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("World not available");
        return 0;
    }

    auto* serverWorld = dynamic_cast<server::ServerWorld*>(world);
    if (serverWorld == nullptr) {
        source.sendError("Server world not available");
        return 0;
    }

    Entity* spectatorEntity = serverWorld->getEntity(static_cast<EntityInstanceId>(spectatorId));
    if (spectatorEntity == nullptr) {
        source.sendError("Player entity not found");
        return 0;
    }

    auto* serverPlayer = dynamic_cast<ServerPlayer*>(spectatorEntity);
    if (serverPlayer == nullptr) {
        source.sendError("Player is not a server player");
        return 0;
    }

    // 重置旁观目标
    serverPlayer->resetCamera();

    std::ostringstream ss;
    ss << playerData->username << " is no longer spectating";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
