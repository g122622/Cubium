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

#include "CustomServerBossInfo.hpp"
#include "CustomServerBossInfoManager.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ComponentUtils.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextEvents.hpp"
#include "common/util/text/TextStyle.hpp"
#include "server/bossbar/BossInfo.hpp"
#include "server/bossbar/ServerBossInfo.hpp"
#include "server/player/ServerPlayer.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace server {

math::Random CustomServerBossInfo::s_random{std::random_device{}()};

CustomServerBossInfo::CustomServerBossInfo(
    const ResourceLocation& id, std::unique_ptr<text::ITextComponent> name, CustomServerBossInfoManager& manager)
    : ServerBossInfo(
          // 使用随机 UUID v4，与 MC Java 的 Mth.createInsecureUUID() 一致
          util::generateRandomUuid(s_random),
          std::move(name),
          BossInfoColor::White,
          BossInfoOverlay::Progress)
    , m_id(id)
    , m_manager(manager)
{
    // 初始百分比为 0
    setPercent(0.0f);
}

void CustomServerBossInfo::setValue(i32 value)
{
    i32 clampedValue = std::clamp(value, 0, m_max);
    if (m_value != clampedValue) {
        m_value = clampedValue;
        // 更新百分比
        f32 percent = (m_max > 0) ? static_cast<f32>(m_value) / static_cast<f32>(m_max) : 0.0f;
        BossInfo::setPercent(percent);
        broadcastUpdate();
    }
}

void CustomServerBossInfo::setMax(i32 max)
{
    i32 clampedMax = std::max(max, 1);
    if (m_max != clampedMax) {
        m_max = clampedMax;
        // 更新百分比
        f32 percent = (m_max > 0) ? static_cast<f32>(m_value) / static_cast<f32>(m_max) : 0.0f;
        BossInfo::setPercent(percent);
        broadcastUpdate();
    }
}

void CustomServerBossInfo::addPlayer(::mc::ServerPlayer& player)
{
    PlayerId playerId = player.playerId();
    if (m_players.insert(playerId).second) {
        // 添加到持久化列表
        m_playerUuids.insert(player.uuid());
        sendAddPacket(player);
    }
}

void CustomServerBossInfo::removePlayer(::mc::ServerPlayer& player)
{
    PlayerId playerId = player.playerId();
    if (m_players.erase(playerId) > 0) {
        // 从持久化列表移除
        m_playerUuids.erase(player.uuid());
        sendRemovePacket(player);
    }
}

void CustomServerBossInfo::addPlayerByUuid(const std::string& playerUuid)
{
    m_playerUuids.insert(playerUuid);
}

void CustomServerBossInfo::removeAllPlayers()
{
    // 发送移除包给所有玩家
    // 注意：这里需要通过 manager 发送，因为我们需要遍历玩家列表
    // 但为了避免循环依赖，我们先清空列表，由调用者负责发送移除包
    m_players.clear();
    m_playerUuids.clear();
}

bool CustomServerBossInfo::setPlayers(const std::vector<::mc::ServerPlayer*>& players)
{
    // 计算要移除的玩家
    std::set<PlayerId> toRemove;
    for (PlayerId pid : m_players) {
        bool found = false;
        for (::mc::ServerPlayer* player : players) {
            if (player && player->playerId() == pid) {
                found = true;
                break;
            }
        }
        if (!found) {
            toRemove.insert(pid);
        }
    }

    // 计算要添加的玩家
    std::vector<::mc::ServerPlayer*> toAdd;
    for (::mc::ServerPlayer* player : players) {
        if (player && m_players.find(player->playerId()) == m_players.end()) {
            toAdd.push_back(player);
        }
    }

    // 如果没有变化，返回 false
    if (toRemove.empty() && toAdd.empty()) {
        return false;
    }

    // 移除玩家
    for (PlayerId pid : toRemove) {
        m_players.erase(pid);
        // 注意：这里无法直接发送移除包，因为没有 ServerPlayer 引用
        // 需要在 Manager 层面处理
    }

    // 添加玩家
    for (::mc::ServerPlayer* player : toAdd) {
        m_players.insert(player->playerId());
        m_playerUuids.insert(player->uuid());
        sendAddPacket(*player);
    }

    // 更新持久化列表
    m_playerUuids.clear();
    for (::mc::ServerPlayer* player : players) {
        if (player) {
            m_playerUuids.insert(player->uuid());
        }
    }

    return true;
}

std::unique_ptr<text::ITextComponent> CustomServerBossInfo::formattedName() const
{
    // 创建带格式的名称
    auto formatted = m_name->deepCopy();

    // 设置颜色
    text::Style style = formatted->getStyle();

    // 根据颜色设置格式
    switch (m_color) {
        case BossInfoColor::Pink:
            style.setColor(text::TextFormatting::Red);
            break;
        case BossInfoColor::Blue:
            style.setColor(text::TextFormatting::Blue);
            break;
        case BossInfoColor::Red:
            style.setColor(text::TextFormatting::DarkRed);
            break;
        case BossInfoColor::Green:
            style.setColor(text::TextFormatting::Green);
            break;
        case BossInfoColor::Yellow:
            style.setColor(text::TextFormatting::Yellow);
            break;
        case BossInfoColor::Purple:
            style.setColor(text::TextFormatting::DarkPurple);
            break;
        case BossInfoColor::White:
        default:
            style.setColor(text::TextFormatting::White);
            break;
    }

    // 设置悬停事件显示 ID
    style.setHoverEvent(text::HoverEvent::showText(m_id.toString()));

    formatted->setStyle(style);

    // 用方括号包裹（使用翻译键 "chat.square_brackets"）
    return text::ComponentUtils::wrapInSquareBrackets(std::move(formatted));
}

void CustomServerBossInfo::onPlayerLogin(::mc::ServerPlayer& player)
{
    // 检查玩家是否在持久化列表中
    if (m_playerUuids.find(player.uuid()) != m_playerUuids.end()) {
        addPlayer(player);
    }
}

void CustomServerBossInfo::onPlayerLogout(::mc::ServerPlayer& player)
{
    // 只从可见列表移除，保留 UUID 记录
    m_players.erase(player.playerId());
    // 注意：不发送移除包，玩家已经断开连接
}

nbt::tags::compound_tag CustomServerBossInfo::toNbt() const
{
    nbt::tags::compound_tag tag;

    // 名称（JSON 格式）
    nlohmann::json nameJson = m_name->toJson();
    tag.put("Name", nameJson.dump());

    // 可见性
    tag.put("Visible", static_cast<i8>(m_visible ? 1 : 0));

    // 值
    tag.put("Value", m_value);

    // 最大值
    tag.put("Max", m_max);

    // 颜色
    tag.put("Color", bossInfoColorToName(m_color));

    // 样式
    tag.put("Overlay", bossInfoOverlayToName(m_overlay));

    // 标志位
    tag.put("DarkenScreen", static_cast<i8>(m_darkenSky ? 1 : 0));
    tag.put("PlayBossMusic", static_cast<i8>(m_playEndBossMusic ? 1 : 0));
    tag.put("CreateWorldFog", static_cast<i8>(m_createFog ? 1 : 0));

    // 玩家 UUID 列表
    nbt::tags::tag_list_tag::value_type playerListValue;
    for (const auto& uuid : m_playerUuids) {
        playerListValue.push_back(std::make_unique<nbt::tags::string_tag>(uuid));
    }
    auto playerList = std::make_unique<nbt::tags::tag_list_tag>(std::move(playerListValue), nbt::TagId::String);
    tag.value.emplace("Players", std::move(playerList));

    return tag;
}

std::unique_ptr<CustomServerBossInfo> CustomServerBossInfo::fromNbt(
    const nbt::tags::compound_tag& nbt, const ResourceLocation& id, CustomServerBossInfoManager& manager)
{
    // 解析名称
    std::unique_ptr<text::ITextComponent> name;
    auto nameIt = nbt.value.find("Name");
    if (nameIt != nbt.value.end() && nameIt->second->id() == nbt::TagId::String) {
        std::string nameJson = dynamic_cast<const nbt::tags::string_tag&>(*nameIt->second).value;
        try {
            nlohmann::json json = nlohmann::json::parse(nameJson);
            name = text::ITextComponent::fromJson(json);
        }
        catch (...) {
            name = std::make_unique<text::StringTextComponent>(id.path());
        }
    }
    if (!name) {
        name = std::make_unique<text::StringTextComponent>(id.path());
    }

    auto bossInfo = std::make_unique<CustomServerBossInfo>(id, std::move(name), manager);

    // 可见性
    auto visibleIt = nbt.value.find("Visible");
    if (visibleIt != nbt.value.end() && visibleIt->second->id() == nbt::TagId::Byte) {
        bossInfo->m_visible = dynamic_cast<const nbt::tags::byte_tag&>(*visibleIt->second).value != 0;
    }

    // 值和最大值
    auto valueIt = nbt.value.find("Value");
    if (valueIt != nbt.value.end() && valueIt->second->id() == nbt::TagId::Int) {
        bossInfo->m_value = dynamic_cast<const nbt::tags::int_tag&>(*valueIt->second).value;
    }

    auto maxIt = nbt.value.find("Max");
    if (maxIt != nbt.value.end() && maxIt->second->id() == nbt::TagId::Int) {
        bossInfo->m_max = std::max(dynamic_cast<const nbt::tags::int_tag&>(*maxIt->second).value, 1);
    }

    // 计算百分比
    bossInfo->m_percent =
        (bossInfo->m_max > 0) ? static_cast<f32>(bossInfo->m_value) / static_cast<f32>(bossInfo->m_max) : 0.0f;

    // 颜色
    auto colorIt = nbt.value.find("Color");
    if (colorIt != nbt.value.end() && colorIt->second->id() == nbt::TagId::String) {
        bossInfo->m_color = bossInfoColorFromName(dynamic_cast<const nbt::tags::string_tag&>(*colorIt->second).value);
    }

    // 样式
    auto overlayIt = nbt.value.find("Overlay");
    if (overlayIt != nbt.value.end() && overlayIt->second->id() == nbt::TagId::String) {
        bossInfo->m_overlay =
            bossInfoOverlayFromName(dynamic_cast<const nbt::tags::string_tag&>(*overlayIt->second).value);
    }

    // 标志位
    auto darkenIt = nbt.value.find("DarkenScreen");
    if (darkenIt != nbt.value.end() && darkenIt->second->id() == nbt::TagId::Byte) {
        bossInfo->m_darkenSky = dynamic_cast<const nbt::tags::byte_tag&>(*darkenIt->second).value != 0;
    }

    auto musicIt = nbt.value.find("PlayBossMusic");
    if (musicIt != nbt.value.end() && musicIt->second->id() == nbt::TagId::Byte) {
        bossInfo->m_playEndBossMusic = dynamic_cast<const nbt::tags::byte_tag&>(*musicIt->second).value != 0;
    }

    auto fogIt = nbt.value.find("CreateWorldFog");
    if (fogIt != nbt.value.end() && fogIt->second->id() == nbt::TagId::Byte) {
        bossInfo->m_createFog = dynamic_cast<const nbt::tags::byte_tag&>(*fogIt->second).value != 0;
    }

    // 玩家 UUID 列表
    auto playersIt = nbt.value.find("Players");
    if (playersIt != nbt.value.end() && playersIt->second->id() == nbt::TagId::List) {
        const auto& playerList = dynamic_cast<const nbt::tags::tag_list_tag&>(*playersIt->second);
        for (size_t i = 0; i < playerList.size(); ++i) {
            auto item = playerList[i];
            if (item && item->id() == nbt::TagId::String) {
                bossInfo->m_playerUuids.insert(dynamic_cast<const nbt::tags::string_tag&>(*item).value);
            }
        }
    }

    return bossInfo;
}

void CustomServerBossInfo::sendAddPacket(::mc::ServerPlayer& player)
{
    // 通过 Manager 发送网络包
    m_manager.sendAddPacket(*this, player);
}

void CustomServerBossInfo::sendRemovePacket(::mc::ServerPlayer& player)
{
    // 通过 Manager 发送网络包
    m_manager.sendRemovePacket(*this, player);
}

void CustomServerBossInfo::broadcastUpdate()
{
    // 通过 Manager 发送更新包
    m_manager.broadcastUpdate(*this);
}

} // namespace server
} // namespace mc
