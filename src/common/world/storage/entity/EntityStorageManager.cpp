#include "EntityStorageManager.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/serialization/EntityDeserializer.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/storage/db/ColumnFamilies.hpp"
#include "common/world/storage/db/RocksDBDatabase.hpp"
#include "spdlog/spdlog.h"

#include <zlib.h>

#include <algorithm>
#include <cmath>
#include <sstream>

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
    key.chunkX = static_cast<ChunkCoord>(std::floor(entity.position().x / 16.0));
    key.chunkZ = static_cast<ChunkCoord>(std::floor(entity.position().z / 16.0));
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
    auto dbKey = makeKey(key);

    // 写入数据库
    return m_db.put(columnFamilyName(dimension), dbKey, binaryResult.value());
}

Result<std::unique_ptr<Entity>> EntityStorageManager::loadEntity(
    const std::string& uuid, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension, IWorld* world)
{
    EntityKey key{chunkX, chunkZ, uuid};
    auto dbKey = makeKey(key);

    auto result = m_db.get(columnFamilyName(dimension), dbKey);
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
    auto dbKey = makeKey(key);
    return m_db.del(columnFamilyName(dimension), dbKey);
}

// ========== 区块级操作 ==========

Result<std::vector<std::unique_ptr<Entity>>> EntityStorageManager::loadEntitiesInChunk(
    ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension, IWorld* world)
{
    std::vector<std::unique_ptr<Entity>> entities;

    auto prefix = makeChunkPrefixKey(chunkX, chunkZ);
    auto endKey = makeChunkEndKey(chunkX, chunkZ);
    const char* cf = columnFamilyName(dimension);

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
    const char* cf = columnFamilyName(dimension);

    for (const auto& entityRef : entities) {
        const Entity& entity = entityRef.get();
        auto result = saveEntity(entity, dimension);
        if (!result.success()) {
            return result;
        }
    }

    return Result<void>::ok();
}

Result<void> EntityStorageManager::deleteEntitiesInChunk(ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    auto startKey = makeChunkPrefixKey(chunkX, chunkZ);
    auto endKey = makeChunkEndKey(chunkX, chunkZ);
    return m_db.deleteRange(columnFamilyName(dimension), startKey, endKey);
}

// ========== 脏数据追踪 ==========

void EntityStorageManager::markDirty(const std::string& entityKey, DimensionId dimension)
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    std::string key = std::to_string(static_cast<i32>(dimension)) + ":" + entityKey;
    m_dirtyEntities.insert(std::move(key));
}

Result<size_t> EntityStorageManager::flushDirty()
{
    // 当前实现不支持脏数据追踪刷新（需要持有实体引用）
    // 将在 ServerWorld 集成时通过区块卸载实现
    return static_cast<size_t>(0);
}

size_t EntityStorageManager::dirtyCount() const
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    return m_dirtyEntities.size();
}

// ========== 私有方法 ==========

const char* EntityStorageManager::columnFamilyName(DimensionId dimension)
{
    return cf::getEntityCF(dimension);
}

std::vector<u8> EntityStorageManager::makeKey(const EntityKey& key)
{
    std::string str = key.toString();
    return std::vector<u8>(str.begin(), str.end());
}

std::vector<u8> EntityStorageManager::makeChunkPrefixKey(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    std::string prefix = EntityKey::buildChunkPrefix(chunkX, chunkZ);
    return std::vector<u8>(prefix.begin(), prefix.end());
}

std::vector<u8> EntityStorageManager::makeChunkEndKey(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    // 区块前缀 + 一个比任何 UUID 都大的字符，确保范围扫描包含整个区块
    std::string prefix = EntityKey::buildChunkPrefix(chunkX, chunkZ);
    prefix.push_back(static_cast<char>(0xFF)); // 结束标记
    return std::vector<u8>(prefix.begin(), prefix.end());
}

} // namespace mc::world::storage
