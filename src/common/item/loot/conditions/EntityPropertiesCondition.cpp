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

#include "common/item/loot/conditions/EntityPropertiesCondition.hpp"

#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace loot {

namespace {

const Entity* getConditionTargetEntity(LootContext& context, EntityPropertiesCondition::EntityTarget target)
{
    switch (target) {
        case EntityPropertiesCondition::EntityTarget::This:
            return context.get<Entity>(LootParams::THIS_ENTITY);
        case EntityPropertiesCondition::EntityTarget::Attacker:
            return context.get<Entity>(LootParams::KILLER_ENTITY);
        case EntityPropertiesCondition::EntityTarget::DirectAttacker:
            return context.get<Entity>(LootParams::DIRECT_KILLER);
        case EntityPropertiesCondition::EntityTarget::AttackingPlayer: {
            auto* player = context.get<Player>(LootParams::KILLER_PLAYER);
            return static_cast<const Entity*>(player);
        }
        case EntityPropertiesCondition::EntityTarget::TargetEntity:
        case EntityPropertiesCondition::EntityTarget::InteractingEntity:
            // TODO(loot-context-targets): TARGET_ENTITY / INTERACTING_ENTITY 参数尚未在
            //   LootContext 中提供，按"目标实体不存在"处理（条件不匹配），避免误用 this。
            return nullptr;
    }
    return nullptr;
}

} // namespace

EntityPropertiesCondition::EntityPropertiesCondition(EntityTarget target, advancement::EntityPredicate predicate)
    : m_target(target)
    , m_predicate(std::move(predicate))
    , m_isAny(m_predicate.isAny())
{}

bool EntityPropertiesCondition::test(LootContext& context) const
{
    const Entity* entity = getConditionTargetEntity(context, m_target);

    if (!entity) {
        return false;
    }

    // 空谓词只检查实体是否存在
    if (m_isAny) {
        return true;
    }

    // 使用 EntityPredicate 的 test(world, x, y, z, entity) 方法
    // 位置使用实体的当前位置
    return m_predicate.test(context.getWorld(),
        static_cast<f64>(entity->x()),
        static_cast<f64>(entity->y()),
        static_cast<f64>(entity->z()),
        *entity);
}

std::unique_ptr<LootCondition> EntityPropertiesCondition::clone() const noexcept
{
    return std::make_unique<EntityPropertiesCondition>(m_target, m_predicate);
}

EntityPropertiesCondition::EntityTarget EntityPropertiesCondition::parseEntityTarget(const std::string& str)
{
    // 对齐 vanilla LootContext.EntityTarget 名称；"killer" / "direct_killer" /
    // "killer_player" 为旧版（1.16）名称，保留兼容映射。
    if (str == "this") {
        return EntityTarget::This;
    }
    if (str == "attacker" || str == "killer") {
        return EntityTarget::Attacker;
    }
    if (str == "direct_attacker" || str == "direct_killer") {
        return EntityTarget::DirectAttacker;
    }
    if (str == "attacking_player" || str == "killer_player") {
        return EntityTarget::AttackingPlayer;
    }
    if (str == "target_entity") {
        return EntityTarget::TargetEntity;
    }
    if (str == "interacting_entity") {
        return EntityTarget::InteractingEntity;
    }
    // 未知目标：返回 This 会静默改变语义，故显式按"目标缺失"处理。
    // TODO(loot-context-targets): 数据包新增目标类型时在此补充映射。
    return EntityTarget::TargetEntity;
}

std::string EntityPropertiesCondition::entityTargetToString(EntityTarget target)
{
    switch (target) {
        case EntityTarget::This:
            return "this";
        case EntityTarget::Attacker:
            return "attacker";
        case EntityTarget::DirectAttacker:
            return "direct_attacker";
        case EntityTarget::AttackingPlayer:
            return "attacking_player";
        case EntityTarget::TargetEntity:
            return "target_entity";
        case EntityTarget::InteractingEntity:
            return "interacting_entity";
        default:
            return "this";
    }
}

} // namespace loot
} // namespace mc
