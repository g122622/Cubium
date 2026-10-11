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

#pragma once

#include "common/core/Types.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {

/**
 * @brief 单写多读 Nibble 数组 (Single Writer Multi Reader Nibble Array)
 *
 * 参考 Starlight 的 SWMRNibbleArray 实现，支持：
 * - 写时复制 (Copy-on-Write)
 * - 延迟初始化
 * - 状态管理 (NULL/UNINIT/INIT/HIDDEN)
 * - 线程安全的读取
 *
 * 状态说明：
 * - NULL: 不存在，所有读取返回0
 * - UNINIT: 未初始化（全零），不分配内存
 * - INIT: 已初始化，有实际数据
 * - HIDDEN: 已初始化但隐藏，对 Vanilla 来说视为 NULL
 */
class SWMRNibbleArray {
public:
    /** 字节数组大小 (16*16*16 / 2 = 2048) */
    static constexpr size_t ARRAY_SIZE = 2048;

    /** 元素数量 (16*16*16 = 4096) */
    static constexpr size_t VALUE_COUNT = 4096;

    /** 最大值 (4位最大值 = 15) */
    static constexpr u8 MAX_VALUE = 15;

    /**
     * @brief 每线程对象池容量上限。
     *
     * s_bytePool 为 thread_local，分配/释放在调用线程本地完成（无锁热路径）。但 SWMRNibbleArray
     * 会随 ChunkData 跨线程交接（worker 生成、主线程卸载销毁）：销毁线程的池会积累来自其他线程的
     * 缓冲区而无上界，导致单线程池无限膨胀（每个 ChunkData 析构释放 52 个 2KB 缓冲区，多区块转移
     * 后目标线程池可达数 MB 且永不回收，直到线程退出）。
     *
     * 上限取一个既能覆盖单区块光照工作集（LIGHT_SECTIONS=52）又限制病态增长的值：保留 64 个
     * 2KB 缓冲区（128KB），超出直接归还堆。原分配线程的池因缓冲区被转移走而变小，会重新向堆申请，
     * 这是 thread_local 池跨线程交接的固有代价（无法把缓冲区归还到原线程），但容量上限把最坏情况
     * 的每线程驻留控制在 128KB。
     */
    static constexpr size_t POOL_CAPACITY_PER_THREAD = 64;

    /** 状态枚举 */
    enum class State : u8 {
        Null = 0,   // 不存在
        Uninit = 1, // 未初始化（全零）
        Init = 2,   // 已初始化
        Hidden = 3  // 隐藏状态
    };

    // ========================================================================
    // 构造函数和析构函数
    // ========================================================================

    /**
     * @brief 默认构造函数，创建未初始化数组
     */
    SWMRNibbleArray();

    /**
     * @brief 析构函数，释放可见侧存储
     */
    ~SWMRNibbleArray();

    /**
     * @brief 从字节数组构造
     * @param data 字节数组（必须为2048字节，可为nullptr）
     * @param isNull 是否为 null 状态
     */
    SWMRNibbleArray(std::unique_ptr<std::array<u8, ARRAY_SIZE>> data, bool isNull);

    /**
     * @brief 从字节数组构造，指定状态
     * @param data 字节数组
     * @param state 状态
     */
    SWMRNibbleArray(std::unique_ptr<std::array<u8, ARRAY_SIZE>> data, State state);

    /**
     * @brief 移动构造函数
     */
    SWMRNibbleArray(SWMRNibbleArray&& other) noexcept
        : m_stateUpdating(other.m_stateUpdating)
        , m_stateVisible(other.m_stateVisible.load())
        , m_storageUpdating(other.m_storageUpdating)
        , m_storageVisible(other.m_storageVisible.load())
        , m_updatingDirty(other.m_updatingDirty)
        , m_ownedUpdating(std::move(other.m_ownedUpdating))
        , m_ownedVisible(std::move(other.m_ownedVisible))
        , m_retiredVisible(std::move(other.m_retiredVisible))
    {
        other.m_stateUpdating = State::Null;
        other.m_stateVisible.store(State::Null);
        other.m_storageUpdating = nullptr;
        other.m_storageVisible.store(nullptr);
        other.m_updatingDirty = false;
    }

    /**
     * @brief 移动赋值运算符
     */
    SWMRNibbleArray& operator=(SWMRNibbleArray&& other) noexcept
    {
        if (this != &other) {
            // 先摘除可见指针、断开更新侧观察，再释放各属主：可见侧是无锁读的，
            // 指针必须先失效，否则残留的可见指针会指向已释放的数组。
            m_storageVisible.store(nullptr, std::memory_order::relaxed);
            m_storageUpdating = nullptr;
            _freeBytes(std::move(m_retiredVisible));
            _freeBytes(std::move(m_ownedVisible));
            _freeBytes(std::move(m_ownedUpdating));

            m_stateUpdating = other.m_stateUpdating;
            m_stateVisible.store(other.m_stateVisible.load());
            m_storageUpdating = other.m_storageUpdating;
            m_storageVisible.store(other.m_storageVisible.load());
            m_updatingDirty = other.m_updatingDirty;
            m_ownedUpdating = std::move(other.m_ownedUpdating);
            m_ownedVisible = std::move(other.m_ownedVisible);
            m_retiredVisible = std::move(other.m_retiredVisible);

            other.m_stateUpdating = State::Null;
            other.m_stateVisible.store(State::Null);
            other.m_storageUpdating = nullptr;
            other.m_storageVisible.store(nullptr);
            other.m_updatingDirty = false;
        }
        return *this;
    }

    // 禁止拷贝
    SWMRNibbleArray(const SWMRNibbleArray&) = delete;
    SWMRNibbleArray& operator=(const SWMRNibbleArray&) = delete;

    /**
     * @brief 从现有 NibbleArray 数据构造
     * @param data 现有数据（将被复制）
     */
    static SWMRNibbleArray fromData(const std::vector<u8>& data);

    /**
     * @brief 创建全零数组
     */
    static SWMRNibbleArray createUninitialized();

    /**
     * @brief 创建 null 数组
     */
    static SWMRNibbleArray createNull();

    // ========================================================================
    // 状态查询 - 更新侧
    // ========================================================================

    /** 检查是否有未同步的更改 */
    [[nodiscard]] bool isDirty() const;

    /** 检查是否为 null 状态（更新侧） */
    [[nodiscard]] bool isNullUpdating() const;

    /** 检查是否为未初始化状态（更新侧） */
    [[nodiscard]] bool isUninitializedUpdating() const;

    /** 检查是否为已初始化状态（更新侧） */
    [[nodiscard]] bool isInitializedUpdating() const;

    /** 检查是否为隐藏状态（更新侧） */
    [[nodiscard]] bool isHiddenUpdating() const;

    // ========================================================================
    // 状态查询 - 可见侧
    // ========================================================================

    /** 检查是否为 null 状态（可见侧） */
    [[nodiscard]] bool isNullVisible() const;

    /** 检查是否为未初始化状态（可见侧） */
    [[nodiscard]] bool isUninitializedVisible() const;

    /** 检查是否为已初始化状态（可见侧） */
    [[nodiscard]] bool isInitializedVisible() const;

    /** 检查是否为隐藏状态（可见侧） */
    [[nodiscard]] bool isHiddenVisible() const;

    // ========================================================================
    // 状态设置 - 更新侧
    // ========================================================================

    /** 设置为全亮 (15) */
    void setFull();

    /** 设置为全零 */
    void setZero();

    /** 设置为非 null（从 null 转为 uninit） */
    void setNonNull();

    /** 设置为 null 状态 */
    void setNull();

    /** 设置为未初始化状态 */
    void setUninitialized();

    /** 设置为隐藏状态 */
    void setHidden();

    // ========================================================================
    // 元素访问 - 更新侧
    // ========================================================================

    /**
     * @brief 获取指定位置的值（更新侧）
     * @param x X坐标 (0-15)
     * @param y Y坐标 (0-15)
     * @param z Z坐标 (0-15)
     * @return 值 (0-15)
     */
    [[nodiscard]] u8 getUpdating(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取指定索引的值（更新侧）
     * @param index 线性索引 (0-4095)
     * @return 值 (0-15)
     */
    [[nodiscard]] u8 getUpdating(i32 index) const;

    /**
     * @brief 设置指定位置的值（更新侧）
     * @param x X坐标 (0-15)
     * @param y Y坐标 (0-15)
     * @param z Z坐标 (0-15)
     * @param value 值 (0-15)
     */
    void set(i32 x, i32 y, i32 z, u8 value);

    /**
     * @brief 设置指定索引的值（更新侧）
     * @param index 线性索引 (0-4095)
     * @param value 值 (0-15)
     */
    void set(i32 index, u8 value);

    // ========================================================================
    // 元素访问 - 可见侧（线程安全）
    // ========================================================================

    /**
     * @brief 获取指定位置的值（可见侧，线程安全）
     */
    [[nodiscard]] u8 getVisible(i32 x, i32 y, i32 z) const;

    /**
     * @brief 获取指定索引的值（可见侧，线程安全）
     */
    [[nodiscard]] u8 getVisible(i32 index) const;

    // ========================================================================
    // 同步操作
    // ========================================================================

    /**
     * @brief 将更新侧数据同步到可见侧
     * @return 如果有更改返回 true
     */
    bool updateVisible();

    // ========================================================================
    // 扩展操作
    // ========================================================================

    /**
     * @brief 从另一个数组向下挤压（天空光照用）
     * @param other 源数组
     */
    void extrudeLower(const SWMRNibbleArray& other);

    /**
     * @brief 获取保存状态
     * @return 保存状态，如果为 null 返回 nullptr
     */
    struct SaveState {
        std::unique_ptr<std::array<u8, ARRAY_SIZE>> data;
        State state;
    };
    [[nodiscard]] SaveState getSaveState() const;

    /**
     * @brief 转换为普通字节数组
     * @return 数据副本，如果为 null/uninit 返回空数组
     */
    [[nodiscard]] std::vector<u8> toByteArray() const;

    /**
     * @brief 原地导出可见侧数据到调用方缓冲
     *
     * 与 toByteArray 的区别：不产生堆分配。落盘等"写完即弃"的热路径应优先用本方法，
     * 避免每段一次 2 KB 的 vector 分配（单区块 24 段 × 2 层光照）。
     *
     * @param out 目标缓冲，容量至少 ARRAY_SIZE
     * @return 是否导出了有效数据；false 表示该 nibble 处于 Null/Uninit（无数据），
     *         调用方应自行按默认值（天空光全亮 15 / 方块光全 0）处理
     */
    bool copyVisibleTo(u8* out) const;

    // ========================================================================
    // 工具方法
    // ========================================================================

    /**
     * @brief 计算线性索引
     */
    [[nodiscard]] static constexpr i32 getIndex(i32 x, i32 y, i32 z)
    {
        return (x & 15) | ((z & 15) << 4) | ((y & 15) << 8);
    }

private:
    /**
     * @brief 分配字节数组
     */
    static std::unique_ptr<std::array<u8, ARRAY_SIZE>> _allocateBytes();

    /**
     * @brief 释放字节数组到对象池
     */
    static void _freeBytes(std::unique_ptr<std::array<u8, ARRAY_SIZE>> bytes);

    /**
     * @brief 检查数组是否全零
     */
    static bool _isAllZero(const std::array<u8, ARRAY_SIZE>& data);

    /**
     * @brief 确保存储可写（写时复制）
     */
    void _ensureWritable();

    /**
     * @brief 为"整块覆写"准备更新侧私有缓冲（对应 Moonrise setFull/setZero/extrudeLower 的分配分支）
     *
     * 若当前更新侧已是 dirty 的私有缓冲，则复用它；否则分配一块新的私有缓冲。
     * 【关键】绝不释放旧缓冲：未 dirty 时 m_storageUpdating 观察的正是已发布的可见侧数组，
     * 它由 m_ownedVisible 持有、仍被无锁读者使用。
     *
     * 与 Moonrise 一致，本方法不修改 m_storageVisible——可见侧要到 updateVisible() 才发布。
     * 调用后 m_storageUpdating == m_ownedUpdating.get() 且非空。
     */
    void _prepareUpdatingForOverwrite();

    /**
     * @brief 把一块"曾作为可见侧"的缓冲放入退役槽，并归还上一代退役缓冲
     *
     * 可见侧是无锁读的：替换或取消发布可见侧数组的瞬间，可能有读者刚载入旧指针、尚未读完，
     * 因此不能立即归还对象池。本方法让该数组多存活一代，再于下一次调用（或析构）时归还。
     * 每 nibble 至多多驻留一块（2 KB）。
     */
    void _retireBytes(std::unique_ptr<std::array<u8, ARRAY_SIZE>> bytes);

    // 状态
    State m_stateUpdating = State::Uninit;
    std::atomic<State> m_stateVisible{State::Uninit};

    /**
     * @brief 更新侧缓冲的裸观察指针（不拥有）
     *
     * 【为什么改成裸指针】可见侧是无锁读的：其他线程直接经 m_storageVisible 载入裸指针后读取，
     * 不参与引用计数。当 nibble 未 dirty 时，更新侧与可见侧指向**同一块**数组（单一缓冲，
     * 与 Moonrise 一致，用于省内存）。此时若用 unique_ptr 表达"更新侧"，任何对它的重新赋值
     * 或置空都会立即 delete 掉这块仍被读者持有的数组，造成 heap-use-after-free。ASAN 实测两处
     * （均为 2048 字节区）：
     *   - it11：setNull() 释放 → 同线程稍后 setFull() 写入（SkyLightEngine::initNibble）；
     *   - it14：_ensureWritable() 释放 → 主线程 getVisible() 读取（SpreadableSnowyDirtBlock::randomTick）。
     *
     * 因此本类把"指针"与"所有权"彻底分离：本成员只观察，所有权一律由下面的 unique_ptr 表达。
     * 写者/读者语义与 Moonrise 一致（单写者 + 无锁多读者）。
     */
    std::array<u8, ARRAY_SIZE>* m_storageUpdating = nullptr;

    /**
     * @brief 可见侧缓冲的发布指针（无锁读取入口，不拥有）
     *
     * 读取方经此指针访问数组；其生命周期由 m_ownedVisible（当前已发布）、m_ownedUpdating
     * （未 dirty 时与可见侧别名）或 m_retiredVisible（刚被取消发布的上一代）保证——
     * 三者合一构成"曾被发布的数组在对象存活期间绝不被归还"的硬保证。
     */
    std::atomic<std::array<u8, ARRAY_SIZE>*> m_storageVisible{nullptr};

    bool m_updatingDirty = false;

    /**
     * @brief 更新侧私有缓冲的所有者（仅 dirty 期间存在）
     *
     * 不变式：m_ownedUpdating 非空 ⟺ m_updatingDirty，且此时 m_storageUpdating 指向它。
     * 它是**未发布**的私有缓冲，因此归还它在任何时刻都是安全的；已发布的可见侧数组
     * 一律归 m_ownedVisible，绝不由本成员持有（否则写时复制会顺带释放读者仍在用的内存）。
     */
    std::unique_ptr<std::array<u8, ARRAY_SIZE>> m_ownedUpdating;

    /**
     * @brief 已发布可见侧缓冲的所有者（当它与更新侧不别名时）
     *
     * 【为什么需要它】Java 版（Moonrise SWMRNibbleArray）依赖 GC 保证已发布数组不被回收：
     * 例如 swapUpdatingAndMarkDirty 中 `arraycopy(storageUpdating, 0,
     * storageUpdating = allocateBytes(), ...)` 之后，旧数组仅由 storageVisible 引用，靠 GC 存活。
     * C++ 必须显式持有所有权，否则旧数组会在替换时被立即释放，而并发读者仍持有其裸指针。
     *
     * 所有权不变式（任一数组至多出现在一个 unique_ptr 中）：
     *   - 未 dirty：m_storageUpdating 与 m_storageVisible 指向同一数组，该数组由 m_ownedVisible
     *     拥有（此时 m_ownedUpdating 必为空）；
     *   - 已 dirty：更新侧私有缓冲归 m_ownedUpdating，已发布的可见侧数组仍归 m_ownedVisible；
     *   - 取消发布（Null/Uninit）后：可见侧数组移入 m_retiredVisible，直到对象析构或下次发布时
     *     被复用为止，绝不归还对象池。
     *
     * 由此可得一条便于推理的结论：**m_ownedUpdating 永远不会拥有已发布的可见侧数组**，
     * 因此"归还 m_ownedUpdating"在任何时刻都是安全的；而 m_ownedVisible 只在
     * updateVisible() 取消发布时才经 m_retiredVisible 转移，同样不会被释放。
     */
    std::unique_ptr<std::array<u8, ARRAY_SIZE>> m_ownedVisible;

    /**
     * @brief 上一代"曾作为可见侧"的缓冲（退役槽，不归还）
     *
     * 可见侧是无锁读的：取消发布可见侧数组的瞬间，可能有读者刚经 m_storageVisible 载入旧指针、
     * 尚未读完。因此旧数组不能归还对象池，否则该读者会命中 heap-use-after-free。本槽接收该数组，
     * 直到对象析构或下次发布可见侧时被复用，从而给出**硬性生命周期保证**：
     * 只要本对象存活，任何曾被发布的数组就始终由本对象持有（m_ownedVisible 或本槽），
     * 读者绝不会读到已释放内存。
     *
     * 不变式：本槽非空 ⟺ m_storageVisible == nullptr（发布分支会先取走本槽再发布），
     * 故本槽至多驻留一块，每 nibble 额外驻留上限为 2 KB。
     *
     * TODO: 复用本槽（以及 updateVisible() 向已发布数组原地 memcpy）时，可能与仍持有旧指针的
     *  读者并发访问同一块内存，构成 C++ 数据竞争（读者读到的是**同一 nibble** 的上一代或当前代
     *  光照值，语义良性，但形式上仍是 UB）。Moonrise 依赖 JMM 对 byte 数组"良性竞争"的定义，
     *  C++ 无此保证。要彻底消除需引入引用计数或 hazard pointer/epoch 回收，代价是在 getVisible
     *  热路径上增加原子操作，需另行评估性能预算。
     */
    std::unique_ptr<std::array<u8, ARRAY_SIZE>> m_retiredVisible;

    // 对象池（线程本地）
    static thread_local std::vector<std::unique_ptr<std::array<u8, ARRAY_SIZE>>> s_bytePool;
};

} // namespace mc
