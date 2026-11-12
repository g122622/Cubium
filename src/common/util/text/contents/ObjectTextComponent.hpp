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
 * @brief 内联对象组件
 *
 * 对应原版 `ObjectContents`：在文本里内联一张图集精灵或玩家头像。
 * JSON 形态：
 * - 图集精灵：`{"object":{"atlas":"minecraft:blocks","sprite":"stone"}}`（atlas 可省略）
 * - 玩家头像：`{"object":{"player":{...},"hat":true}}`
 *
 * `object` 子对象的结构由客户端解释，服务端只做透传，因此这里保存原始 JSON，
 * 不做字段拆解——玩家头像的 profile 结构较复杂，拆解会引入丢失字段的风险。
 */
class ObjectTextComponent : public BaseTextComponent {
public:
    /**
     * @brief 构造对象组件
     * @param object 原版 `object` 子对象（必须是 JSON 对象）
     */
    explicit ObjectTextComponent(nlohmann::json object)
        : m_object(std::move(object))
    {}

    [[nodiscard]] std::string getUnformattedText() const override
    {
        // 原版 ObjectContents 以替换字符 U+FFFC 占位，宽度由客户端按对象尺寸决定。
        std::string result = "￼";
        for (const auto& sibling : m_siblings) {
            result += sibling->getUnformattedText();
        }
        return result;
    }

    [[nodiscard]] std::string getFormattedText() const override
    {
        std::string result = getStyleCodes(m_style);
        result += "￼";
        for (const auto& sibling : m_siblings) {
            result += sibling->getFormattedText();
        }
        return result;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> deepCopy() const override
    {
        auto copy = std::make_unique<ObjectTextComponent>(m_object);
        copyBaseTo(*copy);
        return copy;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> shallowCopy() const override
    {
        auto copy = std::make_unique<ObjectTextComponent>(m_object);
        copy->setStyle(m_style);
        return copy;
    }

    [[nodiscard]] nlohmann::json toJson() const override
    {
        nlohmann::json json = m_style.toJson();
        json["object"] = m_object;
        appendSiblingsToJson(json);
        return json;
    }

    /**
     * @brief 获取原始 object 子对象
     */
    [[nodiscard]] const nlohmann::json& object() const noexcept
    {
        return m_object;
    }

private:
    nlohmann::json m_object;
};

} // namespace mc::text
