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

#include "SectionCodec.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <limits>
#include <rocksdb/slice.h>

using namespace mc::trace;

namespace mc::world::storage {
namespace {

// 所有数值数组固定采用小端布局；当前支持的平台均可直接复制，无逐元素字节序转换。
static_assert(std::endian::native == std::endian::little);
constexpr u32 _FORMAT_MAGIC = 0x31534343;
constexpr size_t _HEADER_WORDS = 8;
constexpr size_t _HEADER_SIZE = _HEADER_WORDS * sizeof(u32);
constexpr size_t _BIOME_BYTES = BiomeContainer::SECTION_BIOME_SIZE * sizeof(BiomeId);

template <typename T, size_t Extent>
rocksdb::Slice _slice(std::span<T, Extent> values)
{
    return rocksdb::Slice(values.empty() ? "" : reinterpret_cast<const char*>(values.data()), values.size_bytes());
}

} // namespace

size_t SectionCodec::encodedSize(const ChunkSection& section)
{
    const auto packed = section.blockStates().packedView();
    return _HEADER_SIZE + _BIOME_BYTES + packed.palette.size_bytes() + packed.storage.size_bytes() +
        packed.hashMap.size_bytes();
}

Result<void> SectionCodec::appendToBatch(rocksdb::WriteBatch& batch,
    rocksdb::ColumnFamilyHandle* columnFamily,
    const SectionKey& key,
    const ChunkData& chunk,
    i32 sectionIndex)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "SectionCodec::appendToBatch");
    MC_ASSERT_RELEASE(sectionIndex >= 0 && sectionIndex < world::CHUNK_SECTIONS);
    const ChunkSection* section = chunk.getSection(sectionIndex);
    MC_ASSERT_RELEASE(section != nullptr);
    const auto packed = section->blockStates().packedView();
    MC_ASSERT_RELEASE(packed.palette.size() <= std::numeric_limits<u32>::max());
    MC_ASSERT_RELEASE(packed.hashMap.size() <= std::numeric_limits<u32>::max());
    const std::array<u32, _HEADER_WORDS> header{_FORMAT_MAGIC,
        static_cast<u32>(packed.bits),
        static_cast<u32>(packed.palette.size()),
        static_cast<u32>(packed.storage.size()),
        static_cast<u32>(packed.hashMap.size()),
        section->getBlockCount(),
        section->blockTickRefCount(),
        section->fluidRefCount()};
    const auto keyBytes = key.serialize();
    const std::array<rocksdb::Slice, 1> keyParts{_slice(std::span(keyBytes))};
    const std::array<rocksdb::Slice, 5> valueParts{_slice(std::span(header)),
        _slice(packed.palette),
        _slice(packed.storage),
        _slice(packed.hashMap),
        _slice(chunk.getBiomes().sectionData(sectionIndex))};
    const auto status = batch.Put(columnFamily,
        rocksdb::SliceParts(keyParts.data(), static_cast<int>(keyParts.size())),
        rocksdb::SliceParts(valueParts.data(), static_cast<int>(valueParts.size())));
    if (!status.ok()) {
        return Error(ErrorCode::FileWriteFailed, status.ToString());
    }
    return {};
}

Result<void> SectionCodec::restore(std::span<const u8> bytes, ChunkData& chunk, i32 sectionIndex)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "SectionCodec::restore", "size", bytes.size());
    MC_ASSERT_RELEASE(sectionIndex >= 0 && sectionIndex < world::CHUNK_SECTIONS);
    if (bytes.size() < _HEADER_SIZE + _BIOME_BYTES) {
        return Error(ErrorCode::InvalidData, "Truncated compact section header");
    }
    std::array<u32, _HEADER_WORDS> header;
    std::memcpy(header.data(), bytes.data(), _HEADER_SIZE);
    if (header[0] != _FORMAT_MAGIC || header[1] > 31 || header[5] > ChunkSection::VOLUME ||
        header[6] > ChunkSection::VOLUME || header[7] > ChunkSection::VOLUME) {
        return Error(ErrorCode::InvalidData, "Invalid compact section header");
    }
    const u64 paletteBytes = static_cast<u64>(header[2]) * sizeof(u32);
    const u64 storageBytes = static_cast<u64>(header[3]) * sizeof(u64);
    const u64 hashBytes = static_cast<u64>(header[4]) * sizeof(u32);
    if (_HEADER_SIZE + paletteBytes + storageBytes + hashBytes + _BIOME_BYTES != bytes.size()) {
        return Error(ErrorCode::InvalidData, "Compact section length does not match header");
    }
    size_t offset = _HEADER_SIZE;
    const auto palette = bytes.subspan(offset, static_cast<size_t>(paletteBytes));
    offset += static_cast<size_t>(paletteBytes);
    const auto storage = bytes.subspan(offset, static_cast<size_t>(storageBytes));
    offset += static_cast<size_t>(storageBytes);
    const auto hashMap = bytes.subspan(offset, static_cast<size_t>(hashBytes));
    offset += static_cast<size_t>(hashBytes);
    world::chunk::PalettedContainer states;
    auto result = states.restorePacked(static_cast<i32>(header[1]), palette, storage, hashMap);
    if (result.failed()) {
        return result.error();
    }
    auto* section = chunk.createSection(sectionIndex);
    section->restoreBlockStates(
        std::move(states), static_cast<u16>(header[5]), static_cast<u16>(header[6]), static_cast<u16>(header[7]));
    std::memcpy(chunk.getBiomes().sectionData(sectionIndex).data(), bytes.data() + offset, _BIOME_BYTES);
    return {};
}

} // namespace mc::world::storage
