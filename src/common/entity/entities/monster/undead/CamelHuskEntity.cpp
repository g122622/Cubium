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

#include "CamelHuskEntity.hpp"

#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"

#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/components/MobFlagComponent.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/world/IWorld.hpp"

#include <memory>

namespace mc {

const entity::EntityClassInfo& CamelHuskEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"CamelHuskEntity", &CamelEntity::classInfo()};
    return s_classInfo;
}

CamelHuskEntity::CamelHuskEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : CamelEntity(id, registry)
{
    // CamelEntity 构造函数按"被动坐骑"注册了繁殖/诱惑/跟随父母/漫步目标，
    // 骆驼尸壳为敌对生物且不可繁殖，须替换为怪物行为集。
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> CamelHuskEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<CamelHuskEntity>(EntityInstanceId(0), registry);
}

void CamelHuskEntity::registerGoals()
{
    // CamelEntity 构造函数按"被动坐骑"注册了繁殖/诱惑/跟随父母/随机坐下等目标
    // （构造期虚派发落在 CamelEntity，无法直接跳过）。骆驼尸壳是敌对生物，须整体替换为
    // 怪物目标集：先清空两个选择器，再按 vanilla Monster/Zombie 的口径重装。
    m_goalSelector.removeAllGoals();
    m_targetSelector.removeAllGoals();

    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, false));
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0));
    m_goalSelector.addGoal(
        8, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true));
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true));
}

void CamelHuskEntity::registerAttributes()
{
    CamelEntity::registerAttributes();
}

bool CamelHuskEntity::isMobControlled() const
{
    if (m_world == nullptr || !hasPassengers()) {
        return false;
    }
    const Entity* firstPassenger = m_world->getEntity(getPassengers().front());
    return firstPassenger != nullptr && firstPassenger->hasComponent<ecs::MobFlagComponent>();
}

ActionResultType CamelHuskEntity::interactMob(Player& player, Hand hand)
{
    // 对齐 vanilla CamelHusk.interact：被玩家右键即标记持久化（不退场），再走骆驼交互。
    enablePersistence();
    return CamelEntity::interactMob(player, hand);
}

bool CamelHuskEntity::isFoodItem(const ItemStack& itemStack) const
{
    // 对齐 vanilla CamelHusk.isFood：ItemTags.CAMEL_HUSK_FOOD（兔子脚），
    // 与被动骆驼的 CAMEL_FOOD（仙人掌）不同。
    const Item* item = itemStack.getItem();
    return item != nullptr && !itemStack.isEmpty() && item->isIn(item::tag::ItemTags::CAMEL_HUSK_FOOD());
}

// ============================================================================
// 声音
// ============================================================================

std::optional<ResourceLocation> CamelHuskEntity::getAmbientSound() const
{
    // makeSoundEventId 会以实体类型补全命名空间与 entity. 前缀，故实体类型 ID 本身即为音效名：
    // minecraft:camel_husk → minecraft:entity.camel_husk.<suffix>。
    return mc::ResourceLocation("minecraft:entity.camel_husk.ambient");
}

std::optional<ResourceLocation> CamelHuskEntity::getHurtSound(DamageSource& /*source*/) const
{
    return mc::ResourceLocation("minecraft:entity.camel_husk.hurt");
}

std::optional<ResourceLocation> CamelHuskEntity::getDeathSound() const
{
    return mc::ResourceLocation("minecraft:entity.camel_husk.death");
}

std::optional<ResourceLocation> CamelHuskEntity::getEatSound() const
{
    return mc::ResourceLocation("minecraft:entity.camel_husk.eat");
}

} // namespace mc