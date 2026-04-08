#pragma once

#include "../../../util/NibbleArray.hpp"

#include <optional>
#include <vector>

namespace mc {

/**
 * @brief 单写多读 nibble 数组
 *
 * 这是对 starlight 单写多读存储的 C++ 翻译版，用于光照数组的更新与可见视图分离。
 */
class SWMRNibbleArray {
public:
    static constexpr i32 INIT_STATE_NULL = 0;
    static constexpr i32 INIT_STATE_UNINIT = 1;
    static constexpr i32 INIT_STATE_INIT = 2;
    static constexpr i32 INIT_STATE_HIDDEN = 3;

    static constexpr Size ARRAY_SIZE = NibbleArray::BYTE_SIZE;

    struct SaveState {
        std::vector<u8> data;
        i32 state;
    };

public:
    SWMRNibbleArray();
    explicit SWMRNibbleArray(std::vector<u8> bytes);
    SWMRNibbleArray(std::vector<u8> bytes, i32 state);

    [[nodiscard]] static SWMRNibbleArray fromVanilla(const NibbleArray* nibble);
    [[nodiscard]] static SWMRNibbleArray nullNibble();

    [[nodiscard]] String toString() const;
    [[nodiscard]] Optional<SaveState> getSaveState() const;

    void extrudeLower(const SWMRNibbleArray& other);
    void setFull();
    void setZero();
    void setNonNull();
    void setNull();
    void setUninitialised();
    void setHidden();
    [[nodiscard]] bool isDirty() const;
    [[nodiscard]] bool isNullNibbleUpdating() const;
    [[nodiscard]] bool isNullNibbleVisible() const;
    [[nodiscard]] bool isUninitialisedUpdating() const;
    [[nodiscard]] bool isUninitialisedVisible() const;
    [[nodiscard]] bool isInitialisedUpdating() const;
    [[nodiscard]] bool isInitialisedVisible() const;
    [[nodiscard]] bool isHiddenUpdating() const;
    [[nodiscard]] bool isHiddenVisible() const;
    [[nodiscard]] bool updateVisible();
    [[nodiscard]] Optional<NibbleArray> toVanillaNibble() const;
    [[nodiscard]] std::vector<u8> toByteArray() const;

    [[nodiscard]] int getUpdating(i32 x, i32 y, i32 z) const;
    [[nodiscard]] int getUpdating(i32 index) const;
    [[nodiscard]] int getVisible(i32 x, i32 y, i32 z) const;
    [[nodiscard]] int getVisible(i32 index) const;
    void set(i32 x, i32 y, i32 z, i32 value);
    void set(i32 index, i32 value);

    [[nodiscard]] static constexpr i32 getIndex(i32 x, i32 y, i32 z) {
        return (y & 0xF) << 8 | (z & 0xF) << 4 | (x & 0xF);
    }

    [[nodiscard]] static constexpr i32 getByteIndex(i32 index) {
        return index >> 1;
    }

    [[nodiscard]] static constexpr bool isLowerNibble(i32 index) {
        return (index & 1) == 0;
    }

private:
    void swapUpdatingAndMarkDirty();
    void ensureAllocated();
    [[nodiscard]] static bool isAllZero(const std::vector<u8>& data);

private:
    i32 m_stateUpdating;
    i32 m_stateVisible;
    std::vector<u8> m_storageUpdating;
    std::vector<u8> m_storageVisible;
    bool m_updatingDirty;
};

} // namespace mc