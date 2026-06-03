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

#include "common/entity/entities/passive/horse/SkeletonHorseEntity.hpp"
#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <memory>

namespace mc {

SkeletonHorseEntity::SkeletonHorseEntity(EntityId id)
    : AbstractHorseEntity(id)
{
    // 骷髅马默认已驯服
    setTame(true);
    // 设置跳跃强度
    setJumpStrength(1.0f);
}

std::unique_ptr<Entity> SkeletonHorseEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SkeletonHorseEntity>(0);
}

bool SkeletonHorseEntity::canBeRiddenBy(Player* player) const
{
    // 骷髅马不需要驯服即可骑乘
    if (m_rider != nullptr && m_rider != player) {
        return false;
    }
    return true;
}

void SkeletonHorseEntity::setTrap(bool trap)
{
    // 设置陷阱状态时，添加/移除 TriggerSkeletonTrapGoal
    if (trap != m_trap) {
        m_trap = trap;
        if (trap) {
            // 添加陷阱触发目标（优先级 1）
            auto goal = std::make_unique<entity::ai::goal::TriggerSkeletonTrapGoal>(this);
            m_trapGoal = goal.get();
            m_goalSelector.addGoal(1, std::move(goal));
        } else {
            // 移除陷阱触发目标
            if (m_trapGoal != nullptr) {
                m_goalSelector.removeGoal(m_trapGoal);
                m_trapGoal = nullptr;
            }
        }
    }
}

void SkeletonHorseEntity::triggerTrap()
{
    if (!m_trap) {
        return;
    }

    IWorld* world = this->world();
    if (world == nullptr) {
        return;
    }

    // 1. 清除陷阱状态
    m_trap = false;

    // 2. 设置骷髅马为已驯服
    setTame(true);

    // 3. 获取难度决定生成骷髅数量
    // 困难模式下额外生成 3 只骷髅马+骑手（共 4 只）
    // 普通和简单模式只生成 1 只骷髅骑手骑这匹马
    Difficulty difficulty = world->difficulty();
    i32 extraHorses = (difficulty == Difficulty::Hard) ? 3 : 0;

    // 4. 获取骷髅实体类型
    const entity::EntityType* skeletonType = entity::EntityRegistry::instance().getType(entity::EntityTypes::SKELETON);
    if (skeletonType == nullptr) {
        return;
    }

    // 5. 获取这匹马的位置用于生成
    Vector3 horsePos = position();
    math::Random& rng = world->getRandom();

    // 6. 创建第一个骷髅骑手（骑在这匹马上）
    {
        auto skeleton = skeletonType->create(world);
        if (skeleton == nullptr) {
            return;
        }

        LivingEntity* skeletonEntity = dynamic_cast<LivingEntity*>(skeleton.get());
        if (skeletonEntity == nullptr) {
            return;
        }

        // 设置位置
        skeleton->setPosition(horsePos);

        // 设置持久化
        MobEntity* mobEntity = dynamic_cast<MobEntity*>(skeleton.get());
        if (mobEntity != nullptr) {
            mobEntity->enablePersistence();
        }

        // 装备铁头盔
        if (skeletonEntity->getEquipment(EquipmentSlot::Head).isEmpty()) {
            ItemStack helmet(Items::IRON_HELMET, 1);
            skeletonEntity->setEquipment(EquipmentSlot::Head, helmet);
        }

        // 装备弓
        ItemStack bow(Items::BOW, 1);
        skeletonEntity->setMainHandItem(bow);

        // 设置无敌帧
        skeletonEntity->setHurtResistantTime(60);

        // 生成骷髅到世界
        EntityId skeletonId = world->spawnEntity(std::move(skeleton));

        // 让骷髅骑上这匹马
        if (skeletonId != INVALID_ENTITY_ID) {
            Entity* spawnedSkeleton = world->getEntity(skeletonId);
            if (spawnedSkeleton != nullptr) {
                spawnedSkeleton->startRiding(*this);
            }
        }
    }

    // 7. 困难模式下生成额外的骷髅马+骑手
    for (i32 i = 0; i < extraHorses; ++i) {
        // 创建额外的骷髅马
        const entity::EntityType* skeletonHorseType =
            entity::EntityRegistry::instance().getType(entity::EntityTypes::SKELETON_HORSE);
        if (skeletonHorseType == nullptr) {
            continue;
        }

        auto extraHorse = skeletonHorseType->create(world);
        if (extraHorse == nullptr) {
            continue;
        }

        // 设置位置（在原马周围随机偏移）
        f32 offsetX = static_cast<f32>(rng.nextGaussian(0.0, 0.5));
        f32 offsetZ = static_cast<f32>(rng.nextGaussian(0.0, 0.5));
        extraHorse->setPosition(horsePos + Vector3(offsetX, 0, offsetZ));

        // 设置为已驯服的骷髅马
        SkeletonHorseEntity* extraHorseEntity = dynamic_cast<SkeletonHorseEntity*>(extraHorse.get());
        if (extraHorseEntity != nullptr) {
            extraHorseEntity->setTame(true);
            extraHorseEntity->setTrap(false);
        }

        // 设置持久化
        MobEntity* extraHorseMob = dynamic_cast<MobEntity*>(extraHorse.get());
        if (extraHorseMob != nullptr) {
            extraHorseMob->enablePersistence();
        }

        // 设置无敌帧
        LivingEntity* extraHorseLiving = dynamic_cast<LivingEntity*>(extraHorse.get());
        if (extraHorseLiving != nullptr) {
            extraHorseLiving->setHurtResistantTime(60);
        }

        // 创建骷髅骑手
        auto extraSkeleton = skeletonType->create(world);
        if (extraSkeleton == nullptr) {
            continue;
        }

        LivingEntity* extraSkeletonEntity = dynamic_cast<LivingEntity*>(extraSkeleton.get());
        if (extraSkeletonEntity == nullptr) {
            continue;
        }

        extraSkeleton->setPosition(extraHorse->position());

        // 设置持久化
        MobEntity* extraSkeletonMob = dynamic_cast<MobEntity*>(extraSkeleton.get());
        if (extraSkeletonMob != nullptr) {
            extraSkeletonMob->enablePersistence();
        }

        // 装备铁头盔
        if (extraSkeletonEntity->getEquipment(EquipmentSlot::Head).isEmpty()) {
            ItemStack helmet(Items::IRON_HELMET, 1);
            extraSkeletonEntity->setEquipment(EquipmentSlot::Head, helmet);
        }

        // 装备弓
        ItemStack extraBow(Items::BOW, 1);
        extraSkeletonEntity->setMainHandItem(extraBow);

        // 设置无敌帧
        extraSkeletonEntity->setHurtResistantTime(60);

        // 生成额外骷髅马
        EntityId extraHorseId = world->spawnEntity(std::move(extraHorse));
        if (extraHorseId != INVALID_ENTITY_ID) {
            // 生成骷髅并让它骑上骷髅马
            EntityId extraSkeletonId = world->spawnEntity(std::move(extraSkeleton));
            if (extraSkeletonId != INVALID_ENTITY_ID) {
                Entity* spawnedHorse = world->getEntity(extraHorseId);
                Entity* spawnedSkeleton = world->getEntity(extraSkeletonId);
                if (spawnedHorse != nullptr && spawnedSkeleton != nullptr) {
                    spawnedSkeleton->startRiding(*spawnedHorse);
                }
            }
        }
    }

    // TODO: 当闪电实体实现后，在此处添加闪电视觉效果
    // LightningBoltEntity lightning = EntityType.LIGHTNING_BOLT.create(serverworld);
    // lightning.moveForced(horsePos.x, horsePos.y, horsePos.z);
    // lightning.setEffectOnly(true);  // 只有视觉效果，不造成伤害
    // serverworld.addEntity(lightning);
}

void SkeletonHorseEntity::onStruckByLightning()
{
    // 陷阱马被闪电击中时触发陷阱
    if (m_trap) {
        triggerTrap();
    }
}

void SkeletonHorseEntity::tick()
{
    AbstractHorseEntity::tick();

    // 陷阱马超时逻辑：如果存在超过 18000 ticks (15分钟)，自动消失
    if (m_trap) {
        m_trapTime++;
        if (m_trapTime >= TRAP_MAX_TIME) {
            remove();
            return;
        }
    }

    // 骷髅马阳光燃烧逻辑
    // 注意：骷髅马不是 MonsterEntity 的子类，所以需要手动实现阳光燃烧
    if (isAlive() && shouldBurnInDaylight() && isInDaylight()) {
        // 检查头盔保护
        // 如果戴着头盔，头盔会受损而不是燃烧
        // 骷髅马通常不戴头盔，但为完整性保留此逻辑
        const ItemStack& helmet = getEquipment(EquipmentSlot::Head);
        if (helmet.isEmpty()) {
            // 没有头盔，燃烧 8 秒
            setFire(8);
        }
        // TODO: 如果有头盔，应该损坏头盔而不是燃烧
        // 这需要实现 ItemStack.damage() 方法
    }
}

void SkeletonHorseEntity::registerGoals()
{
    AbstractHorseEntity::registerGoals();
    // 骷髅马没有额外 AI
}

void SkeletonHorseEntity::registerAttributes()
{
    AbstractHorseEntity::registerAttributes();

    // 骷髅马的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 15.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2f);
}

} // namespace mc
