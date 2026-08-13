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

#include "BoggedEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/monster/undead/AbstractSkeletonEntity.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include <memory>

namespace mc {

BoggedEntity::BoggedEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractSkeletonEntity(id, registry)
{
    registerGoals();
    registerAttributes();
    // 在 registerGoals() 之后设置战斗目标
    // 沼骸使用远程攻击（继承父类的 setCombatTask）
    setCombatTask();

    // 默认主手持弓：沼骸使用弓远程攻击，setCombatTask 判定 shouldUseRanged 依赖主手持弓
    // （不持弓则退化 MeleeAttackGoal 近战）。GameTest 的 test.spawn 不走 finalizeSpawn/
    // populateDefaultEquipmentSlots，故构造期补弓确保 GameTest spawn 的沼骸也能远程攻击。
    // 自然生成路径由 populateDefaultEquipmentSlots 给弓（isEmpty 守卫避免重复）。
    if (getEquipment(EquipmentSlot::MainHand).isEmpty() && Items::BOW != nullptr) {
        setEquipment(EquipmentSlot::MainHand, ItemStack(*Items::BOW, 1));
    }
}

std::unique_ptr<Entity> BoggedEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<BoggedEntity>(EntityInstanceId(0), registry);
}

void BoggedEntity::registerAttributes()
{
    AbstractSkeletonEntity::registerAttributes();

    // 沼骸生命值为 16（普通骷髅为 20）。对应原版 Bogged.createAttributes()
    // 的 Attributes.MAX_HEALTH, 16.0。
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, BOGGED_MAX_HEALTH);
}

void BoggedEntity::customizeArrow(entity::ArrowEntity& arrow)
{
    // 沼骸射出的箭矢附带 5 秒中毒 I 效果（对应原版 Arrow of Poison）。
    // 基类 attackEntityWithRangedAttack 创建普通箭矢后在发射前调用此钩子，
    // 此处为箭矢附加中毒效果，箭矢命中生物时由 ArrowEntity::onEntityHit 施加。
    // 对应原版 Bogged.getArrow()：arrow.addEffect(MobEffectInstance(MobEffects.POISON, 100))。
    // EffectType::Poison = 中毒，amplifier=0 即等级 I，duration=100 ticks = 5 秒。
    arrow.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, POISON_DURATION_TICKS, 0));
}

} // namespace mc
