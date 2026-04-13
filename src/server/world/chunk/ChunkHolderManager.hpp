#pragma once

#include "common/world/chunk/SingleChunkLifecycleManager.hpp"
#include "common/world/chunk/ThreadedTicketLevelPropagator.hpp"
#include "common/util/concurrent/ReentrantAreaLock.hpp"
#include "common/core/Types.hpp"
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>

namespace mc {

// 前向声明
class ChunkData;
class ChunkPrimer;

namespace server {
class ServerWorld;
}

// ============================================================================
// ChunkHolderManager
// ============================================================================

/**
 * @brief 区块持有者管理器
 *
 * 参考 Moonrise 的 ChunkHolderManager，负责：
 * 1. 集中管理所有区块持有者
 * 2. 卸载队列管理
 * 3. 票据级别变化调度
 * 4. 与 ThreadedTicketLevelPropagator 集成
 */
class ChunkHolderManager {
public:
    /**
     * @brief 区块变化回调类型
     */
    using ChunkChangeCallback = std::function<void(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel)>;

    /**
     * @brief 构造函数
     * @param world 服务端世界（可为 nullptr 用于 IntegratedServer）
     * @param ticketPropagator 票据传播器
     */
    ChunkHolderManager(server::ServerWorld* world, world::ThreadedTicketLevelPropagator& ticketPropagator);

    ~ChunkHolderManager() = default;

    // 禁止拷贝
    ChunkHolderManager(const ChunkHolderManager&) = delete;
    ChunkHolderManager& operator=(const ChunkHolderManager&) = delete;

    // ============================================================================
    // 区块持有者访问
    // ============================================================================

    /**
     * @brief 获取区块持有者
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 持有者指针，不存在返回 nullptr
     */
    [[nodiscard]] SingleChunkLifecycleManager* getChunkHolder(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 获取区块持有者（const 版本）
     */
    [[nodiscard]] const SingleChunkLifecycleManager* getChunkHolder(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 获取或创建区块持有者
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 持有者指针
     */
    SingleChunkLifecycleManager* getOrCreateChunkHolder(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 检查是否存在区块持有者
     */
    [[nodiscard]] bool hasChunkHolder(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 获取所有区块持有者数量
     */
    [[nodiscard]] size_t holderCount() const;

    /**
     * @brief 遍历所有区块持有者
     * @param callback 回调函数
     */
    void forEachHolder(const std::function<void(SingleChunkLifecycleManager&)>& callback);

    /**
     * @brief 遍历所有区块持有者（const 版本）
     */
    void forEachHolder(const std::function<void(const SingleChunkLifecycleManager&)>& callback) const;

    // ============================================================================
    // 票据操作
    // ============================================================================

    /**
     * @brief 添加票据
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param level 票据级别
     * @param ticketType 票据类型名称
     */
    void addTicket(ChunkCoord x, ChunkCoord z, i32 level, const String& ticketType);

    /**
     * @brief 移除票据
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param level 票据级别
     * @param ticketType 票据类型名称
     */
    void removeTicket(ChunkCoord x, ChunkCoord z, i32 level, const String& ticketType);

    /**
     * @brief 处理票据更新
     * @return 是否有更新
     */
    bool processTicketUpdates();

    /**
     * @brief 处理指定区块的票据更新
     * @param sectionX Section X 坐标
     * @param sectionZ Section Z 坐标
     * @return 是否有更新
     */
    bool processTicketUpdates(i32 sectionX, i32 sectionZ);

    // ============================================================================
    // 卸载队列
    // ============================================================================

    /**
     * @brief 将区块加入卸载队列
     * @param holder 区块持有者
     */
    void queueUnload(SingleChunkLifecycleManager* holder);

    /**
     * @brief 处理卸载队列
     * @param maxCount 最大处理数量
     * @return 实际卸载数量
     */
    size_t processUnloadQueue(size_t maxCount = 100);

    /**
     * @brief 获取卸载队列大小
     */
    [[nodiscard]] size_t unloadQueueSize() const;

    /**
     * @brief 清空卸载队列
     */
    void clearUnloadQueue();

    // ============================================================================
    // 区域锁
    // ============================================================================

    /**
     * @brief 获取票据区域锁
     */
    [[nodiscard]] concurrent::ReentrantAreaLock& getTicketLock() { return m_ticketLock; }

    /**
     * @brief 检查当前线程是否持有票据锁
     */
    [[nodiscard]] bool isTicketLockHeldByCurrentThread(ChunkCoord x, ChunkCoord z, i32 radius = 0) const {
        return m_ticketLock.isHeldByCurrentThread(x, z, radius);
    }

    // ============================================================================
    // 票据级别传播器访问
    // ============================================================================

    [[nodiscard]] world::ThreadedTicketLevelPropagator& getTicketPropagator() { return m_ticketPropagator; }
    [[nodiscard]] const world::ThreadedTicketLevelPropagator& getTicketPropagator() const { return m_ticketPropagator; }

    // ============================================================================
    // 回调设置
    // ============================================================================

    /**
     * @brief 设置区块级别变化回调
     */
    void setLevelChangeCallback(ChunkChangeCallback callback) {
        m_levelChangeCallback = std::move(callback);
    }

private:
    /**
     * @brief 区块键生成
     */
    [[nodiscard]] static u64 makeKey(ChunkCoord x, ChunkCoord z) {
        return (static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u32>(z);
    }

    /**
     * @brief 从键解析坐标
     */
    static void fromKey(u64 key, ChunkCoord& x, ChunkCoord& z) {
        x = static_cast<ChunkCoord>(static_cast<i32>(key >> 32));
        z = static_cast<ChunkCoord>(static_cast<i32>(key & 0xFFFFFFFF));
    }

    /**
     * @brief 处理票据级别变化
     */
    void onTicketLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel);

    server::ServerWorld* m_world;
    world::ThreadedTicketLevelPropagator& m_ticketPropagator;

    // 区块持有者映射
    std::unordered_map<u64, std::unique_ptr<SingleChunkLifecycleManager>> m_holders;
    mutable std::mutex m_holdersMutex;

    // 卸载队列
    std::vector<SingleChunkLifecycleManager*> m_unloadQueue;
    mutable std::mutex m_unloadMutex;

    // 票据区域锁
    concurrent::ReentrantAreaLock m_ticketLock;

    // 票据级别变化回调
    ChunkChangeCallback m_levelChangeCallback;

    // 待处理更新标志
    std::atomic<bool> m_hasPendingUpdates{false};
};

} // namespace mc
