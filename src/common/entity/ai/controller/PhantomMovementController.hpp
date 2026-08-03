/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

// 前向声明
class PhantomEntity;

namespace entity::ai::controller {

/**
 * @brief 幻翼专用飞行移动控制器
 *
 * 幻翼的移动控制器直接修改速度向量实现飞行，根据 moveTargetPoint
 * （通过 PhantomEntity::orbitOffset() 访问）计算目标方向和旋转。
 *
 * 关键特性：
 * - 水平碰撞时自动180度转向并降低速度
 * - 根据目标方向平滑调整偏航角，接近目标时加速到 1.8，远离时减速到 0.2
 * - 直接设置俯仰角为飞行方向
 * - 使用 20% 惯性混合（与 MC 原版一致）
 */
class PhantomMovementController : public MovementController {
public:
    /**
     * @brief 构造函数
     * @param phantom 幻翼实体指针
     */
    explicit PhantomMovementController(PhantomEntity* phantom);

    /**
     * @brief 刻更新
     *
     * 根据 moveTargetPoint 调整幻翼的飞行方向和速度。
     */
    void tick() override;

private:
    PhantomEntity* m_phantom;
    f32 m_speed = 0.1f;
};

} // namespace entity::ai::controller
} // namespace mc
