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

#include "BedrockLevelDb.hpp"
#include <string_view>
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <spdlog/spdlog.h>

namespace mc::world::storage::reader::bedrock {

BedrockLevelDb::BedrockLevelDb() = default;

BedrockLevelDb::~BedrockLevelDb()
{
    close();
}

Result<void> BedrockLevelDb::open(const std::filesystem::path& dbPath)
{
    if (m_db) {
        return {};
    }

    leveldb::Options options;
    options.create_if_missing = false;
    options.error_if_exists = false;
    options.paranoid_checks = false;

    leveldb::DB* db = nullptr;
    leveldb::Status status = leveldb::DB::Open(options, dbPath.string(), &db);

    if (!status.ok()) {
        return Error(ErrorCode::FileOpenFailed,
            fmt::format("Failed to open Bedrock LevelDB at {}: {}", dbPath.string(), status.ToString()));
    }

    m_db = db;
    m_readonly = true;

    spdlog::info("BedrockLevelDb: Opened database at {}", dbPath.string());
    return {};
}

void BedrockLevelDb::close()
{
    if (m_db) {
        delete m_db;
        m_db = nullptr;
        spdlog::info("BedrockLevelDb: Closed database");
    }
}

bool BedrockLevelDb::isOpen() const
{
    return m_db != nullptr;
}

Result<std::optional<std::vector<u8>>> BedrockLevelDb::get(const std::vector<u8>& key)
{
    if (!m_db) {
        return Error(ErrorCode::InvalidState, "Database not open");
    }

    std::string value;
    leveldb::Slice keySlice(reinterpret_cast<const char*>(key.data()), key.size());
    leveldb::Status status = m_db->Get(leveldb::ReadOptions(), keySlice, &value);

    if (status.IsNotFound()) {
        return std::optional<std::vector<u8>>{};
    }

    if (!status.ok()) {
        return Error(ErrorCode::ChunkLoadFailed, fmt::format("LevelDB read error: {}", status.ToString()));
    }

    std::vector<u8> result(value.begin(), value.end());
    return std::optional<std::vector<u8>>(std::move(result));
}

Result<void> BedrockLevelDb::iteratePrefix(const std::vector<u8>& prefix, KeyCallback callback)
{
    if (!m_db) {
        return Error(ErrorCode::InvalidState, "Database not open");
    }

    leveldb::Slice prefixSlice(reinterpret_cast<const char*>(prefix.data()), prefix.size());

    auto* iter = m_db->NewIterator(leveldb::ReadOptions());
    if (prefix.empty()) {
        iter->SeekToFirst();
    } else {
        iter->Seek(prefixSlice);
    }

    while (iter->Valid()) {
        leveldb::Slice key = iter->key();
        if (!prefix.empty() && !key.starts_with(prefixSlice)) {
            break;
        }

        std::vector<u8> keyVec(key.data(), key.data() + key.size());
        leveldb::Slice value = iter->value();
        std::vector<u8> valueVec(value.data(), value.data() + value.size());

        if (!callback(keyVec, valueVec)) {
            break;
        }

        iter->Next();
    }

    if (!iter->status().ok()) {
        delete iter;
        return Error(ErrorCode::ChunkLoadFailed, fmt::format("LevelDB iteration error: {}", iter->status().ToString()));
    }

    delete iter;
    return {};
}

Result<void> BedrockLevelDb::iterateChunk(i32 chunkX, i32 chunkZ, DimensionId dimension, KeyCallback callback)
{
    auto prefix = buildChunkPrefix(chunkX, chunkZ, dimension);
    return iteratePrefix(prefix, callback);
}

std::vector<u8> BedrockLevelDb::buildKey(i32 chunkX, i32 chunkZ, ChunkType type)
{
    std::vector<u8> key;
    key.reserve(9);
    // 小端序写入 chunkX 和 chunkZ
    key.push_back(static_cast<u8>(chunkX & 0xFF));
    key.push_back(static_cast<u8>((chunkX >> 8) & 0xFF));
    key.push_back(static_cast<u8>((chunkX >> 16) & 0xFF));
    key.push_back(static_cast<u8>((chunkX >> 24) & 0xFF));
    key.push_back(static_cast<u8>(chunkZ & 0xFF));
    key.push_back(static_cast<u8>((chunkZ >> 8) & 0xFF));
    key.push_back(static_cast<u8>((chunkZ >> 16) & 0xFF));
    key.push_back(static_cast<u8>((chunkZ >> 24) & 0xFF));
    key.push_back(static_cast<u8>(type));
    return key;
}

std::vector<u8> BedrockLevelDb::buildKey(i32 chunkX, i32 chunkZ, DimensionId dimension, ChunkType type)
{
    std::vector<u8> key;
    key.reserve(13);
    // 小端序写入 chunkX、chunkZ、dimensionId、type
    key.push_back(static_cast<u8>(chunkX & 0xFF));
    key.push_back(static_cast<u8>((chunkX >> 8) & 0xFF));
    key.push_back(static_cast<u8>((chunkX >> 16) & 0xFF));
    key.push_back(static_cast<u8>((chunkX >> 24) & 0xFF));
    key.push_back(static_cast<u8>(chunkZ & 0xFF));
    key.push_back(static_cast<u8>((chunkZ >> 8) & 0xFF));
    key.push_back(static_cast<u8>((chunkZ >> 16) & 0xFF));
    key.push_back(static_cast<u8>((chunkZ >> 24) & 0xFF));
    key.push_back(static_cast<u8>(dimension & 0xFF));
    key.push_back(static_cast<u8>((dimension >> 8) & 0xFF));
    key.push_back(static_cast<u8>((dimension >> 16) & 0xFF));
    key.push_back(static_cast<u8>((dimension >> 24) & 0xFF));
    key.push_back(static_cast<u8>(type));
    return key;
}

std::vector<u8> BedrockLevelDb::buildSubChunkKey(i32 chunkX, i32 chunkZ, ChunkType type, i8 subChunkY)
{
    auto key = buildKey(chunkX, chunkZ, type);
    key.push_back(static_cast<u8>(subChunkY));
    return key;
}

std::vector<u8> BedrockLevelDb::buildSubChunkKey(
    i32 chunkX, i32 chunkZ, DimensionId dimension, ChunkType type, i8 subChunkY)
{
    auto key = buildKey(chunkX, chunkZ, dimension, type);
    key.push_back(static_cast<u8>(subChunkY));
    return key;
}

std::vector<u8> BedrockLevelDb::buildChunkPrefix(i32 chunkX, i32 chunkZ, DimensionId dimension)
{
    std::vector<u8> prefix;
    prefix.reserve(12);
    // 小端序写入 chunkX 和 chunkZ
    prefix.push_back(static_cast<u8>(chunkX & 0xFF));
    prefix.push_back(static_cast<u8>((chunkX >> 8) & 0xFF));
    prefix.push_back(static_cast<u8>((chunkX >> 16) & 0xFF));
    prefix.push_back(static_cast<u8>((chunkX >> 24) & 0xFF));
    prefix.push_back(static_cast<u8>(chunkZ & 0xFF));
    prefix.push_back(static_cast<u8>((chunkZ >> 8) & 0xFF));
    prefix.push_back(static_cast<u8>((chunkZ >> 16) & 0xFF));
    prefix.push_back(static_cast<u8>((chunkZ >> 24) & 0xFF));
    // 主世界不包含 dimensionId，其他维度包含
    if (dimension != 0) {
        prefix.push_back(static_cast<u8>(dimension & 0xFF));
        prefix.push_back(static_cast<u8>((dimension >> 8) & 0xFF));
        prefix.push_back(static_cast<u8>((dimension >> 16) & 0xFF));
        prefix.push_back(static_cast<u8>((dimension >> 24) & 0xFF));
    }
    return prefix;
}

std::vector<u8> BedrockLevelDb::buildLocalPlayerKey()
{
    static constexpr std::string_view kLocalPlayer = "~local_player";
    return std::vector<u8>(kLocalPlayer.begin(), kLocalPlayer.end());
}

std::vector<u8> BedrockLevelDb::buildActorPrefix()
{
    static constexpr std::string_view kActorPrefix = "actorprefix";
    return std::vector<u8>(kActorPrefix.begin(), kActorPrefix.end());
}

} // namespace mc::world::storage::reader::bedrock
