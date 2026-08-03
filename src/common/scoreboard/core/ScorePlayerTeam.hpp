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

#include "Team.hpp"
#include "common/core/Types.hpp"
#include "common/scoreboard/core/TeamEnums.hpp"
#include "common/util/text/ITextComponent.hpp"
#include <cstddef>
#include <memory>
#include <set>
#include <string>

namespace mc::scoreboard {

// 前向声明
class Scoreboard;

/**
 * @brief 记分板队伍实现
 *
 * 表示记分板中的一个队伍，管理队伍成员、属性和格式化设置。
 *
 * 队伍名称限制：
 * - 最大长度：16 个字符
 * - 仅允许字母、数字、下划线和连字符
 */
class ScorePlayerTeam : public Team {
public:
    /// 队伍名称最大长度
    static constexpr size_t MAX_NAME_LENGTH = 16;

    /**
     * @brief 构造函数
     *
     * @param scoreboard 所属记分板
     * @param name 队伍名称
     */
    ScorePlayerTeam(Scoreboard& scoreboard, const std::string& name);

    // 禁止拷贝
    ScorePlayerTeam(const ScorePlayerTeam&) = delete;
    ScorePlayerTeam& operator=(const ScorePlayerTeam&) = delete;

    // 允许移动构造，禁止移动赋值（有引用成员）
    ScorePlayerTeam(ScorePlayerTeam&&) noexcept = default;
    ScorePlayerTeam& operator=(ScorePlayerTeam&&) noexcept = delete;

    // ========== Team 接口实现 ==========

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
    [[nodiscard]] const text::ITextComponent* getDisplayName() const noexcept override;
    void setDisplayName(std::unique_ptr<text::ITextComponent> displayName) override;

    [[nodiscard]] const std::set<std::string>& getMembers() const noexcept override { return m_members; }
    bool addMember(const std::string& playerName) override;
    bool removeMember(const std::string& playerName) override;
    [[nodiscard]] bool hasMember(const std::string& playerName) const override;
    void clearMembers() override;

    [[nodiscard]] TextFormatting getColor() const noexcept override { return m_color; }
    void setColor(TextFormatting color) override;

    [[nodiscard]] const text::ITextComponent* getPrefix() const noexcept override;
    void setPrefix(std::unique_ptr<text::ITextComponent> prefix) override;

    [[nodiscard]] const text::ITextComponent* getSuffix() const noexcept override;
    void setSuffix(std::unique_ptr<text::ITextComponent> suffix) override;

    [[nodiscard]] bool getAllowFriendlyFire() const noexcept override { return m_allowFriendlyFire; }
    void setAllowFriendlyFire(bool allow) override;

    [[nodiscard]] bool canSeeFriendlyInvisibles() const noexcept override { return m_seeFriendlyInvisibles; }
    void setSeeFriendlyInvisibles(bool see) override;

    [[nodiscard]] TeamVisibility getNameTagVisibility() const noexcept override { return m_nameTagVisibility; }
    void setNameTagVisibility(TeamVisibility visibility) override;

    [[nodiscard]] TeamVisibility getDeathMessageVisibility() const noexcept override
    {
        return m_deathMessageVisibility;
    }
    void setDeathMessageVisibility(TeamVisibility visibility) override;

    [[nodiscard]] TeamCollisionRule getCollisionRule() const noexcept override { return m_collisionRule; }
    void setCollisionRule(TeamCollisionRule rule) override;

    [[nodiscard]] std::unique_ptr<text::ITextComponent> formatName(const text::ITextComponent& name) const override;

    /**
     * @brief 获取格式化后的显示名称
     *
     * 将队伍显示名称用方括号包裹，设置悬停事件显示队伍内部名称，
     * 并应用队伍颜色样式。
     *
     * @return 格式化显示名称
     */
    [[nodiscard]] std::unique_ptr<text::ITextComponent> getFormattedDisplayName() const override;

    // ========== 额外方法 ==========

    /**
     * @brief 获取所属记分板
     *
     * @return 记分板引用
     */
    [[nodiscard]] Scoreboard& getScoreboard() const noexcept { return m_scoreboard; }

    /**
     * @brief 获取友军标志位
     *
     * 返回用于网络同步的标志位：
     * - bit 0: 允许友军伤害
     * - bit 1: 能看到隐身队友
     *
     * @return 标志位
     */
    [[nodiscard]] u8 getFriendlyFlags() const noexcept;

    /**
     * @brief 设置友军标志位
     *
     * @param flags 标志位
     */
    void setFriendlyFlags(u8 flags);

private:
    Scoreboard& m_scoreboard;
    std::string m_name;
    std::unique_ptr<text::ITextComponent> m_displayName;
    std::unique_ptr<text::ITextComponent> m_prefix;
    std::unique_ptr<text::ITextComponent> m_suffix;
    std::set<std::string> m_members;
    TextFormatting m_color = TextFormatting::White;
    bool m_allowFriendlyFire = true;
    bool m_seeFriendlyInvisibles = true;
    TeamVisibility m_nameTagVisibility = TeamVisibility::Always;
    TeamVisibility m_deathMessageVisibility = TeamVisibility::Always;
    TeamCollisionRule m_collisionRule = TeamCollisionRule::Always;
};

} // namespace mc::scoreboard
