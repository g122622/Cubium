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

#include "ITextComponent.hpp"
#include "StringTextComponent.hpp"
#include "TranslationTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include "util/assert/AssertAll.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc::text {

void ITextComponent::appendText(const std::string& text)
{
    append(std::make_unique<StringTextComponent>(text));
}

std::unique_ptr<ITextComponent> ITextComponent::fromJson(const nlohmann::json& json)
{
    // 空值或无效 JSON
    if (json.is_null()) {
        return std::make_unique<StringTextComponent>("");
    }

    // 纯字符串 -> StringTextComponent
    if (json.is_string()) {
        return std::make_unique<StringTextComponent>(json.get<std::string>());
    }

    // 必须是对象
    if (!json.is_object()) {
        return std::make_unique<StringTextComponent>("");
    }

    // 根据 JSON 内容判断组件类型

    // 翻译组件
    if (json.contains("translate")) {
        std::string key = json["translate"].get<std::string>();
        auto component = std::make_unique<TranslationTextComponent>(std::move(key));

        // 解析参数
        if (json.contains("with") && json["with"].is_array()) {
            for (const auto& param : json["with"]) {
                component->addParam(fromJson(param));
            }
        }

        // 解析样式
        component->setStyle(Style::fromJson(json));

        // 解析子组件
        if (json.contains("extra") && json["extra"].is_array()) {
            for (const auto& extra : json["extra"]) {
                component->append(fromJson(extra));
            }
        }

        return component;
    }

    // 默认：字符串组件
    return createStringFromJson(json);
}

std::unique_ptr<ITextComponent> ITextComponent::fromJsonArray(const nlohmann::json& jsonArray)
{
    if (!jsonArray.is_array() || jsonArray.empty()) {
        return std::make_unique<StringTextComponent>("");
    }

    // 第一个元素作为主组件
    auto mainComponent = fromJson(jsonArray[0]);

    // 其余元素作为子组件
    for (size_t i = 1; i < jsonArray.size(); ++i) {
        mainComponent->append(fromJson(jsonArray[i]));
    }

    return mainComponent;
}

} // namespace mc::text
