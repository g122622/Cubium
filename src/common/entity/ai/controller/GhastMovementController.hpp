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
#include "common/util/math/Vector3.hpp"

namespace mc {

// Forward declaration
class GhastEntity;

namespace entity::ai::controller {

/**
 * @brief 恶魂专用飞行移动控制器
 *
 * 恶魂的移动控制器直接修改velocity实现飞行，不使用传统的地面导航。
 * 会检查前方碰撞以确保飞行路径安全。
 */
class GhastMovementController : public MovementController {
public:
    /**
     * @brief 构造函数
     * @param ghast 拥有此控制器的恶魂实体
     */
    explicit GhastMovementController(GhastEntity* ghast);

    /**
     * @brief 刻更新
     *
     * 每tick调用，更新恶魂的飞行移动。
     * 直接修改velocity向量，实现三维空间飞行。
     */
    void tick() override;

private:
    /**
     * @brief 检查飞行路径是否安全
     * @param direction 方向向量（已归一化）
     * @param distance 飞行距离
     * @return 路径安全返回true
     */
    bool _isPathSafe(const math::Vector3f& direction, i32 distance) const;

    GhastEntity* m_ghast;
    i32 m_courseChangeCooldown = 0;
};

} // namespace entity::ai::controller
} // namespace mc
