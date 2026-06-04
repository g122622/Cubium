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
#include <string>

namespace mc::scoreboard {

// 前向声明
class Scoreboard;
class ScoreObjective;

/**
 * @brief 分数类
 *
 * 表示一个玩家/实体在某个目标上的分数。
 *
 * 分数限制：
 * - 最小值：-2147483648 (INT32_MIN)
 * - 最大值：2147483647 (INT32_MAX)
 * - 玩家/实体名称最大长度：40 字符
 */
class Score {
public:
    /// 分数最小值
    static constexpr i32 MIN_SCORE = -2147483647 - 1;

    /// 分数最大值
    static constexpr i32 MAX_SCORE = 2147483647;

    /**
     * @brief 构造函数
     *
     * @param scoreboard 所属记分板
     * @param objective 所属目标
     * @param playerName 玩家/实体名称
     */
    Score(Scoreboard& scoreboard, ScoreObjective& objective, const std::string& playerName);

    // 禁止拷贝
    Score(const Score&) = delete;
    Score& operator=(const Score&) = delete;

    // 允许移动构造，禁止移动赋值（有引用成员）
    Score(Score&&) noexcept = default;
    Score& operator=(Score&&) noexcept = delete;

    // ========== 分数操作 ==========

    /**
     * @brief 获取当前分数
     *
     * @return 分数值
     */
    [[nodiscard]] i32 getScorePoints() const noexcept { return m_score; }

    /**
     * @brief 设置分数
     *
     * 分数会被限制在 [MIN_SCORE, MAX_SCORE] 范围内。
     * 如果分数发生变化，会通知记分板。
     *
     * @param points 新分数值
     */
    void setScorePoints(i32 points);

    /**
     * @brief 增加分数
     *
     * @param amount 增加量
     */
    void addScore(i32 amount);

    /**
     * @brief 减少分数
     *
     * @param amount 减少量
     */
    void subtractScore(i32 amount);

    /**
     * @brief 分数 +1
     */
    void incrementScore() { addScore(1); }

    /**
     * @brief 重置分数为 0
     */
    void reset();

    // ========== Trigger 锁定状态 ==========

    /**
     * @brief 判断分数是否被锁定
     *
     * 对于 trigger 类型的判据，分数在玩家触发后会被锁定，
     * 需要管理员重新 enable 才能再次触发。
     *
     * @return true 如果被锁定
     */
    [[nodiscard]] bool isLocked() const noexcept { return m_locked; }

    /**
     * @brief 设置锁定状态
     *
     * @param locked 锁定状态
     */
    void setLocked(bool locked) noexcept { m_locked = locked; }

    // ========== 信息获取 ==========

    /**
     * @brief 获取玩家/实体名称
     *
     * @return 玩家/实体名称
     */
    [[nodiscard]] const std::string& getPlayerName() const noexcept { return m_playerName; }

    /**
     * @brief 获取所属目标
     *
     * @return 目标引用
     */
    [[nodiscard]] ScoreObjective& getObjective() const noexcept { return *m_objective; }

    /**
     * @brief 获取所属记分板
     *
     * @return 记分板引用
     */
    [[nodiscard]] Scoreboard& getScoreboard() const noexcept { return m_scoreboard; }

    // ========== 内部使用 ==========

    /**
     * @brief 标记需要强制更新
     *
     * 用于首次创建分数时强制发送更新包。
     */
    void markForcedUpdate() noexcept { m_forceUpdate = true; }

    /**
     * @brief 检查是否需要强制更新
     */
    [[nodiscard]] bool needsForcedUpdate() const noexcept { return m_forceUpdate; }

    /**
     * @brief 清除强制更新标记
     */
    void clearForcedUpdate() noexcept { m_forceUpdate = false; }

private:
    Scoreboard& m_scoreboard;
    ScoreObjective* m_objective;
    std::string m_playerName;
    i32 m_score = 0;
    bool m_locked = false;
    bool m_forceUpdate = false;
};

} // namespace mc::scoreboard
