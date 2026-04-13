#include "common/world/chunk/ThreadedTicketLevelPropagator.hpp"
#include "common/util/concurrent/ReentrantAreaLock.hpp"
#include <algorithm>
#include <cstring>

namespace mc::world {

// ============================================================================
// Constants for propagation
// ============================================================================

namespace {

// Propagation direction bitset for all 8 neighbors
// Index = x | (z << 2), centered at (1,1)
constexpr u64 ALL_DIRECTIONS_BITSET =
    // z = -1 (neighbors at y=-1)
    (1ULL << ((0) | ((0) << 2))) |
    (1ULL << ((1) | ((0) << 2))) |
    (1ULL << ((2) | ((0) << 2))) |
    // z = 0 (neighbors at y=0)
    (1ULL << ((0) | ((1) << 2))) |
    (1ULL << ((2) | ((1) << 2))) |
    // z = 1 (neighbors at y=1)
    (1ULL << ((0) | ((2) << 2))) |
    (1ULL << ((1) | ((2) << 2))) |
    (1ULL << ((2) | ((2) << 2)));

// Special flags for propagation queue entries
constexpr u64 FLAG_WRITE_LEVEL = (1ULL << 63);      // Write level to position
constexpr u64 FLAG_RECHECK_LEVEL = (1ULL << 62);    // Recheck current level

// Coordinate bits for propagation encoding
constexpr i32 COORDINATE_BITS = 9;  // Bits to encode coordinates
constexpr i32 COORDINATE_SIZE = 1 << COORDINATE_BITS;

i32 countBits(i32 value) {
    i32 count = 0;
    while (value) {
        count += value & 1;
        value >>= 1;
    }
    return count;
}

i32 countTrailingZeros(i32 value) {
    if (value == 0) return 32;
    i32 count = 0;
    while ((value & 1) == 0) {
        count++;
        value >>= 1;
    }
    return count;
}

} // anonymous namespace

// ============================================================================
// Propagator (internal helper class)
// ============================================================================

class Propagator {
public:
    static constexpr i32 SECTION_RADIUS = 2;
    static constexpr i32 SECTION_CACHE_WIDTH = 2 * SECTION_RADIUS + 1;
    static constexpr i32 PROP_SECTION_SHIFT = ThreadedTicketLevelPropagator::SECTION_SHIFT;
    static constexpr i32 PROP_SECTION_SIZE = ThreadedTicketLevelPropagator::SECTION_SIZE;

    ThreadedTicketLevelPropagator::Section* m_sections[SECTION_CACHE_WIDTH * SECTION_CACHE_WIDTH]{};
    i32 m_encodeOffsetX = 0;
    i32 m_encodeOffsetZ = 0;
    i32 m_coordinateOffset = 0;
    i32 m_sectionIndexOffset = 0;

    std::vector<u64> m_increaseQueue;
    std::vector<u64> m_decreaseQueue;
    i32 m_increaseQueueLen = 0;
    i32 m_decreaseQueueLen = 0;

    std::unordered_map<u64, u8> m_updatedPositions;

    Propagator() {
        m_increaseQueue.resize(PROP_SECTION_SIZE * PROP_SECTION_SIZE * 2);
        m_decreaseQueue.resize(PROP_SECTION_SIZE * PROP_SECTION_SIZE * 2);
    }

    bool hasUpdates() const {
        return m_decreaseQueueLen != 0 || m_increaseQueueLen != 0;
    }

    void setupEncodeOffset(i32 centerSectionX, i32 centerSectionZ) {
        constexpr i32 maxCoordinate = (SECTION_RADIUS * PROP_SECTION_SIZE - 1);
        m_encodeOffsetX = maxCoordinate - (centerSectionX << PROP_SECTION_SHIFT);
        m_encodeOffsetZ = maxCoordinate - (centerSectionZ << PROP_SECTION_SHIFT);
        m_coordinateOffset = m_encodeOffsetX + (m_encodeOffsetZ << COORDINATE_BITS);
        m_sectionIndexOffset = (SECTION_RADIUS - centerSectionX) +
                               (SECTION_RADIUS - centerSectionZ) * SECTION_CACHE_WIDTH;
    }

    void setupCaches(ThreadedTicketLevelPropagator* propagator,
                     i32 centerSectionX, i32 centerSectionZ, i32 rad) {
        for (i32 dz = -rad; dz <= rad; ++dz) {
            for (i32 dx = -rad; dx <= rad; ++dx) {
                i32 sectionX = centerSectionX + dx;
                i32 sectionZ = centerSectionZ + dz;
                auto* section = propagator->getOrCreateSection(sectionX, sectionZ);
                setSectionInCache(sectionX, sectionZ, section);
            }
        }
    }

    void destroyCaches() {
        std::memset(m_sections, 0, sizeof(m_sections));
    }

    u8 getLevel(i32 posX, i32 posZ) {
        auto* section = m_sections[(posX >> PROP_SECTION_SHIFT) +
                                    SECTION_CACHE_WIDTH * (posZ >> PROP_SECTION_SHIFT) +
                                    m_sectionIndexOffset];
        if (section != nullptr) {
            u16 localIndex = ThreadedTicketLevelPropagator::Section::getLocalIndex(posX, posZ);
            return section->getLevel(localIndex);
        }
        return 0;
    }

    void setLevel(i32 posX, i32 posZ, u8 level) {
        auto* section = m_sections[(posX >> PROP_SECTION_SHIFT) +
                                    SECTION_CACHE_WIDTH * (posZ >> PROP_SECTION_SHIFT) +
                                    m_sectionIndexOffset];
        if (section != nullptr) {
            u16 localIndex = ThreadedTicketLevelPropagator::Section::getLocalIndex(posX, posZ);
            section->setLevel(localIndex, level);
            m_updatedPositions[ThreadedTicketLevelPropagator::posToKey(posX, posZ)] = level;
        }
    }

    void appendToIncreaseQueue(u64 value) {
        if (m_increaseQueueLen >= static_cast<i32>(m_increaseQueue.size())) {
            m_increaseQueue.resize(m_increaseQueue.size() * 2);
        }
        m_increaseQueue[m_increaseQueueLen++] = value;
    }

    void appendToDecreaseQueue(u64 value) {
        if (m_decreaseQueueLen >= static_cast<i32>(m_decreaseQueue.size())) {
            m_decreaseQueue.resize(m_decreaseQueue.size() * 2);
        }
        m_decreaseQueue[m_decreaseQueueLen++] = value;
    }

    void performIncrease() {
        constexpr i32 LEVEL_BITS = ThreadedTicketLevelPropagator::LEVEL_BITS;
        constexpr i32 LEVEL_COUNT = ThreadedTicketLevelPropagator::LEVEL_COUNT;

        i32 queueReadIndex = 0;
        i32 queueLength = m_increaseQueueLen;
        m_increaseQueueLen = 0;

        const i32 decodeOffsetX = -m_encodeOffsetX;
        const i32 decodeOffsetZ = -m_encodeOffsetZ;
        const i32 encodeOffset = m_coordinateOffset;
        const i32 sectionOffset = m_sectionIndexOffset;

        while (queueReadIndex < queueLength) {
            u64 queueValue = m_increaseQueue[queueReadIndex++];

            i32 posX = (static_cast<i32>(queueValue) & (COORDINATE_SIZE - 1)) + decodeOffsetX;
            i32 posZ = ((static_cast<i32>(queueValue) >> COORDINATE_BITS) & (COORDINATE_SIZE - 1)) + decodeOffsetZ;
            i32 propagatedLevel = (static_cast<i32>(queueValue) >> (COORDINATE_BITS + COORDINATE_BITS)) & (LEVEL_COUNT - 1);
            i32 propagateDirectionBitset = static_cast<i32>(queueValue >> (COORDINATE_BITS + COORDINATE_BITS + LEVEL_BITS)) & 0xFFFF;

            if ((queueValue & FLAG_RECHECK_LEVEL) != 0) {
                if (getLevel(posX, posZ) != propagatedLevel) {
                    continue;
                }
            } else if ((queueValue & FLAG_WRITE_LEVEL) != 0) {
                setLevel(posX, posZ, static_cast<u8>(propagatedLevel));
            }

            // Current propagation bitset (excludes center)
            u64 currentPropagation = ~(
                // z = -1
                (1ULL << ((1) | ((1) << 3))) |
                (1ULL << ((2) | ((1) << 3))) |
                (1ULL << ((3) | ((1) << 3))) |
                // z = 0
                (1ULL << ((1) | ((2) << 3))) |
                (1ULL << ((2) | ((2) << 3))) |
                (1ULL << ((3) | ((2) << 3))) |
                // z = 1
                (1ULL << ((1) | ((3) << 3))) |
                (1ULL << ((2) | ((3) << 3))) |
                (1ULL << ((3) | ((3) << 3)))
            );

            i32 toPropagate = propagatedLevel + 1;  // Neighbors get higher level (lower priority = further from source)

            for (i32 l = 0, len = countBits(propagateDirectionBitset); l < len; ++l) {
                i32 set = countTrailingZeros(propagateDirectionBitset);
                i32 tailingBit = (-propagateDirectionBitset) & propagateDirectionBitset;
                propagateDirectionBitset ^= tailingBit;

                i32 pDecodeX = set & 3;
                i32 pDecodeZ = (set >> 2) & 3;

                i32 offX = (posX - 1) + pDecodeX;
                i32 offZ = (posZ - 1) + pDecodeZ;

                i32 sectionIndex = (offX >> PROP_SECTION_SHIFT) +
                                   ((offZ >> PROP_SECTION_SHIFT) * SECTION_CACHE_WIDTH) +
                                   sectionOffset;
                u16 localIndex = static_cast<u16>((offX & (PROP_SECTION_SIZE - 1)) |
                                                   ((offZ & (PROP_SECTION_SIZE - 1)) << PROP_SECTION_SHIFT));

                i32 start = pDecodeX | (pDecodeZ << 3);
                u64 bitsetLine1 = currentPropagation & (7ULL << start);
                u64 bitsetLine2 = currentPropagation & (7ULL << (start + 8));
                u64 bitsetLine3 = currentPropagation & (7ULL << (start + 16));

                currentPropagation ^= (bitsetLine1 | bitsetLine2 | bitsetLine3);

                auto* section = m_sections[sectionIndex];
                if (section == nullptr) continue;

                u16 currentStoredLevel = section->levels[localIndex];
                i32 currentLevel = currentStoredLevel & 0xFF;

                // Level 0 means uninitialized (no level set), treat as MAX_LEVEL
                if (currentLevel == 0) {
                    currentLevel = ThreadedTicketLevelPropagator::MAX_LEVEL;
                }

                // For increase propagation: only update if current level is HIGHER (worse) than toPropagate
                // Lower level = higher priority = closer to source
                if (currentLevel <= toPropagate) {
                    continue;  // Already at better or equal level
                }

                // Update level
                section->levels[localIndex] = (currentStoredLevel & 0xFF00) | static_cast<u16>(toPropagate);
                m_updatedPositions[ThreadedTicketLevelPropagator::posToKey(offX, offZ)] = static_cast<u8>(toPropagate);

                // Queue next propagation
                if (toPropagate < LEVEL_COUNT - 1) {
                    u64 childPropagation =
                        ((bitsetLine1 >> start) << (COORDINATE_BITS + COORDINATE_BITS + LEVEL_BITS)) |
                        ((bitsetLine2 >> (start + 8)) << (4 + COORDINATE_BITS + COORDINATE_BITS + LEVEL_BITS)) |
                        ((bitsetLine3 >> (start + 16)) << (8 + COORDINATE_BITS + COORDINATE_BITS + LEVEL_BITS));

                    if (queueLength >= static_cast<i32>(m_increaseQueue.size())) {
                        m_increaseQueue.resize(m_increaseQueue.size() * 2);
                    }
                    m_increaseQueue[queueLength++] =
                        ((static_cast<u64>(offX + (offZ << COORDINATE_BITS) + encodeOffset) &
                          ((1ULL << (COORDINATE_BITS + COORDINATE_BITS)) - 1))) |
                        ((static_cast<u64>(toPropagate) & (LEVEL_COUNT - 1)) << (COORDINATE_BITS + COORDINATE_BITS)) |
                        childPropagation;
                }
            }
        }
    }

    void performDecrease() {
        constexpr i32 LEVEL_BITS = ThreadedTicketLevelPropagator::LEVEL_BITS;
        constexpr i32 LEVEL_COUNT = ThreadedTicketLevelPropagator::LEVEL_COUNT;

        i32 queueReadIndex = 0;
        i32 queueLength = m_decreaseQueueLen;
        m_decreaseQueueLen = 0;
        i32 increaseQueueLength = m_increaseQueueLen;

        const i32 decodeOffsetX = -m_encodeOffsetX;
        const i32 decodeOffsetZ = -m_encodeOffsetZ;
        const i32 encodeOffset = m_coordinateOffset;
        const i32 sectionOffset = m_sectionIndexOffset;

        while (queueReadIndex < queueLength) {
            u64 queueValue = m_decreaseQueue[queueReadIndex++];

            i32 posX = (static_cast<i32>(queueValue) & (COORDINATE_SIZE - 1)) + decodeOffsetX;
            i32 posZ = ((static_cast<i32>(queueValue) >> COORDINATE_BITS) & (COORDINATE_SIZE - 1)) + decodeOffsetZ;
            i32 propagatedLevel = (static_cast<i32>(queueValue) >> (COORDINATE_BITS + COORDINATE_BITS)) & (LEVEL_COUNT - 1);
            i32 propagateDirectionBitset = static_cast<i32>(queueValue >> (COORDINATE_BITS + COORDINATE_BITS + LEVEL_BITS)) & 0xFFFF;

            u64 currentPropagation = ~(
                (1ULL << ((1) | ((1) << 3))) |
                (1ULL << ((2) | ((1) << 3))) |
                (1ULL << ((3) | ((1) << 3))) |
                (1ULL << ((1) | ((2) << 3))) |
                (1ULL << ((2) | ((2) << 3))) |
                (1ULL << ((3) | ((2) << 3))) |
                (1ULL << ((1) | ((3) << 3))) |
                (1ULL << ((2) | ((3) << 3))) |
                (1ULL << ((3) | ((3) << 3)))
            );

            i32 toPropagate = propagatedLevel - 1;

            for (i32 l = 0, len = countBits(propagateDirectionBitset); l < len; ++l) {
                i32 set = countTrailingZeros(propagateDirectionBitset);
                i32 tailingBit = (-propagateDirectionBitset) & propagateDirectionBitset;
                propagateDirectionBitset ^= tailingBit;

                i32 pDecodeX = set & 3;
                i32 pDecodeZ = (set >> 2) & 3;

                i32 offX = (posX - 1) + pDecodeX;
                i32 offZ = (posZ - 1) + pDecodeZ;

                i32 sectionIndex = (offX >> PROP_SECTION_SHIFT) +
                                   ((offZ >> PROP_SECTION_SHIFT) * SECTION_CACHE_WIDTH) +
                                   sectionOffset;
                u16 localIndex = static_cast<u16>((offX & (PROP_SECTION_SIZE - 1)) |
                                                   ((offZ & (PROP_SECTION_SIZE - 1)) << PROP_SECTION_SHIFT));

                i32 start = pDecodeX | (pDecodeZ << 3);
                u64 bitsetLine1 = currentPropagation & (7ULL << start);
                u64 bitsetLine2 = currentPropagation & (7ULL << (start + 8));
                u64 bitsetLine3 = currentPropagation & (7ULL << (start + 16));

                auto* section = m_sections[sectionIndex];
                if (section == nullptr) continue;

                u16 currentStoredLevel = section->levels[localIndex];
                i32 currentLevel = currentStoredLevel & 0xFF;
                i32 sourceLevel = (currentStoredLevel >> 8) & 0xFF;

                if (currentLevel == 0) {
                    continue;  // Already at 0
                }

                if (currentLevel > toPropagate) {
                    // Another source propagated here, re-propagate it
                    if (increaseQueueLength >= static_cast<i32>(m_increaseQueue.size())) {
                        m_increaseQueue.resize(m_increaseQueue.size() * 2);
                    }
                    m_increaseQueue[increaseQueueLength++] =
                        ((static_cast<u64>(offX + (offZ << COORDINATE_BITS) + encodeOffset) &
                          ((1ULL << (COORDINATE_BITS + COORDINATE_BITS)) - 1))) |
                        ((static_cast<u64>(currentLevel) & (LEVEL_COUNT - 1)) << (COORDINATE_BITS + COORDINATE_BITS)) |
                        (FLAG_RECHECK_LEVEL | (ALL_DIRECTIONS_BITSET << (COORDINATE_BITS + COORDINATE_BITS + LEVEL_BITS)));
                    continue;
                }

                // Update level to 0
                section->levels[localIndex] = currentStoredLevel & 0xFF00;  // Keep source, clear level
                m_updatedPositions[ThreadedTicketLevelPropagator::posToKey(offX, offZ)] = 0;

                if (sourceLevel != 0) {
                    // Re-propagate source
                    if (increaseQueueLength >= static_cast<i32>(m_increaseQueue.size())) {
                        m_increaseQueue.resize(m_increaseQueue.size() * 2);
                    }
                    m_increaseQueue[increaseQueueLength++] =
                        ((static_cast<u64>(offX + (offZ << COORDINATE_BITS) + encodeOffset) &
                          ((1ULL << (COORDINATE_BITS + COORDINATE_BITS)) - 1))) |
                        ((static_cast<u64>(sourceLevel) & (LEVEL_COUNT - 1)) << (COORDINATE_BITS + COORDINATE_BITS)) |
                        (FLAG_WRITE_LEVEL | (ALL_DIRECTIONS_BITSET << (COORDINATE_BITS + COORDINATE_BITS + LEVEL_BITS)));
                }

                // Queue next decrease
                if (queueLength >= static_cast<i32>(m_decreaseQueue.size())) {
                    m_decreaseQueue.resize(m_decreaseQueue.size() * 2);
                }
                m_decreaseQueue[queueLength++] =
                    ((static_cast<u64>(offX + (offZ << COORDINATE_BITS) + encodeOffset) &
                      ((1ULL << (COORDINATE_BITS + COORDINATE_BITS)) - 1))) |
                    ((static_cast<u64>(toPropagate) & (LEVEL_COUNT - 1)) << (COORDINATE_BITS + COORDINATE_BITS)) |
                    (ALL_DIRECTIONS_BITSET << (COORDINATE_BITS + COORDINATE_BITS + LEVEL_BITS));
            }
        }

        // Propagate sources we clobbered
        m_increaseQueueLen = increaseQueueLength;
        performIncrease();
    }

private:
    void setSectionInCache(i32 sectionX, i32 sectionZ, ThreadedTicketLevelPropagator::Section* section) {
        m_sections[sectionX + SECTION_CACHE_WIDTH * sectionZ + m_sectionIndexOffset] = section;
    }
};

// ============================================================================
// ThreadedTicketLevelPropagator Implementation
// ============================================================================

ThreadedTicketLevelPropagator::ThreadedTicketLevelPropagator() = default;

void ThreadedTicketLevelPropagator::setSource(i32 x, i32 z, i32 level) {
    if (level < MIN_SOURCE_LEVEL || level > MAX_SOURCE_LEVEL) {
        return;  // Invalid source level
    }

    i32 sectionX = toSectionCoord(x);
    i32 sectionZ = toSectionCoord(z);
    u64 coordinate = sectionToKey(sectionX, sectionZ);

    std::lock_guard<std::mutex> lock(m_sectionsMutex);

    Section* section = getOrCreateSection(sectionX, sectionZ);
    u16 localIndex = Section::getLocalIndex(x, z);

    u16 currentEncoded = section->levels[localIndex];
    i32 currentSource = (currentEncoded >> 8) & 0xFF;

    if (currentSource == level) {
        // Nothing to do, kill any pending update
        section->queuedSources[localIndex] = static_cast<u8>(level);
        return;
    }

    // Queue the source update
    bool wasEmpty = section->queuedSources.empty();
    section->queuedSources[localIndex] = static_cast<u8>(level);

    if (wasEmpty) {
        // Queue section for update
        std::lock_guard<std::mutex> updateLock(m_updateMutex);
        if (m_pendingSections.find(coordinate) == m_pendingSections.end()) {
            m_updateQueue.push_back(coordinate);
            m_pendingSections.insert(coordinate);
        }
    }
}

void ThreadedTicketLevelPropagator::removeSource(i32 x, i32 z) {
    i32 sectionX = toSectionCoord(x);
    i32 sectionZ = toSectionCoord(z);
    u64 coordinate = sectionToKey(sectionX, sectionZ);

    std::lock_guard<std::mutex> lock(m_sectionsMutex);

    auto it = m_sections.find(coordinate);
    if (it == m_sections.end()) {
        return;
    }

    Section* section = it->second.get();
    u16 localIndex = Section::getLocalIndex(x, z);

    i32 currentSource = (section->levels[localIndex] >> 8) & 0xFF;

    if (currentSource == 0) {
        section->queuedSources[localIndex] = 0;
        return;
    }

    // Queue source removal (level 0)
    bool wasEmpty = section->queuedSources.empty();
    section->queuedSources[localIndex] = 0;

    if (wasEmpty) {
        std::lock_guard<std::mutex> updateLock(m_updateMutex);
        if (m_pendingSections.find(coordinate) == m_pendingSections.end()) {
            m_updateQueue.push_back(coordinate);
            m_pendingSections.insert(coordinate);
        }
    }
}

bool ThreadedTicketLevelPropagator::hasPendingUpdates() const {
    std::lock_guard<std::mutex> lock(m_updateMutex);
    return !m_updateQueue.empty();
}

bool ThreadedTicketLevelPropagator::performUpdate(i32 sectionX, i32 sectionZ,
                                                   concurrent::ReentrantAreaLock& schedulingLock,
                                                   std::vector<std::pair<u64, u8>>& outUpdatedPositions) {
    (void)schedulingLock;  // Not used in this simplified implementation

    if (!hasPendingUpdates()) {
        return false;
    }

    u64 coordinate = sectionToKey(sectionX, sectionZ);

    std::lock_guard<std::mutex> sectionsLock(m_sectionsMutex);

    auto it = m_sections.find(coordinate);
    if (it == m_sections.end()) {
        return false;
    }

    Section* section = it->second.get();
    if (section->queuedSources.empty()) {
        return false;
    }

    // Create propagator
    Propagator propagator;
    propagator.setupEncodeOffset(sectionX, sectionZ);

    // Process queued sources
    i32 oldSourceSize = static_cast<i32>(section->sources.size());

    for (auto& [localIndex, newSource] : section->queuedSources) {
        i32 posX = (localIndex & (SECTION_SIZE - 1)) | (sectionX << SECTION_SHIFT);
        i32 posZ = ((localIndex >> SECTION_SHIFT) & (SECTION_SIZE - 1)) | (sectionZ << SECTION_SHIFT);

        u16 currentEncoded = section->levels[localIndex];
        i32 currLevel = currentEncoded & 0xFF;
        i32 prevSource = (currentEncoded >> 8) & 0xFF;

        if (prevSource == newSource) {
            continue;  // No change
        }

        if ((prevSource < currLevel && newSource <= currLevel) || newSource == currLevel) {
            // Just update source, no propagation needed
            section->levels[localIndex] = static_cast<u16>(currLevel | (newSource << 8));
        } else {
            // Set level and source to new value
            section->levels[localIndex] = static_cast<u16>(newSource | (newSource << 8));
            propagator.m_updatedPositions[posToKey(posX, posZ)] = static_cast<u8>(newSource);

            if (newSource != 0) {
                // Queue increase
                propagator.appendToIncreaseQueue(
                    ((static_cast<u64>(posX + (posZ << COORDINATE_BITS) +
                                       propagator.m_coordinateOffset)) &
                     ((1ULL << (COORDINATE_BITS + COORDINATE_BITS)) - 1)) |
                    ((static_cast<u64>(newSource) & (LEVEL_COUNT - 1)) <<
                     (COORDINATE_BITS + COORDINATE_BITS)) |
                    (ALL_DIRECTIONS_BITSET <<
                     (COORDINATE_BITS + COORDINATE_BITS + LEVEL_BITS))
                );
            }

            if (newSource < currLevel) {
                // Queue decrease
                propagator.appendToDecreaseQueue(
                    ((static_cast<u64>(posX + (posZ << COORDINATE_BITS) +
                                       propagator.m_coordinateOffset)) &
                     ((1ULL << (COORDINATE_BITS + COORDINATE_BITS)) - 1)) |
                    ((static_cast<u64>(currLevel) & (LEVEL_COUNT - 1)) <<
                     (COORDINATE_BITS + COORDINATE_BITS)) |
                    (ALL_DIRECTIONS_BITSET <<
                     (COORDINATE_BITS + COORDINATE_BITS + LEVEL_BITS))
                );
            }
        }

        if (newSource == 0) {
            section->sources.erase(localIndex);
        } else if (prevSource == 0) {
            section->sources.insert(localIndex);
        }
    }

    section->queuedSources.clear();
    i32 newSourceSize = static_cast<i32>(section->sources.size());

    // Update neighbor section reference counts
    if (oldSourceSize == 0 && newSourceSize != 0) {
        // Initialize neighbor sections
        for (i32 dz = -1; dz <= 1; ++dz) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                if ((dx | dz) == 0) continue;

                i32 offX = dx + sectionX;
                i32 offZ = dz + sectionZ;
                Section* neighbor = getOrCreateSection(offX, offZ);
                ++neighbor->oneRadNeighboursWithSources;
            }
        }
    }

    // Run propagation if needed
    if (propagator.hasUpdates()) {
        propagator.setupCaches(this, sectionX, sectionZ, 1);
        propagator.performDecrease();
        propagator.destroyCaches();
    }

    // Handle section de-initialization
    if (newSourceSize == 0) {
        bool decrementRef = oldSourceSize != 0;

        for (i32 dz = -1; dz <= 1; ++dz) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                i32 offX = dx + sectionX;
                i32 offZ = dz + sectionZ;
                u64 neighborCoord = sectionToKey(offX, offZ);

                auto neighborIt = m_sections.find(neighborCoord);
                if (neighborIt == m_sections.end()) {
                    continue;
                }

                Section* neighbor = neighborIt->second.get();

                if (decrementRef && (dx | dz) != 0) {
                    --neighbor->oneRadNeighboursWithSources;
                }

                if (neighbor->oneRadNeighboursWithSources == 0 &&
                    neighbor->queuedSources.empty() &&
                    neighbor->sources.empty()) {
                    m_sections.erase(neighborCoord);
                }
            }
        }
    }

    // Copy updated positions
    bool hasUpdates = !propagator.m_updatedPositions.empty();
    if (hasUpdates) {
        outUpdatedPositions.clear();
        outUpdatedPositions.reserve(propagator.m_updatedPositions.size());
        for (const auto& [key, level] : propagator.m_updatedPositions) {
            outUpdatedPositions.emplace_back(key, level);
        }

        // Invoke callback for each position
        // TODO: Properly track old level by reading before propagation
        if (m_levelChangeCallback) {
            for (const auto& [key, newLevel] : outUpdatedPositions) {
                i32 posX, posZ;
                keyToPos(key, posX, posZ);
                m_levelChangeCallback(posX, posZ, 0, static_cast<i32>(newLevel));
            }
        }
    }

    // Remove from pending
    {
        std::lock_guard<std::mutex> updateLock(m_updateMutex);
        m_pendingSections.erase(coordinate);
        auto queueIt = std::find(m_updateQueue.begin(), m_updateQueue.end(), coordinate);
        if (queueIt != m_updateQueue.end()) {
            m_updateQueue.erase(queueIt);
        }
    }

    return hasUpdates;
}

i32 ThreadedTicketLevelPropagator::getLevel(i32 x, i32 z) const {
    i32 sectionX = toSectionCoord(x);
    i32 sectionZ = toSectionCoord(z);
    u64 coordinate = sectionToKey(sectionX, sectionZ);

    std::lock_guard<std::mutex> lock(m_sectionsMutex);

    auto it = m_sections.find(coordinate);
    if (it == m_sections.end()) {
        return MAX_LEVEL;
    }

    Section* section = it->second.get();
    u16 localIndex = Section::getLocalIndex(x, z);
    return section->getLevel(localIndex);
}

ThreadedTicketLevelPropagator::Section* ThreadedTicketLevelPropagator::getSection(i32 sectionX, i32 sectionZ) {
    u64 coordinate = sectionToKey(sectionX, sectionZ);
    auto it = m_sections.find(coordinate);
    return it != m_sections.end() ? it->second.get() : nullptr;
}

ThreadedTicketLevelPropagator::Section* ThreadedTicketLevelPropagator::getOrCreateSection(i32 sectionX, i32 sectionZ) {
    u64 coordinate = sectionToKey(sectionX, sectionZ);
    auto it = m_sections.find(coordinate);
    if (it != m_sections.end()) {
        return it->second.get();
    }

    auto section = std::make_unique<Section>(sectionX, sectionZ);
    Section* ptr = section.get();
    m_sections[coordinate] = std::move(section);
    return ptr;
}

i32 ThreadedTicketLevelPropagator::getMaxSchedulingRadius() {
    // This should match ChunkTaskScheduler::getMaxAccessRadius() * 2
    // For now, return a reasonable default
    return 4;
}

void ThreadedTicketLevelPropagator::onLevelChanged(i32 x, i32 z, i32 oldLevel, i32 newLevel) {
    if (m_levelChangeCallback) {
        m_levelChangeCallback(x, z, oldLevel, newLevel);
    }
}

} // namespace mc::world
