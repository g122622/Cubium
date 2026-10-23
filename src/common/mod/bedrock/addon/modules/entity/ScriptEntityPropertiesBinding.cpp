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

#include "common/mod/bedrock/addon/modules/properties/ScriptDynamicPropertiesBinding.hpp"
#include "common/mod/bedrock/addon/modules/properties/ScriptPropertyValue.hpp"

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

} // namespace

void registerEntityPropertyBindings(IScriptBindingContext& context, void* prototype)
{
    context.registerMethod(prototype,
        "getProperty",
        _method([](IScriptBindingContext& context, mc::Entity& entity, i32 argc, void** args) -> void* {
            if (!hasScriptPropertyName(context, argc, args)) {
                return context.throwTypeError("getProperty requires a nonempty property name");
            }

            const auto* state = entity.tryGetComponent<EntityAddonState>();
            const auto value = state != nullptr ? state->getProperty(*context.toString(args[0])) : std::nullopt;
            return value ? writeScriptPropertyValue(context, *value) : context.createUndefined();
        }),
        1);

    context.registerMethod(prototype,
        "setProperty",
        _method([](IScriptBindingContext& context, mc::Entity& entity, i32 argc, void** args) -> void* {
            if (argc < 2 || !hasScriptPropertyName(context, argc, args)) {
                return context.throwTypeError("setProperty requires a name and value");
            }

            auto* state = entity.tryGetComponent<EntityAddonState>();
            if (state == nullptr) {
                return context.throwTypeError("Entity has no declared properties");
            }
            auto value = readScriptPropertyValue(context, args[1]);
            if (value.failed()) {
                return throwScriptPropertyError(context, value.error().message().c_str());
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
            if (!hasScriptPropertyName(context, argc, args)) {
                return context.throwTypeError("resetProperty requires a nonempty property name");
            }

            auto* state = entity.tryGetComponent<EntityAddonState>();
            const auto value = state != nullptr ? state->resetProperty(*context.toString(args[0])) : std::nullopt;
            return value ? writeScriptPropertyValue(context, *value) : context.createUndefined();
        }),
        1);

    registerDynamicPropertyBindings(
        context, prototype, [](IScriptBindingContext& context, void* receiver, bool) -> Result<DynamicPropertyAccess> {
            const auto entityClass = context.classRegistry().classIdByName("Entity");
            auto* entity = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(context, receiver, entityClass));
            if (entity == nullptr || entity->isRemoved()) {
                return Error(ErrorCode::InvalidState, "Entity is no longer valid");
            }
            return DynamicPropertyAccess{&entity->getOrAddComponent<EntityAddonState>().dynamicProperties(), []() {
                                             return Result<void>::ok();
                                         }};
        });
}

} // namespace mc::mod::bedrock::addon
