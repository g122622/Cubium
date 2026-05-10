#include "EffectEntities.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/explosion/ExplosionMode.hpp"
#include "../player/Player.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <cmath>
#include <chrono>

namespace mc {
namespace entity {

// ==================== EnderCrystalEntity ====================

EnderCrystalEntity::EnderCrystalEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
}

f32 EnderCrystalEntity::width() const {
    return 2.0f;  // MC 1.16.5: 末地水晶宽度
}

f32 EnderCrystalEntity::height() const {
    return 2.0f;  // MC 1.16.5: 末地水晶高度
}

void EnderCrystalEntity::tick() {
    Entity::tick();

    // 治愈末影龙冷却
    if (m_healCooldown > 0) {
        m_healCooldown--;
    }

    // MC 1.16.5: 客户端生成光束粒子效果
    // 参考: EnderCrystalEntity.tick() - 末地水晶在有光束目标时生成 EndRod 粒子
    if (world() != nullptr && world()->isClientSide()) {
        using namespace client::renderer::trident::particle;

        // MC 1.16.5: 初始化随机旋转值（构造函数中 this.rand.nextInt(100000)）
        if (!m_rotationInitialized) {
            math::Random& random = world()->getRandom();
            m_innerRotation = random.nextInt(100000);
            m_rotationInitialized = true;
        }

        // 如果有光束目标，生成指向目标的粒子
        if (hasBeamTarget()) {
            // 增加旋转角度用于动画
            m_innerRotation++;

            // 每tick生成 EndRod 粒子
            // 粒子从水晶位置向光束目标方向移动
            math::Random& random = world()->getRandom();

            // 计算光束方向
            Vector3 crystalCenter = m_position + Vector3(0.0f, 1.0f, 0.0f);  // 水晶中心
            Vector3 targetPos(
                static_cast<f32>(m_beamTarget.x) + 0.5f,
                static_cast<f32>(m_beamTarget.y) + 0.5f,
                static_cast<f32>(m_beamTarget.z) + 0.5f
            );

            // 方向向量（未归一化，用于粒子速度）
            Vector3 direction = targetPos - crystalCenter;
            f32 length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

            if (length > 0.001f) {
                // 归一化方向
                Vector3 normalizedDir(direction.x / length, direction.y / length, direction.z / length);

                // 在水晶位置生成粒子，向目标方向移动
                // 粒子位置带随机偏移
                f32 px = static_cast<f32>(x()) + 0.5f + (random.nextFloat() - 0.5f) * 0.5f;
                f32 py = static_cast<f32>(y()) + 1.0f + (random.nextFloat() - 0.5f) * 0.5f;
                f32 pz = static_cast<f32>(z()) + 0.5f + (random.nextFloat() - 0.5f) * 0.5f;

                // 粒子速度：向光束目标方向移动
                f32 speed = 0.1f + random.nextFloat() * 0.05f;

                world()->addParticle(
                    ParticleTypeId::EndRod,
                    Vector3(px, py, pz),
                    Vector3(normalizedDir.x * speed, normalizedDir.y * speed, normalizedDir.z * speed)
                );
            }
        } else {
            // 没有光束目标时，仍然更新旋转动画
            m_innerRotation++;
        }
    }
}

bool EnderCrystalEntity::hasBeamTarget() const {
    return m_beamTarget.x != 0 || m_beamTarget.y != 0 || m_beamTarget.z != 0;
}

void EnderCrystalEntity::setBeamTarget(BlockPos pos) {
    m_beamTarget = pos;
}

void EnderCrystalEntity::healDragon() {
    // TODO: 找到末影龙并治愈
}

void EnderCrystalEntity::explode() {
    // MC 1.16.5: 末地水晶爆炸
    // 参考: EnderCrystalEntity.attackEntityFrom() line 105
    // this.world.createExplosion((Entity)null, this.getPosX(), this.getPosY(), this.getPosZ(), 6.0F, Explosion.Mode.DESTROY);
    IWorld* worldPtr = world();
    if (worldPtr != nullptr) {
        // 爆炸半径 6.0，模式 DESTROY（破坏方块并掉落物品）
        worldPtr->createExplosion(
            m_position,
            6.0f,  // MC 1.16.5: 末地水晶爆炸半径
            world::explosion::ExplosionMode::Destroy,
            false,  // 不生成火焰
            nullptr  // 无爆炸源实体
        );
    }
    remove();
}

// ==================== LightningBoltEntity ====================

LightningBoltEntity::LightningBoltEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
    // MC 1.16.5: ignoreFrustumCheck = true
    // 闪电总是可见，即使不在视锥内
}

f32 LightningBoltEntity::width() const {
    return 0.0f;  // MC 1.16.5: 闪电没有碰撞箱
}

f32 LightningBoltEntity::height() const {
    return 0.0f;  // MC 1.16.5: 闪电没有碰撞箱
}

void LightningBoltEntity::initializeState() {
    // MC 1.16.5 构造函数中的初始化：
    // lightningState = 2
    // boltVertex = rand.nextLong()
    // boltLivingTime = rand.nextInt(3) + 1

    m_lightningState = 2;

    // 使用世界种子或随机数生成 boltVertex
    if (m_world != nullptr) {
        math::Random rng(static_cast<u64>(m_world->currentTick()) ^ m_world->seed());
        m_boltVertex = rng.nextLong();
        m_boltLivingTime = rng.nextInt(1, 3);  // 1-3
    } else {
        // 无世界时使用确定性种子（基于时间）
        math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
        m_boltVertex = rng.nextLong();
        m_boltLivingTime = rng.nextInt(1, 3);
    }

    m_initialized = true;
}

void LightningBoltEntity::tick() {
    Entity::tick();

    // MC 1.16.5: 首次 tick 初始化状态
    if (!m_initialized) {
        initializeState();
    }

    // MC 1.16.5: lightningState == 2 时执行初始效果
    // 播放音效、点燃方块、造成伤害
    if (m_lightningState == 2) {
        // MC 1.16.5: 难度检查 - NORMAL 和 HARD 点燃更多火焰
        if (m_world != nullptr && !m_effectOnly && !m_world->isClientSide()) {
            Difficulty difficulty = m_world->difficulty();
            if (difficulty == Difficulty::Normal || difficulty == Difficulty::Hard) {
                igniteBlocks(4);
            } else {
                igniteBlocks(0);
            }
        }

        // MC 1.16.5: 播放雷声音效
        // 音量 10000（非常大的范围），音调 0.8-1.0
        if (m_world != nullptr) {
            // 使用 boltVertex 生成一致的随机音调
            f32 thunderPitch = 0.8f + static_cast<f32>(m_boltVertex % 100) / 100.0f * 0.2f;
            m_world->playSound(
                SoundEvents::WEATHER_THUNDER,
                sound::SoundCategory::Weather,
                m_position,
                10000.0f,  // MC 1.16.5: 10000 音量（可传很远）
                thunderPitch
            );

            // MC 1.16.5: 播放雷击声音效（音量 2，音调 0.5-0.7）
            f32 impactPitch = 0.5f + static_cast<f32>((m_boltVertex >> 8) % 100) / 100.0f * 0.2f;
            m_world->playSound(
                SoundEvents::WEATHER_THUNDER,
                sound::SoundCategory::Weather,
                m_position,
                2.0f,
                impactPitch
            );
        }

        // MC 1.16.5: 服务端造成伤害（非 effectOnly，非客户端）
        if (m_world != nullptr && !m_world->isClientSide() && !m_effectOnly) {
            damageEntities();
        }

        // MC 1.16.5: 客户端设置闪电闪烁效果
        // world.setTimeLightningFlash(2) - 需要在 ClientWorld 中实现
        // TODO: 实现客户端闪电闪烁效果
    }

    // MC 1.16.5: 递减 lightningState
    --m_lightningState;

    // MC 1.16.5: lightningState < 0 时检查是否"复活"
    if (m_lightningState < 0) {
        if (m_boltLivingTime == 0) {
            // 所有视觉效果结束，移除实体
            remove();
        } else if (m_lightningState < -static_cast<i32>(m_boltVertex % 10)) {
            // MC 1.16.5: 随机间隔后"复活"
            // 闪电会多次闪烁，模拟真实闪电效果
            --m_boltLivingTime;
            m_lightningState = 1;

            // 生成新的随机种子用于渲染
            if (m_world != nullptr) {
                math::Random rng(static_cast<u64>(m_world->currentTick()) ^ m_boltVertex);
                m_boltVertex = rng.nextLong();
            } else {
                math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()) ^ m_boltVertex);
                m_boltVertex = rng.nextLong();
            }

            // "复活"时再次尝试点燃（不额外点燃）
            igniteBlocks(0);
        }
    }
}

void LightningBoltEntity::igniteBlocks(i32 extraIgnitions) {
    // MC 1.16.5 igniteBlocks():
    // 检查游戏规则 doFireTick 和是否为客户端
    if (m_effectOnly || m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    // MC 1.16.5: 检查游戏规则 doFireTick
    if (!m_world->doFireTick()) {
        return;
    }

    // 获取当前位置
    BlockPos blockPos(static_cast<i32>(std::floor(m_position.x)),
                      static_cast<i32>(std::floor(m_position.y)),
                      static_cast<i32>(std::floor(m_position.z)));

    // 获取火焰方块状态
    const BlockState* fireState = nullptr;
    if (VanillaBlocks::FIRE != nullptr) {
        fireState = &VanillaBlocks::FIRE->defaultState();
    }

    if (fireState == nullptr) {
        return;
    }

    // MC 1.16.5: 在当前位置放置火焰
    const BlockState* currentState = m_world->getBlockState(blockPos);
    // 检查是否为空气方块
    if (currentState != nullptr && currentState->isAir()) {
        // TODO: AbstractFireBlock.getFireForPlacement() - 检查火焰是否可以放置在当前位置
        // 目前直接放置普通火焰
        m_world->setBlockState(blockPos, fireState);
    }

    // MC 1.16.5: 额外点燃周围方块
    if (extraIgnitions > 0) {
        math::Random rng(m_boltVertex);

        for (i32 i = 0; i < extraIgnitions; ++i) {
            // MC 1.16.5: pos.add(rand.nextInt(3) - 1, rand.nextInt(3) - 1, rand.nextInt(3) - 1)
            i32 dx = rng.nextInt(3) - 1;
            i32 dy = rng.nextInt(3) - 1;
            i32 dz = rng.nextInt(3) - 1;

            BlockPos firePos(blockPos.x + dx, blockPos.y + dy, blockPos.z + dz);

            const BlockState* stateAtPos = m_world->getBlockState(firePos);
            if (stateAtPos != nullptr && stateAtPos->isAir()) {
                // TODO: AbstractFireBlock.getFireForPlacement() - 检查火焰是否可以放置
                m_world->setBlockState(firePos, fireState);
            }
        }
    }
}

void LightningBoltEntity::damageEntities() {
    // MC 1.16.5: 获取 3x6x3 范围内的实体
    // AxisAlignedBB(pos.x - 3, pos.y - 3, pos.z - 3, pos.x + 3, pos.y + 6 + 3, pos.z + 3)
    if (m_world == nullptr || m_effectOnly) {
        return;
    }

    // 构建碰撞箱
    // MC 1.16.5: new AxisAlignedBB(posX - 3.0, posY - 3.0, posZ - 3.0, posX + 3.0, posY + 6.0 + 3.0, posZ + 3.0)
    AxisAlignedBB box(
        m_position.x - DAMAGE_RADIUS_XZ,
        m_position.y - DAMAGE_RADIUS_Y_OFFSET,
        m_position.z - DAMAGE_RADIUS_XZ,
        m_position.x + DAMAGE_RADIUS_XZ,
        m_position.y + DAMAGE_RADIUS_Y + DAMAGE_RADIUS_Y_OFFSET,
        m_position.z + DAMAGE_RADIUS_XZ
    );

    // 获取范围内的实体
    std::vector<Entity*> entities = m_world->getEntitiesInAABB(box, this);

    for (Entity* entity : entities) {
        if (entity == nullptr || !entity->isAlive()) {
            continue;
        }

        // MC 1.16.5: 调用 entity.func_241841_a() (onStruckByLightning)
        // 对于 LivingEntity，造成闪电伤害
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (living != nullptr) {
            // 创建闪电伤害来源
            auto damageSource = DamageSources::lightningBolt(this);
            // MC 1.16.5: 闪电伤害为 5.0
            living->hurt(damageSource, 5.0f);
        }

        // MC 1.16.5: 调用实体的 onStruckByLightning() 方法
        // 用于处理特殊效果（如哞菇变色、苦力怕充能等）
        entity->onStruckByLightning();

        // TODO: MC 1.16.5 触发进度 CriteriaTriggers.CHANNELED_LIGHTNING
        // 如果有 caster（引雷附魔的玩家），触发进度
    }
}

// ==================== AreaEffectCloudEntity ====================

AreaEffectCloudEntity::AreaEffectCloudEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
}

f32 AreaEffectCloudEntity::width() const {
    return m_radius * 2.0f;  // MC 1.16.5: 实际宽度是半径的两倍
}

f32 AreaEffectCloudEntity::height() const {
    return 0.5f;  // MC 1.16.5: 药水云高度固定为 0.5
}

void AreaEffectCloudEntity::tick() {
    Entity::tick();

    m_ticksLived++;

    // 等待时间结束后开始应用效果
    if (m_ticksLived > m_waitTime) {
        // 每隔一段时间应用效果
        if (m_ticksLived % m_reapplicationDelay == 0) {
            applyEffects();
        }

        // 更新半径
        updateRadius();

        // 检查是否过期
        if (m_ticksLived >= m_duration) {
            remove();
        }
    }
}

void AreaEffectCloudEntity::applyEffects() {
    // TODO: 应用效果到范围内的实体
    if (m_durationOnUse > 0) {
        m_duration = std::max(0, m_duration - m_durationOnUse);
    }
}

void AreaEffectCloudEntity::updateRadius() {
    m_radius += RADIUS_GROWTH;
    m_radius = std::max(0.5f, m_radius);
}

// 注意: ExperienceOrbEntity 已移动到独立的 orb/ 目录
// 文件位置: src/common/entity/entities/orb/ExperienceOrbEntity.cpp

// ==================== ArmorStandEntity ====================

ArmorStandEntity::ArmorStandEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{
}

f32 ArmorStandEntity::width() const {
    return m_marker ? 0.0f : 0.5f;  // MC 1.16.5: 标记模式无碰撞箱，否则 0.5
}

f32 ArmorStandEntity::height() const {
    return m_marker ? 0.0f : 1.975f;  // MC 1.16.5: 标记模式无碰撞箱，否则 1.975
}

void ArmorStandEntity::tick() {
    Entity::tick();

    // 如果不是标记模式，应用重力
    if (!m_marker && m_hasGravity) {
        Vector3 vel = velocity();
        vel.y -= 0.04f; // 重力
        move(vel.x, vel.y, vel.z);

        // 减速
        vel.x *= 0.98f;
        vel.y *= 0.98f;
        vel.z *= 0.98f;
        setVelocity(vel);
    }
}

void ArmorStandEntity::setHeadRotation(f32 x, f32 y, f32 z) {
    m_head = {x, y, z};
}

void ArmorStandEntity::setBodyRotation(f32 x, f32 y, f32 z) {
    m_body = {x, y, z};
}

void ArmorStandEntity::setLeftArmRotation(f32 x, f32 y, f32 z) {
    m_leftArm = {x, y, z};
}

void ArmorStandEntity::setRightArmRotation(f32 x, f32 y, f32 z) {
    m_rightArm = {x, y, z};
}

void ArmorStandEntity::setLeftLegRotation(f32 x, f32 y, f32 z) {
    m_leftLeg = {x, y, z};
}

void ArmorStandEntity::setRightLegRotation(f32 x, f32 y, f32 z) {
    m_rightLeg = {x, y, z};
}

} // namespace entity
} // namespace mc
