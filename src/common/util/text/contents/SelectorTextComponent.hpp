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

#include "common/util/text/ITextComponent.hpp"
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

namespace mc::text {

/**
 * @brief 实体选择器组件
 *
 * 对应原版 `SelectorContents`：显示实体选择器匹配到的实体名列表，
 * 多个实体之间用可选的 `separator` 组件连接。
 * JSON 形态：`{"selector":"@a","separator":{"text":", "}}`。
 *
 * 解析选择器需要命令上下文；服务端只透传，`getUnformattedText()` 无上下文时返回空串。
 */
class SelectorTextComponent : public BaseTextComponent {
public:
    /**
     * @brief 构造选择器组件
     * @param selector 实体选择器文本（如 "@a"、"@p[team=red]"）
     */
    explicit SelectorTextComponent(std::string selector)
        : m_selector(std::move(selector))
    {}

    [[nodiscard]] std::string getUnformattedText() const override
    {
        std::string result;
        for (const auto& sibling : m_siblings) {
            result += sibling->getUnformattedText();
        }
        return result;
    }

    [[nodiscard]] std::string getFormattedText() const override
    {
        std::string result = getStyleCodes(m_style);
        for (const auto& sibling : m_siblings) {
            result += sibling->getFormattedText();
        }
        return result;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> deepCopy() const override
    {
        auto copy = std::make_unique<SelectorTextComponent>(m_selector);
        if (m_separator != nullptr) {
            copy->m_separator = m_separator->deepCopy();
        }
        copyBaseTo(*copy);
        return copy;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> shallowCopy() const override
    {
        auto copy = std::make_unique<SelectorTextComponent>(m_selector);
        if (m_separator != nullptr) {
            copy->m_separator = m_separator->deepCopy();
        }
        copy->setStyle(m_style);
        return copy;
    }

    [[nodiscard]] nlohmann::json toJson() const override
    {
        nlohmann::json json = m_style.toJson();
        json["selector"] = m_selector;
        if (m_separator != nullptr) {
            json["separator"] = m_separator->toJson();
        }
        appendSiblingsToJson(json);
        return json;
    }

    [[nodiscard]] const std::string& selector() const noexcept
    {
        return m_selector;
    }

    [[nodiscard]] const std::unique_ptr<ITextComponent>& separator() const noexcept
    {
        return m_separator;
    }

    /**
     * @brief 设置多个匹配实体之间的分隔组件
     * @param separator 分隔组件（所有权转移）
     */
    void setSeparator(std::unique_ptr<ITextComponent> separator)
    {
        m_separator = std::move(separator);
    }

private:
    std::string m_selector;
    std::unique_ptr<ITextComponent> m_separator;
};

} // namespace mc::text
