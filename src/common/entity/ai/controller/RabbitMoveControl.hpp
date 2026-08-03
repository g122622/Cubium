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

#include "MovementController.hpp"
#include "common/core/Types.hpp"

namespace mc {

class RabbitEntity;

namespace entity::ai::controller {

/**
 * @brief 兔子专属移动控制器
 *
 * 对应 MC 1.21.11 Rabbit.RabbitMoveControl 内部类。控制兔子跳跃移动的速度：
 * - 在地面且未跳跃且未请求跳跃时，将移动速度设为 0（避免地面滑动）
 * - 在有移动目标或处于 Jumping 状态时，应用 nextJumpSpeed 作为移动速度
 *
 * 与通用 MovementController 的关键差异：
 * - 兔子在地面静止时强制速度为 0，避免兔子在地面"滑行"
 * - setMoveTo 时记录 nextJumpSpeed，供跳跃过程中使用
 * - 在水中时将速度倍率提升至 1.5（对应 MC 原版 setWantedPosition 中的水中逻辑）
 *
 * 调用顺序：tick() 先更新速度倍率，再委托基类 MovementController::tick() 执行
 * 实际的移动、转向和跳跃触发。
 */
class RabbitMoveControl : public MovementController {
public:
    /**
     * @brief 构造函数
     * @param rabbit 兔子实体
     */
    explicit RabbitMoveControl(RabbitEntity* rabbit);

    void tick() override;

    /**
     * @brief 设置移动目标
     *
     * 重写基类 setMoveTo：在水中时将速度倍率提升至 1.5，并记录 nextJumpSpeed。
     *
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param speed 移动速度倍率
     */
    void setMoveTo(f64 x, f64 y, f64 z, f64 speed) override;

    /**
     * @brief 获取下一次跳跃的速度倍率
     */
    [[nodiscard]] f64 nextJumpSpeed() const { return m_nextJumpSpeed; }

private:
    RabbitEntity* m_rabbit;
    f64 m_nextJumpSpeed = 0.0;
};

} // namespace entity::ai::controller
} // namespace mc
