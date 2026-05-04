#include "AbstractHorseEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/DataParameter.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../world/blockentity/core/SimpleInventory.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/special/SpecialGoals.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/math/MathConstants.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include <cmath>

namespace mc {

// MC 1.16.5 数据参数定义 - 必须在命名空间级别定义静态成员
entity::DataParameter<i8> AbstractHorseEntity::STATUS_PARAM{0};
entity::DataParameter<i64> AbstractHorseEntity::OWNER_UUID_PARAM{1};

AbstractHorseEntity::AbstractHorseEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 初始化随机属性
    initRandomAttributes();

    // MC 1.16.5: 初始化马背包（鞍槽 + 马铠槽）
    initHorseChest();
}

void AbstractHorseEntity::registerData() {
    AnimalEntity::registerData();

    // MC 1.16.5 AbstractHorseEntity.registerData()
    m_dataManager.registerParam(STATUS_PARAM, static_cast<i8>(0));
    m_dataManager.registerParam(OWNER_UUID_PARAM, static_cast<i64>(0));  // 0 = 无主人
}

// ========== 状态标志辅助方法 ==========

bool AbstractHorseEntity::getHorseWatchableBoolean(i8 flag) const {
    return (m_dataManager.get(STATUS_PARAM) & flag) != 0;
}

void AbstractHorseEntity::setHorseWatchableBoolean(i8 flag, bool value) {
    i8 status = m_dataManager.get(STATUS_PARAM);
    if (value) {
        m_dataManager.set(STATUS_PARAM, static_cast<i8>(status | flag));
    } else {
        m_dataManager.set(STATUS_PARAM, static_cast<i8>(status & ~flag));
    }
}

void AbstractHorseEntity::setSaddle(bool saddle) {
    m_saddled = saddle;
    setHorseWatchableBoolean(STATUS_FLAG_SADDLE, saddle);
}

void AbstractHorseEntity::onPlayerStartRiding(Player* player) {
    m_rider = player;
    // TODO: 设置乘客位置
}

void AbstractHorseEntity::onPlayerStopRiding(Player* player) {
    m_rider = nullptr;
    // TODO: 移除乘客
}

f32 AbstractHorseEntity::getSteeringSpeed() const {
    if (!m_saddled) return 0.0f;

    f32 speed = getSpeed();

    // 加速时增加速度
    if (m_isBoosting) {
        speed *= 1.5f;
    }

    return speed;
}

bool AbstractHorseEntity::boost() {
    if (!m_saddled || m_boostTime > 0) {
        return false;
    }

    // 开始加速
    m_isBoosting = true;
    m_boostTime = MAX_BOOST_TIME;
    return true;
}

void AbstractHorseEntity::onJump() {
    if (!m_saddled || !m_isJumping) {
        return;
    }

    performJump();
}

void AbstractHorseEntity::setJumpPower(f32 power) {
    m_jumpPower = std::clamp(power, 0.0f, 1.0f);
}

f32 AbstractHorseEntity::getMaxJumpHeight() const {
    // 根据跳跃强度计算最大跳跃高度
    // MC 公式: 0.6 * jumpStrength^2 + 0.1 * jumpStrength + 0.3
    return 0.6f * m_jumpStrength * m_jumpStrength + 0.1f * m_jumpStrength + 0.3f;
}

bool AbstractHorseEntity::canJump() const {
    return m_saddled && m_jumpCooldown <= 0;
}

void AbstractHorseEntity::startJumping() {
    if (!canJump()) {
        return;
    }

    m_isJumping = true;
    m_jumpPower = 0.0f;
}

void AbstractHorseEntity::stopJumping() {
    if (!m_isJumping) {
        return;
    }

    // 执行跳跃
    performJump();
    m_isJumping = false;
    m_jumpPower = 0.0f;
    m_jumpCooldown = 10; // 跳跃冷却
}

bool AbstractHorseEntity::isBeingRidden() const {
    return m_rider != nullptr;
}

bool AbstractHorseEntity::canBeRiddenBy(Player* player) const {
    // 已被骑乘或未驯服
    if (m_rider != nullptr) {
        return false;
    }

    // 需要驯服才能骑乘（子类可覆盖此逻辑）
    if (!m_tame) {
        return false;
    }

    return true;
}

void AbstractHorseEntity::setTame(bool tame) {
    m_tame = tame;
    setHorseWatchableBoolean(STATUS_FLAG_TAME, tame);
}

// ========== 库存初始化 ==========

void AbstractHorseEntity::initHorseChest() {
    // MC 1.16.5: 创建马背包（鞍槽 + 马铠槽）
    m_inventory = std::make_unique<blockentity::SimpleInventory>(getInventorySize());
}

// ========== IEquipable 接口实现 ==========

ItemStack AbstractHorseEntity::getEquipment(i32 slot) const {
    if (!m_inventory || slot < 0 || slot >= getInventorySize()) {
        return ItemStack::EMPTY;
    }
    return m_inventory->getItem(slot);
}

void AbstractHorseEntity::setEquipment(i32 slot, const ItemStack& item) {
    if (!m_inventory || slot < 0 || slot >= getInventorySize()) {
        return;
    }
    m_inventory->setItem(slot, item);

    // 更新鞍/马铠状态
    if (slot == 0) {
        // 鞍槽
        setSaddle(!item.isEmpty());
    } else if (slot == 1) {
        // 马铠槽
        setArmor(!item.isEmpty());
    }
}

bool AbstractHorseEntity::canEquip(const ItemStack& item, i32 slot) const {
    if (item.isEmpty()) {
        return true;
    }

    // 槽位0: 鞍
    // 槽位1: 马铠（子类可扩展）
    // TODO: 检查物品类型
    MC_UNUSED(item);
    MC_UNUSED(slot);
    return true;
}

// ========== IRideable travelTowards ==========

void AbstractHorseEntity::travelTowards(const Vector3& travelVec) {
    // MC 1.16.5: travelTowards -> super.travel
    AnimalEntity::travel(travelVec);
}

bool AbstractHorseEntity::increaseTemper(i32 amount) {
    m_temper += amount;

    if (m_temper >= m_maxTemper) {
        // 达到驯服阈值
        m_temper = m_maxTemper;
        setTame(true);
        return true;
    }

    return false;
}

bool AbstractHorseEntity::isTameItem(const ItemStack& itemStack) const {
    // 默认情况下，马不响应任何驯服物品
    // 子类应覆盖此方法
    (void)itemStack;
    return false;
}

f32 AbstractHorseEntity::getSpeed() const {
    return m_speed;
}

void AbstractHorseEntity::tick() {
    AnimalEntity::tick();

    // 更新跳跃冷却
    if (m_jumpCooldown > 0) {
        m_jumpCooldown--;
    }

    // 更新跳跃蓄力
    if (m_isJumping) {
        updateJumpPower();
    }

    // 更新加速状态
    updateBoost();

    // 更新骑乘状态
    updateRiding();
}

void AbstractHorseEntity::travel(f32 strafing, f32 vertical, f32 forward) {
    // MC 1.16.5 AbstractHorseEntity.travel()
    if (!isAlive()) {
        return;
    }

    // 检查是否被骑乘且可以控制
    // 注意：使用IRideable::canBeSteered()来检查鞍状态
    if (isBeingRidden() && entity::IRideable::canBeSteered() && m_saddled) {
        // 获取控制乘客（玩家）
        const auto& passengerIds = getPassengers();
        Entity* controllingPassenger = nullptr;
        if (!passengerIds.empty() && world() != nullptr) {
            controllingPassenger = world()->getEntity(passengerIds[0]);
        }

        if (controllingPassenger != nullptr) {
            // 同步朝向
            setRotation(controllingPassenger->yaw(), controllingPassenger->pitch() * 0.5f);

            // MC 1.16.5: 侧向移动减半
            f32 sideInput = strafing * 0.5f;
            f32 forwardInput = forward;

            // 后退时速度降低
            if (forwardInput <= 0.0f) {
                forwardInput *= 0.25f;
            }

            // 在地面且没有跳跃力且正在扬蹄时不能移动
            if (onGround() && m_jumpPower == 0.0f && m_isJumping && !m_allowStandSliding) {
                sideInput = 0.0f;
                forwardInput = 0.0f;
            }

            // 处理跳跃
            if (m_jumpPower > 0.0f && !m_isJumping && onGround()) {
                // 计算跳跃力度
                f64 jumpForce = static_cast<f64>(getJumpStrength() * m_jumpPower);
                // TODO: 跳跃药水效果加成

                // 设置跳跃速度
                setVelocity(velocityX(), static_cast<f32>(jumpForce), velocityZ());
                m_isJumping = true;
                m_jumpPower = 0.0f;

                // 前进时额外推力
                if (forwardInput > 0.0f) {
                    f32 yawRad = math::toRadians(yaw());
                    f32 pushX = -0.4f * std::sin(yawRad) * m_jumpPower;
                    f32 pushZ = 0.4f * std::cos(yawRad) * m_jumpPower;
                    addVelocity(pushX, 0.0f, pushZ);
                }
            }

            // 设置AI移动速度
            // m_jumpMovementFactor = getAIMoveSpeed() * 0.1f;

            // 执行移动
            if (canPassengerSteer()) {
                // setAIMoveSpeed(static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED)));
                AnimalEntity::travel(sideInput, vertical, forwardInput);
            } else {
                // 无法控制时停止移动
                setVelocity(Vector3(0.0f, 0.0f, 0.0f));
            }

            // 着地时重置跳跃状态
            if (onGround()) {
                m_jumpPower = 0.0f;
                m_isJumping = false;
            }
        }
    } else {
        // 未被骑乘时使用普通移动
        // m_jumpMovementFactor = 0.02f;
        AnimalEntity::travel(strafing, vertical, forward);
    }
}

void AbstractHorseEntity::registerAttributes() {
    AnimalEntity::registerAttributes();

    // 马类基础属性
    m_attributes.registerAttribute(*entity::attribute::Attributes::horseJumpStrength());
    m_attributes.setBaseValue(entity::attribute::Attributes::HORSE_JUMP_STRENGTH, m_jumpStrength);

    // 设置生命值和速度
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, m_horseHealth);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_speed);
}

void AbstractHorseEntity::registerGoals() {
    AnimalEntity::registerGoals();

    // MC 1.16.5 AbstractHorseEntity.registerGoals()
    // 注意：RunAroundLikeCrazyGoal 的优先级和 PanicGoal 相同（都是1）
    // 这样未驯服的马被骑乘时会优先执行疯狂奔跑
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::RunAroundLikeCrazyGoal>(this, 1.2));
}

void AbstractHorseEntity::updateRiding() {
    // TODO: 更新骑乘者位置
    // TODO: 处理骑乘者控制
}

void AbstractHorseEntity::updateJumpPower() {
    if (m_jumpPower < 1.0f) {
        // 蓄力增加
        m_jumpPower += 0.05f;
        m_jumpPower = std::min(m_jumpPower, 1.0f);
    }
}

void AbstractHorseEntity::performJump() {
    if (!canJump() || m_jumpPower <= 0.0f) {
        return;
    }

    // 计算跳跃力度
    f32 jumpForce = m_jumpStrength * m_jumpPower;

    // 设置垂直速度
    setVelocity(velocityX(), jumpForce, velocityZ());

    // 设置跳跃冷却
    m_jumpCooldown = 10;
}

void AbstractHorseEntity::updateBoost() {
    if (m_boostTime > 0) {
        m_boostTime--;

        if (m_boostTime <= 0) {
            m_isBoosting = false;
        }
    }
}

void AbstractHorseEntity::initRandomAttributes() {
    math::Random rng(ticksExisted());

    // 随机生成马特有属性
    m_speed = rng.nextFloat(MIN_SPEED, MAX_SPEED);
    m_jumpStrength = rng.nextFloat(MIN_JUMP, MAX_JUMP);
    m_horseHealth = rng.nextFloat(MIN_HEALTH, MAX_HEALTH);
}

} // namespace mc
