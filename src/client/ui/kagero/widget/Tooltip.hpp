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
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::widget {

/**
 * @brief Tooltip 数据类
 *
 * 表示一个工具提示的内容。
 * 支持多行文本、最大宽度限制和延迟显示。
 *
 * 使用示例：
 * @code
 * auto tooltip = Tooltip::create("保存", "将当前进度保存到存档");
 * button->setTooltip(tooltip);
 * button->setTooltipDelay(500);
 * @endcode
 */
class Tooltip {
public:
    /**
     * @brief Tooltip 默认最大宽度（像素），默认值 170
     */
    static constexpr i32 DEFAULT_MAX_WIDTH = 170;

    /**
     * @brief 默认构造函数（空提示）
     */
    Tooltip() = default;

    /**
     * @brief 构造单行提示
     * @param text 提示文本
     */
    explicit Tooltip(std::string text)
        : m_lines({std::move(text)})
    {}

    /**
     * @brief 构造多行提示
     * @param lines 提示文本行
     */
    explicit Tooltip(std::vector<std::string> lines)
        : m_lines(std::move(lines))
    {}

    /**
     * @brief 工厂方法：创建单行提示
     * @param text 提示文本
     */
    [[nodiscard]] static Tooltip create(std::string text) { return Tooltip(std::move(text)); }

    /**
     * @brief 工厂方法：创建多行提示
     * @param lines 提示文本行
     */
    [[nodiscard]] static Tooltip create(std::vector<std::string> lines) { return Tooltip(std::move(lines)); }

    /**
     * @brief 检查提示是否为空
     */
    [[nodiscard]] bool isEmpty() const noexcept { return m_lines.empty(); }

    /**
     * @brief 获取提示文本行
     */
    [[nodiscard]] const std::vector<std::string>& lines() const noexcept { return m_lines; }

    /**
     * @brief 获取提示行数
     */
    [[nodiscard]] Size lineCount() const noexcept { return m_lines.size(); }

    /**
     * @brief 添加一行文本
     */
    void addLine(std::string line) { m_lines.push_back(std::move(line)); }

    /**
     * @brief 获取最大宽度
     */
    [[nodiscard]] i32 maxWidth() const noexcept { return m_maxWidth; }

    /**
     * @brief 设置最大宽度
     * @param width 最大宽度（像素）
     */
    void setMaxWidth(i32 width) { m_maxWidth = width; }

private:
    std::vector<std::string> m_lines;   ///< 提示文本行
    i32 m_maxWidth = DEFAULT_MAX_WIDTH; ///< 最大宽度限制
};

} // namespace mc::client::ui::kagero::widget
