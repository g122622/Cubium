#include "MobEntity.hpp"
#include "../ai/controller/LookController.hpp"
#include "../ai/controller/MovementController.hpp"
#include "../ai/controller/JumpController.hpp"
#include "../ai/pathfinding/PathNavigator.hpp"
#include "../../util/math/random/Random.hpp"

namespace mc {

MobEntity::MobEntity(LegacyEntityType type, EntityId id)
    : LivingEntity(type, id)
    , m_lookController(std::make_unique<entity::ai::controller::LookController>(this))
    , m_moveController(std::make_unique<entity::ai::controller::MovementController>(this))
    , m_jumpController(std::make_unique<entity::ai::controller::JumpController>(this))
    , m_navigator(std::make_unique<entity::ai::pathfinding::PathNavigator>(this))
{
    // 子类可在此初始化寻路器
}

MobEntity::~MobEntity() = default;

entity::ai::controller::LookController* MobEntity::lookController() {
    return m_lookController.get();
}

const entity::ai::controller::LookController* MobEntity::lookController() const {
    return m_lookController.get();
}

entity::ai::controller::MovementController* MobEntity::moveController() {
    return m_moveController.get();
}

const entity::ai::controller::MovementController* MobEntity::moveController() const {
    return m_moveController.get();
}

entity::ai::controller::JumpController* MobEntity::jumpController() {
    return m_jumpController.get();
}

const entity::ai::controller::JumpController* MobEntity::jumpController() const {
    return m_jumpController.get();
}

entity::ai::pathfinding::PathNavigator* MobEntity::navigator() {
    return m_navigator.get();
}

const entity::ai::pathfinding::PathNavigator* MobEntity::navigator() const {
    return m_navigator.get();
}

math::Random MobEntity::getRandom() const {
    // 基于实体ID和tick生成随机数种子
    return math::Random(static_cast<u64>(m_id) | (static_cast<u64>(m_ticksExisted) << 32));
}

bool MobEntity::isBeingRidden() const {
    // 目前没有乘客系统，返回 false
    // TODO: 实现乘客系统后检查 getPassengers().empty()
    return false;
}

void MobEntity::clearNavigation() {
    if (m_navigator) {
        m_navigator->clearPath();
    }
}

void MobEntity::lookAt(const Entity& target, f32 deltaYaw, f32 deltaPitch) {
    lookAt(target.x(), target.y() + target.eyeHeight(), target.z(), deltaYaw, deltaPitch);
}

void MobEntity::lookAt(f64 x, f64 y, f64 z, f32 deltaYaw, f32 deltaPitch) {
    if (m_lookController) {
        m_lookController->setLookPosition(x, y, z, deltaYaw, deltaPitch);
    }
}

void MobEntity::tick() {
    // 更新父类
    LivingEntity::tick();

    // 更新空闲时间
    // 如果实体在移动，重置空闲时间；否则增加
    if (m_velocity.x != 0.0f || m_velocity.z != 0.0f) {
        m_idleTime = 0;
    } else {
        m_idleTime++;
    }

    // 更新 AI 目标
    if (m_aiEnabled) {
        m_goalSelector.tick();
        m_targetSelector.tick();
    }

    // 更新控制器
    if (m_lookController) {
        m_lookController->tick();
    }
    if (m_moveController) {
        m_moveController->tick();
    }
    if (m_jumpController) {
        m_jumpController->tick();
    }

    // 执行AI移动物理更新
    // 这会根据 m_moveForward 和 m_moveStrafing 执行实际移动
    // 参考 MC MobEntity.livingTick() 中的 aiStep() 调用
    aiStep();
}

} // namespace mc
