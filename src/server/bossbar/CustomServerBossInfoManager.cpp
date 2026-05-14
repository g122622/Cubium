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
#include "server/player/ServerPlayer.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include <algorithm>

namespace mc {
namespace server {

CustomServerBossInfoManager::CustomServerBossInfoManager(IServer& server)
    : m_server(server)
{
}

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
    // 发送 Boss 栏添加包
    // 格式: UUID + 名称 + 百分比 + 颜色 + 样式 + 标志位
    // 由于 BossInfoPacket 尚未实现，这里暂时发送一个简单的消息给玩家
    // TODO: 实现 BossInfoPacket 后替换为实际的网络包发送

    // 暂时通过聊天消息通知（后续会替换为真正的 Boss 栏网络同步）
    // player.sendSystemMessage("[BossBar] Added: " + bossInfo.id().toString());
    MC_UNUSED(bossInfo);
    MC_UNUSED(player);

    // 标记数据需要保存
    markDirty();
}

void CustomServerBossInfoManager::sendRemovePacket(CustomServerBossInfo& bossInfo, ::mc::ServerPlayer& player)
{
    // 发送 Boss 栏移除包
    // 格式: UUID
    // TODO: 实现 BossInfoPacket 后替换为实际的网络包发送

    MC_UNUSED(bossInfo);
    MC_UNUSED(player);

    // 标记数据需要保存
    markDirty();
}

void CustomServerBossInfoManager::broadcastUpdate(CustomServerBossInfo& bossInfo)
{
    // 广播 Boss 栏更新给所有可见玩家
    // TODO: 实现 BossInfoPacket 后替换为实际的网络包发送

    MC_UNUSED(bossInfo);

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
