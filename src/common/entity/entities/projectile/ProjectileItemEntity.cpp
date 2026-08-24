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

#include "ProjectileItemEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/components/ExperienceBottleComponent.hpp"
#include "common/entity/ecs/components/PotionProjectileComponent.hpp"
#include "common/entity/ecs/components/ProjectileItemComponent.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/monster/nether/BlazeEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/entities/passive/basic/ChickenEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ThrowableEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/decorative/AbstractCandleBlock.hpp"
#include "common/world/block/blocks/decorative/CampfireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace entity {

namespace {

// 辅助函数：基于实体ID和tick创建随机数生成器
math::Random createRandomFromEntity(const Entity& entity)
{
    u64 seed = static_cast<u64>(entity.id()) << 32 | static_cast<u64>(entity.ticksExisted());
    return math::Random(seed);
}

} // anonymous namespace

// ============================================================================
// ProjectileItemEntity
// ============================================================================

ProjectileItemEntity::ProjectileItemEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ThrowableEntity(id, registry)
{
    // 批次6 子目标2 Step1：attach ProjectileItemComponent（投掷物承载物品）。
    // Snowball/Egg/EnderPearl/Potion/ExperienceBottle 经本类继承获得此组件。
    // Step4 将把 m_itemStack 读写改走组件。
    m_entityContext->enttRegistry().emplace<ecs::ProjectileItemComponent>(m_entityContext->entity());
}

void ProjectileItemEntity::tick()
{
    ThrowableEntity::tick();
    // 气泡粒子已在 ThrowableEntity::tick() 中处理，此处无需重复
}

// 批次6 子目标2 Step4：m_itemStack 迁入 ecs::ProjectileItemComponent。
ItemStack ProjectileItemEntity::getItemStack() const
{
    const auto* c = tryGetComponent<ecs::ProjectileItemComponent>();
    return (c != nullptr && c->m_itemStack != nullptr) ? *c->m_itemStack : ItemStack();
}

void ProjectileItemEntity::setItemStack(const ItemStack& stack)
{
    auto* c = tryGetComponent<ecs::ProjectileItemComponent>();
    if (c != nullptr && c->m_itemStack != nullptr) {
        *c->m_itemStack = stack;
    }
}

// ============================================================================
// SnowballEntity
// ============================================================================

SnowballEntity::SnowballEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileItemEntity(id, registry)
{}

std::unique_ptr<Entity> SnowballEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<SnowballEntity>(0, registry);
}

const Item* SnowballEntity::getDefaultItem() const
{
    return Items::SNOWBALL;
}

void SnowballEntity::onEntityHit(const RayTraceResult& result)
{
    if (!result.hitEntity) {
        return;
    }

    // 对烈焰人造成额外伤害（3点）
    i32 damage = 0;

    // 检查目标是否是烈焰人
    BlazeEntity* blaze = dynamic_cast<BlazeEntity*>(result.hitEntity);
    if (blaze != nullptr) {
        damage = 3;
    }

    if (damage > 0) {
        mc::Entity* shooter = getShooter();
        auto damageSource = DamageSources::mobProjectile(this, shooter);
        LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
        if (livingTarget != nullptr) {
            livingTarget->hurt(damageSource, static_cast<f32>(damage));
        }
    }
}

void SnowballEntity::onImpact(const RayTraceResult& result)
{
    // 先调用基类 onImpact 完成命中 dispatch（对齐 vanilla Snowball.onHit 首行 super.onHit()）：
    // 基类 ProjectileEntity::onImpact 会按 result.type 调用 onEntityHit（→ hurt 烈焰人 3 伤害）
    // 或 onBlockHit，并处理投射物偏转（deflection）逻辑。
    // 此前本方法直接放粒子 + remove，绕过基类 dispatch，致 onEntityHit 永不触发——雪球命中烈焰人
    // 的 3 伤害链路（dynamic_cast<BlazeEntity*> + hurt）成为死代码，烈焰人被雪球击中不掉血。
    ProjectileEntity::onImpact(result);

    // 命中即破裂（对齐 vanilla Snowball.onHit：broadcastEntityEvent(3) 广播破裂粒子 + discard）。
    // 注：被偏转（deflection）时基类 onImpact 已 return 不调 onEntityHit，但雪球仍应破裂消失
    // （vanilla 偏转后不 discard，但 Cubium 偏转体系简化，统一破裂；TODO: 偏转后保留待对齐）。
    if (m_world) {
        m_world->addParticle(particle::ParticleTypeId::Snowflake,
            m_builtIn.stateVector->m_pos,
            Vector3(0.0f, 0.0f, 0.0f),
            Vector3(0.5f, 0.5f, 0.5f),
            8);
    }

    remove();
}

// ============================================================================
// EggEntity
// ============================================================================

EggEntity::EggEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileItemEntity(id, registry)
{}

std::unique_ptr<Entity> EggEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<EggEntity>(0, registry);
}

const Item* EggEntity::getDefaultItem() const
{
    return Items::EGG;
}

void EggEntity::onEntityHit(const RayTraceResult& result)
{
    // 对齐 vanilla ThrownEgg.onHitEntity（ThrownEgg.java:55-59）：
    //   super.onHitEntity(p); p.getEntity().hurt(damageSources().thrown(this, getOwner()), 0.0F);
    // 鸡蛋命中实体造成 0 点投掷伤害（thrown 类型）——0 伤害本身不扣血，但 hurt 调用会触发受击副作用：
    // 受击反馈/盾牌格挡判定/荆棘反伤/无敌帧标记/setLastHurtBy 等。此前本方法为空实现（仅 (void)result），
    // 鸡蛋命中实体不调 hurt，上述副作用全丢失（与 SnowballEntity/WindChargeEntity 修复前同类死代码）。
    if (result.hitEntity == nullptr) {
        return;
    }

    Entity* shooter = getShooter();
    IndirectEntityDamageSource thrownSource = DamageSources::thrown(this, shooter);
    result.hitEntity->hurt(thrownSource, 0.0f);
}

void EggEntity::onImpact(const RayTraceResult& result)
{
    // 对齐 vanilla ThrownEgg.onHit（ThrownEgg.java:62-91）：
    //   super.onHit(p);  // 基类 dispatch（→ onHitEntity/onHitBlock）
    //   if (!level.isClientSide) {
    //     if (random.nextInt(8) == 0) {  // 1/8 孵化
    //       int i = 1; if (random.nextInt(32) == 0) i = 4;  // 1/32 子概率孵 4 只
    //       for (j<i) { 生成幼年鸡 setAge(-24000) snapTo(位置) addFreshEntity }
    //     }
    //     broadcastEntityEvent(3);  // 破裂粒子
    //     discard();
    //   }
    //
    // 修复要点（任务 #329）：
    // 1. 首行调基类 ProjectileEntity::onImpact 完成 dispatch（命中实体→onEntityHit 0 伤害副作用 / 命中方块→
    //    onBlockHit 通知方块 onProjectileHit）。此前本方法直接孵化+remove 绕过基类 dispatch，致
    //    onEntityHit/onBlockHit 死代码（与 SnowballEntity #327 / WindChargeEntity #328 修复前同病）。
    // 2. 孵化逻辑对齐 vanilla：1/8 概率孵化，其中 1/32 子概率孵 4 只（此前仅 1/8 孵 1 只，缺 4 只分支）。
    // 3. 小鸡设为幼年（setChild(true) 等价 setAge(-24000)），此前生成的鸡为成年（vanilla setAge(-24000)）。
    // 注：原 TODO 注释"vanilla 命中实体不孵化、命中方块孵化"判断错误——vanilla onHit 对所有命中类型
    //   （实体/方块）都在 super.onHit 之后统一孵化，Cubium 在 onImpact（=onHit）孵化位置正确。
    ProjectileEntity::onImpact(result);

    // 命中后破裂孵化（对齐 vanilla onHit：!isClientSide 分支）。被偏转（deflection）时基类 onImpact
    // 已 return 不调 onEntityHit，但 vanilla onHit 仍执行破裂+discard，此处统一处理（偏转后鸡蛋也破裂）。
    if (m_world == nullptr || m_world->isClientSide()) {
        if (!isRemoved()) {
            remove();
        }
        return;
    }

    // 1/8 概率孵化小鸡，其中 1/32 子概率孵 4 只（对齐 vanilla nextInt(8)==0 + nextInt(32)==0）。
    math::Random& rng = m_world->getRandom();
    if (rng.nextInt(8) == 0) {
        const i32 chickCount = (rng.nextInt(32) == 0) ? 4 : 1;
        for (i32 i = 0; i < chickCount; ++i) {
            _spawnHatchedChicken();
        }
        // 孵化成功粒子（对齐 vanilla broadcastEntityEvent(3) 破裂粒子，孵化时也播放）。
        m_world->addParticle(particle::ParticleTypeId::Heart, m_builtIn.stateVector->m_pos, Vector3(0.0f, 0.0f, 0.0f));
    } else {
        // 未孵化：播放破裂粒子效果（对齐 vanilla broadcastEntityEvent(3) 的 ItemParticle 破裂粒子）。
        m_world->addParticle(particle::ParticleTypeId::Snowflake,
            m_builtIn.stateVector->m_pos,
            Vector3(0.0f, 0.0f, 0.0f),
            Vector3(0.3f, 0.3f, 0.3f),
            4);
    }

    if (!isRemoved()) {
        remove();
    }
}

void EggEntity::_spawnHatchedChicken()
{
    // 对齐 vanilla ThrownEgg.onHit 孵化小鸡（ThrownEgg.java:72-84）：
    //   Chicken chicken = EntityType.CHICKEN.create(level, TRIGGERED);
    //   chicken.setAge(-24000);  // 幼年
    //   chicken.snapTo(x, y, z, yRot, 0);
    //   level.addFreshEntity(chicken);
    // 注：vanilla 还从物品 DataComponents 读 CHICKEN_VARIANT 设鸡变种（:76-78），Cubium 物品组件
    //   体系暂未支持 CHICKEN_VARIANT，TODO 待物品 DataComponents 接入后补全。
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return;
    }

    auto chicken = std::make_unique<ChickenEntity>(0, *registry);
    // 直接构造的实体需显式设置 typeId（注册表路径会自动设置，同 ExperienceBottle/DragonFireball 范式）。
    chicken->setTypeId(EntityTypeKeys::CHICKEN);
    chicken->setWorld(m_world);
    // snapTo(位置, yRot, 0)：vanilla 用鸡蛋自身位置 + yRot，Cubium 用鸡蛋位置 + 当前 yRot。
    chicken->setPosition(x(), y(), z());
    chicken->setRotation(m_builtIn.rotation->m_rot.y, 0.0f);
    // setAge(-24000) 设幼年（对齐 vanilla setAge(-24000)）。setChild(true) 等价设 BABY_AGE=-24000。
    chicken->setChild(true);

    // TODO(CHICKEN_VARIANT): 对齐 vanilla 从鸡蛋物品 DataComponents.CHICKEN_VARIANT 读取并设置鸡变种
    //   （ThrownEgg.java:76-78）。Cubium 物品 DataComponents 体系暂未支持 CHICKEN_VARIANT，待接入后补全。

    m_world->spawnEntity(std::move(chicken));
}

bool EggEntity::_tryHatchChicken()
{
    // 12.5% (1/8) 概率孵化（保留供旧调用方/测试访问器使用，新孵化逻辑已迁至 onImpact 内联 + _spawnHatchedChicken）。
    if (m_world) {
        math::Random& rng = m_world->getRandom();
        return rng.nextInt(8) == 0;
    }
    return false;
}

// ============================================================================
// EnderPearlEntity
// ============================================================================

EnderPearlEntity::EnderPearlEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileItemEntity(id, registry)
{}

std::unique_ptr<Entity> EnderPearlEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<EnderPearlEntity>(0, registry);
}

const Item* EnderPearlEntity::getDefaultItem() const
{
    return Items::ENDER_PEARL;
}

void EnderPearlEntity::onEntityHit(const RayTraceResult& result)
{
    // 命中实体：对目标施加 0 点投掷伤害（thrown 类型），仅用于触发受击/盾牌格挡/荆棘等副作用，
    // 不造成实际伤害。对发射者的传送与摔落伤害统一在 teleportOwnerOnImpact 处理（由 onBlockHit/
    // 本函数末尾调用）。对齐 vanilla ThrownEnderpearl.onHitEntity（ThrownEnderpearl.java:77-80）。
    if (result.hitEntity == nullptr) {
        return;
    }
    Entity* shooter = getShooter();
    IndirectEntityDamageSource thrownSource = DamageSources::thrown(this, shooter);
    result.hitEntity->hurt(thrownSource, 0.0f);

    // 命中实体同样传送发射者（vanilla onHit 在 onHitEntity 之后无条件执行传送）。
    teleportOwnerOnImpact();
}

void EnderPearlEntity::onBlockHit(const RayTraceResult& result)
{
    // 命中方块：先调基类 onBlockHit 保留方块碰撞响应（onProjectileHit + 清零速度），
    // 再传送发射者。对齐 vanilla ThrownEnderpearl.onHitBlock（经 super.onHit 分发）+ onHit 传送。
    ProjectileEntity::onBlockHit(result);
    teleportOwnerOnImpact();
}

void EnderPearlEntity::teleportOwnerOnImpact()
{
    // 对齐 vanilla ThrownEnderpearl.onHit（ThrownEnderpearl.java:83-146）。
    // 仅服务端执行；发射者存在、存活且（若是 LivingEntity）非睡觉时才传送。
    if (m_world == nullptr || m_world->isClientSide()) {
        remove();
        return;
    }

    Entity* shooter = getShooter();
    if (shooter == nullptr) {
        remove();
        return;
    }

    // isAllowedToTeleportOwner 门控：同维度时 LivingEntity 须存活；玩家额外须非睡觉。
    // （Cubium 中仅 Player 可睡觉，LivingEntity 无 isSleeping，故睡眠门控仅对 Player 生效。）
    LivingEntity* livingShooter = dynamic_cast<LivingEntity*>(shooter);
    if (livingShooter != nullptr) {
        if (!livingShooter->isAlive()) {
            remove();
            return;
        }
        Player* sleepingPlayer = dynamic_cast<Player*>(livingShooter);
        if (sleepingPlayer != nullptr && sleepingPlayer->isSleeping()) {
            remove();
            return;
        }
    } else if (!shooter->isAlive()) {
        remove();
        return;
    }

    // 传送目标：珍珠上一帧位置（prevPosition，对应 vanilla oldPosition()），避免传送进命中点
    // 所在的方块/实体内部。
    const Vector3 dest = prevPosition();
    shooter->setPosition(dest.x, dest.y, dest.z);
    // 传送后重置摔落距离，避免继承传送前的高空摔落伤害。对齐 vanilla resetFallDistance()。
    shooter->setFallDistance(0.0f);

    Player* player = dynamic_cast<Player*>(shooter);
    if (player != nullptr) {
        // 玩家分支：施加 5.0 末影珍珠摔落伤害（enderPearl 类型，属 IS_FALL/BYPASSES_ARMOR）。
        // 对齐 vanilla serverplayer1.hurtServer(enderPearl(), 5.0F)。
        EnvironmentalDamage pearlSource = DamageSources::enderPearl();
        player->hurt(pearlSource, 5.0f);

        // TODO(末影螨生成未实现): 对齐 vanilla ThrownEnderpearl.onHit 的 5% 末影螨生成
        // （random.nextFloat()<0.05 && 难度非和平时在发射者位置生成 EndermiteEntity，
        // EntitySpawnReason::TRIGGERED）。EntityTypeKeys 缺 endermite 枚举，待补 setTypeId
        // 后接入完整 spawn 流程。
    }
    // 非玩家 mob 分支：仅传送，不施加伤害（对齐 vanilla 非 ServerPlayer 分支无 hurtServer）。

    // 传送音效。对齐 vanilla playSound(SoundEvents.PLAYER_TELEPORT)。
    // TODO(音效缺失): Cubium 未定义 PLAYER_TELEPORT，暂用末影人传送音效近似。
    playSound(SoundEvents::ENTITY_ENDERMAN_TELEPORT, 1.0f, 1.0f);

    remove();
}

// ============================================================================
// PotionEntity
// ============================================================================

PotionEntity::PotionEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileItemEntity(id, registry)
{
    // 批次6 子目标2 Step1：attach PotionProjectileComponent（药水类型 lingering 标志）。
    // Step4 将把 m_lingering 读写改走组件。
    m_entityContext->enttRegistry().emplace<ecs::PotionProjectileComponent>(m_entityContext->entity());

    // 对齐 vanilla ThrownPotion：实体默认携带 splash_potion 物品，PotionContents 默认为空即水瓶
    // （PotionUtils::getPotion 对 potionId 空的 splash_potion 返回 Potions::WATER）。
    // 此前 getItemStack 返回空 ItemStack → isWaterBottle 判定为 false（空 stack getPotion 返回 EMPTY），
    // 致 test.spawn 生成的 splash_potion 永不进入 onHitAsWater/dowseFire 水瓶分支。女巫投药水后用
    // setItemStack 覆盖为具体药水（WitchEntity.cpp:444），不受此默认值影响。
    setItemStack(potion::PotionUtils::createSplashPotionItem(potion::Potions::WATER));
}

std::unique_ptr<Entity> PotionEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<PotionEntity>(0, registry);
}

const Item* PotionEntity::getDefaultItem() const
{
    // 默认喷溅型药水物品（vanilla ThrownPotion 默认 splash_potion）。
    return Items::SPLASH_POTION;
}

void PotionEntity::onImpact(const RayTraceResult& result)
{
    // 对齐 vanilla AbstractThrownPotion.onHit:70-85：首行 super.onHit() dispatch
    // （基类按命中类型分发 onEntityHit/onBlockHit），再按药水类型分支处理，最后 discard。
    // 此前未调基类 dispatch，致 onBlockHit（通知方块 onProjectileHit + 水瓶浇火）成死代码，
    // 与 SnowballEntity/WindChargeEntity/EggEntity onImpact 修复前同病（任务 #327/#328/#329）。
    ProjectileEntity::onImpact(result);

    // 获取药水效果
    const ItemStack itemStack = getItemStack();
    auto effects = potion::PotionUtils::getEffects(itemStack);

    // 水瓶特例：对齐 vanilla onHit:75-76（potioncontents.is(Potions.WATER)→onHitAsWater）。
    // 水瓶无效果，走水敏感伤害 + 灭火分支，不走喷溅效果逻辑。
    const bool isWater = potion::PotionUtils::isWaterBottle(itemStack);

    // 喷溅药水影响范围为 4.0 格
    constexpr f32 SPLASH_RADIUS = 4.0f;

    if (m_world) {
        // 播放破裂音效
        math::Random rng = createRandomFromEntity(*this);
        playSound(SoundEvents::ENTITY_SPLASH_POTION_BREAK, 1.0f, 0.5f + rng.nextFloat() * 0.3f);

        // 生成粒子效果
        // 药水颜色从效果列表获取
        u32 color = potion::PotionUtils::getColor(effects);

        // 生成喷溅粒子
        m_world->addParticle(particle::ParticleTypeId::Splash,
            m_builtIn.stateVector->m_pos,
            Vector3(0.0f, 0.0f, 0.0f),
            Vector3(0.5f, 0.5f, 0.5f),
            20);

        // 水瓶：对范围内水敏感/着火实体施加效果（伤害/灭火），对齐 onHitAsWater:87-106。
        // 非水瓶且有效果：喷溅效果（onHitAsPotion 等价），原逻辑保留。
        if (isWater) {
            _onHitAsWater();
        } else if (!effects.empty()) {
            // 获取范围内的所有实体
            AxisAlignedBB searchBox(m_builtIn.stateVector->m_pos.x - SPLASH_RADIUS,
                m_builtIn.stateVector->m_pos.y - SPLASH_RADIUS,
                m_builtIn.stateVector->m_pos.z - SPLASH_RADIUS,
                m_builtIn.stateVector->m_pos.x + SPLASH_RADIUS,
                m_builtIn.stateVector->m_pos.y + SPLASH_RADIUS,
                m_builtIn.stateVector->m_pos.z + SPLASH_RADIUS);

            std::vector<Entity*> nearbyEntities = m_world->getEntitiesInAABB(searchBox, this);

            for (Entity* entity : nearbyEntities) {
                // 检查是否在范围内（球形范围）
                f32 distanceSq = entity->position().distanceSquared(m_builtIn.stateVector->m_pos);
                if (distanceSq > SPLASH_RADIUS * SPLASH_RADIUS) {
                    continue;
                }

                // 只对生物实体应用效果
                LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
                if (living == nullptr) {
                    continue;
                }

                // 计算效果强度（距离越近效果越强）
                f32 distance = std::sqrt(distanceSq);
                f32 intensity = 1.0f - (distance / SPLASH_RADIUS);

                // 应用每个效果
                // TODO: InstantDamage/InstantHealth 经 addEffect→applyInstantly→hurt(DamageSources::magic())
                // 伤害源为 EnvironmentalDamage（getEntity()==nullptr），丢失投掷者信息。Java 的
                // ThrownSplashPotion 用 IndirectMagic 伤害源（owner=投掷者），故女巫自投伤害药水溅回自身时
                // source.getEntity()==女巫→WitchEntity.applyPotionDamageCalculations 自伤检查返0免疫；
                // cubium 当前 magic() 无 owner，女巫自伤检查不触发，走 85% 减免后自伤 0.6（Java 为0）。
                // 修复需让 InstantDamage 携带投掷者（改 applyInstantly 签名传 owner，或此处对 InstantDamage
                // 特殊处理用 DamageSources::indirectMagic(this, getShooter()) 直接 hurt）。偏差轻微（罕见溅回
                // 自身场景 + 仅 0.6 自伤），暂不修，待 applyInstantly owner 体系完善。
                for (const auto& effect : effects) {
                    // 计算持续时间（距离越近持续时间越长）
                    i32 duration = static_cast<i32>(effect.duration() * intensity);
                    if (duration < 20) { // 最少 1 秒
                        duration = 20;
                    }

                    // 创建新的效果实例
                    entity::effect::EffectInstance scaledEffect(effect.type(),
                        duration,
                        effect.amplifier(),
                        effect.isAmbient(),
                        effect.isVisible(),
                        effect.showIcon());

                    living->addEffect(scaledEffect);
                }
            }
        }

        // 如果是滞留型药水，创建区域效果云
        // 水瓶不生成滞留云（vanilla onHitAsWater 不走 onHitAsPotion，无 AreaEffectCloud）。
        if (!isWater && isLingering()) {
            // ECS 迁移：实体构造需要 registry 句柄（m_world 为投射物所属世界，此处必非空）
            auto* registry = &ecsRegistry();
            if (registry == nullptr) {
                return;
            }

            // 创建区域效果云实体
            auto cloud = std::make_unique<AreaEffectCloudEntity>(*registry);

            // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
            cloud->setTypeId(EntityTypeKeys::AREA_EFFECT_CLOUD);

            cloud->setWorld(m_world);
            cloud->setPosition(x(), y(), z());

            // 设置拥有者（投射物的发射者）
            Entity* shooter = getShooter();
            if (shooter != nullptr) {
                LivingEntity* livingShooter = dynamic_cast<LivingEntity*>(shooter);
                if (livingShooter != nullptr) {
                    cloud->setOwner(livingShooter);
                }
            }

            // 设置区域效果云参数
            cloud->setRadius(3.0f);
            cloud->setRadiusOnUse(-0.5f);
            cloud->setWaitTime(10);
            cloud->setRadiusPerTick(-cloud->getRadius() / static_cast<f32>(cloud->getDuration()));

            // 添加药水效果到效果云（效果持续时间在区域效果云中为原持续时间的 1/4）
            for (const auto& effect : effects) {
                entity::effect::EffectInstance cloudEffect(effect.type(),
                    effect.duration() / 4,
                    effect.amplifier(),
                    effect.isAmbient(),
                    effect.isVisible(),
                    effect.showIcon());
                cloud->addEffect(cloudEffect);
            }

            // 设置颜色（如果药水有自定义颜色）
            u32 potionColor = potion::PotionUtils::getColor(itemStack);
            cloud->setColor(potionColor);

            // 生成效果云实体
            m_world->spawnEntity(std::move(cloud));
        }
    }

    remove();
}

void PotionEntity::onBlockHit(const RayTraceResult& result)
{
    // 先调基类：清零速度 + 通知命中方块 onProjectileHit（蜡烛点燃等）。
    ProjectileEntity::onBlockHit(result);

    // 水瓶命中方块时浇灭命中点对面 + 反方向 + 四水平方向邻接的火/蜡烛/营火。
    // 对齐 vanilla AbstractThrownPotion.onHitBlock:50-67。
    if (m_world == nullptr || result.type != RayTraceResultType::Block) {
        return;
    }

    const ItemStack itemStack = getItemStack();
    if (!potion::PotionUtils::isWaterBottle(itemStack)) {
        return;
    }

    // vanilla：blockpos1 = blockpos.relative(direction)（命中面外侧方块）。
    const Direction hitFace = result.face;
    const BlockPos blockPos1 = result.blockPos.offset(hitFace);

    _dowseFire(blockPos1);
    // blockpos1.relative(direction.getOpposite())：命中点反向（即命中方块自身一侧）。
    if (hitFace != Direction::None) {
        _dowseFire(blockPos1.offset(Directions::opposite(hitFace)));
        // 四水平方向。
        for (const Direction horiz : Directions::horizontal()) {
            _dowseFire(blockPos1.offset(horiz));
        }
    }
}

void PotionEntity::_dowseFire(const BlockPos& pos)
{
    // 对齐 vanilla AbstractThrownPotion.dowseFire:110-121。
    if (m_world == nullptr) {
        return;
    }

    const BlockState* state = m_world->getBlockState(pos);
    if (state == nullptr) {
        return;
    }

    if (BlockTags::FIRE().contains(*state)) {
        // 火：destroyBlock(pos, false, this) → 置空气。Cubium 无 IWorld::destroyBlock，
        // 用 setBlockState(air) 等价（与 RavagerEntity/EnderDragonEntity 破坏方块范式一致）。
        const BlockState* airState = &VanillaBlocks::AIR->defaultState();
        m_world->setBlockState(pos, airState, 3);
    } else if (blocks::AbstractCandleBlock::isLit(*state)) {
        // 蜡烛：extinguish(null, state, level, pos)。extinguish 是 AbstractCandleBlock 虚函数，
        // Block 基类无此声明，需 dynamic_cast 到 AbstractCandleBlock 后调用（CandleBlock/CandleCakeBlock
        // 未 override，落到基类实现：setBlock LIT=false + 熄灭音效）。
        BlockState mutableState = *state;
        auto* candle = dynamic_cast<blocks::AbstractCandleBlock*>(&state->getBlockMutable());
        if (candle != nullptr) {
            candle->extinguish(*m_world, pos, mutableState, nullptr);
        }
    } else if (blocks::CampfireBlock::isLitCampfire(*state)) {
        // 营火：levelEvent(1009 熄灭音效) + dowse(owner, level, pos, state) + setBlock(LIT=false)。
        // Cubium CampfireBlock::extinguish 内部已做 setBlock(LIT=false) + 播放熄灭音效，
        // 等价合并 vanilla dowse + setBlockAndUpdate。owner 信息丢失（vanilla dowse 仅记伤害归属，
        // 营火无掉落影响轻微），TODO 待 CampfireBlock::dowse 接入 owner。
        BlockState mutableState = *state;
        blocks::CampfireBlock::extinguish(*m_world, pos, mutableState);
    }
}

void PotionEntity::_onHitAsWater()
{
    // 对齐 vanilla AbstractThrownPotion.onHitAsWater:87-106。
    // AABB inflate(4.0, 2.0, 4.0)，对水敏感或着火且距离平方<16 的 LivingEntity：
    //   水敏感 → hurt(indirectMagic(this, owner), 1.0F)；着火 → extinguishFire()。
    if (m_world == nullptr) {
        return;
    }

    constexpr f32 HALF_XZ = 4.0f;
    constexpr f32 HALF_Y = 2.0f;
    constexpr f32 RADIUS_SQ = 16.0f;

    const Vector3 pos = m_builtIn.stateVector->m_pos;
    AxisAlignedBB aabb(
        pos.x - HALF_XZ, pos.y - HALF_Y, pos.z - HALF_XZ, pos.x + HALF_XZ, pos.y + HALF_Y, pos.z + HALF_XZ);

    std::vector<Entity*> nearby = m_world->getEntitiesInAABB(aabb, this);
    Entity* shooter = getShooter();

    for (Entity* entity : nearby) {
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (living == nullptr) {
            continue;
        }

        const f32 distSq = living->position().distanceSquared(pos);
        if (distSq >= RADIUS_SQ) {
            continue;
        }

        // 水敏感：受 1.0 indirectMagic 伤害（owner=投掷者）。
        if (living->isWaterSensitive()) {
            auto source = DamageSources::indirectMagic(this, shooter);
            living->hurt(source, 1.0f);
        }

        // 着火且存活：灭火。
        if (living->isOnFire() && living->isAlive()) {
            living->extinguishFire();
        }
    }

    // TODO: 美西螈 rehydrate（Axolotl.rehydrate）未实现，待 AxolotlEntity 补该方法后在此接入
    // （vanilla onHitAsWater:103-105 遍历范围内 Axolotl 调 rehydrate）。
}

// ============================================================================
// ExperienceBottleEntity
// ============================================================================

ExperienceBottleEntity::ExperienceBottleEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileItemEntity(id, registry)
{
    // 批次6 子目标2 Step1：attach ExperienceBottleComponent（经验瓶释放经验值）。
    // Step4 将把 m_experience 读写改走组件。
    m_entityContext->enttRegistry().emplace<ecs::ExperienceBottleComponent>(m_entityContext->entity());
}

std::unique_ptr<Entity> ExperienceBottleEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ExperienceBottleEntity>(0, registry);
}

const Item* ExperienceBottleEntity::getDefaultItem() const
{
    return Items::EXPERIENCE_BOTTLE;
}

void ExperienceBottleEntity::onImpact(const RayTraceResult& /*result*/)
{
    // TODO: 未调用基类 ProjectileEntity::onImpact 完成 dispatch（命中实体→onEntityHit / 命中方块→
    //   onBlockHit），与 SnowballEntity::onImpact 修复前同病。经验瓶 onEntityHit 无伤害逻辑（vanilla
    //   经验瓶命中实体 0 伤害，破裂才给经验），故命中实体无功能损失；但 onBlockHit（通知方块
    //   onProjectileHit）被绕过。vanilla ThrownExperienceBottle.onHit 首行 super.onHit() dispatch
    //   再破裂散经验，应对齐。
    // 生成经验球（3-11点经验）
    if (m_world) {
        // ECS 迁移：实体构造需要 registry 句柄（m_world 已判空，此处 registry 必非空）
        auto* registry = &ecsRegistry();
        if (registry == nullptr) {
            return;
        }

        math::Random& rng = m_world->getRandom();
        i32 experience = rng.nextInt(3, 11);

        // 生成经验球实体
        for (i32 i = 0; i < experience; ++i) {
            auto orb = std::make_unique<ExperienceOrbEntity>(1, *registry);

            // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
            orb->setTypeId(EntityTypeKeys::EXPERIENCE_ORB);

            orb->setPosition(m_builtIn.stateVector->m_pos.x + (rng.nextFloat() - 0.5f) * 0.5f,
                m_builtIn.stateVector->m_pos.y + 0.5f,
                m_builtIn.stateVector->m_pos.z + (rng.nextFloat() - 0.5f) * 0.5f);
            orb->setWorld(m_world);
            // 给予随机速度，使经验球散开
            orb->setVelocity(
                (rng.nextFloat() - 0.5f) * 0.2f, rng.nextFloat() * 0.3f + 0.1f, (rng.nextFloat() - 0.5f) * 0.2f);
            m_world->spawnEntity(std::move(orb));
        }

        // 播放破裂效果
        m_world->addParticle(particle::ParticleTypeId::Snowflake,
            m_builtIn.stateVector->m_pos,
            Vector3(0.0f, 0.0f, 0.0f),
            Vector3(0.3f, 0.3f, 0.3f),
            4);
    }

    remove();
}

// 批次6 子目标2 Step4：m_lingering 迁入 ecs::PotionProjectileComponent。
bool PotionEntity::isLingering() const
{
    const auto* c = tryGetComponent<ecs::PotionProjectileComponent>();
    return (c != nullptr) ? c->m_lingering : false;
}

void PotionEntity::setLingering(bool lingering)
{
    auto* c = tryGetComponent<ecs::PotionProjectileComponent>();
    if (c != nullptr) {
        c->m_lingering = lingering;
    }
}

// 批次6 子目标2 Step4：m_experience 迁入 ecs::ExperienceBottleComponent。
i32 ExperienceBottleEntity::experience() const
{
    const auto* c = tryGetComponent<ecs::ExperienceBottleComponent>();
    return (c != nullptr) ? c->m_experience : 0;
}

void ExperienceBottleEntity::setExperience(i32 exp)
{
    auto* c = tryGetComponent<ecs::ExperienceBottleComponent>();
    if (c != nullptr) {
        c->m_experience = exp;
    }
}

} // namespace entity
} // namespace mc
