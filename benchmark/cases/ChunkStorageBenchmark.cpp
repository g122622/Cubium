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

#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "server/world/storage/db/SectionCodec.hpp"
#include <benchmark/benchmark.h>

namespace {
using namespace mc;
using namespace mc::world::storage;

ChunkData _makeChunk(i32 values)
{
    VanillaBlocks::initialize();
    ChunkData chunk(0, 0);
    auto* section = chunk.createSection(0);
    section->fill(100);
    for (i32 i = 0; i < ChunkSection::VOLUME; ++i) {
        section->setBlockStateIdFast(i, 100 + static_cast<u32>(i % values));
    }
    return chunk;
}

class _capturedValue final : public rocksdb::WriteBatch::Handler {
public:
    std::string bytes;
    rocksdb::Status PutCF(uint32_t, const rocksdb::Slice&, const rocksdb::Slice& value) override
    {
        bytes.assign(value.data(), value.size());
        return rocksdb::Status::OK();
    }
};

void _capture(::benchmark::State& state)
{
    const auto chunk = _makeChunk(static_cast<i32>(state.range(0)));
    const SectionKey key(0, 0, world::MIN_SECTION_Y, 0);
    size_t batchBytes = 0;
    for (auto iteration : state) {
        MC_UNUSED(iteration);
        rocksdb::WriteBatch batch(SectionCodec::encodedSize(*chunk.getSection(0)) + 44);
        auto result = SectionCodec::appendToBatch(batch, nullptr, key, chunk, 0);
        MC_ASSERT_RELEASE(result.success());
        batchBytes = batch.GetDataSize();
        ::benchmark::DoNotOptimize(batch.Data().data());
        ::benchmark::ClobberMemory();
    }
    state.counters["batch_bytes"] = static_cast<double>(batchBytes);
    state.SetItemsProcessed(state.iterations());
}

void _restore(::benchmark::State& state)
{
    const auto chunk = _makeChunk(static_cast<i32>(state.range(0)));
    rocksdb::WriteBatch batch(SectionCodec::encodedSize(*chunk.getSection(0)) + 44);
    const SectionKey key(0, 0, world::MIN_SECTION_Y, 0);
    MC_ASSERT_RELEASE(SectionCodec::appendToBatch(batch, nullptr, key, chunk, 0).success());
    _capturedValue capture;
    MC_ASSERT_RELEASE(batch.Iterate(&capture).ok());
    for (auto iteration : state) {
        MC_UNUSED(iteration);
        ChunkData restored(0, 0);
        auto result = SectionCodec::restore(
            {reinterpret_cast<const u8*>(capture.bytes.data()), capture.bytes.size()}, restored, 0);
        MC_ASSERT_RELEASE(result.success());
        ::benchmark::DoNotOptimize(restored.getSection(0));
        ::benchmark::ClobberMemory();
    }
    state.counters["value_bytes"] = static_cast<double>(capture.bytes.size());
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(_capture)->Name("CompactSectionCapture")->Unit(::benchmark::kNanosecond)->Arg(1)->Arg(16)->Arg(65)->Arg(256);
BENCHMARK(_restore)->Name("CompactSectionRestore")->Unit(::benchmark::kNanosecond)->Arg(1)->Arg(16)->Arg(65)->Arg(256);

} // namespace
