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

#include "JumpController.hpp"

namespace mc {

class RabbitEntity;

namespace entity::ai::controller {

/**
 * @brief 兔子专属跳跃控制器
 *
 * 对应 MC 1.21.11 Rabbit.RabbitJumpControl 内部类。维护 canJump/wantJump 状态机：
 * - wantJump：AI Goal 或移动控制器调用 setJumping() 设置的跳跃请求（基类 m_isJumping）
 * - canJump：由 RabbitEntity::customServerAiStep 通过 enableJumpControl/disableJumpControl
 *   控制，决定是否允许执行跳跃。着陆后通过 disableJumpControl() 抑制跳跃直到 setLandingDelay() 计时结束
 *
 * tick() 行为：仅在 wantJump 为 true 时调用 rabbit.startJumping() 启动一次跳跃动画，
 * 然后立即清除 wantJump 标志。RabbitEntity::startJumping() 内部有幂等保护，避免动画
 * 进行中重复触发。这与 MC 原版 RabbitJumpControl.tick() 的语义一致。
 *
 * 与通用 JumpController 的关键差异：
 * - 通用控制器总是调用 m_mob->setJumping(m_isJumping)，包括 false（用于重置）
 * - 兔子控制器只在 wantJump 时主动触发 startJumping()，不在 false 时主动调用
 *   setJumping(false)——动画结束由 RabbitEntity::aiStep() 控制
 */
class RabbitJumpControl : public JumpController {
public:
    /**
     * @brief 构造函数
     * @param rabbit 兔子实体
     */
    explicit RabbitJumpControl(RabbitEntity* rabbit);

    void tick() override;

    /**
     * @brief 是否有跳跃请求（对应 MC RabbitJumpControl.wantJump()）
     */
    [[nodiscard]] bool wantJump() const { return isJumping(); }

    /**
     * @brief 是否允许跳跃（对应 MC RabbitJumpControl.canJump()）
     */
    [[nodiscard]] bool canJump() const { return m_canJump; }

    /**
     * @brief 设置允许跳跃标志（对应 MC RabbitJumpControl.setCanJump()）
     * @param canJump 是否允许
     */
    void setCanJump(bool canJump) { m_canJump = canJump; }

private:
    RabbitEntity* m_rabbit;
    bool m_canJump = true;
};

} // namespace entity::ai::controller
} // namespace mc
