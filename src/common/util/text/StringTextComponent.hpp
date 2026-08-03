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

#include "ITextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::text {

/**
 * @brief 纯文本组件
 *
 * 表示一段纯文本字符串。
 * 参考: net.minecraft.util.text.StringTextComponent
 *
 * ## 使用示例
 *
 * ```cpp
 * auto text = std::make_unique<StringTextComponent>("Hello World");
 * text->setStyle(Style().setColor(TextFormatting::Red));
 *
 * // 添加子组件
 * text->append(std::make_unique<StringTextComponent>("!"));
 * ```
 */
class StringTextComponent : public BaseTextComponent {
public:
    /**
     * @brief 默认构造函数
     */
    StringTextComponent() = default;

    /**
     * @brief 构造纯文本组件
     * @param text 文本内容
     */
    explicit StringTextComponent(std::string text)
        : m_text(std::move(text))
    {}

    /**
     * @brief 从 C 风格字符串构造
     * @param text 文本内容
     */
    explicit StringTextComponent(const char* text)
        : m_text(text)
    {}

    // ========== ITextComponent 接口 ==========

    [[nodiscard]] std::string getUnformattedText() const override
    {
        std::string result = m_text;
        for (const auto& sibling : m_siblings) {
            result += sibling->getUnformattedText();
        }
        return result;
    }

    [[nodiscard]] std::string getFormattedText() const override
    {
        std::string result = getStyleCodes(m_style);
        result += m_text;

        for (const auto& sibling : m_siblings) {
            result += sibling->getFormattedText();
        }

        return result;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> deepCopy() const override
    {
        auto copy = std::make_unique<StringTextComponent>(m_text);
        copyBaseTo(*copy);
        return copy;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> shallowCopy() const override
    {
        auto copy = std::make_unique<StringTextComponent>(m_text);
        copy->setStyle(m_style);
        return copy;
    }

    [[nodiscard]] nlohmann::json toJson() const override
    {
        nlohmann::json json = m_style.toJson();
        json["text"] = m_text;

        if (!m_siblings.empty()) {
            nlohmann::json extra = nlohmann::json::array();
            for (const auto& sibling : m_siblings) {
                extra.push_back(sibling->toJson());
            }
            json["extra"] = std::move(extra);
        }

        return json;
    }

    // ========== StringTextComponent 特有方法 ==========

    /**
     * @brief 获取文本内容
     * @return 文本字符串
     */
    [[nodiscard]] const std::string& getText() const noexcept { return m_text; }

    /**
     * @brief 设置文本内容
     * @param text 新文本
     */
    void setText(std::string text) { m_text = std::move(text); }

private:
    std::string m_text;
};

/**
 * @brief 从 JSON 创建 StringTextComponent
 *
 * 内部函数，供 ITextComponent::fromJson 使用。
 *
 * @param json JSON 对象
 * @return 文本组件
 */
inline std::unique_ptr<StringTextComponent> createStringFromJson(const nlohmann::json& json)
{
    std::string text;
    if (json.is_string()) {
        text = json.get<std::string>();
    } else if (json.is_object() && json.contains("text")) {
        text = json["text"].get<std::string>();
    } else {
        text = "";
    }

    auto component = std::make_unique<StringTextComponent>(std::move(text));

    if (json.is_object()) {
        component->setStyle(Style::fromJson(json));

        if (json.contains("extra") && json["extra"].is_array()) {
            for (const auto& extra : json["extra"]) {
                component->append(ITextComponent::fromJson(extra));
            }
        }
    }

    return component;
}

} // namespace mc::text
