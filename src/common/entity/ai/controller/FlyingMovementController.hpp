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
namespace entity::ai::controller {

/**
 * @brief 通用飞行移动控制器
 *
 * 通用飞行移动控制器。
 * 适用于飞行生物（如凋灵、鹦鹉等），允许在三维空间中飞行移动。
 *
 * 与 GhastMovementController 不同，本控制器：
 * - 通过 setMoveTo() 接收目标坐标后，设置实体的朝向和移动输入
 * - 支持俯仰角（pitch）旋转，每tick最大旋转角度可配置（maxTurn）
 * - 空闲时可选保持无重力悬停或恢复重力（hoversInPlace）
 * - 使用 FLYING_SPEED 属性（飞行时）或 MOVEMENT_SPEED 属性（地面时）作为速度
 *
 * @param maxTurn 俯仰角每tick最大旋转度数（凋灵=10，鹦鹉=360）
 * @param hoversInPlace 空闲时是否保持无重力悬停（凋灵=false，鹦鹉=true）
 */
class FlyingMovementController : public MovementController {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此控制器的生物
     * @param maxTurn 俯仰角每tick最大旋转度数
     * @param hoversInPlace 空闲时是否保持无重力悬停
     */
    explicit FlyingMovementController(MobEntity* mob, i32 maxTurn = 90, bool hoversInPlace = false);

    /**
     * @brief 刻更新
     *
     * 根据移动目标调整实体的朝向和速度。
     * 有移动目标时：
     *   - 设置 noGravity=true
     *   - 计算目标偏航角（yaw），以90度/tick旋转
     *   - 计算目标俯仰角（pitch），以maxTurn度/tick旋转
     *   - 根据是否在地面选择 MOVEMENT_SPEED 或 FLYING_SPEED
     *   - 设置前进移动输入（moveForward）
     *   - 根据俯仰方向设置上下移动
     * 无移动目标时：
     *   - 根据 hoversInPlace 决定是否恢复重力
     *   - 清零移动输入
     */
    void tick() override;

private:
    i32 m_maxTurn;        // 俯仰角每tick最大旋转度数
    bool m_hoversInPlace; // 空闲时是否保持无重力悬停
};

} // namespace entity::ai::controller
} // namespace mc
