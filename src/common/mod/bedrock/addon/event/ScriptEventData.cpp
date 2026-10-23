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

#include "common/mod/bedrock/addon/event/ScriptEventData.hpp"

#include "common/item/core/ItemStack.hpp"
#include "common/mod/bedrock/addon/binding/ScriptBlockRef.hpp"
#include "common/mod/bedrock/addon/binding/ScriptEntityReference.hpp"
#include "common/mod/bedrock/addon/binding/ScriptGameObjects.hpp"

#include <utility>

namespace mc::mod::bedrock::addon {

void ScriptEventData::setValue(const std::string& name, ValueBuilder builder)
{
    m_values.insert_or_assign(name, std::move(builder));
}
void ScriptEventData::setString(const std::string& name, std::string value)
{
    setValue(name, [value = std::move(value)](IScriptBindingContext& context) { return context.createString(value); });
}
void ScriptEventData::setNumber(const std::string& name, f64 value)
{
    setValue(name, [value](IScriptBindingContext& context) { return context.createFloat64(value); });
}
void ScriptEventData::setBoolean(const std::string& name, bool value)
{
    setValue(name, [value](IScriptBindingContext& context) { return context.createBoolean(value); });
}
void ScriptEventData::setEntity(const std::string& name, mc::Entity* entity)
{
    setValue(name, [reference = ScriptEntityReference(entity)](IScriptBindingContext& context) {
        return wrapEntity(context, reference.get());
    });
}
void ScriptEventData::setDimension(const std::string& name, mc::IWorld* world)
{
    setValue(name, [world](IScriptBindingContext& context) { return wrapDimension(context, world); });
}
void ScriptEventData::setItem(const std::string& name, const mc::ItemStack& item)
{
    setValue(name, [item](IScriptBindingContext& context) { return wrapItemStack(context, item); });
}
void ScriptEventData::setBlock(
    const std::string& name, mc::IWorld* world, const mc::BlockPos& pos, const mc::BlockState* state)
{
    setValue(
        name, [world, pos, state](IScriptBindingContext& context) { return wrapBlock(context, state, pos, world); });
}
void ScriptEventData::setObject(const std::string& name, ScriptEventData object)
{
    setValue(
        name, [object = std::move(object)](IScriptBindingContext& context) { return object.createObject(context); });
}
void* ScriptEventData::createObject(IScriptBindingContext& context) const
{
    void* object = context.createObject();
    for (const auto& [name, builder] : m_values) {
        void* value = builder(context);
        context.setProperty(object, name.c_str(), value);
        context.releaseValue(value);
    }
    return object;
}

} // namespace mc::mod::bedrock::addon
