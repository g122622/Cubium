#pragma once

#include "../../../core/Types.hpp"
#include "LightEngineUtils.hpp"
#include "LightEngineCache.hpp"
#include <vector>
#include <array>
#include <cstdint>
#include <functional>

namespace mc {

// 前向声明
class StarLightLightingProvider;

/**
 * @brief 基于级别的传播图（Starlight 优化版）
 *
 * 使用方向位集优化的 BFS 传播算法。
 *
 * 队列元素采用显式结构：
 * - pos: 完整世界坐标编码（LightEngineUtils::packPos）
 * - level: 传播级别
 * - directions: 可继续传播方向位集
 * - flags: WRITE/RECHECK/TRANSPARENT 标志
 *
 * 参考: ca.spottedleaf.moonrise.patches.starlight.light.StarLightEngine
 */
class StarLightEngine {
public:
    /** 最大级别数（光照最大15级 + 1个溢出级） */
    static constexpr i32 MAX_LEVEL_COUNT = 16;

    /** 无效级别标记 */
    static constexpr u8 INVALID_LEVEL = 255;

    /** 标志：需要写入光照等级 */
    static constexpr u64 FLAG_WRITE_LEVEL = 1ULL << 63;

    /** 标志：需要重新检查光照等级 */
    static constexpr u64 FLAG_RECHECK_LEVEL = 1ULL << 62;

    /** 标志：有面透明方块 */
    static constexpr u64 FLAG_HAS_SIDED_TRANSPARENT = 1ULL << 61;

    virtual ~StarLightEngine() = default;

    // ========================================================================
    // 公共接口
    // ========================================================================

    /**
     * @brief 检查是否有待处理的更新
     */
    [[nodiscard]] bool needsUpdate() const noexcept { return m_needsUpdate; }

    /**
     * @brief 获取待处理更新数量
     */
    [[nodiscard]] i32 queuedUpdateSize() const noexcept {
        return static_cast<i32>(m_increaseQueueInitialLength + m_decreaseQueueInitialLength);
    }

    /**
     * @brief 处理所有更新
     *
     * @param maxUpdates 最大更新数量
     * @return 剩余配额
     */
    i32 processUpdates(i32 maxUpdates);

    /**
     * @brief 调度位置更新
     * @param pos 位置编码
     */
    void scheduleUpdate(i64 pos);

    /**
     * @brief 调度光照传播更新
     * @param fromPos 源位置
     * @param toPos 目标位置
     * @param level 传播等级
     * @param isIncrease 是否为增亮
     */
    void scheduleUpdate(i64 fromPos, i64 toPos, i32 level, bool isIncrease);

    /**
     * @brief 取消位置更新
     * @param pos 位置编码
     */
    void cancelUpdate(i64 pos);

    /**
     * @brief 取消满足条件的位置更新
     * @param predicate 条件函数
     */
    void cancelUpdates(const std::function<bool(i64)>& predicate);

    /**
     * @brief 设置编码偏移（用于坐标压缩）
     */
    void setEncodeOffset(i32 offsetX, i32 offsetY, i32 offsetZ) {
        (void)offsetX;
        (void)offsetY;
        (void)offsetZ;
    }

    // ========================================================================
    // 缓存管理
    // ========================================================================

    void enableCache(i32 centerX, i32 centerY, i32 centerZ,
                     bool relaxed, bool loadTwoRadius);

    /**
     * @brief 禁用缓存
     *
     * 清除所有缓存数据。
     */
    void disableCache();

    /**
     * @brief 检查缓存是否启用
     */
    [[nodiscard]] bool isCacheEnabled() const noexcept { return m_cacheEnabled; }

    /**
     * @brief 获取缓存命中率（调试用）
     */
    [[nodiscard]] f32 getCacheHitRate() const;

protected:
    /**
     * @brief 构造函数
     * @param levelCount 级别数量（通常为16）
     * @param expectedUpdates 预期更新数量（用于预分配）
     * @param provider 区块光照提供者（用于缓存）
     */
    StarLightEngine(i32 levelCount, i32 expectedUpdates, StarLightLightingProvider* provider);

    // ========================================================================
    // 虚方法（子类实现）
    // ========================================================================

    /** 检查是否为根节点 */
    [[nodiscard]] virtual bool isRoot(i64 pos) const = 0;

    /** 计算位置的新级别 */
    [[nodiscard]] virtual i32 computeLevel(i64 pos, i64 excludedSource, i32 level) = 0;

    /** 通知相邻位置 */
    virtual void notifyNeighbors(i64 pos, i32 level, bool isDecreasing, u8 directionBits) = 0;

    /** 获取位置的当前级别 */
    [[nodiscard]] virtual i32 getLevel(i64 pos) const = 0;

    /** 设置位置的级别 */
    virtual void setLevel(i64 pos, i32 level) = 0;

    /** 计算边缘级别 */
    [[nodiscard]] virtual i32 getEdgeLevel(i64 fromPos, i64 toPos, i32 startLevel) = 0;

    /** 检查区块段是否为空（用于跳过优化） */
    [[nodiscard]] virtual bool isSectionEmpty(i64 sectionPos) const;

    // ========================================================================
    // 缓存访问（供子类使用）
    // ========================================================================

    /**
     * @brief 从缓存获取区块
     */
    [[nodiscard]] const IChunk* getCachedChunk(i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 获取缓存提供者
     */
    [[nodiscard]] StarLightLightingProvider* getChunkProvider() const noexcept { return m_chunkProvider; }

    /**
     * @brief 检查区块段是否为空（使用缓存）
     */
    [[nodiscard]] bool isCachedSectionEmpty(i32 sectionX, i32 sectionY, i32 sectionZ) const;

protected:
    struct QueueEntry {
        i64 pos = 0;
        u8 level = 0;
        u8 directions = DIR_ALL;
        u64 flags = 0;
    };

    // 缓存系统
    LightEngineCache m_cache;
    StarLightLightingProvider* m_chunkProvider = nullptr;
    bool m_cacheEnabled = false;

    /**
     * @brief 传播光照等级到相邻位置
     * @param fromPos 源位置
     * @param toPos 目标位置
     * @param level 传播等级
     * @param isDecreasing 是否为减亮传播
     */
    void propagateLevel(i64 fromPos, i64 toPos, i32 level, bool isDecreasing);

    // ========================================================================
    // 队列操作
    // ========================================================================

    /**
     * @brief 编码队列元素
     */
    [[nodiscard]] QueueEntry encodeQueueEntry(i32 x, i32 y, i32 z, u8 level, u8 directions, u64 flags) const {
        return QueueEntry{packWorldPos(x, y, z), level, directions, flags};
    }

    /**
     * @brief 编码队列元素（使用世界坐标）
     */
    [[nodiscard]] QueueEntry encodeQueueEntryWorld(i32 worldX, i32 worldY, i32 worldZ, u8 level, u8 directions, u64 flags) const {
        return encodeQueueEntry(worldX, worldY, worldZ, level, directions, flags);
    }

    /**
     * @brief 添加到增亮队列
     */
    void appendToIncreaseQueue(const QueueEntry& entry) {
        if (m_increaseQueueInitialLength >= m_increaseQueue.size()) {
            resizeIncreaseQueue();
        }
        m_increaseQueue[m_increaseQueueInitialLength++] = entry;
        m_needsUpdate = true;
    }

    /**
     * @brief 添加到减亮队列
     */
    void appendToDecreaseQueue(const QueueEntry& entry) {
        if (m_decreaseQueueInitialLength >= m_decreaseQueue.size()) {
            resizeDecreaseQueue();
        }
        m_decreaseQueue[m_decreaseQueueInitialLength++] = entry;
        m_needsUpdate = true;
    }

    /**
     * @brief 从世界坐标编码位置
     */
    [[nodiscard]] static constexpr i64 packWorldPos(i32 x, i32 y, i32 z) {
        constexpr i64 XZ_MASK = (1LL << 26) - 1;
        constexpr i64 Y_MASK = (1LL << 12) - 1;
        return ((static_cast<i64>(x) & XZ_MASK) << 38) |
               (static_cast<i64>(y) & Y_MASK) |
               ((static_cast<i64>(z) & XZ_MASK) << 12);
    }

    /**
     * @brief 解码世界坐标
     */
    static void unpackWorldPos(i64 packed, i32& x, i32& y, i32& z) {
        constexpr i64 XZ_MASK = (1LL << 26) - 1;
        x = static_cast<i32>(packed >> 38);
        y = static_cast<i32>((packed << 52) >> 52);
        z = static_cast<i32>((packed >> 12) & XZ_MASK);
        z = (z << 6) >> 6; // 符号扩展
    }

private:
    i32 m_levelCount;

    // 增亮队列（光照增加）
    std::vector<QueueEntry> m_increaseQueue;
    size_t m_increaseQueueInitialLength = 0;

    // 减亮队列（光照减少）
    std::vector<QueueEntry> m_decreaseQueue;
    size_t m_decreaseQueueInitialLength = 0;

    bool m_needsUpdate = false;

    /**
     * @brief 扩展增亮队列
     */
    void resizeIncreaseQueue() {
        m_increaseQueue.resize(m_increaseQueue.size() + (m_increaseQueue.size() >> 1) + 256);
    }

    /**
     * @brief 扩展减亮队列
     */
    void resizeDecreaseQueue() {
        m_decreaseQueue.resize(m_decreaseQueue.size() + (m_decreaseQueue.size() >> 1) + 256);
    }

    /**
     * @brief 处理增亮队列
     */
    i32 processIncreaseQueue(i32 maxUpdates);

    /**
     * @brief 处理减亮队列
     */
    i32 processDecreaseQueue(i32 maxUpdates);

    /**
     * @brief 获取最小级别
     */
    [[nodiscard]] i32 minLevel(i32 level1, i32 level2) const {
        i32 result = level1 < level2 ? level1 : level2;
        return result < m_levelCount ? result : m_levelCount - 1;
    }
};

} // namespace mc
