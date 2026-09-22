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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/serialization/EntityDeserializer.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/WorldConstants.hpp"
#include "server/world/storage/db/ColumnFamilies.hpp"
#include "server/world/storage/db/RocksDBDatabase.hpp"
#include "server/world/storage/entity/EntityKey.hpp"
#include "spdlog/spdlog.h"
#include <cstddef>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <rocksdb/slice.h>
#include <rocksdb/write_batch.h>

using namespace mc::trace;

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

Result<void> EntityStorageManager::saveEntity(
    const Entity& entity, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    MC_ASSERT_RELEASE_MSG(
        isValidChunkCoord(chunkX, chunkZ), "EntityStorageManager::saveEntity: chunk coordinate out of world bounds");

    // 乘客不单独落盘：乘客作为载具 Passengers 标签的一部分被递归序列化，
    // 参考 MC Java: Entity.save → isPassenger 时返回 false，不写入顶层 Entities 列表。
    // 若乘客也单独保存，会在区块中产生冗余记录，反序列化时与载具的 Passengers 重复 spawn。
    if (entity.isRiding()) {
        return Result<void>::ok();
    }

    // 序列化实体为压缩 NBT 二进制
    auto binaryResult = entity::serialization::EntityDeserializer::serializeToBinary(entity);
    if (!binaryResult.success()) {
        return binaryResult.error();
    }

    return m_db.put(_columnFamilyName(dimension), _makeEntityDbKey(entity, chunkX, chunkZ), binaryResult.value());
}

Result<std::unique_ptr<Entity>> EntityStorageManager::loadEntity(
    const std::string& uuid, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension, ecs::EntityRegistry& registry)
{
    MC_ASSERT_RELEASE_MSG(!uuid.empty(), "EntityStorageManager::loadEntity: entity UUID must not be empty");
    MC_ASSERT_RELEASE_MSG(
        isValidChunkCoord(chunkX, chunkZ), "EntityStorageManager::loadEntity: chunk coordinate out of world bounds");

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

    return entity::serialization::EntityDeserializer::deserializeFromBinary(data, registry);
}

Result<void> EntityStorageManager::deleteEntity(
    const std::string& uuid, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    MC_ASSERT_RELEASE_MSG(!uuid.empty(), "EntityStorageManager::deleteEntity: entity UUID must not be empty");
    MC_ASSERT_RELEASE_MSG(
        isValidChunkCoord(chunkX, chunkZ), "EntityStorageManager::deleteEntity: chunk coordinate out of world bounds");

    EntityKey key{chunkX, chunkZ, uuid};
    auto dbKey = _makeKey(key);
    return m_db.del(_columnFamilyName(dimension), dbKey);
}

// ========== 区块级操作 ==========

Result<std::vector<std::unique_ptr<Entity>>> EntityStorageManager::loadEntitiesInChunk(
    ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension, ecs::EntityRegistry& registry)
{
    MC_ASSERT_RELEASE_MSG(isValidChunkCoord(chunkX, chunkZ),
        "EntityStorageManager::loadEntitiesInChunk: chunk coordinate out of world bounds");

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

        auto entityResult = entity::serialization::EntityDeserializer::deserializeFromBinary(data, registry);
        if (!entityResult.success()) {
            spdlog::warn("EntityStorageManager: Failed to deserialize entity in chunk ({}, {})", chunkX, chunkZ);
        } else {
            // 注意：Result<unique_ptr<T>>::value() 按值返回（内部 takeValue 会把 m_value
            // 置空），因此每次调用都"取走"实体。绝不能在 != nullptr 检查里调用一次、
            // 再在 push_back 里调用第二次——第二次会取到空 unique_ptr，导致 vector 里
            // 混入空指针，下游 value()[0]->x() 之类解引用崩溃。这里先取出实体再判空。
            auto entity = entityResult.value();
            if (entity != nullptr) {
                entities.push_back(std::move(entity));
            } else {
                spdlog::warn("EntityStorageManager: Failed to deserialize entity in chunk ({}, {})", chunkX, chunkZ);
            }
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
    MC_ASSERT_RELEASE_MSG(isValidChunkCoord(chunkX, chunkZ),
        "EntityStorageManager::saveEntitiesInChunk: chunk coordinate out of world bounds");

    // 所有实体一律落在传入的区块坐标下，与调用方枚举它们的区块列、以及
    // deleteEntitiesInChunk 的删除范围保持同一口径。
    for (const auto& entityRef : entities) {
        const Entity& entity = entityRef.get();
        // 乘客不单独落盘（详见 saveEntity 注释）
        if (entity.isRiding()) {
            continue;
        }
        auto result = saveEntity(entity, chunkX, chunkZ, dimension);
        if (!result.success()) {
            return result;
        }
    }

    return Result<void>::ok();
}

Result<void> EntityStorageManager::deleteEntitiesInChunk(ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    MC_ASSERT_RELEASE_MSG(isValidChunkCoord(chunkX, chunkZ),
        "EntityStorageManager::deleteEntitiesInChunk: chunk coordinate out of world bounds");

    auto startKey = _makeChunkPrefixKey(chunkX, chunkZ);
    auto endKey = _makeChunkEndKey(chunkX, chunkZ);
    return m_db.deleteRange(_columnFamilyName(dimension), startKey, endKey);
}

Result<void> EntityStorageManager::replaceEntitiesInChunks(
    const std::vector<ChunkEntityWrite>& writes, DimensionId dimension, bool sync)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Storage.Db, "EntityStorageManager::replaceEntitiesInChunks", "chunks", writes.size(), "sync", sync);

    if (writes.empty()) {
        return Result<void>::ok();
    }

    const char* cfName = _columnFamilyName(dimension);
    auto* cf = m_db.getCF(cfName);
    if (cf == nullptr) {
        return Error(ErrorCode::InvalidState, fmt::format("Column family not found: {}", cfName));
    }

    // 同一批次内区块坐标重复会让后一条的 DeleteRange 把前一条刚写入的实体一并抹掉，
    // 是静默丢数据的来源，必须挡住。顺带校验坐标本身合法。
    {
        std::unordered_set<u64> seenChunks;
        seenChunks.reserve(writes.size());
        for (const ChunkEntityWrite& write : writes) {
            MC_ASSERT_RELEASE_MSG(isValidChunkCoord(write.chunkX, write.chunkZ),
                "EntityStorageManager::replaceEntitiesInChunks: chunk coordinate out of world bounds");

            const u64 packed = (static_cast<u64>(static_cast<u32>(write.chunkX)) << 32) |
                static_cast<u64>(static_cast<u32>(write.chunkZ));
            const bool inserted = seenChunks.insert(packed).second;
            MC_ASSERT_RELEASE_MSG(
                inserted, "EntityStorageManager::replaceEntitiesInChunks: duplicate chunk coordinate in batch");
        }
    }

    rocksdb::WriteBatch batch;

    for (const ChunkEntityWrite& write : writes) {
        // 先放删除、再放写入。WriteBatch 内部按插入顺序分配递增的序列号，因此同一个批里
        // 后插入的 Put 不会被先插入的 RangeTombstone 覆盖，结果与"先调 deleteEntitiesInChunk、
        // 再逐条 saveEntity"完全一致——但只产生一次 WAL 写入。
        auto startKey = _makeChunkPrefixKey(write.chunkX, write.chunkZ);
        auto endKey = _makeChunkEndKey(write.chunkX, write.chunkZ);
        batch.DeleteRange(cf,
            rocksdb::Slice(reinterpret_cast<const char*>(startKey.data()), startKey.size()),
            rocksdb::Slice(reinterpret_cast<const char*>(endKey.data()), endKey.size()));

        for (const auto& entityRef : write.entities) {
            const Entity& entity = entityRef.get();
            // 乘客不单独落盘（详见 saveEntity 注释）
            if (entity.isRiding()) {
                continue;
            }

            auto binaryResult = entity::serialization::EntityDeserializer::serializeToBinary(entity);
            if (!binaryResult.success()) {
                return binaryResult.error();
            }

            auto dbKey = _makeEntityDbKey(entity, write.chunkX, write.chunkZ);

            // 【本接口的核心不变量】实体的落键必须落在本条目自己的删除前缀之内。
            // 一旦越界，这条实体写进去就永远删不掉，会在同一 UUID 上不断堆积副本；
            // 而这正是本批次"整段删除 + 写入"能收敛的前提。
            MC_ASSERT_RELEASE_MSG(
                dbKey.size() > startKey.size() && std::memcmp(dbKey.data(), startKey.data(), startKey.size()) == 0,
                "EntityStorageManager::replaceEntitiesInChunks: entity key falls outside its chunk prefix");

            const auto& valueBytes = binaryResult.value();
            batch.Put(cf,
                rocksdb::Slice(reinterpret_cast<const char*>(dbKey.data()), dbKey.size()),
                rocksdb::Slice(reinterpret_cast<const char*>(valueBytes.data()), valueBytes.size()));
        }
    }

    // 每个条目至少贡献一条 DeleteRange，因此条目数只可能多于区块数。
    MC_ASSERT_RELEASE(batch.Count() >= writes.size());

    return m_db.writeBatch(batch, sync);
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

std::vector<u8> EntityStorageManager::_makeEntityDbKey(const Entity& entity, ChunkCoord chunkX, ChunkCoord chunkZ)
{
    EntityKey key;
    key.chunkX = chunkX;
    key.chunkZ = chunkZ;
    key.uuid = entity.uuid();
    MC_ASSERT_RELEASE_MSG(!key.uuid.empty(), "EntityStorageManager: refusing to persist an entity with an empty UUID");

    return _makeKey(key);
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
