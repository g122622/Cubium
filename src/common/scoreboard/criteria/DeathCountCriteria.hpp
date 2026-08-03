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
 * @brief 死亡计数判据
 *
 * 当玩家死亡时自动增加分数。
 */
class DeathCountCriteria : public ScoreCriteria {
public:
    /// 判据名称
    static constexpr const char* NAME = "deathCount";

    /**
     * @brief 构造函数
     */
    DeathCountCriteria();

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
    [[nodiscard]] bool isReadOnly() const noexcept override { return false; }
    [[nodiscard]] RenderType getDefaultRenderType() const noexcept override { return RenderType::Integer; }

    void onPlayerDeath(const std::string& playerName, Scoreboard& scoreboard) override;

private:
    std::string m_name;
};

} // namespace mc::scoreboard
