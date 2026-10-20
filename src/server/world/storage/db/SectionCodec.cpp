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
#include "common/world/fluid/Fluid.hpp"
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
constexpr u32 _formatMagic = 0x31534343;
constexpr size_t _headerWords = 8;
constexpr size_t _headerSize = _headerWords * sizeof(u32);
constexpr size_t _biomeBytes = BiomeContainer::SECTION_BIOME_SIZE * sizeof(BiomeId);

constexpr u8 _blockFlag = 1;
constexpr u8 _tickFlag = 2;
constexpr u8 _fluidFlag = 4;

u8 _stateFlags(u32 stateId, u8 fields)
{
    const BlockState* state = Block::getBlockState(stateId);
    if (state == nullptr) {
        return 0;
    }
    u8 flags = 0;
    if ((fields & _blockFlag) != 0 && !state->isAir()) {
        flags |= _blockFlag;
    }
    if ((fields & _tickFlag) != 0 && state->getBlock().ticksRandomly()) {
        flags |= _tickFlag;
    }
    if ((fields & _fluidFlag) != 0) {
        const auto* fluid = state->getFluidState();
        if (fluid != nullptr && !fluid->isEmpty()) {
            flags |= _fluidFlag;
        }
    }
    return flags;
}

template <typename T, size_t Extent>
rocksdb::Slice _slice(std::span<T, Extent> values)
{
    return rocksdb::Slice(values.empty() ? "" : reinterpret_cast<const char*>(values.data()), values.size_bytes());
}

} // namespace

size_t SectionCodec::encodedSize(const ChunkSection* section)
{
    if (section == nullptr) {
        return _headerSize + _biomeBytes;
    }
    const auto packed = section->blockStates().packedView();
    return _headerSize + _biomeBytes + packed.palette.size_bytes() + packed.storage.size_bytes() +
        packed.hashMap.size_bytes();
}

Result<void> SectionCodec::appendToBatch(rocksdb::WriteBatch& batch,
    rocksdb::ColumnFamilyHandle* columnFamily,
    const SectionKey& key,
    const ChunkData& chunk,
    i32 sectionIndex)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Db, "SectionCodec::appendToBatch");
    MC_ASSERT_RELEASE(sectionIndex >= 0);
    MC_ASSERT_RELEASE(sectionIndex < world::CHUNK_SECTIONS);
    const ChunkSection* section = chunk.getSection(sectionIndex);
    const auto packed = section == nullptr ? world::chunk::PalettedContainer::PackedView{0, {}, {}, {}}
                                           : section->blockStates().packedView();
    MC_ASSERT_RELEASE(packed.palette.size() <= std::numeric_limits<u32>::max());
    MC_ASSERT_RELEASE(packed.hashMap.size() <= std::numeric_limits<u32>::max());
    const std::array<u32, _headerWords> header{_formatMagic,
        static_cast<u32>(packed.bits),
        static_cast<u32>(packed.palette.size()),
        static_cast<u32>(packed.storage.size()),
        static_cast<u32>(packed.hashMap.size()),
        section == nullptr ? 0u : section->getBlockCount(),
        section == nullptr ? 0u : section->blockTickRefCount(),
        section == nullptr ? 0u : section->fluidRefCount()};
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
    MC_ASSERT_RELEASE(sectionIndex >= 0);
    MC_ASSERT_RELEASE(sectionIndex < world::CHUNK_SECTIONS);
    if (bytes.size() < _headerSize + _biomeBytes) {
        return Error(ErrorCode::InvalidData, "Truncated compact section header");
    }
    std::array<u32, _headerWords> header;
    std::memcpy(header.data(), bytes.data(), _headerSize);
    if (header[0] != _formatMagic || header[1] > 31 || header[5] > ChunkSection::VOLUME ||
        header[6] > ChunkSection::VOLUME || header[7] > ChunkSection::VOLUME) {
        return Error(ErrorCode::InvalidData, "Invalid compact section header");
    }
    const u64 paletteBytes = static_cast<u64>(header[2]) * sizeof(u32);
    const u64 storageBytes = static_cast<u64>(header[3]) * sizeof(u64);
    const u64 hashBytes = static_cast<u64>(header[4]) * sizeof(u32);
    if (_headerSize + paletteBytes + storageBytes + hashBytes + _biomeBytes != bytes.size()) {
        return Error(ErrorCode::InvalidData, "Compact section length does not match header");
    }
    if (header[2] == 0) {
        if (header[1] != 0 || header[3] != 0 || header[4] != 0 || header[5] != 0 || header[6] != 0 || header[7] != 0) {
            return Error(ErrorCode::InvalidData, "Biome-only section contains block data");
        }
        std::memcpy(chunk.getBiomes().sectionData(sectionIndex).data(), bytes.data() + _headerSize, _biomeBytes);
        return {};
    }
    size_t offset = _headerSize;
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
    // 存档是外部输入：计数虽然范围合法，也不能让错误计数把有方块的段当成空段。
    // 只读取位数组进行验证，不展开数据、不分配暂存容器、不修改已恢复的哈希槽位。
    const auto packed = states.packedView();
    // 每种状态只查询一次注册表；栈上只缓存一字节计数属性，不展开方块数据。
    std::array<u8, ChunkSection::VOLUME> paletteFlags;
    const bool cacheFits = packed.palette.size() <= paletteFlags.size();
    u8 allFlags = _blockFlag | _tickFlag | _fluidFlag;
    u8 anyFlags = 0;
    for (size_t index = 0; index < packed.palette.size(); ++index) {
        const u8 flags = _stateFlags(packed.palette[index], _blockFlag | _tickFlag | _fluidFlag);
        if (cacheFits) {
            paletteFlags[index] = flags;
        }
        allFlags &= flags;
        anyFlags |= flags;
    }
    u32 blockCount = (allFlags & _blockFlag) != 0 ? ChunkSection::VOLUME : 0;
    u32 blockTickCount = (allFlags & _tickFlag) != 0 ? ChunkSection::VOLUME : 0;
    u32 fluidCount = (allFlags & _fluidFlag) != 0 ? ChunkSection::VOLUME : 0;
    const u8 varyingFlags = anyFlags ^ allFlags;
    // 均匀段，以及所有调色板值拥有相同计数属性的段，都不需要遍历 4096 个位置。
    if (varyingFlags != 0) {
        for (i32 index = 0; index < ChunkSection::VOLUME; ++index) {
            u8 flags;
            if (cacheFits) {
                const i32 bitIndex = index * packed.bits;
                const size_t word = static_cast<size_t>(bitIndex / 64);
                const i32 shift = bitIndex % 64;
                u64 paletteIndex = packed.storage[word] >> shift;
                if (shift + packed.bits > 64) {
                    paletteIndex |= packed.storage[word + 1] << (64 - shift);
                }
                paletteIndex &= (u64{1} << packed.bits) - 1;
                flags = paletteFlags[static_cast<size_t>(paletteIndex)] & varyingFlags;
            } else {
                // 反复修改可能累计超过段容量的调色板条目；此时直接查询，不扩大暂存内存。
                flags = _stateFlags(states.get(index), varyingFlags);
            }
            blockCount += (flags & _blockFlag) != 0 ? 1 : 0;
            blockTickCount += (flags & _tickFlag) != 0 ? 1 : 0;
            fluidCount += (flags & _fluidFlag) != 0 ? 1 : 0;
        }
    }
    if (blockCount != header[5] || blockTickCount != header[6] || fluidCount != header[7]) {
        return Error(ErrorCode::InvalidData, "Compact section counters do not match block states");
    }
    auto* section = chunk.createSection(sectionIndex);
    section->restoreBlockStates(
        std::move(states), static_cast<u16>(header[5]), static_cast<u16>(header[6]), static_cast<u16>(header[7]));
    std::memcpy(chunk.getBiomes().sectionData(sectionIndex).data(), bytes.data() + offset, _biomeBytes);
    return {};
}

} // namespace mc::world::storage
