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
 * The above copyright and this permission notice shall be included in all
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
#include "common/entity/core/EntityType.hpp"
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace mc {

class Entity;

/**
 * @brief 单个 16³ section 内的实体集合
 *
 * 全实体列表 `m_all` 为真源，按类型子列表 `m_byType` 懒加载缓存。对齐 Java
 * `ClassInstanceMultiMap`：同一 section 内既要能枚举全部实体（AABB/球查精筛），
 * 也要能只枚举某类型实体（类型查询跳过无关实体）。
 *
 * 类型 key 用 `const entity::EntityType*`（指针稳定，见 EntityRegistry::getType，
 * 指向 deque 内对象，clear 前地址不变）。主流 40+ 处类型判定已用指针比较。
 *
 * 一致性约定：`m_byType` 仅在 `entitiesOfType(type)` 首次请求某类型时构建该类型的
 * 子列表；此后 `add`/`remove` 同步维护已存在的键，保证缓存与 `m_all` 始终一致。
 * 未被请求过的类型不占缓存空间（懒加载）。
 */
class EntitySectionBucket {
public:
    /**
     * @brief 加入一个实体到本 section
     *
     * 加到 `m_all` 末尾；若 `m_byType` 已含该实体类型键（说明该类型曾被查询过、
     * 缓存已建），则同步 push 到子列表，保持缓存一致。未查询过的类型不建缓存。
     */
    void add(Entity& entity);

    /**
     * @brief 从本 section 移除指定实体
     *
     * swap-remove 从 `m_all`；若 `m_byType` 含该实体类型键，则 swap-remove 子列表，
     * 子列表归零时 erase 键（释放空间）。返回是否找到并移除。
     */
    bool remove(const Entity& entity);

    /**
     * @brief 本 section 全部实体（真源）
     *
     * 查询遍历用下标 + `size()` 重取（见 EntitySpatialIndex 查询算法），因回调内
     * 实体 move 可能触发本 bucket 的 swap-remove，迭代器语义不安全。
     */
    [[nodiscard]] const std::vector<Entity*>& allEntities() const noexcept { return m_all; }

    /**
     * @brief 本 section 内指定类型的实体子列表（懒加载）
     *
     * 首次请求某类型时遍历 `m_all` 构建子列表并缓存；后续请求直接返回缓存。
     * 返回的引用在下次 `add`/`remove` 前 stable，但查询期间回调可能触发 `add`/
     * `remove` 改变其内容，故遍历同样需下标 + `size()` 重取。
     */
    [[nodiscard]] const std::vector<Entity*>& entitiesOfType(const entity::EntityType* type) const;

    [[nodiscard]] size_t size() const noexcept { return m_all.size(); }
    [[nodiscard]] bool isEmpty() const noexcept { return m_all.empty(); }
    void clear();

private:
    std::vector<Entity*> m_all;
    // mutable：entitiesOfType 是 const 方法但需懒填充缓存
    mutable std::unordered_map<const entity::EntityType*, std::vector<Entity*>> m_byType;
};

} // namespace mc
