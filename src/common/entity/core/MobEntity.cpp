#include "MobEntity.hpp"
#include "../ai/EntitySenses.hpp"
#include "../ai/controller/LookController.hpp"
#include "../ai/controller/MovementController.hpp"
#include "../ai/controller/JumpController.hpp"
#include "../ai/pathfinding/PathNavigator.hpp"
#include "../experience/ExperienceDropHandler.hpp"
#include "../../util/math/random/Random.hpp"

namespace mc {

MobEntity::MobEntity(LegacyEntityType type, EntityId id)
    : LivingEntity(type, id)
    , m_lookController(std::make_unique<entity::ai::controller::LookController>(this))
    , m_moveController(std::make_unique<entity::ai::controller::MovementController>(this))
    , m_jumpController(std::make_unique<entity::ai::controller::JumpController>(this))
    , m_senses(std::make_unique<entity::ai::EntitySenses>(this))
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

entity::ai::EntitySenses* MobEntity::senses() {
    return m_senses.get();
}

const entity::ai::EntitySenses* MobEntity::senses() const {
    return m_senses.get();
}

math::Random MobEntity::getRandom() const {
    // 基于实体ID和tick生成随机数种子
    return math::Random(static_cast<u64>(m_id) | (static_cast<u64>(m_ticksExisted) << 32));
}

bool MobEntity::isBeingRidden() const {
    return hasPassengers();
}

void MobEntity::clearNavigation() {
    if (m_navigator) {
        m_navigator->clearPath();
    }
}

void MobEntity::playAmbientSound() {
    auto soundEvent = getAmbientSound();
    if (soundEvent.has_value()) {
        playSound(*soundEvent, getSoundVolume(), getSoundPitch());
    }
}

void MobEntity::playAttackSound(LivingEntity& target) {
    (void)target;
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
    // 更新父类（LivingEntity::tick() 已经调用 aiStep()）
    LivingEntity::tick();

    // MC 1.16.5: 空闲时间在 tick 开头递增
    ++m_idleTime;

    // 环境声音检查
    if (isAlive()) {
        math::Random random = getRandom();
        if (random.nextInt(1000) < m_livingSoundTime++) {
            m_livingSoundTime = -getTalkInterval();
            playAmbientSound();
        }
    }

    // MC 1.16.5 updateEntityActionState() 顺序:
    // 1. 感知更新
    if (m_senses) {
        m_senses->tick();
    }

    // 2. 目标选择器 (先于 goalSelector)
    // 3. 行为目标选择器
    if (m_aiEnabled) {
        m_targetSelector.tick();
        m_goalSelector.tick();

        // 4. 导航器更新
        if (m_navigator) {
            m_navigator->tick();
        }

        // 5. AI 任务更新 (子类可重写)
        updateAITasks();

        // MC 1.16.5: 每 5 tick 更新移动目标标志
        if (m_ticksExisted % 5 == 0) {
            updateMovementGoalFlags();
        }
    }

    // 6. 控制器更新 (顺序: move -> look -> jump)
    if (m_moveController) {
        m_moveController->tick();
    }
    if (m_lookController) {
        m_lookController->tick();
    }
    if (m_jumpController) {
        m_jumpController->tick();
    }

    // 注意：aiStep() 已在 LivingEntity::tick() 中调用，这里不需要再次调用
}

void MobEntity::updateMovementGoalFlags() {
    // MC 1.16.5: 根据骑乘状态更新目标标志
    // 如果被骑乘，禁用 MOVE/JUMP/LOOK 标志
    bool canMove = !isBeingRidden();

    m_goalSelector.setFlag(entity::ai::GoalFlag::Move, canMove);
    m_goalSelector.setFlag(entity::ai::GoalFlag::Jump, canMove);
    m_goalSelector.setFlag(entity::ai::GoalFlag::Look, canMove);
}

std::optional<ResourceLocation> MobEntity::getAmbientSound() const {
    return makeSoundEventId("ambient");
}

void MobEntity::playHurtSound(DamageSource& source) {
    m_livingSoundTime = -getTalkInterval();
    LivingEntity::playHurtSound(source);
}

void MobEntity::dropExperience() {
    // 如果有经验值，生成经验球
    if (m_experienceValue > 0 && m_world) {
        math::Random rng = getRandom();
        entity::ExperienceDropHandler::spawnHostileMobExperience(
            m_world, x(), y(), z(), m_experienceValue, &rng
        );
    }
}

} // namespace mc
