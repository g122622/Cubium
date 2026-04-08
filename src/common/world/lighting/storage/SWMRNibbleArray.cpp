#include "SWMRNibbleArray.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace mc {

SWMRNibbleArray::SWMRNibbleArray()
    : m_stateUpdating(INIT_STATE_UNINIT)
    , m_stateVisible(INIT_STATE_UNINIT)
    , m_updatingDirty(false)
{
}

SWMRNibbleArray::SWMRNibbleArray(std::vector<u8> bytes)
    : SWMRNibbleArray(std::move(bytes), INIT_STATE_UNINIT)
{
    if (!m_storageUpdating.empty()) {
        m_stateUpdating = INIT_STATE_INIT;
        m_stateVisible = INIT_STATE_INIT;
        m_storageVisible = m_storageUpdating;
    }
}

SWMRNibbleArray::SWMRNibbleArray(std::vector<u8> bytes, i32 state)
    : m_stateUpdating(state)
    , m_stateVisible(state)
    , m_storageUpdating(std::move(bytes))
    , m_storageVisible(m_storageUpdating)
    , m_updatingDirty(false)
{
    if (!m_storageUpdating.empty() && m_storageUpdating.size() != ARRAY_SIZE) {
        throw std::invalid_argument("NibbleArray data must be 2048 bytes or empty");
    }
    if (m_storageUpdating.empty() && (state == INIT_STATE_INIT || state == INIT_STATE_HIDDEN)) {
        throw std::invalid_argument("NibbleArray data cannot be empty for initialised states");
    }
}

SWMRNibbleArray SWMRNibbleArray::fromVanilla(const NibbleArray* nibble)
{
    if (nibble == nullptr) {
        return nullNibble();
    }
    if (nibble->isEmpty()) {
        return SWMRNibbleArray();
    }
    return SWMRNibbleArray(nibble->data());
}

SWMRNibbleArray SWMRNibbleArray::nullNibble()
{
    return SWMRNibbleArray(std::vector<u8>{}, INIT_STATE_NULL);
}

String SWMRNibbleArray::toString() const
{
    std::ostringstream builder;
    builder << "State: ";
    switch (m_stateVisible) {
        case INIT_STATE_NULL:
            builder << "null";
            break;
        case INIT_STATE_UNINIT:
            builder << "uninitialised";
            break;
        case INIT_STATE_INIT:
            builder << "initialised";
            break;
        case INIT_STATE_HIDDEN:
            builder << "hidden";
            break;
        default:
            builder << "unknown";
            break;
    }

    builder << "\nData:\n";
    const auto& data = m_storageVisible;
    if (data.empty()) {
        builder << "null";
        return builder.str();
    }

    for (int index = 0; index < 4096; ++index) {
        const int level = ((data[static_cast<size_t>(index) >> 1] >> ((index & 1) << 2)) & 0xF);
        builder << std::hex << level;
        if ((index & 15) == 15) {
            builder << '\n';
        }
        if ((index & 255) == 255) {
            builder << '\n';
        }
    }

    return builder.str();
}

Optional<SWMRNibbleArray::SaveState> SWMRNibbleArray::getSaveState() const
{
    const i32 state = m_stateVisible;
    const auto& data = m_storageVisible;
    if (state == INIT_STATE_NULL) {
        return std::nullopt;
    }
    if (state == INIT_STATE_UNINIT) {
        return SaveState{std::vector<u8>{}, state};
    }
    if (isAllZero(data)) {
        if (state == INIT_STATE_INIT) {
            return SaveState{std::vector<u8>{}, INIT_STATE_UNINIT};
        }
        return std::nullopt;
    }
    return SaveState{data, state};
}

void SWMRNibbleArray::extrudeLower(const SWMRNibbleArray& other)
{
    if (other.m_stateUpdating == INIT_STATE_NULL) {
        throw std::invalid_argument("Cannot extrude from a null nibble");
    }

    if (other.m_storageUpdating.empty()) {
        setUninitialised();
        return;
    }

    if (!m_updatingDirty) {
        if (m_storageUpdating.empty()) {
            m_storageUpdating.assign(ARRAY_SIZE, 0);
            if (m_stateUpdating != INIT_STATE_HIDDEN) {
                m_stateUpdating = INIT_STATE_INIT;
            }
        } else {
            m_storageUpdating = m_storageVisible;
        }
        m_updatingDirty = true;
    }

    const int sliceSize = 128;
    const int end = sliceSize - 1;
    for (int y = 0; y <= 15; ++y) {
        std::copy_n(other.m_storageUpdating.begin(), sliceSize, m_storageUpdating.begin() + (y << 7));
        (void)end;
    }
}

void SWMRNibbleArray::setFull()
{
    if (m_stateUpdating != INIT_STATE_HIDDEN) {
        m_stateUpdating = INIT_STATE_INIT;
    }
    if (m_storageUpdating.empty() || !m_updatingDirty) {
        m_storageUpdating.assign(ARRAY_SIZE, 0xFF);
    } else {
        std::fill(m_storageUpdating.begin(), m_storageUpdating.end(), 0xFF);
    }
    m_updatingDirty = true;
}

void SWMRNibbleArray::setZero()
{
    if (m_stateUpdating != INIT_STATE_HIDDEN) {
        m_stateUpdating = INIT_STATE_INIT;
    }
    if (m_storageUpdating.empty() || !m_updatingDirty) {
        m_storageUpdating.assign(ARRAY_SIZE, 0x00);
    } else {
        std::fill(m_storageUpdating.begin(), m_storageUpdating.end(), 0x00);
    }
    m_updatingDirty = true;
}

void SWMRNibbleArray::setNonNull()
{
    if (m_stateUpdating == INIT_STATE_HIDDEN) {
        m_stateUpdating = INIT_STATE_INIT;
        return;
    }
    if (m_stateUpdating != INIT_STATE_NULL) {
        return;
    }
    m_stateUpdating = INIT_STATE_UNINIT;
}

void SWMRNibbleArray::setNull()
{
    m_stateUpdating = INIT_STATE_NULL;
    m_storageUpdating.clear();
    m_updatingDirty = false;
}

void SWMRNibbleArray::setUninitialised()
{
    m_stateUpdating = INIT_STATE_UNINIT;
    m_storageUpdating.clear();
    m_updatingDirty = false;
}

void SWMRNibbleArray::setHidden()
{
    if (m_stateUpdating == INIT_STATE_HIDDEN) {
        return;
    }
    if (m_stateUpdating != INIT_STATE_INIT) {
        setNull();
    } else {
        m_stateUpdating = INIT_STATE_HIDDEN;
    }
}

bool SWMRNibbleArray::isDirty() const
{
    return m_stateUpdating != m_stateVisible || m_updatingDirty;
}

bool SWMRNibbleArray::isNullNibbleUpdating() const
{
    return m_stateUpdating == INIT_STATE_NULL;
}

bool SWMRNibbleArray::isNullNibbleVisible() const
{
    return m_stateVisible == INIT_STATE_NULL;
}

bool SWMRNibbleArray::isUninitialisedUpdating() const
{
    return m_stateUpdating == INIT_STATE_UNINIT;
}

bool SWMRNibbleArray::isUninitialisedVisible() const
{
    return m_stateVisible == INIT_STATE_UNINIT;
}

bool SWMRNibbleArray::isInitialisedUpdating() const
{
    return m_stateUpdating == INIT_STATE_INIT;
}

bool SWMRNibbleArray::isInitialisedVisible() const
{
    return m_stateVisible == INIT_STATE_INIT;
}

bool SWMRNibbleArray::isHiddenUpdating() const
{
    return m_stateUpdating == INIT_STATE_HIDDEN;
}

bool SWMRNibbleArray::isHiddenVisible() const
{
    return m_stateVisible == INIT_STATE_HIDDEN;
}

bool SWMRNibbleArray::updateVisible()
{
    if (!isDirty()) {
        return false;
    }

    if (m_stateUpdating == INIT_STATE_NULL || m_stateUpdating == INIT_STATE_UNINIT) {
        m_storageVisible.clear();
    } else {
        m_storageVisible = m_storageUpdating;
    }

    m_updatingDirty = false;
    m_stateVisible = m_stateUpdating;

    if (m_stateUpdating == INIT_STATE_NULL || m_stateUpdating == INIT_STATE_UNINIT) {
        m_storageUpdating.clear();
    } else {
        m_storageUpdating = m_storageVisible;
    }

    return true;
}

Optional<NibbleArray> SWMRNibbleArray::toVanillaNibble() const
{
    switch (m_stateVisible) {
        case INIT_STATE_HIDDEN:
        case INIT_STATE_NULL:
            return std::nullopt;
        case INIT_STATE_UNINIT:
            return NibbleArray();
        case INIT_STATE_INIT:
            return NibbleArray(m_storageVisible);
        default:
            return std::nullopt;
    }
}

std::vector<u8> SWMRNibbleArray::toByteArray() const
{
    switch (m_stateVisible) {
        case INIT_STATE_HIDDEN:
        case INIT_STATE_NULL:
            return {};
        case INIT_STATE_UNINIT:
            return std::vector<u8>(ARRAY_SIZE, 0);
        case INIT_STATE_INIT:
            return m_storageVisible;
        default:
            return {};
    }
}

int SWMRNibbleArray::getUpdating(i32 x, i32 y, i32 z) const
{
    return getUpdating(getIndex(x, y, z));
}

int SWMRNibbleArray::getUpdating(i32 index) const
{
    if (m_storageUpdating.empty()) {
        return 0;
    }
    index &= 0xFFF;
    const u8 value = m_storageUpdating[static_cast<size_t>(getByteIndex(index))];
    return (value >> ((index & 1) << 2)) & 0xF;
}

int SWMRNibbleArray::getVisible(i32 x, i32 y, i32 z) const
{
    return getVisible(getIndex(x, y, z));
}

int SWMRNibbleArray::getVisible(i32 index) const
{
    if (m_storageVisible.empty()) {
        return 0;
    }
    index &= 0xFFF;
    const u8 value = m_storageVisible[static_cast<size_t>(getByteIndex(index))];
    return (value >> ((index & 1) << 2)) & 0xF;
}

void SWMRNibbleArray::set(i32 x, i32 y, i32 z, i32 value)
{
    set(getIndex(x, y, z), value);
}

void SWMRNibbleArray::set(i32 index, i32 value)
{
    if (!m_updatingDirty) {
        swapUpdatingAndMarkDirty();
    }

    index &= 0xFFF;
    value &= 0xF;

    const int byteIndex = getByteIndex(index);
    const int shift = (index & 1) << 2;
    const u8 mask = static_cast<u8>(0xF0u >> shift);
    m_storageUpdating[static_cast<size_t>(byteIndex)] = static_cast<u8>((m_storageUpdating[static_cast<size_t>(byteIndex)] & mask) | (value << shift));
}

void SWMRNibbleArray::swapUpdatingAndMarkDirty()
{
    if (m_updatingDirty) {
        return;
    }

    if (m_storageUpdating.empty()) {
        m_storageUpdating.assign(ARRAY_SIZE, 0);
    } else {
        m_storageUpdating = m_storageVisible;
        if (m_storageUpdating.empty()) {
            m_storageUpdating.assign(ARRAY_SIZE, 0);
        }
    }

    if (m_stateUpdating != INIT_STATE_HIDDEN) {
        m_stateUpdating = INIT_STATE_INIT;
    }
    m_updatingDirty = true;
}

void SWMRNibbleArray::ensureAllocated()
{
    if (m_storageUpdating.empty()) {
        m_storageUpdating.assign(ARRAY_SIZE, 0);
    }
}

bool SWMRNibbleArray::isAllZero(const std::vector<u8>& data)
{
    return std::all_of(data.begin(), data.end(), [](u8 value) {
        return value == 0;
    });
}

} // namespace mc