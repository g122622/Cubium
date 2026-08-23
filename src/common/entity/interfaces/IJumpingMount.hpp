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

namespace mc {
namespace entity {

// Forward declarations (Player is in mc namespace, not mc::entity)

/**
 * @brief 可跳跃骑乘接口 - 用于可以通过玩家输入控制跳跃的骑乘实体
 *
 * 实现此接口的实体在玩家骑乘时可以通过玩家的跳跃输入来跳跃。
 * 例如：马、驴、骡、羊驼等。
 */
class IJumpingMount {
public:
    virtual ~IJumpingMount() = default;

    /**
     * @brief 当玩家请求跳跃时调用
     *
     * 此方法由客户端通过发送跳跃包来触发
     */
    virtual void onJump() = 0;

    /**
     * @brief 获取跳跃蓄力值（0 - 100）
     * @return 当前跳跃蓄力值
     *
     * 马的跳跃蓄力由玩家按住跳跃键的时间决定。
     * 注意：此为骑乘蓄力槽位值，与 LivingEntity::getJumpPower()（跳跃垂直初速度）语义不同。
     */
    virtual i32 getJumpCharge() const = 0;

    /**
     * @brief 设置跳跃蓄力值
     * @param power 跳跃蓄力值 (0 - 100)
     */
    virtual void setJumpCharge(i32 power) = 0;

    /**
     * @brief 获取最大跳跃高度
     * @return 最大跳跃高度对应的跳跃高度（方块数）
     */
    virtual f32 getMaxJumpHeight() const = 0;

    /**
     * @brief 检查是否可以跳跃
     * @return 如果可以跳跃返回true
     */
    virtual bool canJump() const = 0;

    /**
     * @brief 开始蓄力跳跃
     * @param jumpPower 初始跳跃力度
     *
     * 当玩家开始按住跳跃键时调用，参数表示初始跳跃力度
     */
    virtual void startJumping(i32 jumpPower) = 0;

    /**
     * @brief 停止跳跃蓄力
     *
     * 当玩家松开跳跃键时调用，执行实际跳跃
     */
    virtual void stopJumping() = 0;
};

} // namespace entity
} // namespace mc
