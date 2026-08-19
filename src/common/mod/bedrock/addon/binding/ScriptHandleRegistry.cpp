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
 *
 */

#include "common/mod/bedrock/addon/binding/ScriptHandleRegistry.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"

#include <algorithm>

namespace mc::mod::bedrock::addon {

ScriptHandleRegistry& ScriptHandleRegistry::instance() noexcept
{
    static ScriptHandleRegistry s_instance;
    return s_instance;
}

void ScriptHandleRegistry::registerHandle(EntityInstanceId entityId, ScriptObjectRegistry::ObjectData* data) noexcept
{
    if (entityId == 0 || data == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handles[entityId].push_back(data);
}

void ScriptHandleRegistry::unregisterHandle(ScriptObjectRegistry::ObjectData* data) noexcept
{
    if (data == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    // finalizer 不知道 entityId，按 data* 值在所有列表中移除。data* 全局唯一（堆分配），
    // 不会误删其他实体的句柄。线性扫描所有 entityId 列表——实体数有限，可接受。
    for (auto& [id, list] : m_handles) {
        auto it = std::find(list.begin(), list.end(), data);
        if (it != list.end()) {
            list.erase(it);
            // 不 erase 空 list：invalidateAll 会处理；避免遍历中改 map 结构。
            return;
        }
    }
}

void ScriptHandleRegistry::invalidateAll(EntityInstanceId entityId) noexcept
{
    if (entityId == 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_handles.find(entityId);
    if (it == m_handles.end()) {
        return;
    }
    // 置所有指向该实体的 JS 句柄 ptr=nullptr。之后 unwrap 返 nullptr，调用点判空守卫拦截。
    // 注意：data->ptr=nullptr 后 finalizer 仍会 delete data（owned=false finalizer 只 delete
    // ObjectData 结构不 delete ptr），并在 delete 前 unregisterHandle——但此时该 data 已从列表
    // 移除（下面的 erase），unregisterHandle 找不到它 no-op，安全。
    for (auto* data : it->second) {
        if (data != nullptr) {
            data->ptr = nullptr;
        }
    }
    m_handles.erase(it);
}

void ScriptHandleRegistry::clear() noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // 注意：不清 data->ptr——clear 时机是引擎重建，此时所有 ObjectData 即将被 finalizer 释放，
    // 不需置 nullptr（JS 侧不会再访问）。仅清注册表结构。
    m_handles.clear();
}

} // namespace mc::mod::bedrock::addon
