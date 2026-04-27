#include "AbstractArrowEntity.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../entities/player/Player.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../util/math/random/Random.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ============================================================================
// AbstractArrowEntity
// ============================================================================

AbstractArrowEntity::AbstractArrowEntity(LegacyEntityType type, EntityId id)
    : ProjectileEntity(type, id)
{
    m_noGravity = false;
}

void AbstractArrowEntity::tick() {
    // 如果插在方块中，执行不同的tick逻辑
    if (m_inGround) {
        tickInGround();
        return;
    }

    // 检查抖动
    if (m_arrowShake > 0) {
        --m_arrowShake;
    }

    // 如果在水中，灭火
    if (isInWater()) {
        setFire(0);
    }

    // 调用父类tick
    ProjectileEntity::tick();

    // MC 1.16.5: 暴击粒子效果
    if (m_critical && !m_inGround && m_world) {
        mc::math::Random rng = getRandom();
        // 每tick有概率生成暴击粒子
        if (rng.nextInt(3) == 0) {
            f32 ox = (rng.nextFloat() * 2.0f - 1.0f) * 0.3f;
            f32 oy = (rng.nextFloat() * 2.0f - 1.0f) * 0.3f;
            f32 oz = (rng.nextFloat() * 2.0f - 1.0f) * 0.3f;

            Vector3 pos(x() + ox, y() + oy, z() + oz);
            // 粒子速度与箭矢速度相反
            Vector3 vel(-velocityX() * 0.01f, -velocityY() * 0.01f, -velocityZ() * 0.01f);

            m_world->addParticle(
                client::renderer::trident::particle::ParticleTypeId::Crit,
                pos, vel);
        }
    }
}

void AbstractArrowEntity::tickInGround() {
    // 检查是否应该脱落
    if (shouldDespawn()) {
        m_inGround = false;
        m_ticksInGround = 0;
        // 随机弹射
        // setMotion(rand.nextFloat() * 0.2, rand.nextFloat() * 0.2, rand.nextFloat() * 0.2);
    }

    ++m_ticksInGround;

    // 超时移除
    if (m_ticksInGround >= 1200) {  // 60秒
        remove();
    }
}

bool AbstractArrowEntity::shouldDespawn() {
    // TODO: 检查箭矢周围的方块是否还在
    // 如果方块被移除，箭矢应该脱落
    return false;
}

void AbstractArrowEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    mc::Entity* target = result.hitEntity;

    // 计算伤害
    f32 speed = std::sqrt(m_velocity.x * m_velocity.x +
                          m_velocity.y * m_velocity.y +
                          m_velocity.z * m_velocity.z);
    i32 damage = static_cast<i32>(speed * m_damage);

    // 暴击伤害加成
    if (m_critical) {
        damage += damage / 2;
    }

    // 穿透检查
    if (m_pierceLevel > 0) {
        if (m_piercedEntities.size() >= m_pierceLevel) {
            // 达到穿透上限，移除箭矢
            remove();
            return;
        }
        m_piercedEntities.push_back(target->id());
    }

    // 获取发射者
    mc::Entity* shooter = getShooter();

    // 创建伤害来源
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Arrow, shooter, this, shooter != nullptr);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Arrow, this, this, false);
    }

    // 应用伤害
    // TODO: 实现实体受伤
    // bool hurt = target->attackEntityFrom(*damageSource, static_cast<f32>(damage));

    // 击退效果
    if (m_knockbackStrength > 0) {
        f32 ratio = 0.6f * m_knockbackStrength;
        Vector3 knockback(m_velocity.x * ratio, 0.1f, m_velocity.z * ratio);
        if (knockback.length() > 0.0f) {
            target->addVelocity(knockback);
        }
    }

    // 火焰伤害
    if (isOnFire()) {
        target->setFire(5);
    }

    // 播放命中音效
    // playSound(SoundEvents.ENTITY_ARROW_HIT, 1.0F, 1.2F / (rand.nextFloat() * 0.2F + 0.9F));

    // 如果不是穿透箭，移除
    if (m_pierceLevel <= 0) {
        remove();
    }
}

void AbstractArrowEntity::onBlockHit(const RayTraceResult& result) {
    m_inGround = true;
    m_arrowShake = 7;

    // 清除暴击和穿透状态
    m_critical = false;
    m_pierceLevel = 0;
    m_piercedEntities.clear();

    // 播放命中音效
    // playSound(SoundEvents.ENTITY_ARROW_HIT_GROUND, 1.0F, 1.2F / (rand.nextFloat() * 0.2F + 0.9F));
}

void AbstractArrowEntity::setEnchantmentEffectsFrom(LivingEntity& shooter, f32 baseVelocity) {
    // TODO: 从发射者获取附魔效果
    // int power = EnchantmentHelper.getMaxEnchantmentLevel(Enchantments.POWER, shooter);
    // int punch = EnchantmentHelper.getMaxEnchantmentLevel(Enchantments.PUNCH, shooter);
    // int flame = EnchantmentHelper.getMaxEnchantmentLevel(Enchantments.FLAME, shooter);

    // 设置伤害
    m_damage = baseVelocity * 2.0f;

    // 力量附魔增加伤害
    // if (power > 0) {
    //     m_damage += power * 0.5f + 0.5f;
    // }

    // 冲击附魔增加击退
    // if (punch > 0) {
    //     m_knockbackStrength = punch;
    // }

    // 火焰附魔
    // if (flame > 0) {
    //     setFire(100);
    // }
}

bool AbstractArrowEntity::onPlayerPickup(Player& player) {
    // TODO: 实现玩家拾取逻辑
    // if (m_pickupStatus == PickupStatus::Disallowed) {
    //     return false;
    // }
    // if (m_pickupStatus == PickupStatus::CreativeOnly && !player.isCreative()) {
    //     return false;
    // }
    // 将箭矢添加到玩家背包
    // player.inventory().addItem(getArrowStack());
    remove();
    return true;
}

// ============================================================================
// ArrowEntity
// ============================================================================

ArrowEntity::ArrowEntity(LegacyEntityType type, EntityId id)
    : AbstractArrowEntity(type, id)
{
    m_damage = 2.0f;
}

std::unique_ptr<Entity> ArrowEntity::create(IWorld* /*world*/) {
    return std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 0);
}

std::unique_ptr<ArrowEntity> ArrowEntity::createFromShooter(
    LivingEntity& shooter, IWorld* world) {
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 0);
    arrow->setWorld(world);
    arrow->setPosition(shooter.x(),
                       shooter.y() + shooter.eyeHeight() - 0.1f,
                       shooter.z());
    arrow->setShooter(&shooter);

    // 玩家射出的箭可以被拾取
    // if (shooter.isPlayer()) {
    //     arrow->setPickupStatus(PickupStatus::Allowed);
    // }

    return arrow;
}

void ArrowEntity::tick() {
    AbstractArrowEntity::tick();

    // 药水箭的效果处理
    if (m_color != 0xFFFFFFFF && !m_inGround) {
        // TODO: 生成彩色粒子
    }
}

// ============================================================================
// SpectralArrowEntity
// ============================================================================

SpectralArrowEntity::SpectralArrowEntity(LegacyEntityType type, EntityId id)
    : AbstractArrowEntity(type, id)
{
    m_damage = 2.0f;
}

std::unique_ptr<Entity> SpectralArrowEntity::create(IWorld* /*world*/) {
    return std::make_unique<SpectralArrowEntity>(LegacyEntityType::Unknown, 0);
}

void SpectralArrowEntity::tick() {
    AbstractArrowEntity::tick();

    // 光灵箭粒子效果
    if (!m_inGround) {
        // TODO: 生成光灵箭粒子
    }
}

} // namespace entity
} // namespace mc
