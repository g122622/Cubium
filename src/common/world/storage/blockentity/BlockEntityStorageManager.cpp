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

#include "BlockEntityStorageManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/core/BlockEntityDeserializer.hpp"
#include "common/world/storage/db/ColumnFamilies.hpp"
#include "common/world/storage/db/RocksDBDatabase.hpp"
#include "spdlog/spdlog.h"
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <rocksdb/slice.h>

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
    auto chunkX =
        static_cast<ChunkCoord>(std::floor(static_cast<f64>(pos.x) / static_cast<f64>(mc::world::CHUNK_WIDTH)));
    auto chunkZ =
        static_cast<ChunkCoord>(std::floor(static_cast<f64>(pos.z) / static_cast<f64>(mc::world::CHUNK_WIDTH)));
    auto key = _buildKey(pos, chunkX, chunkZ);
    auto dbKey = _makeKey(key);

    return m_db.put(_columnFamilyName(dimension), dbKey, binaryResult.value());
}

Result<std::unique_ptr<BlockEntity>> BlockEntityStorageManager::loadBlockEntity(
    const BlockPos& pos, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    auto key = _buildKey(pos, chunkX, chunkZ);
    auto dbKey = _makeKey(key);

    auto result = m_db.get(_columnFamilyName(dimension), dbKey);
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
    auto key = _buildKey(pos, chunkX, chunkZ);
    auto dbKey = _makeKey(key);
    return m_db.del(_columnFamilyName(dimension), dbKey);
}

// ========== 区块级操作 ==========

Result<std::vector<std::unique_ptr<BlockEntity>>> BlockEntityStorageManager::loadBlockEntitiesInChunk(
    ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    std::vector<std::unique_ptr<BlockEntity>> blockEntities;

    auto prefix = _makeChunkPrefixKey(chunkX, chunkZ);
    auto endKey = _makeChunkEndKey(chunkX, chunkZ);
    const char* cf = _columnFamilyName(dimension);

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
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);

    for (const auto& entityRef : blockEntities) {
        const BlockEntity& entity = entityRef.get();
        auto result = saveBlockEntity(entity, dimension);
        if (!result.success()) {
            return result;
        }
    }

    return Result<void>::ok();
}

Result<size_t> BlockEntityStorageManager::saveAllBlockEntities(
    const std::vector<std::reference_wrapper<const BlockEntity>>& blockEntities, DimensionId dimension)
{
    size_t savedCount = 0;
    for (const auto& entityRef : blockEntities) {
        const BlockEntity& entity = entityRef.get();
        auto result = saveBlockEntity(entity, dimension);
        if (result.failed()) {
            return result.error();
        }
        ++savedCount;
    }

    return savedCount;
}

Result<void> BlockEntityStorageManager::deleteBlockEntitiesInChunk(
    ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension)
{
    auto startKey = _makeChunkPrefixKey(chunkX, chunkZ);
    auto endKey = _makeChunkEndKey(chunkX, chunkZ);
    return m_db.deleteRange(_columnFamilyName(dimension), startKey, endKey);
}

// ========== 私有方法 ==========

const char* BlockEntityStorageManager::_columnFamilyName(DimensionId dimension)
{
    return cf::getBlockEntityCF(dimension);
}

std::string BlockEntityStorageManager::_buildKey(const BlockPos& pos, ChunkCoord chunkX, ChunkCoord chunkZ)
{
    return std::to_string(chunkX) + ":" + std::to_string(chunkZ) + ":" + std::to_string(pos.x) + ":" +
        std::to_string(pos.y) + ":" + std::to_string(pos.z);
}

std::vector<u8> BlockEntityStorageManager::_makeKey(const std::string& key)
{
    return std::vector<u8>(key.begin(), key.end());
}

std::vector<u8> BlockEntityStorageManager::_makeChunkPrefixKey(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    std::string prefix = std::to_string(chunkX) + ":" + std::to_string(chunkZ) + ":";
    return std::vector<u8>(prefix.begin(), prefix.end());
}

std::vector<u8> BlockEntityStorageManager::_makeChunkEndKey(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    std::string prefix = std::to_string(chunkX) + ":" + std::to_string(chunkZ) + ":";
    prefix.push_back(static_cast<char>(0xFF));
    return std::vector<u8>(prefix.begin(), prefix.end());
}

} // namespace mc::world::storage
