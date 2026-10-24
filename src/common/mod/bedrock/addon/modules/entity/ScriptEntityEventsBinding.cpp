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

#include "common/mod/bedrock/addon/modules/entity/ScriptEntityEventsBinding.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/content/entity/events/AddonEntityEvents.hpp"
#include "common/mod/bedrock/addon/modules/properties/ScriptPropertyValue.hpp"

namespace mc::mod::bedrock::addon {
void registerEntityEventBindings(IScriptBindingContext& context, void* prototype)
{
    context.registerMethod(
        prototype,
        "triggerEvent",
        [](IScriptBindingContext& context, void* receiver, i32 argc, void** args) -> void* {
            const auto classId = context.classRegistry().classIdByName("Entity");
            auto* entity = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(context, receiver, classId));
            if (entity == nullptr || entity->isRemoved()) {
                return context.throwTypeError("Entity is no longer valid");
            }
            if (!hasScriptPropertyName(context, argc, args)) {
                return context.throwTypeError("triggerEvent requires a nonempty event name");
            }
            auto* events = entity->tryGetComponent<AddonEntityEvents>();
            if (events == nullptr) {
                // TODO: 将原版实体的行为包覆盖定义接入同一事件入口。
                return context.throwTypeError("Entity has no data-driven event definition");
            }
            auto result = events->trigger(*entity, *context.toString(args[0]));
            if (result.failed()) {
                // TODO: 使用官方 InvalidEntityError 和 EngineError 细分运行时错误。
                return context.throwTypeError(result.error().message().c_str());
            }
            return context.createUndefined();
        },
        1);
}
} // namespace mc::mod::bedrock::addon
