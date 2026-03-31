#include "OtherProjectiles.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../entities/player/Player.hpp"
#include "../../damage/DamageSource.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ============================================================================
// LlamaSpitEntity
// ============================================================================

LlamaSpitEntity::LlamaSpitEntity(LegacyEntityType type, EntityId id)
    : ThrowableEntity(type, id)
{
}

std::unique_ptr<Entity> LlamaSpitEntity::create(IWorld* /*world*/) {
    return std::make_unique<LlamaSpitEntity>(LegacyEntityType::Unknown, 0);
}

void LlamaSpitEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 造成伤害
    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::MobProjectile, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::MobProjectile, this, this, false);
    }

    // TODO: target->attackEntityFrom(*damageSource, 1.0f);
}

void LlamaSpitEntity::onImpact(const RayTraceResult& /*result*/) {
    // 播放命中粒子
    remove();
}

// ============================================================================
// FishingBobberEntity
// ============================================================================

FishingBobberEntity::FishingBobberEntity(LegacyEntityType type, EntityId id)
    : Entity(type, id)
{
    m_noGravity = false;
}

std::unique_ptr<Entity> FishingBobberEntity::create(IWorld* /*world*/) {
    return std::make_unique<FishingBobberEntity>(LegacyEntityType::Unknown, 0);
}

void FishingBobberEntity::tick() {
    Entity::tick();

    // 检查钓鱼者是否存在
    if (!m_angler || !m_angler->isAlive()) {
        remove();
        return;
    }

    switch (m_state) {
        case State::Flying:
            // 浮标在飞行中
            // 检测是否入水
            // TODO: 检测水面
            break;

        case State::Bobbing:
            // 浮标浮在水面
            // 检查咬钩
            checkBite();
            spawnFishingParticles();
            break;

        case State::Fishing:
            // 咬钩中，等待玩家拉杆
            m_fishAngle += 0.1f;
            break;

        case State::Hooked:
            // 钩住实体
            break;
    }
}

Player* FishingBobberEntity::getAngler() const {
    return m_angler;
}

void FishingBobberEntity::checkBite() {
    // 随机检查是否咬钩
    // TODO: 根据钓鱼环境计算咬钩概率
}

void FishingBobberEntity::spawnFishingParticles() {
    // TODO: 生成钓鱼粒子
}

i32 FishingBobberEntity::reelIn() {
    // 收杆
    if (m_state == State::Fishing) {
        // 成功钓到鱼
        remove();
        // TODO: 生成物品
        return 1;  // 返回经验值
    } else if (m_state == State::Hooked) {
        // 钩住实体，拉过来
        remove();
        return 0;
    }

    remove();
    return 0;
}

// ============================================================================
// ShulkerBulletEntity
// ============================================================================

ShulkerBulletEntity::ShulkerBulletEntity(LegacyEntityType type, EntityId id)
    : ProjectileEntity(type, id)
{
    m_noGravity = true;
}

std::unique_ptr<Entity> ShulkerBulletEntity::create(IWorld* /*world*/) {
    return std::make_unique<ShulkerBulletEntity>(LegacyEntityType::Unknown, 0);
}

void ShulkerBulletEntity::tick() {
    // 更新飞行方向
    updateDirection();

    // 调用父类tick
    ProjectileEntity::tick();

    // 检查是否击中目标
    if (m_target && m_target->isAlive()) {
        // 计算到目标的方向
        Vector3 dir(
            m_target->x() - m_position.x,
            m_target->y() + m_target->eyeHeight() / 2.0f - m_position.y,
            m_target->z() - m_position.z
        );
        f32 dist = dir.length();
        if (dist > 0.0f) {
            dir = dir.normalized();
            m_direction = dir;
        }
    } else {
        // 目标消失，移除子弹
        remove();
    }
}

void ShulkerBulletEntity::updateDirection() {
    // 潜影贝子弹会沿轴向移动，并在需要时改变方向
    m_flightSteps++;

    // 每隔一段时间改变方向
    if (m_flightSteps % 10 == 0) {
        // 选择一个新的轴向方向
        // TODO: 实现轴向移动逻辑
    }
}

void ShulkerBulletEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 造成伤害并施加漂浮效果
    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::MobProjectile, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::MobProjectile, this, this, false);
    }

    // TODO: target->attackEntityFrom(*damageSource, 4.0f);

    // 施加漂浮效果
    // if (target instanceof LivingEntity) {
    //     ((LivingEntity)target).addEffect(new LevitationEffect(10 * 20, 1));
    // }

    remove();
}

void ShulkerBulletEntity::onBlockHit(const RayTraceResult& /*result*/) {
    remove();
}

// ============================================================================
// EvokerFangsEntity
// ============================================================================

EvokerFangsEntity::EvokerFangsEntity(LegacyEntityType type, EntityId id)
    : Entity(type, id)
{
    m_warmupDelay = 20;  // 默认预热时间
}

std::unique_ptr<Entity> EvokerFangsEntity::create(IWorld* /*world*/) {
    return std::make_unique<EvokerFangsEntity>(LegacyEntityType::Unknown, 0);
}

void EvokerFangsEntity::tick() {
    Entity::tick();

    m_ticksExisted++;

    if (m_warmupDelay > 0) {
        m_warmupDelay--;
        return;
    }

    // 预热完成后开始攻击
    if (!m_sentAttackEvent) {
        // 播放攻击动画
        // TODO: 生成尖牙动画

        // 对范围内的实体造成伤害
        // TODO: 检测并伤害范围内的实体

        m_sentAttackEvent = true;
    }

    // 存在一段时间后消失
    if (m_ticksExisted > 30) {
        remove();
    }
}

// ============================================================================
// EyeOfEnderEntity
// ============================================================================

EyeOfEnderEntity::EyeOfEnderEntity(LegacyEntityType type, EntityId id)
    : Entity(type, id)
{
    m_noGravity = false;
}

std::unique_ptr<Entity> EyeOfEnderEntity::create(IWorld* /*world*/) {
    return std::make_unique<EyeOfEnderEntity>(LegacyEntityType::Unknown, 0);
}

void EyeOfEnderEntity::tick() {
    Entity::tick();

    m_lifetime++;

    // 向目标移动
    if (m_targetX != 0 || m_targetZ != 0) {
        // 计算方向
        f32 dx = static_cast<f32>(m_targetX) - m_position.x;
        f32 dz = static_cast<f32>(m_targetZ) - m_position.z;
        f32 dist = std::sqrt(dx * dx + dz * dz);

        if (dist > 0.0f) {
            // 设置速度
            m_velocity.x = dx / dist * 0.5f;
            m_velocity.z = dz / dist * 0.5f;

            // Y轴波动
            m_velocity.y = std::sin(m_lifetime * 0.1f) * 0.1f;
        }
    }

    // 随机碎裂
    // 15% 概率碎裂
    // if (rand.nextInt(700) == 0) {
    //     m_break = true;
    //     remove();
    // }

    // 超时移除
    if (m_lifetime > 1200) {  // 60秒
        remove();
    }
}

void EyeOfEnderEntity::moveTo(BlockCoord targetX, BlockCoord targetZ) {
    m_targetX = targetX;
    m_targetZ = targetZ;
}

// ============================================================================
// FireworkRocketEntity
// ============================================================================

FireworkRocketEntity::FireworkRocketEntity(LegacyEntityType type, EntityId id)
    : ProjectileEntity(type, id)
{
    m_noGravity = false;
}

std::unique_ptr<Entity> FireworkRocketEntity::create(IWorld* /*world*/) {
    return std::make_unique<FireworkRocketEntity>(LegacyEntityType::Unknown, 0);
}

void FireworkRocketEntity::tick() {
    ProjectileEntity::tick();

    m_lifetime++;

    // 生成烟花粒子
    if (!m_inGround) {
        // TODO: 生成飞行粒子
    }

    // 检查是否爆炸
    if (m_lifetime >= m_flightTime * 10) {
        explode();
    }
}

void FireworkRocketEntity::explode() {
    // 如果从弩射出，对周围实体造成伤害
    if (m_shotFromCrossbow) {
        dealExplosionDamage();
    }

    // 生成烟花爆炸效果
    // TODO: 解析烟花数据并生成爆炸粒子

    remove();
}

void FireworkRocketEntity::dealExplosionDamage() {
    // TODO: 对范围内的实体造成爆炸伤害
    // 如果烟花有爆炸效果，每个爆炸效果造成 5-7 点伤害
    // 还会造成击退
}

} // namespace entity
} // namespace mc
