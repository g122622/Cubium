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
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreCriteriaRenderType.hpp"
#include "common/util/text/TextStyle.hpp"
#include <string>

namespace mc::scoreboard {

// 导入 TextFormatting 类型
using text::TextFormatting;

/**
 * @brief 队伍击杀判据
 *
 * 当玩家击杀指定颜色队伍的玩家时自动增加分数。
 * 名称格式：teamkill.{color} 或 killedByTeam.{color}
 */
class TeamKillCriteria : public ScoreCriteria {
public:
    /**
     * @brief 击杀类型
     */
    enum class Type : u8 {
        TeamKill,    // 玩家击杀指定队伍的成员
        KilledByTeam // 玩家被指定队伍的成员击杀
    };

    /**
     * @brief 构造函数
     *
     * @param color 队伍颜色
     * @param type 击杀类型
     */
    TeamKillCriteria(TextFormatting color, Type type);

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
    [[nodiscard]] bool isReadOnly() const noexcept override { return false; }
    [[nodiscard]] RenderType getDefaultRenderType() const noexcept override { return RenderType::Integer; }

    /**
     * @brief 获取队伍颜色
     */
    [[nodiscard]] TextFormatting getColor() const noexcept { return m_color; }

    /**
     * @brief 获取击杀类型
     */
    [[nodiscard]] Type getType() const noexcept { return m_type; }

    /**
     * @brief 检查击杀是否匹配此判据
     *
     * @param killerTeam 击杀者队伍颜色
     * @param victimTeam 受害者队伍颜色
     * @return true 如果匹配
     */
    [[nodiscard]] bool matches(TextFormatting killerTeam, TextFormatting victimTeam) const;

    /**
     * @brief 生成判据名称
     *
     * @param color 队伍颜色
     * @param type 击杀类型
     * @return 判据名称
     */
    static std::string generateName(TextFormatting color, Type type);

    /**
     * @brief 检查颜色是否支持队伍击杀判据
     *
     * @param color 颜色
     * @return true 如果支持
     */
    static bool isSupportedColor(TextFormatting color);

private:
    std::string m_name;
    TextFormatting m_color;
    Type m_type;
};

} // namespace mc::scoreboard
