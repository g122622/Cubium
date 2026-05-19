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
#include "common/world/time/GameTime.hpp"

namespace mc::server::core {

/**
 * @brief 时间管理器
 *
 * 负责游戏时间、tick 计数、日光周期管理。
 * 线程安全：所有公共方法都是线程安全的。
 *
 * 使用示例：
 * @code
 * TimeManager timeManager;
 * timeManager.tick();  // 每个 tick 调用
 * i64 dayTime = timeManager.dayTime();
 * i64 gameTime = timeManager.gameTime();
 * @endcode
 */
class TimeManager {
public:
    /**
     * @brief 构造时间管理器
     */
    TimeManager() = default;

    /**
     * @brief 构造时间管理器（带初始时间）
     * @param initialGameTime 初始游戏时间
     * @param initialDayTime 初始日光时间
     */
    TimeManager(i64 initialGameTime, i64 initialDayTime = 0);

    // ========== 时间更新 ==========

    /**
     * @brief 更新时间（每 tick 调用）
     *
     * 游戏时间 +1，日光时间 +1（如果启用日光周期）
     */
    void tick();

    // ========== 游戏时间 ==========

    /**
     * @brief 获取游戏时间（总 tick 数）
     */
    [[nodiscard]] i64 gameTime() const;

    /**
     * @brief 获取当前 tick（与 gameTime 相同）
     */
    [[nodiscard]] u64 currentTick() const { return static_cast<u64>(m_gameTime.gameTime()); }

    /**
     * @brief 设置游戏时间
     * @param time 新的游戏时间
     */
    void setGameTime(i64 time);

    // ========== 日光周期 ==========

    /**
     * @brief 获取日光时间（0-23999，对应 0:00-23:59）
     *
     * 0 = 6:00 AM（日出）
     * 6000 = 正午
     * 12000 = 6:00 PM（日落）
     * 18000 = 午夜
     */
    [[nodiscard]] i64 dayTime() const;

    /**
     * @brief 设置日光时间
     * @param time 新的日光时间（会自动取模）
     */
    void setDayTime(i64 time);

    /**
     * @brief 增加日光时间
     * @param ticks 增加的 tick 数
     */
    void addDayTime(i64 ticks);

    /**
     * @brief 检查日光周期是否启用
     */
    [[nodiscard]] bool daylightCycleEnabled() const { return m_gameTime.daylightCycleEnabled(); }

    /**
     * @brief 启用/禁用日光周期
     * @param enabled 是否启用
     */
    void setDaylightCycleEnabled(bool enabled);

    // ========== 天数 ==========

    /**
     * @brief 获取天数
     */
    [[nodiscard]] i64 dayCount() const;

    // ========== GameTime 对象访问 ==========

    /**
     * @brief 获取内部 GameTime 对象
     */
    [[nodiscard]] const time::GameTime& gameTimeObj() const { return m_gameTime; }
    time::GameTime& gameTimeObj() { return m_gameTime; }

private:
    time::GameTime m_gameTime;
};

} // namespace mc::server::core
