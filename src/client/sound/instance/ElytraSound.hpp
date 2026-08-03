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

#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"

namespace mc::client {

// 前向声明
class ClientEntity;

} // namespace mc::client

namespace mc::client::sound {

/**
 * @brief 鞘翅飞行声音
 *
 * 当玩家使用鞘翅滑翔时播放的循环声音。
 * 音量根据玩家速度变化。
 */
class ElytraSound : public TickableSound {
public:
    /**
     * @brief 构造鞘翅声音
     *
     * @param player 玩家客户端实体引用
     */
    explicit ElytraSound(const ClientEntity& player);

    ~ElytraSound() override = default;

    // 禁止拷贝
    ElytraSound(const ElytraSound&) = delete;
    ElytraSound& operator=(const ElytraSound&) = delete;

    // 允许移动
    ElytraSound(ElytraSound&&) noexcept = default;
    ElytraSound& operator=(ElytraSound&&) noexcept = default;

    // ========================================================================
    // TickableSound 接口
    // ========================================================================

    /**
     * @brief 每帧更新
     *
     * 根据玩家的鞘翅飞行状态和速度更新音量。
     */
    void tick() override;

private:
    const ClientEntity& m_player;
    i32 m_time = 0; // 时间计数器
};

} // namespace mc::client::sound
