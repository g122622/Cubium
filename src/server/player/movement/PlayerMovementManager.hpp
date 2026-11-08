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
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <vector>

namespace mc::server {

// 前向声明
class PlayerManager;

/**
 * @brief 玩家移动状态服务
 *
 * 更新位置、朝向与落地状态，并把区块中心变化交给 ChunkSyncManager。
 * 区块订阅、视距和发送标记由同步组件持有。
 */
class PlayerMovementManager {
public:
    /**
     * @brief 构造移动状态服务
     * @param playerManager 玩家管理器引用
     */
    PlayerMovementManager(PlayerManager& playerManager);

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

private:
    PlayerManager& m_playerManager;
};

} // namespace mc::server
