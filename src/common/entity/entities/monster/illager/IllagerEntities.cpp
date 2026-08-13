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

#include "IllagerEntities.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/illager/AbstractIllagerEntity.hpp"
#include "common/entity/entities/monster/illager/AbstractRaiderEntity.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "entity/ai/goal/GoalSelector.hpp"
#include "entity/ai/goal/goals/LookAtGoal.hpp"
#include "entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "entity/ai/goal/goals/SwimGoal.hpp"
#include "entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "entity/ai/goal/goals/interact/BreakDoorGoal.hpp"
#include "entity/ai/goal/goals/interact/RaiderOpenDoorGoal.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/ai/pathfinding/PathNavigator.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/combat/DifficultyHelper.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/core/MobEntity.hpp"
#include "entity/entities/passive/golem/IronGolemEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "entity/entities/villager/AbstractVillagerEntity.hpp"
#include "entity/interfaces/ICrossbowUser.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "item/items/weapon/ArrowItem.hpp"
#include "item/items/weapon/CrossbowItem.hpp"
#include "sound/SoundEvents.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"

#include <cmath>
#include <memory>
#include <utility>

namespace mc {

// ==================== 同步链标识（透传层，无自身同步字段） ====================
const entity::EntityClassInfo& PillagerEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"PillagerEntity", &AbstractIllagerEntity::classInfo()};
    return s_classInfo;
}

const entity::EntityClassInfo& VindicatorEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"VindicatorEntity", &AbstractIllagerEntity::classInfo()};
    return s_classInfo;
}

// ==================== PillagerEntity ====================

std::unique_ptr<Entity> PillagerEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<PillagerEntity>(EntityInstanceId(0), registry);
}

PillagerEntity::PillagerEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractIllagerEntity(id, registry)
{
    registerAttributes();

    // 补调 registerGoals：基类构造期间 vtable 指向基类，派生 override 永不执行，须在派生类构造
    // 显式调用。Pillager 的 registerGoals 加专属 SwimGoal / Crossbow / 近战等目标。
    registerGoals();
}

void PillagerEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    // charge 参数对弩不重要，弩使用固定速度
    MC_UNUSED(charge);

    if (!target || !m_world) return;

    // 获取主手弩
    ItemStack& crossbow = getMutableMainHandItem();
    const Item* item = crossbow.getItem();

    // 检查是否是弩
    if (item == nullptr || item->getUseAction(crossbow) != UseAction::Crossbow) {
        return;
    }

    // 调用 shootCrossbow 发射弩箭
    shootCrossbow(target, crossbow, 1.0f);
}

void PillagerEntity::onCrossbowLoadComplete(ItemStack& crossbow)
{
    // 装填完成后重置空闲时间，防止立即消失
    setIdleTime(0);

    // 装填完成时播放音效（如果需要）
    // 播放音效在 CrossbowItem 中已处理
    MC_UNUSED(crossbow);
}

void PillagerEntity::shootCrossbow(LivingEntity* target, ItemStack& crossbow, f32 charge)
{
    if (!target || !m_world || !crossbow.getItem()) return;

    MC_UNUSED(charge);

    // 计算弹道
    f64 dx = target->x() - x();
    f64 dz = target->z() - z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 目标高度偏移：目标高度的 1/3（瞄准躯干下部，对齐 MC AbstractSkeleton.performRangedAttack）
    // 弹道高度补偿：水平距离 * 0.2
    f64 dy = target->getY(0.3333333333333333) - (getEyeY() - 0.15) + horizontalDist * 0.2;

    // 计算弹道偏移角度（多重射击支持）
    // 掠夺者只有一支箭，偏移为 0
    f32 projectileAngle = 0.0f;

    // 确定速度
    f32 velocity = 1.6f; // 掠夺者使用的速度
    const item::CrossbowItem* crossbowItem = dynamic_cast<const item::CrossbowItem*>(crossbow.getItem());
    if (crossbowItem && item::CrossbowItem::hasChargedProjectile(crossbow, Items::FIREWORK_ROCKET)) {
        velocity = 1.6f; // 烟花速度
    } else {
        velocity = 3.15f; // 箭矢速度
    }

    // 计算难度相关的不精确度：14 - difficulty.getId() * 4
    // Peaceful=14, Easy=10, Normal=6, Hard=2
    f32 inaccuracy = entity::combat::DifficultyHelper::getRangedAttackInaccuracy(m_world->difficulty());

    // 创建箭矢实体
    // 掠夺者不消耗弹药，直接创建箭矢
    // ECS 迁移：实体构造需要 registry 句柄（m_world 已判空，此处 registry 必非空）
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return;
    }
    auto arrow = std::make_unique<entity::ArrowEntity>(EntityInstanceId(0), *registry);
    arrow->setTypeId(entity::EntityTypeKeys::ARROW); // 工厂绕过补救：直接构造缺 typeId
    arrow->setWorld(m_world);
    arrow->setPosition(x(), static_cast<f32>(getEyeY() - 0.15), z());
    arrow->setShooter(this);

    // 设置箭矢属性
    arrow->setShotFromCrossbow(true);
    arrow->setDamage(5.0f); // 掠夺者箭矢伤害

    // 计算发射方向（考虑偏移角度）
    f32 yaw = this->yaw();
    f32 pitch = this->pitch();

    // 如果有目标，计算指向目标的方向
    if (horizontalDist > 0.001) {
        yaw = static_cast<f32>(std::atan2(dz, dx) * 180.0 / math::PI) - 90.0f;
        pitch = static_cast<f32>(std::atan2(dy, horizontalDist) * 180.0 / math::PI);
    }

    // 应用弹道偏移角度（用于多重射击）
    if (projectileAngle != 0.0f) {
        yaw += projectileAngle;
    }

    // 发射箭矢
    arrow->shootFrom(*this, pitch, yaw, 0.0f, velocity, inaccuracy);

    // 生成箭矢实体
    m_world->spawnEntity(std::move(arrow));

    // 播放发射音效
    playSound(SoundEvents::ITEM_CROSSBOW_SHOOT, 1.0f, getRandom().nextFloat() * 0.4f + 0.8f);

    // 清除弩的装填状态
    if (crossbowItem) {
        item::CrossbowItem::setCharged(crossbow, false);
        item::CrossbowItem::clearProjectiles(crossbow);
    }
}

void PillagerEntity::registerGoals()
{
    AbstractIllagerEntity::registerGoals();

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 2: 寻找目标（袭击模式专用，这里简化处理）
    // m_goalSelector.addGoal(2, std::make_unique<FindTargetGoal>(this, 10.0f));

    // 优先级 3: 弩远程攻击
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::RangedCrossbowAttackGoal>(this, 1.0, 8.0f));

    // 优先级 8: 随机行走
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 0.6));

    // 优先级 9: 看向玩家
    m_goalSelector.addGoal(9,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 15.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 10: 看向生物
    m_goalSelector.addGoal(10, std::make_unique<entity::ai::goal::LookAtGoal>(this, 15.0f));

    // 目标选择器
    // 优先级 1: 被攻击后反击并呼叫支援
    // MC 原版: HurtByTargetGoal(this, Raider.class).setAlertOthers()
    // 掠夺者不会反击其他灾厄村民
    m_targetSelector.addGoal(
        1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true, [](const LivingEntity* attacker) -> bool {
            return dynamic_cast<const AbstractRaiderEntity*>(attacker) != nullptr;
        }));

    // 优先级 2: 攻击玩家
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true, 0));

    // 优先级 3: 攻击村民（穿透墙壁感知）
    // MC 原版: NearestAttackableTargetGoal<>(this, AbstractVillager.class, false)
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<entity::AbstractVillagerEntity>>(this, false));

    // 优先级 3: 攻击铁傀儡
    // MC 原版: NearestAttackableTargetGoal<>(this, IronGolem.class, true)
    m_targetSelector.addGoal(
        3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(this, true));
}

void PillagerEntity::registerAttributes()
{
    AbstractIllagerEntity::registerAttributes();

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 32.0);
}

// ==================== VindicatorEntity ====================

std::unique_ptr<Entity> VindicatorEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<VindicatorEntity>(EntityInstanceId(0), registry);
}

VindicatorEntity::VindicatorEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractIllagerEntity(id, registry)
{
    registerAttributes();

    // 补调 registerGoals：基类构造期间 vtable 指向基类，派生 override 永不执行，须在派生类构造
    // 显式调用。Vindicator 的 registerGoals 加专属破门 / 近战等目标。
    registerGoals();
}

void VindicatorEntity::registerGoals()
{
    AbstractIllagerEntity::registerGoals();

    // 对应 MC Java 版 Vindicator.registerGoals / finalizeSpawn：
    // 卫道士需要开启导航器的开门能力，以便破门目标能够激活
    auto* nav = navigator();
    if (nav) {
        nav->setCanOpenDoors(true);
    }

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 破门（所有难度，因为卫道士在袭击中破门）
    m_goalSelector.addGoal(1,
        std::make_unique<entity::ai::goal::BreakDoorGoal>(
            this, entity::ai::goal::defaultDoorBreakDifficultyPredicate()));

    // 优先级 2: 袭击期间开门（不关门，不需要 mobGriefing 规则和难度检查）
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::RaiderOpenDoorGoal>(this));

    // 优先级 3: 寻找目标（袭击模式专用，简化处理）
    // m_goalSelector.addGoal(3, std::make_unique<FindTargetGoal>(this, 10.0f));

    // 优先级 4: 近战攻击
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, false));

    // 优先级 8: 随机行走
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 0.6));

    // 优先级 9: 看向玩家
    m_goalSelector.addGoal(9,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 3.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 10: 看向生物
    m_goalSelector.addGoal(10, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f));

    // 目标选择器
    // 优先级 1: 被攻击后反击并呼叫支援
    // MC 原版: HurtByTargetGoal(this, Raider.class).setAlertOthers()
    // 卫道士不会反击其他灾厄村民
    m_targetSelector.addGoal(
        1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true, [](const LivingEntity* attacker) -> bool {
            return dynamic_cast<const AbstractRaiderEntity*>(attacker) != nullptr;
        }));

    // 优先级 2: 攻击玩家
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true, 0));

    // 优先级 3: 攻击村民
    // MC 原版: NearestAttackableTargetGoal<>(this, AbstractVillager.class, true)
    m_targetSelector.addGoal(
        3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<entity::AbstractVillagerEntity>>(this, true));

    // 优先级 3: 攻击铁傀儡
    // MC 原版: NearestAttackableTargetGoal<>(this, IronGolem.class, true)
    m_targetSelector.addGoal(
        3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(this, true));
}

void VindicatorEntity::registerAttributes()
{
    AbstractIllagerEntity::registerAttributes();

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    // 基础攻击伤害为 5.0（铁斧额外 +3，总计 8）
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 12.0);
}

} // namespace mc
