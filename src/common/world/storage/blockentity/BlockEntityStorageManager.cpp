#include "BlockEntityStorageManager.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/core/BlockEntityDeserializer.hpp"
#include "common/world/storage/db/ColumnFamilies.hpp"
#include "common/world/storage/db/RocksDBDatabase.hpp"
#include "spdlog/spdlog.h"

namespace mc::world::storage {

// ============================================================================
// BlockEntityStorageManager
// ============================================================================

BlockEntityStorageManager::BlockEntityStorageManager(RocksDBDatabase& db)
    : m_db(db)
{}

// ========== 单方块实体操作 ==========

Result<void> BlockEntityStorageManager::saveBlockEntity(const BlockEntity& blockEntity, DimensionId dimension)
{
    auto binaryResult = blockentity::BlockEntityDeserializer::serializeListToBinary({std::cref(blockEntity)});
    if (!binaryResult.success()) {
        return binaryResult.error();
    }

    BlockPos pos = blockEntity.getPos();
    auto chunkX = static_cast<ChunkCoord>(std::floor(static_cast<f64>(pos.x) / 16.0));
    auto chunkZ = static_cast<ChunkCoord>(std::floor(static_cast<f64>(pos.z) / 16.0));
    auto key = buildKey(pos, chunkX, chunkZ);
    auto dbKey = makeKey(key);

    return m_db.put(columnFamilyName(dimension), dbKey, binaryResult.value());
}

Result<std::unique_ptr<BlockEntity>> BlockEntityStorageManager::loadBlockEntity(
    const BlockPos& pos, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    auto key = buildKey(pos, chunkX, chunkZ);
    auto dbKey = makeKey(key);

    auto result = m_db.get(columnFamilyName(dimension), dbKey);
    if (!result.success()) {
        return result.error();
    }

    auto& data = result.value();
    if (data.empty()) {
        return std::unique_ptr<BlockEntity>(nullptr);
    }

    auto listResult = blockentity::BlockEntityDeserializer::deserializeListFromBinary(data);
    if (!listResult.success()) {
        return listResult.error();
    }

    auto& entities = listResult.value();
    if (entities.empty()) {
        return std::unique_ptr<BlockEntity>(nullptr);
    }

    return std::move(entities[0]);
}

Result<void> BlockEntityStorageManager::deleteBlockEntity(
    const BlockPos& pos, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    auto key = buildKey(pos, chunkX, chunkZ);
    auto dbKey = makeKey(key);
    return m_db.del(columnFamilyName(dimension), dbKey);
}

// ========== 区块级操作 ==========

Result<std::vector<std::unique_ptr<BlockEntity>>> BlockEntityStorageManager::loadBlockEntitiesInChunk(
    ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    std::vector<std::unique_ptr<BlockEntity>> blockEntities;

    auto prefix = makeChunkPrefixKey(chunkX, chunkZ);
    auto endKey = makeChunkEndKey(chunkX, chunkZ);
    const char* cf = columnFamilyName(dimension);

    auto iter = m_db.newIterator(cf);
    if (!iter) {
        return blockEntities;
    }

    iter->Seek(rocksdb::Slice(reinterpret_cast<const char*>(prefix.data()), prefix.size()));

    while (iter->Valid()) {
        auto currentKey = iter->key();

        if (currentKey.size() >= endKey.size() && std::memcmp(currentKey.data(), endKey.data(), endKey.size()) >= 0) {
            break;
        }

        auto value = iter->value();
        std::vector<u8> data(value.data(), value.data() + value.size());

        auto listResult = blockentity::BlockEntityDeserializer::deserializeListFromBinary(data);
        if (listResult.success()) {
            for (auto& entity : listResult.value()) {
                if (entity != nullptr) {
                    blockEntities.push_back(std::move(entity));
                }
            }
        } else {
            spdlog::warn(
                "BlockEntityStorageManager: Failed to deserialize block entity in chunk ({}, {})", chunkX, chunkZ);
        }

        iter->Next();
    }

    return blockEntities;
}

Result<void> BlockEntityStorageManager::saveBlockEntitiesInChunk(
    const std::vector<std::reference_wrapper<const BlockEntity>>& blockEntities,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    DimensionId dimension)
{
    for (const auto& entityRef : blockEntities) {
        const BlockEntity& entity = entityRef.get();
        auto result = saveBlockEntity(entity, dimension);
        if (!result.success()) {
            return result;
        }
    }

    return Result<void>::ok();
}

Result<void> BlockEntityStorageManager::deleteBlockEntitiesInChunk(
    ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    auto startKey = makeChunkPrefixKey(chunkX, chunkZ);
    auto endKey = makeChunkEndKey(chunkX, chunkZ);
    return m_db.deleteRange(columnFamilyName(dimension), startKey, endKey);
}

// ========== 脏数据追踪 ==========

void BlockEntityStorageManager::markDirty(const std::string& blockEntityKey, DimensionId dimension)
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    std::string key = std::to_string(static_cast<i32>(dimension)) + ":" + blockEntityKey;
    m_dirtyBlockEntities.insert(std::move(key));
}

Result<size_t> BlockEntityStorageManager::flushDirty()
{
    return static_cast<size_t>(0);
}

size_t BlockEntityStorageManager::dirtyCount() const
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    return m_dirtyBlockEntities.size();
}

// ========== 私有方法 ==========

const char* BlockEntityStorageManager::columnFamilyName(DimensionId dimension)
{
    return cf::getBlockEntityCF(dimension);
}

std::string BlockEntityStorageManager::buildKey(const BlockPos& pos, ChunkCoord chunkX, ChunkCoord chunkZ)
{
    return std::to_string(chunkX) + ":" + std::to_string(chunkZ) + ":" + std::to_string(pos.x) + ":" +
        std::to_string(pos.y) + ":" + std::to_string(pos.z);
}

std::vector<u8> BlockEntityStorageManager::makeKey(const std::string& key)
{
    return std::vector<u8>(key.begin(), key.end());
}

std::vector<u8> BlockEntityStorageManager::makeChunkPrefixKey(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    std::string prefix = std::to_string(chunkX) + ":" + std::to_string(chunkZ) + ":";
    return std::vector<u8>(prefix.begin(), prefix.end());
}

std::vector<u8> BlockEntityStorageManager::makeChunkEndKey(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    std::string prefix = std::to_string(chunkX) + ":" + std::to_string(chunkZ) + ":";
    prefix.push_back(static_cast<char>(0xFF));
    return std::vector<u8>(prefix.begin(), prefix.end());
}

} // namespace mc::world::storage
