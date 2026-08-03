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

#include "../../../core/Types.hpp"
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
        , m_storageUpdating(std::move(other.m_storageUpdating))
        , m_storageVisible(other.m_storageVisible.load())
        , m_updatingDirty(other.m_updatingDirty)
    {
        other.m_stateUpdating = State::Null;
        other.m_stateVisible.store(State::Null);
        other.m_storageVisible.store(nullptr);
        other.m_updatingDirty = false;
    }

    /**
     * @brief 移动赋值运算符
     */
    SWMRNibbleArray& operator=(SWMRNibbleArray&& other) noexcept
    {
        if (this != &other) {
            // 释放当前的可见存储
            auto* oldVisible = m_storageVisible.load();
            if (oldVisible != nullptr && oldVisible != m_storageUpdating.get()) {
                _freeBytes(std::unique_ptr<std::array<u8, ARRAY_SIZE>>(oldVisible));
            }

            m_stateUpdating = other.m_stateUpdating;
            m_stateVisible.store(other.m_stateVisible.load());
            m_storageUpdating = std::move(other.m_storageUpdating);
            m_storageVisible.store(other.m_storageVisible.load());
            m_updatingDirty = other.m_updatingDirty;

            other.m_stateUpdating = State::Null;
            other.m_stateVisible.store(State::Null);
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

    // 状态
    State m_stateUpdating = State::Uninit;
    std::atomic<State> m_stateVisible{State::Uninit};

    // 存储缓冲区
    std::unique_ptr<std::array<u8, ARRAY_SIZE>> m_storageUpdating;
    std::atomic<std::array<u8, ARRAY_SIZE>*> m_storageVisible{nullptr};
    bool m_updatingDirty = false;

    // 对象池（线程本地）
    static thread_local std::vector<std::unique_ptr<std::array<u8, ARRAY_SIZE>>> s_bytePool;
};

} // namespace mc
