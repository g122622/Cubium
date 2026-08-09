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

#include "common/entity/serialization/components/ComponentSerializerRegistry.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/serialization/components/EntityComponentSerialization.hpp"
#include "common/entity/serialization/components/LivingEntityComponentSerialization.hpp"
#include "common/entity/serialization/components/PlayerComponentSerialization.hpp"
#include <algorithm>

namespace mc::entity::serialization::components {

ComponentSerializerRegistry& ComponentSerializerRegistry::instance()
{
    static ComponentSerializerRegistry registry;
    return registry;
}

void ComponentSerializerRegistry::registerSerializerRaw(entt::id_type typeId, SaveFn save, LoadFn load, int priority)
{
    // 同 typeId 覆盖而非追加（幂等重注册场景）
    for (auto& entry : m_entries) {
        if (entry.typeId == typeId) {
            entry.save = save;
            entry.load = load;
            entry.priority = priority;
            return;
        }
    }
    m_entries.push_back(Entry{typeId, save, load, priority});
}

void ComponentSerializerRegistry::registerAll()
{
    // 幂等：重复调用先 clear 再重注册，支持测试 EntityRegistry::clear() 后重跑
    if (m_registered) {
        m_entries.clear();
    }
    m_registered = true;

    // 注册 Entity 层 9 个组件序列化器（覆盖 12 字段 + FallFlying，共 13 字段）。
    registerEntityComponentSerializers(*this);

    // 注册 LivingEntity 层 3 个组件序列化器（覆盖 Health/Absorption/HurtTime/DeathTime/Equipment 5 字段）。
    // 序列化器内部 dynamic_cast<LivingEntity*>，非 LivingEntity 实体早退。
    registerLivingEntityComponentSerializers(*this);

    // 注册 Player 层 1 个组件序列化器（覆盖 Score 1 字段）。
    // 序列化器内部 dynamic_cast<Player*>，非 Player 实体早退。
    registerPlayerComponentSerializers(*this);

    // load 按 priority 升序遍历（本批全 0 无序；未来 Attributes=100/ActiveEffects=200）
    std::stable_sort(
        m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b) { return a.priority < b.priority; });
}

void ComponentSerializerRegistry::saveAll(const Entity& entity, nbt::tags::compound_tag& tag) const
{
    for (const auto& entry : m_entries) {
        if (entry.save != nullptr) {
            entry.save(entity, tag);
        }
    }
}

Result<void> ComponentSerializerRegistry::loadAll(Entity& entity, const nbt::tags::compound_tag& tag) const
{
    // m_entries 已在 registerAll 末尾按 priority 升序排序
    for (const auto& entry : m_entries) {
        if (entry.load != nullptr) {
            MC_TRY(entry.load(entity, tag));
        }
    }
    return {};
}

} // namespace mc::entity::serialization::components
