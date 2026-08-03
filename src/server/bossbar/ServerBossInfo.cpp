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

#include "ServerBossInfo.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "server/bossbar/BossInfo.hpp"
#include "server/player/ServerPlayer.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace server {

ServerBossInfo::ServerBossInfo(
    Uuid uuid, std::unique_ptr<text::ITextComponent> name, BossInfoColor color, BossInfoOverlay overlay)
    : BossInfo(std::move(uuid), std::move(name), color, overlay)
{}

void ServerBossInfo::addPlayer(::mc::ServerPlayer& player)
{
    PlayerId playerId = player.playerId();
    if (m_players.insert(playerId).second) {
        // 新玩家添加成功，发送添加包
        sendAddPacket(player);
    }
}

void ServerBossInfo::removePlayer(::mc::ServerPlayer& player)
{
    PlayerId playerId = player.playerId();
    if (m_players.erase(playerId) > 0) {
        // 玩家移除成功，发送移除包
        sendRemovePacket(player);
    }
}

void ServerBossInfo::removeAllPlayers()
{
    // 注意：此方法不发送网络包，由子类 CustomServerBossInfo 负责
    m_players.clear();
}

void ServerBossInfo::setName(std::unique_ptr<text::ITextComponent> name)
{
    if (name && !name->getUnformattedText().empty()) {
        BossInfo::setName(std::move(name));
        m_pendingUpdateType = BossInfoUpdateType::UpdateName;
        broadcastUpdate();
    }
}

void ServerBossInfo::setPercent(f32 percent)
{
    if (m_percent != percent) {
        BossInfo::setPercent(percent);
        m_pendingUpdateType = BossInfoUpdateType::UpdatePercent;
        broadcastUpdate();
    }
}

void ServerBossInfo::setColor(BossInfoColor color)
{
    if (m_color != color) {
        BossInfo::setColor(color);
        m_pendingUpdateType = BossInfoUpdateType::UpdateStyle;
        broadcastUpdate();
    }
}

void ServerBossInfo::setOverlay(BossInfoOverlay overlay)
{
    if (m_overlay != overlay) {
        BossInfo::setOverlay(overlay);
        m_pendingUpdateType = BossInfoUpdateType::UpdateStyle;
        broadcastUpdate();
    }
}

void ServerBossInfo::setDarkenSky(bool darken)
{
    if (m_darkenSky != darken) {
        BossInfo::setDarkenSky(darken);
        m_pendingUpdateType = BossInfoUpdateType::UpdateProperties;
        broadcastUpdate();
    }
}

void ServerBossInfo::setPlayEndBossMusic(bool play)
{
    if (m_playEndBossMusic != play) {
        BossInfo::setPlayEndBossMusic(play);
        m_pendingUpdateType = BossInfoUpdateType::UpdateProperties;
        broadcastUpdate();
    }
}

void ServerBossInfo::setCreateFog(bool create)
{
    if (m_createFog != create) {
        BossInfo::setCreateFog(create);
        m_pendingUpdateType = BossInfoUpdateType::UpdateProperties;
        broadcastUpdate();
    }
}

void ServerBossInfo::setVisible(bool visible)
{
    if (m_visible != visible) {
        BossInfo::setVisible(visible);
        broadcastUpdate();
    }
}

void ServerBossInfo::broadcastUpdate()
{
    // 基类默认实现为空，由 CustomServerBossInfo 通过 Manager 发送
}

void ServerBossInfo::sendAddPacket(::mc::ServerPlayer& player)
{
    // 基类默认实现为空，由 CustomServerBossInfo 通过 Manager 发送
    MC_UNUSED(player);
}

void ServerBossInfo::sendRemovePacket(::mc::ServerPlayer& player)
{
    // 基类默认实现为空，由 CustomServerBossInfo 通过 Manager 发送
    MC_UNUSED(player);
}

} // namespace server
} // namespace mc
