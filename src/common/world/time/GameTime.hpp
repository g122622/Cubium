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
 * dayTime: 当前一天内的时间 (0-23999)，控制太阳/月亮位置
 * gameTime: 游戏启动以来的总 tick 数，用于统计和月相计算
 *
 * 参考MC 1.16.5: World.tick() 和 DimensionType.calculateCelestialAngle()
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
     * 如果 daylightCycleEnabled，dayTime 会自动递增并循环。
     */
    void tick();

    // ========== 时间设置 ==========

    /**
     * @brief 设置 dayTime
     * @param time 新的 dayTime 值 (会自动取模)
     *
     * 用于 /time set 命令
     */
    void setDayTime(i64 time);

    /**
     * @brief 增加 dayTime
     * @param ticks 要增加的 tick 数
     *
     * 用于 /time add 命令
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
     * @brief 获取当前一天内的时间 (0-23999)
     * @return dayTime
     */
    [[nodiscard]] i64 dayTime() const { return m_dayTime; }

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
    [[nodiscard]] i64 dayCount() const { return m_gameTime / TimeConstants::TICKS_PER_DAY; }

    /**
     * @brief 判断是否是白天 (dayTime 在 0-12000 之间)
     * @return true 如果是白天
     */
    [[nodiscard]] bool isDay() const;

    /**
     * @brief 判断是否是夜晚 (dayTime 在 12000-24000 之间)
     * @return true 如果是夜晚
     */
    [[nodiscard]] bool isNight() const;

    /**
     * @brief 获取用于网络同步的 dayTime 值
     * @return 如果日光周期禁用，返回负值；否则返回正值
     *
     * MC 协议: 负数表示日光周期禁用
     */
    [[nodiscard]] i64 dayTimeForNetwork() const;

private:
    i64 m_dayTime = 0;                  ///< 一天内的时间 (0-23999)
    i64 m_gameTime = 0;                 ///< 游戏启动以来的总 tick 数
    bool m_daylightCycleEnabled = true; ///< 日光周期是否启用
};

} // namespace mc::time
