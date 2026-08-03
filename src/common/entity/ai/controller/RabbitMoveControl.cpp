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

#include "RabbitMoveControl.hpp"

#include "RabbitJumpControl.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/basic/RabbitEntity.hpp"

namespace mc::entity::ai::controller {

RabbitMoveControl::RabbitMoveControl(RabbitEntity* rabbit)
    : MovementController(rabbit)
    , m_rabbit(rabbit)
    , m_nextJumpSpeed(0.0)
{}

void RabbitMoveControl::setMoveTo(f64 x, f64 y, f64 z, f64 speed)
{
    // 对应 MC 1.21.11 Rabbit.RabbitMoveControl.setWantedPosition()：
    //   if (this.rabbit.isInWater()) { speed = 1.5; }
    //   super.setWantedPosition(x, y, z, speed);
    //   if (speed > 0.0) { this.nextJumpSpeed = speed; }
    //
    // 在水中时将速度倍率提升至 1.5，使兔子在水中跳跃移动更敏捷。
    // 保留 nextJumpSpeed 的记录，供 tick() 在跳跃过程中应用。
    if (m_rabbit != nullptr && m_rabbit->isInWater()) {
        speed = 1.5;
    }

    MovementController::setMoveTo(x, y, z, speed);

    if (speed > 0.0) {
        m_nextJumpSpeed = speed;
    }
}

void RabbitMoveControl::tick()
{
    if (m_rabbit == nullptr) {
        MovementController::tick();
        return;
    }

    // 对应 MC 1.21.11 Rabbit.RabbitMoveControl.tick()：
    //   if (this.rabbit.onGround() && !this.rabbit.jumping
    //       && !((RabbitJumpControl)this.rabbit.jumpControl).wantJump()) {
    //       this.rabbit.setSpeedModifier(0.0);
    //   } else if (this.hasWanted() || this.operation == MoveControl.Operation.JUMPING) {
    //       this.rabbit.setSpeedModifier(this.nextJumpSpeed);
    //   }
    //   super.tick();
    //
    // setSpeedModifier(0.0) 会调用 navigation.setSpeedModifier(0.0) 并以当前目标位置、
    // 速度 0 调用 setWantedPosition()。此处等效为：将 m_speed 设为 0，
    // 并同步导航器速度，基类 tick() 会根据 m_speed 计算并设置 setAIMoveSpeed(0)。
    //
    // setSpeedModifier(nextJumpSpeed) 同理：更新 m_speed 为 nextJumpSpeed 并同步导航器。
    //
    // hasWanted() 对应 m_action != MoveAction::Wait；JUMPING 对应 MoveAction::Jumping。

    // 获取兔子的跳跃控制器以检查 wantJump()
    // 注意：m_rabbit 的 jumpController() 返回 JumpController*，需向下转型为 RabbitJumpControl
    auto* jumpCtrl = m_rabbit->jumpController();
    bool wantJump = false;
    if (jumpCtrl != nullptr) {
        // 尝试向下转型为 RabbitJumpControl 以访问 wantJump()
        auto* rabbitJumpCtrl = dynamic_cast<RabbitJumpControl*>(jumpCtrl);
        if (rabbitJumpCtrl != nullptr) {
            wantJump = rabbitJumpCtrl->wantJump();
        }
    }

    bool hasWanted = (m_action != MoveAction::Wait);
    bool isJumpingAction = (m_action == MoveAction::Jumping);

    if (m_rabbit->onGround() && !m_rabbit->isJumping() && !wantJump) {
        // 地面静止：速度设为 0，避免兔子在地面滑行
        m_speed = 0.0;
        if (auto* nav = m_rabbit->navigator(); nav != nullptr) {
            nav->setSpeed(0.0);
        }
    } else if (hasWanted || isJumpingAction) {
        // 有移动目标或跳跃中：应用 nextJumpSpeed
        m_speed = m_nextJumpSpeed;
        if (auto* nav = m_rabbit->navigator(); nav != nullptr) {
            nav->setSpeed(m_nextJumpSpeed);
        }
    }

    // 委托基类处理实际的移动、转向和跳跃触发
    MovementController::tick();
}

} // namespace mc::entity::ai::controller
