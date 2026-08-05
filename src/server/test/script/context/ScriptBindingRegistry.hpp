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

#pragma once

#include "common/core/Types.hpp"

#include <unordered_map>

namespace mc::test {

/**
 * @brief 脚本类原型注册表（单例）。
 *
 * GameTest 模块绑定注册期（`GameTestModuleBinding::registerBindings`）经 `NativeModuleBuilder::exportClass`
 * 取得各 JS 类的原型句柄（`void*`），但运行期回调（如 `Test.startSequence` 返回 `GameTestSequence` 对象）
 * 需重新拿到原型句柄才能 `ScriptObjectRegistry::wrap` 出带方法的 JS 对象。本注册表按 classId 存原型句柄，
 * 供运行期回调查询。
 *
 * 句柄所有权：原型句柄在注册期由 `IScriptBindingContext` 管理（GC 可达），本注册表仅存裸指针不 retain；
 * 脚本引擎销毁前原型稳定。`ScriptManager::shutdown` 销毁引擎后本注册表条目失效，但因引擎已不存在，
 * 不会再有回调查询——`GameTestTicker::forceStop()` 在 shutdown 前清测试实例保证无悬垂回调。
 */
class ScriptBindingRegistry {
public:
    [[nodiscard]] static ScriptBindingRegistry& instance() noexcept;

    /**
     * @brief 记录 classId → 原型句柄（注册期调）。
     */
    void registerProto(u64 classId, void* proto) noexcept;

    /**
     * @brief 按 classId 取原型句柄（运行期回调调）；未登记返回 nullptr。
     */
    [[nodiscard]] void* proto(u64 classId) const noexcept;

    /**
     * @brief 清空所有条目（脚本引擎重建/测试隔离用）。
     */
    void clear() noexcept;

private:
    ScriptBindingRegistry() noexcept = default;

    std::unordered_map<u64, void*> m_protos;
};

} // namespace mc::test
