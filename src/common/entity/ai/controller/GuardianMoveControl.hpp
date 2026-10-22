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

#include "common/entity/ai/controller/MovementController.hpp"

namespace mc {

class GuardianEntity;

namespace entity::ai::controller {

/**
 * @brief 守卫者专用移动控制器（对齐 vanilla Guardian.GuardianMoveControl）
 *
 * 与通用飞行控制器的差异：
 * - 只在"有 MOVE_TO 目标且导航未完成"时接管；其余情况把 AI 移动速度清零并调用
 *   GuardianEntity::setMoving(false)（对齐 vanilla else 分支）；
 * - 接管时按目标方向平滑转向（yaw 90 度/tick），AI 移动速度以 0.125 插值到
 *   speedModifier * MOVEMENT_SPEED；
 * - 在速度上叠加正弦摆动（sin((tickCount+id)*0.5)*0.05 水平、sin(...*0.75)*0.05 垂直，
 *   垂直额外叠加 f2*d2*0.1 的前进分量），产生守卫者"游动"手感；
 * - 目标方向前方 2 格作为注视点，并用 0.125 插值过渡（10 度/tick 偏航、40 度/tick 俯仰）；
 * - 接管与否同步到 GuardianEntity::setMoving，供 DATA_ID_MOVING 下发客户端与荆棘反伤门控。
 */
class GuardianMoveControl : public MovementController {
public:
    explicit GuardianMoveControl(GuardianEntity* guardian);

    void tick() override;

private:
    GuardianEntity* m_guardian;
};

} // namespace entity::ai::controller
} // namespace mc