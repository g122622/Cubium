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
#include <limits>

namespace mc::entity {

/**
 * @brief 动画状态机
 *
 * 用于实体客户端动画的播放控制。每个动画状态对应一个独立的动画通道
 * （如空闲、滑行、射击、吸气、长跳等），由服务端通过 Pose 切换或
 * 客户端逻辑驱动 start/stop。
 *
 * 设计要点：
 * - 使用一个 startTick 字段记录动画起始时刻
 * - stop() 将 startTick 重置为哨兵值 STOPPED，表示未启动
 * - startIfStopped() 仅在未启动时启动，避免重复触发
 * - 客户端渲染器通过 ifStarted() 回调或 isStarted() 查询状态
 *
 * 时间语义：startTick 通常使用实体的 tickCount（ticksExisted），
 * 渲染器在每帧根据 (当前 tick + partialTick - startTick) 计算动画进度。
 */
class AnimationState {
public:
    /**
     * @brief 动画未启动时的哨兵值
     *
     * 取 i32 最小值，确保任何有效的 tickCount 都不会与之相等。
     */
    static constexpr i32 STOPPED = std::numeric_limits<i32>::min();

    AnimationState() = default;
    ~AnimationState() = default;

    AnimationState(const AnimationState&) = default;
    AnimationState& operator=(const AnimationState&) = default;
    AnimationState(AnimationState&&) noexcept = default;
    AnimationState& operator=(AnimationState&&) noexcept = default;

    /**
     * @brief 强制启动动画（无论是否已在播放）
     * @param tickCount 启动时刻（通常为实体 ticksExisted）
     */
    void start(i32 tickCount) { m_startTick = tickCount; }

    /**
     * @brief 若动画未启动则启动
     * @param tickCount 启动时刻（通常为实体 ticksExisted）
     */
    void startIfStopped(i32 tickCount)
    {
        if (!isStarted()) {
            start(tickCount);
        }
    }

    /**
     * @brief 根据条件启动或停止动画
     * @param condition 为 true 则 startIfStopped，为 false 则 stop
     * @param tickCount 启动时刻
     */
    void animateWhen(bool condition, i32 tickCount)
    {
        if (condition) {
            startIfStopped(tickCount);
        } else {
            stop();
        }
    }

    /**
     * @brief 停止动画
     */
    void stop() { m_startTick = STOPPED; }

    /**
     * @brief 是否已启动
     */
    [[nodiscard]] bool isStarted() const noexcept { return m_startTick != STOPPED; }

    /**
     * @brief 获取动画起始 tick
     *
     * 调用前应先检查 isStarted()，未启动时返回哨兵值。
     */
    [[nodiscard]] i32 startTick() const noexcept { return m_startTick; }

    /**
     * @brief 从另一个动画状态复制启动时刻
     * @param other 源动画状态
     */
    void copyFrom(const AnimationState& other) { m_startTick = other.m_startTick; }

private:
    i32 m_startTick = STOPPED;
};

} // namespace mc::entity
