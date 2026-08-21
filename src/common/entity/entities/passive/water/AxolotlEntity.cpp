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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "AxolotlEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/FindWaterGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/RandomSwimmingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/special/AxolotlGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/passive/water/WaterMobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/BucketableUtils.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

namespace mc {

AxolotlEntity::AxolotlEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : WaterMobEntity(id, registry)
{
    // 设置空气值（6000 tick = 5分钟）
    setAir(MAX_AIR_SUPPLY);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> AxolotlEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<AxolotlEntity>(0, registry);
}

void AxolotlEntity::randomizeVariant()
{
    math::Random& rng = getRandom();
    // 从四种普通变体中随机选择
    i32 variantIndex = rng.nextInt(0, 3);
    m_variant = static_cast<AxolotlVariant>(variantIndex);
}

void AxolotlEntity::setPlayingDead(bool playingDead)
{
    m_playingDead = playingDead;
    if (playingDead) {
        m_playingDeadTimer = PLAY_DEAD_DURATION;
    }
}

bool AxolotlEntity::isFoodItem(const ItemStack& itemStack) const
{
    // 美西螈的食物是热带鱼桶
    const Item* item = itemStack.getItem();
    return item == Items::TROPICAL_FISH_BUCKET;
}

bool AxolotlEntity::canBeSeenAsEnemy() const
{
    // 装死时不能被作为敌人看到
    return !m_playingDead;
}

bool AxolotlEntity::canDespawn(double distanceToClosestPlayer) const
{
    MC_UNUSED(distanceToClosestPlayer);
    // 来自桶或有自定义名称的美西螈不会消失
    return !m_fromBucket && !hasCustomName();
}

bool AxolotlEntity::preventDespawn() const
{
    return WaterMobEntity::preventDespawn() || m_fromBucket;
}

std::optional<ResourceLocation> AxolotlEntity::getAmbientSound() const
{
    if (isInWater()) {
        return SoundEvents::ENTITY_AXOLOTL_IDLE_WATER;
    }
    return SoundEvents::ENTITY_AXOLOTL_IDLE_AIR;
}

std::optional<ResourceLocation> AxolotlEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_AXOLOTL_HURT;
}

std::optional<ResourceLocation> AxolotlEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_AXOLOTL_DEATH;
}

void AxolotlEntity::playAttackSound(LivingEntity& /*target*/)
{
    playSound(SoundEvents::ENTITY_AXOLOTL_ATTACK, 1.0f, 1.0f);
}

void AxolotlEntity::applySupportingEffects(Player& player)
{
    // 给予再生I效果，持续时间 = 基础100tick + 现有剩余（上限2400tick）
    // 如果玩家当前没有再生效果，或剩余时间不超过2399tick，则刷新效果
    const auto* existing = player.getEffect(entity::effect::EffectType::Regeneration);
    if (existing == nullptr || existing->endsWithin(REGEN_BUFF_MAX_DURATION - 1)) {
        i32 currentDuration = existing != nullptr ? existing->duration() : 0;
        i32 newDuration = std::min(REGEN_BUFF_MAX_DURATION, REGEN_BUFF_BASE_DURATION + currentDuration);
        player.addEffect(entity::effect::EffectInstance(
            entity::effect::EffectType::Regeneration, newDuration, 0, false, true, true));
    }

    // 移除挖掘疲劳效果
    player.removeEffect(entity::effect::EffectType::MiningFatigue);
}

void AxolotlEntity::tick()
{
    WaterMobEntity::tick();

    // 更新装死状态
    _updatePlayingDead();

    // 更新狩猎冷却
    _updateHuntingCooldown();

    // 检查攻击目标是否刚死亡，触发支援效果
    _checkSupportingEffects();
}

void AxolotlEntity::registerGoals()
{
    // 美西螈 AI 目标优先级

    // 优先级 0: 水中浮起和寻找水源
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::FindWaterGoal>(this));

    // 优先级 1: 装死
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::AxolotlPlayDeadGoal>(this));

    // 优先级 2: 恐慌逃跑
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::PanicGoal>(this, 2.0));

    // 优先级 3: 跟随食物（热带鱼桶）
    // 注意：BreedGoal 和 FollowParentGoal 需要 AnimalEntity，美西螈继承自 WaterMobEntity
    // 繁殖通过水桶交互机制实现，而非 BreedGoal
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::TemptGoal>(
            this,
            1.0,
            [](const ItemStack& stack) -> bool { return stack.getItem() == Items::TROPICAL_FISH_BUCKET; },
            false));

    // 优先级 4: 近战攻击
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.5, true));

    // 优先级 5: 随机游泳
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::RandomSwimmingGoal>(this, 1.0, 40));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(
        6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器
    // 优先级 1: 被攻击后反击
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this));
    // 优先级 2: 攻击水生敌对生物和鱼类
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::AxolotlTargetGoal>(this));
}

void AxolotlEntity::registerAttributes()
{
    WaterMobEntity::registerAttributes();

    // 美西螈属性
    // 最大生命值: 14.0 (7颗心)
    // 移动速度: 1.0
    // 攻击伤害: 2.0 (1颗心)
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 14.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 1.0);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

bool AxolotlEntity::hurt(DamageSource& source, f32 amount)
{
    // 调用父类受伤处理
    bool result = WaterMobEntity::hurt(source, amount);

    if (result && isInWater() && !isPlayingDead()) {
        // 在水中受击时有概率触发装死
        // 条件：在水中 + 攻击者存在 + 33%概率 + 伤害不超过当前生命值
        f32 healthAmount = health();
        if (healthAmount > 0.0f && amount < healthAmount) {
            math::Random& rng = getRandom();
            if (rng.nextInt(3) == 0) {
                // 检查攻击者是否存在
                const Entity* attacker = source.getEntity();
                if (attacker != nullptr) {
                    setPlayingDead(true);
                }
            }
        }
    }

    return result;
}

void AxolotlEntity::_updatePlayingDead()
{
    if (m_playingDead && m_playingDeadTimer > 0) {
        m_playingDeadTimer--;
        if (m_playingDeadTimer <= 0) {
            m_playingDead = false;
        }
    }
}

void AxolotlEntity::_updateHuntingCooldown()
{
    if (m_huntingCooldown > 0) {
        m_huntingCooldown--;
    }
}

void AxolotlEntity::_checkSupportingEffects()
{
    LivingEntity* target = attackTarget();

    if (target != nullptr && m_wasTargetAlive && !target->isAlive()) {
        // 攻击目标刚死亡 - 检查最后一击是否由玩家造成
        DamageSource* lastDamage = target->lastDamageSource();
        if (lastDamage != nullptr) {
            Entity* attacker = lastDamage->getEntity();
            if (attacker != nullptr && attacker->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
                auto* player = static_cast<Player*>(attacker);
                // 检查该玩家是否在美西螈附近20格范围内
                auto* world = this->world();
                if (world != nullptr) {
                    std::vector<Entity*> nearbyEntities =
                        world->getEntitiesInRange(position(), static_cast<f32>(PLAYER_REGEN_DETECTION_RANGE));
                    bool playerNearby = false;
                    for (Entity* entity : nearbyEntities) {
                        if (entity == player) {
                            playerNearby = true;
                            break;
                        }
                    }
                    if (playerNearby) {
                        applySupportingEffects(*player);
                    }
                }
            }
        }

        // 进入狩猎冷却（2分钟）
        m_huntingCooldown = HUNTING_COOLDOWN_DURATION;
    }

    // 更新上一tick目标存活状态
    m_wasTargetAlive = (target != nullptr && target->isAlive());
}

// ========== IBucketable 接口实现（对齐 Java Axolotl implements Bucketable） ==========

ActionResultType AxolotlEntity::interactMob(Player& player, Hand hand)
{
    // 对齐 Java 1.21.11 Axolotl.mobInteract：
    //   return Bucketable.bucketMobPickup(player, hand, this)
    //       .orElse(super.mobInteract(player, hand));
    ActionResultType result = entity::bucketMobPickup(player, *this, hand);
    if (result != ActionResultType::Pass) {
        return result;
    }
    return WaterMobEntity::interactMob(player, hand);
}

ItemStack AxolotlEntity::getBucketItemStack() const
{
    // 对齐 Java Axolotl.getBucketItemStack() = new ItemStack(Items.AXOLOTL_BUCKET)。
    return ItemStack(Items::AXOLOTL_BUCKET, 1);
}

std::optional<ResourceLocation> AxolotlEntity::getPickupSound() const
{
    // 对齐 Java Axolotl.getPickupSound() = SoundEvents.BUCKET_EMPTY_AXOLOTL。
    // 注：vanilla 美西螈装取音效用 EMPTY_AXOLOTL（非 FILL_AXOLOTL），属原版既定行为。
    return SoundEvents::ITEM_BUCKET_EMPTY_AXOLOTL;
}

void AxolotlEntity::saveToBucketTag(ItemStack& bucketStack) const
{
    // 对齐 Java Axolotl.saveToBucketTag → Bucketable.saveDefaultDataToBucketTag +
    // Axolotl 额外保存 Variant/Age。
    // TODO: Cubium FishBucketItem._spawnFish 当前不读桶 NBT（直接创建新鱼），saveToBucketTag
    //       暂为空实现，待 FishBucketItem 支持桶 NBT 读取后补全变体/年龄保存恢复逻辑。
    (void)bucketStack;
}

} // namespace mc
