#include "EnderDragonEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../../core/Constants.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc {
namespace entity {

using namespace mc::math;

// ============================================================================
// BossEntity
// ============================================================================

BossEntity::BossEntity(LegacyEntityType type, EntityId id)
    : MobEntity(type, id)
{
    // Boss 级实体默认显示生命条
}

// ============================================================================
// EnderDragonPartEntity
// ============================================================================

EnderDragonPartEntity::EnderDragonPartEntity(LegacyEntityType type, EntityId id)
    : Entity(type, id)
{
    // 部件尺寸较小，用于碰撞检测
}

void EnderDragonPartEntity::tick() {
    Entity::tick();
    // 部件位置由父龙的 updateDragonParts() 更新
}

void EnderDragonPartEntity::updatePosition(f32 offsetX, f32 offsetY, f32 offsetZ, f32 width, f32 height) {
    if (!m_parent) {
        return;
    }

    // MC 1.16.5: 根据父龙的旋转计算实际位置
    // 龙的 yaw 需要转换为弧度
    f32 yawRad = m_parent->yaw() * (PI / 180.0f);
    f32 sinYaw = std::sin(yawRad);
    f32 cosYaw = std::cos(yawRad);

    // 旋转变换
    f32 rotatedX = offsetX * cosYaw - offsetZ * sinYaw;
    f32 rotatedZ = offsetX * sinYaw + offsetZ * cosYaw;

    // 设置位置
    setPosition(
        m_parent->x() + rotatedX,
        m_parent->y() + offsetY,
        m_parent->z() + rotatedZ
    );

    // 更新碰撞箱尺寸
    // Note: 需要调用 refreshDimensions() 来应用新的尺寸
    // 这里暂时只更新位置
    MC_UNUSED(width);
    MC_UNUSED(height);
}

// ============================================================================
// EnderDragonEntity
// ============================================================================

std::unique_ptr<Entity> EnderDragonEntity::create(IWorld* /*world*/) {
    return std::make_unique<EnderDragonEntity>(LegacyEntityType::EnderDragon, EntityId(0));
}

EnderDragonEntity::EnderDragonEntity(LegacyEntityType type, EntityId id)
    : BossEntity(type, id)
{
    // MC 1.16.5: 初始化龙部件
    initDragonParts();

    // 注册属性
    registerAttributes();
    registerGoals();

    // 初始化路径点
    initPathPoints();
}

void EnderDragonEntity::initDragonParts() {
    // MC 1.16.5: 创建所有龙部件
    // 部件列表顺序：头、颈、身、尾1、尾2、尾3、左翼、右翼

    // 头部
    m_dragonPartHead = new EnderDragonPartEntity(LegacyEntityType::EnderDragonPart, EntityId(0));
    m_dragonPartHead->setPart(EnderDragonPartEntity::Part::Head);
    m_dragonPartHead->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartHead);

    // 颈部
    m_dragonPartNeck = new EnderDragonPartEntity(LegacyEntityType::EnderDragonPart, EntityId(0));
    m_dragonPartNeck->setPart(EnderDragonPartEntity::Part::Neck);
    m_dragonPartNeck->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartNeck);

    // 身体
    m_dragonPartBody = new EnderDragonPartEntity(LegacyEntityType::EnderDragonPart, EntityId(0));
    m_dragonPartBody->setPart(EnderDragonPartEntity::Part::Body);
    m_dragonPartBody->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartBody);

    // 尾部1
    m_dragonPartTail1 = new EnderDragonPartEntity(LegacyEntityType::EnderDragonPart, EntityId(0));
    m_dragonPartTail1->setPart(EnderDragonPartEntity::Part::Tail1);
    m_dragonPartTail1->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartTail1);

    // 尾部2
    m_dragonPartTail2 = new EnderDragonPartEntity(LegacyEntityType::EnderDragonPart, EntityId(0));
    m_dragonPartTail2->setPart(EnderDragonPartEntity::Part::Tail2);
    m_dragonPartTail2->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartTail2);

    // 尾部3
    m_dragonPartTail3 = new EnderDragonPartEntity(LegacyEntityType::EnderDragonPart, EntityId(0));
    m_dragonPartTail3->setPart(EnderDragonPartEntity::Part::Tail3);
    m_dragonPartTail3->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartTail3);

    // 左翼
    m_dragonPartLeftWing = new EnderDragonPartEntity(LegacyEntityType::EnderDragonPart, EntityId(0));
    m_dragonPartLeftWing->setPart(EnderDragonPartEntity::Part::WingLeft);
    m_dragonPartLeftWing->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartLeftWing);

    // 右翼
    m_dragonPartRightWing = new EnderDragonPartEntity(LegacyEntityType::EnderDragonPart, EntityId(0));
    m_dragonPartRightWing->setPart(EnderDragonPartEntity::Part::WingRight);
    m_dragonPartRightWing->setParentDragon(this);
    m_dragonParts.push_back(m_dragonPartRightWing);
}

std::optional<ResourceLocation> EnderDragonEntity::getAmbientSound() const {
    // MC 1.16.5: entity.ender_dragon.ambient
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> EnderDragonEntity::getHurtSound(DamageSource& /*source*/) const {
    // MC 1.16.5: entity.ender_dragon.hurt
    return makeSoundEventId("hurt");
}

void EnderDragonEntity::tick() {
    // MC 1.16.5 EnderDragonEntity.tick()

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
    updateDragonParts();

    // 更新末影水晶
    updateDragonEnderCrystal();

    // 处理碰撞
    collideWithEntities();

    // 死亡处理
    if (isDying()) {
        onDeathUpdate();
    }
}

void EnderDragonEntity::setPhase(Phase phase) {
    // MC 1.16.5: 切换阶段
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

bool EnderDragonEntity::attackEntityPartFrom(EnderDragonPartEntity* part, DamageSource& source, f32 damage) {
    // MC 1.16.5: attackEntityPartFrom()
    // 所有部件的伤害都传递给龙本体
    MC_UNUSED(part);
    MC_UNUSED(source);
    MC_UNUSED(damage);

    // TODO: 实现伤害处理
    // 1. 如果是头部，正常受伤
    // 2. 如果是其他部位，可能减少伤害
    // 3. 检查伤害来源类型

    return false;
}

void EnderDragonEntity::onCrystalDestroyed(EnderCrystalEntity* crystal, const BlockPos& pos, DamageSource& source) {
    // MC 1.16.5: onCrystalDestroyed()
    // 末影水晶被破坏时，龙会受到伤害
    MC_UNUSED(crystal);
    MC_UNUSED(pos);
    MC_UNUSED(source);

    // TODO: 实现末影水晶破坏处理
    // 1. 如果水晶在回血龙附近，对龙造成伤害
    // 2. 播放爆炸效果
}

void EnderDragonEntity::initPathPoints() {
    // MC 1.16.5: initPathPoints()
    // 初始化末影龙飞行路径点
    // 围绕末地中心的8个路径点

    m_pathPoints.clear();

    for (i32 i = 0; i < 8; ++i) {
        f32 angle = static_cast<f32>(i) * (PI * 2.0f / 8.0f);
        m_pathPoints.emplace_back(
            static_cast<BlockCoord>(std::cos(angle) * 64.0),
            64,
            static_cast<BlockCoord>(std::sin(angle) * 64.0)
        );
    }

    m_currentPathPoint = 0;
}

i32 EnderDragonEntity::getNearestPathPointIndex(f64 x, f64 y, f64 z) const {
    // MC 1.16.5: 获取最近的路径点索引
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

void EnderDragonEntity::registerGoals() {
    BossEntity::registerGoals();

    // MC 1.16.5: 末影龙使用特殊的阶段系统，不使用普通AI目标
    // 阶段管理在 PhaseManager 中处理
}

void EnderDragonEntity::registerAttributes() {
    BossEntity::registerAttributes();

    // MC 1.16.5 EnderDragonEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 200.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 256.0);
}

void EnderDragonEntity::updateDragonParts() {
    // MC 1.16.5: 更新龙部件位置
    // 使用环形缓冲区记录位置历史，用于颈部和尾部的平滑动画

    // 更新环形缓冲区
    m_ringBufferIndex = (m_ringBufferIndex + 1) % RING_BUFFER_SIZE;
    m_ringBuffer[m_ringBufferIndex][0] = x();
    m_ringBuffer[m_ringBufferIndex][1] = y();
    m_ringBuffer[m_ringBufferIndex][2] = z();

    // 获取历史位置
    auto getHistoryPos = [this](i32 offset) -> Vector3 {
        i32 index = (m_ringBufferIndex - offset + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
        return Vector3(
            m_ringBuffer[index][0],
            m_ringBuffer[index][1],
            m_ringBuffer[index][2]
        );
    };

    // 更新头部位置（在龙前方）
    if (m_dragonPartHead) {
        m_dragonPartHead->updatePosition(0.0f, 2.0f, -8.0f, 1.0f, 1.0f);
    }

    // 更新颈部位置（跟随头部）
    if (m_dragonPartNeck) {
        Vector3 neckPos = getHistoryPos(5);
        m_dragonPartNeck->updatePosition(
            static_cast<f32>(neckPos.x - x()),
            static_cast<f32>(neckPos.y - y()) + 2.0f,
            static_cast<f32>(neckPos.z - z()) - 4.0f,
            3.0f, 3.0f
        );
    }

    // 更新身体位置
    if (m_dragonPartBody) {
        m_dragonPartBody->updatePosition(0.0f, 0.0f, 0.0f, 8.0f, 4.0f);
    }

    // 更新尾部位置（跟随身体历史）
    if (m_dragonPartTail1) {
        Vector3 tail1Pos = getHistoryPos(10);
        m_dragonPartTail1->updatePosition(
            static_cast<f32>(tail1Pos.x - x()),
            static_cast<f32>(tail1Pos.y - y()),
            static_cast<f32>(tail1Pos.z - z()) + 4.0f,
            2.0f, 2.0f
        );
    }

    if (m_dragonPartTail2) {
        Vector3 tail2Pos = getHistoryPos(15);
        m_dragonPartTail2->updatePosition(
            static_cast<f32>(tail2Pos.x - x()),
            static_cast<f32>(tail2Pos.y - y()),
            static_cast<f32>(tail2Pos.z - z()) + 6.0f,
            2.0f, 2.0f
        );
    }

    if (m_dragonPartTail3) {
        Vector3 tail3Pos = getHistoryPos(20);
        m_dragonPartTail3->updatePosition(
            static_cast<f32>(tail3Pos.x - x()),
            static_cast<f32>(tail3Pos.y - y()),
            static_cast<f32>(tail3Pos.z - z()) + 8.0f,
            2.0f, 2.0f
        );
    }

    // 更新翅膀位置
    f32 wingFlap = std::sin(m_animTime * 0.5f) * 2.0f;  // 翅膀拍打动画

    if (m_dragonPartLeftWing) {
        m_dragonPartLeftWing->updatePosition(-4.0f, wingFlap + 1.0f, 0.0f, 4.0f, 2.0f);
    }

    if (m_dragonPartRightWing) {
        m_dragonPartRightWing->updatePosition(4.0f, wingFlap + 1.0f, 0.0f, 4.0f, 2.0f);
    }
}

void EnderDragonEntity::updateDragonEnderCrystal() {
    // MC 1.16.5: 更新末影水晶链接
    // 寻找最近的末影水晶用于回血

    // TODO: 实现末影水晶查找和链接
    // 1. 在范围内搜索末影水晶
    // 2. 设置 m_closestEnderCrystal
    // 3. 播放连接光束效果
}

void EnderDragonEntity::collideWithEntities() {
    // MC 1.16.5: collideWithEntities()
    // 检测与其他实体的碰撞

    // TODO: 实现碰撞检测
    // 1. 获取附近实体列表
    // 2. 对每个实体调用 attackEntitiesInList()
}

void EnderDragonEntity::attackEntitiesInList() {
    // MC 1.16.5: attackEntitiesInList()
    // 攻击碰撞到的实体

    // TODO: 实现攻击逻辑
    // 1. 对玩家造成伤害
    // 2. 破坏方块
}

bool EnderDragonEntity::destroyBlocksInAABB(const AxisAlignedBB& area) {
    // MC 1.16.5: destroyBlocksInAABB()
    // 破坏区域内的方块
    MC_UNUSED(area);

    // TODO: 实现方块破坏
    // 1. 检查区域内所有方块
    // 2. 跳过不可破坏的方块（基岩、黑曜石等）
    // 3. 破坏其他方块并生成粒子

    return false;
}

void EnderDragonEntity::onDeathUpdate() {
    // MC 1.16.5: onDeathUpdate()
    m_deathTicks++;

    // 死亡动画
    // 前100 ticks 上升并发光
    // 后100 ticks 爆炸并消失

    if (m_deathTicks < DEATH_DURATION) {
        // 死亡动画期间
        // 缓慢上升
        setVelocity(0.0f, 0.1f, 0.0f);

        // 每隔一段时间生成爆炸效果
        if (m_deathTicks % 10 == 0) {
            // TODO: 生成爆炸粒子
            // world->addParticle(...)
        }
    }

    if (m_deathTicks >= DEATH_DURATION) {
        // 死亡完成
        // 掉落经验
        dropExperience(XP_FIRST_KILL);

        // TODO: 生成传送门和龙蛋

        // 移除实体
        remove();
    }
}

void EnderDragonEntity::dropExperience(i32 amount) {
    // MC 1.16.5: dropExperience()
    // 掉落经验球
    MC_UNUSED(amount);

    // TODO: 生成经验球实体
    // 首次击杀: 12000 XP
    // 后续击杀: 500 XP
}

} // namespace entity
} // namespace mc
