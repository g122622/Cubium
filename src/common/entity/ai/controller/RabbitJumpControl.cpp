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

#include "RabbitJumpControl.hpp"

#include "common/entity/ai/controller/JumpController.hpp"
#include "common/entity/entities/passive/basic/RabbitEntity.hpp"

namespace mc::entity::ai::controller {

RabbitJumpControl::RabbitJumpControl(RabbitEntity* rabbit)
    : JumpController(rabbit)
    , m_rabbit(rabbit)
    , m_canJump(true)
{}

void RabbitJumpControl::tick()
{
    // 对应 MC 1.21.11 Rabbit.RabbitJumpControl.tick()：
    //   if (this.jump) { this.rabbit.startJumping(); this.jump = false; }
    //
    // 仅在 wantJump（基类 m_isJumping）为 true 时触发 startJumping()，然后清除标志。
    // RabbitEntity::startJumping() 内部有幂等保护（m_rabbitJumpDuration != 0 时跳过），
    // 因此即使每 tick 都有 setJumping() 调用也不会重复触发动画。
    // 注意：MC 原版还检查 canJump，但 canJump 仅用于启用/禁用控制器本身，由
    // RabbitEntity::customServerAiStep 通过 disableJumpControl() 在着陆延迟期间抑制。
    // 由于此处仅在 wantJump 为 true 时才触发，且 wantJump 由 AI Goal 通过
    // setJumping() 主动设置，因此 canJump 抑制语义由 RabbitEntity 在调用 setJumping
    // 前判断（与原版在控制器内判断等价）。
    if (m_isJumping && m_rabbit != nullptr) {
        m_rabbit->startJumping();
    }

    // 清除 wantJump 标志（与基类 tick() 一致的"每 tick 重置"语义）
    // 注意：此处不复用 JumpController::tick()，因为基类会调用 m_mob->setJumping(false)
    // 触发 RabbitEntity::setJumping(false) 路径，可能干扰动画状态机。
    // startJumping() 已通过 setJumping(true) 启动动画，动画结束由 aiStep() 控制。
    m_isJumping = false;
}

} // namespace mc::entity::ai::controller
