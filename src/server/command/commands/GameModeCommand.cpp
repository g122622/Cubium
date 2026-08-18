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

#include "GameModeCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp" // Player::setGameMode（实体旁路）
#include "common/util/assert/AssertMacros.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp" // getPlayerEntity（实体旁路解析）

#include <memory>
#include <sstream>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {
namespace {

/**
 * @brief 对单个玩家设置游戏模式，含 SimulatedPlayer 实体旁路。
 *
 * 真实玩家走 GameModeManager（改 ServerPlayerData + 发网络包，客户端收敛后实体生效）；
 * GameModeManager 经 PlayerManager 查 ServerPlayerData，对 SimulatedPlayer（不进 PlayerManager）
 * 返回 false，此时回退经 ServerPlayerEntityManager 解析实体直接调 Player::setGameMode
 * （写实体 m_gameMode + abilities + noclip，立即生效）。对齐 TeleportCommand 的旁路模式。
 *
 * @return true 设置成功（含模式相同的短路）。
 */
[[nodiscard]] bool setGameModeOnPlayer(ServerCommandSource& source, PlayerId playerId, GameMode mode)
{
    auto* server = source.server();
    if (server == nullptr) {
        return false;
    }

    if (server->gameModeManager().setGameMode(playerId, mode)) {
        return true;
    }

    // 回退：SimulatedPlayer 不在 PlayerManager，经实体管理器解析实体直接设游戏模式。
    auto* world = source.world();
    if (world == nullptr) {
        return false;
    }
    mc::Player* playerEntity = server->playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return false;
    }
    playerEntity->setGameMode(mode);
    return true;
}

} // namespace

void GameModeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto modeArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, GameMode>>("mode", GameModeArgumentType::gameMode());

    modeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setGameModeSelf(ctx); });

    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::players());
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setGameModeOthers(ctx); });

    modeArg->addChild(targetArg);

    auto literalNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("gamemode");
    literalNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(literalNode,
        support::makeMetadata(
            "Change a player's game mode.", "/gamemode <survival|creative|adventure|spectator> [target]", 2, {}, true));
    literalNode->addChild(modeArg);

    dispatcher.registerCommand(literalNode);
}

/**
 * @brief 将命令源自身切换到指定游戏模式。
 *
 * @param context 命令上下文。
 * @return 成功时返回 `1`，失败时返回 `0`。
 */
i32 GameModeCommand::_setGameModeSelf(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    if (!source.isPlayer()) {
        source.sendError("You must be a player to change your own game mode");
        return 0;
    }

    const GameMode mode = context.getArgument<GameMode>("mode");
    const PlayerId playerId = source.playerId();
    if (playerId == 0 || !setGameModeOnPlayer(source, playerId, mode)) {
        source.sendMessage("Failed to change game mode");
        return 0;
    }

    std::ostringstream ss;
    ss << "Set " << source.name() << "'s game mode to " << _getGameModeName(mode);
    source.sendMessage(ss.str());
    return 1;
}

/**
 * @brief 将目标玩家集合切换到指定游戏模式。
 *
 * @param context 命令上下文。
 * @return 成功修改的玩家数量。
 */
i32 GameModeCommand::_setGameModeOthers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    const GameMode mode = context.getArgument<GameMode>("mode");
    const EntitySelector selector = context.getArgument<EntitySelector>("target");
    const auto playerIds = support::resolvePlayerIds(source, selector);

    i32 changedCount = 0;
    for (const PlayerId playerId : playerIds) {
        if (playerId == 0) {
            continue;
        }

        if (setGameModeOnPlayer(source, playerId, mode)) {
            ++changedCount;
        }
    }

    if (changedCount == 0) {
        source.sendError("No matching players were found");
        return 0;
    }

    std::ostringstream ss;
    ss << "Set game mode of " << changedCount << " player(s) to " << _getGameModeName(mode);
    source.sendMessage(ss.str());
    return changedCount;
}

/**
 * @brief 获取游戏模式的反馈名称。
 *
 * @param mode 游戏模式。
 * @return 用于命令反馈的可读名称。
 */
const char* GameModeCommand::_getGameModeName(GameMode mode)
{
    switch (mode) {
        case GameMode::Survival:
            return "survival";
        case GameMode::Creative:
            return "creative";
        case GameMode::Adventure:
            return "adventure";
        case GameMode::Spectator:
            return "spectator";
        case GameMode::NotSet:
            return "not_set";
    }

    return "unknown";
}

} // namespace command
} // namespace mc
