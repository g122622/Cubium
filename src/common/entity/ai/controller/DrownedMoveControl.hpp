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

class DrownedEntity;

namespace entity::ai::controller {

/**
 * @brief 溺尸两栖移动控制器
 *
 * 根据溺尸是否在水中且想游泳，切换两种移动模式：
 * - 水中模式：直接修改 velocity 实现三维游泳移动
 * - 陆地模式：委托给基类 MovementController 实现地面行走
 *
 * MC 原版中对应 Drowned.DrownedMoveControl 内部类。
 */
class DrownedMoveControl : public MovementController {
public:
    /**
     * @brief 构造函数
     * @param drowned 溺尸实体
     */
    explicit DrownedMoveControl(DrownedEntity* drowned);

    void tick() override;

private:
    DrownedEntity* m_drowned;
};

} // namespace entity::ai::controller
} // namespace mc
