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
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本实体句柄失效注册表（进程级单例）。
 *
 * 问题背景：GameTest 脚本经 `ScriptObjectRegistry::wrap` 以 `owned=false` 持裸 `mc::Entity*`
 * （Entity 本身及 OnFire/Health/Movement/Equippable 等组件 JS 对象都 opaque 持同一 `Entity*`）。
 * C++ 侧实体销毁后这些 JS 句柄悬垂，后续 `getComponent("minecraft:onfire")` 等回调解引用悬垂
 * 指针（如 `Entity::isOnFire`）即 UAF，段错误杀掉整个 GameTestServer 全量运行。
 *
 * 实体销毁两条路径（见 `EntityManager` 的两阶段销毁：逻辑移除 → graveyard 延迟析构）：
 * - 销毁路径（`EntityManager::destroyEntity` / `remove()`+tick 收走 → graveyard 延迟析构）：
 *   对象标记后存活到下一 tick 末尾才 free，此窗口内 `isRemoved()=true`。`getComponent` 入口的
 *   `if (ent->isRemoved()) return undefined` 止血守卫在此窗口有效，且析构时由 `~Entity` 兜底 invalidate。
 * - 移交路径（`EntityManager::takeEntity` 把 `unique_ptr` 交给调用者，跨维度迁移）：若调用者
 *   丢弃 unique_ptr 实体立即 free，`isRemoved()` 守卫本身在已 free 对象上调用即 UAF。故 takeEntity
 *   内主动 invalidate 作为该路径的止血点。
 *
 * 本注册表彻底根治两条路径：实体销毁/移交时（`EntityManager::takeEntity` + `Entity::~Entity` 兜底）
 * 调 `invalidateAll(entity)`，把所有指向该实体的 `ObjectData::ptr` 置 nullptr。之后 `unwrap` 返 nullptr，
 * 各绑定调用点已有的 `if (ent == nullptr) return undefined;` 守卫拦截，JS 侧得到 undefined 而非 UAF。
 *
 * 机制：
 * - `wrap` 对 Entity 系（`entityId != 0`）调 `registerHandle(data)` 登记该 `ObjectData*`。
 * - QuickJS finalizer（`delete data` 前）对 `entityId != 0` 的 data 调 `unregisterHandle(data)`，
 *   防止注册表持有已 delete 的 `ObjectData*`（否则后续 `invalidateAll` 解引用悬垂 data* UAF）。
 * - 实体销毁调 `invalidateAll(entity)`：按实际对象地址遍历 data*，置 `ptr=nullptr`
 * 后移除记录；实体编号可在不同世界重复。
 *
 * 线程约束：包装、解包与实体访问必须在所属游戏线程执行；该线程依次驱动实体和脚本 tick。
 * 内部 `std::mutex` 保护注册表容器，不能延长实体寿命，也不允许跨线程包装正在销毁的裸指针。
 *
 * 与 `ScriptClassRegistry` 同层（addon/binding，仅依赖 core），可被 core 层
 * （`EntityManager`/`Entity`）引用——与既有 core→addon/component（`BlockComponentEvents`）跨层
 * 耦合同性质。引擎重建时 `ScriptClassRegistry::clear` 同步清本表（重建后旧 ObjectData* 失效）。
 */
class ScriptHandleRegistry {
public:
    [[nodiscard]] static ScriptHandleRegistry& instance() noexcept;

    /**
     * @brief 登记一个实体 JS 句柄的 ObjectData（wrap 时调，entityId != 0）。
     *
     * @param data 该 JS 对象的 ObjectData 指针（opaque 存储，finalizer 前 persistent）。
     *
     * 同一实体可有多个 data（Entity 本身 + 各组件对象），均按实际对象地址登记，
     * `invalidateAll` 一次清空全部。
     */
    void registerHandle(ScriptObjectRegistry::ObjectData* data) noexcept;

    /**
     * @brief 注销一个 ObjectData（QuickJS finalizer 在 delete data 前调）。
     *
     * 防止注册表持有已 delete 的 data*——否则后续 invalidateAll 解引用悬垂 data* UAF。
     * 按 data* 值移除记录，即使该句柄的 ptr 已单独失效也能正确注销。
     */
    void unregisterHandle(ScriptObjectRegistry::ObjectData* data) noexcept;

    /**
     * @brief 使某实体所有 JS 句柄失效（实体销毁时调）。
     *
     * 遍历该对象地址下所有 data*，置 `data->ptr = nullptr`（owned 标志不变——owned=false 的
     * Entity 句柄本就不由 JS GC delete，置 ptr=nullptr 后 finalizer 仅 delete ObjectData 结构）。
     * 之后清空该对象的列表。不同维度和临时实体可同号，必须按地址区分。重复调用安全。
     * @param entity 实体实际对象地址
     */
    void invalidateAll(const void* entity) noexcept;

    /**
     * @brief 清空所有条目（脚本引擎重建/进程退出用，与 ScriptClassRegistry::clear 同时执行）。
     */
    void clear() noexcept;

private:
    ScriptHandleRegistry() noexcept = default;

    std::mutex m_mutex;
    // 实体对象地址 → 该实体所有 JS 句柄的 ObjectData* 列表，实体编号不具进程级唯一性。
    // 一个实体可有多个句柄，opaque 可指向实体或其背包等子对象，按 entityOwner 统一管理。
    std::unordered_map<const void*, std::vector<ScriptObjectRegistry::ObjectData*>> m_handles;
};

} // namespace mc::mod::bedrock::addon
