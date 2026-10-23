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

#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp"

namespace mc::mod::bedrock::addon {

void ScriptClassRegistry::registerClass(u64 classId, void* proto, std::string_view name) noexcept
{
    auto& entry = m_byId[classId];
    entry.proto = proto;
    entry.name = std::string(name);
    m_idByName[entry.name] = classId;
}

void* ScriptClassRegistry::proto(u64 classId) const noexcept
{
    auto it = m_byId.find(classId);
    if (it == m_byId.end()) {
        return nullptr;
    }
    return it->second.proto;
}

void* ScriptClassRegistry::protoByName(std::string_view name) const noexcept
{
    auto it = m_idByName.find(std::string(name));
    if (it == m_idByName.end()) {
        return nullptr;
    }
    return proto(it->second);
}

u64 ScriptClassRegistry::classIdByName(std::string_view name) const noexcept
{
    auto it = m_idByName.find(std::string(name));
    if (it == m_idByName.end()) {
        return 0;
    }
    return it->second;
}

void ScriptClassRegistry::unregisterClass(u64 classId) noexcept
{
    auto it = m_byId.find(classId);
    if (it == m_byId.end()) {
        return;
    }
    // 名索引只在仍指向本 classId 时删除：同名类可能已被后续上下文的 registerClass 覆盖为
    // 新的 classId，此时删名会误伤新一代映射（classId 由 JS_NewClassID 分配，跨上下文递增不重复）。
    auto nameIt = m_idByName.find(it->second.name);
    if (nameIt != m_idByName.end() && nameIt->second == classId) {
        m_idByName.erase(nameIt);
    }
    m_byId.erase(it);
}

void ScriptClassRegistry::clear() noexcept
{
    m_byId.clear();
    m_idByName.clear();
}

} // namespace mc::mod::bedrock::addon
