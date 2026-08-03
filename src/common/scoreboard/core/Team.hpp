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

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include "scoreboard/core/TeamEnums.hpp"

namespace mc::scoreboard {

// 导入 TextFormatting 类型
using text::TextFormatting;

/**
 * @brief 队伍抽象类
 *
 * 定义队伍的基本接口。
 */
class Team {
public:
    virtual ~Team() = default;

    // ========== 基本信息 ==========

    /**
     * @brief 获取队伍名称
     *
     * @return 队伍名称
     */
    [[nodiscard]] virtual const std::string& getName() const noexcept = 0;

    /**
     * @brief 获取显示名称
     *
     * @return 显示名称组件指针
     */
    [[nodiscard]] virtual const text::ITextComponent* getDisplayName() const noexcept = 0;

    /**
     * @brief 设置显示名称
     *
     * @param displayName 新的显示名称
     */
    virtual void setDisplayName(std::unique_ptr<text::ITextComponent> displayName) = 0;

    // ========== 成员管理 ==========

    /**
     * @brief 获取所有成员
     *
     * @return 成员名称集合
     */
    [[nodiscard]] virtual const std::set<std::string>& getMembers() const noexcept = 0;

    /**
     * @brief 添加成员
     *
     * @param playerName 玩家名称
     * @return true 如果成功添加（玩家不在队伍中）
     */
    virtual bool addMember(const std::string& playerName) = 0;

    /**
     * @brief 移除成员
     *
     * @param playerName 玩家名称
     * @return true 如果成功移除
     */
    virtual bool removeMember(const std::string& playerName) = 0;

    /**
     * @brief 检查是否包含成员
     *
     * @param playerName 玩家名称
     * @return true 如果包含
     */
    [[nodiscard]] virtual bool hasMember(const std::string& playerName) const = 0;

    /**
     * @brief 清空所有成员
     */
    virtual void clearMembers() = 0;

    // ========== 队伍属性 ==========

    /**
     * @brief 获取队伍颜色
     *
     * @return 颜色
     */
    [[nodiscard]] virtual TextFormatting getColor() const noexcept = 0;

    /**
     * @brief 设置队伍颜色
     *
     * @param color 新颜色
     */
    virtual void setColor(TextFormatting color) = 0;

    /**
     * @brief 获取成员名称前缀
     *
     * @return 前缀组件指针
     */
    [[nodiscard]] virtual const text::ITextComponent* getPrefix() const noexcept = 0;

    /**
     * @brief 设置成员名称前缀
     *
     * @param prefix 新前缀
     */
    virtual void setPrefix(std::unique_ptr<text::ITextComponent> prefix) = 0;

    /**
     * @brief 获取成员名称后缀
     *
     * @return 后缀组件指针
     */
    [[nodiscard]] virtual const text::ITextComponent* getSuffix() const noexcept = 0;

    /**
     * @brief 设置成员名称后缀
     *
     * @param suffix 新后缀
     */
    virtual void setSuffix(std::unique_ptr<text::ITextComponent> suffix) = 0;

    // ========== 友军设置 ==========

    /**
     * @brief 是否允许友军伤害
     *
     * @return true 如果允许
     */
    [[nodiscard]] virtual bool getAllowFriendlyFire() const noexcept = 0;

    /**
     * @brief 设置是否允许友军伤害
     *
     * @param allow 是否允许
     */
    virtual void setAllowFriendlyFire(bool allow) = 0;

    /**
     * @brief 是否能看到隐身的队友
     *
     * @return true 如果能看到
     */
    [[nodiscard]] virtual bool canSeeFriendlyInvisibles() const noexcept = 0;

    /**
     * @brief 设置是否能看到隐身的队友
     *
     * @param see 是否能看到
     */
    virtual void setSeeFriendlyInvisibles(bool see) = 0;

    // ========== 可见性和碰撞 ==========

    /**
     * @brief 获取名称标签可见性
     *
     * @return 可见性设置
     */
    [[nodiscard]] virtual TeamVisibility getNameTagVisibility() const noexcept = 0;

    /**
     * @brief 设置名称标签可见性
     *
     * @param visibility 新的可见性设置
     */
    virtual void setNameTagVisibility(TeamVisibility visibility) = 0;

    /**
     * @brief 获取死亡消息可见性
     *
     * @return 可见性设置
     */
    [[nodiscard]] virtual TeamVisibility getDeathMessageVisibility() const noexcept = 0;

    /**
     * @brief 设置死亡消息可见性
     *
     * @param visibility 新的可见性设置
     */
    virtual void setDeathMessageVisibility(TeamVisibility visibility) = 0;

    /**
     * @brief 获取碰撞规则
     *
     * @return 碰撞规则
     */
    [[nodiscard]] virtual TeamCollisionRule getCollisionRule() const noexcept = 0;

    /**
     * @brief 设置碰撞规则
     *
     * @param rule 新的碰撞规则
     */
    virtual void setCollisionRule(TeamCollisionRule rule) = 0;

    // ========== 格式化 ==========

    /**
     * @brief 获取格式化后的显示名称
     *
     * 返回带方括号包裹和悬停提示的显示名称（使用翻译键 "chat.square_brackets"），
     * 并应用队伍颜色。
     *
     * @return 格式化显示名称
     */
    [[nodiscard]] virtual std::unique_ptr<text::ITextComponent> getFormattedDisplayName() const = 0;

    /**
     * @brief 格式化成员名称
     *
     * 应用队伍颜色、前缀和后缀格式化玩家名称。
     *
     * @param name 玩家名称组件
     * @return 格式化后的名称组件
     */
    [[nodiscard]] virtual std::unique_ptr<text::ITextComponent> formatName(const text::ITextComponent& name) const = 0;
};

} // namespace mc::scoreboard
