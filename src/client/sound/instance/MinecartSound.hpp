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

#include "client/sound/handler/EntitySoundHandler.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"

namespace mc::client::sound {

/**
 * @brief 矿车行驶音效（使用状态快照）
 *
 * 当矿车移动时播放行驶音效，音量根据矿车速度动态变化。
 *
 * 特点:
 * - 循环播放
 * - 音量根据水平速度变化（0 ~ 0.7）
 * - 有 distance 字段用于平滑音量变化
 * - canBeSilent() 返回 true
 */
class MinecartSoundStateful : public TickableSound {
public:
    /**
     * @brief 构造函数
     *
     * @param state 实体状态快照
     * @param handler EntitySoundHandler 指针，用于查询最新状态
     */
    MinecartSoundStateful(const EntitySoundState& state, EntitySoundHandler* handler);

    /**
     * @brief 每帧更新
     *
     * 更新位置和音量。
     */
    void tick() override;

    /**
     * @brief 是否可以静音播放
     */
    [[nodiscard]] bool canBeSilent() const override { return true; }

private:
    EntitySoundHandler* m_handler = nullptr;
    EntityInstanceId m_entityId;
    f32 m_distance = 0.0f; // 音量平滑距离值
};

/**
 * @brief 玩家骑乘矿车时的内部音效（使用状态快照）
 *
 * 当玩家骑乘矿车时播放内部音效，使用无衰减模式。
 *
 * 特点:
 * - 循环播放
 * - 无衰减（玩家内部声音）
 * - 音量根据水平速度变化（0 ~ 0.75）
 * - canBeSilent() 返回 true
 */
class RidingMinecartSoundStateful : public TickableSound {
public:
    /**
     * @brief 构造函数
     *
     * @param playerState 玩家状态快照
     * @param minecartState 矿车状态快照
     * @param handler EntitySoundHandler 指针
     */
    RidingMinecartSoundStateful(
        const EntitySoundState& playerState, const EntitySoundState& minecartState, EntitySoundHandler* handler);

    /**
     * @brief 每帧更新
     *
     * 更新位置和音量，检查玩家是否仍在骑乘。
     */
    void tick() override;

    /**
     * @brief 是否可以静音播放
     */
    [[nodiscard]] bool canBeSilent() const override { return true; }

private:
    EntitySoundHandler* m_handler = nullptr;
    EntityInstanceId m_playerId;
    EntityInstanceId m_minecartId;
};

} // namespace mc::client::sound
