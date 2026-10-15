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

#include "common/util/assert/AssertAll.hpp"
#include "server/world/storage/db/ColumnFamilies.hpp"
#include "server/world/storage/db/ConsistencyMode.hpp"
#include "server/world/storage/db/RocksDBDatabase.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <benchmark/benchmark.h>
#include <rocksdb/perf_context.h>
#include <rocksdb/perf_level.h>

namespace {

using namespace mc;
using namespace mc::world::storage;
using _bounds = std::pair<std::vector<u8>, std::vector<u8>>;

class _rangeSeekFixture {
public:
    ~_rangeSeekFixture()
    {
        db.reset();
        if (!m_directory.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(m_directory, ec);
        }
    }

    Result<void> initialize(i32 chunks, bool flushTombstones)
    {
        std::error_code ec;
        auto temporary = std::filesystem::temp_directory_path(ec);
        if (ec) {
            return Error(ErrorCode::FileWriteFailed, ec.message());
        }
        static std::atomic<u64> sequence{0};
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        m_directory =
            temporary / "cubium_entity_seek" / (std::to_string(stamp) + "_" + std::to_string(sequence.fetch_add(1)));
        RocksDBConfig config;
        config.consistencyMode = ConsistencyMode::Eventual;
        auto opened = RocksDBDatabase::open(m_directory, config);
        if (opened.failed()) {
            return opened.error();
        }
        db = opened.value();
        auto* handle = db->getCF(cf::ENTITIES_OVERWORLD);
        rocksdb::WriteBatch points;
        rocksdb::WriteBatch deletions;
        for (i32 i = 0; i < chunks; ++i) {
            const auto prefix = std::to_string(i) + ":0:";
            points.Put(handle, prefix + "old", "old entity");
            deletions.DeleteRange(handle, prefix, prefix + static_cast<char>(0xFF));
        }
        // 保留远处可见记录，复现空区块 Seek 越过大量已删除键寻找下一条记录。
        points.Put(handle, "zzzz", "live sentinel");
        auto written = db->writeBatch(points, false);
        if (written.failed()) {
            return written;
        }
        auto flushed = db->flush(cf::ENTITIES_OVERWORLD);
        if (flushed.failed()) {
            return flushed;
        }
        written = db->writeBatch(deletions, false);
        if (written.failed()) {
            return written;
        }
        if (flushTombstones) {
            flushed = db->flush(cf::ENTITIES_OVERWORLD);
            if (flushed.failed()) {
                return flushed;
            }
        }
        constexpr i32 QUERY_COUNT = 64;
        for (i32 i = 0; i < QUERY_COUNT; ++i) {
            const auto prefix = std::to_string(i * chunks / QUERY_COUNT) + ":0:";
            std::vector<u8> lower(prefix.begin(), prefix.end());
            auto upper = lower;
            upper.push_back(0xFF);
            queries.emplace_back(std::move(lower), std::move(upper));
        }
        return {};
    }

    std::unique_ptr<RocksDBDatabase> db;
    std::vector<_bounds> queries;

private:
    std::filesystem::path m_directory;
};

Result<size_t> _seekValues(RocksDBDatabase& db, const _bounds& bounds, bool bounded)
{
    if (bounded) {
        auto values = db.readRange(cf::ENTITIES_OVERWORLD, bounds.first, bounds.second);
        if (values.failed()) {
            return values.error();
        }
        return values.value().size();
    }
    auto iter = db.newIterator(cf::ENTITIES_OVERWORLD);
    const rocksdb::Slice lower(reinterpret_cast<const char*>(bounds.first.data()), bounds.first.size());
    const rocksdb::Slice upper(reinterpret_cast<const char*>(bounds.second.data()), bounds.second.size());
    size_t count = 0;
    for (iter->Seek(lower); iter->Valid() && iter->key().compare(upper) < 0; iter->Next()) {
        ++count;
    }
    if (!iter->status().ok()) {
        return Error(ErrorCode::FileReadFailed, iter->status().ToString());
    }
    return count;
}

void _entityRangeSeek(::benchmark::State& state)
{
    _rangeSeekFixture fixture;
    const bool bounded = state.range(0) != 0;
    auto initialized = fixture.initialize(static_cast<i32>(state.range(2)), state.range(1) != 0);
    if (initialized.failed()) {
        state.SkipWithError(initialized.error().message());
        return;
    }
    for (const auto& query : fixture.queries) {
        auto warmed = _seekValues(*fixture.db, query, bounded);
        if (warmed.failed() || warmed.value() != 0) {
            state.SkipWithError("Empty entity range warmup failed");
            return;
        }
    }
    const auto previousLevel = rocksdb::GetPerfLevel();
    rocksdb::SetPerfLevel(rocksdb::PerfLevel::kEnableCount);
    u64 comparisons = 0;
    u64 skipped = 0;
    size_t nextQuery = 0;
    for (auto iteration : state) {
        MC_UNUSED(iteration);
        auto* context = rocksdb::get_perf_context();
        context->Reset();
        auto result = _seekValues(*fixture.db, fixture.queries[nextQuery++ % fixture.queries.size()], bounded);
        comparisons += context->user_key_comparison_count;
        skipped += context->internal_key_skipped_count;
        if (result.failed() || result.value() != 0) {
            state.SkipWithError("Empty entity range query failed");
            break;
        }
        ::benchmark::DoNotOptimize(result.value());
    }
    rocksdb::SetPerfLevel(previousLevel);
    state.counters["comparisons_per_seek"] = static_cast<double>(comparisons) / static_cast<double>(state.iterations());
    state.counters["skipped_keys_per_seek"] = static_cast<double>(skipped) / static_cast<double>(state.iterations());
}

BENCHMARK(_entityRangeSeek)
    ->Name("EntityRangeSeek")
    ->ArgsProduct({{0, 1}, {0, 1}, {1024, 4096}})
    ->ArgNames({"bounded", "tombstones_on_disk", "chunks"});

} // namespace
