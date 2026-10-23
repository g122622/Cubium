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

#include "common/mod/bedrock/addon/modules/properties/ScriptDynamicPropertiesBinding.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/content/base/properties/AddonDynamicProperties.hpp"
#include "common/mod/bedrock/addon/modules/properties/ScriptPropertyValue.hpp"

namespace mc::mod::bedrock::addon {
namespace {
using _PropertyMethod = std::function<void*(IScriptBindingContext&, AddonDynamicProperties&, i32, void**)>;

ScriptMethodCallback _method(DynamicPropertyAccessor accessor, _PropertyMethod callback)
{
    return [accessor = std::move(accessor), callback = std::move(callback)](
               IScriptBindingContext& context, void* receiver, i32 argc, void** args) -> void* {
        auto* properties = accessor(context, receiver);
        if (properties == nullptr) {
            // TODO: 按对象类型抛出官方细分失效错误。
            return context.throwTypeError("Dynamic property owner is no longer valid");
        }
        return callback(context, *properties, argc, args);
    };
}
} // namespace

void registerDynamicPropertyBindings(IScriptBindingContext& context, void* prototype, DynamicPropertyAccessor accessor)
{
    context.registerMethod(prototype,
        "getDynamicProperty",
        _method(accessor,
            [](IScriptBindingContext& context, AddonDynamicProperties& properties, i32 argc, void** args) -> void* {
                if (!hasScriptPropertyName(context, argc, args)) {
                    return context.throwTypeError("getDynamicProperty requires a nonempty name");
                }

                const auto value = properties.get(context.packId(), *context.toString(args[0]));
                return value ? writeScriptPropertyValue(context, *value) : context.createUndefined();
            }),
        1);

    context.registerMethod(prototype,
        "setDynamicProperty",
        _method(accessor,
            [](IScriptBindingContext& context, AddonDynamicProperties& properties, i32 argc, void** args) -> void* {
                if (!hasScriptPropertyName(context, argc, args)) {
                    return context.throwTypeError("setDynamicProperty requires a nonempty name");
                }

                // 未传 value 或显式传入 null/undefined 时，删除当前包作用域内的属性。
                auto value = argc > 1 ? readScriptPropertyValue(context, args[1])
                                      : Result<nlohmann::json>(nlohmann::json(nullptr));
                if (value.failed()) {
                    return throwScriptPropertyError(context, value.error().message().c_str());
                }
                auto result = properties.set(context.packId(), *context.toString(args[0]), value.value());
                if (result.failed()) {
                    return context.throwTypeError(result.error().message().c_str());
                }
                return context.createUndefined();
            }),
        2);

    context.registerMethod(prototype,
        "getDynamicPropertyIds",
        _method(accessor,
            [](IScriptBindingContext& context, AddonDynamicProperties& properties, i32, void**) -> void* {
                void* result = context.createArray();
                const auto names = properties.ids(context.packId());
                for (u32 index = 0; index < names.size(); ++index) {
                    context.setArrayElementString(result, index, names[index]);
                }
                return result;
            }),
        0);

    context.registerMethod(prototype,
        "setDynamicProperties",
        _method(accessor,
            [](IScriptBindingContext& context, AddonDynamicProperties& properties, i32 argc, void** args) -> void* {
                if (argc == 0 || context.getType(args[0]) != ScriptType::Object) {
                    return context.throwTypeError("setDynamicProperties requires an object");
                }

                nlohmann::json values = nlohmann::json::object();
                const auto names = context.getPropertyNames(args[0]);
                if (context.hasException()) {
                    return throwScriptPropertyError(context, "Failed to enumerate dynamic property keys");
                }
                for (const auto& name : names) {
                    void* member = context.getProperty(args[0], name.c_str());
                    auto value = readScriptPropertyValue(context, member);
                    context.releaseValue(member);
                    if (value.failed()) {
                        return throwScriptPropertyError(context, value.error().message().c_str());
                    }
                    values[name] = std::move(value.value());
                }

                auto result = properties.setMany(context.packId(), values);
                if (result.failed()) {
                    return context.throwTypeError(result.error().message().c_str());
                }
                return context.createUndefined();
            }),
        1);

    context.registerMethod(prototype,
        "clearDynamicProperties",
        _method(accessor,
            [](IScriptBindingContext& context, AddonDynamicProperties& properties, i32, void**) -> void* {
                properties.clear(context.packId());
                return context.createUndefined();
            }),
        0);
}

} // namespace mc::mod::bedrock::addon
