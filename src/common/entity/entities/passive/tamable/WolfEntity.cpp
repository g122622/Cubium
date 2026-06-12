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

#include "WolfEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/interact/TameableGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/monster/basic/CreeperEntity.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/entity/entities/passive/basic/RabbitEntity.hpp"
#include "common/entity/entities/passive/basic/SheepEntity.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/special/FoxEntity.hpp"
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"

#include <cmath>

namespace mc {

WolfEntity::WolfEntity(EntityId id)
    : TameableEntity(id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> WolfEntity::create(IWorld* /*world*/)
{
    // 使用临时ID 0，实际ID由 EntityManager 分配
    return std::make_unique<WolfEntity>(0);
}

bool WolfEntity::isTameItem(const ItemStack& itemStack) const
{
    // 狼用骨头驯服
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::BONE;
}

bool WolfEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 驯服后用肉类繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::PORKCHOP || item == Items::COOKED_PORKCHOP || item == Items::BEEF ||
        item == Items::COOKED_BEEF || item == Items::CHICKEN || item == Items::COOKED_CHICKEN ||
        item == Items::RABBIT || item == Items::COOKED_RABBIT || item == Items::MUTTON ||
        item == Items::COOKED_MUTTON || item == Items::ROTTEN_FLESH;
}

bool WolfEntity::isFoodItem(const ItemStack& itemStack) const
{
    // 同繁殖物品
    return isBreedingItem(itemStack);
}

bool WolfEntity::wantsToAttack(const LivingEntity& target, const LivingEntity* owner) const
{
    // 苦力怕、恶魂：永远不攻击（MC 使用 instanceof，此处使用 dynamic_cast）
    // 注：ArmorStandEntity 在本项目中继承自 Entity 而非 LivingEntity，
    // 因此不可能作为 LivingEntity 传入，无需检查
    if (dynamic_cast<const CreeperEntity*>(&target) != nullptr ||
        dynamic_cast<const GhastEntity*>(&target) != nullptr) {
        return false;
    }

    // 其他狼：只攻击未驯服的狼或主不同的狼
    const WolfEntity* otherWolf = dynamic_cast<const WolfEntity*>(&target);
    if (otherWolf != nullptr) {
        if (!otherWolf->isTamed()) {
            return true; // 未驯服的狼可以攻击
        }
        // 已驯服的狼：只有主不同时才攻击
        if (owner != nullptr) {
            return otherWolf->getOwner() != owner;
        }
        return false; // 没有主人，不攻击已驯服的狼
    }

    // TODO: 玩家PvP保护检查（需要实现 Player::canHarmPlayer）
    // 如果目标和主人都是玩家，需要检查 PvP 规则
    // if (dynamic_cast<const Player*>(&target) != nullptr && owner != nullptr) {
    //     const Player* ownerPlayer = dynamic_cast<const Player*>(owner);
    //     const Player* targetPlayer = dynamic_cast<const Player*>(&target);
    //     if (ownerPlayer != nullptr && targetPlayer != nullptr && !ownerPlayer->canHarmPlayer(*targetPlayer))
    //     {
    //         return false;
    //     }
    // }

    // 已驯服的马：不攻击
    const AbstractHorseEntity* horse = dynamic_cast<const AbstractHorseEntity*>(&target);
    if (horse != nullptr && horse->isTame()) {
        return false;
    }

    // 其他已驯服的驯服动物：不攻击
    const TameableEntity* tameable = dynamic_cast<const TameableEntity*>(&target);
    if (tameable != nullptr && tameable->isTamed()) {
        return false;
    }

    // 其他目标：允许攻击
    return true;
}

std::unique_ptr<AnimalEntity> WolfEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // 创建小狼
    auto baby = std::make_unique<WolfEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void WolfEntity::tick()
{
    TameableEntity::tick();

    if (!isAlive()) {
        return;
    }

    const f32 dx = x() - prevX();
    const f32 dz = z() - prevZ();
    const f32 horizontalDistance = std::sqrt(dx * dx + dz * dz);

    if (horizontalDistance > 0.0f) {
        m_stepSoundDistance += horizontalDistance * 0.6f;
        if (m_stepSoundDistance > m_nextStepSoundDistance && onGround() && !isInWater()) {
            m_nextStepSoundDistance = std::floor(m_stepSoundDistance) + 1.0f;
            const BlockPos stepPos(static_cast<i32>(std::floor(x())),
                static_cast<i32>(std::floor(y() - 0.2f)),
                static_cast<i32>(std::floor(z())));
            const BlockState* blockState = m_world != nullptr ? m_world->getBlockState(stepPos) : nullptr;
            playStepSound(stepPos, blockState);
        }
    }

    const bool inWater = isInWater();
    if (m_wasInWater && !inWater && onGround()) {
        playShakingSound();
    }
    m_wasInWater = inWater;
}

std::optional<ResourceLocation> WolfEntity::getAmbientSound() const
{
    math::Random random = getRandom();

    if (isAngry()) {
        return makeSoundEventId("growl");
    }

    if (random.nextInt(3) == 0) {
        if (isTamed() && health() < 10.0f) {
            return makeSoundEventId("whine");
        }

        return makeSoundEventId("pant");
    }

    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> WolfEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> WolfEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

void WolfEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    auto soundEvent = makeSoundEventId("step");
    if (!soundEvent.has_value()) {
        return;
    }

    playSound(*soundEvent, 0.15f, 1.0f);
}

void WolfEntity::playStepSound()
{
    const BlockPos stepPos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y() - 0.2f)), static_cast<i32>(std::floor(z())));
    const BlockState* blockState = m_world != nullptr ? m_world->getBlockState(stepPos) : nullptr;
    playStepSound(stepPos, blockState);
}

void WolfEntity::playShakingSound()
{
    auto soundEvent = makeSoundEventId("shake");
    if (!soundEvent.has_value()) {
        return;
    }

    math::Random random = getRandom();
    playSound(*soundEvent, getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
}

f32 WolfEntity::getTailAngle() const
{
    // 根据生命值计算尾巴角度
    if (isAngry()) {
        // 愤怒时尾巴竖起
        return 1.539f; // 约88度
    }

    // 根据生命值计算
    f32 healthRatio = health() / maxHealth();
    return TAIL_ANGLE_UNHEALTHY + (healthRatio * (TAIL_ANGLE_HEALTHY - TAIL_ANGLE_UNHEALTHY));
}

bool WolfEntity::isInWater() const
{
    // 调用父类实现检查是否在水中
    return TameableEntity::isInWater();
}

void WolfEntity::registerGoals()
{
    // 调用父类方法（已包含 SwimGoal, PanicGoal, BreedGoal, FollowParentGoal, RandomWalkingGoal, LookAtGoal,
    // LookRandomlyGoal）
    TameableEntity::registerGoals();

    // ========================================================================
    // 行为目标 (goalSelector)
    // ========================================================================

    // 优先级 1: 坐下目标（驯服后）- 与PanicGoal同优先级，但SitGoal会检查是否驯服
    m_goalSelector.addGoal(1, new entity::ai::goal::SitGoal(this));

    // 优先级 3: 未驯服时避开羊驼
    // 羊驼有强度属性，强度高的羊驼可以吓跑狼
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::AvoidEntityGoal>(
            this, 24.0f, 1.5, 1.5, [this](const LivingEntity* entity) -> bool {
                // 只在未驯服时避开羊驼
                if (isTamed()) return false;
                // 检查是否是羊驼
                if (entity->typeId() != entity::EntityTypeIdNumber::LLAMA &&
                    entity->typeId() != entity::EntityTypeIdNumber::TRADER_LLAMA) {
                    return false;
                }
                // 检查羊驼的强度
                const LlamaEntity* llama = dynamic_cast<const LlamaEntity*>(entity);
                if (!llama) return false;
                // 羊驼强度 >= 随机值(0-4) 时，狼会躲避
                // 强度1: 20%概率吓跑，强度4: 80%概率吓跑
                math::Random rng = getRandom();
                return llama->getStrength() >= rng.nextInt(5);
            }));

    // 优先级 4: 跳跃攻击
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::LeapAtTargetGoal>(this, 0.4f));

    // 优先级 5: 近战攻击
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, true));

    // 优先级 6: 跟随主人（驯服后）
    m_goalSelector.addGoal(6, new entity::ai::goal::FollowOwnerGoal(this, 1.0, 3.0f, 10.0f, 32.0f));

    // 优先级 9: 乞求目标（看向手持骨头或肉类的玩家）
    // 狼使用 BegGoal（乞求，只看不动），而非 TemptGoal（诱惑，会跟随玩家）
    // 这是因为未驯服的狼不会主动接近玩家，驯服后的狼已跟随主人，不需要 TemptGoal
    // [COMPLETED] 2026-05-15 - 骨头乞求行为已通过 BegGoal 实现
    m_goalSelector.addGoal(9, new entity::ai::goal::BegGoal(this, 8.0f));

    // ========================================================================
    // 目标选择器 (targetSelector)
    // ========================================================================

    // 优先级 1: 主人被攻击时反击
    // 当主人被攻击时，狼会攻击攻击者
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::OwnerHurtByTargetGoal>(this));

    // 优先级 2: 攻击主人正在攻击的目标
    // 当主人攻击某实体时，狼会协助攻击
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::OwnerHurtTargetGoal>(this));

    // 优先级 3: 被攻击后反击，并呼叫同伴
    // setCallsForHelp = true，召唤附近的狼一起攻击
    m_targetSelector.addGoal(3, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true));

    // 优先级 4: 愤怒时攻击玩家
    // 需要配合 IAngerable 接口，当玩家攻击狼后，狼会记住玩家并攻击
    // 当前简化实现：不注册此目标，因为狼默认不会主动攻击玩家
    // m_targetSelector.addGoal(4, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(
    //     this, true, 10, /* angerPredicate */));

    // 优先级 5: 未驯服时攻击羊、兔子、狐狸
    // TARGET_ENTITIES 谓词：羊、兔子、狐狸
    m_targetSelector.addGoal(5,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            true, // checkSight
            0,    // chance (每tick检查)
            [](const LivingEntity* entity) -> bool {
                if (!entity || !entity->isAlive()) return false;
                // 羊、兔子、狐狸
                auto type = entity->typeId();
                return type == entity::EntityTypeIdNumber::SHEEP || type == entity::EntityTypeIdNumber::RABBIT ||
                    type == entity::EntityTypeIdNumber::FOX;
            }));

    // 优先级 6: 未驯服时攻击幼海龟（不在水中）
    // 使用 NonTamedTargetGoal，只在未驯服时执行
    // TurtleEntity.TARGET_DRY_BABY 谓词：幼体且不在水中
    m_targetSelector.addGoal(6,
        std::make_unique<entity::ai::goal::NonTamedTargetGoal<TurtleEntity>>(this,
            true, // checkSight
            [](const LivingEntity* entity) -> bool {
                // TARGET_DRY_BABY: 幼体且不在水中
                const TurtleEntity* turtle = dynamic_cast<const TurtleEntity*>(entity);
                if (!turtle) return false;
                return turtle->isChild() && !turtle->isInWater();
            }));

    // 优先级 7: 攻击骷髅类怪物
    // 无论是否驯服，狼都会攻击骷髅类怪物
    m_targetSelector.addGoal(7,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            false, // checkSight - 不需要视线检查，骷髅是敌对生物
            0,     // chance
            [](const LivingEntity* entity) -> bool {
                if (!entity || !entity->isAlive()) return false;
                // 骷髅、流浪者、凋灵骷髅
                auto type = entity->typeId();
                return type == entity::EntityTypeIdNumber::SKELETON || type == entity::EntityTypeIdNumber::STRAY ||
                    type == entity::EntityTypeIdNumber::WITHER_SKELETON;
            }));

    // 注意：优先级 8 的 ResetAngerGoal 需要 IAngerable 接口完整实现
    // 当前简化处理，不注册此目标
}

void WolfEntity::registerAttributes()
{
    // 调用父类方法
    TameableEntity::registerAttributes();

    // 狼的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0); // 驯服前8血
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0); // 2点攻击力

    // 驯服后会增加到20血，由 onTamed 处理
}

void WolfEntity::onTamed(bool tamed)
{
    if (tamed) {
        // 驯服后增加生命值上限（从8血变为20血）
        m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        setHealth(20.0f);

        // 驯服后增加攻击力
        m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0);
    } else {
        // 放弃驯服后恢复
        m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);
        setHealth(8.0f);
        m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
    }
}

} // namespace mc
