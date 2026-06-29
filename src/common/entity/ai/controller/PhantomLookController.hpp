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
 * copies of substantial portions of the Software.
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

#include "LookController.hpp"

namespace mc {
class MobEntity;

namespace entity::ai::controller {

/**
 * @brief 幻翼专用视线控制器（空操作）
 *
 * 幻翼的朝向完全由 PhantomMovementController 控制，
 * 不使用标准的看向行为，因此视线控制器为空操作。
 */
class PhantomLookController : public LookController {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此控制器的生物
     */
    explicit PhantomLookController(MobEntity* mob)
        : LookController(mob)
    {}

    /**
     * @brief 空操作，幻翼不使用标准看向行为
     */
    void tick() override
    {
        // 幻翼的朝向完全由 PhantomMovementController 控制
    }
};

} // namespace entity::ai::controller
} // namespace mc
