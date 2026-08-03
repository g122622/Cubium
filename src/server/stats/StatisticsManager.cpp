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

#include "server/stats/StatisticsManager.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "server/stats/StatType.hpp"
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace mc {
namespace server {
namespace stats {

Result<StatisticsManager> StatisticsManager::fromNbt(const nbt::tags::compound_tag& tag)
{
    StatisticsManager manager;

    // NBT 结构：
    // {
    //   "stats": {
    //     "minecraft:mined": { "minecraft:stone": 100, ... },
    //     "minecraft:crafted": { "minecraft:diamond_sword": 5, ... },
    //     ...
    //   }
    // }

    auto statsIt = tag.value.find("stats");
    if (statsIt == tag.value.end()) {
        // 没有统计数据，返回空管理器
        return manager;
    }

    auto* statsTag = dynamic_cast<const nbt::tags::compound_tag*>(statsIt->second.get());
    if (!statsTag) {
        return manager;
    }

    for (const auto& [typeKey, typeValue] : statsTag->value) {
        auto* typeCompound = dynamic_cast<const nbt::tags::compound_tag*>(typeValue.get());
        if (!typeCompound) {
            continue;
        }

        // 解析统计类型
        auto typeOpt = parseStatType(typeKey);
        if (!typeOpt.has_value()) {
            continue;
        }
        StatType type = typeOpt.value();

        // 读取该类型下的所有统计
        for (const auto& [idKey, idValue] : typeCompound->value) {
            auto* intTag = dynamic_cast<const nbt::tags::int_tag*>(idValue.get());
            if (!intTag) {
                // 尝试 long_tag
                auto* longTag = dynamic_cast<const nbt::tags::long_tag*>(idValue.get());
                if (longTag) {
                    ResourceLocation id(idKey);
                    manager.setValue(type, id, static_cast<ValueType>(longTag->value));
                }
                continue;
            }

            ResourceLocation id(idKey);
            manager.setValue(type, id, static_cast<ValueType>(intTag->value));
        }
    }

    manager.m_dirty = false;
    return manager;
}

nbt::tags::compound_tag StatisticsManager::toNbt() const
{
    nbt::tags::compound_tag root;

    // 按类型分组
    std::unordered_map<StatType, std::unordered_map<std::string, ValueType>> groupedStats;

    for (const auto& [fullId, value] : m_stats) {
        if (value == 0) {
            continue; // 跳过零值
        }

        // 解析完整ID获取类型和ID
        // 格式：minecraft.{type}:{id}
        std::string idStr = fullId.toString();

        // 找到类型前缀的起始位置（第一个点之后）
        size_t dotPos = idStr.find('.');
        if (dotPos == std::string::npos) {
            continue;
        }

        // 找到冒号位置（分隔类型和ID）
        size_t colonPos = idStr.find(':', dotPos);
        if (colonPos == std::string::npos) {
            continue;
        }

        std::string typePrefix = idStr.substr(dotPos + 1, colonPos - dotPos - 1);
        auto typeOpt = parseStatType(typePrefix);
        if (!typeOpt.has_value()) {
            continue;
        }

        std::string statId = idStr.substr(colonPos + 1);
        groupedStats[typeOpt.value()][statId] = value;
    }

    // 构建 NBT
    auto statsCompound = std::make_unique<nbt::tags::compound_tag>();

    for (const auto& [type, stats] : groupedStats) {
        auto typeCompound = std::make_unique<nbt::tags::compound_tag>();

        for (const auto& [id, value] : stats) {
            // 使用 long 存储大数值
            auto longTag = std::make_unique<nbt::tags::long_tag>();
            longTag->value = value;
            typeCompound->value.emplace(id, std::move(longTag));
        }

        std::string typeKey(getStatTypePrefix(type));
        statsCompound->value.emplace(typeKey, std::move(typeCompound));
    }

    root.value.emplace("stats", std::move(statsCompound));
    return root;
}

StatisticsManager::ValueType StatisticsManager::getValue(StatType type, const ResourceLocation& id) const
{
    ResourceLocation fullId = buildStatLocation(type, id);
    return getValue(fullId);
}

StatisticsManager::ValueType StatisticsManager::getValue(const ResourceLocation& fullId) const
{
    auto it = m_stats.find(fullId);
    if (it != m_stats.end()) {
        return it->second;
    }
    return 0;
}

void StatisticsManager::setValue(StatType type, const ResourceLocation& id, ValueType value)
{
    ResourceLocation fullId = buildStatLocation(type, id);
    m_stats[fullId] = value;
    m_dirty = true;
}

void StatisticsManager::increment(StatType type, const ResourceLocation& id, ValueType delta)
{
    // 如果增量为0，不创建统计条目
    if (delta == 0) {
        return;
    }

    ResourceLocation fullId = buildStatLocation(type, id);
    ValueType current = getValue(fullId);

    // 防止溢出
    if (delta > 0 && current > std::numeric_limits<ValueType>::max() - delta) {
        m_stats[fullId] = std::numeric_limits<ValueType>::max();
    } else if (delta < 0 && current < std::numeric_limits<ValueType>::min() - delta) {
        m_stats[fullId] = std::numeric_limits<ValueType>::min();
    } else {
        m_stats[fullId] = current + delta;
    }

    m_dirty = true;
}

void StatisticsManager::reset(StatType type, const ResourceLocation& id)
{
    ResourceLocation fullId = buildStatLocation(type, id);
    m_stats.erase(fullId);
    m_dirty = true;
}

void StatisticsManager::resetAll()
{
    m_stats.clear();
    m_dirty = true;
}

bool StatisticsManager::hasStat(StatType type, const ResourceLocation& id) const
{
    ResourceLocation fullId = buildStatLocation(type, id);
    return hasStat(fullId);
}

bool StatisticsManager::hasStat(const ResourceLocation& fullId) const
{
    return m_stats.find(fullId) != m_stats.end();
}

std::unordered_map<ResourceLocation, StatisticsManager::ValueType> StatisticsManager::getAllStats() const
{
    return m_stats;
}

std::unordered_map<ResourceLocation, StatisticsManager::ValueType> StatisticsManager::getStatsByType(
    StatType type) const
{
    std::unordered_map<ResourceLocation, ValueType> result;

    std::string prefix(getStatTypePrefix(type));
    for (const auto& [fullId, value] : m_stats) {
        // 检查是否匹配类型
        std::string idStr = fullId.toString();
        if (idStr.find("minecraft." + prefix + ":") == 0) {
            result[fullId] = value;
        }
    }

    return result;
}

void StatisticsManager::forEach(const std::function<bool(const ResourceLocation&, ValueType)>& callback) const
{
    for (const auto& [id, value] : m_stats) {
        if (!callback(id, value)) {
            break;
        }
    }
}

} // namespace stats
} // namespace server
} // namespace mc
