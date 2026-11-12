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
 * @brief 记分板分数组件
 *
 * 对应原版 `ScoreContents`：显示实体在某个计分项上的分数。
 * JSON 形态：`{"score":{"name":"Steve","objective":"obj"}}`。
 *
 * 解析分数需要记分板上下文（原版由 `ScoreContents.resolve` 完成）；服务端只负责
 * 透传组件到客户端，`getUnformattedText()` 在无上下文时返回空串。
 */
class ScoreTextComponent : public BaseTextComponent {
public:
    /**
     * @brief 构造分数组件
     * @param name 计分对象名（玩家名、实体选择器或 "*"）
     * @param objective 计分项名称
     */
    ScoreTextComponent(std::string name, std::string objective)
        : m_name(std::move(name))
        , m_objective(std::move(objective))
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
        auto copy = std::make_unique<ScoreTextComponent>(m_name, m_objective);
        copyBaseTo(*copy);
        return copy;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> shallowCopy() const override
    {
        auto copy = std::make_unique<ScoreTextComponent>(m_name, m_objective);
        copy->setStyle(m_style);
        return copy;
    }

    [[nodiscard]] nlohmann::json toJson() const override
    {
        nlohmann::json json = m_style.toJson();
        json["score"] = nlohmann::json::object({{"name", m_name}, {"objective", m_objective}});
        appendSiblingsToJson(json);
        return json;
    }

    [[nodiscard]] const std::string& name() const noexcept
    {
        return m_name;
    }

    [[nodiscard]] const std::string& objective() const noexcept
    {
        return m_objective;
    }

private:
    std::string m_name;
    std::string m_objective;
};

} // namespace mc::text
