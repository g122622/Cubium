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

#include "common/mod/bedrock/addon/binding/ScriptGameObjects.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"

#include <memory>

namespace mc::mod::bedrock::addon {

void* wrapEntity(IScriptBindingContext& context, mc::Entity* entity)
{
    if (entity == nullptr) {
        return context.createUndefined();
    }

    const char* name = dynamic_cast<mc::Player*>(entity) != nullptr ? "Player" : "Entity";
    const auto id = context.classRegistry().classIdByName(name);
    void* prototype = context.classRegistry().proto(id);
    MC_ASSERT_RELEASE(prototype != nullptr);

    return ScriptObjectRegistry::wrap(context, id, prototype, entity, false, name, nullptr, entity->id(), entity);
}

void* wrapDimension(IScriptBindingContext& context, mc::IWorld* world)
{
    if (world == nullptr) {
        return context.createUndefined();
    }

    const auto id = context.classRegistry().classIdByName("Dimension");
    void* prototype = context.classRegistry().proto(id);
    MC_ASSERT_RELEASE(prototype != nullptr);

    return ScriptObjectRegistry::wrap(context, id, prototype, world, false, "Dimension", nullptr, 0, nullptr);
}

void* wrapItemStack(IScriptBindingContext& context, const mc::ItemStack& item)
{
    const auto id = context.classRegistry().classIdByName("ItemStack");
    void* prototype = context.classRegistry().proto(id);
    MC_ASSERT_RELEASE(prototype != nullptr);
    auto* owned = std::make_unique<mc::ItemStack>(item).release();

    return ScriptObjectRegistry::wrap(
        context,
        id,
        prototype,
        owned,
        true,
        "ItemStack",
        [](void* value) {
            delete static_cast<mc::ItemStack*>(value);
        },
        0,
        nullptr);
}

} // namespace mc::mod::bedrock::addon
