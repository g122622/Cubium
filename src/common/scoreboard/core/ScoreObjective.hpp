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

#include "ScoreCriteriaRenderType.hpp"
#include "common/util/text/ITextComponentFwd.hpp"
#include <memory>
#include <string>

namespace mc::scoreboard {

// 前向声明
class Scoreboard;
class ScoreCriteria;

/**
 * @brief 记分板目标
 *
 * 表示记分板中的一个目标，包含名称、判据、显示名称和渲染类型。
 * 参考 MC 1.16.5: net.minecraft.scoreboard.ScoreObjective
 *
 * 目标名称限制：
 * - 最大长度：16 个字符
 * - 仅允许字母、数字、下划线和连字符
 */
class ScoreObjective {
public:
    /// 目标名称最大长度
    static constexpr size_t MAX_NAME_LENGTH = 16;

    /**
     * @brief 构造函数
     *
     * @param scoreboard 所属记分板
     * @param name 目标名称（最大16字符）
     * @param criteria 判据类型
     * @param displayName 显示名称（可为空）
     * @param renderType 渲染类型
     */
    ScoreObjective(Scoreboard& scoreboard,
        const std::string& name,
        ScoreCriteria& criteria,
        std::unique_ptr<text::ITextComponent> displayName,
        RenderType renderType);

    // 禁止拷贝
    ScoreObjective(const ScoreObjective&) = delete;
    ScoreObjective& operator=(const ScoreObjective&) = delete;

    // 允许移动构造，禁止移动赋值（有引用成员）
    ScoreObjective(ScoreObjective&&) noexcept = default;
    ScoreObjective& operator=(ScoreObjective&&) noexcept = delete;

    // ========== 基本信息 ==========

    /**
     * @brief 获取目标名称
     *
     * @return 目标名称
     */
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }

    /**
     * @brief 获取判据
     *
     * @return 判据引用
     */
    [[nodiscard]] ScoreCriteria& getCriteria() const noexcept { return *m_criteria; }

    /**
     * @brief 获取显示名称
     *
     * 如果没有设置显示名称，返回目标名称。
     *
     * @return 显示名称组件指针
     */
    [[nodiscard]] text::ITextComponent* getDisplayName() const noexcept;

    /**
     * @brief 获取格式化后的显示名称
     *
     * 返回带格式化（名称+悬浮提示）的显示名称。
     *
     * @return 格式化显示名称
     */
    [[nodiscard]] std::unique_ptr<text::ITextComponent> getFormattedDisplayName() const;

    /**
     * @brief 设置显示名称
     *
     * @param displayName 新的显示名称
     */
    void setDisplayName(std::unique_ptr<text::ITextComponent> displayName);

    // ========== 渲染类型 ==========

    /**
     * @brief 获取渲染类型
     *
     * @return 渲染类型
     */
    [[nodiscard]] RenderType getRenderType() const noexcept { return m_renderType; }

    /**
     * @brief 设置渲染类型
     *
     * @param renderType 新的渲染类型
     */
    void setRenderType(RenderType renderType);

    // ========== 所属记分板 ==========

    /**
     * @brief 获取所属记分板
     *
     * @return 记分板引用
     */
    [[nodiscard]] Scoreboard& getScoreboard() const noexcept { return m_scoreboard; }

private:
    Scoreboard& m_scoreboard;
    std::string m_name;
    ScoreCriteria* m_criteria;
    std::unique_ptr<text::ITextComponent> m_displayName;
    RenderType m_renderType;
};

} // namespace mc::scoreboard
