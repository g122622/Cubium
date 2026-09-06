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
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 静态辅助函数
// ============================================================================

i32 PalettedContainer::_calculateBitsForValue(u32 value)
{
    if (value == 0) {
        return 0;
    }
    // 计算存储 value 所需的最小位数
    i32 bits = 0;
    u32 v = value;
    while (v > 0) {
        ++bits;
        v >>= 1;
    }
    return bits;
}

i32 PalettedContainer::_calculateBitsForCount(i32 count)
{
    if (count <= 1) {
        return 0;
    }
    // ceil(log2(count))，但至少 MIN_BITS（对齐原版 MC 线性调色板至少 4 位）
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
    i32 totalBits = VOLUME * bits;
    return (totalBits + 63) / 64;
}

PalettedContainer::Mode PalettedContainer::_modeForBits(i32 bits)
{
    if (bits == 0) {
        return Mode::SingleValue;
    }
    if (bits >= MIN_BITS_FOR_FLAT) {
        return Mode::Flat;
    }
    if (bits <= MIN_BITS) {
        return Mode::Linear;
    }
    return Mode::HashMap;
}

// ============================================================================
// Data 内部方法
// ============================================================================

void PalettedContainer::Data::refreshRawPalette()
{
    if (mode == Mode::Flat) {
        rawPalettePtr = nullptr; // Flat 模式不使用调色板
        return;
    }
    rawPalettePtr = palette.empty() ? nullptr : palette.data();
}

// ============================================================================
// 构造 / 赋值
// ============================================================================

PalettedContainer::PalettedContainer()
{
    // 默认 SingleValue 模式，值为 0（空气）
    m_data.mode = Mode::SingleValue;
    m_data.singleValue = 0;
    m_data.palette.push_back(0);
    m_data.paletteSize = 1;
    m_data.refreshRawPalette();
}

PalettedContainer::PalettedContainer(const PalettedContainer& other)
    : m_data(other.m_data)
{
    m_data.refreshRawPalette();
}

PalettedContainer::PalettedContainer(PalettedContainer&& other) noexcept
    : m_data(std::move(other.m_data))
{}

PalettedContainer& PalettedContainer::operator=(const PalettedContainer& other)
{
    if (this != &other) {
        m_data = other.m_data;
        m_data.refreshRawPalette();
    }
    return *this;
}

PalettedContainer& PalettedContainer::operator=(PalettedContainer&& other) noexcept
{
    if (this != &other) {
        m_data = std::move(other.m_data);
    }
    return *this;
}

// ============================================================================
// 位存储读写
// ============================================================================

i32 PalettedContainer::_readBits(i32 index) const
{
    const Data& d = m_data;
    if (d.bits == 0) {
        return 0; // SingleValue 模式，storage 为空
    }

    i32 bitIndex = index * d.bits;
    i32 wordIndex = bitIndex / 64;
    i32 bitOffset = bitIndex % 64;

    u64 word = d.storage[static_cast<size_t>(wordIndex)];
    u64 mask = (d.bits >= 64) ? ~0ULL : ((1ULL << d.bits) - 1);

    // 处理跨字情况
    if (bitOffset + d.bits <= 64) {
        return static_cast<i32>((word >> bitOffset) & mask);
    }

    // 跨两个字
    u64 low = (word >> bitOffset);
    u64 high = static_cast<u64>(d.storage[static_cast<size_t>(wordIndex + 1)]) << (64 - bitOffset);
    return static_cast<i32>((low | high) & mask);
}

i32 PalettedContainer::_writeBits(i32 index, i32 value)
{
    Data& d = m_data;
    if (d.bits == 0) {
        return 0; // SingleValue 模式，无 storage
    }

    i32 bitIndex = index * d.bits;
    i32 wordIndex = bitIndex / 64;
    i32 bitOffset = bitIndex % 64;

    u64 mask = (d.bits >= 64) ? ~0ULL : ((1ULL << d.bits) - 1);
    u64 valueMasked = static_cast<u64>(value) & mask;

    u64& word = d.storage[static_cast<size_t>(wordIndex)];
    u64 old;

    if (bitOffset + d.bits <= 64) {
        old = (word >> bitOffset) & mask;
        word = (word & ~(mask << bitOffset)) | (valueMasked << bitOffset);
    } else {
        // 跨两个字
        old = (word >> bitOffset) & mask;
        u64 lowBits = valueMasked << bitOffset;
        word = (word & ~(mask << bitOffset)) | lowBits;

        u64& nextWord = d.storage[static_cast<size_t>(wordIndex + 1)];
        i32 highBits = d.bits - (64 - bitOffset);
        u64 highValueMask = (1ULL << highBits) - 1;
        old |= (nextWord & highValueMask) << (64 - bitOffset);
        nextWord = (nextWord & ~highValueMask) | (valueMasked >> (64 - bitOffset));
    }

    return static_cast<i32>(old);
}

i32 PalettedContainer::_getAndSetBits(i32 index, i32 value)
{
    return _writeBits(index, value);
}

// ============================================================================
// 调色板查找
// ============================================================================

u32 PalettedContainer::_paletteLookup(i32 paletteIndex) const
{
    const Data& d = m_data;
    if (d.mode == Mode::SingleValue) {
        return d.singleValue;
    }
    if (d.mode == Mode::Flat) {
        // Flat 模式下 storage 直接存 stateId
        return static_cast<u32>(paletteIndex);
    }
    // Linear / HashMap
    MC_ASSERT_RELEASE(paletteIndex >= 0 && paletteIndex < d.paletteSize);
    return d.palette[static_cast<size_t>(paletteIndex)];
}

i32 PalettedContainer::_hashMapLookup(u32 value) const
{
    const Data& d = m_data;
    if (d.hashMapCapacity == 0) {
        return -1;
    }
    // 开放寻址哈希，使用 value 的位混合作为哈希
    u32 hash = value * 2654435761u; // Knuth 乘法哈希
    i32 slot = static_cast<i32>(hash) & d.hashMapMask;
    for (i32 i = 0; i < d.hashMapCapacity; ++i) {
        u32 entry = d.hashMap[static_cast<size_t>(slot)];
        if (entry == 0) {
            return -1; // 空槽
        }
        i32 idx = static_cast<i32>(entry) - 1;
        if (d.palette[static_cast<size_t>(idx)] == value) {
            return idx;
        }
        slot = (slot + 1) & d.hashMapMask;
    }
    return -1;
}

void PalettedContainer::_hashMapInsert(u32 value, i32 paletteIndex)
{
    Data& d = m_data;
    // 负载因子 > 0.75 时扩容
    if ((d.paletteSize + 1) * 4 > d.hashMapCapacity * 3) {
        _hashMapRebuild();
    }

    u32 hash = value * 2654435761u;
    i32 slot = static_cast<i32>(hash) & d.hashMapMask;
    while (true) {
        if (d.hashMap[static_cast<size_t>(slot)] == 0) {
            d.hashMap[static_cast<size_t>(slot)] = static_cast<u32>(paletteIndex + 1);
            return;
        }
        slot = (slot + 1) & d.hashMapMask;
    }
}

void PalettedContainer::_hashMapRebuild()
{
    Data& d = m_data;
    // 确保容量足够容纳所有条目（负载因子 <= 0.75）
    // 容量必须是 2 的幂
    i32 minCapacity = (d.paletteSize + 1) * 4 / 3; // 满足 0.75 负载因子
    i32 newCapacity = d.hashMapCapacity == 0 ? 16 : d.hashMapCapacity * 2;
    while (newCapacity < minCapacity) {
        newCapacity *= 2;
    }
    d.hashMap.assign(static_cast<size_t>(newCapacity), 0);
    d.hashMapCapacity = newCapacity;
    d.hashMapMask = newCapacity - 1;

    // 重建所有现有条目
    for (i32 i = 0; i < d.paletteSize; ++i) {
        u32 value = d.palette[static_cast<size_t>(i)];
        u32 hash = value * 2654435761u;
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

    if (d.mode == Mode::SingleValue) {
        if (d.singleValue == value) {
            return 0;
        }
        // 需要转换为 Linear
        _transitionSingleToLinear(d.singleValue, value, -1);
        // 转换后，调色板有 [oldValue, value]，value 的索引为 1
        return 1;
    }

    if (d.mode == Mode::Flat) {
        // Flat 模式直接返回 stateId 作为索引
        return static_cast<i32>(value);
    }

    if (d.mode == Mode::Linear) {
        // 线性扫描
        for (i32 i = 0; i < d.paletteSize; ++i) {
            if (d.palette[static_cast<size_t>(i)] == value) {
                return i;
            }
        }
        // 未找到，添加到调色板
        if (d.paletteSize < static_cast<i32>(d.palette.size())) {
            d.palette[static_cast<size_t>(d.paletteSize)] = value;
        } else {
            d.palette.push_back(value);
        }
        i32 newIndex = d.paletteSize;
        ++d.paletteSize;
        d.refreshRawPalette();

        // 检查是否需要扩容位数
        i32 newBits = _calculateBitsForCount(d.paletteSize);
        if (newBits > d.bits) {
            _onResize(newBits);
        }
        return newIndex;
    }

    // HashMap 模式
    i32 existing = _hashMapLookup(value);
    if (existing >= 0) {
        return existing;
    }
    // 未找到，添加到调色板
    if (d.paletteSize < static_cast<i32>(d.palette.size())) {
        d.palette[static_cast<size_t>(d.paletteSize)] = value;
    } else {
        d.palette.push_back(value);
    }
    i32 newIndex = d.paletteSize;
    ++d.paletteSize;
    _hashMapInsert(value, newIndex);
    d.refreshRawPalette();

    // 检查是否需要扩容位数
    i32 newBits = _calculateBitsForCount(d.paletteSize);
    if (newBits > d.bits) {
        _onResize(newBits);
    }
    return newIndex;
}

// ============================================================================
// Resize — 扩容位数和模式转换
// ============================================================================

void PalettedContainer::_onResize(i32 newBits)
{
    Data& d = m_data;
    const i32 oldBits = d.bits;
    const Mode oldMode = d.mode;

    // 保存旧 storage，后续用 oldBits 从中读取 paletteIndex。
    std::vector<u64, PaletteStorageAlloc<u64>> oldStorage = std::move(d.storage);

    // 分配新 storage（用 newBits 的 word count），更新 bits/mode。
    d.storage.assign(static_cast<size_t>(_storageWordCount(newBits)), 0);
    d.bits = newBits;
    const Mode newMode = _modeForBits(newBits);
    d.mode = newMode;

    // 读取旧 paletteIndex（oldBits）的内联位运算。
    // 注意：oldStorage 此时已是空（被 move 走），用 oldStorage 引用。
    auto readOldPaletteIndex = [&oldStorage, oldBits](i32 i) -> i32 {
        i32 oldBitIndex = i * oldBits;
        i32 oldWordIndex = oldBitIndex >> 6;
        i32 oldBitOffset = oldBitIndex & 63;
        u64 oldMask = (oldBits >= 64) ? ~0ULL : ((1ULL << oldBits) - 1);
        u64 oldWord = oldStorage[static_cast<size_t>(oldWordIndex)];
        if (oldBitOffset + oldBits <= 64) {
            return static_cast<i32>((oldWord >> oldBitOffset) & oldMask);
        }
        // 跨两个字
        u64 low = (oldWord >> oldBitOffset);
        u64 high = static_cast<u64>(oldStorage[static_cast<size_t>(oldWordIndex + 1)]) << (64 - oldBitOffset);
        return static_cast<i32>((low | high) & oldMask);
    };

    if (newMode == Mode::Flat) {
        // 转为 Flat：旧 storage 存的是 paletteIndex，新 storage 直接存 stateId。
        // 此时 palette 还在（尚未 clear），用 paletteIndex 查 stateId。
        // 旧模式可能是 Linear/HashMap（storage 存 paletteIndex）或 Flat（已存 stateId）。
        if (oldMode == Mode::Flat) {
            // Flat → Flat：直接复制 paletteIndex（即 stateId）
            for (i32 i = 0; i < VOLUME; ++i) {
                _writeBits(i, readOldPaletteIndex(i));
            }
        } else {
            // Linear/HashMap → Flat：paletteIndex → stateId
            for (i32 i = 0; i < VOLUME; ++i) {
                i32 paletteIndex = readOldPaletteIndex(i);
                _writeBits(i, static_cast<i32>(d.palette[static_cast<size_t>(paletteIndex)]));
            }
        }
        // Flat 模式清空调色板与哈希表
        d.palette.clear();
        d.paletteSize = 0;
        d.hashMap.clear();
        d.hashMapCapacity = 0;
        d.hashMapMask = 0;
        d.rawPalettePtr = nullptr;
        return;
    }

    // Linear / HashMap：保留调色板，重建 storage
    // 调色板顺序不变，bits 变了，但 paletteIndex 值不变——直接搬运。
    if (newMode == Mode::HashMap && d.hashMapCapacity == 0) {
        _hashMapRebuild();
    }
    d.refreshRawPalette();

    // paletteIndex 在 resize 前后语义不变（指向同一 palette 条目），
    // 直接从旧 storage 读 paletteIndex，用新 bits 写入新 storage。
    for (i32 i = 0; i < VOLUME; ++i) {
        i32 paletteIndex = readOldPaletteIndex(i);
        _writeBits(i, paletteIndex);
    }
}

void PalettedContainer::_transitionSingleToLinear(u32 existingValue, u32 newValue, i32 index)
{
    Data& d = m_data;
    MC_ASSERT_RELEASE(d.mode == Mode::SingleValue);

    i32 bits = MIN_BITS; // 4 位
    d.storage.assign(static_cast<size_t>(_storageWordCount(bits)), 0);
    d.bits = bits;
    d.mode = Mode::Linear;
    d.palette.clear();
    d.palette.push_back(existingValue);
    d.palette.push_back(newValue);
    d.paletteSize = 2;
    d.refreshRawPalette();

    // 所有现有条目设为索引 0（existingValue）
    for (i32 i = 0; i < VOLUME; ++i) {
        _writeBits(i, 0);
    }

    // 如果指定了新值的索引，设置它
    if (index >= 0) {
        _writeBits(index, 1);
    }
}

// ============================================================================
// 元素访问
// ============================================================================

u32 PalettedContainer::get(i32 index) const
{
    MC_ASSERT_RELEASE(index >= 0 && index < VOLUME);
    const Data& d = m_data;

    if (d.mode == Mode::SingleValue) {
        return d.singleValue;
    }
    if (d.mode == Mode::Flat) {
        return static_cast<u32>(_readBits(index));
    }
    // Linear / HashMap
    i32 paletteIndex = _readBits(index);
    return d.palette[static_cast<size_t>(paletteIndex)];
}

u32 PalettedContainer::getAndSet(i32 index, u32 value)
{
    MC_ASSERT_RELEASE(index >= 0 && index < VOLUME);
    Data& d = m_data;

    if (d.mode == Mode::SingleValue) {
        u32 old = d.singleValue;
        if (value == old) {
            return old;
        }
        _transitionSingleToLinear(old, value, index);
        return old;
    }

    if (d.mode == Mode::Flat) {
        return static_cast<u32>(_getAndSetBits(index, static_cast<i32>(value)));
    }

    // Linear / HashMap：先 _idFor 取 paletteIndex（可能触发 _onResize 扩容），
    // 再用内联位运算写入 storage 并读回旧 paletteIndex。
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
    d.mode = Mode::SingleValue;
    d.singleValue = value;
    d.storage.clear();
    d.bits = 0;
    d.palette.clear();
    d.palette.push_back(value);
    d.paletteSize = 1;
    d.hashMap.clear();
    d.hashMapCapacity = 0;
    d.hashMapMask = 0;
    d.refreshRawPalette();
}

std::vector<u32> PalettedContainer::toFlat() const
{
    std::vector<u32> result(static_cast<size_t>(VOLUME));
    const Data& d = m_data;

    if (d.mode == Mode::SingleValue) {
        std::fill(result.begin(), result.end(), d.singleValue);
        return result;
    }

    if (d.mode == Mode::Flat) {
        for (i32 i = 0; i < VOLUME; ++i) {
            result[static_cast<size_t>(i)] = static_cast<u32>(_readBits(i));
        }
        return result;
    }

    // Linear / HashMap
    for (i32 i = 0; i < VOLUME; ++i) {
        i32 paletteIndex = _readBits(i);
        result[static_cast<size_t>(i)] = d.palette[static_cast<size_t>(paletteIndex)];
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
    i32 uniqueCount = static_cast<i32>(sorted.size());

    if (uniqueCount == 1) {
        // SingleValue
        d.mode = Mode::SingleValue;
        d.singleValue = sorted[0];
        d.storage.clear();
        d.bits = 0;
        d.palette.clear();
        d.palette.push_back(sorted[0]);
        d.paletteSize = 1;
        d.hashMap.clear();
        d.hashMapCapacity = 0;
        d.hashMapMask = 0;
        d.refreshRawPalette();
        return;
    }

    if (uniqueCount >= (1 << MIN_BITS_FOR_FLAT)) {
        // Flat（VOLUME=4096 时不可达，保留兜底）
        d.mode = Mode::Flat;
        d.storage.assign(static_cast<size_t>(_storageWordCount(MIN_BITS_FOR_FLAT)), 0);
        d.bits = MIN_BITS_FOR_FLAT;
        d.palette.clear();
        d.paletteSize = 0;
        d.hashMap.clear();
        d.hashMapCapacity = 0;
        d.hashMapMask = 0;
        d.rawPalettePtr = nullptr;
        for (i32 i = 0; i < VOLUME; ++i) {
            _writeBits(i, static_cast<i32>(data[i]));
        }
        return;
    }

    // Linear 或 HashMap
    const i32 bits = _calculateBitsForCount(uniqueCount);
    const Mode mode = _modeForBits(bits);
    d.storage.assign(static_cast<size_t>(_storageWordCount(bits)), 0);
    d.bits = bits;
    d.mode = mode;
    d.palette.assign(sorted.begin(), sorted.end()); // 调色板按排序顺序（跨分配器拷贝）
    d.paletteSize = uniqueCount;
    d.hashMap.clear();
    d.hashMapCapacity = 0;
    d.hashMapMask = 0;

    if (mode == Mode::HashMap) {
        _hashMapRebuild();
    }
    d.refreshRawPalette();

    // 构建 value → paletteIndex 的开放寻址哈希表，O(1) 查找替代 std::lower_bound。
    // 哈希槽存储 (paletteIndex + 1)，0 表示空。palette 已排序，paletteIndex 即排序后位置。
    i32 hashCap = 16;
    while (hashCap < uniqueCount * 2) {
        hashCap <<= 1;
    }
    const i32 hashMask = hashCap - 1;
    std::vector<u32> hashTable(static_cast<size_t>(hashCap), 0);

    auto hashInsert = [&](u32 value, i32 paletteIndex) {
        u32 hash = value * 2654435761u;
        i32 slot = static_cast<i32>(hash) & hashMask;
        while (hashTable[static_cast<size_t>(slot)] != 0) {
            slot = (slot + 1) & hashMask;
        }
        hashTable[static_cast<size_t>(slot)] = static_cast<u32>(paletteIndex + 1);
    };

    auto hashLookup = [&](u32 value) -> i32 {
        u32 hash = value * 2654435761u;
        i32 slot = static_cast<i32>(hash) & hashMask;
        for (i32 i = 0; i < hashCap; ++i) {
            u32 entry = hashTable[static_cast<size_t>(slot)];
            if (entry == 0) {
                return -1;
            }
            i32 idx = static_cast<i32>(entry) - 1;
            if (d.palette[static_cast<size_t>(idx)] == value) {
                return idx;
            }
            slot = (slot + 1) & hashMask;
        }
        return -1;
    };

    for (i32 i = 0; i < uniqueCount; ++i) {
        hashInsert(d.palette[static_cast<size_t>(i)], i);
    }

    for (i32 i = 0; i < VOLUME; ++i) {
        u32 value = data[i];
        i32 paletteIndex = hashLookup(value);
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
    if (d.mode == Mode::SingleValue) {
        for (i32 i = 0; i < VOLUME; ++i) {
            visitor(i, d.singleValue);
        }
        return;
    }

    if (d.mode == Mode::Flat) {
        for (i32 i = 0; i < VOLUME; ++i) {
            visitor(i, static_cast<u32>(_readBits(i)));
        }
        return;
    }

    // Linear / HashMap
    for (i32 i = 0; i < VOLUME; ++i) {
        i32 paletteIndex = _readBits(i);
        visitor(i, d.palette[static_cast<size_t>(paletteIndex)]);
    }
}

void PalettedContainer::forEachPaletteValue(const std::function<void(i32, u32)>& visitor) const
{
    const Data& d = m_data;
    if (d.mode == Mode::SingleValue) {
        visitor(0, d.singleValue);
        return;
    }
    if (d.mode == Mode::Flat) {
        // Flat 模式没有调色板，遍历所有值
        std::vector<u32> flat = toFlat();
        std::sort(flat.begin(), flat.end());
        flat.erase(std::unique(flat.begin(), flat.end()), flat.end());
        for (size_t i = 0; i < flat.size(); ++i) {
            visitor(static_cast<i32>(i), flat[i]);
        }
        return;
    }
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

u32 PalettedContainer::paletteValue(i32 paletteIndex) const
{
    return _paletteLookup(paletteIndex);
}

const u32* PalettedContainer::rawPalette() const
{
    return m_data.rawPalettePtr;
}

const std::vector<u64, PaletteStorageAlloc<u64>>& PalettedContainer::storage() const
{
    return m_data.storage;
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
