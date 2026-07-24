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

#include "GameModeManager.hpp"
#include "ConnectionManager.hpp"
#include "PlayerManager.hpp"
#include "ServerPlayerData.hpp"
#include "common/entity/entities/player/GameModeUtils.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::core {

GameModeManager::GameModeManager(PlayerManager& playerManager, ConnectionManager& connectionManager)
    : m_playerManager(playerManager)
    , m_connectionManager(connectionManager)
{}

// ============================================================================
// 游戏模式管理
// ============================================================================

bool GameModeManager::setGameMode(PlayerId playerId, GameMode mode)
{
    // 获取玩家数据
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        spdlog::warn("GameModeManager: Player {} not found", playerId);
        return false;
    }

    GameMode oldMode = player->gameMode;

    // 如果模式相同，不需要更新
    if (oldMode == mode) {
        return true;
    }

    // 更新玩家游戏模式
    player->gameMode = mode;

    // 根据游戏模式更新玩家能力
    // 注意：飞行状态等能力会在客户端收到 PlayerAbilitiesPacket 后更新

    // 发送网络包
    if (!_sendGameModeChangePacket(playerId, mode)) {
        spdlog::warn("GameModeManager: Failed to send game mode change packet to player {}", playerId);
    }

    if (!_sendAbilitiesPacket(playerId, mode)) {
        spdlog::warn("GameModeManager: Failed to send abilities packet to player {}", playerId);
    }

    // 调用回调
    if (m_onGameModeChange) {
        m_onGameModeChange(playerId, oldMode, mode);
    }

    spdlog::info("GameModeManager: Player {} changed game mode from {} to {}",
        playerId,
        static_cast<int>(oldMode),
        static_cast<int>(mode));

    return true;
}

bool GameModeManager::setGameModeLocal(PlayerId playerId, GameMode mode)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return false;
    }

    player->gameMode = mode;
    return true;
}

GameMode GameModeManager::getGameMode(PlayerId playerId) const noexcept
{
    const auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return GameMode::NotSet;
    }
    return player->gameMode;
}

// ============================================================================
// 能力同步
// ============================================================================

bool GameModeManager::syncAbilities(PlayerId playerId)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return false;
    }

    return _sendAbilitiesPacket(playerId, player->gameMode);
}

u8 GameModeManager::getAbilitiesForGameMode(GameMode mode) noexcept
{
    // 使用 GameModeUtils 计算能力
    PlayerAbilities abilities = entity::GameModeUtils::getAbilitiesForGameMode(mode);

    u8 flags = 0;
    if (abilities.invulnerable) {
        flags |= static_cast<u8>(network::PlayerAbilityFlags::Invulnerable);
    }
    if (abilities.flying) {
        flags |= static_cast<u8>(network::PlayerAbilityFlags::Flying);
    }
    if (abilities.canFly) {
        flags |= static_cast<u8>(network::PlayerAbilityFlags::CanFly);
    }
    if (abilities.creativeMode) {
        flags |= static_cast<u8>(network::PlayerAbilityFlags::CreativeMode);
    }

    return flags;
}

// ============================================================================
// 私有方法
// ============================================================================

bool GameModeManager::_sendGameModeChangePacket(PlayerId playerId, GameMode mode)
{
    // 对应 ir::play::GameEvent：event=ChangeGameMode(3)，value=mode
    mc::network::ir::play::GameEvent evt;
    evt.event = 3; // ChangeGameMode
    evt.value = static_cast<f32>(mode);

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(evt)},
    };
    return m_connectionManager.sendToPlayer(playerId, packet);
}

bool GameModeManager::_sendAbilitiesPacket(PlayerId playerId, GameMode mode)
{
    mc::network::ir::play::PlayerAbilities abilities;
    abilities.flags = getAbilitiesForGameMode(mode);
    abilities.flyingSpeed = 0.05f; // MC 默认飞行速度
    abilities.walkingSpeed = 0.1f; // MC 默认行走速度

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(abilities)},
    };
    return m_connectionManager.sendToPlayer(playerId, packet);
}

} // namespace mc::server::core
