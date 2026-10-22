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

#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/components/MobFlagComponent.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
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
    // TODO: 敌对骆驼缺乏 monster 基类目标（近战/目标选择/随机张望的怪物版）。
    //       registerGoals 由 CamelEntity 构造函数（此时虚派发仍落在 CamelEntity）调用过一次，
    //       且 GoalSelector 不提供"清空全部目标"的公开接口，故此处只能补注册。
    //       CamelHusk 真正需要的目标集（MeleeAttackGoal / NearestAttackableTargetGoal 等）
    //       待 GoalSelector 支持移除全部目标后按 vanilla Monster 重新装配。
    (void)0;
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

bool CamelHuskEntity::isFoodItem(const ItemStack& /*itemStack*/) const
{
    // TODO: 对齐 vanilla CamelHusk.isFood → ItemTags.CAMEL_HUSK_FOOD。项目物品标签体系
    //       尚未定义该标签，暂不识别任何食物，待标签补齐后按 camel_husk_food 判定。
    return false;
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