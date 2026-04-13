#pragma once

#include "common/core/Types.hpp"
#include <unordered_set>
#include <unordered_map>
#include <functional>

namespace mc::world {

/**
 * @brief 玩家区块追踪器
 *
 * 追踪单个玩家的视距范围区块，管理玩家与区块的追踪关系。
 * 用于确定哪些玩家需要接收区块更新。
 */
class PlayerChunkTracker {
public:
    /**
     * @brief 区块进入/离开回调类型
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param isTracking true = 进入视距，false = 离开视距
     */
    using ChunkChangeCallback = std::function<void(ChunkCoord x, ChunkCoord z, bool isTracking)>;

    /**
     * @brief 构造函数
     * @param viewDistance 初始视距
     */
    explicit PlayerChunkTracker(i32 viewDistance = 10);

    /**
     * @brief 设置玩家区块位置
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param enterCallback 区块进入回调（可选）
     * @param leaveCallback 区块离开回调（可选）
     */
    void setPlayerPosition(ChunkCoord x, ChunkCoord z,
                           ChunkChangeCallback enterCallback = nullptr,
                           ChunkChangeCallback leaveCallback = nullptr);

    /**
     * @brief 设置视距
     * @param distance 新视距（2-32）
     * @param enterCallback 区块进入回调（可选）
     * @param leaveCallback 区块离开回调（可选）
     */
    void setViewDistance(i32 distance,
                         ChunkChangeCallback enterCallback = nullptr,
                         ChunkChangeCallback leaveCallback = nullptr);

    /**
     * @brief 获取玩家当前区块 X 坐标
     */
    [[nodiscard]] ChunkCoord playerX() const { return m_playerX; }

    /**
     * @brief 获取玩家当前区块 Z 坐标
     */
    [[nodiscard]] ChunkCoord playerZ() const { return m_playerZ; }

    /**
     * @brief 获取当前视距
     */
    [[nodiscard]] i32 viewDistance() const { return m_viewDistance; }

    /**
     * @brief 检查区块是否在视距范围内
     */
    [[nodiscard]] bool isChunkInRange(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 获取视距范围内的所有区块
     */
    [[nodiscard]] const std::unordered_set<u64>& chunksInRange() const { return m_chunksInRange; }

    /**
     * @brief 检查是否已设置过位置
     */
    [[nodiscard]] bool hasPosition() const { return m_positionSet; }

    /**
     * @brief 清除追踪器状态
     * @param leaveCallback 区块离开回调（可选）
     */
    void clear(ChunkChangeCallback leaveCallback = nullptr);

    /**
     * @brief 计算区块到玩家的切比雪夫距离
     * @return 距离，如果区块不在视距范围内返回 -1
     */
    [[nodiscard]] i32 getDistanceToPlayer(ChunkCoord x, ChunkCoord z) const;

private:
    /**
     * @brief 更新视距范围内的区块集合
     * @param enterCallback 区块进入回调
     * @param leaveCallback 区块离开回调
     */
    void updateChunksInRange(ChunkChangeCallback enterCallback,
                             ChunkChangeCallback leaveCallback);

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

    ChunkCoord m_playerX = 0;
    ChunkCoord m_playerZ = 0;
    i32 m_viewDistance;
    bool m_positionSet = false;

    /// 视距范围内的区块集合
    std::unordered_set<u64> m_chunksInRange;
};

} // namespace mc::world
