#pragma once

#include "PlayerChunkTracker.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <vector>

namespace mc::world {

/**
 * @brief 区块追踪管理器
 *
 * 管理所有玩家的区块追踪关系，提供查询接口用于确定哪些玩家在追踪特定区块。
 * 主要用于区块发送系统，确定区块需要发送给哪些玩家。
 *
 * 票据管理已迁移到 ThreadedTicketLevelPropagator + ChunkHolderManager。
 */
class ChunkTrackingManager {
public:
    /**
     * @brief 追踪变化回调类型
     * @param playerId 玩家 ID
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param isTracking true = 开始追踪，false = 停止追踪
     */
    using TrackingChangeCallback = std::function<void(PlayerId player, ChunkCoord x, ChunkCoord z, bool isTracking)>;

    ChunkTrackingManager() = default;
    ~ChunkTrackingManager() = default;

    // 禁止拷贝
    ChunkTrackingManager(const ChunkTrackingManager&) = delete;
    ChunkTrackingManager& operator=(const ChunkTrackingManager&) = delete;

    // ============================================================================
    // 玩家管理
    // ============================================================================

    /**
     * @brief 添加或更新玩家位置
     *
     * @param playerId 玩家 ID
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     */
    void updatePlayerPosition(PlayerId playerId, ChunkCoord x, ChunkCoord z);

    /**
     * @brief 移除玩家
     *
     * @param playerId 玩家 ID
     */
    void removePlayer(PlayerId playerId);

    /**
     * @brief 检查玩家是否存在
     */
    [[nodiscard]] bool hasPlayer(PlayerId playerId) const;

    /**
     * @brief 获取玩家追踪器
     * @return 追踪器指针，不存在返回 nullptr
     */
    [[nodiscard]] const PlayerChunkTracker* getPlayerTracker(PlayerId playerId) const;

    /**
     * @brief 设置玩家视距
     *
     * @param playerId 玩家 ID
     * @param distance 视距（2-32）
     */
    void setPlayerViewDistance(PlayerId playerId, i32 distance);

    /**
     * @brief 获取默认视距
     */
    [[nodiscard]] i32 defaultViewDistance() const { return m_defaultViewDistance; }

    /**
     * @brief 设置默认视距
     */
    void setDefaultViewDistance(i32 distance) { m_defaultViewDistance = std::clamp(distance, 2, 32); }

    // ============================================================================
    // 区块追踪查询
    // ============================================================================

    /**
     * @brief 获取追踪指定区块的所有玩家
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 追踪该区块的玩家 ID 列表
     */
    [[nodiscard]] std::vector<PlayerId> getTrackingPlayers(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 检查指定玩家是否追踪指定区块
     *
     * @param playerId 玩家 ID
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     */
    [[nodiscard]] bool isPlayerTracking(PlayerId playerId, ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 检查指定区块是否有玩家追踪
     *
     * @param chunkKey 区块键（posToKey(x, z)）
     */
    [[nodiscard]] bool hasTrackingPlayers(u64 chunkKey) const;

    /**
     * @brief 获取玩家数量
     */
    [[nodiscard]] size_t playerCount() const;

    // ============================================================================
    // 回调设置
    // ============================================================================

    /**
     * @brief 设置追踪变化回调
     *
     * 当玩家开始或停止追踪某个区块时触发。
     */
    void setTrackingChangeCallback(TrackingChangeCallback callback) {
        m_trackingChangeCallback = std::move(callback);
    }

    // ============================================================================
    // 工具方法
    // ============================================================================

    /**
     * @brief 区块坐标转键
     */
    static constexpr u64 posToKey(ChunkCoord x, ChunkCoord z) {
        return (static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u32>(z);
    }

    /**
     * @brief 键转区块坐标
     */
    static constexpr void keyToPos(u64 key, ChunkCoord& x, ChunkCoord& z) {
        x = static_cast<ChunkCoord>(static_cast<i32>(key >> 32));
        z = static_cast<ChunkCoord>(static_cast<i32>(key & 0xFFFFFFFF));
    }

private:
    /// 玩家追踪器映射
    std::unordered_map<PlayerId, std::unique_ptr<PlayerChunkTracker>> m_playerTrackers;

    /// 区块到玩家的映射（用于快速查询）
    std::unordered_map<u64, std::unordered_set<PlayerId>> m_chunkTrackingPlayers;
    mutable std::mutex m_trackingMutex;

    /// 追踪变化回调
    TrackingChangeCallback m_trackingChangeCallback;

    /// 默认视距
    i32 m_defaultViewDistance = 10;
};

} // namespace mc::world
