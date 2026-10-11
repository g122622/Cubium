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
    // 先摘除可见指针、断开更新侧观察，再释放各所有者（可见侧是无锁读的，指针必须先失效）。
    // 归还对象池而非直接 delete：与既有 _allocateBytes/_freeBytes 的池化约定一致。
    m_storageVisible.store(nullptr, std::memory_order::relaxed);
    m_storageUpdating = nullptr;
    _freeBytes(std::move(m_retiredVisible));
    _freeBytes(std::move(m_ownedVisible));
    _freeBytes(std::move(m_ownedUpdating));
}

SWMRNibbleArray::SWMRNibbleArray(std::unique_ptr<std::array<u8, ARRAY_SIZE>> data, bool isNull)
    : m_stateUpdating(data ? State::Init : (isNull ? State::Null : State::Uninit))
    , m_stateVisible(m_stateUpdating)
    , m_storageUpdating(data.get())
    , m_storageVisible(data.get()) // 与 Moonrise 一致：初始时更新侧与可见侧指向同一数组
    , m_updatingDirty(false)
    , m_ownedVisible(std::move(data))
{}

SWMRNibbleArray::SWMRNibbleArray(std::unique_ptr<std::array<u8, ARRAY_SIZE>> data, State state)
    : m_stateUpdating(state)
    , m_stateVisible(state)
    , m_storageUpdating(data.get())
    , m_storageVisible(data.get()) // 与 Moonrise 一致：初始时更新侧与可见侧指向同一数组
    , m_updatingDirty(false)
    , m_ownedVisible(std::move(data))
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

    // 与 Moonrise 一致：只准备更新侧缓冲，可见侧要等 updateVisible() 才发布。
    _prepareUpdatingForOverwrite();
    std::fill(m_storageUpdating->begin(), m_storageUpdating->end(), static_cast<u8>(0xFF)); // 每字节两个15
    m_updatingDirty = true;
}

void SWMRNibbleArray::setZero()
{
    if (m_stateUpdating != State::Hidden) {
        m_stateUpdating = State::Init;
    }

    // 与 Moonrise 一致：只准备更新侧缓冲，可见侧要等 updateVisible() 才发布。
    _prepareUpdatingForOverwrite();
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
    // 与 Moonrise 一致：只把"更新侧"字段置空，不动可见侧。
    // dirty 时 m_ownedUpdating 是未发布的私有缓冲，可安全归还；未 dirty 时它为 null，
    // 更新侧观察的正是可见侧数组，仍由 m_ownedVisible 持有并被读者使用，故绝不能在此释放。
    _freeBytes(std::move(m_ownedUpdating));
    m_ownedUpdating = nullptr;
    m_storageUpdating = nullptr;
    m_updatingDirty = false;
}

void SWMRNibbleArray::setUninitialized()
{
    m_stateUpdating = State::Uninit;
    // 同 setNull()：只归还未发布的私有缓冲，已发布的可见侧数组必须保活。
    _freeBytes(std::move(m_ownedUpdating));
    m_ownedUpdating = nullptr;
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

    // 同步更新 - SWMR 模式：与 Moonrise updateVisible() 一致
    if (m_stateUpdating == State::Null || m_stateUpdating == State::Uninit) {
        // 可见侧不再发布任何存储。注意：可见侧是无锁读的，刚载入旧指针的读者可能尚未读完，
        // 故旧数组不能立即归还，而是移入退役槽延迟一代回收。
        auto* oldVisible = m_storageVisible.exchange(nullptr, std::memory_order::release);

        // 已发布的数组必由 m_ownedVisible 持有（见所有权不变式）；此处绝不释放它。
        if (oldVisible != nullptr) {
            MC_ASSERT_RELEASE(m_ownedVisible != nullptr && m_ownedVisible.get() == oldVisible);
            _retireBytes(std::move(m_ownedVisible));
        }

        // 归还未发布的私有缓冲（若上一步未动它则在此归还；_freeBytes 对空指针安全）。
        _freeBytes(std::move(m_ownedUpdating));
        m_ownedUpdating = nullptr;
        m_storageUpdating = nullptr;
    } else {
        // Init 或 Hidden 状态：把更新侧数据同步到可见侧
        auto* currentVisible = m_storageVisible.load(std::memory_order::acquire);

        if (currentVisible == nullptr) {
            // 可见侧尚未发布：克隆更新侧数据作为新的可见数组（对应 Moonrise 的 clone()）。
            // 此时 m_ownedVisible 必为空（可见指针为空 ⟺ 可见数组无属主），断言以防不变式被破坏
            // 后静默释放一块仍被读者持有的数组。
            MC_ASSERT_RELEASE(m_ownedVisible == nullptr);
            auto newStorage = _allocateBytes();
            if (m_storageUpdating != nullptr) {
                *newStorage = *m_storageUpdating;
            } else {
                std::fill(newStorage->begin(), newStorage->end(), 0);
            }
            m_ownedVisible = std::move(newStorage);
            currentVisible = m_ownedVisible.get();
            m_storageVisible.store(currentVisible, std::memory_order::release);
        } else if (m_storageUpdating != nullptr && m_storageUpdating != currentVisible) {
            // 可见侧已有独立存储：把更新侧数据拷入可见侧
            std::memcpy(currentVisible->data(), m_storageUpdating->data(), ARRAY_SIZE);
        }

        // 更新侧私有缓冲已完成使命：归还，并让更新侧重新观察可见侧（未 dirty：两者别名）
        _freeBytes(std::move(m_ownedUpdating));
        m_ownedUpdating = nullptr;
        m_storageUpdating = currentVisible;
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
        // 与 Moonrise 一致：分配一块新的私有缓冲用于写入（对应
        // `into = storageUpdating = allocateBytes()`）。旧数组若已发布，则由 m_ownedVisible
        // 继续持有并被无锁读者使用，这里只是让更新侧改看新缓冲，绝不动它。
        if (m_storageUpdating == nullptr) {
            m_stateUpdating = State::Init;
        }
        _prepareUpdatingForOverwrite();
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

bool SWMRNibbleArray::copyVisibleTo(u8* out) const
{
    MC_ASSERT_RELEASE(out != nullptr);

    auto* data = m_storageVisible.load();
    const State state = m_stateVisible.load();

    if (data == nullptr || state == State::Null || state == State::Uninit) {
        return false;
    }

    std::memcpy(out, data->data(), ARRAY_SIZE);
    return true;
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

void SWMRNibbleArray::_prepareUpdatingForOverwrite()
{
    // 已是 dirty 的私有缓冲 → 直接复用（对应 Moonrise 三元表达式的 else 分支）。
    if (m_storageUpdating != nullptr && m_updatingDirty) {
        return;
    }

    // 未 dirty：m_storageUpdating 观察的是已发布的可见侧数组（由 m_ownedVisible 持有），
    // 或为 nullptr。两者都不能释放，直接换上一块新的私有缓冲即可。
    // 注意：此时 m_ownedUpdating 必为空（私有缓冲只在 dirty 期间存在）。
    MC_ASSERT_RELEASE(m_ownedUpdating == nullptr);
    m_ownedUpdating = _allocateBytes();
    m_storageUpdating = m_ownedUpdating.get();
}

void SWMRNibbleArray::_retireBytes(std::unique_ptr<std::array<u8, ARRAY_SIZE>> bytes)
{
    if (bytes == nullptr) {
        return;
    }
    // 归还上一代退役缓冲，让当前这块顶替它，从而保证"曾被发布的数组"至少再存活一代。
    _freeBytes(std::move(m_retiredVisible));
    m_retiredVisible = std::move(bytes);
}

void SWMRNibbleArray::_ensureWritable()
{
    if (m_updatingDirty) {
        return;
    }

    if (m_storageUpdating == nullptr) {
        // 无任何数据：分配一块清零的私有缓冲。
        m_ownedUpdating = _allocateBytes();
        std::fill(m_ownedUpdating->begin(), m_ownedUpdating->end(), 0);
    } else {
        // 写时复制：分配新缓冲并复制当前（可见侧）数据。
        // 【关键】旧缓冲是已发布的可见侧数组，仍由 m_ownedVisible 持有并被无锁读者使用，
        // 因此这里既不移动也不释放它，只是让更新侧改为观察新的私有缓冲。
        // 这正是 ASAN it14 的崩溃点（旧代码在此释放，主线程 getVisible 随后读取）。
        m_ownedUpdating = _allocateBytes();
        *m_ownedUpdating = *m_storageUpdating;
    }

    m_storageUpdating = m_ownedUpdating.get();
    if (m_stateUpdating != State::Hidden) {
        m_stateUpdating = State::Init;
    }
    m_updatingDirty = true;
}

} // namespace mc
