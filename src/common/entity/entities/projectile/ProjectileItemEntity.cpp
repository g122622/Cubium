#include "ProjectileItemEntity.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../../item/Items.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../util/math/random/Random.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <cmath>

namespace mc {
namespace entity {

namespace {

// 辅助函数：基于实体ID和tick创建随机数生成器
math::Random createRandomFromEntity(const Entity& entity) {
    u64 seed = static_cast<u64>(entity.id()) << 32 | static_cast<u64>(entity.ticksExisted());
    return math::Random(seed);
}

} // anonymous namespace

// ============================================================================
// ProjectileItemEntity
// ============================================================================

ProjectileItemEntity::ProjectileItemEntity(LegacyEntityType type, EntityId id)
    : ThrowableEntity(type, id)
{
}

void ProjectileItemEntity::tick() {
    ThrowableEntity::tick();

    // 在水中生成气泡粒子
    if (isInWater() && m_world) {
        m_world->addParticle(
            client::renderer::trident::particle::ParticleTypeId::Bubble,
            m_position,
            Vector3(0.0f, 0.1f, 0.0f));
    }
}

// ============================================================================
// SnowballEntity
// ============================================================================

SnowballEntity::SnowballEntity(LegacyEntityType type, EntityId id)
    : ProjectileItemEntity(type, id)
{
}

std::unique_ptr<Entity> SnowballEntity::create(IWorld* /*world*/) {
    return std::make_unique<SnowballEntity>(LegacyEntityType::Snowball, 0);
}

const Item* SnowballEntity::getDefaultItem() const {
    return Items::SNOWBALL;
}

void SnowballEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 对烈焰人造成额外伤害（3点）
    // TODO: 检查目标是否是烈焰人
    i32 damage = 0;

    if (damage > 0) {
        mc::Entity* shooter = getShooter();
        std::unique_ptr<DamageSource> damageSource;
        if (shooter) {
            damageSource = std::make_unique<IndirectEntityDamageSource>(
                DamageType::MobProjectile, shooter, this, false);
        } else {
            damageSource = std::make_unique<IndirectEntityDamageSource>(
                DamageType::MobProjectile, this, this, false);
        }
        // TODO: result.hitEntity->hurt(*damageSource, static_cast<f32>(damage));
    }
}

void SnowballEntity::onImpact(const RayTraceResult& /*result*/) {
    // 播放破裂粒子效果
    if (m_world) {
        m_world->addParticle(
            client::renderer::trident::particle::ParticleTypeId::Snowflake,
            m_position,
            Vector3(0.0f, 0.0f, 0.0f),
            Vector3(0.5f, 0.5f, 0.5f),
            8);
    }

    remove();
}

// ============================================================================
// EggEntity
// ============================================================================

EggEntity::EggEntity(LegacyEntityType type, EntityId id)
    : ProjectileItemEntity(type, id)
{
}

std::unique_ptr<Entity> EggEntity::create(IWorld* /*world*/) {
    return std::make_unique<EggEntity>(LegacyEntityType::Egg, 0);
}

const Item* EggEntity::getDefaultItem() const {
    return Items::EGG;
}

void EggEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 鸡蛋对实体造成极小伤害（通常为0）
    (void)result;
}

void EggEntity::onImpact(const RayTraceResult& /*result*/) {
    // 12.5% (1/8) 概率孵化小鸡
    if (tryHatchChicken()) {
        // 孵化成功
        if (m_world) {
            m_world->addParticle(
                client::renderer::trident::particle::ParticleTypeId::Heart,
                m_position,
                Vector3(0.0f, 0.0f, 0.0f));
        }
    } else {
        // 播放破裂粒子效果
        if (m_world) {
            m_world->addParticle(
                client::renderer::trident::particle::ParticleTypeId::Snowflake,
                m_position,
                Vector3(0.0f, 0.0f, 0.0f),
                Vector3(0.3f, 0.3f, 0.3f),
                4);
        }
    }

    remove();
}

bool EggEntity::tryHatchChicken() {
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

EnderPearlEntity::EnderPearlEntity(LegacyEntityType type, EntityId id)
    : ProjectileItemEntity(type, id)
{
}

std::unique_ptr<Entity> EnderPearlEntity::create(IWorld* /*world*/) {
    return std::make_unique<EnderPearlEntity>(LegacyEntityType::EnderPearl, 0);
}

const Item* EnderPearlEntity::getDefaultItem() const {
    return Items::ENDER_PEARL;
}

void EnderPearlEntity::onEntityHit(const RayTraceResult& result) {
    mc::Entity* shooter = getShooter();

    // 对发射者造成5点伤害（传送伤害）
    if (shooter && shooter->isAlive()) {
        // TODO: shooter->hurt(DamageSource::fall(), 5.0f);
    }
    (void)result;
}

void EnderPearlEntity::onImpact(const RayTraceResult& result) {
    mc::Entity* shooter = getShooter();

    // 传送发射者
    if (shooter && shooter->isAlive()) {
        // 检查发射者是否是生物
        LivingEntity* livingShooter = dynamic_cast<LivingEntity*>(shooter);
        if (livingShooter) {
            // 如果命中实体，不能传送到实体内部
            if (result.type != RayTraceResultType::Entity || !result.hitEntity) {
                // 传送到落点
                shooter->setPosition(result.hitPosition.x,
                                     result.hitPosition.y,
                                     result.hitPosition.z);
                // 造成摔落伤害
                // TODO: shooter->hurt(DamageSource::fall(), 5.0f);
            }
        }
    }

    remove();
}

// ============================================================================
// PotionEntity
// ============================================================================

PotionEntity::PotionEntity(LegacyEntityType type, EntityId id)
    : ProjectileItemEntity(type, id)
{
}

std::unique_ptr<Entity> PotionEntity::create(IWorld* /*world*/) {
    return std::make_unique<PotionEntity>(LegacyEntityType::Potion, 0);
}

const Item* PotionEntity::getDefaultItem() const {
    // TODO: 返回默认药水物品（喷溅型水瓶）
    return nullptr;
}

void PotionEntity::onImpact(const RayTraceResult& /*result*/) {
    // TODO: 应用药水效果
    // 1. 获取药水效果列表
    // 2. 影响范围内的生物
    // 3. 如果是滞留型，创建区域效果云

    remove();
}

// ============================================================================
// ExperienceBottleEntity
// ============================================================================

ExperienceBottleEntity::ExperienceBottleEntity(LegacyEntityType type, EntityId id)
    : ProjectileItemEntity(type, id)
{
}

std::unique_ptr<Entity> ExperienceBottleEntity::create(IWorld* /*world*/) {
    return std::make_unique<ExperienceBottleEntity>(LegacyEntityType::ExperienceBottle, 0);
}

const Item* ExperienceBottleEntity::getDefaultItem() const {
    return Items::EXPERIENCE_BOTTLE;
}

void ExperienceBottleEntity::onImpact(const RayTraceResult& /*result*/) {
    // 生成经验球（3-11点经验）
    if (m_world) {
        math::Random& rng = m_world->getRandom();
        i32 experience = rng.nextInt(3, 11);

        // TODO: 生成经验球实体
        // for (int i = 0; i < experience; ++i) {
        //     ExperienceOrbEntity::spawn(m_world, m_position, 1);
        // }

        // 播放破裂效果
        m_world->addParticle(
            client::renderer::trident::particle::ParticleTypeId::Snowflake,
            m_position,
            Vector3(0.0f, 0.0f, 0.0f),
            Vector3(0.3f, 0.3f, 0.3f),
            4);
    }

    remove();
}

} // namespace entity
} // namespace mc
