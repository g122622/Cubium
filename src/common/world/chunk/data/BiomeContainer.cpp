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

#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include <algorithm>
#include <bit>
#include <cstddef>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// BiomeContainer 实现
// ============================================================================

void BiomeContainer::setBiome(i32 sectionIndex, i32 x, i32 y, i32 z, BiomeId biome)
{
    MC_ASSERT_RELEASE(sectionIndex >= 0);
    MC_ASSERT_RELEASE(sectionIndex < SECTION_COUNT);
    if (x >= 0 && x < HORIZ_SIZE && y >= 0 && y < VERT_SIZE && z >= 0 && z < HORIZ_SIZE) {
        const i32 index = sectionIndex * SECTION_BIOME_SIZE + y * HORIZ_SIZE * HORIZ_SIZE + z * HORIZ_SIZE + x;
        _set(static_cast<size_t>(index), biome);
    }
}

BiomeId BiomeContainer::getBiome(i32 sectionIndex, i32 x, i32 y, i32 z) const
{
    MC_ASSERT_RELEASE(sectionIndex >= 0);
    MC_ASSERT_RELEASE(sectionIndex < SECTION_COUNT);
    if (x >= 0 && x < HORIZ_SIZE && y >= 0 && y < VERT_SIZE && z >= 0 && z < HORIZ_SIZE) {
        const i32 index = sectionIndex * SECTION_BIOME_SIZE + y * HORIZ_SIZE * HORIZ_SIZE + z * HORIZ_SIZE + x;
        return _get(static_cast<size_t>(index));
    }
    return 0;
}

BiomeId BiomeContainer::getBiomeAtBlock(i32 x, i32 y, i32 z) const
{
    // 将方块坐标映射到生物群系采样点
    // X 和 Z：每 4 个方块对应一个采样点
    const i32 biomeX = std::clamp(x >> 2, 0, HORIZ_SIZE - 1);
    const i32 biomeZ = std::clamp(z >> 2, 0, HORIZ_SIZE - 1);

    // Y：需要计算正确的 section 索引和 biome Y 索引
    // section 索引 = (y - MIN_BUILD_HEIGHT) / CHUNK_SECTION_HEIGHT
    // biome Y 索引 = ((y - MIN_BUILD_HEIGHT) % CHUNK_SECTION_HEIGHT) / 4
    const i32 yOffset = y - mc::world::MIN_BUILD_HEIGHT;
    if (yOffset < 0 || yOffset >= mc::world::CHUNK_HEIGHT) {
        return 0;
    }

    const i32 sectionIndex = yOffset / mc::world::CHUNK_SECTION_HEIGHT;
    const i32 biomeY = (yOffset % mc::world::CHUNK_SECTION_HEIGHT) >> 2;

    return getBiome(sectionIndex, biomeX, biomeY, biomeZ);
}

std::array<BiomeId, BiomeContainer::SECTION_BIOME_SIZE> BiomeContainer::sectionData(i32 sectionIndex) const
{
    MC_ASSERT_RELEASE(sectionIndex >= 0);
    MC_ASSERT_RELEASE(sectionIndex < SECTION_COUNT);
    std::array<BiomeId, SECTION_BIOME_SIZE> result;
    const size_t start = static_cast<size_t>(sectionIndex) * SECTION_BIOME_SIZE;
    for (size_t index = 0; index < result.size(); ++index) {
        result[index] = _get(start + index);
    }
    return result;
}

void BiomeContainer::setSectionData(i32 sectionIndex, std::span<const BiomeId, SECTION_BIOME_SIZE> biomes)
{
    MC_ASSERT_RELEASE(sectionIndex >= 0);
    MC_ASSERT_RELEASE(sectionIndex < SECTION_COUNT);
    const size_t start = static_cast<size_t>(sectionIndex) * SECTION_BIOME_SIZE;
    for (size_t index = 0; index < biomes.size(); ++index) {
        _set(start + index, biomes[index]);
    }
}

BiomeId BiomeContainer::_get(size_t index) const
{
    if (m_bits == 0) {
        return m_uniform;
    }
    const u16 value = _getPacked(index);
    return m_bits == 16 ? value : m_palette[value];
}

u16 BiomeContainer::_getPacked(size_t index) const
{
    const size_t perWord = 64 / m_bits;
    const size_t shift = (index % perWord) * m_bits;
    return static_cast<u16>((m_storage[index / perWord] >> shift) & ((u64{1} << m_bits) - 1));
}

void BiomeContainer::_setPacked(size_t index, u16 value)
{
    const size_t perWord = 64 / m_bits;
    const size_t shift = (index % perWord) * m_bits;
    const u64 mask = ((u64{1} << m_bits) - 1) << shift;
    auto& word = m_storage[index / perWord];
    word = (word & ~mask) | (static_cast<u64>(value) << shift);
}

void BiomeContainer::_repack(u8 bits)
{
    const u8 oldBits = m_bits;
    auto oldStorage = std::move(m_storage);
    m_bits = bits;
    const size_t perWord = 64 / bits;
    m_storage.assign((TOTAL_SIZE + perWord - 1) / perWord, 0);
    for (size_t index = 0; index < TOTAL_SIZE; ++index) {
        u16 value = 0;
        if (oldBits != 0) {
            const size_t oldPerWord = 64 / oldBits;
            value = static_cast<u16>(
                (oldStorage[index / oldPerWord] >> ((index % oldPerWord) * oldBits)) & ((u64{1} << oldBits) - 1));
        }
        // 切到直接存储时，把调色板索引转换成真实 ID；其他扩容只改变索引位宽。
        _setPacked(index, bits == 16 ? m_palette[value] : value);
    }
}

void BiomeContainer::_set(size_t index, BiomeId biome)
{
    if (_get(index) == biome) {
        return;
    }
    if (m_bits == 16) {
        _setPacked(index, biome);
        return;
    }
    if (m_bits == 0) {
        m_palette = {m_uniform, biome};
        _repack(1);
        _setPacked(index, 1);
        return;
    }

    const auto found = std::find(m_palette.begin(), m_palette.end(), biome);
    if (found != m_palette.end()) {
        _setPacked(index, static_cast<u16>(found - m_palette.begin()));
        return;
    }
    if (m_palette.size() == 512) {
        _repack(16);
        std::vector<BiomeId>().swap(m_palette);
        _setPacked(index, biome);
        return;
    }
    m_palette.push_back(biome);
    const auto bits = static_cast<u8>(std::bit_width(static_cast<u32>(m_palette.size() - 1)));
    if (bits != m_bits) {
        _repack(bits);
    }
    _setPacked(index, static_cast<u16>(m_palette.size() - 1));
}

void BiomeContainer::compact()
{
    if (m_bits == 0) {
        return;
    }
    BiomeContainer compacted;
    compacted.m_uniform = _get(0);
    for (size_t index = 1; index < TOTAL_SIZE; ++index) {
        compacted._set(index, _get(index));
    }
    compacted.m_palette.shrink_to_fit();
    *this = std::move(compacted);
}

size_t BiomeContainer::estimatedMemoryUsage() const noexcept
{
    return sizeof(*this) + m_palette.capacity() * sizeof(BiomeId) + m_storage.capacity() * sizeof(u64);
}

std::vector<u8> BiomeContainer::serialize() const
{
    std::vector<u8> data;
    data.reserve(TOTAL_SIZE * sizeof(BiomeId));
    for (size_t index = 0; index < TOTAL_SIZE; ++index) {
        const BiomeId biome = _get(index);
        data.push_back(static_cast<u8>(biome & 0xFF));
        data.push_back(static_cast<u8>((biome >> 8) & 0xFF));
    }
    return data;
}

Result<BiomeContainer> BiomeContainer::deserialize(const u8* data, size_t size)
{
    const size_t expectedSize = TOTAL_SIZE * sizeof(BiomeId);
    if (data == nullptr || size < expectedSize) {
        return Error(ErrorCode::InvalidArgument, "BiomeContainer deserialize: data too small");
    }

    BiomeContainer container;
    for (size_t i = 0; i < TOTAL_SIZE; ++i) {
        const u16 low = data[i * 2];
        const u16 high = data[i * 2 + 1];
        container._set(i, static_cast<BiomeId>(low | (high << 8)));
    }
    container.compact();
    return container;
}

} // namespace mc::world::chunk
