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

#include "common/mod/bedrock/addon/modules/item/ScriptItemPropertiesBinding.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/binding/ScriptGameObjects.hpp"
#include "common/mod/bedrock/addon/content/item/ItemAddonProperties.hpp"
#include "common/mod/bedrock/addon/modules/properties/ScriptDynamicPropertiesBinding.hpp"
#include <memory>

namespace mc::mod::bedrock::addon {

void registerItemPropertyBindings(IScriptBindingContext& context, void* prototype)
{
    registerDynamicPropertyBindings(context,
        prototype,
        [](IScriptBindingContext& context, void* receiver, bool writable) -> Result<DynamicPropertyAccess> {
            const auto classId = context.classRegistry().classIdByName("ItemStack");
            auto* item = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(context, receiver, classId));
            if (item == nullptr || item->isEmpty()) {
                return Error(ErrorCode::InvalidState, "Item stack is no longer valid");
            }
            if (writable && item->getMaxStackSize() > 1) {
                // TODO: 通过 @minecraft/common.UnsupportedFunctionalityError 报告此限制。
                return Error(ErrorCode::InvalidArgument, "Dynamic properties require a non-stackable item");
            }

            auto loaded = ItemAddonProperties::read(*item);
            if (loaded.failed()) {
                return loaded.error();
            }
            auto snapshot = std::make_shared<AddonDynamicProperties>(std::move(loaded.value()));
            return DynamicPropertyAccess{snapshot.get(), [snapshot, item]() -> Result<void> {
                                             if (!snapshot->isDirty()) {
                                                 return Result<void>::ok();
                                             }
                                             return ItemAddonProperties::write(*item, *snapshot);
                                         }};
        });

    context.registerMethod(
        prototype,
        "clone",
        [](IScriptBindingContext& context, void* receiver, i32, void**) -> void* {
            const auto classId = context.classRegistry().classIdByName("ItemStack");
            auto* item = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(context, receiver, classId));
            if (item == nullptr || item->isEmpty()) {
                return context.throwTypeError("Item stack is no longer valid");
            }
            return wrapItemStack(context, *item);
        },
        0);
}

} // namespace mc::mod::bedrock::addon
