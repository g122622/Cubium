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

#include "ServerDragonBossBar.hpp"
#include "common/network/packet/BossInfoPacket.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "server/application/IServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace server {

ServerDragonBossBar::ServerDragonBossBar(IServer& server,
    Uuid uuid,
    std::unique_ptr<text::ITextComponent> name,
    BossInfoColor color,
    BossInfoOverlay overlay)
    : m_server(server)
    , m_uuid(std::move(uuid))
    , m_name(std::move(name))
    , m_color(color)
    , m_overlay(overlay)
{}

ServerDragonBossBar::~ServerDragonBossBar()
{
    // 析构时移除所有玩家（发送移除包）
    removeAllPlayers();
}

void ServerDragonBossBar::setPercent(f32 percent)
{
    const f32 clamped = math::clamp(percent, 0.0f, 1.0f);
    if (m_percent != clamped) {
        m_percent = clamped;
        _broadcastUpdate(network::BossInfoAction::UpdatePercent);
    }
}

void ServerDragonBossBar::setName(std::unique_ptr<text::ITextComponent> name)
{
    if (name == nullptr) {
        return;
    }
    m_name = std::move(name);
    _broadcastUpdate(network::BossInfoAction::UpdateName);
}

void ServerDragonBossBar::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        // 可见性变化不发送单独的更新包，而是通过重新发送 Add/Remove 包实现
        // 对应 MC Java: ServerBossEvent.setVisible() 内部调用 broadcastUpdate()
        // 但实际可见性对客户端的影响通过 Add/Remove 包控制
        if (m_visible) {
            // 变为可见：向所有已追踪玩家发送 Add 包
            for (PlayerId playerId : m_players) {
                _sendAddPacket(playerId);
            }
        } else {
            // 变为不可见：向所有已追踪玩家发送 Remove 包（但保留在 m_players 中）
            for (PlayerId playerId : m_players) {
                _sendRemovePacket(playerId);
            }
        }
    }
}

void ServerDragonBossBar::addPlayer(PlayerId playerId)
{
    if (m_players.insert(playerId).second && m_visible) {
        _sendAddPacket(playerId);
    }
}

void ServerDragonBossBar::removePlayer(PlayerId playerId)
{
    if (m_players.erase(playerId) > 0 && m_visible) {
        _sendRemovePacket(playerId);
    }
}

void ServerDragonBossBar::removeAllPlayers()
{
    if (m_visible) {
        for (PlayerId playerId : m_players) {
            _sendRemovePacket(playerId);
        }
    }
    m_players.clear();
}

void ServerDragonBossBar::replacePlayers(const std::set<PlayerId>& playerIds)
{
    // 计算要移除的玩家（当前已追踪但不在新列表中）
    std::vector<PlayerId> toRemove;
    for (PlayerId pid : m_players) {
        if (playerIds.find(pid) == playerIds.end()) {
            toRemove.push_back(pid);
        }
    }

    // 计算要添加的玩家（在新列表中但当前未追踪）
    std::vector<PlayerId> toAdd;
    for (PlayerId pid : playerIds) {
        if (m_players.find(pid) == m_players.end()) {
            toAdd.push_back(pid);
        }
    }

    // 执行移除
    for (PlayerId pid : toRemove) {
        m_players.erase(pid);
        if (m_visible) {
            _sendRemovePacket(pid);
        }
    }

    // 执行添加
    for (PlayerId pid : toAdd) {
        m_players.insert(pid);
        if (m_visible) {
            _sendAddPacket(pid);
        }
    }
}

bool ServerDragonBossBar::hasPlayers() const
{
    return !m_players.empty();
}

f32 ServerDragonBossBar::percent() const
{
    return m_percent;
}

bool ServerDragonBossBar::visible() const
{
    return m_visible;
}

void ServerDragonBossBar::_sendAddPacket(PlayerId playerId)
{
    MC_ASSERT_RELEASE(m_name != nullptr);

    auto nameCopy = m_name->deepCopy();
    network::BossInfoPacket packet = network::BossInfoPacket::add(m_uuid,
        std::move(nameCopy),
        m_percent,
        static_cast<u8>(m_color),
        static_cast<u8>(m_overlay),
        m_darkenSky,
        m_playEndBossMusic,
        m_createFog);

    auto result = packet.serialize();
    if (!result.success()) {
        spdlog::error("ServerDragonBossBar: Failed to serialize BossInfoPacket (Add): {}", result.error().message());
        return;
    }

    m_server.connectionManager().sendPacketToPlayer(playerId, network::PacketType::BossInfo, result.value());
}

void ServerDragonBossBar::_sendRemovePacket(PlayerId playerId)
{
    network::BossInfoPacket packet = network::BossInfoPacket::remove(m_uuid);

    auto result = packet.serialize();
    if (!result.success()) {
        spdlog::error("ServerDragonBossBar: Failed to serialize BossInfoPacket (Remove): {}", result.error().message());
        return;
    }

    m_server.connectionManager().sendPacketToPlayer(playerId, network::PacketType::BossInfo, result.value());
}

void ServerDragonBossBar::_broadcastUpdate(network::BossInfoAction action)
{
    if (m_players.empty()) {
        return;
    }

    // 根据操作类型构建不同的更新包
    network::BossInfoPacket packet = [&]() {
        switch (action) {
            case network::BossInfoAction::UpdatePercent:
                return network::BossInfoPacket::updatePercent(m_uuid, m_percent);

            case network::BossInfoAction::UpdateName: {
                MC_ASSERT_RELEASE(m_name != nullptr);
                auto nameCopy = m_name->deepCopy();
                return network::BossInfoPacket::updateName(m_uuid, std::move(nameCopy));
            }

            case network::BossInfoAction::UpdateProperties:
                return network::BossInfoPacket::updateProperties(m_uuid, m_darkenSky, m_playEndBossMusic, m_createFog);

            default:
                return network::BossInfoPacket::updatePercent(m_uuid, m_percent);
        }
    }();

    auto result = packet.serialize();
    if (!result.success()) {
        spdlog::error("ServerDragonBossBar: Failed to serialize BossInfoPacket (Update): {}", result.error().message());
        return;
    }

    for (PlayerId playerId : m_players) {
        m_server.connectionManager().sendPacketToPlayer(playerId, network::PacketType::BossInfo, result.value());
    }
}

} // namespace server
} // namespace mc
