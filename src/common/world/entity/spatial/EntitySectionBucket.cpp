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

#include "EntitySectionBucket.hpp"
#include "common/entity/core/Entity.hpp"

#include <algorithm>

namespace mc {

void EntitySectionBucket::add(Entity& entity)
{
    m_all.push_back(&entity);
    // 仅当该类型缓存已建（曾被 entitiesOfType 请求过）时同步维护，避免为未查询类型
    // 提前分配缓存空间。
    const entity::EntityType* type = entity.entityType();
    auto it = m_byType.find(type);
    if (it != m_byType.end()) {
        it->second.push_back(&entity);
    }
}

bool EntitySectionBucket::remove(const Entity& entity)
{
    // swap-remove 从 m_all：找到目标指针，与末尾交换后 pop_back，O(1)。
    auto it = std::find(m_all.begin(), m_all.end(), &entity);
    if (it == m_all.end()) {
        return false;
    }
    *it = m_all.back();
    m_all.pop_back();

    // 同步维护已缓存的类型子列表
    const entity::EntityType* type = entity.entityType();
    auto typeIt = m_byType.find(type);
    if (typeIt != m_byType.end()) {
        auto& sub = typeIt->second;
        auto subIt = std::find(sub.begin(), sub.end(), &entity);
        if (subIt != sub.end()) {
            *subIt = sub.back();
            sub.pop_back();
        }
        if (sub.empty()) {
            m_byType.erase(typeIt);
        }
    }
    return true;
}

const std::vector<Entity*>& EntitySectionBucket::entitiesOfType(const entity::EntityType* type) const
{
    // 懒加载：首次请求某类型时遍历 m_all 构建子列表。后续 add/remove 同步维护。
    auto it = m_byType.find(type);
    if (it == m_byType.end()) {
        std::vector<Entity*> sub;
        for (Entity* e : m_all) {
            if (e->entityType() == type) {
                sub.push_back(e);
            }
        }
        it = m_byType.emplace(type, std::move(sub)).first;
    }
    return it->second;
}

void EntitySectionBucket::clear()
{
    m_all.clear();
    m_byType.clear();
}

} // namespace mc
