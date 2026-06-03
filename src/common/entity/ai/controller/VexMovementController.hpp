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

namespace mc {

// Forward declaration
class VexEntity;

namespace entity::ai::controller {

/**
 * @brief 恼鬼专用飞行移动控制器
 *
 * 恼鬼的移动控制器直接修改velocity实现飞行，不使用传统的地面导航。
 * 这使得恼鬼能够穿墙飞行并追踪目标。
 */
class VexMovementController : public MovementController {
public:
    /**
     * @brief 构造函数
     * @param vex 拥有此控制器的恼鬼实体
     */
    explicit VexMovementController(VexEntity* vex);

    /**
     * @brief 刻更新
     *
     * 每tick调用，更新恼鬼的飞行移动。
     * 直接修改velocity向量，实现三维空间飞行。
     */
    void tick() override;

private:
    VexEntity* m_vex;
};

} // namespace entity::ai::controller
} // namespace mc
