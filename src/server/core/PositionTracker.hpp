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
#include "common/network/sync/ChunkSync.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <vector>

namespace mc::server::core {

// 前向声明
class PlayerManager;

/**
 * @brief 位置追踪器
 *
 * 负责玩家位置更新、区块订阅、移动验证。
 * 与 PlayerManager 和 ChunkSyncManager 协同工作。
 *
 * 使用示例：
 * @code
 * PositionTracker posTracker(playerManager, viewDistance);
 * posTracker.updatePosition(playerId, x, y, z, yaw, pitch, onGround);
 *
 * // 获取需要加载/卸载的区块
 * std::vector<ChunkPos> toLoad, toUnload;
 * posTracker.calculateChunkUpdates(playerId, toLoad, toUnload);
 * @endcode
 */
class PositionTracker {
public:
    /**
     * @brief 构造位置追踪器
     * @param playerManager 玩家管理器引用
     * @param viewDistance 默认视距
     */
    PositionTracker(PlayerManager& playerManager, i32 viewDistance);

    // ========== 位置更新 ==========

    /**
     * @brief 更新玩家位置
     * @param playerId 玩家ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param yaw 偏航角
     * @param pitch 俯仰角
     * @param onGround 是否在地面
     * @return true 如果更新成功
     */
    bool updatePosition(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, bool onGround);

    /**
     * @brief 更新玩家位置（仅位置）
     * @param playerId 玩家ID
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return true 如果更新成功
     */
    bool updatePosition(PlayerId playerId, f64 x, f64 y, f64 z);

    /**
     * @brief 更新玩家旋转
     * @param playerId 玩家ID
     * @param yaw 偏航角
     * @param pitch 俯仰角
     * @return true 如果更新成功
     */
    bool updateRotation(PlayerId playerId, f32 yaw, f32 pitch);

    // ========== 区块更新 ==========

    /**
     * @brief 计算区块更新
     * @param playerId 玩家ID
     * @param[out] chunksToLoad 需要加载的区块
     * @param[out] chunksToUnload 需要卸载的区块
     */
    void calculateChunkUpdates(
        PlayerId playerId, std::vector<ChunkPos>& chunksToLoad, std::vector<ChunkPos>& chunksToUnload);

    /**
     * @brief 标记区块为已发送
     * @param playerId 玩家ID
     * @param x 区块X坐标
     * @param z 区块Z坐标
     */
    void markChunkSent(PlayerId playerId, ChunkCoord x, ChunkCoord z);

    /**
     * @brief 标记区块为已卸载
     * @param playerId 玩家ID
     * @param x 区块X坐标
     * @param z 区块Z坐标
     */
    void markChunkUnloaded(PlayerId playerId, ChunkCoord x, ChunkCoord z);

    /**
     * @brief 获取区块订阅者
     * @param x 区块X坐标
     * @param z 区块Z坐标
     * @return 需要该区块的玩家ID列表
     */
    [[nodiscard]] std::vector<PlayerId> getChunkSubscribers(ChunkCoord x, ChunkCoord z) const;

    // ========== 位置查询 ==========

    /**
     * @brief 获取玩家位置
     * @param playerId 玩家ID
     * @return 位置向量，如果玩家不存在返回 (0, 64, 0)
     */
    [[nodiscard]] Vector3f getPosition(PlayerId playerId) const;

    /**
     * @brief 获取玩家旋转
     * @param playerId 玩家ID
     * @return 旋转向量 (yaw, pitch)
     */
    [[nodiscard]] Vector2f getRotation(PlayerId playerId) const;

    /**
     * @brief 获取玩家区块坐标
     * @param playerId 玩家ID
     * @return 区块坐标 (x, z)
     */
    [[nodiscard]] ChunkPos getChunkPosition(PlayerId playerId) const;

    /**
     * @brief 检查玩家是否在地面
     * @param playerId 玩家ID
     * @return true 如果玩家在地面
     */
    [[nodiscard]] bool isOnGround(PlayerId playerId) const;

    // ========== 视距 ==========

    /**
     * @brief 设置玩家视距
     * @param playerId 玩家ID
     * @param viewDistance 视距（区块数）
     */
    void setViewDistance(PlayerId playerId, i32 viewDistance);

    /**
     * @brief 获取玩家视距
     * @param playerId 玩家ID
     * @return 视距，如果玩家不存在返回默认值
     */
    [[nodiscard]] i32 getViewDistance(PlayerId playerId) const;

private:
    PlayerManager& m_playerManager;
    i32 m_defaultViewDistance;
};

} // namespace mc::server::core
