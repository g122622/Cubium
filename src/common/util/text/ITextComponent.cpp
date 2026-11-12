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
#include "common/util/json/JsonRead.hpp"
#include "common/util/text/TextStyle.hpp"
#include "contents/KeybindTextComponent.hpp"
#include "contents/NbtTextComponent.hpp"
#include "contents/ObjectTextComponent.hpp"
#include "contents/ScoreTextComponent.hpp"
#include "contents/SelectorTextComponent.hpp"
#include "util/assert/AssertAll.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc::text {

namespace {

/**
 * @brief 读取可选的字符串字段
 *
 * 语言/数据包文本组件是不可信输入：字段缺失或类型不符时返回空串，不使用会抛出异常的
 * `json.get<T>()`。
 */
std::string _optionalString(const nlohmann::json& json, const char* field)
{
    const auto value = util::json::getString(json, field);
    return value.success() ? value.value() : std::string{};
}

} // namespace

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
        const auto text = util::json::getString(json);
        return std::make_unique<StringTextComponent>(text.success() ? text.value() : std::string{});
    }

    // 数组：首元素为主组件，其余为 siblings
    if (json.is_array()) {
        return fromJsonArray(json);
    }

    if (!json.is_object()) {
        return std::make_unique<StringTextComponent>("");
    }

    // 按 contents 类型分派（与原版 ComponentSerialization 的 type 匹配顺序一致）
    std::unique_ptr<ITextComponent> component;

    if (const auto* score = util::json::findField(json, "score"); score != nullptr && score->is_object()) {
        component =
            std::make_unique<ScoreTextComponent>(_optionalString(*score, "name"), _optionalString(*score, "objective"));
    } else if (util::json::findField(json, "selector") != nullptr) {
        auto selector = std::make_unique<SelectorTextComponent>(_optionalString(json, "selector"));
        if (const auto* separator = util::json::findField(json, "separator"); separator != nullptr) {
            selector->setSeparator(fromJson(*separator));
        }
        component = std::move(selector);
    } else if (util::json::findField(json, "keybind") != nullptr) {
        component = std::make_unique<KeybindTextComponent>(_optionalString(json, "keybind"));
    } else if (util::json::findField(json, "nbt") != nullptr) {
        NbtDataSourceKind kind = NbtDataSourceKind::Storage;
        std::string dataSourceValue;
        if (util::json::findField(json, "block") != nullptr) {
            kind = NbtDataSourceKind::Block;
            dataSourceValue = _optionalString(json, "block");
        } else if (util::json::findField(json, "entity") != nullptr) {
            kind = NbtDataSourceKind::Entity;
            dataSourceValue = _optionalString(json, "entity");
        } else if (util::json::findField(json, "storage") != nullptr) {
            kind = NbtDataSourceKind::Storage;
            dataSourceValue = _optionalString(json, "storage");
        } else {
            spdlog::warn("Component NBT contents is missing a data source (block/entity/storage)");
        }

        const auto interpret = util::json::getBool(json, "interpret");
        auto nbt = std::make_unique<NbtTextComponent>(
            _optionalString(json, "nbt"), interpret.success() && interpret.value(), kind, std::move(dataSourceValue));
        if (const auto* separator = util::json::findField(json, "separator"); separator != nullptr) {
            nbt->setSeparator(fromJson(*separator));
        }
        component = std::move(nbt);
    } else if (const auto* object = util::json::findField(json, "object"); object != nullptr && object->is_object()) {
        component = std::make_unique<ObjectTextComponent>(*object);
    } else if (util::json::findField(json, "translate") != nullptr) {
        auto translation = std::make_unique<TranslationTextComponent>(_optionalString(json, "translate"));
        if (const auto* with = util::json::findField(json, "with"); with != nullptr && with->is_array()) {
            for (const auto& param : *with) {
                translation->addParam(fromJson(param));
            }
        }
        component = std::move(translation);
    } else {
        // 未知对象：按纯文本组件降级（"text" 缺失时为空串）
        component = std::make_unique<StringTextComponent>(_optionalString(json, "text"));
    }

    component->setStyle(Style::fromJson(json));

    if (const auto* extra = util::json::findField(json, "extra"); extra != nullptr && extra->is_array()) {
        for (const auto& sibling : *extra) {
            component->append(fromJson(sibling));
        }
    }

    return component;
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
