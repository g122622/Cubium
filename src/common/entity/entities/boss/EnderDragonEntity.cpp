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

#include "EnderDragonEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/core/MoverType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/experience/ExperienceDropHandler.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/end/EndDragonFight.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

namespace mc {
namespace entity {

using namespace mc::math;
using ParticleTypeId = particle::ParticleTypeId;

// ============================================================================
// BossEntity
// ============================================================================

BossEntity::BossEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MobEntity(id, registry)
{
    // Boss 级实体默认显示生命条
}

// ============================================================================
// EnderDragonPartEntity
// ============================================================================

EnderDragonPartEntity::EnderDragonPartEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : Entity(id, nullptr, registry)
{
    // 部件尺寸较小，用于碰撞检测
}

void EnderDragonPartEntity::tick()
{
    Entity::tick();
    // 部件位置由父龙的 _updateDragonParts() 更新
}

void EnderDragonPartEntity::updatePosition(f32 offsetX, f32 offsetY, f32 offsetZ, f32 width, f32 height)
{
    if (!m_parent) {
        return;
    }

    // 根据父龙的旋转计算实际位置
    f32 yawRad = m_parent->yaw() * (math::PI / 180.0f);
    f32 sinYaw = std::sin(yawRad);
    f32 cosYaw = std::cos(yawRad);

    // 旋转变换
    f32 rotatedX = offsetX * cosYaw - offsetZ * sinYaw;
    f32 rotatedZ = offsetX * sinYaw + offsetZ * cosYaw;

    // 设置位置
    setPosition(m_parent->x() + rotatedX, m_parent->y() + offsetY, m_parent->z() + rotatedZ);

    MC_UNUSED(width);
    MC_UNUSED(height);
}

// ============================================================================
// EnderDragonEntity
// ============================================================================

std::unique_ptr<Entity> EnderDragonEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<EnderDragonEntity>(EntityInstanceId(0), registry);
}

EnderDragonEntity::EnderDragonEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : BossEntity(id, registry)
{
    // 初始化龙部件
    initDragonParts();

    // 注册属性
    registerAttributes();
    registerGoals();

    // 初始化路径点
    initPathPoints();

    // 构造时设置满血，对齐 MC 1.21.11 EnderDragon 构造函数中的
    // this.setHealth(this.getMaxHealth());
    // registerAttributes() 已设置 MAX_HEALTH=200，此处显式赋值确保 m_health 同步。
    // MC 原版还在此处设置 noPhysics=true，Cubium 通过物理系统侧的碰撞过滤处理，
    // LivingEntity 不暴露 noPhysics 接口，因此此处不重复设置。
    setHealth(maxHealth());
}

void EnderDragonEntity::initDragonParts()
{
    // 创建所有龙部件
    // 部件列表顺序：头、颈、身、尾1、尾2、尾3、左翼、右翼

    // ECS 迁移：部件构造需要 registry 句柄，从本实体（已由基类构造建立）的 context 取。
    auto& registry = m_entityContext->registry();

    // 头部
    m_dragonPartHead = new EnderDragonPartEntity(EntityInstanceId(0), registry);
    m_dragonPartHead->setPart(EnderDragonPartEntity::Part::Head);
    m_dragonPartHead->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartHead);

    // 颈部
    m_dragonPartNeck = new EnderDragonPartEntity(EntityInstanceId(0), registry);
    m_dragonPartNeck->setPart(EnderDragonPartEntity::Part::Neck);
    m_dragonPartNeck->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartNeck);

    // 身体
    m_dragonPartBody = new EnderDragonPartEntity(EntityInstanceId(0), registry);
    m_dragonPartBody->setPart(EnderDragonPartEntity::Part::Body);
    m_dragonPartBody->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartBody);

    // 尾部1
    m_dragonPartTail1 = new EnderDragonPartEntity(EntityInstanceId(0), registry);
    m_dragonPartTail1->setPart(EnderDragonPartEntity::Part::Tail1);
    m_dragonPartTail1->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartTail1);

    // 尾部2
    m_dragonPartTail2 = new EnderDragonPartEntity(EntityInstanceId(0), registry);
    m_dragonPartTail2->setPart(EnderDragonPartEntity::Part::Tail2);
    m_dragonPartTail2->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartTail2);

    // 尾部3
    m_dragonPartTail3 = new EnderDragonPartEntity(EntityInstanceId(0), registry);
    m_dragonPartTail3->setPart(EnderDragonPartEntity::Part::Tail3);
    m_dragonPartTail3->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartTail3);

    // 左翼
    m_dragonPartLeftWing = new EnderDragonPartEntity(EntityInstanceId(0), registry);
    m_dragonPartLeftWing->setPart(EnderDragonPartEntity::Part::WingLeft);
    m_dragonPartLeftWing->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartLeftWing);

    // 右翼
    m_dragonPartRightWing = new EnderDragonPartEntity(EntityInstanceId(0), registry);
    m_dragonPartRightWing->setPart(EnderDragonPartEntity::Part::WingRight);
    m_dragonPartRightWing->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartRightWing);
}

std::optional<ResourceLocation> EnderDragonEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> EnderDragonEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

void EnderDragonEntity::tick()
{
    // 更新动画时间
    // MC 原版：栖息阶段 flapTime += 0.1F，碰墙时 flapTime += f7 * 0.5F（半速），正常 flapTime += f7
    m_prevAnimTime = m_animTime;
    if (isDying()) {
        // 死亡时动画加速
        m_animTime += 0.02f;
    } else if (m_slowed) {
        // 碰到 DRAGON_IMMUNE 方块时翅膀扇动速度减半
        m_animTime += 0.005f;
    } else {
        m_animTime += 0.01f;
    }

    // 先更新龙部件位置（基于龙当前位置），对齐 MC 1.21.11 的调用顺序：
    // MC 在 LivingEntity.tick() → aiStep() 中通过 tickPart() 更新部件位置，
    // 随后 tickDeath()（若 isDead()）才执行死亡动画的部件位移。
    // 因此 _updateDragonParts() 必须在 BossEntity::tick() 之前调用，
    // 使死亡动画 _onDeathUpdate() 中的 part->setPosition(part.pos + riseVelocity)
    // 成为最终位置（覆盖 _updateDragonParts() 的结果）。
    _updateDragonParts();

    // 调用父类 tick
    // 注意：LivingEntity::tick() 会在 isDead() 时调用 tickDeath()，
    // 而 EnderDragon 重写了 tickDeath() 委托给 _onDeathUpdate()，
    // 因此死亡动画逻辑会在 BossEntity::tick() 内部触发，无需在此重复调用。
    BossEntity::tick();

    // 同步 Boss 栏血量/名称到 EndDragonFight
    // 对应 MC 1.21.11 EnderDragon.aiStep() 中：
    //   if (this.dragonFight != null) { this.dragonFight.updateDragon(this); }
    // 注意：死亡动画期间的 updateDragon 调用在 _onDeathUpdate() 中，
    // 此处仅处理龙存活时的常规同步。
    if (!isDead()) {
        IWorld* worldPtr = world();
        if (worldPtr != nullptr) {
            EndDragonFight* fight = worldPtr->dragonFight();
            if (fight != nullptr) {
                fight->updateDragon(*this);
            }
        }
    }

    // 更新末影水晶
    _updateDragonEnderCrystal();

    // 处理碰撞
    _collideWithEntities();
}

void EnderDragonEntity::setPhase(Phase phase)
{
    // 切换阶段
    if (phase == m_phase) {
        return;
    }

    m_phase = phase;

    // 阶段切换时的特殊处理
    switch (phase) {
        case Phase::Dying:
            m_deathTicks = 0;
            break;
        case Phase::ChargingPlayer:
        case Phase::StrafePlayer:
            // 攻击阶段
            break;
        default:
            break;
    }
}

bool EnderDragonEntity::attackEntityPartFrom(EnderDragonPartEntity* part, DamageSource& source, f32 damage)
{
    // 所有部件的伤害都传递给龙本体
    if (isDead() || isDying() || isInvulnerableTo(source)) {
        return false;
    }

    // 只有玩家直接攻击或爆炸类型伤害才能对末影龙造成伤害
    // MC 原版：source.getEntity() instanceof Player || source.is(DamageTypeTags.ALWAYS_HURTS_ENDER_DRAGONS)
    bool canHurt = source.isPlayerSource() || source.isExplosion();
    if (!canHurt) {
        return false;
    }

    // 非头部伤害减伤：damage / 4 + min(damage, 1)
    f32 actualDamage = damage;
    if (part && part->part() != EnderDragonPartEntity::Part::Head) {
        actualDamage = damage / 4.0f + std::min(damage, 1.0f);
    }

    // 伤害低于 0.01 时忽略
    if (actualDamage < 0.01f) {
        return false;
    }

    // 应用伤害到龙本体
    bool hurt = BossEntity::hurt(source, actualDamage);

    if (hurt) {
        // 设置被攻击计时器，用于阶段切换判断
        m_sittingDamageReceived += static_cast<i32>(actualDamage);
    }

    return hurt;
}

void EnderDragonEntity::onCrystalDestroyed(EnderCrystalEntity* crystal, const BlockPos& pos, DamageSource& source)
{
    // 末影水晶被破坏时，龙会受到伤害
    if (isDead()) {
        return;
    }

    // 计算龙到水晶的距离
    f64 dx = x() - static_cast<f64>(pos.x);
    f64 dy = y() - static_cast<f64>(pos.y);
    f64 dz = z() - static_cast<f64>(pos.z);
    f64 distSq = dx * dx + dy * dy + dz * dz;

    // 如果水晶在龙的回血范围内，对龙造成伤害
    // 回血范围大约是 32 格
    constexpr f64 HEAL_RANGE_SQ = 32.0 * 32.0;

    if (distSq < HEAL_RANGE_SQ) {
        // MC 原版：仅当被破坏的水晶是龙当前绑定的最近水晶时才对龙头部造成伤害
        // 先检查再清除，避免清除后检查结果不正确
        bool isClosestCrystal = (crystal == m_closestEnderCrystal);

        // 清除回血目标
        if (isClosestCrystal) {
            m_closestEnderCrystal = nullptr;
        }

        if (isClosestCrystal) {
            // MC 原版：对龙头部造成爆炸伤害，伤害值为 10
            // 使用 IndirectEntityDamageSource 表示由水晶引起的爆炸伤害
            // crystal 是直接来源（水晶），causeEntity 是造成者（玩家）
            // MC 原版逻辑：如果 source.getEntity() 是 Player，直接使用；
            // 否则搜索水晶位置 64 格内最近的玩家作为 fallback
            Entity* sourceEntity = source.getEntity();
            Player* causePlayer = nullptr;

            if (sourceEntity) {
                causePlayer = dynamic_cast<Player*>(sourceEntity);
            }

            // MC 原版：CRYSTAL_DESTROY_TARGETING = TargetingConditions.forCombat().range(64.0)
            // 如果伤害来源不是玩家，搜索附近最近的玩家
            if (!causePlayer) {
                IWorld* worldPtr = world();
                if (worldPtr) {
                    causePlayer = worldPtr->getClosestPlayer(Vector3(static_cast<f32>(pos.x) + 0.5f,
                                                                 static_cast<f32>(pos.y) + 0.5f,
                                                                 static_cast<f32>(pos.z) + 0.5f),
                        64.0f);
                }
            }

            auto explosionDamage = DamageSources::explosion(crystal, causePlayer);
            hurt(explosionDamage, 10.0f);
        }
    }
}

void EnderDragonEntity::initPathPoints()
{
    // 初始化末影龙飞行路径点
    // 围绕末地中心的8个路径点
    m_pathPoints.clear();

    for (i32 i = 0; i < 8; ++i) {
        f32 angle = static_cast<f32>(i) * (math::PI * 2.0f / 8.0f);
        m_pathPoints.emplace_back(
            static_cast<BlockCoord>(std::cos(angle) * 64.0), 64, static_cast<BlockCoord>(std::sin(angle) * 64.0));
    }

    m_currentPathPoint = 0;
}

i32 EnderDragonEntity::getNearestPathPointIndex(f64 x, f64 y, f64 z) const
{
    // 获取最近的路径点索引
    if (m_pathPoints.empty()) {
        return 0;
    }

    i32 nearestIndex = 0;
    f64 minDistSq = std::numeric_limits<f64>::max();

    for (size_t i = 0; i < m_pathPoints.size(); ++i) {
        const BlockPos& point = m_pathPoints[i];
        f64 dx = x - static_cast<f64>(point.x);
        f64 dy = y - static_cast<f64>(point.y);
        f64 dz = z - static_cast<f64>(point.z);
        f64 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < minDistSq) {
            minDistSq = distSq;
            nearestIndex = static_cast<i32>(i);
        }
    }

    return nearestIndex;
}

void EnderDragonEntity::registerGoals()
{
    BossEntity::registerGoals();

    // 末影龙使用特殊的阶段系统，不使用普通AI目标
    // 阶段管理在 PhaseManager 中处理
}

void EnderDragonEntity::registerAttributes()
{
    BossEntity::registerAttributes();

    // 末影龙属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 200.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 256.0);
}

void EnderDragonEntity::_updateDragonParts()
{
    // 更新龙部件位置
    // 使用环形缓冲区记录位置历史，用于颈部和尾部的平滑动画

    // 更新环形缓冲区
    m_ringBufferIndex = (m_ringBufferIndex + 1) % RING_BUFFER_SIZE;
    m_ringBuffer[m_ringBufferIndex][0] = x();
    m_ringBuffer[m_ringBufferIndex][1] = y();
    m_ringBuffer[m_ringBufferIndex][2] = z();

    // 获取历史位置
    auto getHistoryPos = [this](i32 offset) -> Vector3 {
        i32 index = (m_ringBufferIndex - offset + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
        return Vector3(m_ringBuffer[index][0], m_ringBuffer[index][1], m_ringBuffer[index][2]);
    };

    // 更新头部位置（在龙前方）
    if (m_dragonPartHead) {
        m_dragonPartHead->updatePosition(0.0f, 2.0f, -8.0f, 1.0f, 1.0f);
    }

    // 更新颈部位置（跟随头部）
    if (m_dragonPartNeck) {
        Vector3 neckPos = getHistoryPos(5);
        m_dragonPartNeck->updatePosition(static_cast<f32>(neckPos.x - x()),
            static_cast<f32>(neckPos.y - y()) + 2.0f,
            static_cast<f32>(neckPos.z - z()) - 4.0f,
            3.0f,
            3.0f);
    }

    // 更新身体位置
    if (m_dragonPartBody) {
        m_dragonPartBody->updatePosition(0.0f, 0.0f, 0.0f, 8.0f, 4.0f);
    }

    // 更新尾部位置（跟随身体历史）
    if (m_dragonPartTail1) {
        Vector3 tail1Pos = getHistoryPos(10);
        m_dragonPartTail1->updatePosition(static_cast<f32>(tail1Pos.x - x()),
            static_cast<f32>(tail1Pos.y - y()),
            static_cast<f32>(tail1Pos.z - z()) + 4.0f,
            2.0f,
            2.0f);
    }

    if (m_dragonPartTail2) {
        Vector3 tail2Pos = getHistoryPos(15);
        m_dragonPartTail2->updatePosition(static_cast<f32>(tail2Pos.x - x()),
            static_cast<f32>(tail2Pos.y - y()),
            static_cast<f32>(tail2Pos.z - z()) + 6.0f,
            2.0f,
            2.0f);
    }

    if (m_dragonPartTail3) {
        Vector3 tail3Pos = getHistoryPos(20);
        m_dragonPartTail3->updatePosition(static_cast<f32>(tail3Pos.x - x()),
            static_cast<f32>(tail3Pos.y - y()),
            static_cast<f32>(tail3Pos.z - z()) + 8.0f,
            2.0f,
            2.0f);
    }

    // 更新翅膀位置
    f32 wingFlap = std::sin(m_animTime * 0.5f) * 2.0f; // 翅膀拍打动画

    if (m_dragonPartLeftWing) {
        m_dragonPartLeftWing->updatePosition(-4.0f, wingFlap + 1.0f, 0.0f, 4.0f, 2.0f);
    }

    if (m_dragonPartRightWing) {
        m_dragonPartRightWing->updatePosition(4.0f, wingFlap + 1.0f, 0.0f, 4.0f, 2.0f);
    }
}

void EnderDragonEntity::_updateDragonEnderCrystal()
{
    // 更新末影水晶链接
    // 寻找最近的末影水晶用于回血

    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 在范围内搜索末影水晶
    constexpr f32 CRYSTAL_SEARCH_RANGE = 32.0f;
    Vector3 pos(x(), y(), z());

    std::vector<Entity*> entities = worldPtr->getEntitiesInRange(pos, CRYSTAL_SEARCH_RANGE, this);

    EnderCrystalEntity* nearestCrystal = nullptr;
    f32 nearestDistSq = CRYSTAL_SEARCH_RANGE * CRYSTAL_SEARCH_RANGE;

    for (Entity* entity : entities) {
        if (entity && entity->entityType() == entity::VanillaEntityTypeKeys::END_CRYSTAL) {
            f32 dx = static_cast<f32>(entity->x() - x());
            f32 dy = static_cast<f32>(entity->y() - y());
            f32 dz = static_cast<f32>(entity->z() - z());
            f32 distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < nearestDistSq) {
                nearestDistSq = distSq;
                nearestCrystal = static_cast<EnderCrystalEntity*>(entity);
            }
        }
    }

    m_closestEnderCrystal = nearestCrystal;

    // 如果附近有水晶，每秒回复 1 点生命
    if (nearestCrystal && ticksExisted() % 20 == 0) {
        heal(1.0f);
    }
}

void EnderDragonEntity::_collideWithEntities()
{
    // 检测与其他实体的碰撞

    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 获取龙的所有部件碰撞箱
    for (EnderDragonPartEntity* part : m_dragonParts) {
        if (!part) continue;

        // 获取部件碰撞箱内的实体
        AxisAlignedBB partBox = part->boundingBox();
        std::vector<Entity*> entities = worldPtr->getEntitiesInAABB(partBox, this);

        // 对碰撞的实体造成伤害
        for (Entity* entity : entities) {
            if (entity && entity != this && entity->isAlive()) {
                // 只对玩家造成伤害
                if (entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
                    // 龙碰撞造成伤害
                    // 注意：实际伤害应该在 attackEntitiesInList 中处理
                    _attackEntitiesInList();
                    break;
                }
            }
        }
    }
}

void EnderDragonEntity::_attackEntitiesInList()
{
    // 攻击碰撞到的实体
    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 获取龙的碰撞箱
    AxisAlignedBB dragonBox = boundingBox();

    // 扩展碰撞箱以包含翅膀
    dragonBox = dragonBox.expand(1.0f, 0.5f, 1.0f);

    // 获取碰撞的实体
    std::vector<Entity*> entities = worldPtr->getEntitiesInAABB(dragonBox, this);

    for (Entity* entity : entities) {
        if (!entity || !entity->isAlive()) {
            continue;
        }

        // 只对玩家造成碰撞伤害
        if (entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
            // 龙碰撞对玩家造成伤害
            // MC 原版：头部和颈部碰撞造成 10.0F 伤害，翅膀碰撞造成 5.0F 伤害
            auto damageSource = DamageSources::mobAttack(this);
            entity->hurt(damageSource, 10.0f);
        }
    }

    // MC 原版：checkWalls(head) | checkWalls(neck) | checkWalls(body)
    // 对龙头、颈、身三个部件的碰撞箱调用 _destroyBlocksInAABB
    // 这同时完成了方块破坏和碰墙检测，不需要对 dragonBox 单独调用
    // 碰到 DRAGON_IMMUNE 或 mobGriefing 关闭时的方块会设置 m_slowed，
    // 导致龙的飞行速度降低（乘以 0.8）和翅膀扇动速度减半
    m_slowed = false;
    if (m_dragonPartHead) {
        m_slowed = _destroyBlocksInAABB(m_dragonPartHead->boundingBox()) || m_slowed;
    }
    if (m_dragonPartNeck) {
        m_slowed = _destroyBlocksInAABB(m_dragonPartNeck->boundingBox()) || m_slowed;
    }
    if (m_dragonPartBody) {
        m_slowed = _destroyBlocksInAABB(m_dragonPartBody->boundingBox()) || m_slowed;
    }
}

bool EnderDragonEntity::_destroyBlocksInAABB(const AxisAlignedBB& area)
{
    // 检查并破坏区域内的方块
    // 返回值：是否碰到了不可破坏的方块（DRAGON_IMMUNE 或 mobGriefing 关闭时的方块）
    // 此返回值用于设置龙的 m_slowed 标志（MC 原版 inWall）
    IWorld* worldPtr = world();
    if (!worldPtr) {
        return false;
    }

    // MC 原版：如果 mobGriefing 游戏规则关闭，龙不能破坏任何方块
    // 但仍然会检测是否碰到了 DRAGON_IMMUNE 方块（影响飞行行为）
    bool mobGriefing = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);

    // 计算方块坐标范围
    BlockCoord minX = static_cast<BlockCoord>(std::floor(area.minX));
    BlockCoord minY = static_cast<BlockCoord>(std::floor(area.minY));
    BlockCoord minZ = static_cast<BlockCoord>(std::floor(area.minZ));
    BlockCoord maxX = static_cast<BlockCoord>(std::floor(area.maxX));
    BlockCoord maxY = static_cast<BlockCoord>(std::floor(area.maxY));
    BlockCoord maxZ = static_cast<BlockCoord>(std::floor(area.maxZ));

    bool destroyedAny = false;
    bool hitWall = false;

    const BlockState* airState = &VanillaBlocks::AIR->defaultState();

    for (BlockCoord bx = minX; bx <= maxX; ++bx) {
        for (BlockCoord by = minY; by <= maxY; ++by) {
            for (BlockCoord bz = minZ; bz <= maxZ; ++bz) {
                BlockPos pos(bx, by, bz);
                const BlockState* state = worldPtr->getBlockState(pos);

                if (!state || state->isAir()) {
                    continue;
                }

                const Block& block = state->getBlock();

                // 龙透明方块：龙穿过时不破坏（如光照方块、火）
                if (BlockTags::DRAGON_TRANSPARENT().contains(block)) {
                    continue;
                }

                // 龙免疫方块：碰到后标记为碰墙，但不破坏
                // MC 原版：DRAGON_IMMUNE 包含基岩、黑曜石、末地石、末地传送门等
                if (BlockTags::DRAGON_IMMUNE().contains(block)) {
                    hitWall = true;
                    continue;
                }

                // mobGriefing 关闭时不能破坏方块，但仍标记碰墙
                if (!mobGriefing) {
                    hitWall = true;
                    continue;
                }

                // 破坏方块：使用 setBlockState 设为空气 + spawnAfterBreak 触发掉落
                // MC 原版使用 removeBlock(pos, false)，效果等价于 setBlockState(air, 3)
                worldPtr->setBlockState(pos, airState, 3);
                block.spawnAfterBreak(*worldPtr, pos, *state, nullptr, false);

                // 生成爆炸粒子效果
                worldPtr->addParticle(ParticleTypeId::Explosion,
                    Vector3(static_cast<f32>(bx) + 0.5f, static_cast<f32>(by) + 0.5f, static_cast<f32>(bz) + 0.5f),
                    Vector3(0.0f, 0.0f, 0.0f));

                destroyedAny = true;
            }
        }
    }

    // 如果有方块被破坏，在世界事件位置播放全局爆炸粒子
    // MC 原版：levelEvent(2008, randomPosInBounds, 0)
    if (destroyedAny) {
        math::Random rng(ticksExisted());
        Vector3 particlePos(static_cast<f32>(minX + rng.nextInt(maxX - minX + 1)) + 0.5f,
            static_cast<f32>(minY + rng.nextInt(maxY - minY + 1)) + 0.5f,
            static_cast<f32>(minZ + rng.nextInt(maxZ - minZ + 1)) + 0.5f);
        worldPtr->addParticle(ParticleTypeId::HugeExplosion, particlePos, Vector3(1.0f, 0.0f, 0.0f));
    }

    // 返回是否碰到了不可破坏的方块（用于设置 m_slowed，影响龙的飞行行为）
    // MC 原版：inWall = checkWalls(head) | checkWalls(neck) | checkWalls(body)
    return hitWall;
}

void EnderDragonEntity::_onDeathUpdate()
{
    // 对齐 MC 1.21.11 EnderDragon.tickDeath()
    IWorld* worldPtr = world();

    // MC: if (this.dragonFight != null) { this.dragonFight.updateDragon(this); }
    // 死亡动画期间持续同步 Boss 栏血量（龙血量为 0，Boss 栏显示 0%）和名称
    if (worldPtr != nullptr) {
        EndDragonFight* fight = worldPtr->dragonFight();
        if (fight != nullptr) {
            fight->updateDragon(*this);
        }
    }

    m_deathTicks++;

    // 死亡爆炸粒子：在 180-200 tick 之间生成
    // MC: if (this.dragonDeathTime >= 180 && this.dragonDeathTime <= 200) { ... }
    if (m_deathTicks >= 180 && m_deathTicks <= DEATH_DURATION) {
        // MC: float f = (this.random.nextFloat() - 0.5F) * 8.0F;
        //     float f1 = (this.random.nextFloat() - 0.5F) * 4.0F;
        //     float f2 = (this.random.nextFloat() - 0.5F) * 8.0F;
        //     this.level().addParticle(ParticleTypes.EXPLOSION_EMITTER,
        //         this.getX() + f, this.getY() + 2.0 + f1, this.getZ() + f2, 0.0, 0.0, 0.0);
        math::Random& rng = getRandom();
        const f32 offsetX = (rng.nextFloat() - 0.5f) * 8.0f;
        const f32 offsetY = (rng.nextFloat() - 0.5f) * 4.0f;
        const f32 offsetZ = (rng.nextFloat() - 0.5f) * 8.0f;
        const Vector3 particlePos(x() + offsetX, y() + 2.0f + offsetY, z() + offsetZ);
        if (worldPtr != nullptr) {
            // ParticleTypeId::HugeExplosion 对应 MC 的 ParticleTypes.EXPLOSION_EMITTER
            worldPtr->addParticle(ParticleTypeId::HugeExplosion, particlePos, Vector3(0.0f, 0.0f, 0.0f));
        }
    }

    // 经验掉落总量：首次击杀 12000，后续 500
    // MC: int i = 500; if (dragonFight != null && !hasPreviouslyKilledDragon()) i = 12000;
    const bool previouslyKilled = (worldPtr != nullptr && worldPtr->dragonFight() != nullptr)
        ? worldPtr->dragonFight()->hasPreviouslyKilled()
        : false;
    const i32 totalXP = previouslyKilled ? XP_SUBSEQUENT : XP_FIRST_KILL;

    // 阶段性经验掉落：150 tick 之后，每 5 tick 掉落 8% 经验（需要 doMobLoot 规则）
    // MC: if (dragonDeathTime > 150 && dragonDeathTime % 5 == 0 && MOB_DROPS) {
    //         ExperienceOrb.award(serverlevel, this.position(), Mth.floor(i * 0.08F));
    //     }
    if (worldPtr != nullptr && m_deathTicks > 150 && m_deathTicks % 5 == 0) {
        const auto& gameRules = worldPtr->getGameRules();
        if (gameRules.getBoolean(world::gamerule::GameRuleKeys::DO_MOB_LOOT)) {
            const i32 xpDropAmount = static_cast<i32>(std::floor(static_cast<f32>(totalXP) * 0.08f));
            _dropExperienceAmount(xpDropAmount);
        }
    }

    // 死亡音效：在 1 tick 时触发（需要非静默）
    // MC: if (dragonDeathTime == 1 && !isSilent()) {
    //         serverlevel.globalLevelEvent(1028, this.blockPosition(), 0);
    //     }
    if (worldPtr != nullptr && m_deathTicks == 1 && !isSilent()) {
        // Cubium 没有 globalLevelEvent，使用 playEvent 广播世界事件 1028（DRAGON_DEATH_SOUND）
        // 给附近的客户端；同时通过 playSound 显式播放末影龙死亡音效以确保声音实际触发。
        const BlockPos blockPos(
            static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));
        worldPtr->playEvent(world::WorldEvents::DRAGON_DEATH_SOUND, blockPos, 0);
        // 龙死亡音效音量较大，使其能在更远距离听到（近似 MC 的全局广播）
        worldPtr->playSound(
            SoundEvents::ENTITY_ENDER_DRAGON_DEATH, sound::SoundCategory::Hostile, position(), 5.0f, 1.0f);
    }

    // 死亡动画期间缓慢上升
    // MC: Vec3 vec3 = new Vec3(0.0, 0.1F, 0.0);
    //     this.move(MoverType.SELF, vec3);
    const Vector3 riseVelocity(0.0f, 0.1f, 0.0f);
    move(entity::MoverType::Self, riseVelocity);

    // 同步子部件位置：每个部件应用相同的位移
    // MC: for (EnderDragonPart enderdragonpart : this.subEntities) {
    //         enderdragonpart.setOldPosAndRot();
    //         enderdragonpart.setPos(enderdragonpart.position().add(vec3));
    //     }
    // Cubium 的 Entity::setPosition() 内部会将 m_builtIn.stateVector->m_posPrev 更新为当前位置，
    // 再设置新位置，等价于 MC 的 setOldPosAndRot() + setPos() 组合。
    for (EnderDragonPartEntity* part : m_dragonParts) {
        if (part == nullptr) {
            continue;
        }
        const Vector3 partNewPos(part->x() + riseVelocity.x, part->y() + riseVelocity.y, part->z() + riseVelocity.z);
        part->setPosition(partNewPos);
    }

    // 死亡完成：200 tick 时掉落剩余 20% 经验、生成出口传送门、移除实体、触发 ENTITY_DIE 事件
    // MC: if (dragonDeathTime == 200 && level instanceof ServerLevel) {
    //         if (MOB_DROPS) ExperienceOrb.award(serverlevel, this.position(), Mth.floor(i * 0.2F));
    //         if (dragonFight != null) dragonFight.setDragonKilled(this);
    //         this.remove(Entity.RemovalReason.KILLED);
    //         this.gameEvent(GameEvent.ENTITY_DIE);
    //     }
    if (m_deathTicks == DEATH_DURATION) {
        if (worldPtr != nullptr) {
            const auto& gameRules = worldPtr->getGameRules();
            if (gameRules.getBoolean(world::gamerule::GameRuleKeys::DO_MOB_LOOT)) {
                const i32 xpFinalDrop = static_cast<i32>(std::floor(static_cast<f32>(totalXP) * 0.2f));
                _dropExperienceAmount(xpFinalDrop);
            }

            // 通过 EndDragonFight 统一处理龙蛋放置、折跃门生成和出口传送门
            EndDragonFight* fight = worldPtr->dragonFight();
            if (fight != nullptr) {
                fight->setDragonKilled(*worldPtr);
            } else {
                // 没有 EndDragonFight 时回退到仅创建出口传送门
                // （例如调试用的非末地维度）
                EndTeleporter::createExitPortal(*worldPtr, BlockPos(0, 0, 0), true);
            }
        }

        // 移除实体（Cubium 的 remove() 是无参数版本，对应 MC 的 remove(KILLED)）
        remove();

        // 触发 ENTITY_DIE 游戏事件，通知附近的幽匿感测体
        // MC: this.gameEvent(GameEvent.ENTITY_DIE);
        if (worldPtr != nullptr) {
            const BlockPos eventPos(static_cast<i32>(std::floor(x())),
                static_cast<i32>(std::floor(y())),
                static_cast<i32>(std::floor(z())));
            worldPtr->gameEvent(gameevent::GameEvents::ENTITY_DIE, eventPos, gameevent::GameEvent::Context::of(this));
        }
    }
}

void EnderDragonEntity::dropExperience()
{
    // 末影龙不调用父类的 dropExperience
    // 它使用自定义的经验掉落逻辑，在死亡动画结束时调用
}

void EnderDragonEntity::_dropExperienceAmount(i32 amount)
{
    // 使用 ExperienceDropHandler 生成经验球
    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 使用 ExperienceDropHandler 生成经验球
    ExperienceDropHandler::spawnExperienceOrbs(worldPtr, x(), y(), z(), amount);
}

} // namespace entity
} // namespace mc
