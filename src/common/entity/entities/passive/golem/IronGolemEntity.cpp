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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "IronGolemEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/IronGolemGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/passive/golem/GolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <algorithm>
#include <memory>
#include <optional>

namespace mc {

IronGolemEntity::IronGolemEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : GolemEntity(id, registry)
{
    // 铁傀儡可以走上1格高的方块
    setStepHeight(1.0f);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> IronGolemEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<IronGolemEntity>(0, registry);
}

void IronGolemEntity::tick()
{
    GolemEntity::tick();

    // 更新攻击动画
    if (m_attackTimer > 0) {
        m_attackTimer--;
        m_armsRaised = true;
        if (m_attackTimer <= 0) {
            m_armsRaised = false;
        }
    }

    // 更新持花状态
    if (m_holdRoseTick > 0) {
        m_holdRoseTick--;
        if (m_holdRoseTick <= 0) {
            // 持花结束
        }
    }
}

void IronGolemEntity::registerGoals()
{
    // 调用父类方法
    GolemEntity::registerGoals();

    // 优先级 0: 游泳目标
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 近战攻击目标
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, true));

    // 优先级 2: 向目标移动
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::MoveTowardsTargetGoal>(this, 0.9, 32.0f));

    // 优先级 5: 给村民/铜傀儡赠花
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::OfferFlowerGoal>(this));

    // 优先级 7: 看向玩家
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LookAtGoal>(this, 6.0f));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器
    // 优先级 1: 保卫村庄——扫描附近村民的攻击者并锁定。对齐 vanilla 1.21.11
    //   IronGolem.registerGoals:74 `targetSelector.addGoal(1, new DefendVillageTargetGoal(this))`。
    //   DefendVillageTargetGoal.shouldExecute 找 16 格内最近存活村民 → 取该村民
    //   getLastHurtBy() → isSuitableTarget 通过后写入 attackTarget。此前 goal 类已在
    //   IronGolemGoals 完整实现但 registerGoals 未激活（死代码），村庄内村民被攻击时
    //   铁傀儡不会自动锁敌。本注册激活保卫村庄链路。
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::DefendVillageTargetGoal>(this));

    // 优先级 2: 被攻击后反击
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, false));

    // 优先级 3: 攻击敌对生物
    // canAttackType 已在 TargetGoal::isSuitableTarget 中自动调用，排除苦力怕
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            true, // checkSight
            5,    // chance (每5tick检查一次)
            [](const LivingEntity* entity) -> bool {
                if (!entity || !entity->isAlive()) return false;
                // 攻击敌对生物（实现了 IMob 接口/是 MonsterEntity 子类）
                const MonsterEntity* monster = dynamic_cast<const MonsterEntity*>(entity);
                return monster != nullptr;
            }));
}

void IronGolemEntity::registerAttributes()
{
    // 调用父类方法
    GolemEntity::registerAttributes();

    // 铁傀儡的属性
    // ATTACK_DAMAGE 需要先注册（GolemEntity 继承链中未注册此属性，MonsterEntity 才注册）
    attributes().registerAttribute(*entity::attribute::Attributes::attackDamage());
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 100.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    attributes().setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 1.0);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE);
}

std::optional<ResourceLocation> IronGolemEntity::getAmbientSound() const
{
    // 铁傀儡无环境音，对齐原版 AbstractGolem.getAmbientSound 返回 null。
    // sounds.json 中无 entity.iron_golem.ambient，仅 attack/step/hurt/death/repair。
    return std::nullopt;
}

void IronGolemEntity::setHoldingRose(bool holding)
{
    if (holding) {
        m_holdRoseTick = 400; // 400 ticks = 20秒
        // 广播实体状态到客户端：开始持花
        if (m_world != nullptr) {
            m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::IronGolemHoldRose));
        }
    } else {
        m_holdRoseTick = 0;
        // 广播实体状态到客户端：停止持花
        if (m_world != nullptr) {
            m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::IronGolemStopRose));
        }
    }
}

bool IronGolemEntity::attackEntityAsMob(LivingEntity& target)
{
    // 设置攻击动画
    m_attackTimer = ATTACK_DURATION;
    m_armsRaised = true;

    // 广播攻击动画到客户端
    if (m_world != nullptr) {
        m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::IronGolemAttack));
    }

    // 计算伤害：随机化伤害值
    // 对应 MC 原版 IronGolem.doHurtTarget:
    //   float f = this.getAttackDamage();
    //   float f1 = (int)f > 0 ? f / 2.0F + this.random.nextInt((int)f) : f;
    // 注意：(int)f 是截断取整而非向上取整，但对于整数 ATTACK_DAMAGE=7.0 无差异
    f32 damage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE));

    math::Random& rng = getRandom();
    if (static_cast<i32>(damage) > 0) {
        damage = damage / 2.0f + static_cast<f32>(rng.nextInt(static_cast<i32>(damage)));
    }

    // 应用伤害
    EntityDamageSource damageSource = DamageSources::mobAttack(this);
    bool success = target.hurt(damageSource, damage);

    if (success) {
        // 铁傀儡击退：向上击飞，考虑目标击退抗性
        f64 knockbackResistance = target.getAttributeValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0);
        f64 knockbackMultiplier = std::max(0.0, 1.0 - knockbackResistance);
        target.addVelocity(0.0, 0.4 * knockbackMultiplier, 0.0);

        // 触发附魔后续效果（节肢杀手减速等）
        onAttackEntity(target);

        // 攻击者记录"我打了谁"（对齐 vanilla Mob.doHurtTarget:1356 setLastHurtMob）。
        // 本 override 自管攻击链不调基类 attackEntityAsMob，须显式补 setLastHurtTarget，
        // 供 OwnerHurtTargetGoal 消费（驯服动物帮主人攻击主人正在打的怪）。
        setLastHurtTarget(&target);
    }

    // 播放攻击声音（无论是否命中都播放）
    playSound(SoundEvents::ENTITY_IRON_GOLEM_ATTACK, 1.0f, 1.0f);

    return success;
}

void IronGolemEntity::playAttackSound(LivingEntity& /*target*/)
{
    // 攻击声音在 attackEntityAsMob 中已经播放（无论是否命中）
    // 此方法保留为空，避免基类和AI目标中重复播放
}

bool IronGolemEntity::canAttackType(const entity::EntityType& type) const
{
    // 玩家创建的铁傀儡不攻击玩家
    if (isPlayerCreated() && &type == entity::VanillaEntityTypeKeys::PLAYER) {
        return false;
    }

    // 铁傀儡不攻击苦力怕
    if (&type == entity::VanillaEntityTypeKeys::CREEPER) {
        return false;
    }

    // 其他情况由父类处理
    return MobEntity::canAttackType(type);
}

ActionResultType IronGolemEntity::interactMob(Player& player, Hand hand)
{
    // 对齐 Java 1.21.11 IronGolem.mobInteract(Player, InteractionHand)：
    //   ItemStack itemstack = player.getItemInHand(hand);
    //   if (!itemstack.is(Items.IRON_INGOT)) return PASS;
    //   float f = this.getHealth();
    //   this.heal(25.0F);
    //   if (this.getHealth() == f) return PASS;            // 已满血，治疗无效
    //   float f1 = 1.0F + (random.nextFloat() - random.nextFloat()) * 0.2F;
    //   this.playSound(IRON_GOLEM_REPAIR, 1.0F, f1);
    //   itemstack.consume(1, player);
    //   return SUCCESS;
    //
    // 此前 Cubium 铁傀儡无 interactMob override（基类 MobEntity::interactMob 返 Pass），
    // Player::interactOn 第3步 processInitialInteract→interactMob 返 Pass 后，第4步走
    // Item::itemInteractionForEntity——而 IronIngotItem 未 override itemInteractionForEntity，
    // 致铁锭右键铁傀儡完全不治疗（对齐缺陷）。此处补全实体侧治疗链路。
    ItemStack& heldItem = player.getHeldItem(hand);
    const Item* item = heldItem.getItem();
    if (item == nullptr || item != Items::IRON_INGOT) {
        // 非铁锭交由父类处理（基类默认 Pass）。
        return MobEntity::interactMob(player, hand);
    }

    // 铁锭治疗量（对齐 Java IRON_INGOT_HEAL_AMOUNT=25）。
    // 先记录治疗前的血量，heal 后若血量未变（已满血）则返回 Pass 不消耗。
    const f32 healthBefore = health();
    heal(IRON_INGOT_HEAL_AMOUNT);
    if (health() == healthBefore) {
        // 已满血，治疗无效，不消耗铁锭（对齐 Java getHealth()==f 返回 PASS）。
        return ActionResultType::Pass;
    }

    // 治疗生效：播放 IRON_GOLEM_REPAIR 音效，pitch=1.0±0.2（对齐 Java f1）。
    // playSound 在客户端/服务端均可，但 heal 必须服务端生效（heal 内部已处理）。
    if (!isSilent()) {
        math::Random& rng = getRandom();
        const f32 pitch = 1.0f + (rng.nextFloat() - rng.nextFloat()) * 0.2f;
        playSound(SoundEvents::ENTITY_IRON_GOLEM_REPAIR, 1.0f, pitch);
    }

    // 消耗 1 铁锭。Java 用 itemstack.consume(1, player)（创造模式由 consume 内部跳过）。
    // Cubium ItemStack 无 consume 接口，铁锭不可损坏（maxDamage=0），直接 shrink(1)；
    // 创造模式显式跳过消耗（对齐 Java consume 创造保护语义）。
    if (m_world != nullptr && !m_world->isClientSide() && !player.abilities().creativeMode) {
        heldItem.shrink(1);
    }

    return ActionResultType::Success;
}

} // namespace mc
