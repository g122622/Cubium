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

#include "common/core/Types.hpp"

namespace mc {

// 前向声明
class MobEntity;

namespace entity::ai::controller {

/**
 * @brief 跳跃控制器
 *
 * 控制实体的跳跃行为。基类提供"每 tick 重置"语义：AI Goal 调用 setJumping()
 * 设置跳跃请求，tick() 将其应用到实体后立即清零，调用方需每 tick 持续请求。
 *
 * 派生类可重写 tick() 实现自定义跳跃状态机（如 RabbitJumpControl 的
 * canJump/wantJump 状态机）。子类构造时持有具体实体指针以便访问子类特有方法。
 */
class JumpController {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此控制器的生物
     */
    explicit JumpController(MobEntity* mob) noexcept;

    /**
     * @brief 虚析构函数，支持多态销毁
     */
    virtual ~JumpController() = default;

    /**
     * @brief 设置跳跃请求
     *
     * 基类实现仅置 m_isJumping = true；派生类可重写以维护自定义状态机。
     */
    virtual void setJumping();

    /**
     * @brief 是否正在请求跳跃（基类语义：当前 tick 内有跳跃请求）
     */
    [[nodiscard]] bool isJumping() const { return m_isJumping; }

    /**
     * @brief 刻更新
     *
     * 每tick调用，将跳跃状态应用到实体。基类实现总是调用 m_mob->setJumping(m_isJumping)
     * 后重置标志；派生类可完全重写此行为。
     */
    virtual void tick();

protected:
    /**
     * @brief 当前 tick 内的跳跃请求标志
     *
     * 派生类（如 RabbitJumpControl）需要直接读写此标志以实现自定义状态机。
     * 基类 tick() 在应用后将其清零；派生类重写 tick() 时也需自行管理清零时机。
     */
    bool m_isJumping = false;

private:
    MobEntity* m_mob;
};

} // namespace entity::ai::controller
} // namespace mc
