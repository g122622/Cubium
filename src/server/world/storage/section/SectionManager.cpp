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

#include "server/world/storage/section/SectionManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "server/world/storage/db/ColumnFamilies.hpp"
#include "server/world/storage/db/ConsistencyMode.hpp"
#include "server/world/storage/db/RocksDBDatabase.hpp"
#include "server/world/storage/db/SectionCodec.hpp"
#include "server/world/storage/db/SectionKey.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <rocksdb/slice.h>
#include <rocksdb/write_batch.h>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::world::storage {

// ============================================================================
// 构造与析构
// ============================================================================

SectionManager::SectionManager(RocksDBDatabase& db, DimensionId dimension, const Config& config)
    : m_db(db)
    , m_dimension(dimension)
    , m_cfName(cf::getSectionCF(dimension))
    , m_config(config)
{
    spdlog::info("SectionManager created for dimension {} (CF: {})", static_cast<i32>(m_dimension), m_cfName);
}

// ============================================================================
// Section加载
// ============================================================================

Result<std::vector<std::shared_ptr<const SectionData>>> SectionManager::loadSectionsSync(
    const std::vector<SectionKey>& keys)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::loadSectionsSync", "count", keys.size());

    for (const auto& key : keys) {
        if (key.dimension != m_dimension) {
            return Error(ErrorCode::InvalidArgument,
                fmt::format("Dimension mismatch: expected {}, got {}",
                    static_cast<i32>(m_dimension),
                    static_cast<i32>(key.dimension)));
        }
    }

    if (keys.empty()) {
        return std::vector<std::shared_ptr<const SectionData>>{};
    }

    return _loadFromDatabaseBatch(keys);
}

// ============================================================================
// Section保存
// ============================================================================

Result<size_t> SectionManager::saveSectionsBatch(const std::vector<SectionWrite>& writes, bool sync)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::saveSectionsBatch", "count", writes.size());

    if (writes.empty()) {
        return 0;
    }

    auto* cf = m_db.getCF(m_cfName);
    if (cf == nullptr) {
        return Error(ErrorCode::InvalidState, fmt::format("Column family not found: {}", m_cfName));
    }

    rocksdb::WriteBatch batch;
    for (const auto& write : writes) {
        // 落错列族的数据按本维度再也读不回来，属于静默丢档，必须当场暴露。
        MC_ASSERT_RELEASE_MSG(write.key.dimension == m_dimension,
            "SectionManager::saveSectionsBatch: section key dimension does not match this manager's dimension");
        MC_ASSERT_RELEASE_MSG(!write.bytes.empty(), "SectionManager::saveSectionsBatch: empty serialized section");

        auto keyBytes = write.key.toKey();
        batch.Put(cf,
            rocksdb::Slice(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size()),
            rocksdb::Slice(reinterpret_cast<const char*>(write.bytes.data()), write.bytes.size()));
    }

    // 批次条目数必须与传入段数严格一致：少了说明有条目被静默吞掉（那些段会在
    // "已保存"的假象下丢掉），多了说明同一条目被重复计入。
    MC_ASSERT_RELEASE(static_cast<size_t>(batch.Count()) == writes.size());

    // 一批只提交一次：逐段 put 会让每段各付一次 WAL fsync，视野距离 16 下 2048 个段
    // 实测约 6.6 秒，而聚合后同一份数据只需几十毫秒。sync=true 供关服与显式
    // /save-all flush 使用；其余情况由一致性模式决定（Eventual 下不等待 fsync）。
    const bool syncWrites = sync || m_config.consistencyMode != ConsistencyMode::Eventual;
    auto writeResult = m_db.writeBatch(batch, syncWrites);
    if (writeResult.failed()) {
        return writeResult.error();
    }

    return writes.size();
}

// ============================================================================
// 内部方法
// ============================================================================

Result<std::vector<std::shared_ptr<const SectionData>>> SectionManager::_loadFromDatabaseBatch(
    const std::vector<SectionKey>& keys)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::loadFromDatabaseBatch", "count", keys.size());

    std::vector<std::vector<u8>> keyBytesList;
    keyBytesList.reserve(keys.size());
    for (const auto& key : keys) {
        keyBytesList.push_back(key.toKey());
    }

    auto multiGetResult = m_db.multiGet(m_cfName, keyBytesList);
    if (multiGetResult.failed()) {
        return multiGetResult.error();
    }

    const auto& rawResults = multiGetResult.value();
    if (rawResults.size() != keys.size()) {
        return Error(ErrorCode::InvalidData,
            fmt::format("RocksDB multiGet returned {} results for {} keys", rawResults.size(), keys.size()));
    }

    std::vector<std::shared_ptr<const SectionData>> results;
    results.reserve(keys.size());

    for (size_t i = 0; i < rawResults.size(); ++i) {
        const auto& rawResult = rawResults[i];
        const auto& key = keys[i];

        if (rawResult.failed()) {
            if (rawResult.error().code() == ErrorCode::NotFound) {
                results.emplace_back(nullptr);
                continue;
            }
            return rawResult.error();
        }

        const auto& value = rawResult.value();
        auto deserializeResult = SectionData::deserialize(value.data(), value.size());
        if (deserializeResult.failed()) {
            return deserializeResult.error();
        }

        auto data = std::make_shared<SectionData>(std::move(deserializeResult.value()));
        // key 不存储在序列化数据里，只能从 RocksDB 键恢复。
        data->key = key;
        results.push_back(std::static_pointer_cast<const SectionData>(data));
    }

    return results;
}

} // namespace mc::world::storage
