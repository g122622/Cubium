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

#include "common/mod/bedrock/addon/modules/entity/ScriptEntityPropertiesBinding.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"

#include <functional>
#include <string>

namespace mc::mod::bedrock::addon {
namespace {

using _EntityMethod = std::function<void*(IScriptBindingContext&, mc::Entity&, i32, void**)>;

ScriptMethodCallback _method(_EntityMethod callback)
{
    // TODO: 接入官方 InvalidEntityError、InvalidArgumentError 和 ArgumentOutOfBoundsError 类型。
    return [callback = std::move(callback)](IScriptBindingContext& context, void* receiver, i32 argc, void** args) {
        const auto entityClass = context.classRegistry().classIdByName("Entity");
        auto* entity = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(context, receiver, entityClass));
        if (entity == nullptr || entity->isRemoved()) {
            return context.throwTypeError("Entity is no longer valid");
        }

        return callback(context, *entity, argc, args);
    };
}

Result<nlohmann::json> _readValue(IScriptBindingContext& context, void* value)
{
    switch (context.getType(value)) {
        case ScriptType::Undefined:
        case ScriptType::Null:
            return nlohmann::json(nullptr);
        case ScriptType::Boolean:
            return nlohmann::json(*context.toBool(value));
        case ScriptType::Number:
            return nlohmann::json(*context.toFloat64(value));
        case ScriptType::String:
            return nlohmann::json(*context.toString(value));
        case ScriptType::Object: {
            nlohmann::json vector = nlohmann::json::object();
            for (const char* coordinate : {"x", "y", "z"}) {
                void* member = context.getProperty(value, coordinate);
                const bool numeric = context.isNumber(member);
                const auto number = numeric ? context.toFloat64(member) : std::nullopt;
                context.releaseValue(member);
                if (!number) {
                    return Error(ErrorCode::InvalidArgument, "Property object must be a Vector3");
                }
                vector[coordinate] = *number;
            }
            return vector;
        }
        default:
            return Error(ErrorCode::InvalidArgument, "Unsupported property value type");
    }
}

void* _writeValue(IScriptBindingContext& context, const nlohmann::json& value)
{
    if (value.is_boolean()) {
        return context.createBoolean(value.get<bool>());
    }
    if (value.is_string()) {
        return context.createString(value.get_ref<const std::string&>());
    }
    if (value.is_number()) {
        return context.createFloat64(value.get<double>());
    }

    void* vector = context.createObject();
    for (const char* coordinate : {"x", "y", "z"}) {
        context.setPropertyFloat(vector, coordinate, value[coordinate].get<double>());
    }
    return vector;
}

bool _hasName(IScriptBindingContext& context, i32 argc, void** args)
{
    return argc > 0 && context.isString(args[0]) && !context.toString(args[0])->empty();
}

} // namespace

void registerEntityPropertyBindings(IScriptBindingContext& context, void* prototype)
{
    context.registerMethod(prototype,
        "getProperty",
        _method([](IScriptBindingContext& context, mc::Entity& entity, i32 argc, void** args) -> void* {
            if (!_hasName(context, argc, args)) {
                return context.throwTypeError("getProperty requires a nonempty property name");
            }

            const auto* state = entity.tryGetComponent<EntityAddonState>();
            const auto value = state != nullptr ? state->getProperty(*context.toString(args[0])) : std::nullopt;
            return value ? _writeValue(context, *value) : context.createUndefined();
        }),
        1);

    context.registerMethod(prototype,
        "setProperty",
        _method([](IScriptBindingContext& context, mc::Entity& entity, i32 argc, void** args) -> void* {
            if (argc < 2 || !_hasName(context, argc, args)) {
                return context.throwTypeError("setProperty requires a name and value");
            }

            auto* state = entity.tryGetComponent<EntityAddonState>();
            if (state == nullptr) {
                return context.throwTypeError("Entity has no declared properties");
            }
            auto value = _readValue(context, args[1]);
            if (value.failed()) {
                return context.throwTypeError(value.error().message().c_str());
            }

            auto result = state->setProperty(*context.toString(args[0]), value.value());
            if (result.failed()) {
                return context.throwTypeError(result.error().message().c_str());
            }
            return context.createUndefined();
        }),
        2);

    context.registerMethod(prototype,
        "resetProperty",
        _method([](IScriptBindingContext& context, mc::Entity& entity, i32 argc, void** args) -> void* {
            if (!_hasName(context, argc, args)) {
                return context.throwTypeError("resetProperty requires a nonempty property name");
            }

            auto* state = entity.tryGetComponent<EntityAddonState>();
            const auto value = state != nullptr ? state->resetProperty(*context.toString(args[0])) : std::nullopt;
            return value ? _writeValue(context, *value) : context.createUndefined();
        }),
        1);

    context.registerMethod(prototype,
        "getDynamicProperty",
        _method([](IScriptBindingContext& context, mc::Entity& entity, i32 argc, void** args) -> void* {
            if (!_hasName(context, argc, args)) {
                return context.throwTypeError("getDynamicProperty requires a nonempty name");
            }

            const auto* state = entity.tryGetComponent<EntityAddonState>();
            const auto value = state != nullptr
                ? state->getDynamicProperty(context.packId(), *context.toString(args[0]))
                : std::nullopt;
            return value ? _writeValue(context, *value) : context.createUndefined();
        }),
        1);

    context.registerMethod(prototype,
        "setDynamicProperty",
        _method([](IScriptBindingContext& context, mc::Entity& entity, i32 argc, void** args) -> void* {
            if (!_hasName(context, argc, args)) {
                return context.throwTypeError("setDynamicProperty requires a nonempty name");
            }

            // 未传 value 或显式传入 null/undefined 时，删除当前包作用域内的属性。
            auto value = argc > 1 ? _readValue(context, args[1]) : Result<nlohmann::json>(nlohmann::json(nullptr));
            if (value.failed()) {
                return context.throwTypeError(value.error().message().c_str());
            }
            auto result = entity.getOrAddComponent<EntityAddonState>().setDynamicProperty(
                context.packId(), *context.toString(args[0]), value.value());
            if (result.failed()) {
                return context.throwTypeError(result.error().message().c_str());
            }
            return context.createUndefined();
        }),
        2);

    context.registerMethod(prototype,
        "getDynamicPropertyIds",
        _method([](IScriptBindingContext& context, mc::Entity& entity, i32, void**) -> void* {
            void* result = context.createArray();
            const auto* state = entity.tryGetComponent<EntityAddonState>();
            if (state != nullptr) {
                const auto names = state->dynamicPropertyIds(context.packId());
                for (u32 index = 0; index < names.size(); ++index) {
                    context.setArrayElementString(result, index, names[index]);
                }
            }
            return result;
        }),
        0);

    context.registerMethod(prototype,
        "setDynamicProperties",
        _method([](IScriptBindingContext& context, mc::Entity& entity, i32 argc, void** args) -> void* {
            if (argc == 0 || context.getType(args[0]) != ScriptType::Object) {
                return context.throwTypeError("setDynamicProperties requires an object");
            }

            nlohmann::json values = nlohmann::json::object();
            for (const auto& name : context.getPropertyNames(args[0])) {
                void* member = context.getProperty(args[0], name.c_str());
                auto value = _readValue(context, member);
                context.releaseValue(member);
                if (value.failed()) {
                    return context.throwTypeError(value.error().message().c_str());
                }
                values[name] = std::move(value.value());
            }

            auto result = entity.getOrAddComponent<EntityAddonState>().setDynamicProperties(context.packId(), values);
            if (result.failed()) {
                return context.throwTypeError(result.error().message().c_str());
            }
            return context.createUndefined();
        }),
        1);

    context.registerMethod(prototype,
        "clearDynamicProperties",
        _method([](IScriptBindingContext& context, mc::Entity& entity, i32, void**) -> void* {
            if (auto* state = entity.tryGetComponent<EntityAddonState>()) {
                state->clearDynamicProperties(context.packId());
            }
            return context.createUndefined();
        }),
        0);
}

} // namespace mc::mod::bedrock::addon
