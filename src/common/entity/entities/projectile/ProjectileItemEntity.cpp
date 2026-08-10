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
    mc::Entity* shooter = getShooter();

    // 对发射者造成5点伤害（传送伤害）
    if (shooter && shooter->isAlive()) {
        LivingEntity* livingShooter = dynamic_cast<LivingEntity*>(shooter);
        if (livingShooter != nullptr) {
            auto damageSource = DamageSources::fall();
            livingShooter->hurt(damageSource, 5.0f);
        }
    }
    (void)result;
}

void EnderPearlEntity::onImpact(const RayTraceResult& result)
{
    mc::Entity* shooter = getShooter();

    // 传送发射者
    if (shooter && shooter->isAlive()) {
        // 检查发射者是否是生物
        LivingEntity* livingShooter = dynamic_cast<LivingEntity*>(shooter);
        if (livingShooter) {
            // 如果命中实体，不能传送到实体内部
            if (result.type != RayTraceResultType::Entity || !result.hitEntity) {
                // 传送到落点
                shooter->setPosition(result.hitPosition.x, result.hitPosition.y, result.hitPosition.z);
                // 造成摔落伤害
                auto damageSource = DamageSources::fall();
                livingShooter->hurt(damageSource, 5.0f);
            }
        }
    }

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
