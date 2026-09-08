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

#include "common/entity/core/LivingEntity.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/CombatRules.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/damage/CombatTracker.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/ecs/components/ArrowStateComponent.hpp"
#include "common/entity/ecs/components/AttributeComponent.hpp"
#include "common/entity/ecs/components/EquipmentComponent.hpp"
#include "common/entity/ecs/components/HealthComponent.hpp"
#include "common/entity/ecs/components/HurtStateComponent.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/EquipmentSlotNames.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/enchantments/AllEnchantments.hpp"
#include "common/item/enchantment/enchantments/weapon/KnockbackEnchantment.hpp"
#include "common/item/items/armor/ElytraItem.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockSoundType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// 常量
// ============================================================================

namespace {
// 使用统一物理常量，避免重复定义
// 参考 physics::PhysicsConstants.hpp
using physics::DRAG_AIR;
using physics::DRAG_GROUND;
using physics::GRAVITY;
using physics::MOTION_THRESHOLD;
} // namespace

// ============================================================================
// 静态数据参数定义（通过 createKey 自动分配唯一 ID，避免跨类 ID 冲突）
// ============================================================================
// 字段集对齐 vanilla 1.21.11 LivingEntity.defineId 顺序（id 8..14）：
//   LIVING_ENTITY_FLAGS(8,Byte) / HEALTH(9,Float) / EFFECT_PARTICLES(10,Particles) /
//   EFFECT_AMBIENCE(11,Boolean) / ARROW_COUNT(12,Int) / STINGER_COUNT(13,Int) /
//   SLEEPING_POS(14,OptionalBlockPos)。
// 旧版 DATA_POTION_EFFECTS_PARAM（1.16.5 颜色 Int）已删，改由 EFFECT_PARTICLES
// （Particles，客户端按效果本地渲染）承载药水粒子同步语义。
entity::DataParameter<i8> LivingEntity::DATA_LIVING_FLAGS_PARAM = entity::EntityDataManager::createKey<i8>();
entity::DataParameter<f32> LivingEntity::DATA_HEALTH_PARAM = entity::EntityDataManager::createKey<f32>();
entity::DataParameter<entity::ParticlesValue> LivingEntity::DATA_EFFECT_PARTICLES_PARAM =
    entity::EntityDataManager::createKey<entity::ParticlesValue>();
entity::DataParameter<bool> LivingEntity::DATA_EFFECT_AMBIENCE_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<i32> LivingEntity::DATA_ARROW_COUNT_PARAM = entity::EntityDataManager::createKey<i32>();
entity::DataParameter<i32> LivingEntity::DATA_STINGER_COUNT_PARAM = entity::EntityDataManager::createKey<i32>();
entity::DataParameter<entity::OptionalBlockPosValue> LivingEntity::DATA_SLEEPING_POS_PARAM =
    entity::EntityDataManager::createKey<entity::OptionalBlockPosValue>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = Entity::classInfo()）
// ============================================================================
const entity::EntityClassInfo& LivingEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"LivingEntity", &Entity::classInfo()};
    return s_classInfo;
}

// ============================================================================
// 构造函数
// ============================================================================

LivingEntity::LivingEntity(EntityInstanceId id, IWorld* world, ecs::EntityRegistry& registry)
    : Entity(id, world, registry)
    , m_combatTracker(this)
{
    // 第二批：attach HurtStateComponent（仅 LivingEntity 持有，普通 Entity 不 attach）。
    // 第三批：续接 attach HealthComponent（health 同步真相源）+ EquipmentComponent（装备无同步单写）
    // + ArrowStateComponent（arrowCount/stingerCount 同步真相源）+ AttributeComponent
    // （unique_ptr<AttributeMap> 包裹，须在 registerAttributes 之前 attach，因后者经
    // attributes() getter 取组件填充默认属性）。
    // 基类 Entity 构造已建好 ecsEntity 并 attach 7 组件，此处续接 attach。
    m_entityContext->enttRegistry().emplace<ecs::HurtStateComponent>(m_entityContext->entity());
    m_entityContext->enttRegistry().emplace<ecs::HealthComponent>(m_entityContext->entity());
    m_entityContext->enttRegistry().emplace<ecs::EquipmentComponent>(m_entityContext->entity());
    m_entityContext->enttRegistry().emplace<ecs::ArrowStateComponent>(m_entityContext->entity());
    m_entityContext->enttRegistry().emplace<ecs::AttributeComponent>(m_entityContext->entity());

    // 构造函数中设置 stepHeight = 0.6F
    setStepHeight(physics::STEP_HEIGHT);

    // 注册属性
    // 注意：此处 registerAttributes() 因 C++ 基类构造期虚函数不派发到派生类（vtable
    // 此时仍是 LivingEntity 的），仅注册基类默认属性（MAX_HEALTH 默认 20.0）。派生类
    // 构造体须显式再次调用 registerAttributes() 设实体专属值（如 chicken=4.0/fox=10.0）。
    // 初始生命值同步至 maxHealth 在 tick() 首帧兜底执行（见 tick 开头 HealthComponent.m_healthSynced），
    // 因派生类 registerAttributes 时序晚于 LivingEntity 构造，此处 setHealth 无法拿到
    // 派生类 MAX_HEALTH。
    registerAttributes();
}

void LivingEntity::registerData()
{
    Entity::registerData();

    // 标记当前正在注册 LivingEntity 类的字段，使 registerParam 沿 LivingEntity
    // 继承链分配 id（续接 Entity 的 id 7 之后）。RAII 守卫：基类 registerData 已
    // 为 Entity 字段分配 id 并弹栈，此处压入 LivingEntity classInfo 分配本类字段。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册生物数据参数（顺序对齐 vanilla 1.21.11 LivingEntity.defineId：
    // LIVING_ENTITY_FLAGS(8)/HEALTH(9)/EFFECT_PARTICLES(10)/EFFECT_AMBIENCE(11)/
    // ARROW_COUNT(12)/STINGER_COUNT(13)/SLEEPING_POS(14)）。继承链分配器按此调用
    // 顺序连续分配 id 8..14，MobEntity 续接到 id 15。
    m_dataManager.registerParam(DATA_LIVING_FLAGS_PARAM, static_cast<i8>(0));
    // health 默认值 20.0f 仅用于参数注册；真正同步由 setHealth 驱动（组件真相源 + 镜像）。
    m_dataManager.registerParam(DATA_HEALTH_PARAM, 20.0f);
    m_dataManager.registerParam(DATA_EFFECT_PARTICLES_PARAM, entity::ParticlesValue{true}); // 空粒子列表
    m_dataManager.registerParam(DATA_EFFECT_AMBIENCE_PARAM, false);
    m_dataManager.registerParam(DATA_ARROW_COUNT_PARAM, static_cast<i32>(0));
    m_dataManager.registerParam(DATA_STINGER_COUNT_PARAM, static_cast<i32>(0));
    m_dataManager.registerParam(DATA_SLEEPING_POS_PARAM, entity::OptionalBlockPosValue{false, {}}); // 无睡眠位置
}

// ============================================================================
// 生命值
// ============================================================================

f32 LivingEntity::maxHealth() const
{
    return static_cast<f32>(attributes().getValue(entity::attribute::Attributes::MAX_HEALTH, 20.0));
}

void LivingEntity::setHealth(f32 health)
{
    f32 max = maxHealth();
    const f32 clamped = std::max(0.0f, std::min(health, max));
    // 组件为真相源，DATA_HEALTH_PARAM 退为同步镜像。
    if (auto* c = m_entityContext->tryGetComponent<ecs::HealthComponent>()) {
        c->m_health = clamped;
        // 任何显式 setHealth 都视为已完成首帧生命值同步：避免 tick() 首帧兜底
        // setHealth(maxHealth) 覆盖测试/业务在构造后手动设置的 health（如 setHealth(5)
        // 后 hurt 致死，兜底会把 health 重置回 maxHealth 致 isDying/deathTime 不更新）。
        c->m_healthSynced = true;
    }
    m_dataManager.set(DATA_HEALTH_PARAM, clamped);
}

void LivingEntity::setAbsorptionAmount(f32 amount)
{
    // 与 MC 原版一致：吸收值限制在 [0, maxAbsorption] 范围内
    const f32 maxAbsorption = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::MAX_ABSORPTION, 0.0));
    if (auto* c = m_entityContext->tryGetComponent<ecs::HurtStateComponent>()) {
        c->m_absorption = std::max(0.0f, std::min(amount, maxAbsorption));
    }
}

void LivingEntity::heal(f32 amount)
{
    if (amount > 0.0f && !isDead()) {
        setHealth(health() + amount);
    }
}

bool LivingEntity::hurt(DamageSource& source, f32 amount)
{
    // 1. 检查是否对伤害类型免疫
    if (isInvulnerableTo(source)) {
        return false;
    }

    // 1.1 死亡中实体不再接受伤害（对齐 vanilla LivingEntity.hurtServer:1160-1161：
    //     isInvulnerableTo 通过后、IS_FIRE 检查前的 isDeadOrDying() 门控）。
    //     vanilla isDeadOrDying() = getHealth() <= 0.0F（LivingEntity.java:1152-1154），
    //     等价 Cubium isDead()（health() <= 0.0f）。生命值已归零的实体（死亡动画/tickDeath
    //     移除前的 20 tick 窗口内）不再被 hurt 扣血、不再触发战斗追踪器/荆棘等后置链路。
    //     此前 Cubium 缺此门控，致 health<=0 的实体仍能被 hurt 推到更深负值、重复触发
    //     死亡伤害链路（die() 已调用一次后仍可能被二次伤害）。
    if (isDead()) {
        return false;
    }

    // 1.5 抗火药水免疫火焰伤害（对齐 vanilla LivingEntity.hurtServer:1162：
    //   p_376460_.is(DamageTypeTags.IS_FIRE) && this.hasEffect(MobEffects.FIRE_RESISTANCE) → return false）。
    // 持有 FireResistance 效果的实体对所有 IS_FIRE 伤害源免疫（in_fire/campfire/on_fire/lava/hot_floor/
    // fireball）。此前 Cubium 此检查完全缺失，抗火药水无法免疫火焰伤害（站熔岩/营火仍受伤）。
    // 注：火焰免疫实体（isImmuneToFire）已在 isInvulnerableTo 的 IS_FIRE 分支拦截，此处覆盖非火焰免疫
    // 但喝了抗火药水的实体（如玩家/猪）。放 isInvulnerableTo 后、无敌帧逻辑前，对齐 vanilla 顺序。
    if (source.is(DamageTypeTags::IS_FIRE()) && hasEffect(entity::effect::EffectType::FireResistance)) {
        return false;
    }

    // 2. 无敌帧逻辑（对齐 vanilla LivingEntity.hurtServer:1191-1206）。
    //   if (invulnerableTime > 10.0F && !source.is(BYPASSES_COOLDOWN)) {
    //       if (amount <= lastHurt) return false;          // 同额/更小伤害被吞
    //       actuallyHurt(amount - lastHurt); lastHurt = amount;  // 更大伤害承受差额
    //   } else {
    //       lastHurt = amount; invulnerableTime = 20; hurtTime = hurtDuration = maxHurtTime(10);
    //       actuallyHurt(amount);                            // 首次/冷却后伤害全额承受
    //   }
    //   BYPASSES_COOLDOWN 标签的成员走 else 分支（无视无敌帧冷却，全额承受并重置计时器）。
    //   1.21.11 vanilla 数据包中该标签为空，但守卫语义保留以支持数据包扩展。
    if (m_hurtResistantTime > 10 && !source.is(DamageTypeTags::BYPASSES_COOLDOWN())) {
        // 已经在无敌帧内，只承受差额伤害
        if (amount <= m_lastDamage) {
            return false; // 伤害不足
        }
        // 承受差额伤害
        actuallyHurt(source, amount - m_lastDamage);
        m_lastDamage = amount;
    } else {
        // 新的伤害，重置无敌帧
        m_lastDamage = amount;
        m_hurtResistantTime = MAX_HURT_RESISTANT_TIME;
        // LivingEntity.hurtServer：hurtTime = hurtDuration = maxHurtTime(10)。
        if (auto* c = m_entityContext->tryGetComponent<ecs::HurtStateComponent>()) {
            c->m_hurtTime = c->m_maxHurtTime;
        }
        actuallyHurt(source, amount);
    }

    // 2.5 记录最近攻击者（对齐 MC Java 1.21.11 LivingEntity.hurtServer:1208-1209 的
    //     resolveMobResponsibleForDamage + resolvePlayerResponsibleForDamage）。
    //     vanilla 这两个方法在 actuallyHurt 之后**无条件**执行（不受伤害是否被护甲/药水/吸收
    //     完全抵消影响）—— 即便 amount 归零，受害方仍记录"谁打了我"，供 HurtByTargetGoal/
    //     OwnerHurtByTargetGoal 反击链路消费。
    //     此前 Cubium 把等价逻辑放在 actuallyHurt 内 step 8（amount<=0 提前返回之后），致伤害被
    //     完全抵消（如钻石套减伤到 0、狼铠吸收、抗性药水减免）时攻击者不被记录、反击不触发，
    //     偏离 vanilla。WolfEntity::actuallyHurt 狼铠吸收分支直接 return 不调基类，此前同样丢失
    //     攻击者记录。现迁到 hurt() 的 actuallyHurt 调用之后（两无敌帧分支的公共出口），对齐
    //     vanilla 无条件语义。getTrueSource() 对应 vanilla getEntity()（causingEntity 真凶）。
    //     注：SquidEntity::hurt 在 WaterMobEntity::hurt 返回后立即读 getLastHurtBy()，resolve
    //     在 hurt 的 return true 之前执行，时序正确。
    //     NO_ANGER / NO_ANGER_FROM_WIND_CHARGE 门控详见块内注释（对齐 resolveMobResponsibleForDamage）。
    {
        Entity* trueSource = source.getTrueSource();
        if (trueSource != nullptr && trueSource != this) {
            LivingEntity* attacker = dynamic_cast<LivingEntity*>(trueSource);

            // 2.5a 记录最近攻击生物（lastHurtByMob）—— 对齐 resolveMobResponsibleForDamage
            //     (LivingEntity.java:1326-1331)：getEntity() instanceof LivingEntity
            //     && !source.is(NO_ANGER) && (!source.is(WIND_CHARGE) || !this.getType().is(NO_ANGER_FROM_WIND_CHARGE))
            //     才 setLastHurtByMob。
            //     - NO_ANGER = {mob_attack_no_aggro}（DamageTypeTags.cpp:644）：铁傀儡等生物的
            //       mob_attack_no_aggro 攻击设计为不激怒目标，故不记录 lastHurtByMob。
            //     - NO_ANGER_FROM_WIND_CHARGE = {breeze,skeleton,bogged,stray,zombie,husk,spider,
            //       cave_spider,slime}（EntityTypeTags.cpp:724）：风弹（WindBurst=minecraft:wind_charge）
            //       击中这些生物时不激怒（vanilla 设计：风弹不应打扰这些生物的仇恨）。
            //     注：vanilla 用 source.is(DamageTypes.WIND_CHARGE) 判定风弹（单伤害类型非标签），
            //     Cubium 无 is(DamageType) 单类型查询，用 source.type()==DamageType::WindBurst 等价
            //     （DamageType::WindBurst 即 minecraft:wind_charge，DamageTypeTag.cpp:150）。
            const bool isWindCharge = (source.type() == DamageType::WindBurst);
            const bool shouldAnger = !source.is(DamageTypeTags::NO_ANGER()) &&
                (!isWindCharge || !EntityTypeTags::NO_ANGER_FROM_WIND_CHARGE().contains(getTypeId()));
            if (attacker != nullptr && shouldAnger) {
                setLastHurtBy(attacker);
            }

            // 2.5b 记录最近攻击玩家（lastHurtByPlayer，100 tick 记忆窗口）—— 对齐
            //     resolvePlayerResponsibleForDamage (LivingEntity.java:1334-1348)：
            //     getEntity() instanceof Player 即 setLastHurtByPlayer(player, 100)，无 NO_ANGER 门控。
            //     即 mob_attack_no_aggro 由玩家造成时仍记 lastHurtByPlayer（用于死亡经验掉落守卫，
            //     见 shouldDropExperienceOnDeath / dropAllDeathLoot），但不激怒（2.5a 的 lastHurtByMob
            //     被 NO_ANGER 门控挡住）。
            //     TODO: 驯服狼代攻时归属其玩家主人（vanilla wolf.getOwnerReference 分支）未实现。
            if (dynamic_cast<Player*>(trueSource) != nullptr) {
                setLastHurtByPlayerMemoryTime(100);
            }
        }
    }

    // 3. 标记受伤（用于速度同步到客户端和AI目标检测）
    // 对应 MC Java LivingEntity.hurtServer:1218-1220：
    //   if (!source.is(DamageTypeTags.NO_IMPACT) && (!flag || amount > 0.0F)) markHurt();
    // flag 为盾牌格挡标志，Cubium hurt 入口无 flag 概念（盾牌在 canBlockDamageSource/actuallyHurt
    // 内处理），此处简化为 !source.is(NO_IMPACT)。NO_IMPACT = {Drown}（DamageTypeTags.cpp:649）：
    // 溺水伤害不触发受击标记（不产生受击动画/速度同步），对齐 vanilla——溺水是渐进缺氧，无受击反馈。
    // 此前 Cubium 无条件 markHurt，溺水也触发受击标记，偏离 vanilla。
    if (!source.is(DamageTypeTags::NO_IMPACT())) {
        markHurt();
    }

    // 4. 通用击退 + 受伤倾斜（对齐 vanilla LivingEntity.hurtServer:1222-1238 的 NO_KNOCKBACK 分支）。
    // vanilla：if (!source.is(NO_KNOCKBACK)) {
    //   if (directEntity instanceof Projectile) { d0/d1 = -projectile.calculateHorizontalHurtKnockbackDirection(); }
    //   else if (sourcePosition != null) { d0 = srcPos.x - x; d1 = srcPos.z - z; }
    //   knockback(0.4F, d0, d1);     // 对受害者施加 0.4 强度击退
    //   if (!flag) indicateDamage(d0, d1);  // flag=格挡标志，格挡时不 indicateDamage
    // }
    // NO_KNOCKBACK = {Explosion, InFire, OnFire, Lava, Fall, Magic, Drown, ...}（DamageTypeTags.cpp:654，
    // 近 30 类型）：这些伤害不产生击退与受击倾斜。
    // 此前 Cubium 仅对齐 indicateDamage 的 NO_KNOCKBACK 门控，通用 knockback(0.4) 完全缺失（原 TODO:287），
    // 致玩家近战无附魔攻击零击退（causeExtraKnockback 无附魔时 strength=0 跳过）、mob 攻击及其他非特化
    // 路径伤害无击退，偏离 vanilla。现补 applyKnockback(0.4, d0, d1) 对齐 vanilla 通用击退。
    // Projectile 方向：vanilla calculateHorizontalHurtKnockbackDirection 返回投射物 deltaMovement 的 x/z，
    //   hurtServer 取反（d0=-vx, d1=-vz），即击退方向 = 投射物飞行方向（投射物从A飞向victim，victim被
    //   推向A的反方向=继续远离投射物来源）。Cubium ProjectileEntity 经 directSource() 取 velocity 对齐。
    // TODO: flag（盾牌格挡）门控——vanilla 格挡时仍 knockback 但不 indicateDamage。Cubium hurt 入口无 flag
    //   概念（格挡在 actuallyHurt 内 canBlockDamageSource 命中后 return），此处无法区分格挡，简化为总是
    //   indicateDamage。格挡时 indicateDamage 的视觉偏差待接入 BlocksAttacks flag 后修正。
    if (m_world != nullptr && amount > 0.0f && !source.is(DamageTypeTags::NO_KNOCKBACK())) {
        f64 d0 = 0.0;
        f64 d1 = 0.0;
        Entity* directEntity = source.directSource();
        if (source.isProjectile() && directEntity != nullptr) {
            // Projectile：用投射物速度方向取反（对齐 vanilla calculateHorizontalHurtKnockbackDirection 取反）。
            const Vector3 projVel = directEntity->velocity();
            d0 = -static_cast<f64>(projVel.x);
            d1 = -static_cast<f64>(projVel.z);
        } else {
            const auto sourcePos = source.sourcePosition();
            if (sourcePos.has_value()) {
                d0 = static_cast<f64>(sourcePos->x) - static_cast<f64>(x());
                d1 = static_cast<f64>(sourcePos->z) - static_cast<f64>(z());
            }
        }

        // 通用击退（对受害者，强度 0.4，受 KNOCKBACK_RESISTANCE 减免）。
        applyKnockback(0.4f, d0, d1);
        indicateDamage(d0, d1);
    }

    return true;
}

void LivingEntity::indicateDamage(f64 d0, f64 d1)
{
    // 基类仅本地设置 hurtDir；网络广播由 ServerPlayer 重写负责。
    // hurtDir = atan2(d1, d0) * RAD_TO_DEG - yaw。
    m_hurtDir = static_cast<f32>(std::atan2(d1, d0) * static_cast<f64>(math::RAD_TO_DEG) - static_cast<f64>(yaw()));
}

bool LivingEntity::isScoping() const
{
    // 对应 MC LivingEntity.isScoping：使用中的物品是望远镜。
    // 项目无独立 Spyglass 物品指针，按 UseAction::Spyglass 判定。
    if (!isUsingItem() || m_activeItem.isEmpty()) {
        return false;
    }
    const Item* item = m_activeItem.getItem();
    return item != nullptr && item->getUseAction(m_activeItem) == UseAction::Spyglass;
}

void LivingEntity::actuallyHurt(DamageSource& source, f32 amount)
{
    if (amount <= 0.0f) {
        return;
    }

    // 生命值组件局部指针复用（避免热路径多次 try_get）
    auto* healthState = m_entityContext->tryGetComponent<ecs::HealthComponent>();

    // 1. 盾牌格挡检查（子类可重写）
    if (canBlockDamageSource(source)) {
        damageShield(amount);

        // 破盾判定（对齐 MC Java 1.21.11 Player.blockUsingItem:727-730 破盾分支）：
        // 受害者举盾格挡时，取直接来源（攻击者）的 getSecondsToDisableBlocking，若 >0 则受害者
        // 盾牌被禁用 round(seconds*20) tick（斧头 5.0 秒 = 100 tick）。破盾独立于 IS_PROJECTILE
        // 门控——投射物攻击者（箭矢等）getSecondsToDisableBlocking 返回 0 天然不破盾，故此处
        // 不需 IS_PROJECTILE 门控（对齐 vanilla blockUsingItem 内破盾与 blockedByItem 回调分离）。
        // onShieldDisabled 基类空实现，Player 重写执行 setItemCooldown + stopActiveHand + 破盾音效。
        Entity* directEntityForDisable = source.directSource();
        if (directEntityForDisable != nullptr && directEntityForDisable != this) {
            LivingEntity* disableAttacker = dynamic_cast<LivingEntity*>(directEntityForDisable);
            if (disableAttacker != nullptr) {
                onShieldDisabled(*disableAttacker);
            }
        }

        // 格挡成功时回调攻击者（对齐 MC Java 1.21.11 LivingEntity.applyItemBlocking →
        // blockUsingItem → attacker.blockedByItem(victim)）。vanilla 条件（LivingEntity.java:1306）：
        //   f > 0.0F && !source.is(IS_PROJECTILE) && directEntity instanceof LivingEntity
        // 即仅近战等直接来源（directSource 是 LivingEntity 且非 IS_PROJECTILE 投射物）才回调,
        // 让攻击者执行"被格挡"特殊行为（如 Ravager 50% 眩晕→咆哮）。投射物格挡不回调攻击者。
        // 此前用 getTrueSource()（射击者）且缺 !source.is(IS_PROJECTILE) 门控，致箭矢格挡错误回调
        // 射击者。改为 directSource()（直接来源：近战=攻击者，箭=箭矢）+ IS_PROJECTILE 门控对齐 vanilla。
        // 注：IS_PROJECTILE 成员={Arrow,Trident,MobProjectile,Fireball,WitherSkull,Thrown,WindBurst}。
        if (!source.is(DamageTypeTags::IS_PROJECTILE())) {
            Entity* directEntity = source.directSource();
            if (directEntity != nullptr && directEntity != this) {
                LivingEntity* attacker = dynamic_cast<LivingEntity*>(directEntity);
                if (attacker != nullptr) {
                    attacker->blockedByItem(*this);
                }
            }
        }
        return; // 格挡成功，不造成伤害
    }

    // 1.5 冰冻额外伤害：冻结额外伤害标签中的实体（烈焰人、岩浆怪、炽足兽）受到5倍冰冻伤害
    if (source.isFreezing() && EntityTypeTags::FREEZE_HURTS_EXTRA_TYPES().contains(getTypeId())) {
        amount *= 5.0f;
    }

    // 1.6 头盔减伤（对齐 vanilla LivingEntity.hurtServer:1182）：DAMAGES_HELMET 标签伤害
    // （坠落铁砧/坠落方块/坠落钟乳石）命中戴头盔的实体时，伤害 ×0.75（减免 1/4），并回调
    // hurtHelmet 消耗头盔耐久。此分支独立于护甲减伤（在护甲减伤之前，无 bypassesArmor 门控），
    // 对齐 vanilla 位置（hurtServer 主流程，getDamageAfterArmorAbsorb 之前）。
    // 注：vanilla LivingEntity.hurtHelmet 基类为空实现（耐久消耗由 doHurtEquipment 统一处理），
    // Cubium hurtHelmet 基类同样空实现，耐久消耗留待装备耐久体系完善后补全（TODO）。
    if (source.is(DamageTypeTags::DAMAGES_HELMET()) && !getEquipment(EquipmentSlot::Head).isEmpty()) {
        hurtHelmet(source, amount);
        amount *= 0.75f;
    }

    // 2. 护甲减伤（如果伤害不绕过护甲）
    if (!source.bypassesArmor()) {
        amount = applyArmorCalculations(source, amount);
        damageArmor(source, amount);
    }

    // 3. 药水效果和附魔保护减伤
    amount = applyPotionDamageCalculations(source, amount);

    // 4. 吸收值处理（金苹果额外生命）
    auto* hurtState = m_entityContext->tryGetComponent<ecs::HurtStateComponent>();
    if (hurtState != nullptr && hurtState->m_absorption > 0.0f) {
        const f32 absorbed = std::min(hurtState->m_absorption, amount);
        setAbsorptionAmount(hurtState->m_absorption - absorbed);
        amount -= absorbed;
    }

    if (amount <= 0.0f) {
        return; // 伤害被完全吸收
    }

    // 5. 实际扣血
    if (healthState != nullptr) {
        healthState->m_health -= amount;
        healthState->m_lastHealth = healthState->m_health;
    }

    // 6. 记录到战斗追踪器
    m_combatTracker.trackDamage(source, health(), amount);

    // 7. 记录伤害来源
    m_lastDamageSource = source.clone();
    // 同步捕获真凶 id 与时间戳（任务 #272 UAF 根治）：m_lastDamageSource clone 持真凶裸 Entity*
    // 指针，真凶析构后 getTrueSource() 悬垂。此处同步上下文（hurt 调用栈内）真凶必活，安全取 id，
    // 供 HurtBySensor 等经 IWorld::getEntity(id) 安全校验绕开悬垂指针。m_lastDamageStamp 对齐
    // vanilla lastDamageStamp（LivingEntity.java:257），配合 lastDamageSource() 的 40 tick 过期守卫。
    // 注意：resolve（记录最近攻击者 setLastHurtBy/setLastHurtByPlayerMemoryTime）已在 hurt() 的
    // actuallyHurt 调用之后无条件执行（对齐 vanilla hurtServer:1208-1209），不在此处。此处仅
    // 捕获真凶 id 供 UAF 安全校验，与 resolve 的 NO_ANGER 门控语义无关。
    {
        Entity* trueSourceForId = source.getTrueSource();
        m_lastDamageSourceTrueId = (trueSourceForId != nullptr) ? trueSourceForId->id() : INVALID_ENTITY_ID;
    }
    m_lastDamageStamp = ticksExisted();

    // 8. 记录最近攻击者的逻辑（setLastHurtBy / setLastHurtByPlayerMemoryTime）已迁移到 hurt()
    //    的 actuallyHurt 调用之后无条件执行（对齐 vanilla resolveMob/PlayerResponsibleForDamage
    //    在 hurtServer 内 actuallyHurt 之后无条件调用）。此处保留 trueSource 取值供第 9/11 步使用。
    Entity* trueSource = source.getTrueSource();

    // 9. 触发荆棘附魔（对攻击者造成反伤）
    // 注意：荆棘伤害不触发无限循环，因为荆棘伤害的 isThornsDamage() 返回 true
    if (!source.isThornsDamage() && trueSource != nullptr && trueSource != this) {
        // 调用荆棘附魔回调（内部按 [Head,Chest,Legs,Feet] 遍历护甲，触发反伤 + 耐久消耗）
        item::enchant::EnchantmentHelper::applyThornsEnchantments(*this, *trueSource);
    }

    // 10. 更新战斗状态
    if (!m_inCombat) {
        m_inCombat = true;
        m_lastDamageTimestamp = ticksExisted();
        sendEnterCombat();
    }

    // 11. 死亡检查
    if (health() <= 0.0f) {
        playDeathSound();
        die(source);
    } else {
        playHurtSound(source);
    }
}

bool LivingEntity::canBlockDamageSource(DamageSource& /*source*/) const
{
    // 默认返回 false，由 Player 子类重写实现盾牌格挡
    return false;
}

void LivingEntity::damageArmor(DamageSource& /*source*/, f32 /*amount*/)
{
    // 默认空实现，由 Player 子类重写
}

void LivingEntity::damageShield(f32 /*amount*/)
{
    // 默认空实现，由 Player 子类重写
}

void LivingEntity::hurtHelmet(DamageSource& /*source*/, f32 /*amount*/)
{
    // 对齐 vanilla LivingEntity.hurtHelmet:1793 基类空实现。DAMAGES_HELMET 伤害（坠落铁砧/方块/
    // 钟乳石）命中头盔时由 actuallyHurt 的 DAMAGES_HELMET 分支调用，基类不消耗耐久。
    // TODO: 装备耐久体系完善后补全头盔耐久消耗（对齐 vanilla doHurtEquipment 统一耐久逻辑）。
}

void LivingEntity::blockedByItem(LivingEntity& victim)
{
    // 对齐 MC Java 1.21.11 LivingEntity.blockedByItem 默认实现：
    //   protected void blockedByItem(LivingEntity p_21246_) {
    //       p_21246_.knockback(0.5, p_21246_.getX() - this.getX(), p_21246_.getZ() - this.getZ());
    //   }
    // 即攻击者（this）被受害者（victim）格挡后，攻击者受到一次小幅击退（强度 0.5），
    // 方向为从攻击者指向受害者的反方向（被推开）。子类（如 RavagerEntity）重写以实现
    // 眩晕→咆哮等特殊行为。
    // 注：本基类实现用于普通生物攻击被格挡时的通用击退；玩家攻击被格挡时也走此路径
    // （玩家作为攻击者 this，victim 是举盾格挡的目标）。
    f64 dx = static_cast<f64>(victim.x()) - static_cast<f64>(x());
    f64 dz = static_cast<f64>(victim.z()) - static_cast<f64>(z());
    applyKnockback(0.5f, dx, dz);
}

f32 LivingEntity::applyArmorCalculations(DamageSource& source, f32 damage)
{
    if (source.bypassesArmor()) {
        return damage;
    }

    const f32 armor = static_cast<f32>(attributes().getValue(entity::attribute::Attributes::ARMOR, 0.0));
    const f32 toughness = static_cast<f32>(attributes().getValue(entity::attribute::Attributes::ARMOR_TOUGHNESS, 0.0));

    // 破甲（Breach）附魔修正：从攻击者武器查等级（对齐 vanilla CombatRules.getDamageAfterArmor
    // 第 20-26 行从 DamageSource.getWeaponItem()=directEntity.getWeaponItem() 取武器）。
    // vanilla directEntity 对玩家近战即攻击者本身；Cubium EntityDamageSource::directSource() 同。
    // 投射物非 LivingEntity，dynamic_cast 返回 nullptr 跳过（箭矢无 Breach，正确）。
    i32 breachLevel = 0;
    if (Entity* directAttacker = source.directSource()) {
        if (LivingEntity* livingAttacker = dynamic_cast<LivingEntity*>(directAttacker)) {
            const ItemStack& weapon = livingAttacker->getMainHandItem();
            if (!weapon.isEmpty()) {
                breachLevel = item::enchant::EnchantmentHelper::getBreachLevel(weapon);
            }
        }
    }

    return entity::combat::CombatRules::getDamageAfterAbsorb(damage, armor, toughness, breachLevel);
}

f32 LivingEntity::applyPotionDamageCalculations(DamageSource& source, f32 damage)
{
    if (damage <= 0.0f) {
        return damage;
    }

    // 0. BYPASSES_EFFECTS 伤害（成员={Starve}）跳过抗性药水与附魔保护减伤，直接返回原值
    // （对齐 vanilla LivingEntity.getDamageAfterMagicAbsorb:1822-1823）。饥饿伤害不应被抗性
    // 药水减免。此前抗性门控用 !bypassesInvulnerability()（=OutOfWorld+GenericKill）漏 Starve，
    // 致抗性药水错误减免饥饿伤害。
    if (source.is(DamageTypeTags::BYPASSES_EFFECTS())) {
        return damage;
    }

    // 1. 抗性药水减伤
    // BYPASSES_RESISTANCE 伤害源（成员={OutOfWorld, GenericKill}）不受抗性药水影响
    // （LivingEntity.getDamageAfterMagicAbsorb:1825：hasEffect(RESISTANCE) && !is(BYPASSES_RESISTANCE)）。
    // 此前用 !bypassesInvulnerability()（=BYPASSES_INVULNERABILITY）门控，两者成员当前恰好相同
    // 故行为暂对但语义错位——一旦数据包扩展任一标签即偏离。改查 BYPASSES_RESISTANCE 对齐标签语义。
    if (!source.is(DamageTypeTags::BYPASSES_RESISTANCE())) {
        const i32 resistanceLevel = getEffectLevel(entity::effect::EffectType::Resistance);
        if (resistanceLevel > 0) {
            damage = entity::combat::CombatRules::getDamageAfterResistance(damage, resistanceLevel);
        }
    }

    // 2. 附魔保护减伤
    // 遍历所有护甲槽位，计算保护附魔的 EPF 总和
    // 只有非 isDamageAbsolute 的伤害才受附魔保护影响
    // BYPASSES_ENCHANTMENTS 伤害源（sonic_boom，监守者音爆）跳过附魔保护减伤（对齐 vanilla
    // LivingEntity.getDamageAfterMagicAbsorb:1843：BYPASSES_ENCHANTMENTS 直接返回，抗性药水已在前一步生效）。
    // vanilla 中监守者音爆设计为无视护甲和附魔保护（但仍受抗性药水减免）。
    if (!source.isDamageAbsolute() && !source.is(DamageTypeTags::BYPASSES_ENCHANTMENTS())) {
        u32 damageTypeFlags = 0;
        if (source.isFire()) damageTypeFlags |= DamageFlags::FIRE;
        if (source.isFall()) damageTypeFlags |= DamageFlags::FALL;
        if (source.isExplosion()) damageTypeFlags |= DamageFlags::EXPLOSION;
        if (source.isProjectile()) damageTypeFlags |= DamageFlags::PROJECTILE;
        // 其他类型由全保护附魔处理 (ProtectionEnchantment::Type::All)

        auto armorSlots = getArmorSlots();
        i32 protectionEPF = item::enchant::EnchantmentHelper::getTotalArmorProtection(armorSlots, damageTypeFlags);
        if (protectionEPF > 0) {
            damage = entity::combat::CombatRules::getDamageAfterMagicAbsorb(damage, static_cast<f32>(protectionEPF));
        }
    }

    return damage;
}

f32 LivingEntity::computeFinalDamage(DamageSource& source, f32 damage)
{
    // 计算所有减伤后的最终伤害
    if (damage <= 0.0f || source.bypassesInvulnerability()) {
        return damage;
    }

    // 护甲减伤
    if (!source.bypassesArmor()) {
        damage = applyArmorCalculations(source, damage);
    }

    // 药水和附魔减伤
    damage = applyPotionDamageCalculations(source, damage);

    return damage;
}

LivingEntity* LivingEntity::getKillCredit()
{
    // 对齐 MC Java 1.21.11 LivingEntity.getKillCredit（LivingEntity.java:1886-1892）：
    //   if (this.lastHurtByPlayer != null) return this.lastHurtByPlayer.getEntity(...);
    //   else return this.lastHurtByMob != null ? ... : null;
    // Cubium 没有 lastHurtByPlayer 实体引用，改用 CombatTracker::getBestAttacker()——
    // 这与原版 ServerPlayer.die 中 awardKillScore 的取值来源一致。
    return m_combatTracker.getBestAttackerLiving();
}

void LivingEntity::die(DamageSource& cause)
{
    if (!isDead()) {
        return; // 已经死亡，避免重复执行（等价 vanilla !this.dead 守卫）
    }

    // 重置服务端死亡计时器，tickDeath 据此递增驱动 20 tick 死亡动画。
    if (auto* c = m_entityContext->tryGetComponent<ecs::HurtStateComponent>()) {
        c->m_deathTime = 0;
    }

    // 停用所有位置依赖的附魔效果（如灵魂疾行的速度修饰符）
    // 避免实体死亡后属性修饰符残留
    item::enchant::EnchantmentHelper::stopAllLocationBasedEffects(*this);

    // 对齐 MC Java 1.21.11 LivingEntity.die（LivingEntity.java:1430-1461）。
    // vanilla 步骤：getKillCredit→awardKillScore、stopSleeping、stopUsingItem、
    // 命名实体死亡日志、dead=true、getCombatTracker().recheckStatus()、
    // killedEntity 守卫内 gameEvent(ENTITY_DIE)+dropAllDeathLoot+createWitherRose、
    // broadcastEntityState((byte)3)、setPose(DYING)。
    // 以下按原版顺序补全，Cubium 暂未实现的部分加 TODO 标注，不阻塞主死亡链路。

    // 1. getKillCredit → awardKillScore（LivingEntity.java:1433-1435）
    LivingEntity* killCredit = getKillCredit();
    if (killCredit != nullptr) {
        killCredit->awardKillScore(*this, cause);
    }

    // 2. stopSleeping（LivingEntity.java:1438-1439）
    // TODO: isSleeping()/stopSleeping() 在 Cubium 中仅 Player 子类实现，
    //       LivingEntity 基类无此方法。待 LivingEntity 引入睡眠状态后补此调用。

    // 3. stopUsingItem（LivingEntity.java:1442）
    //    Cubium 等价方法为 stopActiveHand()。
    stopActiveHand();

    // 4. 命名实体死亡日志（LivingEntity.java:1443-1445）
    if (m_world != nullptr && !m_world->isClientSide() && hasCustomName()) {
        // TODO: 接入 spdlog 输出 "Named entity {} died: {}"
    }

    // 5. dead = true（LivingEntity.java:1447）
    //    Cubium 以 m_removed 语义承载，isDead() 由 health<=0 判定，此处无需额外标记。

    // 6. getCombatTracker().recheckStatus()（LivingEntity.java:1448）
    //    对齐 vanilla：死亡时重新检查战斗状态，结束战斗并清空过期条目。
    m_combatTracker.recheckStatus();

    // 7. killedEntity 守卫内：gameEvent(ENTITY_DIE) + dropAllDeathLoot + createWitherRose
    //    （LivingEntity.java:1449-1454）
    if (m_world != nullptr) {
        // gameEvent(ENTITY_DIE)（LivingEntity.java:1451）
        // TODO: Cubium 的 gameEvent 便捷方法尚未在 LivingEntity 引入，待补 gameEvent(event) 成员后替换为便捷调用。
        m_world->gameEvent(gameevent::GameEvents::ENTITY_DIE, onPos(), gameevent::GameEvent::Context::of(this));

        // dropAllDeathLoot（LivingEntity.java:1452）
        // dropAllDeathLoot 内部按 shouldDropLoot(doMobLoot 守卫) 决定物品是否掉落，
        // 经验/装备在守卫之外（vanilla 同样在守卫外）。
        dropAllDeathLoot(cause);

        // createWitherRose（LivingEntity.java:1453）
        // 对齐 vanilla：若击杀者为凋灵，在死亡位置生成凋零玫瑰方块或掉落凋零玫瑰物品。
        createWitherRose(killCredit);

        // 8. broadcastEntityState((byte)3)（LivingEntity.java:1456）
        //    服务端广播 EntityEvent(Death=3)，客户端收到后启动死亡倒地动画。
        m_world->broadcastEntityStatus(m_id, static_cast<u8>(network::EntityStatus::Death));

        // 9. 通知世界实体死亡（触发进度检测 / 事件发布）。
        //    参考 MC: LivingEntity.die 末尾由子类/世界触发 CriteriaTriggers。
        //    killer 取 CombatTracker::getBestAttacker()（最佳伤害条目的攻击者）。
        Entity* killer = m_combatTracker.getBestAttacker();
        m_world->onEntityDeath(this, killer, &cause);
    }

    // 9. setPose(Pose.DYING)（LivingEntity.java:1459）
    setPose(EntityPose::Dying);
}

void LivingEntity::createWitherRose(LivingEntity* killCredit)
{
    // 对齐 MC Java 1.21.11 LivingEntity.createWitherRose（LivingEntity.java:1463-1482）。
    // 仅当击杀者为凋灵（WitherBoss）时触发凋零玫瑰生成逻辑。
    //  - 若 MOB_GRIEFING 游戏规则开启，且死亡位置方块为空气且凋零玫瑰可存活，
    //    则在该位置放置凋零玫瑰方块（flags=3）。
    //  - 否则（未放置方块，即 MOB_GRIEFING 关闭或位置不满足条件），
    //    在死亡位置掉落一个凋零玫瑰物品实体。
    //
    // 注意：Cubium 的 WitherRoseBlock 未重写 isValidPosition（对应原版 canSurvive），
    //       默认实现恒返回 true，因此 canSurvive 检查在此处不会产生过滤效果。
    //       这与原版凋零玫瑰需要在特定方块（灵魂沙/草方块等）上才能存活的语义存在偏差。
    // TODO: 待 WitherRoseBlock 实现 canSurvive 重写后，此处的 isValidPosition 检查将生效。

    // 原版 createWitherRose 无 m_world 守卫（仅判定 level instanceof ServerLevel），
    // Cubium 用成员 m_world，入口处判空避免解引用崩溃。
    if (m_world == nullptr || killCredit == nullptr) {
        return;
    }

    // 对齐原版 instanceof WitherBoss：通过 entityType 判定凋灵。
    if (killCredit->entityType() != entity::VanillaEntityTypeKeys::WITHER) {
        return;
    }

    bool placed = false;

    // MOB_GRIEFING 游戏规则守卫。
    if (m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        // blockPosition() 等价物：实体所在方块坐标。
        BlockPos blockPos(
            static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));

        const BlockState* currentState = m_world->getBlockState(blockPos);
        // 对齐原版：getBlockState(blockpos).isAir() && blockstate.canSurvive(level, blockpos)
        if (currentState != nullptr && currentState->isAir()) {
            const BlockState& witherRoseState = VanillaBlocks::WITHER_ROSE->defaultState();
            // isValidPosition 对应原版 canSurvive。
            if (VanillaBlocks::WITHER_ROSE->isValidPosition(
                    witherRoseState, static_cast<IBlockReader&>(*m_world), blockPos)) {
                // setBlock(pos, blockstate, 3)：flags=3 即 NOTIFY(1)|SYNC_CLIENT(2)。
                m_world->setBlockState(blockPos, &witherRoseState, 3);
                placed = true;
            }
        }
    }

    // 对齐原版：若未放置方块，掉落一个凋零玫瑰物品实体。
    if (!placed) {
        // 对齐原版 new ItemStack(Items.WITHER_ROSE)。
        // 注意：Cubium 的 ItemDropHelper::spawnItemEntity 需要随机数生成器，
        //       此处用实体自带的 getRandom()（成员 m_random）。
        ItemStack witherRoseStack(*Items::WITHER_ROSE, 1);
        ItemDropHelper::spawnItemEntity(
            m_world, witherRoseStack, x(), y(), z(), getRandom(), ItemDropHelper::DEFAULT_PICKUP_DELAY);
    }
}

void LivingEntity::dropAllDeathLoot(DamageSource& cause)
{
    // 无世界上下文时跳过掉落（与各子函数 dropFromLootTable:616 / dropCustomDeathLoot:480 /
    // dropExperience:462 的 m_world null 守卫风格一致）。vanilla dropAllDeathLoot 接收 ServerLevel
    // 参数假定非空，Cubium 用成员 m_world 且测试环境允许 nullptr，入口处统一判空避免解引用崩溃。
    // 生产环境实体死亡时 m_world 必非空，此守卫不改变生产行为。
    if (m_world == nullptr) {
        return;
    }

    // 对齐 MC Java 1.21.11 LivingEntity.dropAllDeathLoot（LivingEntity.java:1484-1493）。
    // flag 表示最近是否被玩家伤害过——vanilla 用 lastHurtByPlayerMemoryTime > 0（玩家伤害后
    // 维持 100 tick 的记忆窗口），影响掉落表条件（如掠夺附魔生效、luck 应用、部分条件分支）。
    // 对齐 MC Java 1.21.11 LivingEntity.dropAllDeathLoot（LivingEntity.java:1484-1493）。
    // flag（recentlyHitByPlayer）表示最近是否被玩家伤害过——vanilla 用
    // lastHurtByPlayerMemoryTime > 0（玩家伤害后维持 100 tick 的记忆窗口），影响掉落表条件
    // （如掠夺附魔生效、luck 应用、部分条件分支）。
    // m_lastHurtByPlayerMemoryTime 由 hurt() 的 resolve（对齐 resolvePlayerResponsibleForDamage）
    // 在玩家造成伤害时设 100、每 tick 递减。注：resolvePlayerResponsibleForDamage 对 Player 无
    // NO_ANGER 门控，故 mob_attack_no_aggro 由玩家造成时仍记 memoryTime（用于掉落守卫），但不
    // 激怒（lastHurtByMob 被 NO_ANGER 门控挡住）。
    // KILLER_PLAYER loot 参数仍从 m_lastHurtBy 取具体 Player 实体（见 dropFromLootTable:835）：
    // Cubium 暂无独立的 lastHurtByPlayer 实体引用字段（仅 memoryTime 计数器），用 m_lastHurtBy
    // 近似。TODO: 引入 lastHurtByPlayer 实体引用（对齐 vanilla EntityReference<Player>）后，
    //       KILLER_PLAYER 改从该引用取，覆盖"玩家打残后非玩家补刀"场景（此时 m_lastHurtBy 是
    //       补刀的非玩家，但 memoryTime>0 仍应归属原玩家）。
    const bool recentlyHitByPlayer = (m_lastHurtByPlayerMemoryTime > 0);

    // 1. shouldDropLoot 守卫（doMobLoot gamerule + 非幼体）：守卫包住物品掉落
    //    （dropFromLootTable + dropCustomDeathLoot）。MobEntity override dropCustomDeathLoot
    //    在此守卫内按掉落概率掉落装备（对齐 vanilla Mob.dropCustomDeathLoot）。
    if (shouldDropLoot(*m_world)) {
        dropFromLootTable(cause, recentlyHitByPlayer);
        dropCustomDeathLoot(cause, recentlyHitByPlayer);
    }

    // 2. 装备掉落（在守卫之外，不受 doMobLoot 影响）。对齐 vanilla LivingEntity.dropEquipment（守卫外）。
    //    基类空；MobEntity 不 override（其装备掉落由上面的 dropCustomDeathLoot 在守卫内处理）；
    //    Player override dropEquipment 实现玩家死亡掉落库存（keepInventory 守卫 + 销毁消失诅咒 +
    //    inventory.dropAll，对齐 vanilla Player.dropEquipment）。
    dropEquipment();

    // 3. 经验掉落（在守卫之外，但 vanilla dropExperience 内部自带守卫——仅当
    //    !wasExperienceConsumed() && (isAlwaysExperienceDropper() ||
    //    (lastHurtByPlayerMemoryTime>0 && shouldDropExperience() && doMobLoot)) 时才掉经验）。
    //    Cubium MobEntity::dropExperience 已对齐此守卫（见 shouldDropExperienceOnDeath）。
    //    Player/EnderDragon override dropExperience 直接掉落（等价 isAlwaysExperienceDropper=true，
    //    不受守卫约束）。基类 dropExperience 为空。
    dropExperience();
}

bool LivingEntity::shouldDropLoot(IWorld& world) const
{
    // 对齐 MC Java 1.21.11 LivingEntity.shouldDropLoot（LivingEntity.java:567-569）：
    // `!isBaby() && level.getGameRules().get(MOB_DROPS)`。
    // Cubium 用 isChild() 等价 vanilla isBaby()（Entity::isChild 虚函数，AgeableEntity 等
    // 子类 override 返回幼体状态）。MOB_DROPS 即 doMobLoot gamerule（默认 true）。
    // 幼体生物（isChild）死亡不掉落战利品物品，对齐 vanilla。
    return !isChild() && world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_MOB_LOOT);
}

bool LivingEntity::shouldDropExperienceOnDeath(IWorld& world) const
{
    // 对齐 MC Java 1.21.11 LivingEntity.dropExperience 守卫（LivingEntity.java:1499-1503）：
    // `!wasExperienceConsumed() && (isAlwaysExperienceDropper() ||
    //   (lastHurtByPlayerMemoryTime > 0 && shouldDropExperience() && MOB_DROPS))`。
    // 普通生物（isAlwaysExperienceDropper=false）需被玩家伤害过（lastHurtByPlayerMemoryTime>0）
    // 且非幼体（shouldDropExperience）且 doMobLoot=true 才掉经验；常掉经验实体无条件掉。
    // 由 MobEntity::dropExperience 调用判定。
    if (wasExperienceConsumed()) {
        return false;
    }
    if (isAlwaysExperienceDropper()) {
        return true;
    }
    return m_lastHurtByPlayerMemoryTime > 0 && shouldDropExperience() &&
        world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_MOB_LOOT);
}

void LivingEntity::dropFromLootTable(DamageSource& cause, bool recentlyHitByPlayer)
{
    // 对齐 MC Java 1.21.11 LivingEntity.dropFromLootTable（LivingEntity.java:1522-1550）。
    // 取实体死亡掉落表（getLootTableId → LootTableManager），构建 entity 参数集 LootContext，
    // generate 生成 ItemStack 列表后经 ItemDropHelper::spawnItemAtEntity 掉落于实体位置。
    if (m_world == nullptr) {
        return;
    }

    const std::string lootTableId = getLootTableId();
    if (lootTableId.empty()) {
        return;
    }

    const auto* lootTableManager = m_world->lootTableManager();
    if (lootTableManager == nullptr) {
        return;
    }
    const loot::LootTable* table = lootTableManager->getTable(lootTableId);
    if (table == nullptr) {
        return;
    }

    // 构建 entity 参数集 LootContext（对齐 vanilla dropFromLootTable 参数集）：
    //   THIS_ENTITY = this（被杀实体，必需）
    //   DAMAGE_SOURCE = cause（伤害来源，可选，掉落表条件判定用）
    //   KILLER_ENTITY = cause.getEntity()（攻击者/射击者，可选，对应 vanilla ATTACKING_ENTITY）
    //   DIRECT_KILLER = cause.directSource()（直接来源，投射物本身或近战攻击者，对应 vanilla DIRECT_ATTACKING_ENTITY）
    //   KILLER_PLAYER = 最近伤害玩家（仅 recentlyHitByPlayer 时设，对应 vanilla LAST_DAMAGE_PLAYER，并应用 luck）
    // 注：vanilla 还传 ORIGIN（实体位置），Cubium entity 参数集未含 ORIGIN（非必需），不传。
    auto builder = loot::LootContextBuilder(*m_world);

    // 随机源：用实体自身的随机数生成器 + 掉落表种子（vanilla getLootTableSeed 默认 0）。
    // 为避免每次死亡掉落结果相同，用实体随机数生成器（构造时已播种）。
    math::Random& rng = getRandom();
    builder.withRandom(rng);

    builder.withParameter(loot::LootParams::THIS_ENTITY, static_cast<Entity*>(this));

    // DAMAGE_SOURCE：使用最近伤害来源（m_lastDamageSource，actuallyHurt 第 356 行 clone），
    // 退回到当前 die 的 cause。优先 m_lastDamageSource 因其更精确记录致死伤害。
    DamageSource* damageSource = m_lastDamageSource.get();
    if (damageSource == nullptr) {
        damageSource = &cause;
    }
    builder.withParameter(loot::LootParams::DAMAGE_SOURCE, damageSource);

    // 攻击者（KILLER_ENTITY）：伤害的真实来源（射击者/近战者）。
    Entity* attackingEntity = damageSource->getTrueSource();
    if (attackingEntity != nullptr) {
        builder.withNullableParameter(loot::LootParams::KILLER_ENTITY, attackingEntity);
    }

    // 掠夺附魔等级（LOOTING_MODIFIER）：从攻击者主手武器读取掠夺附魔等级，供 LootingEnchantBonusFunction
    // 增加掉落数量（如 rotten_flesh/beef 多掉）。对齐 vanilla 1.21.11 EnchantedCountIncreaseFunction
    // 从 ATTACKING_ENTITY 武器查 EnchantmentHelper.getEnchantmentLevel(LOOTING, attacker) 的语义
    // （EnchantedCountIncreaseFunction.java:67-72）。Cubium 用旧式 LOOTING_MODIFIER 参数接口适配
    // （LootingEnchantBonusFunction::apply 读 context.getLootingModifier()）。修复前未设此参数，
    // 致玩家持掠夺附魔武器击杀 mob 时掉落数量不增加。
    if (attackingEntity != nullptr) {
        LivingEntity* livingAttacker = dynamic_cast<LivingEntity*>(attackingEntity);
        if (livingAttacker != nullptr) {
            const i32 lootingLevel =
                item::enchant::EnchantmentHelper::getLootingLevel(livingAttacker->getMainHandItem());
            if (lootingLevel > 0) {
                builder.withLootingModifier(lootingLevel);
            }
        }
    }

    // 直接来源（DIRECT_KILLER）：投射物本身或近战攻击者（IndirectEntityDamageSource 返回
    // m_directSource，EntityDamageSource 返回攻击者自身，环境伤害返回 nullptr）。
    Entity* directEntity = damageSource->directSource();
    if (directEntity != nullptr) {
        builder.withNullableParameter(loot::LootParams::DIRECT_KILLER, directEntity);
    }

    // 击杀玩家（KILLER_PLAYER）：仅当最近被玩家伤害时设，并应用该玩家的 luck。
    // 对应 vanilla `if (flag && player != null) withParameter(LAST_DAMAGE_PLAYER, player).withLuck(player.getLuck())`。
    if (recentlyHitByPlayer) {
        Player* player = dynamic_cast<Player*>(m_lastHurtBy);
        if (player != nullptr) {
            builder.withParameter(loot::LootParams::KILLER_PLAYER, player);
            // TODO: luck 属性未接入属性系统时暂用 0。对齐 vanilla player.getLuck()。
            builder.withLuck(0.0f);
        }
    }

    // 掉落表/谓词解析器：供掉落表内部引用其他表（如 reference entry）时解析。
    builder.withLootTableResolver(
        [lootTableManager](const std::string& id) -> const loot::LootTable* { return lootTableManager->getTable(id); });
    builder.withPredicateResolver([lootTableManager](const std::string& id) -> const loot::LootCondition* {
        return lootTableManager->getPredicate(id);
    });

    auto context = builder.build(loot::LootParameterSets::entity());
    if (context == nullptr) {
        return;
    }

    std::vector<ItemStack> drops = table->generate(*context);
    if (drops.empty()) {
        return;
    }

    // 掉落于实体位置（对齐 vanilla spawnAtLocation）。offsetY=0.5 使物品在实体腰部生成。
    for (const auto& stack : drops) {
        if (stack.isEmpty()) {
            continue;
        }
        ItemDropHelper::spawnItemAtEntity(this, stack, 0.5f, rng);
    }
}

void LivingEntity::onKillCommand()
{
    // 使用虚空伤害杀死实体，这会触发完整的死亡流程
    auto damageSource = DamageSources::outOfWorld();
    hurt(damageSource, std::numeric_limits<f32>::max());
}

void LivingEntity::remove()
{
    // 停用所有位置依赖的附魔效果（如灵魂疾行的速度修饰符）
    // 防止实体被移除后属性修饰符残留
    item::enchant::EnchantmentHelper::stopAllLocationBasedEffects(*this);

    Entity::remove();
}

// ============================================================================
// 属性
// ============================================================================

void LivingEntity::registerAttributes()
{
    // 基础属性：所有生物实体都有
    attributes().registerAttribute(*entity::attribute::Attributes::maxHealth());
    attributes().registerAttribute(*entity::attribute::Attributes::knockbackResistance());
    attributes().registerAttribute(*entity::attribute::Attributes::movementSpeed());
    attributes().registerAttribute(*entity::attribute::Attributes::armor());
    attributes().registerAttribute(*entity::attribute::Attributes::armorToughness());
    attributes().registerAttribute(*entity::attribute::Attributes::maxAbsorption());
    attributes().registerAttribute(*entity::attribute::Attributes::movementEfficiency());
    // 摔落相关属性（默认 SAFE_FALL_DISTANCE=3.0、FALL_DAMAGE_MULTIPLIER=1.0），
    // 由 causeFallDamage 的摔落伤害公式消费。JumpBoost 药水通过 Addition 修饰符
    // 每级给 SAFE_FALL_DISTANCE +1。马类覆盖 FALL_DAMAGE_MULTIPLIER=0.5。
    attributes().registerAttribute(*entity::attribute::Attributes::safeFallDistance());
    attributes().registerAttribute(*entity::attribute::Attributes::fallDamageMultiplier());
    // 跳跃力属性（默认 JUMP_STRENGTH=0.42），由 getJumpPower() 消费：
    //   getJumpPower() = JUMP_STRENGTH * getBlockJumpFactor() + getJumpBoostPower()
    // JumpBoost 药水的跳跃加成走 getJumpBoostPower 独立项（0.1*(amplifier+1)），非属性修饰符。
    attributes().registerAttribute(*entity::attribute::Attributes::jumpStrength());
    // 额外氧气属性（默认 OXYGEN_BONUS=0.0），由 decreaseAirSupply 的氧气消耗概率
    // 门控消费（LivingEntity.java:340 注册 OXYGEN_BONUS、:571-582 消费）。
    // 水下呼吸魔咒通过 enchantment.respiration 修饰符（每级 +1.0 ADD_VALUE，HEAD 槽位）注入。
    attributes().registerAttribute(*entity::attribute::Attributes::oxygenBonus());
    // 燃烧时间倍率属性（默认 BURNING_TIME=1.0），由 igniteForTicks override 消费：
    // ceil(ticks * getAttributeValue(BURNING_TIME))。
    // 火焰保护魔咒通过 enchantment.fire_protection 修饰符（每级 -0.15 MULTIPLY_BASE，4 盔甲槽位）
    // 缩减此值，从而缩短被点燃后的燃烧时间。
    attributes().registerAttribute(*entity::attribute::Attributes::burningTime());
    // 爆炸击退抗性属性（默认 0.0），由爆炸击退计算消费：
    // finalKnockback = baseKnockback * (1 - getAttributeValue(EXPLOSION_KNOCKBACK_RESISTANCE))。
    // 爆炸保护魔咒通过 enchantment.blast_protection 修饰符（每级 +0.15 ADD_VALUE，4 盔甲槽位）
    // 增加此值，从而衰减被爆炸推开时的击退力度。
    attributes().registerAttribute(*entity::attribute::Attributes::explosionKnockbackResistance());

    // 注意：以下属性不在基类中注册：
    // - FOLLOW_RANGE: 由 MobEntity 设置默认值 16.0
    // - FLYING_SPEED: 由需要飞行的实体注册
    // - ATTACK_DAMAGE: 由 MonsterEntity 注册
    // - ATTACK_KNOCKBACK: 由需要攻击击退的实体注册
    // - LUCK: 由需要的实体注册
}

f64 LivingEntity::getAttributeValue(const std::string& name, f64 defaultValue) const
{
    return attributes().getValue(name, defaultValue);
}

void LivingEntity::setAttributeBaseValue(const std::string& name, f64 value)
{
    attributes().setBaseValue(name, value);
}

f32 LivingEntity::getBlockSpeedFactor()
{
    // 获取脚下方块的 speedFactor
    f32 blockSpeedFactor = 1.0f;
    if (m_world != nullptr) {
        BlockPos belowPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
            static_cast<i32>(std::floor(m_builtIn.aabbShape->m_aabb.minY - 0.001f)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
        const BlockState* state = m_world->getBlockState(belowPos);
        if (state != nullptr) {
            blockSpeedFactor = state->getBlock().getSpeedFactor(*state, m_world, &belowPos);
        }
    }

    // 使用 MOVEMENT_EFFICIENCY 属性在方块 speedFactor 和 1.0 之间插值
    // efficiency=0.0 返回原始 blockSpeedFactor，efficiency=1.0 返回 1.0（完全忽略减速）
    f64 efficiency = getAttributeValue(entity::attribute::Attributes::MOVEMENT_EFFICIENCY, 0.0);
    efficiency = std::clamp(efficiency, 0.0, 1.0);
    f32 result = static_cast<f32>(blockSpeedFactor + (1.0f - blockSpeedFactor) * static_cast<f32>(efficiency));
    return result;
}

f32 LivingEntity::getSoundPitch() const
{
    math::Random random(static_cast<u64>(m_id) ^ (static_cast<u64>(m_ticksExisted) << 32));
    const f32 basePitch = isChild() ? 1.5f : 1.0f;
    return (random.nextFloat() - random.nextFloat()) * 0.2f + basePitch;
}

// ============================================================================
// 装备
// ============================================================================

const ItemStack& LivingEntity::getEquipment(EquipmentSlot slot) const
{
    size_t index = static_cast<size_t>(slot);
    auto* c = m_entityContext->tryGetComponent<ecs::EquipmentComponent>();
    if (c == nullptr || index >= c->m_equipment.size()) {
        static ItemStack empty;
        return empty;
    }
    return c->m_equipment[index];
}

ItemStack& LivingEntity::getMutableEquipment(EquipmentSlot slot)
{
    size_t index = static_cast<size_t>(slot);
    auto* c = m_entityContext->tryGetComponent<ecs::EquipmentComponent>();
    if (c == nullptr || index >= c->m_equipment.size()) {
        static ItemStack empty;
        return empty;
    }
    return c->m_equipment[index];
}

void LivingEntity::setEquipment(EquipmentSlot slot, const ItemStack& stack)
{
    size_t index = static_cast<size_t>(slot);
    if (auto* c = m_entityContext->tryGetComponent<ecs::EquipmentComponent>()) {
        if (index < c->m_equipment.size()) {
            c->m_equipment[index] = stack;
        }
    }
}

void LivingEntity::onEquippedItemBroken(const Item& item, EquipmentSlot slot)
{
    // 广播装备破损动画给追踪玩家
    broadcastBreakEvent(slot);

    // 停止基于位置的物品效果（移除属性修饰符）
    // 对应 MC 原版 LivingEntity.onEquippedItemBroken() 中调用 stopLocationBasedEffects()
    // 注意：MC 原版读取当前槽位中的物品（此时可能已被清空或替换），
    // 这里使用传入的 item 引用获取该物品类型的属性修饰符
    stopLocationBasedEffects(getEquipment(slot), slot);

    // 播放装备破损音效
    if (m_world != nullptr && !isSilent()) {
        m_world->playSound(SoundEvents::ENTITY_ITEM_BREAK,
            getSoundCategory(),
            m_builtIn.stateVector->m_pos,
            0.8f,
            0.8f + m_world->getRandom().nextFloat() * 0.4f);
    }
}

void LivingEntity::broadcastBreakEvent(EquipmentSlot slot)
{
    if (m_world != nullptr) {
        u8 slotIndex = static_cast<u8>(slot);
        auto status = network::equipmentBreakStatus(slotIndex);
        m_world->broadcastEntityStatus(m_id, static_cast<u8>(status));
    }
}

bool LivingEntity::equipmentHasChanged(const ItemStack& a, const ItemStack& b)
{
    // 对应 MC 原版 LivingEntity.equipmentHasChanged()
    // 使用 operator!= 比较两个物品堆（比较物品类型、数量、耐久、附魔、自定义名称等）
    return a != b;
}

void LivingEntity::detectEquipmentUpdates()
{
    // 对应 MC 原版 LivingEntity.detectEquipmentUpdates()
    // 仅在服务端执行
    if (m_world != nullptr && m_world->isClientSide()) {
        return;
    }

    auto* equip = m_entityContext->tryGetComponent<ecs::EquipmentComponent>();
    if (equip == nullptr) {
        return;
    }

    // 对齐 MC 原版 LivingEntity.collectEquipmentChanges()：m_lastEquipment 默认初始化为全空
    // （同 vanilla lastEquipmentItems 声明期 = ItemStack.EMPTY），首帧即用全空快照对比当前装备。
    // 若实体在 spawn 时已装备物品（如怪物 finalizeSpawn 持武器、SimulatedPlayer spawn 后 setItem），
    // 首帧 equipmentHasChanged(空, 当前装备) 为 true，正确应用该物品的属性修饰符与位置依赖附魔效果。
    // 此前实现曾有"首帧初始化快照=当前装备后 return"分支，导致 spawn 时已装备物品的修饰符永不应用
    // （如玩家手持钻石剑 ATTACK_DAMAGE +6 modifier 未生效，baseDamage 仅有基础 1.0）——已移除。

    // 检查每个槽位是否有变化
    bool anyChanged = false;

    for (u8 i = 0; i < static_cast<u8>(EquipmentSlot::Count); ++i) {
        auto slot = static_cast<EquipmentSlot>(i);
        const ItemStack& lastStack = equip->m_lastEquipment[i];
        const ItemStack& currentStack = getEquipment(slot);

        if (equipmentHasChanged(lastStack, currentStack)) {
            anyChanged = true;

            // 移除旧物品的属性修饰符
            // 对应 MC 原版 collectEquipmentChanges() 中对旧物品调用 stopLocationBasedEffects()
            if (!lastStack.isEmpty()) {
                stopLocationBasedEffects(lastStack, slot);
            }
        }
    }

    // 对变化槽位的新物品应用属性修饰符
    if (anyChanged) {
        for (u8 i = 0; i < static_cast<u8>(EquipmentSlot::Count); ++i) {
            auto slot = static_cast<EquipmentSlot>(i);
            const ItemStack& lastStack = equip->m_lastEquipment[i];
            const ItemStack& currentStack = getEquipment(slot);

            if (equipmentHasChanged(lastStack, currentStack)) {
                // 添加新物品的属性修饰符
                // 对应 MC 原版 collectEquipmentChanges() 中的 forEachModifier + addTransientModifier
                if (!currentStack.isEmpty()) {
                    const Item* item = currentStack.getItem();
                    if (item != nullptr) {
                        item::ItemAttributeModifiers modifiers = item->getAttributeModifiers(static_cast<i32>(slot));
                        for (const auto& entry : modifiers.getEntries()) {
                            if (entry.equipmentSlot == static_cast<i32>(slot)) {
                                // 先移除可能存在的同ID修饰符，再添加新的
                                // 对应 MC 原版 removeModifier(id) + addTransientModifier()
                                attributes().removeModifier(entry.attributeName, entry.modifier.id());
                                attributes().addModifier(entry.attributeName, entry.modifier);
                            }
                        }
                    }

                    // 对新装备运行位置依赖附魔效果
                    // 对应 MC Java 中 collectEquipmentChanges() 后调用 runLocationChangedEffects()
                    if (m_world != nullptr && !m_world->isClientSide()) {
                        item::enchant::EnchantmentHelper::runLocationChangedEffects(*this, currentStack, slot);
                    }

                    // 应用附魔提供的常驻属性修饰符（对齐 vanilla EnchantmentEffectComponents.ATTRIBUTES）
                    // 在物品固有修饰符与位置依赖效果之后应用，使 decreaseAirSupply 等属性消费点能读到
                    // 经附魔修饰符调整后的属性值（如水下呼吸→oxygen_bonus）。
                    item::enchant::EnchantmentHelper::applyEnchantmentAttributeModifiers(*this, currentStack, slot);
                }

                // 更新快照
                equip->m_lastEquipment[i] = currentStack;
            }
        }
    }
}

void LivingEntity::stopLocationBasedEffects(const ItemStack& stack, EquipmentSlot slot)
{
    // 对应 MC 原版 LivingEntity.stopLocationBasedEffects()
    // 移除物品提供的属性修饰符
    if (stack.isEmpty()) {
        return;
    }

    const Item* item = stack.getItem();
    if (item == nullptr) {
        return;
    }

    // 移除该物品在该槽位提供的所有属性修饰符
    item::ItemAttributeModifiers modifiers = item->getAttributeModifiers(static_cast<i32>(slot));
    for (const auto& entry : modifiers.getEntries()) {
        if (entry.equipmentSlot == static_cast<i32>(slot)) {
            attributes().removeModifier(entry.attributeName, entry.modifier.id());
        }
    }

    // 移除附魔提供的常驻属性修饰符（对齐 vanilla EnchantmentEffectComponents.ATTRIBUTES）
    // 必须在物品固有修饰符移除后、位置依赖效果停用前移除，保证属性消费点不再读到附魔修饰符。
    item::enchant::EnchantmentHelper::removeEnchantmentAttributeModifiers(*this, stack, slot);

    // 停用位置相关的附魔效果（如 Frost Walker 冰面替换、Soul Speed 速度加成等）
    item::enchant::EnchantmentHelper::stopLocationBasedEffects(*this, stack, slot);
}

void LivingEntity::onChangedBlock()
{
    // 仅在服务端运行位置依赖附魔效果
    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    // 评估位置依赖附魔效果（如冰霜行者、灵魂疾行）
    // 当附魔的激活条件不再满足时（如灵魂沙被挖走），自动停用效果并清理属性修饰符。
    // 此方法在两种场景下被调用：
    // 1. 实体跨越方块边界时（tick 中检测 m_lastBlockPos 变化）
    // 2. 周期性重新评估（每 20 tick，当有活跃位置附魔但未移动时）
    item::enchant::EnchantmentHelper::runLocationChangedEffects(*this);
}

bool LivingEntity::hurtAndBreak(ItemStack& stack, i32 amount, LivingEntity* entity, EquipmentSlot slot)
{
    if (!stack.isDamageable() || amount <= 0) {
        return false;
    }

    // 在物品被销毁之前保存物品引用，用于 onEquippedItemBroken 回调
    // 因为 attemptDamageItem 会在耐久耗尽时清空 ItemStack（setItem(nullptr)）
    const Item* brokenItem = (stack.getDamage() + amount >= stack.getMaxDamage()) ? stack.getItem() : nullptr;

    stack.attemptDamageItem(amount, entity);

    // 物品损坏时触发回调：广播装备破损动画、播放音效、更新统计
    // 使用 isEmpty() 检查而非 attemptDamageItem 返回值，与 PlayerInventory::damageArmor
    // 和 MobEntity::burnUndead 中已验证的模式保持一致
    if (brokenItem != nullptr && stack.isEmpty() && entity != nullptr) {
        entity->onEquippedItemBroken(*brokenItem, slot);
    }

    return stack.isEmpty();
}

// ============================================================================
// 受伤无敌帧
// ============================================================================

bool LivingEntity::isInvulnerableTo(DamageSource& source) const
{
    // 0. 已移除实体对所有伤害免疫（对齐 vanilla Entity.isInvulnerableToBase:2919 首项 isRemoved()）。
    //    LivingEntity override 不调基类 Entity::isInvulnerableTo，故此处独立补 isRemoved 守卫，
    //    与 Entity::isInvulnerableTo 基类首项语义一致。覆盖 health>0 但 m_removed=true 的场景
    //    （雷电转化原体、despawn 等），此时 isDead()（health<=0）不拦截，须由 isRemoved 兜底。
    //    remove()/discard() 标记 m_removed=true 后实体将在本 tick 末移除，此僵尸窗口内
    //    （见 world/entity/README.md:143）若被 hurt 应免疫，避免对正在清理的实体施加伤害。
    if (isRemoved()) {
        return true;
    }

    // 1. 检查实体是否处于无敌状态（对齐 vanilla isInvulnerableToBase:2920：
    //    invulnerable && !BYPASSES_INVULNERABILITY && !isCreativePlayer()）。
    //    创造模式玩家造成的伤害绕过 invulnerable 标志（如末影龙复活仪式 invulnerable 末影水晶，
    //    创造玩家应能击毁）。LivingEntity override 不调基类，故此处独立补 isCreativePlayer 守卫，
    //    与 Entity::isInvulnerableTo 基类第 1 步语义一致。
    if (Entity::isInvulnerable()) {
        // 虚空/创造玩家可绕过无敌；BYPASSES_INVULNERABILITY 伤害源（虚空/OutOfWorld）也绕过
        return !source.bypassesInvulnerability() && !source.isCreativePlayer();
    }

    // 2. 摔落伤害免疫标签（对齐 vanilla Entity.isInvulnerableToBase:2922：
    //   p_20122_.is(DamageTypeTags.IS_FALL) && this.getType().is(EntityTypeTags.FALL_DAMAGE_IMMUNE)）。
    // FALL_DAMAGE_IMMUNE 实体对任何 IS_FALL 类型伤害源（fall/ender_pearl/stalagmite）免疫。
    // 这是摔落免疫的最外层 hurt 门控，与 causeFallDamage 层的 calculateFallDamage 免疫（LivingEntity.cpp:1448）
    // 互为纵深防御——即便伤害不经 causeFallDamage 而直接 hurt（如末影珍珠摔落），此层也拦截。
    if (source.is(DamageTypeTags::IS_FALL()) && EntityTypeTags::FALL_DAMAGE_IMMUNE().contains(getTypeId())) {
        return true;
    }

    // 3. 火焰伤害免疫（对齐 vanilla Entity.isInvulnerableToBase:2921：
    //   p_20122_.is(DamageTypeTags.IS_FIRE) && this.fireImmune()）。
    // 火焰免疫实体（isImmuneToFire()==true，如烈焰人/岩浆怪/僵尸猪灵/恶魂/末影龙/潜影贝/凋灵等）
    // 对所有 IS_FIRE 伤害源（in_fire/campfire/on_fire/lava/hot_floor/fireball/unattributed_fireball）免疫。
    // 这是火焰免疫的最外层 hurt 门控。FireBlock::onEntityCollision/Entity::lavaHurt 等前置守卫点已查
    // isImmuneToFire() 提前 return，但 CampfireBlock::onEntityCollision/MagmaBlock::stepOn 无前置守卫，
    // 直接 hurt(campfire/hotFloor) 依赖此门控过滤火焰免疫实体（vanilla CampfireBlock/MagmaBlock 同样
    // 不自查 fireImmune，完全依赖 isInvulnerableToBase 的此分支）。
    // source.is(IS_FIRE) 标签查询等价于 source.isFire() flag（成员集已对齐，见 DamageSource.hpp:351）。
    if (source.is(DamageTypeTags::IS_FIRE()) && isImmuneToFire()) {
        return true;
    }

    // 4. 无敌帧门控**不在此处**（对齐 vanilla Entity.isInvulnerableToBase:2918-2923）。
    //   vanilla 的 isInvulnerableToBase 只含：isRemoved/invulnerable 标志、IS_FALL+FALL_DAMAGE_IMMUNE、
    //   IS_FIRE+fireImmune——**不含** invulnerableTime（无敌帧）。无敌帧在 LivingEntity.hurtServer
    //   :1191-1206 的主流程独立做（invulnerableTime>10 && !BYPASSES_COOLDOWN 的差额逻辑）。
    //
    //   此前 Cubium 在此塞入了 `m_hurtResistantTime > 0 && !source.bypassesInvulnerability() → true`
    //   的无敌帧门控（用 >0 比伤害主流程的 >10 更宽），致各 hurt 入口首查 isInvulnerableTo 时就在此
    //   拦截，使 LivingEntity::hurt:260 的"无敌帧差额逻辑"（无敌帧内更大伤害承受差额）成为死代码。
    //   真实行为缺陷：实体在无敌帧内受到比上次更大的伤害时，vanilla 承受差额（actuallyHurt(amount-lastHurt)、
    //   更新 lastHurt），Cubium 完全免疫 0 点（被多怪围攻、连续受不同强度伤害时显著偏离 vanilla）。
    //
    //   现移除此门控，无敌帧完全交给 LivingEntity::hurt 的差额逻辑处理，对齐 vanilla 设计。
    //   isRemoved() 守卫已在上方第 0 步补齐（对齐 vanilla isInvulnerableToBase:2918 首项）。

    // 5. 附魔 DAMAGE_IMMUNITY 效果（对齐 vanilla LivingEntity.isInvulnerableTo:3857
    //   isInvulnerableToBase || EnchantmentHelper.isImmuneToDamage）。
    //   vanilla 中冰霜行者附魔（FrostWalker）经 EnchantmentEffectComponents.DAMAGE_IMMUNITY 组件
    //   （Enchantments.java:388-396，条件 tag(is(BURN_FROM_STEPPING)) && tag(isNot(BYPASSES_INVULNERABILITY))）
    //   使穿戴者完全免疫 BURN_FROM_STEPPING 标签伤害（campfire 营火踩踏、hot_floor 岩浆块踩踏）。
    //   Cubium 无附魔效果组件体系，此处硬编码等价实现：查靴子（冰霜行者仅注册在 FEET 槽）的
    //   frost_walker 附魔等级 > 0 且伤害源属 BURN_FROM_STEPPING 且非 BYPASSES_INVULNERABILITY → 免疫。
    //   任意等级均免疫（vanilla DamageImmunity 不依赖 level，条件匹配即免疫）。
    if (!source.bypassesInvulnerability() && source.is(DamageTypeTags::BURN_FROM_STEPPING())) {
        const ItemStack& boots = getEquipment(EquipmentSlot::Feet);
        if (!boots.isEmpty() &&
            item::enchant::EnchantmentHelper::getEnchantmentLevel(boots, "minecraft:frost_walker") > 0) {
            return true;
        }
    }

    return false;
}

void LivingEntity::playHurtSound(DamageSource& source)
{
    auto soundEvent = getHurtSound(source);
    if (soundEvent.has_value()) {
        playSound(*soundEvent, getSoundVolume(), getSoundPitch());
    }
}

void LivingEntity::playDeathSound()
{
    auto soundEvent = getDeathSound();
    if (soundEvent.has_value()) {
        playSound(*soundEvent, getSoundVolume(), getSoundPitch());
    }
}

std::optional<ResourceLocation> LivingEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> LivingEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

// ============================================================================
// 刻更新
// ============================================================================

void LivingEntity::tick()
{
    Entity::tick();

    // 首帧生命值同步：构造期 registerAttributes 因虚函数时序拿不到派生类 MAX_HEALTH
    // （如 chicken=4.0），m_health 停在默认 20.0 违反 health<=maxHealth 不变式，致
    // hurt 的扣血在超 max 基线上扣除，实体"打不死"。此处兜底同步。
    // 对齐 vanilla LivingEntity 构造末尾 setHealth(getMaxHealth())。
    auto* healthState = m_entityContext->tryGetComponent<ecs::HealthComponent>();
    if (healthState != nullptr && !healthState->m_healthSynced) {
        healthState->m_healthSynced = true;
        setHealth(maxHealth());
    }

    // 更新使用中的物品
    // 对应 MC 1.21.11 LivingEntity.tick() 中的 this.updatingUsingItem()
    // 递减使用计时器、调用 Item::onUseTick、使用完成时调用 Item::onItemUseFinish
    updateActiveItem();

    // 保存上一帧渲染属性
    m_prevLimbSwing = m_limbSwing;
    m_prevLimbSwingAmount = m_limbSwingAmount;
    m_prevSwingProgress = m_swingProgress;
    m_prevRenderYawOffset = m_renderYawOffset;
    m_prevRotationYawHead = m_rotationYawHead;

    // 更新效果
    m_effectManager.tick(*this);

    // 检测装备更新（服务端）
    // 对应 MC 原版 LivingEntity.tick() 中的 detectEquipmentUpdates() 调用
    detectEquipmentUpdates();

    // 检测方块位置变化（服务端），触发位置依赖的附魔效果
    // 对应 MC Java 的 LivingEntity.baseTick() 中 lastPos != blockPosition() 检测。
    // 守卫 m_world：与同文件 tickFreeze/updateAirSupply/tickArrows 等位置一致，
    // 也与 Entity::baseTick 的 null-safe 契约对齐（Entity 构造允许 world=nullptr，
    // baseTick 全程守卫）。原版 LivingEntity.tick 假设 level 非空（实体注册后才 tick），
    // 但 Cubium 允许"无世界 tick 基类部分"，此处需显式守卫避免空指针解引用。
    if (m_world != nullptr && !m_world->isClientSide()) {
        BlockPos currentBlockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
        if (currentBlockPos != m_lastBlockPos) {
            m_lastBlockPos = currentBlockPos;
            onChangedBlock();
        } else if (m_locationEnchantmentTracker.hasActiveEnchantments()) {
            // 周期性重新评估位置依赖附魔效果：当实体有活跃的位置依赖附魔但未移动时，
            // 脚下方块可能已被破坏或替换（如灵魂沙被挖走），需要重新评估附魔是否应停用。
            // 每 20 tick（1秒）检查一次，与 MC 原版行为一致（MC 通过 tickEffects 提供周期性评估机会）。
            if (m_ticksExisted % 20 == 0) {
                onChangedBlock();
            }
        }
    }

    // 更新无敌帧计时器
    if (m_hurtResistantTime > 0) {
        m_hurtResistantTime--;
    }

    // 过期清理"最近攻击者"（对齐 vanilla LivingEntity.aiStep:475-484）：
    //   LivingEntity livingentity = this.getLastHurtByMob();
    //   if (livingentity != null) {
    //       if (!livingentity.isAlive()) this.setLastHurtByMob(null);
    //       else if (this.tickCount - this.lastHurtByMobTimestamp > 100) this.setLastHurtByMob(null);
    //   }
    // vanilla 每 tick 检查 lastHurtByMob：攻击者已死亡或距上次受伤超 100 tick 则遗忘复仇目标。
    // 此前 Cubium 仅在 MobEntity::tick 用 isRemoved() 清理（任务 #272 UAF 防护），缺 100 tick
    // 过期分支——m_lastHurtBy 永久保留（直到攻击者 isRemoved），致 HurtByTargetGoal 在很久之后
    // 仍可能读到陈旧复仇目标（虽 timestamp 去重防重复触发，但 getLastHurtBy() 返回非 null 偏离
    // vanilla 语义）。此处补全双分支：isAlive 守卫 + 100 tick 过期，与 MobEntity 的 isRemoved
    // 清理互补（isRemoved 比 isAlive 更严格，二者共存无冲突——isRemoved 时 isAlive 亦 false）。
    // 注：vanilla 此清理在 aiStep（Mob 每 tick 调），Cubium aiStep 被部分掏空（任务 #314），
    // 故放在 LivingEntity::tick（所有 LivingEntity 每 tick 必经）确保覆盖。
    if (m_lastHurtBy != nullptr) {
        if (!m_lastHurtBy->isAlive() || ticksExisted() - m_lastHurtByTimestamp > 100) {
            setLastHurtBy(nullptr);
        }
    }

    // 递减"最近被玩家伤害"记忆时间（对齐 vanilla LivingEntity.aiStep:466-470）。
    // 归零后玩家伤害记忆失效，影响死亡经验掉落守卫（普通生物需此值>0 才掉经验）。
    if (m_lastHurtByPlayerMemoryTime > 0) {
        m_lastHurtByPlayerMemoryTime--;
    }

    // 更新受伤动画计时器
    if (auto* c = m_entityContext->tryGetComponent<ecs::HurtStateComponent>()) {
        if (c->m_hurtTime > 0) {
            c->m_hurtTime--;
        }
    }

    // 更新攻击动画
    if (m_swingInProgress) {
        ++m_swingProgressInt;
        const i32 swingEnd = getArmSwingAnimationEnd();
        if (m_swingProgressInt >= swingEnd) {
            m_swingProgressInt = 0;
            m_swingInProgress = false;
        }
        m_swingProgress = static_cast<f32>(m_swingProgressInt) / static_cast<f32>(swingEnd);
    } else {
        m_swingProgress = 0.0f;
        m_swingProgressInt = 0;
    }

    // 更新步态动画
    updateAnimation();

    // 执行 AI 步进（物理更新）
    aiStep();

    // 更新生命值
    tickHealth();

    // 更新冰冻状态
    tickFreeze();

    // 更新空气供应和溺水
    updateAirSupply();

    // 更新箭矢自动脱落
    tickArrows();

    // 更新死亡
    if (isDead()) {
        tickDeath();
    }

    // 周期性重新检查战斗状态（对齐 vanilla LivingEntity.tick() LivingEntity.java:2648-2650）。
    // 每 20 tick 调一次 recheckStatus()，清理超时战斗条目、结束战斗。
    if (m_ticksExisted % 20 == 0) {
        m_combatTracker.recheckStatus();
    }

    // 重置战斗状态
    if (m_inCombat && ticksExisted() - m_lastDamageTimestamp > CombatTracker::COMBAT_TIMEOUT) {
        m_inCombat = false;
        sendEndCombat();
    }

    // 更新激流攻击状态
    updateSpinAttack();

    // 鞘翅飞行计时器
    // 对应 MC 1.21.11 LivingEntity.tick() 末尾：
    //   if (this.isFallFlying()) { this.fallFlyTicks++; } else { this.fallFlyTicks = 0; }
    // fallFlyTicks 用于 updateFallFlying() 中每 10 tick 周期触发 ELYTRA_GLIDE
    // 游戏事件与装备损坏，客户端渲染器（BipedModel）也可读取此值驱动头部过渡动画。
    if (isElytraFlying()) {
        ++m_fallFlyTicks;
    } else {
        m_fallFlyTicks = 0;
    }

    // 游泳动画渐变量推进
    // 对应 MC 1.21.11 LivingEntity.tick() 中的 this.updateSwimAmount()。
    // 注意：客户端 ClientEntity 有自己的 tick 实现，会单独推进本地副本，
    // 因此本调用只在服务端 tick 路径生效；ClientEntity::tick 中会复刻同样的推进逻辑。
    updateSwimAmount();
}

void LivingEntity::syncMetadataFromDataManager()
{
    Entity::syncMetadataFromDataManager();
    // health/arrowCount/stingerCount 已组件化为真相源，不从 DataParameter 镜像回填。
}

void LivingEntity::updateAnimation()
{
    // 计算移动距离
    f32 dx = x() - prevX();
    f32 dz = z() - prevZ();
    f32 distance = std::sqrt(dx * dx + dz * dz);

    // 更新步态动画
    m_prevLimbSwingAmount = m_limbSwingAmount;
    m_limbSwingAmount += (distance - m_limbSwingAmount) * 0.4f;

    // 如果在移动，增加步态周期
    if (distance > 0.001f) {
        m_limbSwing += std::min(distance, 1.0f);
    }

    // 更新移动距离
    m_prevMovedDistance = m_movedDistance;
    m_movedDistance = distance;
}

i32 LivingEntity::getArmSwingAnimationEnd() const
{
    // 默认 6 tick，急迫效果减少，挖掘疲劳增加
    i32 base = 6;

    // 检查急迫效果（加快挥动）
    const i32 hasteLevel = getEffectLevel(entity::effect::EffectType::Haste);
    if (hasteLevel > 0) {
        base -= 1 + hasteLevel;
    }

    // 检查挖掘疲劳效果（减慢挥动）
    const i32 fatigueLevel = getEffectLevel(entity::effect::EffectType::MiningFatigue);
    if (fatigueLevel > 0) {
        base += (1 + fatigueLevel) * 2;
    }

    // 确保最小值为 1
    return std::max(base, 1);
}

void LivingEntity::swing(Hand hand)
{
    // 条件判断允许在动画进行到一半时重新触发
    if (!m_swingInProgress || m_swingProgressInt >= getArmSwingAnimationEnd() / 2 || m_swingProgressInt < 0) {
        m_swingProgressInt = -1;
        m_swingInProgress = true;
        m_swingingHand = hand;

        // 服务端广播挥动动画事件，对应 MC 1.21.11 LivingEntity.swing() 中
        // 发送 ClientboundAnimatePacket 的逻辑。客户端收到后通过
        // ClientEntity::triggerSwingAnimation 启动本地 6 tick 挥动动画。
        if (m_world != nullptr && !m_world->isClientSide()) {
            const u8 animation = (hand == Hand::MainHand) ? static_cast<u8>(network::EntityAnimation::SwingMainHand)
                                                          : static_cast<u8>(network::EntityAnimation::SwingOffHand);
            m_world->broadcastEntityAnimation(m_id, animation);
        }
    }
}

void LivingEntity::updateTravelAnimation(bool includeVertical)
{
    // 在 travel() 结束时调用，更新肢体摆动动画

    // 保存上一帧的 limbSwingAmount
    m_prevLimbSwingAmount = m_limbSwingAmount;

    // 计算位移
    f64 dx = static_cast<f64>(x() - prevX());
    f64 dy = includeVertical ? static_cast<f64>(y() - prevY()) : 0.0;
    f64 dz = static_cast<f64>(z() - prevZ());

    // 计算移动距离并乘以 4（用于动画速度）
    f32 distance = static_cast<f32>(std::sqrt(dx * dx + dy * dy + dz * dz)) * 4.0f;

    // 限制最大值为 1.0
    if (distance > 1.0f) {
        distance = 1.0f;
    }

    // 平滑插值 limbSwingAmount
    m_limbSwingAmount += (distance - m_limbSwingAmount) * 0.4f;

    // 增加 limbSwing 周期计数
    m_limbSwing += m_limbSwingAmount;
}

void LivingEntity::tickHealth()
{
    // 自然回血逻辑
    // 生命恢复效果每 50/(level+1) tick 治疗 1 点生命
    // 和平模式下每秒恢复 1 点生命

    // 检查生命恢复效果
    const i32 regenLevel = getEffectLevel(entity::effect::EffectType::Regeneration);
    if (regenLevel > 0 && health() < maxHealth()) {
        // 生命恢复 tick 计数器
        m_regenTickCounter++;
        const i32 regenInterval = 50 / (regenLevel + 1);
        if (m_regenTickCounter >= regenInterval) {
            m_regenTickCounter = 0;
            heal(1.0f);
        }
    } else {
        m_regenTickCounter = 0;
    }

    // 更新属性缓存
    for (auto& [name, instance] : attributes().allInstances()) {
        if (instance->isDirty()) {
            (void)instance->getValue(); // 重新计算并缓存，故意丢弃返回值
        }
    }
}

void LivingEntity::tickDeath()
{
    auto* c = m_entityContext->tryGetComponent<ecs::HurtStateComponent>();
    if (c == nullptr) {
        return;
    }
    c->m_deathTime++;

    // 死亡动画（20 ticks = 1 秒）
    if (c->m_deathTime >= 20) {
        // 广播 MobPoof(60)，客户端收到后对生物生成消散烟雾粒子。
        if (m_world != nullptr) {
            m_world->broadcastEntityStatus(m_id, static_cast<u8>(network::EntityStatus::MobPoof));
        }
        remove(); // 移除实体
    }
}

// ============================================================================
// 冰冻系统
// ============================================================================

void LivingEntity::clearFreeze()
{
    // 重置冰冻计时器
    setTicksFrozen(0);
    // 移除冰冻减速修饰符
    removeFrost();
}

void LivingEntity::igniteForTicks(i32 ticks)
{
    // 在委托基类设置火焰计时器前，将传入 tick 数乘以 BURNING_TIME 属性值并向上取整：
    //   scaledTicks = ceil(ticks * getAttributeValue(BURNING_TIME))。
    // 火焰保护魔咒通过 enchantment.fire_protection 修饰符（每级 -0.15 MULTIPLY_BASE，
    // 4 盔甲槽位）缩减 BURNING_TIME 值，从而缩短被点燃后的燃烧时间（1 级 0.85、4 级 0.4）。
    // 默认 1.0 时 ceil(ticks*1.0)=ticks，行为不变。
    //
    // 入口缩放而非 tick 递减时缩放：仅缩放新设置的燃烧时间，已存的火焰计时器不追溯调整
    // （多源点燃时后到的较小缩放值因 m_fire < ticks 守卫不会缩短已有燃烧时间）。
    const f64 burningTimeMultiplier = attributes().getValue(entity::attribute::Attributes::BURNING_TIME, 1.0);
    const i32 scaledTicks = static_cast<i32>(std::ceil(static_cast<f64>(ticks) * burningTimeMultiplier));
    Entity::igniteForTicks(scaledTicks);
}

bool LivingEntity::canFreeze() const
{
    // 旁观者不能被冰冻
    if (isSpectator()) {
        return false;
    }

    // 检查皮革护甲：任意一件皮革护甲即可免疫冰冻
    // 皮革护甲包括：皮革头盔、皮革胸甲、皮革护腿、皮革靴子、皮革马铠
    if (item::tag::ItemTags::isInitialized()) {
        const auto& freezeImmuneTag = item::tag::ItemTags::FREEZE_IMMUNE_WEARABLES();
        for (const ItemStack* slot : getArmorSlots()) {
            if (slot != nullptr && !slot->isEmpty() && slot->getItem() != nullptr) {
                if (freezeImmuneTag.contains(*slot)) {
                    return false;
                }
            }
        }
    }

    // 委托给基类检查实体类型标签
    return Entity::canFreeze();
}

void LivingEntity::tickFreeze()
{
    // 仅在服务端执行
    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    // 如果不在细雪中或不可冰冻，冰冻计时器每 tick -2（解冻速度是冰冻速度的两倍）
    if (!isInPowderSnow() || !canFreeze()) {
        setTicksFrozen(std::max(0, getTicksFrozen() - 2));
    }

    // 移除旧的冰冻减速修饰符，然后根据当前冰冻百分比重新添加
    removeFrost();
    tryAddFrost();

    // 每 40 tick（2 秒），如果完全冰冻且可冰冻，造成 1.0 冰冻伤害
    // 非玩家实体始终受到冰冻伤害，玩家通过 isInvulnerableTo() 检查 FREEZE_DAMAGE 游戏规则
    if (ticksExisted() % FREEZE_HURT_FREQUENCY == 0 && isFullyFrozen() && canFreeze()) {
        auto freezeSource = DamageSources::freeze();

        // 5倍伤害乘数在 actuallyHurt() 中处理，此处始终传入 1.0 伤害
        hurt(freezeSource, 1.0f);
    }
}

void LivingEntity::removeFrost()
{
    // 移除冰冻减速修饰符
    auto* speedAttr = attributes().getInstance(entity::attribute::Attributes::MOVEMENT_SPEED);
    if (speedAttr != nullptr) {
        speedAttr->removeModifier(SPEED_MODIFIER_POWDER_SNOW_UUID);
    }
}

void LivingEntity::tryAddFrost()
{
    // 如果冰冻计时器 > 0 且脚下方块不是空气，添加减速修饰符
    if (getTicksFrozen() <= 0) {
        return;
    }

    // 检查脚下方块是否为空气
    if (m_world != nullptr) {
        const BlockPos belowPos = onPos();
        const BlockState* belowState = m_world->getBlockState(belowPos);
        if (belowState == nullptr || belowState->isAir()) {
            return;
        }
    }

    auto* speedAttr = attributes().getInstance(entity::attribute::Attributes::MOVEMENT_SPEED);
    if (speedAttr == nullptr) {
        return;
    }

    // 冰冻减速修饰符：-0.05 * 冰冻百分比
    // 完全冰冻时减少 0.05 移动速度（基础速度 0.1，减速50%）
    const f32 frostAmount = -0.05f * getPercentFrozen();
    entity::attribute::AttributeModifier modifier(SPEED_MODIFIER_POWDER_SNOW_UUID,
        "powder_snow",
        static_cast<f64>(frostAmount),
        entity::attribute::Operation::Addition);
    speedAttr->addModifier(modifier);
}

// ============================================================================
// 摔落伤害
// ============================================================================

void LivingEntity::handleFallDamage(f32 distance, f32 damageMultiplier)
{
    // 默认使用普通摔落伤害来源
    causeFallDamage(distance, damageMultiplier, DamageSources::fall());
}

void LivingEntity::causeFallDamage(f32 distance, f32 damageMultiplier, const DamageSource& source)
{
    // MC 1.21.11: LivingEntity.causeFallDamage 先调用 super.causeFallDamage（传播给乘客）
    // 参考: net.minecraft.world.entity.LivingEntity.causeFallDamage → super.causeFallDamage
    Entity::causeFallDamage(distance, damageMultiplier, source);

    // 摔落伤害免疫标签（对齐 vanilla LivingEntity.calculateFallDamage:1755：
    //   if (getType().is(EntityTypeTags.FALL_DAMAGE_IMMUNE)) return 0;）。
    // FALL_DAMAGE_IMMUNE 成员（magma_cube/breeze/iron_golem/snow_golem/shulker/allay/bat/bee/
    //   blaze/cat/chicken/ghast/happy_ghast/phantom/ocelot/parrot/wither/copper_golem）免疫摔落伤害。
    // 此前 Cubium 此标签无任何运行时查询（EntityTypeTags.cpp:634 旧注释误称"由各实体 causeFallDamage
    //   override 实现"，但实际这些实体均无 override，会受摔落伤害——系统性偏差）。此处补查询对齐 vanilla。
    // 标签免疫时不 hurt、不播音效（对齐 vanilla calculateFallDamage 返 0 → causeFallDamage 走 else 分支
    //   不 hurt 不播音效），乘客已由上面 Entity::causeFallDamage 传播（vanilla 同样先传播再判自身免疫）。
    // 同时覆盖普通摔落（DamageSources::fall）与石笋（DamageSources::stalagmite）路径——两者都经本方法。
    if (EntityTypeTags::FALL_DAMAGE_IMMUNE().contains(getTypeId())) {
        return;
    }

    // 缓降效果免疫摔落伤害
    if (hasEffect(entity::effect::EffectType::SlowFalling)) {
        return;
    }

    // 计算摔落伤害（对齐 vanilla LivingEntity.calculateFallDamage/calculateFallPower:
    //   fallPower = distance + 1e-6 - SAFE_FALL_DISTANCE;
    //   fallDamage = floor(fallPower * damageMultiplier * FALL_DAMAGE_MULTIPLIER);）
    // - SAFE_FALL_DISTANCE 默认 3.0，JumpBoost 药水通过 Addition 修饰符每级 +1
    //   （等价于原先在 distance 上减 jumpBoostLevel，现统一走属性体系）。
    // - FALL_DAMAGE_MULTIPLIER 默认 1.0，马类覆盖为 0.5。
    // - damageMultiplier 由调用方传入（普通方块 1.0、干草块/蜂蜜块 0.2、石笋 2.0、史莱姆块 0）。
    // - 1e-6 偏移消除浮点边界：恰好等于安全距离时不产生伤害。
    // - floor 整体取整：伤害按整数格结算（vanilla calculateFallDamage 返回 int）。
    // 注意：摔落保护附魔的减伤在 actuallyHurt 管线的 applyPotionDamageCalculations
    //   统一处理（对 isFall() 伤害），此处不再重复减伤（此前此处重复减伤为 bug）。
    const f32 safeFallDistance =
        static_cast<f32>(attributes().getValue(entity::attribute::Attributes::SAFE_FALL_DISTANCE, 3.0));
    const f32 fallDamageMultiplier =
        static_cast<f32>(attributes().getValue(entity::attribute::Attributes::FALL_DAMAGE_MULTIPLIER, 1.0));
    const f32 fallPower = distance + 1.0e-6f - safeFallDistance;
    const i32 damage = static_cast<i32>(std::floor(fallPower * damageMultiplier * fallDamageMultiplier));

    if (damage > 0) {
        auto sourceClone = source.clone();
        hurt(*sourceClone, static_cast<f32>(damage));
    }

    // 播放摔落音效
    playFallSound(distance);
}

// ============================================================================
// 摔落音效
// ============================================================================

std::optional<ResourceLocation> LivingEntity::getFallSound(i32 /*fallHeight*/) const
{
    // 基类默认不返回摔落音效
    // 子类（Player, MonsterEntity 等）可以重写
    return std::nullopt;
}

void LivingEntity::playFallSound(f32 distance)
{
    if (m_world == nullptr || isSilent()) {
        return;
    }

    // 播放实体的摔落音效
    i32 fallHeight = static_cast<i32>(distance);
    auto fallSound = getFallSound(fallHeight);
    if (fallSound.has_value()) {
        playSound(*fallSound, 1.0f, 1.0f);
    }

    // 播放脚下方块的摔落音效
    BlockPos landPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y - 0.2f)), // 脚底位置
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    const BlockState* landState = m_world->getBlockState(landPos);
    if (landState != nullptr && !landState->isAir()) {
        const BlockSoundType& soundType = landState->getSoundType();
        playSound(soundType.getFallSound(), soundType.getVolume() * 0.5f, soundType.getPitch() * 0.75f);
    }
}

// ============================================================================
// AI移动（travel方法）
// ============================================================================

void LivingEntity::jump()
{
    // 起跳垂直初速度 = JUMP_STRENGTH * blockJumpFactor + JumpBoost 药水加成
    f32 jumpPower = getJumpPower();

    // 设置垂直速度
    m_builtIn.velocity->m_velocity.y = jumpPower;

    // 冲刺跳跃
    // 如果正在冲刺，添加额外的向前动量
    if (hasFlag(EntityFlags::Sprinting)) {
        // 获取朝向方向的水平分量
        f32 yawRad = yaw() * math::DEG_TO_RAD;
        f32 forwardX = -std::sin(yawRad) * 0.2f;
        f32 forwardZ = std::cos(yawRad) * 0.2f;
        m_builtIn.velocity->m_velocity.x += forwardX;
        m_builtIn.velocity->m_velocity.z += forwardZ;
    }

    m_builtIn.physicsState->m_onGround = false;
}

f32 LivingEntity::getJumpPower() const
{
    // getJumpPower() = JUMP_STRENGTH * getBlockJumpFactor() + getJumpBoostPower()
    // JUMP_STRENGTH 默认 0.42，蜂蜜块等方块通过 getBlockJumpFactor 削弱跳跃，
    // JumpBoost 药水加成走 getJumpBoostPower 独立项（非属性修饰符）。
    const f64 jumpStrength = getAttributeValue(entity::attribute::Attributes::JUMP_STRENGTH, 0.42);
    return static_cast<f32>(jumpStrength) * getBlockJumpFactor() + getJumpBoostPower();
}

f32 LivingEntity::getJumpBoostPower() const
{
    // 有 JumpBoost 效果时 0.1*(amplifier+1)，否则 0.0。
    // getEffectLevel 返回 1-based 等级（I级=1，II级=2），即 amplifier+1。
    const i32 jumpBoostLevel = getEffectLevel(entity::effect::EffectType::JumpBoost);
    if (jumpBoostLevel <= 0) {
        return 0.0f;
    }
    return 0.1f * static_cast<f32>(jumpBoostLevel);
}

f32 LivingEntity::getBlockJumpFactor() const
{
    // 脚下方块对跳跃力的缩放因子，蜂蜜块为 0.5（削弱跳跃），其余 1.0。
    if (m_world == nullptr) {
        return 1.0f;
    }
    BlockPos belowPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.aabbShape->m_aabb.minY - 0.001f)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    const BlockState* state = m_world->getBlockState(belowPos);
    if (state == nullptr) {
        return 1.0f;
    }
    return state->getBlock().getJumpFactor(*state, m_world, &belowPos);
}

void LivingEntity::aiStep()
{
    // 处理跳跃
    if (m_isJumping) {
        // 在地面时执行跳跃
        if (m_builtIn.physicsState->m_onGround && m_jumpTicks == 0) {
            jump();
            m_jumpTicks = 10; // 跳跃冷却
        }
    } else {
        m_jumpTicks = 0;
    }

    // 更新跳跃冷却
    if (m_jumpTicks > 0) {
        m_jumpTicks--;
    }

    // 鞘翅飞行状态机更新
    // 对应 MC 1.21.11 LivingEntity.aiStep() 中的：
    //   if (this.isFallFlying()) { this.updateFallFlying(); }
    // 检查可滑翔条件、周期性触发 ELYTRA_GLIDE 游戏事件与装备损坏。
    if (isElytraFlying()) {
        updateFallFlying();
    }

    // 执行 travel（物理移动）
    // 注意：aiStep() 中不对输入值应用阻力
    // 阻力是在 travel() 中应用到速度上的
    travel(m_moveStrafing, 0.0f, m_moveForward);

    // 执行方块碰撞检测
    // 对应 MC 原版 LivingEntity.aiStep() 中的 applyEffectsFromBlocks() 调用
    // 遍历实体碰撞箱覆盖的所有方块，触发 onEntityCollision 和 onInsideBlock 回调
    // 用于处理蜘蛛网减速、仙人掌伤害、甜浆果丛伤害、气泡柱推拉、传送门激活等效果
    doBlockCollisions();
}

void LivingEntity::travel(f32 strafing, f32 vertical, f32 forward)
{
    // 对应 MC 1.21.11 LivingEntity.travel(Vec3)：
    //   if (shouldTravelInFluid) travelInFluid
    //   else if (isFallFlying) travelFallFlying
    //   else travelInAir
    // Cubium 的 travel 已将 travelInAir/travelInFluid 内联在此方法中。
    // 当实体处于鞘翅飞行状态且不在液体中时，委托给 travelFallFlying() 处理滑翔物理。
    // 在水中/岩浆中时走常规分支（液体中无法滑翔，tryToStartFallFlying 已阻止开始）。
    if (isElytraFlying() && !isInWater() && !isInLava()) {
        travelFallFlying(Vector3(strafing, vertical, forward));
        // 滑翔物理已自行执行 moveWithCollision 与动画更新
        updateTravelAnimation(true);
        return;
    }

    // 正确的物理顺序：
    // 1. 计算移动因子
    // 2. moveRelative(): 速度 += 输入 * 移动因子
    // 3. 应用重力（或攀爬）
    // 4. 执行移动（碰撞检测）
    // 5. 应用摩擦/阻力（基于滑度）
    // 6. 重置过小速度

    // 检查是否在梯子上
    bool onLadder = isOnLadder();

    // 获取移动速度属性
    f32 moveSpeed = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2));

    // 获取脚下方块的滑度
    f32 slipperiness = 0.6f; // 默认滑度
    if (m_builtIn.physicsState->m_onGround && m_world != nullptr) {
        BlockPos blockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
            static_cast<i32>(std::floor(m_builtIn.aabbShape->m_aabb.minY - 0.001f)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
        const BlockState* blockState = m_world->getBlockState(blockPos);
        if (blockState != nullptr) {
            slipperiness = blockState->getBlock().getSlipperiness(*blockState, m_world, &blockPos, this);
        }
    }

    // 根据是否在地面选择不同的移动因子
    f32 moveFactor;
    if (m_builtIn.physicsState->m_onGround) {
        // 地面移动：使用滑度计算
        moveFactor = moveSpeed * 0.21600002f / (slipperiness * slipperiness * slipperiness);

        // 地面移动因子需要乘以 getBlockSpeedFactor()
        // getBlockSpeedFactor() 使用 MOVEMENT_EFFICIENCY 属性在方块 speedFactor 和 1.0 之间插值
        // 灵魂疾行附魔设置 MOVEMENT_EFFICIENCY=1.0，使灵魂沙/土上的 speedFactor 从 0.4 插值到 1.0
        moveFactor *= getBlockSpeedFactor();
    } else {
        // 空中移动：使用跳跃移动因子
        moveFactor = m_jumpMovementFactor;
    }

    // 1. 计算移动向量并添加到速度
    if (strafing != 0.0f || forward != 0.0f) {
        f32 length = std::sqrt(strafing * strafing + forward * forward);
        if (length < 1.0E-7f) {
            length = 1.0E-7f;
        }

        // 归一化并应用速度
        f32 normalizedStrafe = strafing / length * moveFactor;
        f32 normalizedForward = forward / length * moveFactor;

        // 根据偏航角计算实际移动方向
        f32 yawRad = m_builtIn.rotation->m_rot.x * math::DEG_TO_RAD;
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        // moveRelative 公式
        f32 moveX = normalizedStrafe * cosYaw - normalizedForward * sinYaw;
        f32 moveZ = normalizedForward * cosYaw + normalizedStrafe * sinYaw;

        // 添加到速度（累加，不是替换）
        m_builtIn.velocity->m_velocity.x += moveX;
        m_builtIn.velocity->m_velocity.z += moveZ;
    }

    // 2. 应用重力或攀爬物理
    if (onLadder) {
        // 在梯子上时的特殊物理
        // 水平速度限制为 0.15，重力被抵消

        // 对齐 vanilla LivingEntity#handleOnClimbable（LivingEntity.java:2570-2572）第一行
        // this.resetFallDistance()：实体在攀爬方块（梯子/藤蔓/脚手架等 isLadder 返回 true 的方块）
        // 上时，每帧 travel 清零 fallDistance。这是攀爬方块摔伤免疫的主机制——实体沿梯子缓慢下滑
        // 时 fallDistance 不累积，离开攀爬方块落地不承受摔落伤害。
        // 此前 Cubium 缺这行，攀爬时 updateFallDistance（Entity.cpp:1074 不考虑攀爬状态，y<0 即累积）
        // 持续累积下滑量，落地触发摔伤，与 vanilla 偏差。
        // 注： FALL_DAMAGE_RESETTING 射线（_checkFallDamageResettingBlocks）是补充机制，仅在本帧
        // 位移>=1.0 时触发；攀爬时位移通常 <1.0 故射线不生效，摔伤免疫依赖此主机制。
        m_builtIn.physicsState->m_fallDistance = 0.0f;

        // 限制水平速度
        f32 horizontalSpeed = std::sqrt(m_builtIn.velocity->m_velocity.x * m_builtIn.velocity->m_velocity.x +
            m_builtIn.velocity->m_velocity.z * m_builtIn.velocity->m_velocity.z);
        constexpr f32 LADDER_MAX_SPEED = 0.15f;
        if (horizontalSpeed > LADDER_MAX_SPEED) {
            f32 scale = LADDER_MAX_SPEED / horizontalSpeed;
            m_builtIn.velocity->m_velocity.x *= scale;
            m_builtIn.velocity->m_velocity.z *= scale;
        }

        // 梯子上的垂直移动
        // 向上爬：Y速度正（输入控制）
        // 向下滑：Y速度负（重力控制，但被限制）
        // 静止：Y速度趋近于 0（缓慢滑落）

        // 如果在移动（按住前进键），允许向上爬
        // 否则应用轻微重力使其缓慢下滑
        if (forward > 0.0f) {
            // 向上攀爬
            m_builtIn.velocity->m_velocity.y = physics::LADDER_CLIMB_SPEED;
        } else if (forward < 0.0f) {
            // 向下滑落（比正常下落慢）
            m_builtIn.velocity->m_velocity.y = -physics::LADDER_SLIDE_SPEED;
        } else {
            // 不按键时，缓慢滑落
            if (m_builtIn.velocity->m_velocity.y < -physics::LADDER_SPEED_MAX) {
                m_builtIn.velocity->m_velocity.y = -physics::LADDER_SPEED_MAX;
            }
        }

        // 不应用正常重力（梯子上重力已被处理）
    } else if (hasEffect(entity::effect::EffectType::Levitation)) {
        // 飘浮效果：每 tick 向上加速 0.05 * (amplifier + 1)，抵消重力。
        // 对应 MC 1.21.11 LivingEntity.travel() 中 Levitation 分支：
        //   if (this.hasEffect(MobEffects.LEVITATION)) {
        //       d4 += (0.05 * (double)(this.getEffect(...).getAmplifier() + 1) - vec3.y) * ...;
        //   }
        // vec3.y 为本帧重力位移，这里等价为"加成替代重力"：飘浮时不应用重力，
        // 仅施加向上加成，并重置摔落距离。
        const i32 level = getEffectLevel(entity::effect::EffectType::Levitation);
        m_builtIn.velocity->m_velocity.y += physics::LEVITATION_LIFT_PER_LEVEL * static_cast<f32>(level);
        m_builtIn.physicsState->m_fallDistance = 0.0f;
    } else if (!hasNoGravity()) {
        // 缓降效果处理
        f32 gravity = GRAVITY;

        if (hasEffect(entity::effect::EffectType::SlowFalling)) {
            // 缓降效果下重力大幅降低
            gravity = physics::SLOW_FALLING_GRAVITY;
            // 同时重置摔落距离
            m_builtIn.physicsState->m_fallDistance = 0.0f;
        }

        // 应用重力
        m_builtIn.velocity->m_velocity.y -= gravity;
    }

    // 3. 执行碰撞移动
    // 注意：moveWithCollision() 内部会根据碰撞结果重置速度
    if (m_builtIn.velocity->m_velocity.x != 0.0f || m_builtIn.velocity->m_velocity.y != 0.0f ||
        m_builtIn.velocity->m_velocity.z != 0.0f) {
        moveWithCollision(
            m_builtIn.velocity->m_velocity.x, m_builtIn.velocity->m_velocity.y, m_builtIn.velocity->m_velocity.z);
    }

    // 4. 应用摩擦/阻力（在移动后）
    if (m_builtIn.physicsState->m_onGround) {
        // 地面摩擦 = slipperiness * 0.91
        f32 groundFriction = slipperiness * 0.91f;
        m_builtIn.velocity->m_velocity.x *= groundFriction;
        m_builtIn.velocity->m_velocity.z *= groundFriction;
    } else if (isInWater()) {
        // 水中阻力
        f32 waterDrag = physics::DRAG_WATER;

        // 海豚的恩惠效果：大幅降低水中阻力
        if (hasEffect(entity::effect::EffectType::DolphinsGrace)) {
            waterDrag = physics::DOLPHINS_GRACE_WATER_DRAG;
        }

        // 深度守卫附魔减少水中阻力影响
        const ItemStack& boots = getEquipment(EquipmentSlot::Feet);
        if (!boots.isEmpty()) {
            i32 depthStriderLevel =
                item::enchant::EnchantmentHelper::getEnchantmentLevel(boots, "minecraft:depth_strider");
            if (depthStriderLevel > 0) {
                // 每级减少阻力差值的 1/3
                // 阻力从 0.8 提升到 max 0.546
                f32 dragReduction = static_cast<f32>(depthStriderLevel) * physics::DEPTH_STRIDER_SPEED_BONUS;
                waterDrag = std::min(waterDrag + dragReduction * (1.0f - waterDrag), physics::DEPTH_STRIDER_MAX_DRAG);
            }
        }

        m_builtIn.velocity->m_velocity.x *= waterDrag;
        m_builtIn.velocity->m_velocity.y *= waterDrag * 0.8f; // 垂直阻力略大
        m_builtIn.velocity->m_velocity.z *= waterDrag;
    } else if (isInLava()) {
        // 岩浆阻力
        m_builtIn.velocity->m_velocity.x *= physics::DRAG_LAVA;
        m_builtIn.velocity->m_velocity.y *= physics::DRAG_LAVA * 0.8f;
        m_builtIn.velocity->m_velocity.z *= physics::DRAG_LAVA;
    } else if (!onLadder) {
        // 空气阻力（不在梯子上）
        m_builtIn.velocity->m_velocity.x *= DRAG_AIR;
        m_builtIn.velocity->m_velocity.y *= DRAG_AIR;
        m_builtIn.velocity->m_velocity.z *= DRAG_AIR;
    } else {
        // 梯子上的阻力
        m_builtIn.velocity->m_velocity.x *= DRAG_GROUND;
        m_builtIn.velocity->m_velocity.z *= DRAG_GROUND;
    }

    // 5. 重置过小的速度
    if (std::abs(m_builtIn.velocity->m_velocity.x) < MOTION_THRESHOLD) m_builtIn.velocity->m_velocity.x = 0.0f;
    if (std::abs(m_builtIn.velocity->m_velocity.y) < MOTION_THRESHOLD) m_builtIn.velocity->m_velocity.y = 0.0f;
    if (std::abs(m_builtIn.velocity->m_velocity.z) < MOTION_THRESHOLD) m_builtIn.velocity->m_velocity.z = 0.0f;
}

// ============================================================================
// 鞘翅飞行（Elytra Glide）
// ============================================================================

bool LivingEntity::canGlide() const
{
    // 对应 MC 1.21.11 LivingEntity.canGlide()
    // 不在地面、非骑乘、无飘浮效果
    if (onGround() || isRiding() || hasEffect(entity::effect::EffectType::Levitation)) {
        return false;
    }

    // 任意装备槽位的物品可滑翔
    for (u8 i = 0; i < static_cast<u8>(EquipmentSlot::Count); ++i) {
        auto slot = static_cast<EquipmentSlot>(i);
        if (canGlideUsing(getEquipment(slot), slot)) {
            return true;
        }
    }
    return false;
}

bool LivingEntity::canGlideUsing(const ItemStack& stack, EquipmentSlot slot)
{
    // 对应 MC 1.21.11 LivingEntity.canGlideUsing(ItemStack, EquipmentSlot)
    // Cubium 当前仅 ElytraItem 提供滑翔能力：
    // 1. 物品必须是 ElytraItem（占用 Chest 槽位）
    // 2. 物品必须可受损且未接近损坏（与 ElytraItem::isUsable 一致）
    if (stack.isEmpty() || !stack.isDamageable()) {
        return false;
    }
    if (slot != EquipmentSlot::Chest) {
        return false;
    }
    const auto* elytra = dynamic_cast<const item::items::ElytraItem*>(stack.getItem());
    if (elytra == nullptr) {
        return false;
    }
    // isUsable 内部判定：getDamage < getMaxDamage - 1
    // 即差 1 点耐久时还能使用，与 MC nextDamageWillBreak() 取反语义一致
    return item::items::ElytraItem::isUsable(stack);
}

bool LivingEntity::tryToStartFallFlying()
{
    // 对应 MC 1.21.11 Player.tryToStartFallFlying()
    // （MC 中此方法仅定义在 Player 中，Cubium 将其上提到 LivingEntity
    // 以便非玩家生物（未来扩展）也能复用基础滑翔逻辑）
    if (!isElytraFlying() && canGlide() && !isInWater()) {
        startFallFlying();
        return true;
    }
    return false;
}

void LivingEntity::startFallFlying()
{
    // 对应 MC 1.21.11 Player.startFallFlying()
    // 设置 EntityFlags::FallFlying 标志位（bit 7）
    addFlag(EntityFlags::FallFlying);
}

void LivingEntity::stopFallFlying()
{
    // 对应 MC 1.21.11 LivingEntity.stopFallFlying()
    // MC 原版通过两次 setSharedFlag(7, true)/setSharedFlag(7, false) 触发数据同步
    // Cubium 的 addFlag/removeFlag 同样会更新 DataManager，无需重复调用
    removeFlag(EntityFlags::FallFlying);
}

void LivingEntity::updateFallFlying()
{
    // 对应 MC 1.21.11 LivingEntity.updateFallFlying()
    // 仅服务端执行
    if (m_world != nullptr && m_world->isClientSide()) {
        return;
    }

    // 不可滑翔时清除飞行标志
    if (!canGlide()) {
        removeFlag(EntityFlags::FallFlying);
        return;
    }

    // 周期性触发：每 10 tick 一次
    const i32 i = m_fallFlyTicks + 1;
    if (i % 10 == 0) {
        const i32 j = i / 10;
        // 偶数次（每 20 tick）随机损坏一件可滑翔装备
        if (j % 2 == 0) {
            // 收集所有可滑翔装备槽位
            std::vector<EquipmentSlot> glideSlots;
            for (u8 k = 0; k < static_cast<u8>(EquipmentSlot::Count); ++k) {
                auto slot = static_cast<EquipmentSlot>(k);
                if (canGlideUsing(getEquipment(slot), slot)) {
                    glideSlots.push_back(slot);
                }
            }
            if (!glideSlots.empty()) {
                // 从可滑翔槽位中随机选一个损坏
                i32 randomIdx = static_cast<i32>(m_random.nextInt(static_cast<i32>(glideSlots.size())));
                EquipmentSlot chosen = glideSlots[static_cast<size_t>(randomIdx)];
                ItemStack& stack = getMutableEquipment(chosen);
                hurtAndBreak(stack, 1, this, chosen);
            }
        }

        // 触发 ELYTRA_GLIDE 游戏事件（通知幽匿感测体）
        if (m_world != nullptr) {
            BlockPos eventPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
                static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
                static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
            m_world->gameEvent(gameevent::GameEvents::ELYTRA_GLIDE, eventPos, gameevent::GameEvent::Context::of(this));
        }
    }
}

void LivingEntity::updateSwimAmount()
{
    // 对应 MC 1.21.11 LivingEntity.updateSwimAmount()
    //   this.swimAmountO = this.swimAmount;
    //   if (this.isVisuallySwimming()) {
    //       this.swimAmount = Math.min(1.0F, this.swimAmount + 0.09F);
    //   } else {
    //       this.swimAmount = Math.max(0.0F, this.swimAmount - 0.09F);
    //   }
    m_swimAmountO = m_swimAmount;
    if (isVisuallySwimming()) {
        m_swimAmount = std::min(1.0f, m_swimAmount + 0.09f);
    } else {
        m_swimAmount = std::max(0.0f, m_swimAmount - 0.09f);
    }
}

bool LivingEntity::isVisuallySwimming() const
{
    // 对应 MC 1.21.11 LivingEntity.isVisuallySwimming()
    //   return super.isVisuallySwimming() || !this.isFallFlying() && this.hasPose(Pose.FALL_FLYING);
    // 即基类的 Swimming 姿态判定，或（未处于 FallFlying 标志但姿态为 FALL_FLYING）的过渡情形。
    // 后者用于玩家在地面准备起飞时的爬行过渡：玩家按下跳跃准备鞘翅飞行时，
    // 姿态先变为 FALL_FLYING 但 FallFlying 标志尚未置位，此时应视为视觉游泳以驱动爬行动画。
    if (Entity::isVisuallySwimming()) {
        return true;
    }
    return !isElytraFlying() && pose() == EntityPose::FallFlying;
}

void LivingEntity::travelFallFlying(const Vector3& travelVec)
{
    // 对应 MC 1.21.11 LivingEntity.travelFallFlying(Vec3)
    (void)travelVec; // Cubium 的输入向量已通过 m_moveStrafing/m_moveForward 体现，
                     // 滑翔物理主要由视线方向驱动，与原版一致

    // 在梯子上时改用常规空中移动并停止飞行
    // 注意：MC 1.21.11 调用 travelInAir（非 travel），避免再次进入 travelFallFlying 分支。
    // Cubium 中 travel() 开头会根据 isElytraFlying() 分发，若直接调用 travel() 会无限递归。
    // 此处先 stopFallFlying() 清除标志，再调用 travel() 走常规分支。
    if (isOnLadder()) {
        stopFallFlying();
        travel(0.0f, 0.0f, 0.0f);
        return;
    }

    // 记录移动前的水平速度（用于撞墙伤害计算）
    const Vector3 velocityBefore = m_builtIn.velocity->m_velocity;
    const f64 prevHorizontalSpeed = std::sqrt(
        static_cast<f64>(velocityBefore.x) * velocityBefore.x + static_cast<f64>(velocityBefore.z) * velocityBefore.z);

    // 计算滑翔后的速度
    m_builtIn.velocity->m_velocity = updateFallFlyingMovement(velocityBefore);

    // 执行移动（碰撞检测）
    if (m_builtIn.velocity->m_velocity.x != 0.0f || m_builtIn.velocity->m_velocity.y != 0.0f ||
        m_builtIn.velocity->m_velocity.z != 0.0f) {
        moveWithCollision(
            m_builtIn.velocity->m_velocity.x, m_builtIn.velocity->m_velocity.y, m_builtIn.velocity->m_velocity.z);
    }

    // 服务端检测撞墙伤害
    if (m_world != nullptr && !m_world->isClientSide()) {
        const f64 currHorizontalSpeed =
            std::sqrt(static_cast<f64>(m_builtIn.velocity->m_velocity.x) * m_builtIn.velocity->m_velocity.x +
                static_cast<f64>(m_builtIn.velocity->m_velocity.z) * m_builtIn.velocity->m_velocity.z);
        handleFallFlyingCollisions(prevHorizontalSpeed, currHorizontalSpeed);
    }
}

Vector3 LivingEntity::updateFallFlyingMovement(const Vector3& currentVelocity) const
{
    // 对应 MC 1.21.11 LivingEntity.updateFallFlyingMovement(Vec3)
    Vector3 look = getLookAngle();
    const f32 pitchRad = m_builtIn.rotation->m_rot.y * math::DEG_TO_RAD;
    const f64 d0 = std::sqrt(static_cast<f64>(look.x) * look.x + static_cast<f64>(look.z) * look.z); // 视线水平分量长度
    const f64 d1 = std::sqrt(static_cast<f64>(currentVelocity.x) * currentVelocity.x +
        static_cast<f64>(currentVelocity.z) * currentVelocity.z); // 速度水平分量长度
    const f64 d2 = getEffectiveGravity();
    const f64 d3 = static_cast<f64>(std::cos(pitchRad)) * static_cast<f64>(std::cos(pitchRad)); // cos²(pitch)

    Vector3 velocity = currentVelocity;
    // 重力被 cos²(pitch) * 0.75 部分抵消
    velocity.y += static_cast<f32>(d2 * (-1.0 + d3 * 0.75));

    // 俯冲时（y<0）将向下速度转化为前方加速
    if (velocity.y < 0.0f && d0 > 0.0) {
        const f64 d4 = static_cast<f64>(velocity.y) * -0.1 * d3;
        velocity.x += static_cast<f32>(static_cast<f64>(look.x) * d4 / d0);
        velocity.y += static_cast<f32>(d4);
        velocity.z += static_cast<f32>(static_cast<f64>(look.z) * d4 / d0);
    }

    // 抬头时（pitch<0）将水平速度转化为向上爬升
    if (pitchRad < 0.0f && d0 > 0.0) {
        const f64 d5 = d1 * -static_cast<f64>(std::sin(pitchRad)) * 0.04;
        velocity.x -= static_cast<f32>(static_cast<f64>(look.x) * d5 / d0);
        velocity.y += static_cast<f32>(d5 * 3.2);
        velocity.z -= static_cast<f32>(static_cast<f64>(look.z) * d5 / d0);
    }

    // 水平分量朝视线方向缓慢对齐（lerp 0.1）
    if (d0 > 0.0) {
        velocity.x += static_cast<f32>((static_cast<f64>(look.x) / d0 * d1 - static_cast<f64>(velocity.x)) * 0.1);
        velocity.z += static_cast<f32>((static_cast<f64>(look.z) / d0 * d1 - static_cast<f64>(velocity.z)) * 0.1);
    }

    // 应用阻力 0.99/0.98/0.99
    velocity.x *= 0.99f;
    velocity.y *= 0.98f;
    velocity.z *= 0.99f;
    return velocity;
}

void LivingEntity::handleFallFlyingCollisions(f64 prevHorizontalSpeed, f64 currHorizontalSpeed)
{
    // 对应 MC 1.21.11 LivingEntity.handleFallFlyingCollisions(double, double)
    if (!m_builtIn.physicsState->m_collidedHorizontally) {
        return;
    }
    const f64 d0 = prevHorizontalSpeed - currHorizontalSpeed;
    const f32 f = static_cast<f32>(d0 * 10.0 - 3.0);
    if (f > 0.0f) {
        // 播放摔落音效（与摔落伤害共用）
        i32 fallHeight = static_cast<i32>(f);
        auto fallSound = getFallSound(fallHeight);
        if (fallSound.has_value()) {
            playSound(*fallSound, 1.0f, 1.0f);
        }
        // 施加撞墙伤害
        auto source = DamageSources::flyIntoWall();
        hurt(source, f);
    }
}

Vector3 LivingEntity::getLookAngle() const
{
    // 对应 MC 1.21.11 Entity.getLookAngle()
    // 与 Player::getLookVector 算法一致：
    // MC 坐标系：yaw=0 看向 +Z，yaw=90 看向 -X
    // pitch 正值向下看，负值向上看
    const f32 yawRad = math::toRadians(m_builtIn.rotation->m_rot.x);
    const f32 pitchRad = math::toRadians(m_builtIn.rotation->m_rot.y);
    const f32 cosYaw = std::cos(yawRad);
    const f32 sinYaw = std::sin(yawRad);
    const f32 cosPitch = std::cos(pitchRad);
    const f32 sinPitch = std::sin(pitchRad);
    return Vector3(-sinYaw * cosPitch, -sinPitch, cosYaw * cosPitch).normalized();
}

f64 LivingEntity::getEffectiveGravity() const
{
    // 对应 MC 1.21.11 LivingEntity.getEffectiveGravity()
    // 向下移动且有缓降效果时，重力被钳制到最大 0.01
    const bool movingDown = m_builtIn.velocity->m_velocity.y <= 0.0;
    if (movingDown && hasEffect(entity::effect::EffectType::SlowFalling)) {
        return std::min(getAttributeValue(entity::attribute::Attributes::ENTITY_GRAVITY, GRAVITY), 0.01);
    }
    return getAttributeValue(entity::attribute::Attributes::ENTITY_GRAVITY, GRAVITY);
}

// ============================================================================
// 效果系统
// ============================================================================

bool LivingEntity::addEffect(entity::effect::EffectInstance effect)
{
    return m_effectManager.addEffect(std::move(effect), *this);
}

void LivingEntity::removeEffect(entity::effect::EffectType type)
{
    m_effectManager.removeEffect(type, *this);
}

void LivingEntity::removeAllEffects()
{
    m_effectManager.removeAllEffects(*this);
}

bool LivingEntity::hasEffect(entity::effect::EffectType type) const
{
    return m_effectManager.hasEffect(type);
}

const entity::effect::EffectInstance* LivingEntity::getEffect(entity::effect::EffectType type) const
{
    return m_effectManager.getEffect(type);
}

i32 LivingEntity::getEffectLevel(entity::effect::EffectType type) const
{
    return m_effectManager.getEffectLevel(type);
}

// ============================================================================
// 箭矢计数
// ============================================================================

void LivingEntity::setArrowCountInEntity(i32 count)
{
    const i32 clamped = std::max(0, count);
    // 组件为真相源，DATA_ARROW_COUNT_PARAM 退为同步镜像。
    if (auto* c = m_entityContext->tryGetComponent<ecs::ArrowStateComponent>()) {
        c->m_arrowCount = clamped;
    }
    m_dataManager.set(DATA_ARROW_COUNT_PARAM, clamped);
}

i32 LivingEntity::getStingerCount() const
{
    const auto* c = m_entityContext->tryGetComponent<ecs::ArrowStateComponent>();
    return c != nullptr ? c->m_stingerCount : 0;
}

void LivingEntity::setStingerCountInEntity(i32 count)
{
    const i32 clamped = std::max(0, count);
    // 组件为真相源，DATA_STINGER_COUNT_PARAM 退为同步镜像。
    if (auto* c = m_entityContext->tryGetComponent<ecs::ArrowStateComponent>()) {
        c->m_stingerCount = clamped;
    }
    m_dataManager.set(DATA_STINGER_COUNT_PARAM, clamped);
}

void LivingEntity::tickArrows()
{
    // 箭矢自动脱落逻辑
    // 仅在服务端执行
    if (world() == nullptr || world()->isClientSide()) {
        return;
    }

    auto* arrowState = m_entityContext->tryGetComponent<ecs::ArrowStateComponent>();
    if (arrowState == nullptr) {
        return;
    }

    if (arrowState->m_arrowCount > 0) {
        // 如果计时器未启动，初始化计时器
        // 公式: 20 * (30 - arrowCount) ticks
        // 箭矢越多，脱落越快
        if (arrowState->m_arrowHitTimer <= 0) {
            arrowState->m_arrowHitTimer = 20 * (30 - arrowState->m_arrowCount);
        }

        --arrowState->m_arrowHitTimer;
        if (arrowState->m_arrowHitTimer <= 0) {
            // 计时器归零，减少一支箭
            setArrowCountInEntity(arrowState->m_arrowCount - 1);
        }
    }
}

// ============================================================================
// 攻击附魔回调
// ============================================================================

void LivingEntity::onAttackEntity(Entity& target)
{
    // 获取主手武器上的附魔，触发 onEntityDamaged 回调
    const ItemStack& mainHand = getMainHandItem();
    if (!mainHand.isEmpty()) {
        item::enchant::EnchantmentHelper::applyArthropodEnchantmentDamage(*this, target, mainHand);
    }
}

// ============================================================================
// 受伤追踪（Target Goals 使用）
// ============================================================================

// 40 tick 过期阈值（对齐 vanilla LivingEntity.getLastDamageSource:1391-1397 的 lastDamageStamp 守卫）。
// vanilla: if (level.getGameTime() - lastDamageStamp > 40L) lastDamageSource = null。
// Cubium 用实体 ticksExisted() 替代 level.getGameTime()（实体 tick 计数，等价相对时间判定）。
static constexpr u32 LAST_DAMAGE_SOURCE_EXPIRY_TICKS = 40;

DamageSource* LivingEntity::lastDamageSource() const
{
    // 对齐 vanilla 40 tick 过期守卫：超过 40 tick 置空 m_lastDamageSource。
    // 此守卫缩小 m_lastDamageSource（clone 持真凶裸指针）的悬垂窗口，并对齐 vanilla 语义。
    // 注意：40 tick 内真凶析构仍致 getTrueSource() 悬垂，取攻击者须用 lastDamageSourceTrueId()
    // 经 world 校验，不可直接解引用 lastDamageSource()->getTrueSource()（任务 #272 UAF）。
    if (m_lastDamageSource != nullptr && ticksExisted() - m_lastDamageStamp > LAST_DAMAGE_SOURCE_EXPIRY_TICKS) {
        // const 方法内清除需 const_cast。对齐 vanilla getLastDamageSource 内 this.lastDamageSource = null。
        const_cast<LivingEntity*>(this)->m_lastDamageSource.reset();
        const_cast<LivingEntity*>(this)->m_lastDamageSourceTrueId = INVALID_ENTITY_ID;
    }
    return m_lastDamageSource.get();
}

EntityInstanceId LivingEntity::lastDamageSourceTrueId() const
{
    // 同样受 40 tick 过期守卫约束：超期则视为无最近伤害来源。
    if (m_lastDamageSource != nullptr && ticksExisted() - m_lastDamageStamp > LAST_DAMAGE_SOURCE_EXPIRY_TICKS) {
        const_cast<LivingEntity*>(this)->m_lastDamageSource.reset();
        const_cast<LivingEntity*>(this)->m_lastDamageSourceTrueId = INVALID_ENTITY_ID;
    }
    return m_lastDamageSourceTrueId;
}

void LivingEntity::setLastHurtBy(LivingEntity* attacker)
{
    m_lastHurtBy = attacker;
    m_lastHurtByTimestamp = ticksExisted();
}

void LivingEntity::setLastHurtTarget(LivingEntity* target)
{
    m_lastHurtTarget = target;
    m_lastHurtTargetTimestamp = ticksExisted();
}

// ============================================================================
// 击退
// ============================================================================

void LivingEntity::applyKnockback(f32 strength, f64 ratioX, f64 ratioZ)
{
    // 击退强度会被击退抗性降低
    strength = static_cast<f32>(static_cast<f64>(strength) *
        (1.0 - getAttributeValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0)));

    if (strength <= 0.0f) {
        return; // 击退被完全抗性抵消
    }

    // 归一化方向向量
    // 如果方向向量过小（长度平方 < 1.0E-5），随机扰动方向以避免零向量
    f64 lengthSquared = ratioX * ratioX + ratioZ * ratioZ;
    if (lengthSquared < 1.0E-5) {
        auto& rng = m_world->getRandom();
        ratioX = static_cast<f64>(rng.nextFloat() - rng.nextFloat()) * 0.01;
        ratioZ = static_cast<f64>(rng.nextFloat() - rng.nextFloat()) * 0.01;
        lengthSquared = ratioX * ratioX + ratioZ * ratioZ;
    }

    f64 length = std::sqrt(lengthSquared);
    ratioX /= length;
    ratioZ /= length;

    // 计算击退速度
    // 击退会减少当前水平速度的一半，然后加上击退向量
    f64 knockbackX = ratioX * static_cast<f64>(strength);
    f64 knockbackZ = ratioZ * static_cast<f64>(strength);

    // Y轴速度
    f64 newVelocityY;
    if (m_builtIn.physicsState->m_onGround) {
        // 在地面时：Y速度 = min(0.4, 当前Y速度/2 + 击退强度)
        newVelocityY =
            std::min(0.4, static_cast<f64>(m_builtIn.velocity->m_velocity.y) / 2.0 + static_cast<f64>(strength));
    } else {
        // 在空中时：保持当前Y速度
        newVelocityY = static_cast<f64>(m_builtIn.velocity->m_velocity.y);
    }

    // 设置新速度
    // X轴：当前速度的一半减去击退向量
    // Z轴：当前速度的一半减去击退向量
    m_builtIn.velocity->m_velocity.x =
        static_cast<f32>(static_cast<f64>(m_builtIn.velocity->m_velocity.x) / 2.0 - knockbackX);
    m_builtIn.velocity->m_velocity.y = static_cast<f32>(newVelocityY);
    m_builtIn.velocity->m_velocity.z =
        static_cast<f32>(static_cast<f64>(m_builtIn.velocity->m_velocity.z) / 2.0 - knockbackZ);

    // 设置为空中状态
    m_builtIn.physicsState->m_onGround = false;

    // 标记受伤（击退改变了速度，需要同步到客户端）
    markHurt();
}

void LivingEntity::applyKnockbackFrom(LivingEntity* attacker, f32 strength)
{
    if (attacker == nullptr) {
        return;
    }

    // 从攻击者位置计算击退方向
    f64 ratioX = static_cast<f64>(attacker->position().x - m_builtIn.stateVector->m_pos.x);
    f64 ratioZ = static_cast<f64>(attacker->position().z - m_builtIn.stateVector->m_pos.z);

    applyKnockback(strength, ratioX, ratioZ);
}

void LivingEntity::causeExtraKnockback(Entity& target, f32 strength, const Vector3& /*preHurtVelocity*/)
{
    // 基类版本：如果击退强度 > 0 且目标是 LivingEntity，则对目标施加击退
    // 同时减缓攻击者的水平速度
    // 注意：setSprinting(false) 仅在 Player 子类中调用
    if (strength > 0.0f) {
        if (auto* livingTarget = dynamic_cast<LivingEntity*>(&target)) {
            // 击退方向基于攻击者的朝向
            f32 yawRad = math::toRadians(yaw());
            f64 sinYaw = static_cast<f64>(std::sin(yawRad));
            f64 cosYaw = static_cast<f64>(-std::cos(yawRad));
            livingTarget->applyKnockback(strength, sinYaw, cosYaw);
        } else {
            // 非生物实体使用 push
            f32 yawRad = math::toRadians(yaw());
            target.addVelocity(-std::sin(yawRad) * strength, 0.1f, std::cos(yawRad) * strength);
        }

        // 减缓攻击者的水平速度
        Vector3 vel = velocity();
        setVelocity(vel.x * 0.6f, vel.y, vel.z * 0.6f);
    }

    // 注意：Player 子类重写此方法，添加 setSprinting(false) 和 ServerPlayer 速度修正
}

f32 LivingEntity::getKnockback(Entity& /*target*/)
{
    // 击退强度 = (ATTACK_KNOCKBACK 属性 + 击退附魔加成) / 2.0（对齐 vanilla LivingEntity.java:1515-1520）。
    // vanilla：getKnockback = (getAttributeValue(ATTACK_KNOCKBACK) + EnchantmentHelper.modifyKnockback) / 2.0F
    //   modifyKnockback 累加 KNOCKBACK 附魔组件值（linear base=1.0, per_level=1.0 → 每级 +1.0）。
    //   玩家 ATTACK_KNOCKBACK 属性默认 0，故 Knockback I = (0+1.0)/2.0 = 0.5；II = (0+2.0)/2.0 = 1.0。
    // Cubium：KnockbackEnchantment::getKnockbackBonus 每级 +1.0（对齐 KNOCKBACK 组件），此处再 /2.0。
    f32 baseKnockback = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 0.0));

    // 加上击退附魔加成（每级 +1.0，对齐 vanilla KNOCKBACK 组件 linear base=1.0）
    const ItemStack& weapon = getMainHandItem();
    if (!weapon.isEmpty()) {
        i32 knockbackLevel =
            item::enchant::EnchantmentHelper::getEnchantmentLevel(weapon, &item::enchant::AllEnchantments::KNOCKBACK);
        if (knockbackLevel > 0) {
            baseKnockback += item::enchant::KnockbackEnchantment::getKnockbackBonus(knockbackLevel);
        }
    }

    // /2.0 对齐 vanilla（hurt 通用击退 0.4 之外，causeExtraKnockback 的附魔/冲刺击退强度由此方法提供）。
    return baseKnockback / 2.0f;
}

// ============================================================================
// 物品使用
// ============================================================================

void LivingEntity::setActiveHand(Hand hand)
{
    ItemStack heldItem = getEquipment(hand == Hand::MainHand ? EquipmentSlot::MainHand : EquipmentSlot::OffHand);

    if (heldItem.isEmpty()) {
        return;
    }

    i32 useDuration = heldItem.getItem()->getUseDuration(heldItem);
    if (useDuration <= 0) {
        return;
    }

    m_activeHand = hand;
    m_activeItem = heldItem;
    m_activeItemUseCount = useDuration;
}

void LivingEntity::stopActiveHand()
{
    if (!isUsingItem()) {
        return;
    }

    // 调用物品的 onPlayerStoppedUsing
    // 注意：const_cast 是安全的，因为 Items 在注册后是不可变的
    const Item* item = m_activeItem.getItem();
    if (!m_activeItem.isEmpty() && item != nullptr) {
        // 传 m_activeItem 引用（非拷贝）：onPlayerStoppedUsing 内的 hurtAndBreak（弓/十字弓/三叉戟耐久
        // 损耗）/shrink（三叉戟投掷消耗数量）/setTag（十字弓装填箭矢）等写操作须作用于 m_activeItem，
        // 再由下方 setEquipment 回写权威装备槽。此前传 stackCopy 拷贝不回写，致弓/十字弓/三叉戟耐久损耗
        // 与三叉戟投掷消耗不回写权威手持（对齐缺陷，同 itemInteractionForEntity 拷贝不回写范式）。
        // 回写对齐 onItemUseFinish（line 2204 setEquipment(result)）范式。
        const_cast<Item*>(item)->onPlayerStoppedUsing(m_activeItem, *m_world, *this, m_activeItemUseCount);
        // 回写权威装备槽：onPlayerStoppedUsing 可能改 m_activeItem（耐久损耗/数量消耗/NBT），须回写。
        // 创造模式 onPlayerStoppedUsing 内部跳过 hurtAndBreak/shrink（各物品 isCreative 守卫），m_activeItem
        // 不变，回写无害（覆盖为相同值）。
        setEquipment(m_activeHand == Hand::MainHand ? EquipmentSlot::MainHand : EquipmentSlot::OffHand, m_activeItem);
    }

    // 重置状态
    m_activeItem = ItemStack();
    m_activeItemUseCount = 0;
}

void LivingEntity::updateActiveItem()
{
    if (!isUsingItem()) {
        return;
    }

    // 递减使用计时器
    m_activeItemUseCount--;

    // 计算已使用的tick数（1-based）
    if (!m_activeItem.isEmpty()) {
        const Item* item = m_activeItem.getItem();
        if (item != nullptr) {
            i32 totalDuration = item->getUseDuration(m_activeItem);
            i32 elapsedTicks = totalDuration - m_activeItemUseCount;
            // 注意：const_cast 是安全的，因为 Items 在注册后是不可变的
            const_cast<Item*>(item)->onUseTick(m_activeItem, *m_world, *this, elapsedTicks);
        }
    }

    // 对齐 Java LivingEntity#updateUsingItem：先 onUseTick 再递减，递减到 0 即 completeUsingItem
    // （onItemUseFinish）。此处不再用 isUsingItem() 中间检查——递减后 useCount=0 会使 isUsingItem()
    // 返回 false，导致提前 return 跳过下方完成分支，onItemUseFinish 永不触发（食物食用完成链路断裂）。
    // onUseTick 内若调 stopActiveHand（如 BrushItem 非玩家/未对准方块分支）会清空 m_activeItem，
    // 下方完成分支的 `if (!m_activeItem.isEmpty())` 守卫已挡住误触 onItemUseFinish，故无需此处再判。

    // 检查是否完成使用
    if (m_activeItemUseCount <= 0) {
        // 使用完成
        const Item* item = m_activeItem.getItem();
        if (!m_activeItem.isEmpty() && item != nullptr) {
            // 注意：const_cast 是安全的，因为 Items 在注册后是不可变的
            ItemStack result = const_cast<Item*>(item)->onItemUseFinish(m_activeItem, *m_world, *this);

            // 派发自定义物品组件回调 - onCompleteUse
            auto& itemCompReg = mc::mod::bedrock::addon::ItemComponentRegistry::instance();
            std::string itemTypeId = item->itemLocation().toString();
            if (itemCompReg.hasCompleteUseCallback(itemTypeId)) {
                mc::mod::bedrock::addon::ItemComponentCompleteUseEvent compEvent;
                compEvent.itemTypeId = itemTypeId;
                compEvent.sourceId = id();
                compEvent.useDuration = item->getUseDuration(m_activeItem);
                compEvent.itemStackAmount = m_activeItem.getCount();
                itemCompReg.dispatchCompleteUse(itemTypeId, compEvent);
            }

            // 更新装备槽
            setEquipment(m_activeHand == Hand::MainHand ? EquipmentSlot::MainHand : EquipmentSlot::OffHand, result);
        }
        m_activeItem = ItemStack();
        m_activeItemUseCount = 0;
    }
}

// ============================================================================
// 空气供应和溺水
// ============================================================================

bool LivingEntity::isInvertedHealAndHarm() const
{
    // 查询 INVERTED_HEALING_AND_HARM 标签判定瞬间治疗/伤害反转。标签成员=亡灵
    // （#undead 派生）。瞬间治疗/伤害用本方法判定反转分支（HealOrHarmMobEffect：
    // isHarm==isInvertedHealAndHarm 时治疗，否则伤害）。
    // 标签未初始化（极早期/测试未初始化）回退 getCreatureAttribute==Undead 保持原行为避免回归。
    if (!EntityTypeTags::isInitialized()) {
        return getCreatureAttribute() == CreatureAttribute::Undead;
    }
    return EntityTypeTags::INVERTED_HEALING_AND_HARM().contains(getTypeId());
}

bool LivingEntity::canBreatheUnderwater() const
{
    // 对齐 vanilla 1.21.11 LivingEntity.canBreatheUnderwater():385：
    //   return this.getType().is(EntityTypeTags.CAN_BREATHE_UNDER_WATER);
    // 标签成员=亡灵（#undead）+ 水生生物（guardian/elder_guardian/axolotl/frog/turtle/
    // glow_squid/鱼/鱿鱼/tadpole/armor_stand/copper_golem/nautilus）。
    //
    // 此前实现仅查 getCreatureAttribute()==Undead，遗漏 guardian/elder_guardian（Water 属性、
    // 继承 MonsterEntity 非 WaterMobEntity，走基类 updateAirSupply）→ 守卫者在水中溺水扣血，
    // 与 vanilla 相反（vanilla 守卫者永久水下生存）。改查标签对齐 vanilla，所有标签成员自动正确。
    // 注：WaterMobEntity 子类（鱼/鱿鱼/海龟/美西螈等）override updateAirSupply 独立处理空气
    // （水中回满、陆地消耗），不调用本方法，故本方法改查标签不影响其行为。
    if (!EntityTypeTags::isInitialized()) {
        // 标签未初始化（极早期/测试未初始化）回退到亡灵判定，保持原行为避免回归。
        return getCreatureAttribute() == CreatureAttribute::Undead;
    }
    return EntityTypeTags::CAN_BREATHE_UNDER_WATER().contains(getTypeId());
}

i32 LivingEntity::decreaseAirSupply(i32 currentAir)
{
    // 对齐 vanilla 1.21.11 LivingEntity.decreaseAirSupply（LivingEntity.java:571-582）：
    //   AttributeInstance ai = getAttribute(OXYGEN_BONUS);
    //   double d0 = (ai != null) ? ai.getValue() : 0.0;
    //   return d0 > 0.0 && random.nextDouble() >= 1.0 / (d0 + 1.0) ? currentAir : currentAir - 1;
    // oxygen_bonus 默认 0.0（每 tick 必消耗 1 点），水下呼吸魔咒经 enchantment.respiration
    // 修饰符（每级 +1.0 ADD_VALUE）注入，使 d0=level，仅 1/(level+1) 概率消耗：
    //   I级 50%、II级 66.7%、III级 75% 不消耗（与原硬编码 random.nextInt(level+1)>0 等价）。
    // 此前硬编码读 getRespirationLevel(helmet) 绕过属性体系，任何经属性修饰符（非附魔）
    // 改变氧气消耗概率的机制均无法生效——现已统一走 oxygen_bonus 属性。
    const f64 oxygenBonus = attributes().getValue(entity::attribute::Attributes::OXYGEN_BONUS, 0.0);

    if (oxygenBonus > 0.0 && m_world != nullptr) {
        math::Random& random = m_world->getRandom();
        // 对齐 vanilla：random.nextDouble() >= 1.0/(d0+1.0) 时不消耗
        if (random.nextDouble() >= 1.0 / (oxygenBonus + 1.0)) {
            return currentAir; // 属性生效，不消耗空气
        }
    }

    return currentAir - 1;
}

i32 LivingEntity::increaseAirSupply(i32 currentAir) const
{
    // 每tick恢复4点空气，上限为 maxAir()
    // MC Java: LivingEntity.increaseAirSupply()
    return std::min(currentAir + 4, maxAir());
}

i32 LivingEntity::determineNextAir(i32 currentAir) const
{
    // 委托给 increaseAirSupply()
    return increaseAirSupply(currentAir);
}

bool LivingEntity::shouldTakeDrowningDamage() const
{
    // MC Java: LivingEntity.shouldTakeDrowningDamage()
    // 当空气值降到 -20 或以下时触发溺水伤害
    return air() <= -20;
}

void LivingEntity::updateAirSupply()
{
    // MC Java: LivingEntity.baseTick() 中的空气处理逻辑
    if (!isAlive()) {
        return;
    }

    // 仅在服务端处理空气逻辑（MC Java 仅在 ServerLevel 中处理）
    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    // 检查实体是否在水中，并排除气泡柱
    // MC Java 使用 isEyeInFluid(FluidTags.WATER) 检测眼部位置，
    // 当前项目使用 isInWater() 作为等价检测（实体身体在水下通常意味着眼睛也在水下），
    // 未来可切换为 areEyesInWater() 以精确检测眼部流体位置
    // MC Java: isEyeInFluid(FluidTags.WATER) && !level.getBlockState(blockPos).is(Blocks.BUBBLE_COLUMN)
    bool inWater = isInWater();
    bool inBubbleColumn = false;

    if (inWater && m_world != nullptr) {
        // 计算实体所在方块位置（使用眼睛高度）
        // MC Java: BlockPos.containing(this.getX(), this.getEyeY(), this.getZ())
        f32 eyeY = m_builtIn.stateVector->m_pos.y + eyeHeight();
        BlockPos eyeBlockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
            static_cast<i32>(std::floor(eyeY)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
        const BlockState* eyeState = m_world->getBlockState(eyeBlockPos);
        if (eyeState != nullptr && eyeState->is(VanillaBlocks::BUBBLE_COLUMN)) {
            inBubbleColumn = true;
        }
    }

    if (inWater && !inBubbleColumn) {
        // 在水中且不在气泡柱中
        // 检查是否需要消耗空气
        // MC Java: !this.canBreatheUnderwater() && !MobEffectUtil.hasWaterBreathing(this)
        //          && (!flag || !((Player)this).getAbilities().invulnerable)
        // 其中 flag = (this instanceof Player)
        bool canBreathe = canBreatheUnderwater() || hasEffect(entity::effect::EffectType::WaterBreathing) ||
            hasEffect(entity::effect::EffectType::ConduitPower);

        // 玩家的无敌模式检查由 Player::updateAirSupply() 在调用基类之前处理
        // 此处仅检查非玩家实体的呼吸条件

        if (!canBreathe) {
            // 需要消耗空气
            i32 newAir = decreaseAirSupply(air());
            setAir(newAir);

            // 溺水伤害判定
            // MC Java: if (this.shouldTakeDrowningDamage()) { setAirSupply(0); broadcastEntityEvent(67);
            // hurtServer(drown, 2.0F) }
            if (shouldTakeDrowningDamage()) {
                setAir(0);

                // 广播溺水实体事件（客户端用于播放溺水动画/音效）
                // MC Java: serverlevel.broadcastEntityEvent(this, (byte)67)
                m_world->broadcastEntityStatus(id(), static_cast<u8>(67));

                // 造成溺水伤害
                EnvironmentalDamage drownSource = DamageSources::drown();
                hurt(drownSource, physics::DROWN_DAMAGE_AMOUNT);
            }
        } else if (air() < maxAir()) {
            // 可以在水下呼吸且空气未满时恢复空气
            // MC Java: MobEffectUtil.shouldEffectsRefillAirsupply(this) 检查
            // 当有水下呼吸或潮涌效果时恢复空气
            setAir(increaseAirSupply(air()));
        }

        // 水下骑乘强制下坐骑检测
        // MC Java: if (this.isPassenger() && this.getVehicle() != null && this.getVehicle().dismountsUnderwater())
        // 当乘客眼睛位置在水中时，如果所骑乘的载具类型属于 dismounts_underwater 标签，
        // 则强制乘客下坐骑。马、猪、骆驼等陆地骑乘实体会强制下坐骑，船则不会。
        if (isRiding()) {
            EntityInstanceId vehicleId = getVehicle();
            if (vehicleId != INVALID_ENTITY_ID && m_world != nullptr) {
                Entity* vehicle = m_world->getEntity(vehicleId);
                if (vehicle != nullptr && vehicle->dismountsUnderwater()) {
                    stopRiding();
                }
            }
        }
    } else if (air() < maxAir()) {
        // 不在水中（或空气未满），恢复空气
        // MC Java: else if (this.getAirSupply() < this.getMaxAirSupply()) { setAirSupply(increaseAirSupply) }
        setAir(increaseAirSupply(air()));
    }
}

// ============================================================================
// 三叉戟激流攻击
// ============================================================================

bool LivingEntity::isSpinAttacking() const
{
    // 检查 LIVING_FLAGS 的第2位（0x04）
    // LIVING_FLAGS 位定义：
    // - 位 0 (0x01): 是否正在使用物品 (isHandActive)
    // - 位 1 (0x02): 使用的手（0=主手，1=副手）
    // - 位 2 (0x04): 是否正在旋转攻击（三叉戟激流）
    i8 flags = m_dataManager.get<i8>(DATA_LIVING_FLAGS_PARAM);
    return (flags & 0x04) != 0;
}

void LivingEntity::startSpinAttack(i32 duration)
{
    // 设置旋转攻击持续时间和标志
    m_spinAttackDuration = duration;

    // 设置 LIVING_FLAGS 的第2位（0x04）
    // 只在服务端设置，客户端通过数据参数同步
    if (m_world == nullptr || !m_world->isClientSide()) {
        i8 flags = m_dataManager.get<i8>(DATA_LIVING_FLAGS_PARAM);
        flags |= 0x04;
        m_dataManager.set(DATA_LIVING_FLAGS_PARAM, flags);
    }
}

void LivingEntity::stopSpinAttack()
{
    // 清除旋转攻击状态
    m_spinAttackDuration = 0;

    // 清除 LIVING_FLAGS 的第2位（0x04）
    // 只在服务端设置，客户端通过数据参数同步
    if (m_world == nullptr || !m_world->isClientSide()) {
        i8 flags = m_dataManager.get<i8>(DATA_LIVING_FLAGS_PARAM);
        flags &= ~0x04;
        m_dataManager.set(DATA_LIVING_FLAGS_PARAM, flags);
    }
}

void LivingEntity::updateSpinAttack()
{
    // 每tick递减持续时间，归零时停止
    if (m_spinAttackDuration > 0) {
        m_spinAttackDuration--;

        // 检查是否结束
        if (m_spinAttackDuration <= 0) {
            stopSpinAttack();
        } else if (isWet()) {
            // 在水中或雨中时，激流攻击会有额外的上升速度
            // 注意：这部分在 Player 的 travel() 中更详细实现
        }
    }
}

// ============================================================================
// 药水效果和摔落免疫
// ============================================================================

bool LivingEntity::isPotionApplicable(const entity::effect::EffectInstance& effect) const
{
    // 对齐 MC Java 1.21.11 LivingEntity.canBeAffected（LivingEntity.java:1014-1024）。
    // vanilla 用 EntityTypeTags 标签判定免疫：
    //   - IGNORES_POISON_AND_REGEN（亡灵 13 种 + iron_golem）免疫 REGENERATION + POISON
    //   - IMMUNE_TO_INFESTED 免疫 INFESTED；IMMUNE_TO_OOZING 免疫 OOZING
    // Cubium EntityTypeTags::IGNORES_POISON_AND_REGEN 标签成员与 vanilla 完全一致
    // （EntityTypeTags.cpp:629-645，13 亡灵 + iron_golem）。此处实现 Poison/Regen 免疫；
    // INFESTED/OOZING 效果 Cubium 暂未实现，留 TODO 待效果就绪后补 IMMUNE_TO_* 标签判定。
    //
    // 亡灵免疫 Poison/Regen 是 vanilla 核心语义：亡灵被中毒效果不应扣血、被再生效果不应治疗。
    // 此前 Cubium EffectManager::addEffect 不调 isPotionApplicable，亡灵会被中毒扣血——与 vanilla 相反。
    if (EntityTypeTags::IGNORES_POISON_AND_REGEN().contains(getTypeId())) {
        if (effect.type() == entity::effect::EffectType::Poison ||
            effect.type() == entity::effect::EffectType::Regeneration) {
            return false;
        }
    }
    // TODO: 对齐 vanilla IMMUNE_TO_INFESTED（免疫 INFESTED）/ IMMUNE_TO_OOZING（免疫 OOZING）标签判定，
    //       待 Infested/Oozing 效果实现后补全（EntityTypeTags::IMMUNE_TO_INFESTED/IMMUNE_TO_OOZING 已定义）。
    return true;
}

bool LivingEntity::onLivingFall(f32 distance, f32 damageMultiplier)
{
    // 默认实现：处理摔落伤害
    // 子类可重写此方法来免疫摔落伤害（如凋灵、末影龙）
    MC_UNUSED(distance);
    MC_UNUSED(damageMultiplier);
    return true;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void LivingEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    Entity::addAdditionalSaveData(tag);

    // Health / AbsorptionAmount / HurtTime / DeathTime / Equipment 已迁入组件序列化器注册表，
    // 经 Entity::writeToNBT 的 saveAll 写出（批次6 子目标1 Step4），此处不再重复写。

    // HurtByTimestamp (i32) — 纯 OOP 字段（m_lastDamageTimestamp 未组件化），保留直写
    tag.put(nbt_keys::HURT_BY_TIMESTAMP, m_lastDamageTimestamp);

    // FallFlying 已上提为按 EntityFlagsComponent 注册的组件序列化器，经
    // Entity::writeToNBT 的 saveAll 写出（批次6 子目标1），此处不再重复写。

    // ActiveEffects - 药水效果列表
    const auto& effects = m_effectManager.getAllEffects();
    if (!effects.empty()) {
        auto effectsList = std::make_unique<nbt::tags::compound_list_tag>();
        for (const auto& effect : effects) {
            nbt::tags::compound_tag effectTag;
            effect.toNbt(effectTag);
            effectsList->value.push_back(std::move(effectTag));
        }
        tag.value.emplace(nbt_keys::ACTIVE_EFFECTS, std::move(effectsList));
    }

    // Attributes - 属性列表
    nbt_helper::writeAttributeMap(tag, nbt_keys::ATTRIBUTES, attributes());

    // Equipment 已迁入组件序列化器（见上注释）。
}

Result<void> LivingEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    MC_TRY(Entity::readAdditionalSaveData(tag));

    // Health / AbsorptionAmount / HurtTime / DeathTime / Equipment 已迁入组件序列化器注册表，
    // 经 Entity::readFromNBT 的 loadAll 读回（批次6 子目标1 Step4），此处不再重复读。

    // HurtByTimestamp (i32) — 纯 OOP 字段，保留直读
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::HURT_BY_TIMESTAMP)) {
        m_lastDamageTimestamp = *val;
    }

    // FallFlying 已上提为按 EntityFlagsComponent 注册的组件序列化器，经
    // Entity::readFromNBT 的 loadAll 读回（批次6 子目标1），此处不再重复读。

    // Attributes - 属性列表
    // 参考 MC Java: 属性必须在效果之前加载，因为效果 NBT 中的修改器已作为 permanentModifiers
    // 保存在属性 NBT 中。readAttributeMap 会先清除旧修改器再从 NBT 加载，确保属性状态与存档一致。
    nbt_helper::readAttributeMap(tag, nbt_keys::ATTRIBUTES, attributes());

    // ActiveEffects - 药水效果列表
    // 参考 MC Java: LivingEntity.readAdditionalSaveData()
    // 效果在属性之后加载。效果直接填入效果列表，不通过 addEffect()（不会重新应用属性修改器），
    // 因为效果修改器已经作为 permanentModifiers 保存在属性 NBT 中。
    // 先移除所有旧效果（含属性修改器），然后直接填充效果列表。
    if (auto* effectsList = nbt_helper::tryGetList(tag, nbt_keys::ACTIVE_EFFECTS)) {
        if (effectsList->element_id() == nbt::TagId::Compound) {
            removeAllEffects();
            auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*effectsList);
            for (const auto& effectTag : compoundList.value) {
                m_effectManager.getAllEffects().push_back(entity::effect::EffectInstance::fromNbt(effectTag));
            }
        }
    }

    return Result<void>::ok();
}

} // namespace mc
