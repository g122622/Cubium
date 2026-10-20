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

#include "SectionManager.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "server/world/storage/db/SectionCodec.hpp"
#include <array>

using namespace mc::trace;

namespace mc::world::storage {

SectionManager::SectionManager(RocksDBDatabase& db, DimensionId dimension, ConsistencyMode consistencyMode)
    : m_db(db)
    , m_dimension(dimension)
    , m_cfName(cf::getSectionCF(dimension))
    , m_consistencyMode(consistencyMode)
{}

Result<PinnedValues> SectionManager::loadSectionsSync(std::span<const SectionKey> keys)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::loadSectionsSync", "count", keys.size());
    std::vector<std::array<u8, 13>> keyBytes(keys.size());
    std::vector<rocksdb::Slice> slices;
    slices.reserve(keys.size());
    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i].dimension != m_dimension) {
            return Error(ErrorCode::InvalidArgument, "Section key dimension does not match manager");
        }
        keyBytes[i] = keys[i].serialize();
        slices.emplace_back(reinterpret_cast<const char*>(keyBytes[i].data()), keyBytes[i].size());
    }
    return m_db.multiGetPinned(m_cfName, slices);
}

Result<void> SectionManager::appendChunk(rocksdb::WriteBatch& batch, const ChunkData& chunk)
{
    auto* columnFamily = m_db.getCF(m_cfName);
    if (columnFamily == nullptr) {
        return Error(ErrorCode::InvalidState, "Section column family not found");
    }
    // 主线程串行写入方块；调用方在交给后台线程之前完成本次批次快照。
    const auto lock = chunk.lockForLightRead();
    for (i32 index = 0; index < world::CHUNK_SECTIONS; ++index) {
        const SectionKey key(chunk.x(), chunk.z(), static_cast<i8>(world::sectionIndexToCoord(index)), m_dimension);
        auto result = SectionCodec::appendToBatch(batch, columnFamily, key, chunk, index);
        if (result.failed()) {
            return result.error();
        }
    }
    return {};
}

Result<size_t> SectionManager::saveBatch(rocksdb::WriteBatch& batch, bool sync)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::saveBatch", "count", batch.Count());
    if (batch.Count() == 0) {
        return size_t{0};
    }
    auto result = m_db.writeBatch(batch, sync || consistencyModeSyncsEveryWrite(m_consistencyMode));
    if (result.failed()) {
        return result.error();
    }
    return static_cast<size_t>(batch.Count());
}

} // namespace mc::world::storage
