/**
 * @file PhantomGoals.cpp
 * @brief 幻翼专用的AI目标类实现
 *
 * 参考 MC 1.16.5: net.minecraft.entity.monster.PhantomEntity
 */

#include "PhantomGoals.hpp"
#include "../../../../../util/assert/AssertMacros.hpp"
#include "../../../../../util/math/MathConstants.hpp"
#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../sound/SoundEvents.hpp"
#include "../../../../entities/monster/basic/PhantomEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../../core/EntityUtils.hpp"
#include "../../../../../core/Constants.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/Block.hpp"
#include "../../../../../world/block/BlockState.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

// ============================================================================
// PhantomAttackPlayerTargetGoal
// ============================================================================

PhantomAttackPlayerTargetGoal::PhantomAttackPlayerTargetGoal(PhantomEntity* phantom)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Target})
    , m_phantom(phantom)
    , m_tickDelay(20)  // MC 1.16.5: 初始延迟20tick
{
    MC_ASSERT_RELEASE(phantom != nullptr);
}

bool PhantomAttackPlayerTargetGoal::shouldExecute()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return false;
    }

    // MC 1.16.5: 每60tick搜索一次玩家（初始20tick后）
    if (m_tickDelay > 0) {
        --m_tickDelay;
        return false;
    }

    Player* player = findAttackablePlayer();
    if (player != nullptr) {
        m_phantom->setAttackTarget(player);
        m_tickDelay = 60;  // MC 1.16.5: 成功后60tick延迟
        return true;
    }

    m_tickDelay = 60;
    return false;
}

bool PhantomAttackPlayerTargetGoal::shouldContinueExecuting()
{
    if (m_phantom == nullptr) {
        return false;
    }

    LivingEntity* target = m_phantom->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    // MC 1.16.5: 检查是否仍可攻击目标
    // 需要验证：目标是否是玩家、是否在旁观/创造模式
    auto* player = dynamic_cast<Player*>(target);
    if (player == nullptr) {
        return false;
    }

    // 不能攻击旁观者或创造模式玩家
    if (player->isSpectator() || player->isCreative()) {
        return false;
    }

    // 检查距离
    f64 distanceSq = m_phantom->position().distanceSquared(target->position());
    f64 followRange = m_phantom->getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 64.0);
    return distanceSq <= followRange * followRange;
}

void PhantomAttackPlayerTargetGoal::resetTask()
{
    if (m_phantom != nullptr) {
        m_phantom->setAttackTarget(nullptr);
    }
}

Player* PhantomAttackPlayerTargetGoal::findAttackablePlayer()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return nullptr;
    }

    IWorld* world = m_phantom->world();
    math::Vector3 pos = m_phantom->position();

    // MC 1.16.5: 搜索范围 16x64x16
    // 使用 EntityUtils 搜索玩家
    Player* nearestPlayer = EntityUtils::findClosestEntity<Player>(
        world,
        pos,
        static_cast<f32>(SEARCH_RANGE),
        m_phantom,
        [this](Player* player) -> bool {
            if (player == nullptr || !player->isAlive()) {
                return false;
            }
            // 不能攻击旁观者或创造模式玩家
            if (player->isSpectator() || player->isCreative()) {
                return false;
            }
            // MC 1.16.5: 玩家必须在海平面以上
            if (player->position().y < static_cast<f64>(world::SEA_LEVEL)) {
                return false;
            }
            return true;
        });

    return nearestPlayer;
}

// ============================================================================
// PhantomMoveGoal
// ============================================================================

PhantomMoveGoal::PhantomMoveGoal(PhantomEntity* phantom)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_phantom(phantom)
{
    MC_ASSERT_RELEASE(phantom != nullptr);
}

bool PhantomMoveGoal::isNearOrbitOffset() const
{
    if (m_phantom == nullptr) {
        return false;
    }

    // MC 1.16.5: func_203146_f
    // 检查距离平方是否小于 4.0
    math::Vector3 offset = m_phantom->orbitOffset();
    math::Vector3 pos = m_phantom->position();
    f64 dx = offset.x - pos.x;
    f64 dy = offset.y - pos.y;
    f64 dz = offset.z - pos.z;
    return (dx * dx + dy * dy + dz * dz) < 4.0;
}

// ============================================================================
// PhantomOrbitPointGoal
// ============================================================================

PhantomOrbitPointGoal::PhantomOrbitPointGoal(PhantomEntity* phantom)
    : PhantomMoveGoal(phantom)
    , m_orbitAngle(0.0f)
    , m_orbitRadius(5.0f)
    , m_orbitHeightOffset(0.0f)
    , m_orbitDirection(1.0f)
{
}

bool PhantomOrbitPointGoal::shouldExecute()
{
    if (m_phantom == nullptr) {
        return false;
    }

    // MC 1.16.5: 无攻击目标或处于环绕阶段时执行
    LivingEntity* target = m_phantom->attackTarget();
    return target == nullptr || m_phantom->getAttackPhase() == PhantomEntity::AttackPhase::CIRCLE;
}

void PhantomOrbitPointGoal::startExecuting()
{
    if (m_phantom == nullptr) {
        return;
    }

    math::Random rng = m_phantom->getRandom();

    // MC 1.16.5: 初始化环绕参数
    m_orbitRadius = 5.0f + rng.nextFloat() * 10.0f;      // 5.0 + [0, 10.0)
    m_orbitHeightOffset = -4.0f + rng.nextFloat() * 9.0f; // -4.0 + [0, 9.0)
    m_orbitDirection = rng.nextBoolean() ? 1.0f : -1.0f;  // 随机方向

    updateOrbitOffset();
}

void PhantomOrbitPointGoal::tick()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return;
    }

    math::Random rng = m_phantom->getRandom();

    // MC 1.16.5: 350tick 概率改变高度
    if (rng.nextInt(350) == 0) {
        m_orbitHeightOffset = -4.0f + rng.nextFloat() * 9.0f;
    }

    // MC 1.16.5: 250tick 概率增加半径或反转方向
    if (rng.nextInt(250) == 0) {
        ++m_orbitRadius;
        if (m_orbitRadius > 15.0f) {
            m_orbitRadius = 5.0f;
            m_orbitDirection = -m_orbitDirection;
        }
    }

    // MC 1.16.5: 450tick 概率重新选择起始角度
    if (rng.nextInt(450) == 0) {
        m_orbitAngle = rng.nextFloat() * math::TWO_PI;
        updateOrbitOffset();
    }

    // 接近目标点时更新环绕偏移
    if (isNearOrbitOffset()) {
        updateOrbitOffset();
    }

    // MC 1.16.5: 避开地面和天花板
    math::Vector3 pos = m_phantom->position();
    math::Vector3 offset = m_phantom->orbitOffset();
    IWorld* world = m_phantom->world();

    // 检查下方方块
    BlockPos belowPos(
        static_cast<i32>(std::floor(pos.x)),
        static_cast<i32>(std::floor(pos.y - 1.0)),
        static_cast<i32>(std::floor(pos.z))
    );
    const BlockState* belowState = world->getBlockState(belowPos);

    if (offset.y < pos.y && belowState != nullptr && !belowState->getBlock().isAir(*belowState)) {
        // 下方有方块，向上飞
        m_orbitHeightOffset = std::max(1.0f, m_orbitHeightOffset);
        updateOrbitOffset();
    }

    // 检查上方方块
    BlockPos abovePos(
        static_cast<i32>(std::floor(pos.x)),
        static_cast<i32>(std::floor(pos.y + 1.0)),
        static_cast<i32>(std::floor(pos.z))
    );
    const BlockState* aboveState = world->getBlockState(abovePos);

    if (offset.y > pos.y && aboveState != nullptr && !aboveState->getBlock().isAir(*aboveState)) {
        // 上方有方块，向下飞
        m_orbitHeightOffset = std::min(-1.0f, m_orbitHeightOffset);
        updateOrbitOffset();
    }
}

void PhantomOrbitPointGoal::updateOrbitOffset()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return;
    }

    // MC 1.16.5: func_203148_i
    BlockPos orbitPos = m_phantom->orbitPosition();

    // 如果环绕位置未初始化，使用当前位置
    if (orbitPos.x == 0 && orbitPos.y == 0 && orbitPos.z == 0) {
        math::Vector3 pos = m_phantom->position();
        orbitPos = BlockPos(
            static_cast<i32>(std::floor(pos.x)),
            static_cast<i32>(std::floor(pos.y)),
            static_cast<i32>(std::floor(pos.z))
        );
        m_phantom->setOrbitPosition(orbitPos);
    }

    // 更新环绕角度
    m_orbitAngle += m_orbitDirection * 15.0f * math::DEG_TO_RAD;

    // 计算环绕偏移
    // orbitOffset = orbitPosition + (radius * cos(angle), heightOffset, radius * sin(angle))
    f32 offsetX = m_orbitRadius * std::cos(m_orbitAngle);
    f32 offsetY = -4.0f + m_orbitHeightOffset;
    f32 offsetZ = m_orbitRadius * std::sin(m_orbitAngle);

    math::Vector3f offset(
        static_cast<f32>(orbitPos.x) + offsetX,
        static_cast<f32>(orbitPos.y) + offsetY,
        static_cast<f32>(orbitPos.z) + offsetZ
    );

    m_phantom->setOrbitOffset(offset);
}

// ============================================================================
// PhantomPickAttackGoal
// ============================================================================

PhantomPickAttackGoal::PhantomPickAttackGoal(PhantomEntity* phantom)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_phantom(phantom)
    , m_tickDelay(0)
{
    MC_ASSERT_RELEASE(phantom != nullptr);
}

bool PhantomPickAttackGoal::shouldExecute()
{
    if (m_phantom == nullptr) {
        return false;
    }

    LivingEntity* target = m_phantom->attackTarget();
    return target != nullptr && target->isAlive();
}

void PhantomPickAttackGoal::startExecuting()
{
    // MC 1.16.5: 初始延迟10tick后切换到俯冲
    m_tickDelay = 10;
    m_phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    setOrbitPositionAboveTarget();
}

void PhantomPickAttackGoal::resetTask()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return;
    }

    // MC 1.16.5: 更新环绕位置到目标上方
    LivingEntity* target = m_phantom->attackTarget();
    if (target != nullptr) {
        math::Random rng = m_phantom->getRandom();
        math::Vector3 targetPos = target->position();

        // 获取最高方块位置 + 10~30格
        i32 surfaceY = m_phantom->world()->getHeight(
            static_cast<i32>(std::floor(targetPos.x)),
            static_cast<i32>(std::floor(targetPos.z))
        );

        BlockPos newOrbitPos(
            static_cast<i32>(std::floor(targetPos.x)),
            surfaceY + 10 + rng.nextInt(20),
            static_cast<i32>(std::floor(targetPos.z))
        );

        m_phantom->setOrbitPosition(newOrbitPos);
    }
}

void PhantomPickAttackGoal::tick()
{
    if (m_phantom == nullptr) {
        return;
    }

    // MC 1.16.5: 只在环绕阶段处理
    if (m_phantom->getAttackPhase() == PhantomEntity::AttackPhase::CIRCLE) {
        --m_tickDelay;
        if (m_tickDelay <= 0) {
            // 切换到俯冲阶段
            m_phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
            setOrbitPositionAboveTarget();

            // 俯冲持续时间：8-12秒 (160-240 tick)
            math::Random rng = m_phantom->getRandom();
            m_tickDelay = (8 + rng.nextInt(4)) * 20;

            // 播放俯冲音效
            // MC 1.16.5: playSound(SoundEvents.ENTITY_PHANTOM_SWOOP, 10.0F, 0.95F + rand.nextFloat() * 0.1F)
            m_phantom->playSound(SoundEvents::ENTITY_PHANTOM_SWOOP, 10.0f, 0.95f + rng.nextFloat() * 0.1f);
        }
    }
}

void PhantomPickAttackGoal::setOrbitPositionAboveTarget()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return;
    }

    LivingEntity* target = m_phantom->attackTarget();
    if (target == nullptr) {
        return;
    }

    math::Random rng = m_phantom->getRandom();
    math::Vector3 targetPos = target->position();

    // 目标上方 20-40 格
    i32 orbitY = static_cast<i32>(std::floor(targetPos.y)) + 20 + rng.nextInt(20);

    // 不能低于海平面
    if (orbitY < world::SEA_LEVEL) {
        orbitY = world::SEA_LEVEL + 1;
    }

    BlockPos orbitPos(
        static_cast<i32>(std::floor(targetPos.x)),
        orbitY,
        static_cast<i32>(std::floor(targetPos.z))
    );

    m_phantom->setOrbitPosition(orbitPos);
}

// ============================================================================
// PhantomSweepAttackGoal
// ============================================================================

PhantomSweepAttackGoal::PhantomSweepAttackGoal(PhantomEntity* phantom)
    : PhantomMoveGoal(phantom)
    , m_catCheckTimer(0)
{
}

bool PhantomSweepAttackGoal::shouldExecute()
{
    if (m_phantom == nullptr) {
        return false;
    }

    LivingEntity* target = m_phantom->attackTarget();
    return target != nullptr && m_phantom->getAttackPhase() == PhantomEntity::AttackPhase::SWOOP;
}

bool PhantomSweepAttackGoal::shouldContinueExecuting()
{
    if (m_phantom == nullptr) {
        return false;
    }

    LivingEntity* target = m_phantom->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    // 检查目标是否是玩家，且不在旁观/创造模式
    auto* player = dynamic_cast<Player*>(target);
    if (player != nullptr && (player->isSpectator() || player->isCreative())) {
        return false;
    }

    // 检查是否仍在俯冲阶段
    if (!shouldExecute()) {
        return false;
    }

    // MC 1.16.5: 每20tick检测猫
    return checkForCats();
}

void PhantomSweepAttackGoal::resetTask()
{
    if (m_phantom != nullptr) {
        m_phantom->setAttackTarget(nullptr);
        m_phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    }
}

void PhantomSweepAttackGoal::tick()
{
    if (m_phantom == nullptr) {
        return;
    }

    LivingEntity* target = m_phantom->attackTarget();
    if (target == nullptr) {
        return;
    }

    // MC 1.16.5: 设置环绕偏移为目标位置（眼睛高度 0.5）
    math::Vector3f targetPos = target->position();
    math::Vector3f offset(
        targetPos.x,
        targetPos.y + target->eyeHeight() * 0.5f,
        targetPos.z
    );
    m_phantom->setOrbitOffset(offset);

    // MC 1.16.5: 检测碰撞
    // 如果幻翼的碰撞箱扩大0.2格后与目标碰撞箱相交，执行攻击
    AxisAlignedBB phantomBB = m_phantom->boundingBox().grow(0.2);
    AxisAlignedBB targetBB = target->boundingBox();

    if (phantomBB.intersects(targetBB)) {
        // 执行攻击
        m_phantom->attackEntityAsMob(*target);
        m_phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);

        // MC 1.16.5: 播放攻击音效
        // world.playEvent(1039, getPosition(), 0) - 事件ID 1039 是幻翼攻击事件
        if (IWorld* world = m_phantom->world()) {
            world->playEvent(1039, BlockPos(math::floorTo<i32>(m_phantom->x()),
                                            math::floorTo<i32>(m_phantom->y()),
                                            math::floorTo<i32>(m_phantom->z())), 0);
        }
    }
    // MC 1.16.5: 水平碰撞或受伤时切回环绕
    else if (m_phantom->collidedHorizontally() || m_phantom->hurtTime() > 0) {
        m_phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    }
}

bool PhantomSweepAttackGoal::checkForCats()
{
    if (m_phantom == nullptr || m_phantom->world() == nullptr) {
        return true;  // 无猫时继续
    }

    // MC 1.16.5: 每20tick检测一次猫
    ++m_catCheckTimer;
    if (m_catCheckTimer < 20) {
        return true;
    }
    m_catCheckTimer = 0;

    // 搜索16格内的猫
    // MC 1.16.5: 如果发现猫，猫会发出嘶嘶声，幻翼停止攻击
    // TODO: 当 CatEntity 实现后，搜索猫实体并调用其 hiss() 方法
    // 目前简化：假设没有猫
    return true;
}

} // namespace mc::entity::ai::goal
