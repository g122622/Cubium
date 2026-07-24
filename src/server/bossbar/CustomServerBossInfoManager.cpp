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

#include "CustomServerBossInfoManager.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "server/application/IServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {
namespace server {

namespace {

/// 打包 BossEvent flags：bit0=DARKEN bit1=MUSIC bit2=FOG
u8 packBossFlags(bool darkenSky, bool playEndBossMusic, bool createFog)
{
    u8 flags = 0;
    if (darkenSky) flags |= 0x01;
    if (playEndBossMusic) flags |= 0x02;
    if (createFog) flags |= 0x04;
    return flags;
}

/// 把 ITextComponent 序列化为 1.21.11 组件 opaque 字节（JSON 文本）。
/// TODO(Phase6): 未对齐 1.21.11 ComponentType 前缀树，真互通需补完整 Component codec。
std::vector<u8> bossNameToBytes(const text::ITextComponent& name)
{
    std::string json = name.toJson().dump();
    return std::vector<u8>(json.begin(), json.end());
}

mc::network::ir::IrPacket makePlayPacket(mc::network::ir::play::BossEvent evt)
{
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(evt)},
    };
}

} // namespace

CustomServerBossInfoManager::CustomServerBossInfoManager(IServer& server)
    : m_server(server)
{}

CustomServerBossInfoManager::~CustomServerBossInfoManager() = default;

std::unique_ptr<CustomServerBossInfo> CustomServerBossInfoManager::create(
    const ResourceLocation& id, std::unique_ptr<text::ITextComponent> name)
{
    // 检查 ID 是否已存在
    if (m_bossBars.find(id) != m_bossBars.end()) {
        return nullptr;
    }
    return std::make_unique<CustomServerBossInfo>(id, std::move(name), *this);
}

CustomServerBossInfo* CustomServerBossInfoManager::add(std::unique_ptr<CustomServerBossInfo> bossInfo)
{
    if (!bossInfo) {
        return nullptr;
    }

    const ResourceLocation& id = bossInfo->id();

    // 检查 ID 是否已存在
    if (m_bossBars.find(id) != m_bossBars.end()) {
        return nullptr;
    }

    CustomServerBossInfo* ptr = bossInfo.get();
    m_bossBars[id] = std::move(bossInfo);
    markDirty();

    return ptr;
}

void CustomServerBossInfoManager::remove(CustomServerBossInfo& bossInfo)
{
    auto it = m_bossBars.find(bossInfo.id());
    if (it != m_bossBars.end() && it->second.get() == &bossInfo) {
        // 先移除所有玩家
        bossInfo.removeAllPlayers();
        m_bossBars.erase(it);
        markDirty();
    }
}

CustomServerBossInfo* CustomServerBossInfoManager::get(const ResourceLocation& id)
{
    auto it = m_bossBars.find(id);
    if (it != m_bossBars.end()) {
        return it->second.get();
    }
    return nullptr;
}

const CustomServerBossInfo* CustomServerBossInfoManager::get(const ResourceLocation& id) const
{
    auto it = m_bossBars.find(id);
    if (it != m_bossBars.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<ResourceLocation> CustomServerBossInfoManager::getIds() const
{
    std::vector<ResourceLocation> ids;
    ids.reserve(m_bossBars.size());
    for (const auto& pair : m_bossBars) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<CustomServerBossInfo*> CustomServerBossInfoManager::getBossBars()
{
    std::vector<CustomServerBossInfo*> bars;
    bars.reserve(m_bossBars.size());
    for (auto& pair : m_bossBars) {
        bars.push_back(pair.second.get());
    }
    return bars;
}

void CustomServerBossInfoManager::onPlayerLogin(::mc::ServerPlayer& player)
{
    // 恢复玩家对之前可见的 Boss 栏的可见性
    for (auto& pair : m_bossBars) {
        pair.second->onPlayerLogin(player);
    }
}

void CustomServerBossInfoManager::onPlayerLogout(::mc::ServerPlayer& player)
{
    // 清理玩家的 Boss 栏可见性
    for (auto& pair : m_bossBars) {
        pair.second->onPlayerLogout(player);
    }
}

void CustomServerBossInfoManager::sendAddPacket(CustomServerBossInfo& bossInfo, ::mc::ServerPlayer& player)
{
    mc::network::ir::play::BossEvent evt;
    evt.uuid = bossInfo.uuid();
    evt.operation = 0; // ADD
    evt.name = bossNameToBytes(bossInfo.name());
    evt.progress = bossInfo.percent();
    evt.color = static_cast<i32>(bossInfo.color());
    evt.overlay = static_cast<i32>(bossInfo.overlay());
    evt.flags = packBossFlags(bossInfo.darkenSky(), bossInfo.playEndBossMusic(), bossInfo.createFog());

    m_server.connectionManager().sendToPlayer(player.playerId(), makePlayPacket(std::move(evt)));

    // 标记数据需要保存
    markDirty();
}

void CustomServerBossInfoManager::sendRemovePacket(CustomServerBossInfo& bossInfo, ::mc::ServerPlayer& player)
{
    mc::network::ir::play::BossEvent evt;
    evt.uuid = bossInfo.uuid();
    evt.operation = 1; // REMOVE

    m_server.connectionManager().sendToPlayer(player.playerId(), makePlayPacket(std::move(evt)));

    // 标记数据需要保存
    markDirty();
}

void CustomServerBossInfoManager::broadcastUpdate(CustomServerBossInfo& bossInfo)
{
    // 获取所有可见玩家并发送更新
    const auto& playerIds = bossInfo.players();
    if (playerIds.empty()) {
        return;
    }

    // 获取待发送的更新类型
    BossInfoUpdateType updateType = bossInfo.pendingUpdateType();
    bossInfo.clearPendingUpdate();

    // 如果没有更新，跳过
    if (updateType == BossInfoUpdateType::None) {
        return;
    }

    // 根据更新类型创建不同的更新包
    mc::network::ir::play::BossEvent evt;
    evt.uuid = bossInfo.uuid();
    switch (updateType) {
        case BossInfoUpdateType::UpdatePercent:
            evt.operation = 2; // UPDATE_PROGRESS
            evt.progress = bossInfo.percent();
            break;

        case BossInfoUpdateType::UpdateName:
            evt.operation = 3; // UPDATE_NAME
            evt.name = bossNameToBytes(bossInfo.name());
            break;

        case BossInfoUpdateType::UpdateStyle:
            evt.operation = 4; // UPDATE_STYLE
            evt.color = static_cast<i32>(bossInfo.color());
            evt.overlay = static_cast<i32>(bossInfo.overlay());
            break;

        case BossInfoUpdateType::UpdateProperties:
            evt.operation = 5; // UPDATE_PROPERTIES
            evt.flags = packBossFlags(bossInfo.darkenSky(), bossInfo.playEndBossMusic(), bossInfo.createFog());
            break;

        default:
            // 其他类型默认使用百分比更新
            evt.operation = 2; // UPDATE_PROGRESS
            evt.progress = bossInfo.percent();
            break;
    }

    // 广播给所有可见玩家
    auto packet = makePlayPacket(std::move(evt));
    for (PlayerId playerId : playerIds) {
        m_server.connectionManager().sendToPlayer(playerId, packet);
    }

    // 标记数据需要保存
    markDirty();
}

nbt::tags::compound_tag CustomServerBossInfoManager::toNbt() const
{
    nbt::tags::compound_tag tag;

    for (const auto& pair : m_bossBars) {
        auto bossTag = std::make_unique<nbt::tags::compound_tag>(pair.second->toNbt());
        tag.value.emplace(pair.first.toString(), std::move(bossTag));
    }

    return tag;
}

void CustomServerBossInfoManager::fromNbt(const nbt::tags::compound_tag& nbt)
{
    m_bossBars.clear();

    for (const auto& pair : nbt.value) {
        if (pair.second->id() == nbt::TagId::Compound) {
            ResourceLocation id(pair.first);
            auto bossInfo =
                CustomServerBossInfo::fromNbt(dynamic_cast<const nbt::tags::compound_tag&>(*pair.second), id, *this);
            if (bossInfo) {
                m_bossBars[id] = std::move(bossInfo);
            }
        }
    }

    clearDirty();
}

} // namespace server
} // namespace mc
