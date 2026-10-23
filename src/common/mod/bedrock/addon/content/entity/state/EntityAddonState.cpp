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

#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/serialization/components/ComponentSerializerRegistry.hpp"
#include "common/mod/bedrock/addon/content/base/properties/AddonPropertyValue.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace mc::mod::bedrock::addon {
namespace {

constexpr const char* _STATE_TAG = "CubiumAddonState";

bool _validEnumName(const std::string& name)
{
    const auto alphabetic = [](char character) {
        return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
    };

    if (name.empty() || name.size() > 32 || !alphabetic(name.front())) {
        return false;
    }

    return std::all_of(name.begin(), name.end(), [&](char character) {
        return alphabetic(character) || (character >= '0' && character <= '9') || character == '_';
    });
}

void _saveState(const mc::Entity& entity, mc::nbt::tags::compound_tag& tag)
{
    const auto* state = entity.tryGetComponent<EntityAddonState>();
    if (state == nullptr) {
        return;
    }

    tag.value.insert_or_assign(_STATE_TAG, mc::nbt::jsonToNbt(state->save()));
}

Result<void> _loadState(mc::Entity& entity, const mc::nbt::tags::compound_tag& tag)
{
    const auto found = tag.value.find(_STATE_TAG);
    if (found == tag.value.end()) {
        return Result<void>::ok();
    }
    if (found->second == nullptr || found->second->id() != mc::nbt::TagId::Compound) {
        return Error(ErrorCode::InvalidData, "Invalid entity addon state tag");
    }

    return entity.getOrAddComponent<EntityAddonState>().load(mc::nbt::nbtToJson(*found->second));
}

} // namespace

Result<void> EntityAddonState::_validateProperty(
    const std::string& name, const nlohmann::json& definition, const nlohmann::json& value)
{
    if (!definition.is_object() || !definition.contains("type") || !definition["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "Entity property has no valid type: " + name);
    }

    const auto kind = definition["type"].get<std::string>();
    if (kind == "bool") {
        if (value.is_boolean()) {
            return Result<void>::ok();
        }
    } else if (kind == "enum") {
        const auto choices = definition.find("values");
        if (choices != definition.end() && choices->is_array() && value.is_string()) {
            for (const auto& choice : *choices) {
                if (choice == value) {
                    return Result<void>::ok();
                }
            }
        }
    } else if ((kind == "int" || kind == "float") && value.is_number()) {
        const auto number = value.get<double>();
        const bool integer = kind == "int";
        if (!std::isfinite(number) || (integer && std::floor(number) != number)) {
            return Error(ErrorCode::InvalidArgument, "Entity property requires a finite compatible number: " + name);
        }

        const double typeMinimum = integer ? static_cast<double>(std::numeric_limits<i32>::min())
                                           : -static_cast<double>(std::numeric_limits<f32>::max());
        const double typeMaximum = integer ? static_cast<double>(std::numeric_limits<i32>::max())
                                           : static_cast<double>(std::numeric_limits<f32>::max());
        double minimum = typeMinimum;
        double maximum = typeMaximum;
        if (const auto range = definition.find("range"); range != definition.end()) {
            if (!range->is_array() || range->size() != 2 || !(*range)[0].is_number() || !(*range)[1].is_number()) {
                return Error(ErrorCode::InvalidData, "Entity property has an invalid numeric range: " + name);
            }
            minimum = (*range)[0].get<double>();
            maximum = (*range)[1].get<double>();
        }
        // 声明的 range 不能扩大底层数值类型范围，也不能为整数属性引入小数边界。
        if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum > maximum || minimum < typeMinimum ||
            maximum > typeMaximum || (integer && (std::floor(minimum) != minimum || std::floor(maximum) != maximum))) {
            return Error(ErrorCode::InvalidData, "Entity property has an invalid numeric range: " + name);
        }
        if (number < minimum || number > maximum) {
            return Error(ErrorCode::InvalidArgument, "Entity property value is outside its range: " + name);
        }

        return Result<void>::ok();
    }

    return Error(ErrorCode::InvalidArgument, "Invalid entity property value: " + name);
}

Result<void> EntityAddonState::defineProperties(const nlohmann::json& definitions)
{
    if (!definitions.is_object() || definitions.size() > 32) {
        return Error(ErrorCode::InvalidData, "Entity property definitions must be an object with at most 32 entries");
    }

    nlohmann::json values = nlohmann::json::object();
    for (const auto& [name, definition] : definitions.items()) {
        if (!mc::ResourceLocation::parse(name).isValid() || name.find(':') == std::string::npos ||
            !definition.is_object() || !definition.contains("default")) {
            return Error(ErrorCode::InvalidData, "Invalid entity property definition: " + name);
        }
        if (definition.contains("client_sync") && !definition["client_sync"].is_boolean()) {
            return Error(ErrorCode::InvalidData, "Entity property client_sync must be boolean: " + name);
        }

        // 表达式由 EntityPropertyDefinitions 在创建实例时求值，这里只安装实际默认值。
        MC_TRY(_validateProperty(name, definition, definition["default"]));
        if (definition["type"] == "enum") {
            if (definition["values"].size() > 16) {
                return Error(ErrorCode::InvalidData, "Entity property enum has more than 16 choices: " + name);
            }
            std::unordered_set<std::string> choices;
            for (const auto& choice : definition["values"]) {
                if (!choice.is_string() || !_validEnumName(choice.get_ref<const std::string&>()) ||
                    !choices.insert(choice.get<std::string>()).second) {
                    return Error(
                        ErrorCode::InvalidData, "Entity property enum choices must be unique strings: " + name);
                }
            }
        }
        values[name] = definition["default"];
    }

    m_definitions = definitions;
    m_properties = std::move(values);
    m_pending.clear();
    return Result<void>::ok();
}

std::optional<nlohmann::json> EntityAddonState::getProperty(const std::string& name) const
{
    const auto value = m_properties.find(name);
    return value != m_properties.end() ? std::optional<nlohmann::json>(*value) : std::nullopt;
}

Result<void> EntityAddonState::setProperty(const std::string& name, const nlohmann::json& value)
{
    const auto definition = m_definitions.find(name);
    if (definition == m_definitions.end()) {
        return Error(ErrorCode::InvalidArgument, "Entity property is not declared: " + name);
    }

    MC_TRY(_validateProperty(name, *definition, value));
    m_pending[name] = value;
    return Result<void>::ok();
}

bool EntityAddonState::applyPendingProperties()
{
    bool changed = false;
    for (const auto& [name, value] : m_pending.items()) {
        changed = changed || m_properties[name] != value;
    }
    m_properties.update(m_pending);
    m_pending.clear();
    return changed;
}

AddonDynamicProperties& EntityAddonState::dynamicProperties()
{
    return m_dynamic;
}

const AddonDynamicProperties& EntityAddonState::dynamicProperties() const
{
    return m_dynamic;
}

std::optional<nlohmann::json> EntityAddonState::resetProperty(const std::string& name)
{
    const auto definition = m_definitions.find(name);
    if (definition == m_definitions.end()) {
        return std::nullopt;
    }

    m_pending[name] = (*definition)["default"];
    return m_pending[name];
}

nlohmann::json EntityAddonState::save() const
{
    // 保存前合并尚未进入下一 tick 的合法写入，避免停服边界丢失已提交的状态。
    nlohmann::json properties = m_properties;
    properties.update(m_pending);
    nlohmann::json saved = {{"properties", nlohmann::json::object()}, {"dynamic", nlohmann::json::object()}};
    for (const auto& [name, value] : properties.items()) {
        saved["properties"][name] = encodeAddonPropertyValue(value);
    }
    saved["dynamic"] = m_dynamic.save();
    return saved;
}

Result<void> EntityAddonState::load(const nlohmann::json& saved)
{
    if (!saved.is_object() || !saved.contains("properties") || !saved["properties"].is_object() ||
        !saved.contains("dynamic") || !saved["dynamic"].is_object()) {
        return Error(ErrorCode::InvalidData, "Invalid saved entity addon state");
    }

    EntityAddonState restored;
    MC_TRY(restored.defineProperties(m_definitions));
    for (const auto& [name, encoded] : saved["properties"].items()) {
        auto value = decodeAddonPropertyValue(encoded);
        if (value.failed()) {
            return value.error();
        }
        MC_TRY(restored.setProperty(name, value.value()));
    }
    restored.applyPendingProperties();
    MC_TRY(restored.m_dynamic.load(saved["dynamic"]));

    *this = std::move(restored);
    return Result<void>::ok();
}

void registerEntityAddonStateSerializer(mc::entity::serialization::components::ComponentSerializerRegistry& registry)
{
    registry.registerSerializer<EntityAddonState>(_saveState, _loadState, 0);
}

} // namespace mc::mod::bedrock::addon
