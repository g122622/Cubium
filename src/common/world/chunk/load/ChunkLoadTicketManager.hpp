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
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/load/ChunkDistanceGraph.hpp"
#include "common/world/chunk/load/ChunkLoadLevel.hpp"
#include "common/world/chunk/load/ChunkLoadTicket.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc::world::chunk {

// 前向声明
class SingleChunkLifecycleManager;

/**
 * @brief 区块加载票据管理器
 *
 * 参考 Minecraft 的 TicketManager，管理所有区块的票据，计算区块加载级别。
 *
 * 使用方法：
 * 1. 创建管理器并设置视距
 * 2. 设置级别变化回调
 * 3. 更新玩家位置或添加/移除票据
 * 4. 调用 processUpdates() 或 tick() 处理更新
 *
 * @example
 * @code
 * ChunkLoadTicketManager manager;
 * manager.setViewDistance(10);
 *
 * // 设置回调
 * manager.setLevelChangeCallback([](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
 *     if (newLevel <= 33 && oldLevel > 33) {
 *         loadChunk(x, z);
 *     } else if (newLevel > 33 && oldLevel <= 33) {
 *         unloadChunk(x, z);
 *     }
 * });
 *
 * // 玩家进入
 * manager.updatePlayerPosition(playerId, chunkX, chunkZ);
 *
 * // 处理更新
 * manager.processUpdates();
 *
 * // 玩家离开
 * manager.removePlayer(playerId);
 * @endcode
 *
 * 票据类型说明：
 * - 玩家来源：通过统一 source aggregator 注入距离图
 * - 强制加载票据：通过 API 添加，永久加载
 * - 传送门票据：临时加载，有过期时间
 *
 * @note 必须定期调用 tick() 或 processUpdates() 来处理更新
 */
class ChunkLoadTicketManager {
public:
    /// 最大加载级别（Level <= BORDER 的区块会被加载）
    /// BORDER = 34，对应完全加载区块的外围
    /// 未加载级别 = ChunkLoadLevel::MaxLevel = 46
    static constexpr i32 MAX_LOADED_LEVEL = static_cast<i32>(ChunkLoadLevel::Border);

    ChunkLoadTicketManager();
    ~ChunkLoadTicketManager() = default;

    /**
     * @brief 注册票据
     *
     * @tparam T 票据值类型
     * @param type 票据类型
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param level 票据级别
     * @param value 关联值
     *
     * @example
     * @code
     * // 添加强制加载票据
     * manager.registerTicket(TicketTypes::FORCED, 100, 200, 31, ChunkPos(100, 200));
     * @endcode
     */
    template <typename T>
    void registerTicket(const ChunkLoadTicketType<T>& type, ChunkCoord x, ChunkCoord z, i32 level, const T& value)
    {
        ChunkPos pos(x, z);
        ChunkLoadTicket ticket(type, level, value);
        _addTicket(pos, std::move(ticket));
    }

    /**
     * @brief 移除票据
     *
     * @tparam T 票据值类型
     * @param type 票据类型
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param level 票据级别
     * @param value 关联值
     *
     * @example
     * @code
     * // 移除强制加载票据
     * manager.releaseTicket(TicketTypes::FORCED, 100, 200, 31, ChunkPos(100, 200));
     * @endcode
     */
    template <typename T>
    void releaseTicket(const ChunkLoadTicketType<T>& type, ChunkCoord x, ChunkCoord z, i32 level, const T& value)
    {
        ChunkPos pos(x, z);
        ChunkLoadTicket ticket(type, level, value);
        _removeTicket(pos, ticket);
    }

    /**
     * @brief 更新玩家位置
     *
     * 当玩家移动到新区块时调用。会自动：
     * 1. 更新玩家来源中心点
     * 2. 重建该玩家的追踪覆盖集合
     * 3. 触发区块加载/卸载与追踪 enter/leave
     *
     * @param playerId 玩家 ID
     * @param x 新区块 X 坐标
     * @param z 新区块 Z 坐标
     *
     * @note 调用后会自动处理更新
     *
     * @example
     * @code
     * // 玩家从 (0, 0) 移动到 (1, 0)
     * manager.updatePlayerPosition(playerId, 1, 0);
     * // 自动触发相关区块的加载和卸载
     * @endcode
     */
    void updatePlayerPosition(PlayerId playerId, ChunkCoord x, ChunkCoord z);

    /**
     * @brief 移除玩家
     *
     * 玩家离开时调用。会移除该玩家相关的来源和追踪覆盖。
     *
     * @param playerId 玩家 ID
     *
     * @note 调用后会自动处理更新
     */
    void removePlayer(PlayerId playerId);

    /**
     * @brief 获取区块级别
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 区块级别，越小优先级越高
     *
     * @note 级别 <= 34 (Border) 的区块应该被加载
     */
    [[nodiscard]] i32 getChunkLevel(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 检查区块是否应该加载
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return true 表示区块应该被加载
     */
    [[nodiscard]] bool shouldChunkLoad(ChunkCoord x, ChunkCoord z) const
    {
        return shouldChunkLoad(getChunkLevel(x, z));
    }

    /**
     * @brief 检查区块是否应该加载（静态版本）
     *
     * @param level 区块级别
     * @return true 表示区块应该被加载
     */
    [[nodiscard]] static bool shouldChunkLoad(i32 level) { return level <= MAX_LOADED_LEVEL; }

    /**
     * @brief 处理票据更新
     *
     * 清理过期票据、处理距离图更新。
     * 应该每 tick 调用一次。
     */
    void tick();

    /**
     * @brief 设置视距
     *
     * @param distance 视距（区块数）
     *
     * @note 改变视距会触发区块加载/卸载
     * @note 会更新所有已注册玩家的来源和追踪覆盖
     */
    void setViewDistance(i32 distance);

    /** @brief 获取当前视距 */
    [[nodiscard]] i32 viewDistance() const { return m_viewDistance; }

    /** @brief 获取玩家数量 */
    [[nodiscard]] size_t playerCount() const { return m_playerPositions.size(); }

    /** @brief 获取总票据数量 */
    [[nodiscard]] size_t totalTicketCount() const;

    /**
     * @brief 强制加载区块
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param force true 表示强制加载，false 表示取消强制加载
     *
     * @note 强制加载的区块不会因玩家离开而卸载
     */
    void forceChunk(ChunkCoord x, ChunkCoord z, bool force);

    /**
     * @brief 级别变化回调类型
     *
     * 参数：
     * - x, z: 区块坐标
     * - oldLevel: 旧级别
     * - newLevel: 新级别
     *
     * 当 newLevel <= 34 且 oldLevel > 34 时，区块应该被加载
     * 当 newLevel > 34 且 oldLevel <= 34 时，区块应该被卸载
     */
    using LevelChangeCallback = std::function<void(ChunkCoord, ChunkCoord, i32, i32)>;

    /**
     * @brief 设置级别变化回调
     * @param callback 回调函数
     */
    void setLevelChangeCallback(LevelChangeCallback callback)
    {
        MC_ASSERT_RELEASE(!m_levelChangeCallback);
        m_levelChangeCallback = std::move(callback);
    }

    /**
     * @brief 追踪变化回调类型
     *
     * 参数：
     * - playerId: 玩家 ID
     * - x, z: 区块坐标
     * - isTracking: true 表示玩家开始追踪该区块，false 表示停止追踪
     */
    using TrackingChangeCallback = std::function<void(PlayerId, ChunkCoord, ChunkCoord, bool)>;

    /**
     * @brief 设置追踪变化回调
     * @param callback 回调函数
     *
     * 当玩家进入或离开某区块的视距范围时触发。
     * 可用于自动发送区块数据或卸载通知。
     */
    void setTrackingChangeCallback(TrackingChangeCallback callback) { m_trackingChangeCallback = std::move(callback); }

    /**
     * @brief 区块缓存中心变化回调类型
     *
     * 参数：
     * - playerId: 玩家 ID
     * - x, z: 玩家当前所在区块坐标（新中心）
     *
     * 当玩家所在区块中心变化时触发（含首次设置）。对齐 vanilla ChunkMap.applyChunkTrackingView
     * 在玩家区块中心变化时发送 ClientboundSetChunkCacheCenterPacket 的语义——客户端
     * ClientChunkCache.Storage 的 viewCenterX/Z 默认 (0,0)，唯有收到此回调对应的包才更新，
     * 否则出生点远离原点的玩家收到的区块会被 “Ignoring chunk since it's not in the view
     * range” 丢弃。回调仅传递坐标，不含网络语义（common 层不感知协议）。
     */
    using ChunkCacheCenterCallback = std::function<void(PlayerId, ChunkCoord, ChunkCoord)>;

    /**
     * @brief 设置区块缓存中心变化回调
     * @param callback 回调函数
     *
     * 在 updatePlayerPosition 检测到玩家区块中心变化时调用，须先于区块数据发送。
     */
    void setChunkCacheCenterCallback(ChunkCacheCenterCallback callback)
    {
        m_chunkCacheCenterCallback = std::move(callback);
    }

    /**
     * @brief 获取追踪某区块的所有玩家
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 追踪该区块的玩家 ID 列表
     */
    [[nodiscard]] std::vector<PlayerId> getTrackingPlayers(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 检查玩家是否追踪某区块
     *
     * @param playerId 玩家 ID
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return true 表示玩家正在追踪该区块
     */
    [[nodiscard]] bool isPlayerTracking(PlayerId playerId, ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 检查区块是否有玩家追踪
     *
     * @param chunkKey 区块键（_posToKey 生成）
     * @return true 表示有至少一个玩家追踪该区块
     */
    [[nodiscard]] bool hasTrackingPlayers(u64 chunkKey) const;

    /**
     * @brief 处理所有待处理的更新
     *
     * 处理票据更新和距离图传播。
     * 应该在添加/移除票据后调用。
     */
    void processUpdates();

    /**
     * @brief 获取距离图（用于调试/测试）
     * @return 距离图的常量引用
     */
    [[nodiscard]] const ChunkDistanceGraph& distanceGraph() const { return m_distanceGraph; }

    /**
     * @brief 获取区块的票据集合
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 票据集合指针，如果区块没有票据则返回 nullptr
     */
    [[nodiscard]] const ChunkTicketSet* getChunkTickets(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 获取所有强制加载的区块
     *
     * 遍历所有票据，找出包含 FORCED 票据的区块。
     *
     * @return 强制加载区块的坐标列表
     */
    [[nodiscard]] std::vector<ChunkPos> getForcedChunks() const;

    /**
     * @brief 检查指定区块是否被强制加载
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return true 表示该区块有强制加载票据
     */
    [[nodiscard]] bool isForcedChunk(ChunkCoord x, ChunkCoord z) const;

private:
    struct PlayerSourceState {
        ChunkPos center{0, 0};
        bool hasPosition = false;
        std::unordered_set<u64> trackedChunks;
    };

    /// 内部票据操作
    void _addTicket(ChunkPos pos, ChunkLoadTicket ticket);
    void _removeTicket(ChunkPos pos, const ChunkLoadTicket& ticket);

    /// 重新计算单个区块的聚合源级别并同步到距离图
    void _refreshChunkSourceLevel(ChunkCoord x, ChunkCoord z);

    /// 更新玩家来源中心区块
    void _updatePlayerSourceCenter(const ChunkPos* oldPos, const ChunkPos* newPos);

    /// 计算某个玩家在给定中心和视距下覆盖的区块集合
    [[nodiscard]] std::unordered_set<u64> _buildTrackedChunkSet(ChunkCoord centerX, ChunkCoord centerZ) const;

    /// 应用玩家追踪覆盖变化并派发 enter/leave 回调
    void _applyTrackingDelta(
        PlayerId playerId, const std::unordered_set<u64>& oldChunks, const std::unordered_set<u64>& newChunks);

    /// 根据当前玩家位置和视距重建所有玩家来源
    void _rebuildAllPlayerSources();

    /// 区块位置转键
    [[nodiscard]] static u64 _posToKey(ChunkCoord x, ChunkCoord z) { return mc::math::chunkPosToId(x, z); }

    /// 每个区块的票据集合
    std::unordered_map<u64, ChunkTicketSet> m_chunkTickets;

    /// 玩家位置映射
    std::unordered_map<PlayerId, ChunkPos> m_playerPositions;

    /// 玩家来源和追踪状态
    std::unordered_map<PlayerId, PlayerSourceState> m_playerStates;

    /// 区块 -> 追踪该区块的玩家集合（视距范围内）
    /// 用于区块加载完成时发送给所有追踪该区块的玩家
    std::unordered_map<u64, std::unordered_set<PlayerId>> m_chunkTrackingPlayers;
    mutable std::mutex m_trackingPlayersMutex;

    /// 区块 -> 玩家来源数量
    std::unordered_map<u64, i32> m_playerSourceCounts;

    /// 距离传播图（统一承载 ticket 和玩家来源的聚合结果）
    ChunkDistanceGraph m_distanceGraph;

    /// 当前时间（用于票据过期）
    u64 m_currentTime = 0;

    /// 视距
    i32 m_viewDistance = 10;

    /// 级别变化回调
    LevelChangeCallback m_levelChangeCallback;

    /// 追踪变化回调
    TrackingChangeCallback m_trackingChangeCallback;

    /// 区块缓存中心变化回调
    ChunkCacheCenterCallback m_chunkCacheCenterCallback;

    /// 需要重新计算的区块
    std::unordered_set<u64> m_dirtyChunks;
};

} // namespace mc::world::chunk
