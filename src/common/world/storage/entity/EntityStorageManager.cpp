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

#include "EntityStorageManager.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "common/entity/core/Entity.hpp"
#include "common/entity/serialization/EntityDeserializer.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/storage/db/ColumnFamilies.hpp"
#include "common/world/storage/db/RocksDBDatabase.hpp"
#include "spdlog/spdlog.h"
#include <zlib.h>

namespace mc::world::storage {

// ============================================================================
// EntityKey
// ============================================================================

std::string EntityKey::toString() const
{
    return std::to_string(chunkX) + ":" + std::to_string(chunkZ) + ":" + uuid;
}

Result<EntityKey> EntityKey::parse(const std::string& str)
{
    // 格式: {chunkX}:{chunkZ}:{uuid}
    auto firstColon = str.find(':');
    if (firstColon == std::string::npos) {
        return Error(ErrorCode::InvalidData, "Invalid entity key format: missing first colon");
    }

    auto secondColon = str.find(':', firstColon + 1);
    if (secondColon == std::string::npos) {
        return Error(ErrorCode::InvalidData, "Invalid entity key format: missing second colon");
    }

    EntityKey key;
    try {
        key.chunkX = static_cast<ChunkCoord>(std::stoi(str.substr(0, firstColon)));
        key.chunkZ = static_cast<ChunkCoord>(std::stoi(str.substr(firstColon + 1, secondColon - firstColon - 1)));
        key.uuid = str.substr(secondColon + 1);
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, fmt::format("Invalid entity key format: {}", e.what()));
    }

    if (key.uuid.empty()) {
        return Error(ErrorCode::InvalidData, "Invalid entity key format: empty UUID");
    }

    return key;
}

EntityKey EntityKey::fromEntity(const Entity& entity)
{
    EntityKey key;
    // 使用 math::toChunkCoord 进行世界坐标到区块坐标的转换，避免硬编码区块尺寸
    key.chunkX = math::toChunkCoord(entity.position().x);
    key.chunkZ = math::toChunkCoord(entity.position().z);
    key.uuid = entity.uuid();
    return key;
}

std::string EntityKey::buildChunkPrefix(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    return std::to_string(chunkX) + ":" + std::to_string(chunkZ) + ":";
}

// ============================================================================
// EntityStorageManager
// ============================================================================

EntityStorageManager::EntityStorageManager(RocksDBDatabase& db)
    : m_db(db)
{}

// ========== 单实体操作 ==========

Result<void> EntityStorageManager::saveEntity(const Entity& entity, DimensionId dimension)
{
    // 序列化实体为压缩 NBT 二进制
    auto binaryResult = entity::serialization::EntityDeserializer::serializeToBinary(entity);
    if (!binaryResult.success()) {
        return binaryResult.error();
    }

    // 计算键
    EntityKey key = EntityKey::fromEntity(entity);
    auto dbKey = _makeKey(key);

    // 写入数据库
    return m_db.put(_columnFamilyName(dimension), dbKey, binaryResult.value());
}

Result<std::unique_ptr<Entity>> EntityStorageManager::loadEntity(
    const std::string& uuid, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension, IWorld* world)
{
    EntityKey key{chunkX, chunkZ, uuid};
    auto dbKey = _makeKey(key);

    auto result = m_db.get(_columnFamilyName(dimension), dbKey);
    if (!result.success()) {
        return result.error();
    }

    auto& data = result.value();
    if (data.empty()) {
        return std::unique_ptr<Entity>(nullptr);
    }

    return entity::serialization::EntityDeserializer::deserializeFromBinary(data, world);
}

Result<void> EntityStorageManager::deleteEntity(
    const std::string& uuid, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    EntityKey key{chunkX, chunkZ, uuid};
    auto dbKey = _makeKey(key);
    return m_db.del(_columnFamilyName(dimension), dbKey);
}

// ========== 区块级操作 ==========

Result<std::vector<std::unique_ptr<Entity>>> EntityStorageManager::loadEntitiesInChunk(
    ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension, IWorld* world)
{
    std::vector<std::unique_ptr<Entity>> entities;

    auto prefix = _makeChunkPrefixKey(chunkX, chunkZ);
    auto endKey = _makeChunkEndKey(chunkX, chunkZ);
    const char* cf = _columnFamilyName(dimension);

    auto iter = m_db.newIterator(cf);
    if (!iter) {
        return entities;
    }

    iter->Seek(rocksdb::Slice(reinterpret_cast<const char*>(prefix.data()), prefix.size()));

    while (iter->Valid()) {
        auto currentKey = iter->key();

        // 检查是否仍在区块范围内
        if (currentKey.size() >= endKey.size() && std::memcmp(currentKey.data(), endKey.data(), endKey.size()) >= 0) {
            break;
        }

        // 读取并反序列化实体
        auto value = iter->value();
        std::vector<u8> data(value.data(), value.data() + value.size());

        auto entityResult = entity::serialization::EntityDeserializer::deserializeFromBinary(data, world);
        if (entityResult.success() && entityResult.value() != nullptr) {
            entities.push_back(entityResult.value());
        } else {
            spdlog::warn("EntityStorageManager: Failed to deserialize entity in chunk ({}, {})", chunkX, chunkZ);
        }

        iter->Next();
    }

    return entities;
}

Result<void> EntityStorageManager::saveEntitiesInChunk(const std::vector<std::reference_wrapper<Entity>>& entities,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    DimensionId dimension)
{
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);

    for (const auto& entityRef : entities) {
        const Entity& entity = entityRef.get();
        auto result = saveEntity(entity, dimension);
        if (!result.success()) {
            return result;
        }
    }

    return Result<void>::ok();
}

Result<size_t> EntityStorageManager::saveAllEntities(
    const std::vector<std::reference_wrapper<Entity>>& entities, DimensionId dimension)
{
    size_t savedCount = 0;
    for (const auto& entityRef : entities) {
        const Entity& entity = entityRef.get();
        auto result = saveEntity(entity, dimension);
        if (result.failed()) {
            return result.error();
        }
        ++savedCount;
    }

    return savedCount;
}

Result<void> EntityStorageManager::deleteEntitiesInChunk(ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    auto startKey = _makeChunkPrefixKey(chunkX, chunkZ);
    auto endKey = _makeChunkEndKey(chunkX, chunkZ);
    return m_db.deleteRange(_columnFamilyName(dimension), startKey, endKey);
}

// ========== 私有方法 ==========

const char* EntityStorageManager::_columnFamilyName(DimensionId dimension)
{
    return cf::getEntityCF(dimension);
}

std::vector<u8> EntityStorageManager::_makeKey(const EntityKey& key)
{
    std::string str = key.toString();
    return std::vector<u8>(str.begin(), str.end());
}

std::vector<u8> EntityStorageManager::_makeChunkPrefixKey(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    std::string prefix = EntityKey::buildChunkPrefix(chunkX, chunkZ);
    return std::vector<u8>(prefix.begin(), prefix.end());
}

std::vector<u8> EntityStorageManager::_makeChunkEndKey(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    // 区块前缀 + 一个比任何 UUID 都大的字符，确保范围扫描包含整个区块
    std::string prefix = EntityKey::buildChunkPrefix(chunkX, chunkZ);
    prefix.push_back(static_cast<char>(0xFF)); // 结束标记
    return std::vector<u8>(prefix.begin(), prefix.end());
}

} // namespace mc::world::storage
