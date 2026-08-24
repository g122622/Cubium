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
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ThrowableEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
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

void SnowballEntity::onImpact(const RayTraceResult& /*result*/)
{
    // 播放破裂粒子效果
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
    if (!result.hitEntity) {
        return;
    }

    // 鸡蛋对实体造成极小伤害（通常为0）
    (void)result;
}

void EggEntity::onImpact(const RayTraceResult& /*result*/)
{
    // 12.5% (1/8) 概率孵化小鸡
    if (_tryHatchChicken()) {
        // 孵化成功
        if (m_world) {
            m_world->addParticle(
                particle::ParticleTypeId::Heart, m_builtIn.stateVector->m_pos, Vector3(0.0f, 0.0f, 0.0f));
        }
    } else {
        // 播放破裂粒子效果
        if (m_world) {
            m_world->addParticle(particle::ParticleTypeId::Snowflake,
                m_builtIn.stateVector->m_pos,
                Vector3(0.0f, 0.0f, 0.0f),
                Vector3(0.3f, 0.3f, 0.3f),
                4);
        }
    }

    remove();
}

bool EggEntity::_tryHatchChicken()
{
    // 12.5% (1/8) 概率孵化
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
}

std::unique_ptr<Entity> PotionEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<PotionEntity>(0, registry);
}

const Item* PotionEntity::getDefaultItem() const
{
    // 返回默认药水物品（喷溅型水瓶）
    // 注意：药水系统尚未完全实现
    return nullptr;
}

void PotionEntity::onImpact(const RayTraceResult& result)
{
    // 获取药水效果
    const ItemStack itemStack = getItemStack();
    auto effects = potion::PotionUtils::getEffects(itemStack);

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

        // 应用效果到范围内的生物
        if (!effects.empty()) {
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
        if (isLingering()) {
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
