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

#pragma once

#include "common/core/Types.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace mc::mod::bedrock::addon {

/**
 * @brief 跨模块脚本类原型注册表（进程级单例）。
 *
 * 问题背景：QuickJS 的 classId 按绑定上下文（`IScriptBindingContext`）分配，各原生模块
 * （如 `@minecraft/server` 与 `@minecraft/server-gametest`）各自 `exportClass` 独立分配
 * classId 与原型句柄。但跨模块 wrap 需求客观存在——例如 GameTest 模块的 `test.spawn`
 * 返回值应是 `@minecraft/server` 的 `Entity` JS 对象，运行期回调要用 `@minecraft/server`
 * 的 Entity classId + proto 才能经 `ScriptObjectRegistry::wrap` 构造出带方法的 JS 对象。
 *
 * 本注册表按 classId 与类名双索引存原型句柄（`void*`），供：
 * 1. 注册期：模块工厂 `exportClass` 后调 `registerClass` 登记（如 `MinecraftModuleFactory`
 *    登记 Entity/Dimension，`GameTestModuleBinding` 登记 Test/Sequence）；
 * 2. 运行期：跨模块 wrap 回调按名/按 classId 查 proto（如 `test.spawn` 查 Entity proto）。
 *
 * 与 `mc::test::ScriptBindingRegistry` 的区别：后者在 `mc::test` 命名空间（仅 minecraft-server
 * 链接），GameTest 专属；本注册表在 `mc::mod::bedrock::addon`（common 共享层），可被
 * `@minecraft/server` 与 `@minecraft/server-gametest` 两模块工厂共用，避免层级违反。
 *
 * 句柄所有权：原型句柄由 `IScriptBindingContext` 管理（GC 可达），本注册表仅存裸指针不 retain；
 * 脚本引擎销毁前原型稳定。引擎重建时须 `clear()` 清空，因重建后 classId 重新分配、旧 proto 失效。
 */
class ScriptClassRegistry {
public:
    [[nodiscard]] static ScriptClassRegistry& instance() noexcept;

    /**
     * @brief 登记 classId → 原型句柄 + 类名（注册期调）。
     *
     * 同 classId 重复登记覆盖（引擎重建后重新登记场景）。
     */
    void registerClass(u64 classId, void* proto, std::string_view name) noexcept;

    /**
     * @brief 按 classId 取原型句柄（运行期 wrap 调）；未登记返回 nullptr。
     */
    [[nodiscard]] void* proto(u64 classId) const noexcept;

    /**
     * @brief 按类名取原型句柄；未登记返回 nullptr。
     */
    [[nodiscard]] void* protoByName(std::string_view name) const noexcept;

    /**
     * @brief 按类名取 classId；未登记返回 0。
     */
    [[nodiscard]] u64 classIdByName(std::string_view name) const noexcept;

    /**
     * @brief 清空所有条目（脚本引擎重建/测试隔离用）。
     */
    void clear() noexcept;

private:
    ScriptClassRegistry() noexcept = default;

    struct Entry {
        void* proto = nullptr;
        std::string name;
    };

    std::unordered_map<u64, Entry> m_byId;
    std::unordered_map<std::string, u64> m_idByName;
};

} // namespace mc::mod::bedrock::addon
