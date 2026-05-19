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

#include "../../../core/Types.hpp"

namespace mc {

// 前向声明
class MobEntity;

namespace entity::ai::controller {

/**
 * @brief 跳跃控制器
 *
 * 控制实体的跳跃行为。
 *
 * 参考 MC 1.16.5 JumpController
 */
class JumpController {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此控制器的生物
     */
    explicit JumpController(MobEntity* mob);

    /**
     * @brief 设置跳跃状态
     */
    void setJumping();

    /**
     * @brief 是否正在跳跃
     */
    [[nodiscard]] bool isJumping() const { return m_isJumping; }

    /**
     * @brief 刻更新
     *
     * 每tick调用，将跳跃状态应用到实体。
     */
    void tick();

private:
    MobEntity* m_mob;
    bool m_isJumping = false;
};

} // namespace entity::ai::controller
} // namespace mc
