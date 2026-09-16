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

#include "SWMRNibbleArray.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

namespace mc {

// 线程本地对象池
thread_local std::vector<std::unique_ptr<std::array<u8, SWMRNibbleArray::ARRAY_SIZE>>> SWMRNibbleArray::s_bytePool;

// ============================================================================
// 构造函数
// ============================================================================

SWMRNibbleArray::SWMRNibbleArray()
    : m_stateUpdating(State::Uninit)
    , m_stateVisible(State::Uninit)
    , m_storageUpdating(nullptr)
    , m_storageVisible(nullptr)
    , m_updatingDirty(false)
{}

SWMRNibbleArray::~SWMRNibbleArray()
{
    // 释放可见侧存储（只有当它与更新侧不同时）
    auto* visible = m_storageVisible.load();
    if (visible != nullptr && visible != m_storageUpdating.get()) {
        _freeBytes(std::unique_ptr<std::array<u8, ARRAY_SIZE>>(visible));
    }
    // 更新侧存储由 unique_ptr 自动释放
}

SWMRNibbleArray::SWMRNibbleArray(std::unique_ptr<std::array<u8, ARRAY_SIZE>> data, bool isNull)
    : m_stateUpdating(data ? State::Init : (isNull ? State::Null : State::Uninit))
    , m_stateVisible(m_stateUpdating)
    , m_storageUpdating(std::move(data))
    , m_storageVisible(m_storageUpdating ? m_storageUpdating.get() : nullptr) // 与 Moonrise 一致：初始时指向同一数组
    , m_updatingDirty(false)
{}

SWMRNibbleArray::SWMRNibbleArray(std::unique_ptr<std::array<u8, ARRAY_SIZE>> data, State state)
    : m_stateUpdating(state)
    , m_stateVisible(state)
    , m_storageUpdating(std::move(data))
    , m_storageVisible(m_storageUpdating ? m_storageUpdating.get() : nullptr) // 与 Moonrise 一致：初始时指向同一数组
    , m_updatingDirty(false)
{
    // 与 Moonrise 一致：data 为空时不允许 Init/Hidden 状态
    MC_ASSERT_RELEASE(!(m_storageUpdating == nullptr && (state == State::Init || state == State::Hidden)));
}

SWMRNibbleArray SWMRNibbleArray::fromData(const std::vector<u8>& data)
{
    if (data.empty()) {
        return SWMRNibbleArray();
    }

    auto storage = std::make_unique<std::array<u8, ARRAY_SIZE>>();
    if (data.size() >= ARRAY_SIZE) {
        std::memcpy(storage->data(), data.data(), ARRAY_SIZE);
    } else {
        std::memcpy(storage->data(), data.data(), data.size());
        std::fill(storage->begin() + data.size(), storage->end(), 0);
    }

    return SWMRNibbleArray(std::move(storage), State::Init);
}

SWMRNibbleArray SWMRNibbleArray::createUninitialized()
{
    return SWMRNibbleArray(nullptr, false);
}

SWMRNibbleArray SWMRNibbleArray::createNull()
{
    return SWMRNibbleArray(nullptr, true);
}

// ============================================================================
// 状态查询 - 更新侧
// ============================================================================

bool SWMRNibbleArray::isDirty() const
{
    return m_stateUpdating != m_stateVisible.load() || m_updatingDirty;
}

bool SWMRNibbleArray::isNullUpdating() const
{
    return m_stateUpdating == State::Null;
}

bool SWMRNibbleArray::isUninitializedUpdating() const
{
    return m_stateUpdating == State::Uninit;
}

bool SWMRNibbleArray::isInitializedUpdating() const
{
    return m_stateUpdating == State::Init;
}

bool SWMRNibbleArray::isHiddenUpdating() const
{
    return m_stateUpdating == State::Hidden;
}

// ============================================================================
// 状态查询 - 可见侧
// ============================================================================

bool SWMRNibbleArray::isNullVisible() const
{
    return m_stateVisible.load() == State::Null;
}

bool SWMRNibbleArray::isUninitializedVisible() const
{
    return m_stateVisible.load() == State::Uninit;
}

bool SWMRNibbleArray::isInitializedVisible() const
{
    return m_stateVisible.load() == State::Init;
}

bool SWMRNibbleArray::isHiddenVisible() const
{
    return m_stateVisible.load() == State::Hidden;
}

// ============================================================================
// 状态设置 - 更新侧
// ============================================================================

void SWMRNibbleArray::setFull()
{
    if (m_stateUpdating != State::Hidden) {
        m_stateUpdating = State::Init;
    }

    // 与 Moonrise 一致：当 storageUpdating 为空或不是 dirty 时，分配新数组
    if (m_storageUpdating == nullptr || !m_updatingDirty) {
        // 检查更新侧和可见侧是否共享同一块内存
        auto* currentVisible = m_storageVisible.load(std::memory_order::acquire);
        bool sharedWithVisible = (m_storageUpdating.get() == currentVisible);

        if (sharedWithVisible && m_storageUpdating != nullptr) {
            // 如果与可见侧共享内存，需要先创建新存储
            auto newStorage = _allocateBytes();
            m_storageUpdating = std::move(newStorage);
            // 更新可见侧指向新存储
            m_storageVisible.store(m_storageUpdating.get(), std::memory_order::release);
        } else {
            m_storageUpdating = _allocateBytes();
        }
    }

    std::fill(m_storageUpdating->begin(), m_storageUpdating->end(), static_cast<u8>(0xFF)); // 每字节两个15
    m_updatingDirty = true;
}

void SWMRNibbleArray::setZero()
{
    if (m_stateUpdating != State::Hidden) {
        m_stateUpdating = State::Init;
    }

    // 与 Moonrise 一致：当 storageUpdating 为空或不是 dirty 时，分配新数组
    if (m_storageUpdating == nullptr || !m_updatingDirty) {
        // 检查更新侧和可见侧是否共享同一块内存
        auto* currentVisible = m_storageVisible.load(std::memory_order::acquire);
        bool sharedWithVisible = (m_storageUpdating.get() == currentVisible);

        if (sharedWithVisible && m_storageUpdating != nullptr) {
            // 如果与可见侧共享内存，需要先创建新存储
            auto newStorage = _allocateBytes();
            m_storageUpdating = std::move(newStorage);
            // 更新可见侧指向新存储
            m_storageVisible.store(m_storageUpdating.get(), std::memory_order::release);
        } else {
            m_storageUpdating = _allocateBytes();
        }
    }

    std::fill(m_storageUpdating->begin(), m_storageUpdating->end(), static_cast<u8>(0));
    m_updatingDirty = true;
}

void SWMRNibbleArray::setNonNull()
{
    if (m_stateUpdating == State::Hidden) {
        m_stateUpdating = State::Init;
        return;
    }
    if (m_stateUpdating != State::Null) {
        return;
    }
    m_stateUpdating = State::Uninit;
}

void SWMRNibbleArray::setNull()
{
    m_stateUpdating = State::Null;
    if (m_updatingDirty && m_storageUpdating != nullptr) {
        _freeBytes(std::move(m_storageUpdating));
    }
    m_storageUpdating = nullptr;
    m_updatingDirty = false;
}

void SWMRNibbleArray::setUninitialized()
{
    m_stateUpdating = State::Uninit;
    if (m_storageUpdating != nullptr && m_updatingDirty) {
        _freeBytes(std::move(m_storageUpdating));
    }
    m_storageUpdating = nullptr;
    m_updatingDirty = false;
}

void SWMRNibbleArray::setHidden()
{
    if (m_stateUpdating == State::Hidden) {
        return;
    }
    if (m_stateUpdating != State::Init) {
        setNull();
    } else {
        m_stateUpdating = State::Hidden;
    }
}

// ============================================================================
// 元素访问 - 更新侧
// ============================================================================

u8 SWMRNibbleArray::getUpdating(i32 x, i32 y, i32 z) const
{
    return getUpdating(getIndex(x, y, z));
}

u8 SWMRNibbleArray::getUpdating(i32 index) const
{
    if (m_storageUpdating == nullptr) {
        return 0;
    }

    const u8 value = (*m_storageUpdating)[static_cast<size_t>(index >> 1)];
    // 偶数索引取低4位，奇数索引取高4位
    return static_cast<u8>((value >> ((index & 1) << 2)) & 0x0F);
}

void SWMRNibbleArray::set(i32 x, i32 y, i32 z, u8 value)
{
    set(getIndex(x, y, z), value);
}

void SWMRNibbleArray::set(i32 index, u8 value)
{
    _ensureWritable();

    const i32 shift = (index & 1) << 2;
    const size_t i = static_cast<size_t>(index >> 1);

    // 清除旧值并设置新值
    (*m_storageUpdating)[i] = static_cast<u8>(((*m_storageUpdating)[i] & (0xF0 >> shift)) | ((value & 0x0F) << shift));
}

// ============================================================================
// 元素访问 - 可见侧（线程安全）
// ============================================================================

u8 SWMRNibbleArray::getVisible(i32 x, i32 y, i32 z) const
{
    return getVisible(getIndex(x, y, z));
}

u8 SWMRNibbleArray::getVisible(i32 index) const
{
    auto* storage = m_storageVisible.load(std::memory_order::acquire);
    if (storage == nullptr || m_stateVisible.load() == State::Null) {
        return 0;
    }

    const u8 value = (*storage)[static_cast<size_t>(index >> 1)];
    return static_cast<u8>((value >> ((index & 1) << 2)) & 0x0F);
}

// ============================================================================
// 同步操作
// ============================================================================

bool SWMRNibbleArray::updateVisible()
{
    if (!isDirty()) {
        return false;
    }

    // 同步更新 - SWMR 模式：与 Moonrise 一致
    if (m_stateUpdating == State::Null || m_stateUpdating == State::Uninit) {
        // 对于 Null/Uninit 状态，可见侧也应该没有存储
        auto* oldVisible = m_storageVisible.load(std::memory_order::acquire);
        m_storageVisible.store(nullptr, std::memory_order::release);
        if (oldVisible != nullptr && oldVisible != m_storageUpdating.get()) {
            _freeBytes(std::unique_ptr<std::array<u8, ARRAY_SIZE>>(oldVisible));
        }
        if (m_storageUpdating != nullptr) {
            _freeBytes(std::move(m_storageUpdating));
            m_storageUpdating = nullptr;
        }
    } else {
        // Init 或 Hidden 状态：需要同步数据到可见侧
        auto* currentVisible = m_storageVisible.load(std::memory_order::acquire);

        if (currentVisible == nullptr) {
            // 可见侧为空，复制更新侧数据
            auto newStorage = std::make_unique<std::array<u8, ARRAY_SIZE>>();
            *newStorage = *m_storageUpdating;
            m_storageVisible.store(newStorage.get(), std::memory_order::release);
            m_storageUpdating = std::move(newStorage);
        } else {
            // 可见侧已有存储，如果更新侧和可见侧不同则复制数据
            if (m_storageUpdating.get() != currentVisible) {
                std::memcpy(currentVisible->data(), m_storageUpdating->data(), ARRAY_SIZE);
                // 释放旧的更新侧存储
                _freeBytes(std::move(m_storageUpdating));
                m_storageUpdating = std::unique_ptr<std::array<u8, ARRAY_SIZE>>(currentVisible);
            }
        }
    }

    m_updatingDirty = false;
    m_stateVisible.store(m_stateUpdating, std::memory_order::release);

    return true;
}

// ============================================================================
// 扩展操作
// ============================================================================

void SWMRNibbleArray::extrudeLower(const SWMRNibbleArray& other)
{
    if (other.m_stateUpdating == State::Null) {
        return; // 不能从 null 挤压
    }

    if (other.m_storageUpdating == nullptr) {
        setUninitialized();
        return;
    }

    if (!m_updatingDirty) {
        // 检查更新侧和可见侧是否共享同一块内存
        auto* currentVisible = m_storageVisible.load(std::memory_order::acquire);
        bool sharedWithVisible = (m_storageUpdating.get() == currentVisible);

        if (m_storageUpdating != nullptr) {
            if (sharedWithVisible) {
                // 如果与可见侧共享内存，需要先创建新存储
                auto newStorage = _allocateBytes();
                m_storageUpdating = std::move(newStorage);
                m_storageVisible.store(m_storageUpdating.get(), std::memory_order::release);
            } else {
                m_storageUpdating = _allocateBytes();
            }
        } else {
            m_storageUpdating = _allocateBytes();
            m_stateUpdating = State::Init;
        }
        m_updatingDirty = true;
    }

    // 从源数组复制第一层到所有层
    // 索引公式: x | (z << 4) | (y << 8)
    const size_t start = 0;
    const size_t end = (15 | (15 << 4)) >> 1; // 顶层 XZ 面的字节索引

    for (i32 y = 0; y <= 15; ++y) {
        const size_t destOffset = static_cast<size_t>(y) << (8 - 1); // y * 128 字节
        std::copy(other.m_storageUpdating->begin() + start,
            other.m_storageUpdating->begin() + end + 1,
            m_storageUpdating->begin() + destOffset);
    }
}

SWMRNibbleArray::SaveState SWMRNibbleArray::getSaveState() const
{
    State state = m_stateVisible.load();
    auto* data = m_storageVisible.load();

    if (state == State::Null) {
        return SaveState{nullptr, State::Null};
    }

    if (state == State::Uninit) {
        return SaveState{nullptr, State::Uninit};
    }

    if (data != nullptr && _isAllZero(*data)) {
        return state == State::Init ? SaveState{nullptr, State::Uninit} : SaveState{nullptr, State::Null};
    }

    auto copiedData = std::make_unique<std::array<u8, ARRAY_SIZE>>();
    *copiedData = *data;
    return SaveState{std::move(copiedData), state};
}

std::vector<u8> SWMRNibbleArray::toByteArray() const
{
    auto* data = m_storageVisible.load();
    State state = m_stateVisible.load();

    if (data == nullptr || state == State::Null || state == State::Uninit) {
        return std::vector<u8>();
    }

    return std::vector<u8>(data->begin(), data->end());
}

// ============================================================================
// 私有方法
// ============================================================================

std::unique_ptr<std::array<u8, SWMRNibbleArray::ARRAY_SIZE>> SWMRNibbleArray::_allocateBytes()
{
    // 尝试从对象池获取
    if (!s_bytePool.empty()) {
        auto bytes = std::move(s_bytePool.back());
        s_bytePool.pop_back();
        return bytes;
    }

    return std::make_unique<std::array<u8, ARRAY_SIZE>>();
}

void SWMRNibbleArray::_freeBytes(std::unique_ptr<std::array<u8, ARRAY_SIZE>> bytes)
{
    if (bytes == nullptr) {
        return;
    }
    // 容量上限：超出直接归还堆，防止单线程池因跨线程交接积累的缓冲区而无界膨胀。
    if (s_bytePool.size() >= POOL_CAPACITY_PER_THREAD) {
        return; // unique_ptr 析构归还堆
    }
    s_bytePool.push_back(std::move(bytes));
}

bool SWMRNibbleArray::_isAllZero(const std::array<u8, ARRAY_SIZE>& data)
{
    // 使用 64 位比较加速
    constexpr size_t numU64 = ARRAY_SIZE / sizeof(u64);
    const u64* ptr = reinterpret_cast<const u64*>(data.data());

    for (size_t i = 0; i < numU64; ++i) {
        if (ptr[i] != 0) {
            return false;
        }
    }

    // 检查剩余字节
    for (size_t i = numU64 * sizeof(u64); i < ARRAY_SIZE; ++i) {
        if (data[i] != 0) {
            return false;
        }
    }

    return true;
}

void SWMRNibbleArray::_ensureWritable()
{
    if (m_updatingDirty) {
        return;
    }

    if (m_storageUpdating == nullptr) {
        m_storageUpdating = _allocateBytes();
        std::fill(m_storageUpdating->begin(), m_storageUpdating->end(), 0);
    } else {
        // 检查更新侧和可见侧是否共享同一块内存
        auto* currentVisible = m_storageVisible.load(std::memory_order::acquire);
        bool sharedWithVisible = (m_storageUpdating.get() == currentVisible);

        // 写时复制：创建新数组并复制数据
        auto newStorage = _allocateBytes();
        *newStorage = *m_storageUpdating;

        if (sharedWithVisible) {
            // 如果与可见侧共享内存，先更新可见侧指向新存储
            // 这样旧内存就不会被释放（因为 unique_ptr 的 move 会释放旧内存）
            // 但由于可见侧仍持有指针，我们需要让可见侧也指向新存储
            // 注意：这里不释放旧内存，因为可见侧仍在使用它
            // 但 unique_ptr 的 move 会自动释放旧内存...
            //
            // 正确做法：先让更新侧指向新存储，同时让可见侧也指向新存储
            // 然后旧内存由析构函数处理（只在可见侧 != 更新侧 时释放）
            m_storageUpdating = std::move(newStorage);
            m_storageVisible.store(m_storageUpdating.get(), std::memory_order::release);
            // 此时 m_stateUpdating == m_stateVisible，所以 isDirty() == false
            // 但我们需要标记为 dirty 以便后续 updateVisible() 能正确工作
        } else {
            // 不与可见侧共享，可以安全释放旧内存
            m_storageUpdating = std::move(newStorage);
        }
    }

    if (m_stateUpdating != State::Hidden) {
        m_stateUpdating = State::Init;
    }
    m_updatingDirty = true;
}

} // namespace mc
