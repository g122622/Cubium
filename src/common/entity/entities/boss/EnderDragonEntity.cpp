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
#include "common/core/Constants.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/experience/ExperienceDropHandler.hpp"
#include "common/util/math/AxisAlignedBB.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cmath>
#include <limits>

namespace mc {
namespace entity {

using namespace mc::math;

// ============================================================================
// BossEntity
// ============================================================================

BossEntity::BossEntity(EntityId id)
    : MobEntity(id)
{
    // Boss 级实体默认显示生命条
}

// ============================================================================
// EnderDragonPartEntity
// ============================================================================

EnderDragonPartEntity::EnderDragonPartEntity(EntityId id)
    : Entity(id)
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

std::unique_ptr<Entity> EnderDragonEntity::create(IWorld* /*world*/)
{
    return std::make_unique<EnderDragonEntity>(EntityId(0));
}

EnderDragonEntity::EnderDragonEntity(EntityId id)
    : BossEntity(id)
{
    // 初始化龙部件
    initDragonParts();

    // 注册属性
    registerAttributes();
    registerGoals();

    // 初始化路径点
    initPathPoints();
}

void EnderDragonEntity::initDragonParts()
{
    // 创建所有龙部件
    // 部件列表顺序：头、颈、身、尾1、尾2、尾3、左翼、右翼

    // 头部
    m_dragonPartHead = new EnderDragonPartEntity(EntityId(0));
    m_dragonPartHead->setPart(EnderDragonPartEntity::Part::Head);
    m_dragonPartHead->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartHead);

    // 颈部
    m_dragonPartNeck = new EnderDragonPartEntity(EntityId(0));
    m_dragonPartNeck->setPart(EnderDragonPartEntity::Part::Neck);
    m_dragonPartNeck->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartNeck);

    // 身体
    m_dragonPartBody = new EnderDragonPartEntity(EntityId(0));
    m_dragonPartBody->setPart(EnderDragonPartEntity::Part::Body);
    m_dragonPartBody->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartBody);

    // 尾部1
    m_dragonPartTail1 = new EnderDragonPartEntity(EntityId(0));
    m_dragonPartTail1->setPart(EnderDragonPartEntity::Part::Tail1);
    m_dragonPartTail1->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartTail1);

    // 尾部2
    m_dragonPartTail2 = new EnderDragonPartEntity(EntityId(0));
    m_dragonPartTail2->setPart(EnderDragonPartEntity::Part::Tail2);
    m_dragonPartTail2->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartTail2);

    // 尾部3
    m_dragonPartTail3 = new EnderDragonPartEntity(EntityId(0));
    m_dragonPartTail3->setPart(EnderDragonPartEntity::Part::Tail3);
    m_dragonPartTail3->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartTail3);

    // 左翼
    m_dragonPartLeftWing = new EnderDragonPartEntity(EntityId(0));
    m_dragonPartLeftWing->setPart(EnderDragonPartEntity::Part::WingLeft);
    m_dragonPartLeftWing->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartLeftWing);

    // 右翼
    m_dragonPartRightWing = new EnderDragonPartEntity(EntityId(0));
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
    m_prevAnimTime = m_animTime;
    if (isDying()) {
        // 死亡时动画加速
        m_animTime += 0.02f;
    } else {
        m_animTime += 0.01f;
    }

    // 调用父类 tick
    BossEntity::tick();

    // 更新龙部件位置
    _updateDragonParts();

    // 更新末影水晶
    _updateDragonEnderCrystal();

    // 处理碰撞
    _collideWithEntities();

    // 死亡处理
    if (isDying()) {
        _onDeathUpdate();
    }
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
    if (isDead() || isInvulnerableTo(source)) {
        return false;
    }

    // 头部和身体受到正常伤害，其他部位伤害减半
    f32 actualDamage = damage;
    if (part) {
        EnderDragonPartEntity::Part partType = part->part();
        if (partType != EnderDragonPartEntity::Part::Head && partType != EnderDragonPartEntity::Part::Body) {
            // 尾部和翅膀受到的伤害减半
            actualDamage = damage * 0.5f;
        }
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
        // 清除回血目标
        if (m_closestEnderCrystal == crystal) {
            m_closestEnderCrystal = nullptr;
        }

        // 对龙造成伤害
        // 水晶被破坏时造成 10 点伤害
        constexpr f32 CRYSTAL_DESTRUCTION_DAMAGE = 10.0f;

        // 创建伤害源（使用通用伤害，因为水晶爆炸）
        // TODO: 实际应该使用 DamageSources::explosion()
        hurt(source, CRYSTAL_DESTRUCTION_DAMAGE);
    }

    MC_UNUSED(pos);
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
        if (entity && entity->typeId() == entity::EntityTypeIdNumber::END_CRYSTAL) {
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
        AxisAlignedBB partBox = part->getBoundingBox();
        std::vector<Entity*> entities = worldPtr->getEntitiesInAABB(partBox, this);

        // 对碰撞的实体造成伤害
        for (Entity* entity : entities) {
            if (entity && entity != this && !entity->isDead()) {
                // 只对玩家造成伤害
                if (entity->typeId() == entity::EntityTypeIdNumber::PLAYER) {
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

    // 破坏龙路径上的方块
    // 注意：方块破坏应该在 collideWithEntities 中获取碰撞实体后调用
    // 这里只处理实体伤害

    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 获取龙的碰撞箱
    AxisAlignedBB dragonBox = getBoundingBox();

    // 扩展碰撞箱以包含翅膀
    dragonBox = dragonBox.grow(1.0f, 0.5f, 1.0f);

    // 获取碰撞的实体
    std::vector<Entity*> entities = worldPtr->getEntitiesInAABB(dragonBox, this);

    for (Entity* entity : entities) {
        if (entity && !entity->isDead()) {
            // 对玩家造成伤害
            if (entity->typeId() == entity::EntityTypeIdNumber::PLAYER) {
                // TODO: 龙冲撞造成伤害，实际应该使用 DamageSources::mobAttack(this)
                // entity->hurt(DamageSources::mobAttack(this), 10.0f);
            }
        }
    }

    // 破坏方块
    _destroyBlocksInAABB(dragonBox);
}

bool EnderDragonEntity::_destroyBlocksInAABB(const AxisAlignedBB& area)
{
    // 破坏区域内的方块
    IWorld* worldPtr = world();
    if (!worldPtr) {
        return false;
    }

    // 龙可以破坏大多数方块，但不能破坏以下类型：
    // - 基岩
    // - 黑曜石
    // - 末地石（部分）
    // - 屏障方块
    // - 结构方块

    // 计算方块坐标范围
    BlockCoord minX = static_cast<BlockCoord>(std::floor(area.minX()));
    BlockCoord minY = static_cast<BlockCoord>(std::floor(area.minY()));
    BlockCoord minZ = static_cast<BlockCoord>(std::floor(area.minZ()));
    BlockCoord maxX = static_cast<BlockCoord>(std::floor(area.maxX()));
    BlockCoord maxY = static_cast<BlockCoord>(std::floor(area.maxY()));
    BlockCoord maxZ = static_cast<BlockCoord>(std::floor(area.maxZ()));

    bool destroyedAny = false;

    for (BlockCoord bx = minX; bx <= maxX; ++bx) {
        for (BlockCoord by = minY; by <= maxY; ++by) {
            for (BlockCoord bz = minZ; bz <= maxZ; ++bz) {
                BlockPos pos(bx, by, bz);
                const BlockState* state = worldPtr->getBlockState(pos);

                if (!state || state->isAir()) {
                    continue;
                }

                // 检查方块是否可以被龙破坏
                const Block& block = state->getBlock();

                // 跳过不可破坏的方块
                // 龙不能破坏基岩、黑曜石、末地传送门等
                if (block.is(VanillaBlocks::BEDROCK) || block.is(VanillaBlocks::OBSIDIAN) ||
                    block.is(VanillaBlocks::CRYING_OBSIDIAN) || block.is(VanillaBlocks::END_PORTAL) ||
                    block.is(VanillaBlocks::END_PORTAL_FRAME)) {
                    continue;
                }

                // 末地石有一定概率不被破坏
                if (block.is(VanillaBlocks::END_STONE)) {
                    // 末地石有 50% 概率被破坏
                    math::Random rng(ticksExisted() + bx + by * 31 + bz * 961);
                    if (rng.nextFloat() > 0.5f) {
                        continue;
                    }
                }

                // 破坏方块
                // TODO: 实际应该调用 world->destroyBlock(pos, false) 来触发掉落和粒子
                // 这里简化为直接设置为空气，但需要调用 spawnAfterBreak 以支持虫蚀方块等特殊行为
                const BlockState* oldState = state;
                worldPtr->setBlockState(bx, by, bz, nullptr);
                block.spawnAfterBreak(*worldPtr, BlockPos(bx, by, bz), *oldState, nullptr, false);

                // TODO: 生成粒子效果
                // worldPtr->addParticle(ParticleTypeId::EXPLOSION, Vector3(bx + 0.5, by + 0.5, bz + 0.5), Vector3(0, 0,
                // 0));

                destroyedAny = true;
            }
        }
    }

    return destroyedAny;
}

void EnderDragonEntity::_onDeathUpdate()
{
    m_deathTicks++;

    // 死亡动画
    // 前100 ticks 上升并发光
    // 后100 ticks 爆炸并消失

    if (m_deathTicks < DEATH_DURATION) {
        // 死亡动画期间
        // 缓慢上升
        setVelocity(0.0f, 0.1f, 0.0f);

        // 每隔一段时间生成爆炸粒子
        if (m_deathTicks % 10 == 0) {
            IWorld* worldPtr = world();
            if (worldPtr) {
                // 生成爆炸粒子
                math::Random rng(m_deathTicks);
                f32 px = static_cast<f32>(x() + rng.nextFloat(-5.0f, 5.0f));
                f32 py = static_cast<f32>(y() + rng.nextFloat(-2.0f, 5.0f));
                f32 pz = static_cast<f32>(z() + rng.nextFloat(-5.0f, 5.0f));
                // worldPtr->addParticle(ParticleTypeId::EXPLOSION_EMITTER, Vector3(px, py, pz), Vector3(0, 0, 0));
                MC_UNUSED(px);
                MC_UNUSED(py);
                MC_UNUSED(pz);
            }
        }
    }

    if (m_deathTicks >= DEATH_DURATION) {
        // 死亡完成
        // 掉落经验
        _dropExperienceAmount(XP_FIRST_KILL);

        // TODO: 生成传送门和龙蛋
        // 这需要访问世界生成系统来放置传送门结构

        // 移除实体
        remove();
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
