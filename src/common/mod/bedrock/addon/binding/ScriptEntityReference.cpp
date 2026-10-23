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

#include "common/mod/bedrock/addon/binding/ScriptEntityReference.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/mod/bedrock/addon/binding/ScriptHandleRegistry.hpp"

namespace mc::mod::bedrock::addon {

struct ScriptEntityReference::State {
    ScriptObjectRegistry::ObjectData data;

    explicit State(mc::Entity* entity)
        : data{entity, false, "Entity", nullptr, entity->id(), entity}
    {
        ScriptHandleRegistry::instance().registerHandle(&data);
    }
    ~State() { ScriptHandleRegistry::instance().unregisterHandle(&data); }
};

ScriptEntityReference::ScriptEntityReference(mc::Entity* entity)
    : m_state(entity != nullptr ? std::make_shared<State>(entity) : nullptr)
{}

mc::Entity* ScriptEntityReference::get() const
{
    return m_state ? static_cast<mc::Entity*>(m_state->data.ptr) : nullptr;
}

} // namespace mc::mod::bedrock::addon
