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

#include "PointOfInterestStorage.hpp"
#include "../../../util/nbt/Nbt.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/village/poi/PointOfInterest.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace village {
namespace poi {

// ========== POI注册 ==========

bool PointOfInterestStorage::registerPOI(BlockPos pos, PointOfInterestType type)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查是否已存在
    if (m_byPosition.find(pos) != m_byPosition.end()) {
        return false;
    }

    // 创建POI（使用list避免指针失效）
    u64 chunkKey = _getChunkKey(pos);
    auto& chunkPOIs = _getOrCreateChunkPOIs(chunkKey);
    chunkPOIs.emplace_back(pos, type);

    // 更新索引（list迭代器稳定，指针有效）
    PointOfInterest* poi = &chunkPOIs.back();
    m_byPosition[pos] = poi;
    m_byType[type].push_back(poi);

    return true;
}

bool PointOfInterestStorage::unregisterPOI(BlockPos pos)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_byPosition.find(pos);
    if (it == m_byPosition.end()) {
        return false;
    }

    PointOfInterest* poi = it->second;
    PointOfInterestType type = poi->getType();

    // 从类型索引中移除
    auto& typeList = m_byType[type];
    typeList.erase(std::remove(typeList.begin(), typeList.end(), poi), typeList.end());

    // 从位置索引中移除
    m_byPosition.erase(it);

    // 从区块存储中移除（list版本）
    u64 chunkKey = _getChunkKey(pos);
    auto chunkIt = m_chunkPOIs.find(chunkKey);
    if (chunkIt != m_chunkPOIs.end()) {
        auto& chunkPOIs = chunkIt->second;
        chunkPOIs.remove_if([pos](const PointOfInterest& p) { return p.getPosition() == pos; });
    }

    return true;
}

bool PointOfInterestStorage::updatePOI(BlockPos pos, PointOfInterestType newType)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_byPosition.find(pos);
    if (it == m_byPosition.end()) {
        return false;
    }

    PointOfInterest* poi = it->second;
    PointOfInterestType oldType = poi->getType();

    if (oldType == newType) {
        return true; // 无需更新
    }

    // 从旧类型索引中移除
    auto& oldTypeList = m_byType[oldType];
    oldTypeList.erase(std::remove(oldTypeList.begin(), oldTypeList.end(), poi), oldTypeList.end());

    // 更新POI类型（list中元素地址稳定）
    u64 chunkKey = _getChunkKey(pos);
    auto chunkIt = m_chunkPOIs.find(chunkKey);
    if (chunkIt != m_chunkPOIs.end()) {
        for (auto& chunkPoi : chunkIt->second) {
            if (chunkPoi.getPosition() == pos) {
                // 直接更新POI类型
                chunkPoi = PointOfInterest(pos, newType);
                // 指针仍然有效（list特性）
                m_byPosition[pos] = &chunkPoi;
                m_byType[newType].push_back(&chunkPoi);
                return true;
            }
        }
    }

    return false;
}

bool PointOfInterestStorage::hasPOI(BlockPos pos) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_byPosition.find(pos) != m_byPosition.end();
}

const PointOfInterest* PointOfInterestStorage::getPOI(BlockPos pos) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_byPosition.find(pos);
    return it != m_byPosition.end() ? it->second : nullptr;
}

// ========== 占用管理 ==========

bool PointOfInterestStorage::acquirePOI(BlockPos pos, u64 ownerId, i64 gameTime)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_byPosition.find(pos);
    if (it == m_byPosition.end()) {
        return false;
    }

    return it->second->acquire(ownerId, gameTime);
}

bool PointOfInterestStorage::releasePOI(BlockPos pos, u64 ownerId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_byPosition.find(pos);
    if (it == m_byPosition.end()) {
        return false;
    }

    return it->second->release(ownerId);
}

i32 PointOfInterestStorage::releaseAllByOwner(u64 ownerId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    i32 count = 0;
    for (auto& [pos, poi] : m_byPosition) {
        if (poi->isOwnedBy(ownerId)) {
            poi->release(ownerId);
            ++count;
        }
    }
    return count;
}

// ========== 空间查询 ==========

POISearchResult PointOfInterestStorage::findNearest(BlockPos center, const POISearchCriteria& criteria) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    POISearchResult best;
    best.distance = std::numeric_limits<f32>::max();

    // 如果指定了类型，使用类型索引
    if (criteria.type.has_value()) {
        auto typeIt = m_byType.find(criteria.type.value());
        if (typeIt != m_byType.end()) {
            for (const auto* poi : typeIt->second) {
                if (!_matchesCriteria(*poi, criteria)) {
                    continue;
                }

                f32 dist = _distance(center, poi->getPosition());
                if (criteria.maxDistance > 0 && dist > criteria.maxDistance) {
                    continue;
                }
                if (dist < criteria.minDistance) {
                    continue;
                }

                if (dist < best.distance) {
                    best.poi = poi;
                    best.distance = dist;
                }
            }
        }
    }
    // 如果指定了类型列表
    else if (!criteria.types.empty()) {
        for (auto type : criteria.types) {
            auto typeIt = m_byType.find(type);
            if (typeIt != m_byType.end()) {
                for (const auto* poi : typeIt->second) {
                    if (!_matchesCriteria(*poi, criteria)) {
                        continue;
                    }

                    f32 dist = _distance(center, poi->getPosition());
                    if (criteria.maxDistance > 0 && dist > criteria.maxDistance) {
                        continue;
                    }
                    if (dist < criteria.minDistance) {
                        continue;
                    }

                    if (dist < best.distance) {
                        best.poi = poi;
                        best.distance = dist;
                    }
                }
            }
        }
    }
    // 遍历所有POI
    else {
        for (const auto& [pos, poi] : m_byPosition) {
            if (!_matchesCriteria(*poi, criteria)) {
                continue;
            }

            f32 dist = _distance(center, poi->getPosition());
            if (criteria.maxDistance > 0 && dist > criteria.maxDistance) {
                continue;
            }
            if (dist < criteria.minDistance) {
                continue;
            }

            if (dist < best.distance) {
                best.poi = poi;
                best.distance = dist;
            }
        }
    }

    return best;
}

std::optional<BlockPos> PointOfInterestStorage::findNearestFree(
    BlockPos center, PointOfInterestType type, f32 maxDistance) const
{
    POISearchCriteria criteria;
    criteria.type = type;
    criteria.requireUnoccupied = true;
    criteria.maxDistance = maxDistance;

    auto result = findNearest(center, criteria);
    return result.isValid() ? std::optional<BlockPos>(result.poi->getPosition()) : std::nullopt;
}

std::optional<BlockPos> PointOfInterestStorage::findNearest(
    BlockPos center, PointOfInterestType type, f32 maxDistance) const
{
    POISearchCriteria criteria;
    criteria.type = type;
    criteria.maxDistance = maxDistance;

    auto result = findNearest(center, criteria);
    return result.isValid() ? std::optional<BlockPos>(result.poi->getPosition()) : std::nullopt;
}

std::optional<BlockPos> PointOfInterestStorage::findNearestUnacquired(
    BlockPos center, PointOfInterestType type, f32 maxDistance, u64 excludeOwnerId) const
{
    POISearchCriteria criteria;
    criteria.type = type;
    criteria.maxDistance = maxDistance;
    criteria.excludeOwner = excludeOwnerId;

    auto result = findNearest(center, criteria);
    return result.isValid() ? std::optional<BlockPos>(result.poi->getPosition()) : std::nullopt;
}

std::vector<const PointOfInterest*> PointOfInterestStorage::findAllInChunk(
    ChunkCoord chunkX, ChunkCoord chunkZ, std::optional<PointOfInterestType> typeFilter) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<const PointOfInterest*> result;
    u64 chunkKey = _getChunkKey(chunkX, chunkZ);

    auto it = m_chunkPOIs.find(chunkKey);
    if (it == m_chunkPOIs.end()) {
        return result;
    }

    for (const auto& poi : it->second) {
        if (!typeFilter.has_value() || poi.getType() == typeFilter.value()) {
            result.push_back(&poi);
        }
    }

    return result;
}

std::vector<const PointOfInterest*> PointOfInterestStorage::findAllInRange(
    BlockPos center, f32 radius, std::optional<PointOfInterestType> typeFilter) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<const PointOfInterest*> result;

    for (const auto& [pos, poi] : m_byPosition) {
        if (typeFilter.has_value() && poi->getType() != typeFilter.value()) {
            continue;
        }

        f32 dist = _distance(center, poi->getPosition());
        if (dist <= radius) {
            result.push_back(poi);
        }
    }

    return result;
}

std::vector<const PointOfInterest*> PointOfInterestStorage::findAllByType(PointOfInterestType type) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<const PointOfInterest*> result;
    auto it = m_byType.find(type);
    if (it != m_byType.end()) {
        result.reserve(it->second.size());
        for (const auto* poi : it->second) {
            result.push_back(poi);
        }
    }
    return result;
}

// ========== 区块回调 ==========

void PointOfInterestStorage::onChunkLoaded(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_loadedChunks.insert(_getChunkKey(chunkX, chunkZ));
}

void PointOfInterestStorage::onChunkUnloaded(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    u64 chunkKey = _getChunkKey(chunkX, chunkZ);
    m_loadedChunks.erase(chunkKey);

    // 清理该区块的POI索引
    auto it = m_chunkPOIs.find(chunkKey);
    if (it != m_chunkPOIs.end()) {
        for (const auto& poi : it->second) {
            m_byPosition.erase(poi.getPosition());
            auto& typeList = m_byType[poi.getType()];
            typeList.erase(std::remove(typeList.begin(), typeList.end(), &poi), typeList.end());
        }
        m_chunkPOIs.erase(it);
    }
}

// ========== 统计 ==========

size_t PointOfInterestStorage::getTotalCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_byPosition.size();
}

size_t PointOfInterestStorage::getCountByType(PointOfInterestType type) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_byType.find(type);
    return it != m_byType.end() ? it->second.size() : 0;
}

size_t PointOfInterestStorage::getLoadedChunkCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_loadedChunks.size();
}

// ========== 序列化 ==========

void PointOfInterestStorage::serialize(nbt::tags::compound_tag& tag) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto poisList = std::make_unique<nbt::tags::compound_list_tag>();
    for (const auto& [chunkKey, pois] : m_chunkPOIs) {
        for (const auto& poi : pois) {
            nbt::tags::compound_tag poiTag;
            poi.serialize(poiTag);
            poisList->value.push_back(std::move(poiTag));
        }
    }
    tag.value["POIs"] = std::move(poisList);

    // 序列化已加载区块
    auto chunksList = std::make_unique<nbt::tags::long_list_tag>();
    for (u64 chunkKey : m_loadedChunks) {
        chunksList->value.push_back(static_cast<i64>(chunkKey));
    }
    tag.value["LoadedChunks"] = std::move(chunksList);
}

void PointOfInterestStorage::deserialize(const nbt::tags::compound_tag& tag)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 清空现有数据
    m_chunkPOIs.clear();
    m_byPosition.clear();
    m_byType.clear();
    m_loadedChunks.clear();

    // 反序列化POI
    auto poisIt = tag.value.find("POIs");
    if (poisIt != tag.value.end()) {
        auto* poisList = dynamic_cast<const nbt::tags::compound_list_tag*>(poisIt->second.get());
        if (poisList) {
            for (const auto& poiTag : poisList->value) {
                PointOfInterest poi = PointOfInterest::deserialize(poiTag);

                u64 chunkKey = _getChunkKey(poi.getPosition());
                auto& chunkPOIs = m_chunkPOIs[chunkKey];
                chunkPOIs.push_back(std::move(poi));

                // 更新索引
                PointOfInterest* poiPtr = &chunkPOIs.back();
                m_byPosition[poiPtr->getPosition()] = poiPtr;
                m_byType[poiPtr->getType()].push_back(poiPtr);
            }
        }
    }

    // 反序列化已加载区块
    auto chunksIt = tag.value.find("LoadedChunks");
    if (chunksIt != tag.value.end()) {
        auto* chunksList = dynamic_cast<const nbt::tags::long_list_tag*>(chunksIt->second.get());
        if (chunksList) {
            for (i64 chunkKey : chunksList->value) {
                m_loadedChunks.insert(static_cast<u64>(chunkKey));
            }
        }
    }
}

// ========== 私有方法 ==========

u64 PointOfInterestStorage::_getChunkKey(BlockPos pos)
{
    return _getChunkKey(static_cast<ChunkCoord>(pos.x >> CHUNK_SHIFT), static_cast<ChunkCoord>(pos.z >> CHUNK_SHIFT));
}

u64 PointOfInterestStorage::_getChunkKey(ChunkCoord x, ChunkCoord z)
{
    return (static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u64>(static_cast<u32>(z));
}

f32 PointOfInterestStorage::_distance(BlockPos a, BlockPos b)
{
    f32 dx = static_cast<f32>(a.x - b.x);
    f32 dy = static_cast<f32>(a.y - b.y);
    f32 dz = static_cast<f32>(a.z - b.z);
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool PointOfInterestStorage::_matchesCriteria(const PointOfInterest& poi, const POISearchCriteria& criteria) const
{
    // 检查占用状态
    if (criteria.requireUnoccupied && poi.isOccupied()) {
        return false;
    }

    // 检查排除的占用者
    if (criteria.excludeOwner.has_value() && poi.isOwnedBy(criteria.excludeOwner.value())) {
        return false;
    }

    return true;
}

std::list<PointOfInterest>& PointOfInterestStorage::_getOrCreateChunkPOIs(u64 chunkKey)
{
    return m_chunkPOIs[chunkKey];
}

const std::list<PointOfInterest>* PointOfInterestStorage::_getChunkPOIs(u64 chunkKey) const
{
    auto it = m_chunkPOIs.find(chunkKey);
    return it != m_chunkPOIs.end() ? &it->second : nullptr;
}

} // namespace poi
} // namespace village
} // namespace world
} // namespace mc
