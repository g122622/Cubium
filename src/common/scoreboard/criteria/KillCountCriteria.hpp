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

#include "../core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreCriteriaRenderType.hpp"
#include <string>

namespace mc::scoreboard {

/**
 * @brief 击杀计数判据基类
 *
 * 当玩家击杀实体时自动增加分数。
 */
class KillCountCriteria : public ScoreCriteria {
public:
    /**
     * @brief 构造函数
     *
     * @param name 判据名称
     * @param playersOnly 是否只统计击杀玩家
     */
    KillCountCriteria(const std::string& name, bool playersOnly);

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
    [[nodiscard]] bool isReadOnly() const noexcept override { return false; }
    [[nodiscard]] RenderType getDefaultRenderType() const noexcept override { return RenderType::Integer; }

    void onPlayerKill(
        const std::string& playerName, const std::string& victimType, bool isPlayer, Scoreboard& scoreboard) override;

protected:
    std::string m_name;
    bool m_playersOnly;
};

/**
 * @brief 玩家击杀计数判据
 *
 * 当玩家击杀其他玩家时自动增加分数。
 */
class PlayerKillCountCriteria : public KillCountCriteria {
public:
    static constexpr const char* NAME = "playerKillCount";

    PlayerKillCountCriteria();
};

/**
 * @brief 总击杀计数判据
 *
 * 当玩家击杀任何实体时自动增加分数。
 */
class TotalKillCountCriteria : public KillCountCriteria {
public:
    static constexpr const char* NAME = "totalKillCount";

    TotalKillCountCriteria();
};

} // namespace mc::scoreboard
