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

#include "VillageGossip.hpp"
#include "../../util/nbt/Nbt.hpp"
#include "common/core/Types.hpp"
#include "common/world/village/VillageGossipType.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace village {

// ========== GossipEntry ==========

i32 GossipEntry::decay(i64 currentTime)
{
    i64 interval = GossipTypeHelper::getDecayInterval(type);
    f32 rate = GossipTypeHelper::getDecayRate(type);

    // 计算衰减次数
    i64 timePassed = currentTime - lastUpdateTime;
    i32 decayCount = static_cast<i32>(timePassed / interval);

    if (decayCount > 0) {
        // 应用衰减
        for (i32 i = 0; i < decayCount && value > 0; ++i) {
            value = static_cast<i32>(std::floor(value * rate));
        }
        lastUpdateTime = currentTime;
    }

    return value;
}

// ========== VillageGossipManager ==========

void VillageGossipManager::addGossip(u64 playerId, VillageGossipType type, i32 value)
{
    auto& gossips = m_gossips[playerId];

    // 查找是否已有该类型的流言
    auto it =
        std::find_if(gossips.begin(), gossips.end(), [type](const GossipEntry& entry) { return entry.type == type; });

    if (it != gossips.end()) {
        // 累加，不超过最大值
        i32 maxVal = GossipTypeHelper::getMaxValue(type);
        it->value = std::min(it->value + value, maxVal);
        it->lastUpdateTime = 0; // 重置衰减计时
    } else {
        // 新增
        GossipEntry entry;
        entry.type = type;
        entry.value = std::min(value, GossipTypeHelper::getMaxValue(type));
        entry.lastUpdateTime = 0;
        gossips.push_back(entry);
    }
}

void VillageGossipManager::removeGossip(u64 playerId, VillageGossipType type)
{
    auto it = m_gossips.find(playerId);
    if (it != m_gossips.end()) {
        auto& gossips = it->second;
        gossips.erase(
            std::remove_if(
                gossips.begin(), gossips.end(), [type](const GossipEntry& entry) { return entry.type == type; }),
            gossips.end());

        // 如果玩家没有流言了，移除记录
        if (gossips.empty()) {
            m_gossips.erase(it);
        }
    }
}

void VillageGossipManager::clearGossip(u64 playerId)
{
    m_gossips.erase(playerId);
}

void VillageGossipManager::clearAll()
{
    m_gossips.clear();
}

i32 VillageGossipManager::getReputation(u64 playerId) const
{
    auto it = m_gossips.find(playerId);
    if (it == m_gossips.end()) {
        return 0;
    }

    i32 reputation = 0;
    for (const auto& entry : it->second) {
        reputation += GossipTypeHelper::getReputationImpact(entry.type) * entry.value;
    }

    // 限制在范围内
    return std::clamp(reputation, MIN_REPUTATION, MAX_REPUTATION);
}

i32 VillageGossipManager::getGossipValue(u64 playerId, VillageGossipType type) const
{
    auto it = m_gossips.find(playerId);
    if (it == m_gossips.end()) {
        return 0;
    }

    for (const auto& entry : it->second) {
        if (entry.type == type) {
            return entry.value;
        }
    }
    return 0;
}

f32 VillageGossipManager::getPriceModifier(u64 playerId) const
{
    i32 reputation = getReputation(playerId);

    // 价格修正因子：声誉越高，价格越低
    // reputation = -1000 → modifier = 1.5 (价格提高50%)
    // reputation = 0     → modifier = 1.0 (价格不变)
    // reputation = +1000 → modifier = 0.5 (价格降低50%)
    f32 modifier = 1.0f - static_cast<f32>(reputation) / 1000.0f;

    // 限制在0.5-1.5范围
    return std::clamp(modifier, 0.5f, 1.5f);
}

bool VillageGossipManager::hasGossip(u64 playerId) const
{
    return m_gossips.find(playerId) != m_gossips.end();
}

std::vector<u64> VillageGossipManager::getAllPlayers() const
{
    std::vector<u64> players;
    players.reserve(m_gossips.size());
    for (const auto& [playerId, _] : m_gossips) {
        players.push_back(playerId);
    }
    return players;
}

void VillageGossipManager::tick(i64 gameTime)
{
    // 对所有流言执行衰减
    for (auto& [playerId, gossips] : m_gossips) {
        for (auto& entry : gossips) {
            entry.decay(gameTime);
        }

        // 移除值为0的流言
        gossips.erase(
            std::remove_if(gossips.begin(), gossips.end(), [](const GossipEntry& entry) { return entry.value <= 0; }),
            gossips.end());
    }

    // 移除空记录
    for (auto it = m_gossips.begin(); it != m_gossips.end();) {
        if (it->second.empty()) {
            it = m_gossips.erase(it);
        } else {
            ++it;
        }
    }
}

void VillageGossipManager::serialize(nbt::tags::compound_tag& tag) const
{
    auto gossipsList = std::make_unique<nbt::tags::compound_list_tag>();

    for (const auto& [playerId, entries] : m_gossips) {
        nbt::tags::compound_tag playerTag;
        playerTag.put("PlayerId", static_cast<std::int64_t>(playerId));

        auto entriesList = std::make_unique<nbt::tags::compound_list_tag>();
        for (const auto& entry : entries) {
            nbt::tags::compound_tag entryTag;
            entryTag.put("Type", static_cast<std::int32_t>(entry.type));
            entryTag.put("Value", static_cast<std::int32_t>(entry.value));
            entryTag.put("LastUpdateTime", static_cast<std::int64_t>(entry.lastUpdateTime));
            entriesList->value.push_back(std::move(entryTag));
        }
        playerTag.value["Entries"] = std::move(entriesList);

        gossipsList->value.push_back(std::move(playerTag));
    }

    tag.value["Gossips"] = std::move(gossipsList);
}

void VillageGossipManager::deserialize(const nbt::tags::compound_tag& tag)
{
    m_gossips.clear();

    auto gossipsIt = tag.value.find("Gossips");
    if (gossipsIt == tag.value.end()) {
        return;
    }

    auto* gossipsList = dynamic_cast<const nbt::tags::compound_list_tag*>(gossipsIt->second.get());
    if (!gossipsList) {
        return;
    }

    for (const auto& playerTag : gossipsList->value) {
        u64 playerId = static_cast<u64>(playerTag.get<nbt::tags::long_tag>("PlayerId"));

        auto entriesIt = playerTag.value.find("Entries");
        if (entriesIt != playerTag.value.end()) {
            auto* entriesList = dynamic_cast<const nbt::tags::compound_list_tag*>(entriesIt->second.get());
            if (entriesList) {
                std::vector<GossipEntry> entries;
                entries.reserve(entriesList->value.size());

                for (const auto& entryTag : entriesList->value) {
                    GossipEntry entry;
                    entry.type = static_cast<VillageGossipType>(entryTag.get<nbt::tags::int_tag>("Type"));
                    entry.value = entryTag.get<nbt::tags::int_tag>("Value");
                    entry.lastUpdateTime = entryTag.get<nbt::tags::long_tag>("LastUpdateTime");
                    entries.push_back(entry);
                }

                m_gossips[playerId] = std::move(entries);
            }
        }
    }
}

} // namespace village
} // namespace world
} // namespace mc
