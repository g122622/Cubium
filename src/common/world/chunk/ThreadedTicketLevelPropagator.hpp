#pragma once

#include "common/core/Types.hpp"
#include "common/util/concurrent/ReentrantAreaLock.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include <functional>
#include <mutex>
#include <atomic>

namespace mc::world {

// Forward declaration
class Propagator;

/**
 * @brief 线程安全的票据级别传播器
 *
 * 参考 Moonrise 的 ThreadedTicketLevelPropagator 实现。
 *
 * 核心设计：
 * 1. Section 分组：64x64 区块为一个 Section，减少内存使用
 * 2. 增加/减少传播分离队列：高效处理级别变化
 * 3. 区域锁保护：与 ReentrantAreaLock 集成
 *
 * 票据级别：
 * - 级别越小优先级越高
 * - 级别 1-31 用于玩家加载（31 = FULL, 32 = ENTITY_TICKING, 33 = BORDER）
 * - 级别 34+ 表示未加载
 */
class ThreadedTicketLevelPropagator {
    friend class Propagator;  // Allow Propagator to access private members
public:
    /// Section 位移（64x64 区块）
    static constexpr i32 SECTION_SHIFT = 6;
    static constexpr i32 SECTION_SIZE = 1 << SECTION_SHIFT;  // 64
    static constexpr i32 SECTION_MASK = SECTION_SIZE - 1;     // 63

    /// 级别位宽
    static constexpr i32 LEVEL_BITS = SECTION_SHIFT;
    static constexpr i32 LEVEL_COUNT = 1 << LEVEL_BITS;  // 64

    /// 最小源级别（票据级别从 1 开始）
    static constexpr i32 MIN_SOURCE_LEVEL = 1;
    /// 最大源级别（63，避免传播到 0）
    static constexpr i32 MAX_SOURCE_LEVEL = 62;
    /// 最大级别（未加载）
    static constexpr i32 MAX_LEVEL = 64;

    /// 级别变化回调类型
    using LevelChangeCallback = std::function<void(i32 x, i32 z, i32 oldLevel, i32 newLevel)>;

    /**
     * @brief Section 数据结构
     *
     * 每个 Section 存储 64x64 区块的级别信息。
     */
    struct Section {
        /// 级别数据：高 8 位 = 源级别，低 8 位 = 当前级别
        std::array<u16, SECTION_SIZE * SECTION_SIZE> levels{};

        /// 源位置集合（相对于 Section 的局部索引）
        std::unordered_set<u16> sources;

        /// 等待更新的源位置
        std::unordered_map<u16, u8> queuedSources;

        /// 有源的邻居 Section 数量
        i32 oneRadNeighboursWithSources = 0;

        i32 sectionX;
        i32 sectionZ;

        explicit Section(i32 x, i32 z) : sectionX(x), sectionZ(z) {
            levels.fill(0);
        }

        /// 获取局部索引
        static constexpr u16 getLocalIndex(i32 localX, i32 localZ) {
            return static_cast<u16>((localX & SECTION_MASK) | ((localZ & SECTION_MASK) << SECTION_SHIFT));
        }

        /// 获取级别（低 8 位）
        [[nodiscard]] u8 getLevel(u16 localIndex) const {
            return static_cast<u8>(levels[localIndex] & 0xFF);
        }

        /// 获取源级别（高 8 位）
        [[nodiscard]] u8 getSourceLevel(u16 localIndex) const {
            return static_cast<u8>((levels[localIndex] >> 8) & 0xFF);
        }

        /// 设置级别
        void setLevel(u16 localIndex, u8 level) {
            levels[localIndex] = (levels[localIndex] & 0xFF00) | level;
        }

        /// 设置源级别
        void setSourceLevel(u16 localIndex, u8 sourceLevel) {
            levels[localIndex] = (levels[localIndex] & 0x00FF) | (static_cast<u16>(sourceLevel) << 8);
        }

        /// 设置级别和源级别
        void setLevelAndSource(u16 localIndex, u8 level, u8 sourceLevel) {
            levels[localIndex] = level | (static_cast<u16>(sourceLevel) << 8);
        }
    };

    // ============================================================================
    // 构造与析构
    // ============================================================================

    ThreadedTicketLevelPropagator();
    ~ThreadedTicketLevelPropagator() = default;

    // 禁止拷贝
    ThreadedTicketLevelPropagator(const ThreadedTicketLevelPropagator&) = delete;
    ThreadedTicketLevelPropagator& operator=(const ThreadedTicketLevelPropagator&) = delete;

    // ============================================================================
    // 源级别操作（需要在票据锁保护下调用）
    // ============================================================================

    /**
     * @brief 设置源级别
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param level 源级别（1-62）
     *
     * @note 需要在票据锁保护下调用
     */
    void setSource(i32 x, i32 z, i32 level);

    /**
     * @brief 移除源级别
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     *
     * @note 需要在票据锁保护下调用
     */
    void removeSource(i32 x, i32 z);

    // ============================================================================
    // 更新处理
    // ============================================================================

    /**
     * @brief 检查是否有待处理的更新
     */
    [[nodiscard]] bool hasPendingUpdates() const;

    /**
     * @brief 执行更新
     *
     * @param sectionX Section X 坐标
     * @param sectionZ Section Z 坐标
     * @param schedulingLock 调度锁
     * @param outUpdatedPositions 输出：更新的位置和级别
     * @return 是否有更新
     *
     * @note 需要在票据锁保护下调用
     */
    bool performUpdate(i32 sectionX, i32 sectionZ,
                       concurrent::ReentrantAreaLock& schedulingLock,
                       std::vector<std::pair<u64, u8>>& outUpdatedPositions);

    // ============================================================================
    // 级别查询
    // ============================================================================

    /**
     * @brief 获取区块级别
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 区块级别
     */
    [[nodiscard]] i32 getLevel(i32 x, i32 z) const;

    /**
     * @brief 获取 Section
     *
     * @param sectionX Section X 坐标
     * @param sectionZ Section Z 坐标
     * @return Section 指针，不存在返回 nullptr
     */
    [[nodiscard]] Section* getSection(i32 sectionX, i32 sectionZ);

    // ============================================================================
    // 回调设置
    // ============================================================================

    /**
     * @brief 设置级别变化回调
     */
    void setLevelChangeCallback(LevelChangeCallback callback) {
        m_levelChangeCallback = std::move(callback);
    }

    // ============================================================================
    // 调度半径
    // ============================================================================

    /**
     * @brief 获取最大调度半径
     */
    static i32 getMaxSchedulingRadius();

    /**
     * @brief 获取票据锁半径
     */
    static constexpr i32 getTicketLockRadius() { return 1; }

private:
    // ============================================================================
    // 内部方法
    // ============================================================================

    /// 区块坐标转 Section 坐标
    static constexpr i32 toSectionCoord(i32 coord) {
        return coord >> SECTION_SHIFT;
    }

    /// 区块坐标转局部坐标
    static constexpr i32 toLocalCoord(i32 coord) {
        return coord & SECTION_MASK;
    }

    /// 区块坐标转键
    static constexpr u64 posToKey(i32 x, i32 z) {
        return (static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u32>(z);
    }

    /// 键转区块坐标
    static constexpr void keyToPos(u64 key, i32& x, i32& z) {
        x = static_cast<i32>(key >> 32);
        z = static_cast<i32>(key & 0xFFFFFFFF);
    }

    /// Section 坐标转键
    static constexpr u64 sectionToKey(i32 sectionX, i32 sectionZ) {
        return (static_cast<u64>(static_cast<u32>(sectionX)) << 32) | static_cast<u32>(sectionZ);
    }

    /// 获取或创建 Section
    Section* getOrCreateSection(i32 sectionX, i32 sectionZ);

    /// 执行传播
    void performIncrease();
    void performDecrease();

    /// 传播到邻居
    void propagateToNeighbors(i32 x, i32 z, i32 level, bool isDecreasing);

    /// 级别变化通知
    void onLevelChanged(i32 x, i32 z, i32 oldLevel, i32 newLevel);

    // ============================================================================
    // 成员变量
    // ============================================================================

    /// Section 映射
    std::unordered_map<u64, std::unique_ptr<Section>> m_sections;
    mutable std::mutex m_sectionsMutex;

    /// 更新队列
    std::vector<u64> m_updateQueue;
    std::unordered_set<u64> m_pendingSections;
    mutable std::mutex m_updateMutex;

    /// 级别变化回调
    LevelChangeCallback m_levelChangeCallback;
};

} // namespace mc::world
