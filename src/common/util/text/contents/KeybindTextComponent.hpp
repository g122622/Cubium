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
 * @brief 按键绑定组件
 *
 * 对应原版 `KeybindContents`：客户端按当前按键绑定表把键名解析成实际键位。
 * JSON 形态：`{"keybind":"key.jump"}`。
 *
 * 服务端没有按键绑定表，`getUnformattedText()` 原样返回键名（`key.jump`），
 * 便于日志排查；实际显示由客户端完成。
 */
class KeybindTextComponent : public BaseTextComponent {
public:
    /**
     * @brief 构造按键绑定组件
     * @param key 按键绑定翻译键（如 "key.jump"）
     */
    explicit KeybindTextComponent(std::string key)
        : m_key(std::move(key))
    {}

    [[nodiscard]] std::string getUnformattedText() const override
    {
        std::string result = m_key;
        for (const auto& sibling : m_siblings) {
            result += sibling->getUnformattedText();
        }
        return result;
    }

    [[nodiscard]] std::string getFormattedText() const override
    {
        std::string result = getStyleCodes(m_style);
        result += m_key;
        for (const auto& sibling : m_siblings) {
            result += sibling->getFormattedText();
        }
        return result;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> deepCopy() const override
    {
        auto copy = std::make_unique<KeybindTextComponent>(m_key);
        copyBaseTo(*copy);
        return copy;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> shallowCopy() const override
    {
        auto copy = std::make_unique<KeybindTextComponent>(m_key);
        copy->setStyle(m_style);
        return copy;
    }

    [[nodiscard]] nlohmann::json toJson() const override
    {
        nlohmann::json json = m_style.toJson();
        json["keybind"] = m_key;
        appendSiblingsToJson(json);
        return json;
    }

    [[nodiscard]] const std::string& key() const noexcept
    {
        return m_key;
    }

private:
    std::string m_key;
};

} // namespace mc::text
