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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace mc::mod::bedrock::addon {
namespace {

constexpr std::size_t _MAX_DYNAMIC_STRING_BYTES = 32767;
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

nlohmann::json _encode(const nlohmann::json& value)
{
    // JSON 布尔写入 NBT 后表现为 Byte，独立类型标记用于恢复原始脚本类型。
    const char* kind = value.is_boolean() ? "boolean"
        : value.is_string()               ? "string"
        : value.is_number()               ? "number"
                                          : "vector";
    return {{"kind", kind}, {"value", value}};
}

Result<nlohmann::json> _decode(const nlohmann::json& encoded)
{
    if (!encoded.is_object() || !encoded.contains("kind") || !encoded["kind"].is_string() ||
        !encoded.contains("value")) {
        return Error(ErrorCode::InvalidData, "Invalid addon property encoding");
    }

    const auto kind = encoded["kind"].get<std::string>();
    const auto& value = encoded["value"];
    if (kind == "boolean") {
        if (value.is_boolean()) {
            return value;
        }
        if (value.is_number_integer() && (value == 0 || value == 1)) {
            return nlohmann::json(value.get<int>() != 0);
        }
    } else if ((kind == "number" && value.is_number()) || (kind == "string" && value.is_string()) ||
        (kind == "vector" && value.is_object())) {
        return value;
    }

    return Error(ErrorCode::InvalidData, "Addon property type does not match its encoding");
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

        // TODO: 将非枚举属性的字符串默认值接入 Molang 求值；当前显式拒绝表达式。
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

std::optional<nlohmann::json> EntityAddonState::resetProperty(const std::string& name)
{
    const auto definition = m_definitions.find(name);
    if (definition == m_definitions.end()) {
        return std::nullopt;
    }

    m_pending[name] = (*definition)["default"];
    return m_pending[name];
}

Result<void> EntityAddonState::_validateDynamicValue(const nlohmann::json& value)
{
    if (value.is_null() || value.is_boolean()) {
        return Result<void>::ok();
    }
    if (value.is_string() && value.get_ref<const std::string&>().size() <= _MAX_DYNAMIC_STRING_BYTES) {
        return Result<void>::ok();
    }
    if (value.is_number() && std::isfinite(value.get<double>())) {
        return Result<void>::ok();
    }
    if (value.is_object() && value.size() == 3) {
        for (const char* coordinate : {"x", "y", "z"}) {
            if (!value.contains(coordinate) || !value[coordinate].is_number() ||
                !std::isfinite(value[coordinate].get<double>())) {
                return Error(ErrorCode::InvalidArgument, "Dynamic property requires finite Vector3 coordinates");
            }
        }
        return Result<void>::ok();
    }

    return Error(ErrorCode::InvalidArgument, "Unsupported dynamic property value or oversized string");
}

std::optional<nlohmann::json> EntityAddonState::getDynamicProperty(
    const std::string& pack, const std::string& name) const
{
    const auto scope = m_dynamic.find(pack);
    if (scope == m_dynamic.end()) {
        return std::nullopt;
    }

    const auto value = scope->find(name);
    return value != scope->end() ? std::optional<nlohmann::json>(*value) : std::nullopt;
}

Result<void> EntityAddonState::setDynamicProperty(
    const std::string& pack, const std::string& name, const nlohmann::json& value)
{
    if (pack.empty() || name.empty()) {
        return Error(ErrorCode::InvalidArgument, "Dynamic property requires a pack identity and nonempty key");
    }

    MC_TRY(_validateDynamicValue(value));
    if (value.is_null()) {
        if (auto scope = m_dynamic.find(pack); scope != m_dynamic.end()) {
            scope->erase(name);
            if (scope->empty()) {
                m_dynamic.erase(scope);
            }
        }
    } else {
        m_dynamic[pack][name] = value;
    }
    return Result<void>::ok();
}

std::vector<std::string> EntityAddonState::dynamicPropertyIds(const std::string& pack) const
{
    std::vector<std::string> names;
    if (const auto scope = m_dynamic.find(pack); scope != m_dynamic.end()) {
        for (const auto& [name, value] : scope->items()) {
            names.push_back(name);
        }
    }
    return names;
}

Result<void> EntityAddonState::setDynamicProperties(const std::string& pack, const nlohmann::json& values)
{
    if (pack.empty() || !values.is_object()) {
        return Error(ErrorCode::InvalidArgument, "Dynamic property batch requires a pack identity and an object");
    }

    // 校验和应用分离，避免后面的非法值令前面的写入部分提交。
    for (const auto& [name, value] : values.items()) {
        if (name.empty()) {
            return Error(ErrorCode::InvalidArgument, "Dynamic property requires a nonempty key");
        }
        MC_TRY(_validateDynamicValue(value));
    }

    for (const auto& [name, value] : values.items()) {
        MC_TRY(setDynamicProperty(pack, name, value));
    }
    return Result<void>::ok();
}

void EntityAddonState::clearDynamicProperties(const std::string& pack)
{
    m_dynamic.erase(pack);
}

nlohmann::json EntityAddonState::save() const
{
    // 保存前合并尚未进入下一 tick 的合法写入，避免停服边界丢失已提交的状态。
    nlohmann::json properties = m_properties;
    properties.update(m_pending);
    nlohmann::json saved = {{"properties", nlohmann::json::object()}, {"dynamic", nlohmann::json::object()}};
    for (const auto& [name, value] : properties.items()) {
        saved["properties"][name] = _encode(value);
    }
    for (const auto& [pack, scope] : m_dynamic.items()) {
        for (const auto& [name, value] : scope.items()) {
            saved["dynamic"][pack][name] = _encode(value);
        }
    }
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
        auto value = _decode(encoded);
        if (value.failed()) {
            return value.error();
        }
        MC_TRY(restored.setProperty(name, value.value()));
    }
    restored.applyPendingProperties();
    for (const auto& [pack, scope] : saved["dynamic"].items()) {
        if (!scope.is_object()) {
            return Error(ErrorCode::InvalidData, "Invalid dynamic property scope");
        }
        for (const auto& [name, encoded] : scope.items()) {
            auto value = _decode(encoded);
            if (value.failed()) {
                return value.error();
            }
            MC_TRY(restored.setDynamicProperty(pack, name, value.value()));
        }
    }

    *this = std::move(restored);
    return Result<void>::ok();
}

void registerEntityAddonStateSerializer(mc::entity::serialization::components::ComponentSerializerRegistry& registry)
{
    registry.registerSerializer<EntityAddonState>(_saveState, _loadState, 0);
}

} // namespace mc::mod::bedrock::addon
