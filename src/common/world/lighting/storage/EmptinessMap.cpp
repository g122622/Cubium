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

#include "EmptinessMap.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include <cstddef>

namespace mc {

EmptinessMap::EmptinessMap(i32 minSection, i32 maxSection)
    : m_minSection(minSection)
    , m_maxSection(maxSection)
    , m_sectionCount(maxSection - minSection + 1)
    , m_sectionEmpty(static_cast<size_t>(m_sectionCount), u8(0))
{}

void EmptinessMap::setHeightRange(i32 minSection, i32 maxSection)
{
    m_minSection = minSection;
    m_maxSection = maxSection;
    m_sectionCount = maxSection - minSection + 1;
    m_sectionEmpty.resize(static_cast<size_t>(m_sectionCount));
    m_sectionEmpty.assign(static_cast<size_t>(m_sectionCount), u8(0));
}

bool EmptinessMap::updateFromChunk(const IChunk& chunk)
{
    bool changed = false;

    for (i32 sectionY = m_minSection; sectionY <= m_maxSection; ++sectionY) {
        i32 index = _sectionYToIndex(sectionY);
        if (!_isValidSectionIndex(index)) {
            continue;
        }

        // 检查区块段是否为空
        const ChunkSection* section = chunk.getSection(sectionY - m_minSection);
        bool isEmpty = (section == nullptr) || section->isEmpty();

        size_t idx = static_cast<size_t>(index);
        u8 emptyValue = isEmpty ? u8(1) : u8(0);
        if (m_sectionEmpty[idx] != emptyValue) {
            m_sectionEmpty[idx] = emptyValue;
            changed = true;
        }
    }

    return changed;
}

void EmptinessMap::reset()
{
    m_sectionEmpty.assign(static_cast<size_t>(m_sectionCount), u8(0));
}

void EmptinessMap::setAllEmpty()
{
    m_sectionEmpty.assign(static_cast<size_t>(m_sectionCount), u8(1));
}

} // namespace mc
