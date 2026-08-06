/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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
 */

#include "server/test/script/context/ScriptBindingRegistry.hpp"

namespace mc::test {

ScriptBindingRegistry& ScriptBindingRegistry::instance() noexcept
{
    static ScriptBindingRegistry s_instance;
    return s_instance;
}

void ScriptBindingRegistry::registerProto(u64 classId, void* proto) noexcept
{
    m_protos[classId] = proto;
}

void* ScriptBindingRegistry::proto(u64 classId) const noexcept
{
    auto it = m_protos.find(classId);
    if (it == m_protos.end()) {
        return nullptr;
    }
    return it->second;
}

void ScriptBindingRegistry::clear() noexcept
{
    m_protos.clear();
    m_testClassId = 0;
    // m_scheduler 不清空：它由 GameTestModuleBinding::setScheduler 注入，指向 ScriptManager 拥有的
    // ScriptScheduler，其生命周期与脚本引擎重建无关（引擎重建时 ScriptManager 仍存活）。
}

} // namespace mc::test
