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
 * THE SOFTWARE IS PROVIDED " IS ", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "OcelotEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/GoalConstants.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/basic/ChickenEntity.hpp"
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>
#include <unordered_set>

namespace mc {

// ==================== OcelotEntity ====================

OcelotEntity::OcelotEntity(EntityId id)
    : AnimalEntity(id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> OcelotEntity::create(IWorld* /*world*/)
{
    return std::make_unique<OcelotEntity>(0);
}

bool OcelotEntity::trustsPlayer(u64 playerId) const
{
    return m_trusting && m_trustingPlayerId == playerId;
}

void OcelotEntity::setPlayerTrust(u64 playerId, bool trust)
{
    if (trust && !m_trusting) {
        m_trusting = true;
        m_trustingPlayerId = playerId;
        // 触发 AI 更新
        _setupTrustingAI();
    }
}

void OcelotEntity::setTrusting(bool trusting)
{
    if (m_trusting != trusting) {
        m_trusting = trusting;
        // 触发 AI 更新
        _setupTrustingAI();
    }
}

bool OcelotEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 豹猫使用生鳕鱼和生鲑鱼繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::COD || item == Items::SALMON;
}

std::unique_ptr<AnimalEntity> OcelotEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // 创建一个新的豹猫实体，不需要继承父母特征
    auto baby = std::make_unique<OcelotEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void OcelotEntity::tick()
{
    AnimalEntity::tick();

    // 如果已建立信任，停止逃跑
    if (m_trusting) {
        m_fleeing = false;
    }
}

void OcelotEntity::updateAITasks()
{
    // 根据移动速度设置潜行/奔跑姿态
    auto* moveController = this->moveController();
    if (moveController && moveController->isUpdating()) {
        f64 speed = moveController->speed();
        if (std::abs(speed - TEMPT_SPEED) < 0.01) {
            // 被诱惑时潜行
            setPose(EntityPose::Crouching);
        } else if (std::abs(speed - AVOID_NEAR_SPEED) < 0.01) {
            // 逃跑时站立
            setPose(EntityPose::Standing);
        } else {
            setPose(EntityPose::Standing);
        }
    } else {
        setPose(EntityPose::Standing);
    }
}

bool OcelotEntity::canDespawn(double distanceToClosestPlayer) const noexcept
{
    // 未信任的豹猫存在超过 2400 tick (2分钟) 后可以消失
    MC_UNUSED(distanceToClosestPlayer);
    return !m_trusting && ticksExisted() > DESPAWN_TICKS;
}

bool OcelotEntity::attackEntityAsMob(LivingEntity& target)
{
    EntityDamageSource damageSource = DamageSources::mobAttack(this);
    return target.hurt(damageSource, ATTACK_DAMAGE);
}

ActionResultType OcelotEntity::interactMob(Player& player, Hand hand)
{
    ItemStack itemStack = player.getHeldItem(hand);
    const Item* item = itemStack.getItem();

    // 检查条件：
    // 1. 诱惑目标正在运行（或为空）
    // 2. 尚未信任
    // 3. 手持繁殖物品（生鱼）
    // 4. 玩家距离 < 9.0D (3格)
    bool isTempting = (m_temptGoal == nullptr || m_temptGoal->isRunning());
    bool isBreedingFood = item != nullptr && (item == Items::COD || item == Items::SALMON);
    double distSq = player.distanceSqTo(*this);

    if (isTempting && !m_trusting && isBreedingFood && distSq < 9.0) {
        // 消耗物品
        itemStack.shrink(1);

        // 服务端处理
        if (m_world && !m_world->isClientSide()) {
            // 1/3 概率建立信任
            math::Random rng = getRandom();
            if (rng.nextInt(3) == 0) {
                // 建立信任
                setPlayerTrust(player.playerId(), true);
                _spawnTrustingParticles(true);
            } else {
                // 失败，显示烟雾粒子
                _spawnTrustingParticles(false);
            }
        }

        return ActionResultType::Success;
    }

    // 调用父类处理
    return AnimalEntity::interactMob(player, hand);
}

void OcelotEntity::registerGoals()
{
    // AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表

    // 优先级 1: 游泳（最高优先级）
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 3: 食物诱惑（生鱼）
    // scaredByMovement = true，玩家快速移动会吓跑豹猫
    m_temptGoal = new entity::ai::goal::OcelotTemptGoal(
        this,
        TEMPT_SPEED,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr && (item == Items::COD || item == Items::SALMON);
        },
        true); // scaredByMovement = true
    m_goalSelector.addGoal(3, m_temptGoal);

    // 优先级 4: 躲避玩家（未信任时）- 在 _setupTrustingAI() 中动态添加
    _setupTrustingAI();

    // 优先级 7: 跳跃攻击
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::LeapAtTargetGoal>(this, 0.3f));

    // 优先级 8: 豹猫近战攻击
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::OcelotAttackGoal>(this));

    // 优先级 9: 繁殖
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::BreedGoal>(this, 0.8));

    // 优先级 10: 避水随机漫步
    m_goalSelector.addGoal(
        10, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 0.8, 1.0000001E-5f));

    // 优先级 11: 看向玩家
    m_goalSelector.addGoal(11, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f));

    // 目标选择器：攻击小鸡和小海龟
    m_targetSelector.addGoal(
        1, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<ChickenEntity>>(this, false, 0));
    // 注意：攻击海龟时，完整实现应该只攻击干燥的小海龟
    // 当前简化实现：攻击所有海龟
    m_targetSelector.addGoal(
        1, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<TurtleEntity>>(this, false, 10));
}

void OcelotEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE);
}

void OcelotEntity::registerData()
{
    AnimalEntity::registerData();

    // 注册信任状态数据参数
    // TODO: 当前简化实现使用成员变量，未来可以添加网络同步
}

void OcelotEntity::_setupTrustingAI()
{
    // 动态添加/移除 AvoidPlayerGoal

    if (m_avoidPlayerGoal == nullptr) {
        // 创建躲避玩家目标
        m_avoidPlayerGoal =
            new entity::ai::goal::OcelotAvoidPlayerGoal(this, AVOID_DISTANCE, AVOID_FAR_SPEED, AVOID_NEAR_SPEED);
    }

    // 先移除已有的 AvoidPlayerGoal
    m_goalSelector.removeGoal(m_avoidPlayerGoal);

    // 如果未信任，添加躲避玩家目标
    if (!m_trusting) {
        m_goalSelector.addGoal(4, m_avoidPlayerGoal);
    }
}

void OcelotEntity::_spawnTrustingParticles(bool success)
{
    if (!m_world) {
        return;
    }

    // 通过 broadcastEntityStatus 广播事件码，由客户端在 handleEntityEvent 中生成粒子
    // 豹猫使用独立事件码 40/41（不同于 TamableAnimal 的 6/7）
    if (success) {
        m_world->broadcastEntityStatus(
            id(), static_cast<u8>(network::EntityStatusPacket::Status::OcelotTrustSucceeded));
    } else {
        m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatusPacket::Status::OcelotTrustFailed));
    }
}

// ==================== OcelotAvoidPlayerGoal ====================

namespace entity::ai::goal {

OcelotAvoidPlayerGoal::OcelotAvoidPlayerGoal(OcelotEntity* ocelot, f32 avoidDistance, f64 farSpeed, f64 nearSpeed)
    : AvoidEntityGoal(ocelot,
          avoidDistance,
          farSpeed,
          nearSpeed,
          // 只避开可以作为 AI 目标的玩家
          [](const LivingEntity* entity) -> bool {
              if (entity == nullptr) {
                  return false;
              }
              // 检查是否是玩家
              const Player* player = dynamic_cast<const Player*>(entity);
              if (player == nullptr) {
                  return false;
              }
              // !isSpectator() && isAlive()
              return !player->isSpectator() && player->isAlive();
          })
    , m_ocelot(ocelot)
{
    // 重写 shouldExecute() 和 shouldContinueExecuting() 使其只在未信任时执行
}

bool OcelotAvoidPlayerGoal::shouldExecute()
{
    // 只有未信任的豹猫才会避开玩家
    if (m_ocelot == nullptr || m_ocelot->isTrusting()) {
        return false;
    }
    return AvoidEntityGoal::shouldExecute();
}

bool OcelotAvoidPlayerGoal::shouldContinueExecuting()
{
    // 只有未信任的豹猫才会继续避开玩家
    if (m_ocelot == nullptr || m_ocelot->isTrusting()) {
        return false;
    }
    return AvoidEntityGoal::shouldContinueExecuting();
}

// ==================== OcelotTemptGoal ====================

OcelotTemptGoal::OcelotTemptGoal(OcelotEntity* ocelot, f64 speed, ItemPredicate itemPredicate, bool scaredByMovement)
    : TemptGoal(ocelot, speed, std::move(itemPredicate), scaredByMovement)
    , m_ocelot(ocelot)
{
    // 重写 isScaredByPlayerMovement() 使其根据信任状态变化
}

bool OcelotTemptGoal::isScaredByPlayerMovement() const
{
    // 只有未信任的豹猫才会被玩家移动吓跑
    // 已信任的豹猫仍然会被诱惑，但不会被移动吓跑
    return TemptGoal::isScaredByPlayerMovement() && !m_ocelot->isTrusting();
}

// ==================== OcelotAttackGoal ====================

OcelotAttackGoal::OcelotAttackGoal(OcelotEntity* ocelot)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_ocelot(ocelot)
{}

bool OcelotAttackGoal::shouldExecute()
{
    LivingEntity* target = m_ocelot->attackTarget();
    if (target == nullptr) {
        return false;
    }

    m_target = target;
    return true;
}

bool OcelotAttackGoal::shouldContinueExecuting()
{
    if (!m_target || !m_target->isAlive()) {
        return false;
    }

    // 距离超过 225.0D (15*15) 时停止追踪
    if (m_ocelot->distanceSqTo(*m_target) > STOP_ATTACK_DISTANCE_SQ) {
        return false;
    }

    // 路径未完成或仍有目标
    auto* nav = m_ocelot->navigator();
    return (nav && !nav->noPath()) || shouldExecute();
}

void OcelotAttackGoal::startExecuting()
{
    // 移动到目标
    if (m_target && m_ocelot->navigator()) {
        static_cast<void>(m_ocelot->navigator()->moveTo(*m_target, 1.0));
    }
    m_attackCooldown = 0;
}

void OcelotAttackGoal::resetTask()
{
    m_target = nullptr;
    if (m_ocelot->navigator()) {
        m_ocelot->navigator()->clearPath();
    }
}

void OcelotAttackGoal::tick()
{
    if (!m_target || !m_ocelot) return;

    // 看向目标
    m_ocelot->lookController()->setLookPositionWithEntity(*m_target, 30.0f, 30.0f);

    // 攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 计算距离
    f64 distSq = m_ocelot->distanceSqTo(*m_target);

    // 检查攻击范围（宽度 * 2 的平方）
    f32 attackReachSq = (m_ocelot->width() * 2.0f) * (m_ocelot->width() * 2.0f) + m_target->width();

    // 在攻击范围内且冷却完成
    if (distSq <= attackReachSq && m_attackCooldown <= 0) {
        m_attackCooldown = ATTACK_COOLDOWN_TICKS;
        static_cast<void>(m_ocelot->attackEntityAsMob(*m_target));
    }

    // 路径更新
    auto* nav = m_ocelot->navigator();
    if (nav && !nav->noPath()) {
        static_cast<void>(nav->moveTo(*m_target, 1.0));
    }
}

} // namespace entity::ai::goal
} // namespace mc
