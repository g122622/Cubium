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
 * The above copyright notice and this permission notice shall included in all
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

#include "../../core/Types.hpp"

namespace mc::time {

/**
 * @brief 时间常量定义
 *
 * Minecraft 1.16.5 时间系统常量：
 * - 一天 = 24000 ticks
 * - 0 = 日出, 6000 = 正午, 12000 = 日落, 18000 = 午夜
 */
namespace TimeConstants {
/// 一天的 tick 数
constexpr i64 TICKS_PER_DAY = 24000;

/// 正午时刻 (太阳最高点)
constexpr i64 NOON = 6000;

/// 日落时刻
constexpr i64 SUNSET = 12000;

/// 午夜时刻
constexpr i64 MIDNIGHT = 18000;

/// 日出时刻
constexpr i64 SUNRISE = 0;

/// 时间同步间隔 (ticks)
constexpr i64 TIME_SYNC_INTERVAL = 20;

/// 默认日光周期更新间隔 (毫秒)
constexpr i64 DEFAULT_MS_PER_TICK = 50;
} // namespace TimeConstants

/**
 * @brief 游戏时间管理类
 *
 * 管理游戏世界的 dayTime 和 gameTime。
 *
 * dayTime: 累积的日光时间（无边界），可用于计算天数、月相等
 *          使用 dayTimeOfDay() 获取一天内的时间 (0-23999)
 * gameTime: 游戏启动以来的总 tick 数，用于统计和月相计算
 *
 * 参考MC 1.16.5: World.tick() 和 DimensionType.calculateCelestialAngle()
 *
 * 【重要】与项目旧实现的区别：
 * - MC 1.16.5 中 dayTime 是无边界计数器，不会自动取模
 * - /time add 100000 后 dayTime 可以是 125000（即 5 天 + 1000）
 * - 只有在计算天体角度、显示时间等场景才使用 dayTime % 24000
 */
class GameTime {
public:
    GameTime() = default;
    ~GameTime() = default;

    // 禁止拷贝
    GameTime(const GameTime&) = delete;
    GameTime& operator=(const GameTime&) = delete;

    // 允许移动
    GameTime(GameTime&&) noexcept = default;
    GameTime& operator=(GameTime&&) noexcept = default;

    // ========== 时间更新 ==========

    /**
     * @brief 更新时间 (每 tick 调用一次)
     *
     * 递增 gameTime 和 dayTime。
     * 如果 daylightCycleEnabled，dayTime 会递增（不自动取模）。
     */
    void tick();

    // ========== 时间设置 ==========

    /**
     * @brief 设置 dayTime
     * @param time 新的 dayTime 值（直接存储，不取模）
     *
     * 用于 /time set 命令。
     * MC 1.16.5 行为：dayTime 是无边界计数器，可存储任意值。
     */
    void setDayTime(i64 time);

    /**
     * @brief 增加 dayTime
     * @param ticks 要增加的 tick 数
     *
     * 用于 /time add 命令。
     */
    void addDayTime(i64 ticks);

    /**
     * @brief 设置 gameTime
     * @param time 新的 gameTime 值
     */
    void setGameTime(i64 time);

    /**
     * @brief 设置日光周期是否启用
     * @param enabled true 启用，false 禁用
     */
    void setDaylightCycleEnabled(bool enabled);

    // ========== 时间查询 ==========

    /**
     * @brief 获取累积的日光时间（可能超过 24000）
     * @return 原始 dayTime 值
     *
     * 注意：此值可能超过 24000，如需一天内的时间请使用 dayTimeOfDay()。
     */
    [[nodiscard]] i64 dayTime() const { return m_dayTime; }

    /**
     * @brief 获取当前一天内的时间 (0-23999)
     * @return dayTime % 24000
     *
     * 用于天体角度计算、时间显示等场景。
     */
    [[nodiscard]] i64 dayTimeOfDay() const;

    /**
     * @brief 获取游戏启动以来的总 tick 数
     * @return gameTime
     */
    [[nodiscard]] i64 gameTime() const { return m_gameTime; }

    /**
     * @brief 检查日光周期是否启用
     * @return true 如果启用
     */
    [[nodiscard]] bool daylightCycleEnabled() const { return m_daylightCycleEnabled; }

    /**
     * @brief 获取天数 (gameTime / 24000)
     * @return 已过去的天数
     */
    [[nodiscard]] i64 dayCount() const;

    /**
     * @brief 判断是否是白天 (dayTimeOfDay 在 0-12000 之间)
     * @return true 如果是白天
     */
    [[nodiscard]] bool isDay() const;

    /**
     * @brief 判断是否是夜晚 (dayTimeOfDay 在 12000-24000 之间)
     * @return true 如果是夜晚
     */
    [[nodiscard]] bool isNight() const;

    /**
     * @brief 获取用于网络同步的 dayTime 值
     * @return 如果日光周期禁用，返回负值；否则返回正值
     *
     * MC 协议: 负数表示日光周期禁用
     * 返回的是 dayTimeOfDay (0-23999) 而非原始 dayTime
     */
    [[nodiscard]] i64 dayTimeForNetwork() const;

private:
    i64 m_dayTime = 0;                  ///< 累积的日光时间（无边界）
    i64 m_gameTime = 0;                 ///< 游戏启动以来的总 tick 数
    bool m_daylightCycleEnabled = true; ///< 日光周期是否启用
};

} // namespace mc::time
