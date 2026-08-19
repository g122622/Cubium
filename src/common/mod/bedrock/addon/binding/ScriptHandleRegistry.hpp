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
 * 实体销毁两条路径（见 `EntityManager` graveyard 机制）：
 * - 路径A（`remove()`/discard 标记 → graveyard 延迟析构）：对象标记后存活到下一 tick 末尾才 free，
 *   此窗口内 `isRemoved()=true`。`getComponent` 入口的 `if (ent->isRemoved()) return undefined`
 *   止血守卫在此窗口有效。
 * - 路径B（`EntityManager::removeEntity` 丢弃 `unique_ptr` → 立即 free，如区块卸载
 *   `ServerWorld::onChunkUnloading`）：对象立即 free，`isRemoved()` 守卫本身在已 free 对象上调用
 *   即 UAF，止血无效。
 *
 * 本注册表彻底根治两条路径：实体销毁时（`EntityManager::removeEntity` + `Entity::~Entity` 兜底）
 * 调 `invalidateAll(id)`，把所有指向该实体的 `ObjectData::ptr` 置 nullptr。之后 `unwrap` 返 nullptr，
 * 各绑定调用点已有的 `if (ent == nullptr) return undefined;` 守卫拦截，JS 侧得到 undefined 而非 UAF。
 *
 * 机制：
 * - `wrap` 对 Entity 系（`entityId != 0`）调 `registerHandle(entityId, data)` 登记该 `ObjectData*`。
 * - QuickJS finalizer（`delete data` 前）对 `entityId != 0` 的 data 调 `unregisterHandle(data)`，
 *   防止注册表持有已 delete 的 `ObjectData*`（否则后续 `invalidateAll` 解引用悬垂 data* UAF）。
 * - 实体销毁调 `invalidateAll(id)`：遍历该 id 下所有 data*，置 `ptr=nullptr`，再清空该 id 的列表。
 *
 * 线程安全：JS 回调（finalizer）与游戏侧 remove/析构可能并发访问注册表，内部 `std::mutex` 串行化。
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
     * @param entityId 实体实例 ID（EntityInstanceId）。
     * @param data 该 JS 对象的 ObjectData 指针（opaque 存储，finalizer 前 persistent）。
     *
     * 同一实体可有多个 data（Entity 本身 + 各组件对象），均登记在同一 entityId 下，
     * `invalidateAll` 一次清空全部。
     */
    void registerHandle(EntityInstanceId entityId, ScriptObjectRegistry::ObjectData* data) noexcept;

    /**
     * @brief 注销一个 ObjectData（QuickJS finalizer 在 delete data 前调）。
     *
     * 防止注册表持有已 delete 的 data*——否则后续 invalidateAll 解引用悬垂 data* UAF。
     * 按 data* 值在所有 entityId 列表中移除（finalizer 不知道 entityId，但 data* 全局唯一）。
     */
    void unregisterHandle(ScriptObjectRegistry::ObjectData* data) noexcept;

    /**
     * @brief 使某实体所有 JS 句柄失效（实体销毁时调）。
     *
     * 遍历该 entityId 下所有 data*，置 `data->ptr = nullptr`（owned 标志不变——owned=false 的
     * Entity 句柄本就不由 JS GC delete，置 ptr=nullptr 后 finalizer 仅 delete ObjectData 结构）。
     * 之后清空该 entityId 的列表。重复调用安全（已清空则 no-op）。
     */
    void invalidateAll(EntityInstanceId entityId) noexcept;

    /**
     * @brief 清空所有条目（脚本引擎重建/进程退出用，对齐 ScriptClassRegistry::clear 时机）。
     */
    void clear() noexcept;

private:
    ScriptHandleRegistry() noexcept = default;

    std::mutex m_mutex;
    // EntityInstanceId → 该实体所有 JS 句柄的 ObjectData* 列表。
    // 一个实体可有多个句柄（Entity 本身 + OnFire/Health/... 组件对象），均 opaque 持同一 Entity*。
    std::unordered_map<EntityInstanceId, std::vector<ScriptObjectRegistry::ObjectData*>> m_handles;
};

} // namespace mc::mod::bedrock::addon
