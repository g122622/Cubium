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

#include "common/world/chunk/data/PalettedContainer.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 静态辅助
// ============================================================================

i32 PalettedContainer::_calculateBitsForCount(i32 count)
{
    if (count <= 1) {
        return 0; // 均匀态：不需要 storage
    }
    // ceil(log2(count))，下限 MIN_BITS（1 位）
    i32 bits = 0;
    i32 v = count - 1;
    while (v > 0) {
        ++bits;
        v >>= 1;
    }
    return std::max(bits, MIN_BITS);
}

i32 PalettedContainer::_storageWordCount(i32 bits)
{
    // 总位数 = VOLUME * bits，每个 u64 存 64 位
    return (VOLUME * bits + 63) / 64;
}

// ============================================================================
// 构造 / 赋值
// ============================================================================

PalettedContainer::PalettedContainer()
{
    // 均匀态：全空气（stateId 0），不分配 storage / 哈希表
    m_data.palette.push_back(0);
    m_data.paletteSize = 1;
}

// ============================================================================
// 位存储读写（仅在 bits > 0 时调用；均匀态由调用方短路）
// ============================================================================

i32 PalettedContainer::_readBits(i32 index) const
{
    const Data& d = m_data;
    const i32 bitIndex = index * d.bits;
    const i32 wordIndex = bitIndex / 64;
    const i32 bitOffset = bitIndex % 64;

    const u64 word = d.storage[static_cast<size_t>(wordIndex)];
    const u64 mask = (d.bits >= 64) ? ~0ULL : ((1ULL << d.bits) - 1);

    if (bitOffset + d.bits <= 64) {
        return static_cast<i32>((word >> bitOffset) & mask);
    }

    // 跨两个字
    const u64 low = word >> bitOffset;
    const u64 high = static_cast<u64>(d.storage[static_cast<size_t>(wordIndex + 1)]) << (64 - bitOffset);
    return static_cast<i32>((low | high) & mask);
}

i32 PalettedContainer::_writeBits(i32 index, i32 value)
{
    Data& d = m_data;
    const i32 bitIndex = index * d.bits;
    const i32 wordIndex = bitIndex / 64;
    const i32 bitOffset = bitIndex % 64;

    const u64 mask = (d.bits >= 64) ? ~0ULL : ((1ULL << d.bits) - 1);
    const u64 valueMasked = static_cast<u64>(value) & mask;

    u64& word = d.storage[static_cast<size_t>(wordIndex)];
    i32 old;
    if (bitOffset + d.bits <= 64) {
        old = static_cast<i32>((word >> bitOffset) & mask);
        word = (word & ~(mask << bitOffset)) | (valueMasked << bitOffset);
    } else {
        // 跨两个字
        old = static_cast<i32>((word >> bitOffset) & mask);
        const u64 lowBits = valueMasked << bitOffset;
        word = (word & ~(mask << bitOffset)) | lowBits;

        u64& nextWord = d.storage[static_cast<size_t>(wordIndex + 1)];
        const i32 highBits = d.bits - (64 - bitOffset);
        const u64 highValueMask = (1ULL << highBits) - 1;
        old |= static_cast<i32>((nextWord & highValueMask) << (64 - bitOffset));
        nextWord = (nextWord & ~highValueMask) | (valueMasked >> (64 - bitOffset));
    }
    return old;
}

void PalettedContainer::_onResize(i32 newBits)
{
    Data& d = m_data;
    const i32 oldBits = d.bits;

    // 保存旧 storage，后续用 oldBits 从中读 paletteIndex。
    std::vector<u64, PaletteStorageAlloc<u64>> oldStorage = std::move(d.storage);

    d.storage.assign(static_cast<size_t>(_storageWordCount(newBits)), 0);
    d.bits = newBits;

    if (oldBits == 0) {
        // 均匀态升位：旧 storage 为空，所有位置语义上都是 paletteIndex 0，
        // 新 storage 已零填充，无需搬运。paletteIndex 语义与调色板顺序都不变。
        return;
    }

    // paletteIndex 在升位前后语义不变（指向同一 palette 条目），用旧位宽读、新位宽写。
    const u64 oldMask = (oldBits >= 64) ? ~0ULL : ((1ULL << oldBits) - 1);
    for (i32 i = 0; i < VOLUME; ++i) {
        const i32 oldBitIndex = i * oldBits;
        const i32 oldWordIndex = oldBitIndex >> 6;
        const i32 oldBitOffset = oldBitIndex & 63;

        const u64 oldWord = oldStorage[static_cast<size_t>(oldWordIndex)];
        i32 paletteIndex;
        if (oldBitOffset + oldBits <= 64) {
            paletteIndex = static_cast<i32>((oldWord >> oldBitOffset) & oldMask);
        } else {
            const u64 low = oldWord >> oldBitOffset;
            const u64 high = static_cast<u64>(oldStorage[static_cast<size_t>(oldWordIndex + 1)]) << (64 - oldBitOffset);
            paletteIndex = static_cast<i32>((low | high) & oldMask);
        }
        _writeBits(i, paletteIndex);
    }
}

// ============================================================================
// 反向哈希表（开放寻址 + 线性探测；无删除操作，故无需墓碑）
// ============================================================================

i32 PalettedContainer::_hashMapLookup(u32 value) const
{
    const Data& d = m_data;
    if (d.hashMapCapacity == 0) {
        return -1;
    }
    // 使用 value 的位混合作为哈希（Knuth 乘法）
    const u32 hash = value * 2654435761u;
    i32 slot = static_cast<i32>(hash) & d.hashMapMask;
    for (i32 i = 0; i < d.hashMapCapacity; ++i) {
        const u32 entry = d.hashMap[static_cast<size_t>(slot)];
        if (entry == 0) {
            return -1; // 空槽
        }
        const i32 index = static_cast<i32>(entry) - 1;
        if (d.palette[static_cast<size_t>(index)] == value) {
            return index;
        }
        slot = (slot + 1) & d.hashMapMask;
    }
    return -1;
}

void PalettedContainer::_hashMapInsert(u32 value, i32 paletteIndex)
{
    Data& d = m_data;
    // 负载因子 > 0.75 时扩容。此时 paletteSize 已自增、palette[paletteIndex] 已写入，
    // 因此重建会把本次新增条目一并纳入表中，这里直接返回即可（不能再插一次，否则表中
    // 会出现同一 paletteIndex 的重复槽位）。
    if ((d.paletteSize + 1) * 4 > d.hashMapCapacity * 3) {
        _hashMapRebuild();
        return;
    }

    const u32 hash = value * 2654435761u;
    i32 slot = static_cast<i32>(hash) & d.hashMapMask;
    while (d.hashMap[static_cast<size_t>(slot)] != 0) {
        slot = (slot + 1) & d.hashMapMask;
    }
    d.hashMap[static_cast<size_t>(slot)] = static_cast<u32>(paletteIndex + 1);
}

void PalettedContainer::_hashMapRebuild()
{
    Data& d = m_data;
    // 容量必须是 2 的幂，且满足负载因子 <= 0.75
    const i32 minCapacity = (d.paletteSize + 1) * 4 / 3;
    i32 newCapacity = (d.hashMapCapacity == 0) ? 16 : d.hashMapCapacity * 2;
    while (newCapacity < minCapacity) {
        newCapacity *= 2;
    }
    d.hashMap.assign(static_cast<size_t>(newCapacity), 0);
    d.hashMapCapacity = newCapacity;
    d.hashMapMask = newCapacity - 1;

    for (i32 i = 0; i < d.paletteSize; ++i) {
        const u32 value = d.palette[static_cast<size_t>(i)];
        const u32 hash = value * 2654435761u;
        i32 slot = static_cast<i32>(hash) & d.hashMapMask;
        while (d.hashMap[static_cast<size_t>(slot)] != 0) {
            slot = (slot + 1) & d.hashMapMask;
        }
        d.hashMap[static_cast<size_t>(slot)] = static_cast<u32>(i + 1);
    }
}

// ============================================================================
// idFor — 查找或插入调色板
// ============================================================================

i32 PalettedContainer::_idFor(u32 value)
{
    Data& d = m_data;

    // 均匀态：storage / 哈希表都为空，唯一值存在 palette[0]
    if (d.paletteSize == 1) {
        if (d.palette[0] == value) {
            return 0;
        }
        // 出现第二个取值：追加调色板、升到 1 位、建立反向哈希表
        d.palette.push_back(value);
        d.paletteSize = 2;
        _onResize(_calculateBitsForCount(d.paletteSize));
        _hashMapRebuild(); // 重建会把两个条目都写入表中
        return 1;
    }

    const i32 existing = _hashMapLookup(value);
    if (existing >= 0) {
        return existing;
    }

    // 未命中：追加到调色板（复用已失效的槽位，避免无谓的 push_back）
    if (d.paletteSize < static_cast<i32>(d.palette.size())) {
        d.palette[static_cast<size_t>(d.paletteSize)] = value;
    } else {
        d.palette.push_back(value);
    }
    const i32 newIndex = d.paletteSize;
    ++d.paletteSize;

    _hashMapInsert(value, newIndex);

    const i32 newBits = _calculateBitsForCount(d.paletteSize);
    if (newBits > d.bits) {
        _onResize(newBits);
    }
    return newIndex;
}

// ============================================================================
// 元素访问
// ============================================================================

u32 PalettedContainer::get(i32 index) const
{
    MC_ASSERT_RELEASE(index >= 0 && index < VOLUME);
    const Data& d = m_data;
    if (d.bits == 0) {
        return d.palette[0]; // 均匀态：整段同一取值
    }
    return d.palette[static_cast<size_t>(_readBits(index))];
}

u32 PalettedContainer::getAndSet(i32 index, u32 value)
{
    MC_ASSERT_RELEASE(index >= 0 && index < VOLUME);
    Data& d = m_data;

    // 均匀态快速路径（热路径：往全空气/全同值段里写同值，如世界生成反复写同一种方块）：
    // 同值直接返回，不进入哈希表查找；异值则升到 1 位并写入该格。
    if (d.bits == 0) {
        const u32 old = d.palette[0];
        if (value == old) {
            return old;
        }
        const i32 newPaletteIndex = _idFor(value);
        _writeBits(index, newPaletteIndex);
        return old;
    }

    const i32 paletteIndex = _idFor(value);

    // 内联 _writeBits：消除函数调用边界，便于编译器优化 index*bits 与位提取。
    const i32 bits = d.bits;
    const i32 bitIndex = index * bits;
    const i32 wordIndex = bitIndex >> 6;
    const i32 bitOffset = bitIndex & 63;
    const u64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    const u64 valueMasked = static_cast<u64>(paletteIndex) & mask;

    u64& word = d.storage[static_cast<size_t>(wordIndex)];
    i32 oldPaletteIndex;
    if (bitOffset + bits <= 64) {
        oldPaletteIndex = static_cast<i32>((word >> bitOffset) & mask);
        word = (word & ~(mask << bitOffset)) | (valueMasked << bitOffset);
    } else {
        // 跨两个字
        oldPaletteIndex = static_cast<i32>((word >> bitOffset) & mask);
        const u64 lowBits = valueMasked << bitOffset;
        word = (word & ~(mask << bitOffset)) | lowBits;

        u64& nextWord = d.storage[static_cast<size_t>(wordIndex + 1)];
        const i32 highBits = bits - (64 - bitOffset);
        const u64 highValueMask = (1ULL << highBits) - 1;
        oldPaletteIndex |= static_cast<i32>((nextWord & highValueMask) << (64 - bitOffset));
        nextWord = (nextWord & ~highValueMask) | (valueMasked >> (64 - bitOffset));
    }

    return d.palette[static_cast<size_t>(oldPaletteIndex)];
}

void PalettedContainer::set(i32 index, u32 value)
{
    getAndSet(index, value);
}

// ============================================================================
// 批量操作
// ============================================================================

void PalettedContainer::fill(u32 value)
{
    Data& d = m_data;
    // 释放调色板容量（此前可能为上千项留了容量）
    d.palette.clear();
    d.palette.shrink_to_fit();
    d.palette.push_back(value);
    d.paletteSize = 1;
    d.bits = 0;

    // 释放位存储与哈希表：均匀态完全不需要它们（shrink_to_fit 让内存真正归还，
    // 而不是留在 vector 的 capacity 里）。
    d.storage.clear();
    d.storage.shrink_to_fit();
    d.hashMap.clear();
    d.hashMap.shrink_to_fit();
    d.hashMapCapacity = 0;
    d.hashMapMask = 0;
}

std::vector<u32> PalettedContainer::toFlat() const
{
    std::vector<u32> result(static_cast<size_t>(VOLUME));
    const Data& d = m_data;

    if (d.bits == 0) {
        std::fill(result.begin(), result.end(), d.palette[0]);
        return result;
    }

    for (i32 i = 0; i < VOLUME; ++i) {
        result[static_cast<size_t>(i)] = d.palette[static_cast<size_t>(_readBits(i))];
    }
    return result;
}

void PalettedContainer::fromFlat(const u32* data, i32 count)
{
    MC_ASSERT_RELEASE(count == VOLUME);
    Data& d = m_data;

    // 统计唯一值数量：排序去重
    std::vector<u32> sorted(data, data + count);
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    const i32 uniqueCount = static_cast<i32>(sorted.size());

    // 调色板按排序顺序（跨分配器拷贝），并复位为均匀态基线
    d.palette.assign(sorted.begin(), sorted.end());
    d.paletteSize = uniqueCount;
    d.bits = 0;
    d.storage.clear();
    d.hashMap.clear();
    d.hashMapCapacity = 0;
    d.hashMapMask = 0;

    if (uniqueCount == 1) {
        return; // 均匀态：无 storage，整段按 palette[0] 取值
    }

    d.bits = _calculateBitsForCount(uniqueCount);
    d.storage.assign(static_cast<size_t>(_storageWordCount(d.bits)), 0);
    _hashMapRebuild();

    for (i32 i = 0; i < VOLUME; ++i) {
        const i32 paletteIndex = _hashMapLookup(data[i]);
        MC_ASSERT_RELEASE(paletteIndex >= 0);
        _writeBits(i, paletteIndex);
    }
}

// ============================================================================
// 遍历
// ============================================================================

void PalettedContainer::forEach(const std::function<void(i32, u32)>& visitor) const
{
    const Data& d = m_data;
    if (d.bits == 0) {
        for (i32 i = 0; i < VOLUME; ++i) {
            visitor(i, d.palette[0]);
        }
        return;
    }
    for (i32 i = 0; i < VOLUME; ++i) {
        visitor(i, d.palette[static_cast<size_t>(_readBits(i))]);
    }
}

void PalettedContainer::forEachPaletteValue(const std::function<void(i32, u32)>& visitor) const
{
    const Data& d = m_data;
    for (i32 i = 0; i < d.paletteSize; ++i) {
        visitor(i, d.palette[static_cast<size_t>(i)]);
    }
}

// ============================================================================
// 状态查询
// ============================================================================

i32 PalettedContainer::paletteSize() const
{
    return m_data.paletteSize;
}

i32 PalettedContainer::bitsPerEntry() const
{
    return m_data.bits;
}

size_t PalettedContainer::estimatedMemoryUsage() const
{
    const Data& d = m_data;
    size_t size = sizeof(Data);
    size += d.storage.capacity() * sizeof(u64);
    size += d.palette.capacity() * sizeof(u32);
    size += d.hashMap.capacity() * sizeof(u32);
    return size;
}

} // namespace mc::world::chunk
