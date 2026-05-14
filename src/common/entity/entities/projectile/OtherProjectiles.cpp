#include "OtherProjectiles.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../entities/player/Player.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ============================================================================
// FishingBobberEntity 常量
// ============================================================================

namespace {
// 钓鱼时间常量（MC 1.16.5）
constexpr i32 MIN_WAIT_TICKS = 100;     // 最小等待时间 (5秒)
constexpr i32 MAX_WAIT_TICKS = 600;     // 最大等待时间 (30秒)
constexpr i32 LURE_REDUCTION = 100;     // 饵钓每级减少的时间 (5秒)
constexpr i32 MIN_CATCHABLE_TICKS = 20; // 最小捕获窗口 (1秒)
constexpr i32 MAX_CATCHABLE_TICKS = 40; // 最大捕获窗口 (2秒)
} // namespace

LlamaSpitEntity::LlamaSpitEntity(LegacyEntityType type, EntityId id)
    : ThrowableEntity(type, id)
{}

std::unique_ptr<Entity> LlamaSpitEntity::create(IWorld* /*world*/)
{
    return std::make_unique<LlamaSpitEntity>(LegacyEntityType::Unknown, 0);
}

void LlamaSpitEntity::onEntityHit(const RayTraceResult& result)
{
    if (!result.hitEntity) {
        return;
    }

    // 造成伤害
    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::MobProjectile, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::MobProjectile, this, this, false);
    }

    // TODO: target->attackEntityFrom(*damageSource, 1.0f);
}

void LlamaSpitEntity::onImpact(const RayTraceResult& /*result*/)
{
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

std::unique_ptr<Entity> FishingBobberEntity::create(IWorld* /*world*/)
{
    return std::make_unique<FishingBobberEntity>(LegacyEntityType::Unknown, 0);
}

void FishingBobberEntity::setShooter(Entity* shooter)
{
    // 设置钓鱼者（仅支持玩家）
    m_angler = dynamic_cast<Player*>(shooter);
}

void FishingBobberEntity::shootFrom(Entity& shooter, f32 pitch, f32 yaw, f32 pitchOffset, f32 velocity, f32 inaccuracy)
{
    // 设置发射者
    setShooter(&shooter);

    // 计算发射方向
    // MC 1.16.5: ProjectileHelper.func_234618_a_
    constexpr f32 DEG_TO_RAD = 0.017453292f; // PI / 180

    f32 radPitch = (pitch + pitchOffset) * DEG_TO_RAD;
    f32 radYaw = -yaw * DEG_TO_RAD;

    f32 x = -std::sin(radYaw) * std::cos(radPitch);
    f32 y = -std::sin(radPitch);
    f32 z = std::cos(radYaw) * std::cos(radPitch);

    // 归一化并乘以速度
    f32 length = std::sqrt(x * x + y * y + z * z);
    if (length > 0.0f) {
        x = x / length * velocity;
        y = y / length * velocity;
        z = z / length * velocity;
    }

    // 添加不准确性
    if (inaccuracy > 0.0f) {
        // MC 1.16.5: 使用世界的随机数生成器添加高斯偏移
        math::Random& random = shooter.world()->getRandom();
        f32 offsetX = random.nextGaussian() * inaccuracy * 0.0075f;
        f32 offsetY = random.nextGaussian() * inaccuracy * 0.0075f;
        f32 offsetZ = random.nextGaussian() * inaccuracy * 0.0075f;
        x += offsetX;
        y += offsetY;
        z += offsetZ;
    }

    // 设置速度
    m_velocity.x = x;
    m_velocity.y = y;
    m_velocity.z = z;
}

void FishingBobberEntity::tick()
{
    Entity::tick();

    m_lifetime++;

    // 检查钓鱼者是否存在
    if (!m_angler || !m_angler->isAlive()) {
        remove();
        return;
    }

    // 检查玩家是否还持有钓鱼竿
    // 如果玩家切换了物品或钓鱼浮标ID已被清除，则移除浮标
    if (!m_angler->isFishing() || m_angler->fishingBobber() != id()) {
        remove();
        return;
    }

    // 更新水面状态
    updateWaterState();

    switch (m_state) {
        case State::Flying:
            // 浮标在飞行中，检测是否入水
            if (isInWater()) {
                m_state = State::Bobbing;
                // 设置初始等待时间
                setWaitTime();
                // 检测是否在开放水域
                m_inOpenWater = checkOpenWater();
            }
            break;

        case State::Bobbing:
            // 浮标浮在水面，执行钓鱼逻辑
            if (isInWater()) {
                m_outOfWaterTime = std::max(0, m_outOfWaterTime - 1);
                // 检查开放水域状态（进入水后延迟检查）
                if (m_outOfWaterTime < 10) {
                    m_inOpenWater = m_inOpenWater && checkOpenWater();
                }
                catchingFish();
            } else {
                m_outOfWaterTime = std::min(10, m_outOfWaterTime + 1);
            }
            spawnFishingParticles();
            break;

        case State::Fishing:
            // 咬钩中，等待玩家收杆
            if (m_ticksCatchable > 0) {
                m_ticksCatchable--;
                m_fishAngle += 0.15f; // 鱼游动动画
                // 如果超时未收杆，重置状态
                if (m_ticksCatchable <= 0) {
                    m_state = State::Bobbing;
                    setWaitTime();
                }
            } else {
                m_state = State::Bobbing;
            }
            break;

        case State::Hooked:
            // 钩住实体（TODO: 实现钩住实体逻辑）
            break;
    }
}

void FishingBobberEntity::updateWaterState()
{
    // 通过检查碰撞箱判断是否在水中
    // Entity::isInWater() 已在 tick() 中更新
}

bool FishingBobberEntity::isInWater() const
{
    return Entity::isInWater();
}

bool FishingBobberEntity::checkOpenWater()
{
    // MC 1.16.5: 检查浮标位置周围是否满足开放水域条件
    // 需要检查 Y-1 到 Y+2 四层，每层 5x5 范围
    if (m_world == nullptr) {
        return false;
    }

    BlockPos bobberPos(static_cast<i32>(std::floor(m_position.x)),
        static_cast<i32>(std::floor(m_position.y)),
        static_cast<i32>(std::floor(m_position.z)));

    // 简化实现：检查浮标周围是否有足够的水
    // 完整实现需要检查每层的水类型
    i32 waterCount = 0;
    for (i32 dy = -1; dy <= 2; ++dy) {
        for (i32 dx = -2; dx <= 2; ++dx) {
            for (i32 dz = -2; dz <= 2; ++dz) {
                BlockPos checkPos(bobberPos.x + dx, bobberPos.y + dy, bobberPos.z + dz);
                const BlockState* state = m_world->getBlockState(checkPos);
                if (state != nullptr && state->isLiquid()) {
                    waterCount++;
                }
            }
        }
    }

    // 开放水域大约需要 75% 以上是水
    return waterCount >= 60; // 4层 * 25格 * 0.6 = 60
}

void FishingBobberEntity::catchingFish()
{
    // 阶段1：等待咬钩
    if (m_ticksCaughtDelay > 0) {
        m_ticksCaughtDelay--;

        // 接近咬钩时产生水花
        // MC 1.16.5 FishingBobberEntity.catchingFish() 第334-354行
        if (m_ticksCaughtDelay < 100 && m_ticksCaughtDelay % 10 == 0 && m_world) {
            // 生成水花粒子
            math::Random rng;
            f32 angle = rng.nextFloat() * 360.0f * math::DEG_TO_RAD;
            f32 radius = rng.nextFloat(25.0f, 60.0f) * 0.1f;
            f32 px = x() + std::sin(angle) * radius;
            f32 py = std::floor(y()) + 1.0f;
            f32 pz = z() + std::cos(angle) * radius;
            m_world->addParticle(client::renderer::trident::particle::ParticleTypeId::Splash,
                Vector3(px, py, pz),
                Vector3(0.0f, 0.0f, 0.0f),
                Vector3(0.1f, 0.0f, 0.1f),
                2 + rng.nextInt(2));
        }
        return;
    }

    // 阶段2：鱼接近浮标
    if (m_ticksCatchableDelay > 0) {
        m_ticksCatchableDelay--;

        // 产生气泡和钓鱼粒子
        // MC 1.16.5 FishingBobberEntity.catchingFish() 第306-324行
        if (m_ticksCatchableDelay % 5 == 0 && m_world) {
            math::Random rng;
            f32 angle = m_fishAngle * math::DEG_TO_RAD;
            f32 sinAngle = std::sin(angle);
            f32 cosAngle = std::cos(angle);
            f32 d0 = x() + sinAngle * static_cast<f32>(m_ticksCatchableDelay) * 0.1f;
            f32 d1 = std::floor(y()) + 1.0f;
            f32 d2 = z() + cosAngle * static_cast<f32>(m_ticksCatchableDelay) * 0.1f;

            // 15% 概率生成气泡
            if (rng.nextFloat() < 0.15f) {
                m_world->addParticle(client::renderer::trident::particle::ParticleTypeId::Bubble,
                    Vector3(d0, d1 - 0.1f, d2),
                    Vector3(sinAngle, 0.1f, cosAngle));
            }

            // 钓鱼涟漪粒子
            m_world->addParticle(client::renderer::trident::particle::ParticleTypeId::Fishing,
                Vector3(d0, d1, d2),
                Vector3(cosAngle * 0.04f, 0.01f, -sinAngle * 0.04f));
            m_world->addParticle(client::renderer::trident::particle::ParticleTypeId::Fishing,
                Vector3(d0, d1, d2),
                Vector3(-cosAngle * 0.04f, 0.01f, sinAngle * 0.04f));
        }

        // 鱼接近角度动画
        m_fishAngle += 0.1f;

        // 接近完成，进入可捕获状态
        if (m_ticksCatchableDelay <= 0) {
            m_ticksCatchable = math::Random().nextInt(MIN_CATCHABLE_TICKS, MAX_CATCHABLE_TICKS);
            m_state = State::Fishing;
            // MC 1.16.5: 播放水溅音效
            playSound(SoundEvents::ENTITY_FISHING_BOBBER_SPLASH,
                0.25f,
                1.0f + (math::Random().nextFloat() - math::Random().nextFloat()) * 0.4f);
        }
        return;
    }

    // 阶段0：初始化等待
    if (m_ticksCaughtDelay <= 0 && m_ticksCatchableDelay <= 0 && m_ticksCatchable <= 0) {
        // 设置下一轮等待时间
        setWaitTime();
    }
}

void FishingBobberEntity::spawnFishingParticles()
{
    // MC 1.16.5: 钓鱼粒子效果
    // 浮标在水面时的涟漪效果
    if (isInWater() && m_world) {
        math::Random rng;
        if (rng.nextInt(5) == 0) {
            m_world->addParticle(
                client::renderer::trident::particle::ParticleTypeId::Fishing, m_position, Vector3(0.0f, 0.01f, 0.0f));
        }
    }
}

void FishingBobberEntity::setWaitTime()
{
    // MC 1.16.5: 设置咬钩等待时间
    // 基础时间: 100-600 ticks
    // 饵钓附魔: 每级减少 100 ticks
    math::Random rng;
    m_ticksCaughtDelay = rng.nextInt(MIN_WAIT_TICKS, MAX_WAIT_TICKS);
    m_ticksCaughtDelay -= m_speedBonus * LURE_REDUCTION;
    m_ticksCaughtDelay = std::max(20, m_ticksCaughtDelay); // 最小 1 秒

    // 鱼接近时间
    m_ticksCatchableDelay = 0;
    m_ticksCatchable = 0;
}

i32 FishingBobberEntity::spawnCatchItems()
{
    // TODO: 使用钓鱼掉落表生成物品
    // 目前返回 1 表示钓到鱼（用于耐久消耗）
    return 1;
}

i32 FishingBobberEntity::reelIn()
{
    // 收杆
    i32 damage = 0; // 钓鱼竿耐久消耗

    if (m_state == State::Fishing && m_ticksCatchable > 0) {
        // 成功钓到鱼
        damage = spawnCatchItems();
        // TODO: 生成经验球 (1-6 经验)
        // TODO: 生成钓鱼物品实体
        remove();
    } else if (m_state == State::Hooked) {
        // 钩住实体，拉过来
        // TODO: 实现钩住实体的逻辑
        remove();
    } else {
        // 未咬钩时收杆，无耐久消耗
        remove();
    }

    return damage;
}

// ============================================================================
// ShulkerBulletEntity
// ============================================================================

ShulkerBulletEntity::ShulkerBulletEntity(LegacyEntityType type, EntityId id)
    : ProjectileEntity(type, id)
{
    m_noGravity = true;
}

std::unique_ptr<Entity> ShulkerBulletEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ShulkerBulletEntity>(LegacyEntityType::Unknown, 0);
}

void ShulkerBulletEntity::tick()
{
    // 更新飞行方向
    updateDirection();

    // 调用父类tick
    ProjectileEntity::tick();

    // 检查是否击中目标
    if (m_target && m_target->isAlive()) {
        // 计算到目标的方向
        Vector3 dir(m_target->x() - m_position.x,
            m_target->y() + m_target->eyeHeight() / 2.0f - m_position.y,
            m_target->z() - m_position.z);
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

void ShulkerBulletEntity::updateDirection()
{
    // 潜影贝子弹会沿轴向移动，并在需要时改变方向
    m_flightSteps++;

    // 每隔一段时间改变方向
    if (m_flightSteps % 10 == 0) {
        // 选择一个新的轴向方向
        // TODO: 实现轴向移动逻辑
    }
}

void ShulkerBulletEntity::onEntityHit(const RayTraceResult& result)
{
    if (!result.hitEntity) {
        return;
    }

    // 造成伤害并施加漂浮效果
    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::MobProjectile, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::MobProjectile, this, this, false);
    }

    // TODO: target->attackEntityFrom(*damageSource, 4.0f);

    // 施加漂浮效果
    // if (target instanceof LivingEntity) {
    //     ((LivingEntity)target).addEffect(new LevitationEffect(10 * 20, 1));
    // }

    remove();
}

void ShulkerBulletEntity::onBlockHit(const RayTraceResult& /*result*/)
{
    remove();
}

// ============================================================================
// EvokerFangsEntity
// ============================================================================

EvokerFangsEntity::EvokerFangsEntity(LegacyEntityType type, EntityId id)
    : Entity(type, id)
{
    m_warmupDelay = 20; // 默认预热时间
}

std::unique_ptr<Entity> EvokerFangsEntity::create(IWorld* /*world*/)
{
    return std::make_unique<EvokerFangsEntity>(LegacyEntityType::Unknown, 0);
}

void EvokerFangsEntity::tick()
{
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

std::unique_ptr<Entity> EyeOfEnderEntity::create(IWorld* /*world*/)
{
    return std::make_unique<EyeOfEnderEntity>(LegacyEntityType::Unknown, 0);
}

void EyeOfEnderEntity::tick()
{
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
    if (m_lifetime > 1200) { // 60秒
        remove();
    }
}

void EyeOfEnderEntity::moveTo(BlockCoord targetX, BlockCoord targetZ)
{
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

std::unique_ptr<Entity> FireworkRocketEntity::create(IWorld* /*world*/)
{
    return std::make_unique<FireworkRocketEntity>(LegacyEntityType::Unknown, 0);
}

void FireworkRocketEntity::tick()
{
    ProjectileEntity::tick();

    m_lifetime++;

    // 生成烟花粒子
    // TODO: 检查是否在地面
    // if (!m_inGround) {
    //     // TODO: 生成飞行粒子
    // }

    // 检查是否爆炸
    if (m_lifetime >= m_flightTime * 10) {
        explode();
    }
}

void FireworkRocketEntity::explode()
{
    // 如果从弩射出，对周围实体造成伤害
    if (m_shotFromCrossbow) {
        dealExplosionDamage();
    }

    // 生成烟花爆炸效果
    // TODO: 解析烟花数据并生成爆炸粒子

    remove();
}

void FireworkRocketEntity::dealExplosionDamage()
{
    // TODO: 对范围内的实体造成爆炸伤害
    // 如果烟花有爆炸效果，每个爆炸效果造成 5-7 点伤害
    // 还会造成击退
}

} // namespace entity
} // namespace mc
