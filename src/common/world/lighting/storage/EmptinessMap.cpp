#include "EmptinessMap.hpp"
#include "../../chunk/ChunkData.hpp"

namespace mc {

EmptinessMap::EmptinessMap(i32 minSection, i32 maxSection)
    : m_minSection(minSection)
    , m_maxSection(maxSection)
    , m_sectionCount(maxSection - minSection + 1)
    , m_sectionEmpty(static_cast<size_t>(m_sectionCount), u8(0)) {
}

void EmptinessMap::setHeightRange(i32 minSection, i32 maxSection) {
    m_minSection = minSection;
    m_maxSection = maxSection;
    m_sectionCount = maxSection - minSection + 1;
    m_sectionEmpty.resize(static_cast<size_t>(m_sectionCount));
    m_sectionEmpty.assign(static_cast<size_t>(m_sectionCount), u8(0));
}

bool EmptinessMap::updateFromChunk(const IChunk& chunk) {
    bool changed = false;

    for (i32 sectionY = m_minSection; sectionY <= m_maxSection; ++sectionY) {
        i32 index = sectionYToIndex(sectionY);
        if (!isValidSectionIndex(index)) {
            continue;
        }

        // 检查区块段是否为空
        const ChunkSection* section = chunk.getSection(sectionY);
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

void EmptinessMap::reset() {
    m_sectionEmpty.assign(static_cast<size_t>(m_sectionCount), u8(0));
}

void EmptinessMap::setAllEmpty() {
    m_sectionEmpty.assign(static_cast<size_t>(m_sectionCount), u8(1));
}

} // namespace mc
