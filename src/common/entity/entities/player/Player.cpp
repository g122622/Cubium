#include "Player.hpp"
#include "GameModeUtils.hpp"
#include "../../inventory/Slot.hpp"
#include "../../experience/ExperienceManager.hpp"
#include "../../../physics/PhysicsEngine.hpp"
#include "../../../physics/PhysicsConstants.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../inventory/CreativeInventory.hpp"
#include "../../experience/ExperienceDropHandler.hpp"
#include "../../../world/IWorld.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cmath>
#include <chrono>

namespace mc {

namespace {

/// 获取玩家指定姿态的宽度
/// MC 1.16.5: Sleeping 姿态宽度为 0.2，其他姿态为 0.6
[[nodiscard]] f32 getPlayerPoseWidth(EntityPose pose) {
    if (pose == EntityPose::Sleeping) {
        return 0.2f;  // MC 1.16.5: EntitySize.fixed(0.2F, 0.2F)
    }
    return Player::PLAYER_WIDTH;
}

[[nodiscard]] f32 getPlayerPoseHeight(EntityPose pose) {
    switch (pose) {
        case EntityPose::Sleeping:
            return 0.2f;
        case EntityPose::Swimming:
        case EntityPose::FallFlying:
        case EntityPose::SpinAttack:
            return Player::PLAYER_SWIM_HEIGHT;
        case EntityPose::Crouching:
            return Player::PLAYER_CROUCH_HEIGHT;
        default:
            return Player::PLAYER_HEIGHT;
    }
}

[[nodiscard]] f32 getPlayerPoseEyeHeight(EntityPose pose) {
    switch (pose) {
        case EntityPose::Sleeping:
            return 0.2f;
        case EntityPose::Swimming:
        case EntityPose::FallFlying:
        case EntityPose::SpinAttack:
            return 0.4f;
        case EntityPose::Crouching:
            return 1.27f;
        default:
            return Player::PLAYER_EYE_HEIGHT;
    }
}

constexpr f32 PLAYER_POSE_FIT_EPSILON = 1.0e-4f;

} // namespace

// ============================================================================
// Player 实现
// ============================================================================

Player::Player(EntityId id, const String& username)
    : Entity(LegacyEntityType::Player, id)
    , m_username(username)
    , m_experienceManager(std::make_unique<entity::experience::ExperienceManager>(*this))
{
    // 生成随机XP seed
    math::Random rng(static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    m_experienceManager->resetXpSeed(rng);
}

Player::~Player() = default;

void Player::setPosition(f32 x, f32 y, f32 z) {
    Entity::setPosition(x, y, z);

    // 外部改坐标时同步复位步距采样，避免沿用旧位移或旧脚步阈值
    m_moveDistanceSamplePosition = m_position;
    m_moveDistanceWalked = 0.0f;
    m_prevMoveDistanceWalked = 0.0f;
    m_moveDistanceSwam = 0.0f;
    m_prevMoveDistanceSwam = 0.0f;
    m_distanceWalkedOnStep = 0.0f;
    m_nextStepDistance = 1.0f;
    m_shouldPlayStepSound = false;
    m_shouldPlaySwimSound = false;
    m_swimSoundVolume = 0.0f;
}

void Player::setGameMode(GameMode mode) {
    m_gameMode = mode;

    // 使用 GameModeUtils 更新能力
    m_abilities = entity::GameModeUtils::getAbilitiesForGameMode(mode);
}

void Player::setHealth(f32 health) {
    m_health = std::clamp(health, 0.0f, m_maxHealth);
}

void Player::heal(f32 amount) {
    if (amount <= 0.0f || isDead()) return;
    setHealth(m_health + amount);
}

void Player::damage(f32 amount) {
    if (m_abilities.invulnerable || amount <= 0.0f) return;
    setHealth(m_health - amount);

    if (m_health <= 0.0f) {
        // 玩家死亡
        if (auto soundEvent = makeSoundEventId("death")) {
            playSound(*soundEvent, 1.0f, 1.0f);
        }
        m_health = 0.0f;
        m_deathTime = 0;
    } else {
        if (auto soundEvent = makeSoundEventId("hurt")) {
            playSound(*soundEvent, 1.0f, 1.0f);
        }
        m_hurtTime = 10;
    }
}

// ============================================================================
// 经验系统 - 委托给 ExperienceManager
// ============================================================================

void Player::addExperience(i32 amount) {
    m_experienceManager->addExperience(amount);
}

void Player::setExperienceLevel(i32 level) {
    m_experienceManager->setLevel(level);
}

void Player::addExperienceLevels(i32 levels) {
    m_experienceManager->addLevels(levels);
}

bool Player::consumeExperience(i32 amount) {
    return m_experienceManager->consumeExperience(amount);
}

bool Player::consumeExperienceLevels(i32 levels) {
    return m_experienceManager->consumeLevels(levels);
}

i32 Player::experienceBarCapacity() const {
    return m_experienceManager->getExperienceForNextLevel();
}

void Player::setExperience(i32 level, f32 progress, i32 totalExperience) {
    m_experienceManager->setExperience(level, progress, totalExperience);
}

void Player::dropExperience() {
    // 玩家死亡时掉落经验
    // 参考 MC 1.16.5: min(level * 7, 100)
    if (m_world && m_experienceManager->getLevel() > 0) {
        i32 xpToDrop = m_experienceManager->calculateDeathDropXp();
        if (xpToDrop > 0) {
            math::Random rng(static_cast<u64>(m_id) ^ static_cast<u64>(std::chrono::system_clock::now().time_since_epoch().count()));
            entity::ExperienceDropHandler::spawnExperienceOrbs(
                m_world, x(), y(), z(), xpToDrop, &rng
            );
        }
    }
}

// ============================================================================

void Player::setSprinting(bool sprinting) {
    m_isSprinting = sprinting;
    if (sprinting) {
        addFlag(EntityFlags::Sprinting);
    } else {
        removeFlag(EntityFlags::Sprinting);
    }
}

void Player::setSneaking(bool sneaking) {
    if (sneaking) {
        m_isSneaking = true;
        addFlag(EntityFlags::Crouching);
        setPose(EntityPose::Crouching);
        return;
    }

    if (canFitPose(EntityPose::Standing)) {
        m_isSneaking = false;
        removeFlag(EntityFlags::Crouching);
        setPose(EntityPose::Standing);
        return;
    }

    m_isSneaking = true;
    addFlag(EntityFlags::Crouching);
    setPose(EntityPose::Crouching);
}

void Player::setSwimming(bool swimming) {
    m_isSwimming = swimming;
    if (swimming) {
        addFlag(EntityFlags::Swimming);
        setPose(EntityPose::Swimming);
        return;
    }

    removeFlag(EntityFlags::Swimming);

    if (canFitPose(EntityPose::Standing)) {
        m_isSneaking = false;
        removeFlag(EntityFlags::Crouching);
        setPose(EntityPose::Standing);
        return;
    }

    setSneaking(true);
}

void Player::toggleFlying() {
    if (!m_abilities.canFly) {
        return; // 不允许飞行则无法切换
    }
    m_abilities.flying = !m_abilities.flying;
}

void Player::setSleeping(bool sleeping) {
    m_isSleeping = sleeping;
    if (sleeping) {
        setPose(EntityPose::Sleeping);
        return;
    }

    if (canFitPose(EntityPose::Standing)) {
        m_isSneaking = false;
        removeFlag(EntityFlags::Crouching);
        setPose(EntityPose::Standing);
        return;
    }

    setSneaking(true);
}

void Player::startSleeping(const BlockPos& pos) {
    // 设置睡眠位置
    m_sleepingPosition = pos;

    // 设置睡眠状态（这会切换姿态）
    setSleeping(true);

    // 重置睡眠计时器
    sleepTimer = 0;

    // 清除速度
    setVelocity(Vector3(0.0f, 0.0f, 0.0f));
}

void Player::stopSleeping() {
    if (!m_isSleeping) {
        return;
    }

    // 清除睡眠位置
    m_sleepingPosition = std::nullopt;

    // 设置睡眠状态为 false（这会尝试切换到站立姿态）
    setSleeping(false);

    // 注意：睡眠计时器在 tick() 中会处理唤醒后的渐变
    // 唤醒后 sleepTimer 会继续增加到 110 然后重置
}

void Player::setSpawnPoint(DimensionId dimension, const BlockPos& pos, bool forced) {
    m_spawnPoint = GlobalPos(dimension, pos);
    m_spawnForced = forced;
}

f32 Player::height() const {
    return getPlayerPoseHeight(m_pose);
}

f32 Player::eyeHeight() const {
    return getPlayerPoseEyeHeight(m_pose);
}

entity::EntitySize Player::getDimensions(EntityPose pose) const {
    // MC 1.16.5: Sleeping 姿态使用固定宽度 0.2
    return entity::EntitySize(getPlayerPoseWidth(pose), getPlayerPoseHeight(pose), getPlayerPoseEyeHeight(pose), false);
}

bool Player::canFitPose(EntityPose pose) const {
    if (pose == m_pose || m_world == nullptr) {
        return true;
    }

    AxisAlignedBB candidateBox = getDimensions(pose).makeBoundingBox(m_position.x, m_position.y, m_position.z).shrink(PLAYER_POSE_FIT_EPSILON);
    return !m_world->hasBlockCollision(candidateBox) && !m_world->hasEntityCollision(candidateBox, this);
}

void Player::tick() {
    Entity::tick();

    // 更新 XP 冷却
    if (m_xpCooldown > 0) {
        m_xpCooldown--;
    }

    // 更新受伤/死亡计时器
    if (m_hurtTime > 0) {
        m_hurtTime--;
    }

    if (isDead()) {
        m_deathTime++;
    }

    // 睡眠计时器逻辑
    // 参考 MC 1.16.5 PlayerEntity.tick()
    // 睡眠时：每 tick 递增，上限 100
    // 唤醒后：计时器继续增加到 110 后才重置为 0（用于唤醒动画）
    if (m_isSleeping) {
        sleepTimer++;
        if (sleepTimer > 100) {
            sleepTimer = 100;
        }
    } else if (sleepTimer > 0) {
        sleepTimer++;
        if (sleepTimer >= 110) {
            sleepTimer = 0;
        }
    }

    // 饥饿系统 tick
    // 只有生存模式和冒险模式才处理饥饿
    if (m_gameMode == GameMode::Survival || m_gameMode == GameMode::Adventure) {
        // TODO: 从世界获取游戏规则 naturalRegeneration
        bool naturalRegeneration = true;  // 默认启用自然恢复
        // 从世界获取难度，如果没有世界则默认为 Normal
        Difficulty difficulty = m_world ? m_world->difficulty() : Difficulty::Normal;
        m_foodStats.tick(*this, difficulty, naturalRegeneration);
    }

    // 更新游泳状态和动画
    updateSwimming();

    // 更新姿态（MC 1.16.5: PlayerEntity.tick() 中调用 updatePose()）
    updatePose();

    // 更新空气供应和溺水
    updateAirSupply();

    // 更新移动距离（用于视野晃动）
    updateMoveDistance();
}

bool Player::tickPortal() {
    // 玩家需要 80 tick (4秒) 在传送门中才能传送
    // 参考 MC 1.16.5 PlayerEntity.tick() line 578-607
    if (m_inPortal && m_portalCooldown <= 0) {
        m_portalTime++;

        // 80 ticks = 4 秒 (20 ticks/秒)
        if (m_portalTime >= 80) {
            m_portalTime = 0;
            return true; // 触发传送
        }
    } else {
        m_portalTime = 0;
    }
    return false;
}

void Player::update() {
    Entity::update();
}

/**
 * @brief 处理移动输入
 *
 * 参考MC Java版 Entity.getAbsoluteMotion() 和 LivingEntity.travel() 的逻辑：
 * - MC坐标系: yaw=0 看向 -Z, yaw=90 看向 +X
 * - forward: 正值向前走, 负值向后走
 * - strafe: 正值向右走, 负值向左走
 *
 * MC公式 (Entity.getAbsoluteMotion):
 *   sinYaw = sin(yaw * PI/180)
 *   cosYaw = cos(yaw * PI/180)
 *   moveX = strafe * cosYaw - forward * sinYaw
 *   moveZ = forward * cosYaw + strafe * sinYaw
 *
 * 重要：MC中 moveRelative 是将速度**添加**到当前速度，而不是替换！
 * 参考 Entity.moveRelative() line 1166-1169:
 *   Vector3d vector3d = getAbsoluteMotion(relative, p_213309_1_, this.rotationYaw);
 *   this.setMotion(this.getMotion().add(vector3d));
 *
 * 参考源码: Entity.java:1166-1181, LivingEntity.java:2148-2167
 * 飞行上升/下降: ClientPlayerEntity.java:788-801 - 使用 flySpeed * 3.0F
 */
void Player::handleMovementInput(f32 forward, f32 strafe, bool jumping, bool sneaking) {
    // 更新跳跃状态（用于动画等）
    m_isJumping = jumping;

    // 先刷新环境状态，保证水中/岩浆中的输入分支基于当前世界状态。
    updateEnvironmentState();
    checkOnGround();

    // 水中移动使用特殊物理
    // 参考 MC LivingEntity.travel() 水中分支
    if (isInWater() && !m_abilities.flying) {
        handleWaterMovement(forward, strafe, jumping, sneaking);
        return;
    }

    // 岩浆中移动
    if (isInLava() && !m_abilities.flying) {
        handleLavaMovement(forward, strafe, jumping, sneaking);
        return;
    }

    // 计算移动速度因子
    // 参考MC: LivingEntity.getRelevantMoveFactor() - 在空中时使用jumpMovementFactor
    // 参考MC: PlayerEntity.travel() line 1448 - 飞行时jumpMovementFactor = flySpeed * (sprinting ? 2 : 1)
    f32 speedFactor = m_abilities.walkSpeed;
    if (m_isSprinting) {
        speedFactor *= 1.3f; // 冲刺速度倍率
    }
    if (sneaking && !m_abilities.flying) {
        speedFactor *= 0.3f; // 潜行速度倍率
    }
    if (m_abilities.flying) {
        // 飞行时使用flySpeed作为速度因子
        // MC: this.jumpMovementFactor = this.abilities.getFlySpeed() * (float)(this.isSprinting() ? 2 : 1);
        speedFactor = m_abilities.flySpeed * (m_isSprinting ? 2.0f : 1.0f);
    }

    // 根据朝向计算移动方向（只有有输入时才处理）
    // MC: moveRelative() 调用 getAbsoluteMotion() 然后添加到速度
    if (forward != 0.0f || strafe != 0.0f) {
        // MC坐标系: yaw单位是度，转换为弧度
        // MC中: yaw=0 看向 -Z, yaw=90 看向 +X
        f32 yawRad = m_yaw * math::DEG_TO_RAD;
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        // MC的getAbsoluteMotion公式
        // moveX = strafe * cosYaw - forward * sinYaw
        // moveZ = forward * cosYaw + strafe * sinYaw
        f32 moveX = strafe * cosYaw - forward * sinYaw;
        f32 moveZ = forward * cosYaw + strafe * sinYaw;

        // 归一化
        f32 length = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (length > 0.0f) {
            moveX /= length;
            moveZ /= length;
        }

        // 关键：MC中是**添加**到速度，而不是替换！
        // 参考 MC Entity.moveRelative() line 1168:
        //   this.setMotion(this.getMotion().add(vector3d));
        m_velocity.x += moveX * speedFactor;
        m_velocity.z += moveZ * speedFactor;
    }
    // 注意：没有输入时不重置速度，让阻力系统自然减速
    // 这样更符合MC的行为（速度会逐渐衰减而不是立即停止）

    // 处理飞行上升/下降
    // 参考 MC ClientPlayerEntity.livingTick() lines 788-801:
    // if (this.abilities.isFlying && this.isCurrentViewEntity()) {
    //     int j = 0;
    //     if (this.movementInput.sneaking) --j;
    //     if (this.movementInput.jump) ++j;
    //     if (j != 0) {
    //         this.setMotion(this.getMotion().add(0.0D, (double)((float)j * this.abilities.getFlySpeed() * 3.0F), 0.0D));
    //     }
    // }
    // 关键：飞行上升/下降速度是 flySpeed * 3.0，而且是**添加**到Y速度
    if (m_abilities.flying) {
        i32 verticalInput = 0;
        if (jumping) {
            verticalInput += 1;
        }
        if (sneaking) {
            verticalInput -= 1;
        }
        if (verticalInput != 0) {
            // 飞行时上升/下降速度 = flySpeed * 3.0
            // 如果冲刺则再乘以2
            f32 verticalSpeed = m_abilities.flySpeed * 3.0f * (m_isSprinting ? 2.0f : 1.0f);
            m_velocity.y += static_cast<f32>(verticalInput) * verticalSpeed;
        }
        // 注意：飞行时Y速度的衰减在updatePhysics中处理（0.6倍）
    } else {
        // 非飞行模式下处理跳跃
        if (jumping && m_onGround && m_jumpTicks == 0) {
            jump();
        }
    }
}

void Player::handleWaterMovement(f32 forward, f32 strafe, bool jumping, bool sneaking) {
    // 参考 MC 1.16.5 LivingEntity.travel() 水中分支
    // 关键逻辑：
    // 1. 水中重力减弱（浮力）
    // 2. 水中阻力
    // 3. 深度守卫附魔增加速度
    // 4. 海豚的恩惠减少阻力
    // 5. 水中移动

    // 基础水中游泳速度
    f32 swimSpeed = physics::SWIM_SPEED_BASE;

    // 冲刺时增加速度
    if (m_isSprinting) {
        swimSpeed *= physics::SWIM_SPEED_SPRINT_MULTIPLIER;
    }

    // 深度守卫附魔加成
    // MC 1.16.5: float f7 = (float)EnchantmentHelper.getDepthStriderModifier(this);
    // TODO: 实现附魔系统后获取深度守卫等级
    i32 depthStriderLevel = 0;  // EnchantmentHelper::getDepthStriderModifier(this);
    if (depthStriderLevel > 0) {
        // 深度守卫效果：
        // 1. 增加游泳速度
        swimSpeed += depthStriderLevel * physics::DEPTH_STRIDER_SPEED_BONUS;
        // 2. 速度上限
        swimSpeed = std::min(swimSpeed, 0.1f);
    }

    // 海豚的恩惠药水效果
    // MC 1.16.5: if (this.isPotionActive(Effects.DOLPHINS_GRACE))
    bool hasDolphinsGrace = hasEffect(entity::effect::EffectType::DolphinsGrace);

    // 水中阻力
    f32 waterDrag = m_isSprinting ? physics::WATER_DRAG_SPRINT : physics::WATER_DRAG;

    // 深度守卫对阻力的影响
    // MC: f5 += (0.54600006F - f5) * f7 / 3.0F
    if (depthStriderLevel > 0) {
        waterDrag += (physics::DEPTH_STRIDER_MAX_DRAG - waterDrag) *
                     static_cast<f32>(depthStriderLevel) / 3.0f;
    }

    // 海豚的恩惠大幅减少水中阻力
    // MC: if (this.isPotionActive(Effects.DOLPHINS_GRACE)) { f5 = 0.96F; }
    if (hasDolphinsGrace) {
        waterDrag = physics::DOLPHINS_GRACE_WATER_DRAG;
    }

    // 根据朝向计算水平移动方向
    if (forward != 0.0f || strafe != 0.0f) {
        f32 yawRad = m_yaw * math::DEG_TO_RAD;
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        // MC的getAbsoluteMotion公式
        f32 moveX = strafe * cosYaw - forward * sinYaw;
        f32 moveZ = forward * cosYaw + strafe * sinYaw;

        // 归一化
        f32 length = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (length > 0.0f) {
            moveX /= length;
            moveZ /= length;
        }

        // 添加到速度
        m_velocity.x += moveX * swimSpeed;
        m_velocity.z += moveZ * swimSpeed;
    }

    // 垂直移动（跳跃向上，潜行向下）
    if (jumping) {
        // 向上游泳
        // MC: this.setMotion(this.getMotion().add(0.0D, 0.04D, 0.0D));
        m_velocity.y += physics::SWIM_UP_SPEED;
    } else if (sneaking) {
        // 向下潜
        m_velocity.y -= physics::SWIM_DOWN_SPEED;
    }

    // 应用水中阻力
    // MC: this.setMotion(this.getMotion().mul((double)f5, (double)0.8F, (double)f5));
    // 垂直方向阻力固定为 0.8
    m_velocity.x *= waterDrag;
    m_velocity.y *= 0.8f;  // 水中垂直阻力固定为 0.8
    m_velocity.z *= waterDrag;

    // 应用水中的"浮力"效果
    // MC 1.16.5: func_233626_a_() - 浮力计算
    // 重力减少到 1/16 (0.08 / 16 = 0.005)
    if (!m_abilities.flying && !m_noGravity) {
        f32 gravity = physics::GRAVITY;
        f32 buoyancy = gravity / 16.0f;  // MC 标准：重力 / 16

        // 下落时应用浮力
        if (m_velocity.y < 0.0f) {
            m_velocity.y += buoyancy;
        }
    }

    // 碰撞到墙后尝试上跳（爬出水面的行为）
    // MC: if (this.collidedHorizontally && this.isOffsetPositionInLiquid(...))
    if (m_collidedHorizontally && !m_onGround && m_physicsEngine) {
        // 尝试向上跳
        m_velocity.y = physics::WATER_WALL_JUMP_VELOCITY;
    }

    // 重置过小的速度
    clampMotion();
}

void Player::handleLavaMovement(f32 forward, f32 strafe, bool jumping, bool sneaking) {
    // 参考 MC 1.16.5 LivingEntity.travel() 岩浆分支
    // 岩浆中移动比水中更慢

    // 岩浆中基础移动速度
    f32 lavaSpeed = physics::LAVA_SWIM_SPEED;

    // 根据朝向计算移动方向
    if (forward != 0.0f || strafe != 0.0f) {
        f32 yawRad = m_yaw * math::DEG_TO_RAD;
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        f32 moveX = strafe * cosYaw - forward * sinYaw;
        f32 moveZ = forward * cosYaw + strafe * sinYaw;

        f32 length = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (length > 0.0f) {
            moveX /= length;
            moveZ /= length;
        }

        m_velocity.x += moveX * lavaSpeed;
        m_velocity.z += moveZ * lavaSpeed;
    }

    // 垂直移动（岩浆中也能向上游，但更慢）
    if (jumping) {
        m_velocity.y += physics::SWIM_UP_SPEED * 0.5f;  // 岩浆中向上游更慢
    } else if (sneaking) {
        m_velocity.y -= physics::SWIM_DOWN_SPEED * 0.5f;
    }

    // 岩浆阻力（比水更大）
    m_velocity.x *= physics::LAVA_DRAG;
    m_velocity.y *= physics::LAVA_DRAG;
    m_velocity.z *= physics::LAVA_DRAG;

    // 岩浆中重力（减弱）
    if (!m_abilities.flying) {
        if (m_velocity.y < 0.0f && !sneaking) {
            m_velocity.y += physics::LAVA_GRAVITY;
        }
    }

    clampMotion();
}

void Player::jump() {
    if (m_onGround && m_jumpTicks == 0) {
        m_velocity.y = physics::JUMP_VELOCITY;
        m_onGround = false;
        m_jumpTicks = JUMP_COOLDOWN; // 设置跳跃冷却

        // 跳跃消耗饥饿值
        // 参考 MC 1.16.5: PlayerEntity.jump() 调用 addExhaustion
        if (m_isSprinting) {
            addExhaustion(EXHAUSTION_SPRINT_JUMP);
        } else {
            addExhaustion(EXHAUSTION_JUMP);
        }
    }
}

/**
 * @brief 重置过小的速度为零
 *
 * 参考MC: LivingEntity.aiStep()
 * if (Math.abs(motion.x) < 0.003) motion.x = 0;
 * if (Math.abs(motion.y) < 0.003) motion.y = 0;
 * if (Math.abs(motion.z) < 0.003) motion.z = 0;
 */
void Player::clampMotion() {
    if (std::abs(m_velocity.x) < MOTION_THRESHOLD) m_velocity.x = 0.0f;
    if (std::abs(m_velocity.y) < MOTION_THRESHOLD) m_velocity.y = 0.0f;
    if (std::abs(m_velocity.z) < MOTION_THRESHOLD) m_velocity.z = 0.0f;
}

/**
 * @brief 潜行时检查是否可以移动到边缘
 *
 * 参考MC: PlayerEntity.maybeBackOffFromEdge()
 * 当玩家潜行时，检查前方是否有方块支撑，防止掉落。
 *
 * @param movement 期望移动向量
 * @return 修正后的移动向量
 */
Vector3 Player::maybeBackOffFromEdge(const Vector3& movement) const {
    // 只在潜行时检测
    if (!m_isSneaking) {
        return movement;
    }

    // 如果没有物理引擎或向上移动，不检测
    if (!m_physicsEngine) {
        return movement;
    }

    // 只检测水平移动
    if (movement.x == 0.0f && movement.z == 0.0f) {
        return movement;
    }

    // 获取当前碰撞箱
    AxisAlignedBB box = boundingBox();

    // 计算移动后的位置
    f32 newX = m_position.x + movement.x;
    f32 newZ = m_position.z + movement.z;

    // 检查移动后的位置下方是否有方块
    // 向下检测一小段距离
    AxisAlignedBB testBox = AxisAlignedBB(
        newX - PLAYER_WIDTH / 2.0f,
        m_position.y - SNEAK_EDGE_DISTANCE,
        newZ - PLAYER_WIDTH / 2.0f,
        newX + PLAYER_WIDTH / 2.0f,
        m_position.y,
        newZ + PLAYER_WIDTH / 2.0f
    );

    // 检查是否有碰撞
    std::vector<AxisAlignedBB> boxes;
    m_physicsEngine->collectCollisionBoxes(testBox, boxes);

    if (boxes.empty()) {
        // 没有支撑，阻止移动
        return Vector3(0.0f, movement.y, 0.0f);
    }

    return movement;
}

void Player::updatePhysics() {
    // 0. 更新跳跃冷却（客户端物理每帧都会调用）
    // 之前仅在tick()中减少，客户端未调用tick()会导致只能跳一次。
    if (m_jumpTicks > 0) {
        m_jumpTicks--;
    }

    // 0.1 更新自动跳跃冷却
    m_autoJump.tick();

    // 1. 重置过小的速度（MC: LivingEntity.aiStep）
    clampMotion();

    // 刷新环境状态，确保后续判断使用当前位置。
    updateEnvironmentState();
    checkOnGround();

    // 水中和岩浆中的物理在 handleWaterMovement/handleLavaMovement 中已处理
    // 这里只处理地面和空中的物理
    if ((isInWater() || isInLava()) && !m_abilities.flying) {
        // 水中/岩浆中的移动和碰撞
        Vector3 movement(m_velocity.x, m_velocity.y, m_velocity.z);
        if (m_physicsEngine && (movement.x != 0.0f || movement.y != 0.0f || movement.z != 0.0f)) {
            moveWithCollision(movement.x, movement.y, movement.z);
        }
    }

    if (!(isInWater() || isInLava()) || m_abilities.flying) {
        // 2. 应用重力（飞行时不应用重力）
        // MC 1.16.5: 重力始终应用，碰撞检测会处理停止
        // 参考 Entity.move() 和 LivingEntity.travel()
        if (!m_abilities.flying && !hasNoGravity()) {
            m_velocity.y -= physics::GRAVITY;
        }

        // 3. 应用阻力
        // 参考MC: LivingEntity.travel() 和 PlayerEntity.travel()
        // 飞行时阻力处理不同：Y方向用0.6，水平方向用0.91
        if (m_abilities.flying) {
            // 飞行模式：参考 MC PlayerEntity.travel() line 1451
            // this.setMotion(vector3d.x, d5 * 0.6D, vector3d.z);
            // 其中 d5 是旅行前的Y速度
            m_velocity.x *= physics::DRAG_GROUND;  // 0.91 (水平阻力)
            m_velocity.y *= 0.6f;                    // 飞行Y阻力 (MC: 0.6D)
            m_velocity.z *= physics::DRAG_GROUND;  // 0.91 (水平阻力)
        } else {
            // 非飞行模式：应用空气阻力
            m_velocity.x *= physics::DRAG_AIR;     // 0.98
            m_velocity.y *= physics::DRAG_AIR;     // 0.98
            m_velocity.z *= physics::DRAG_AIR;     // 0.98
        }

        // 4. 如果在地面，停止Y方向速度（防止下落速度累积）
        // 飞行模式下不处理
        if (!m_abilities.flying && m_onGround && m_velocity.y < 0.0f) {
            m_velocity.y = 0.0f;
        }

        // 5. 潜行边缘检测（飞行时不检测）
        Vector3 movement(m_velocity.x, m_velocity.y, m_velocity.z);
        if (m_isSneaking && !m_abilities.flying) {
            movement = maybeBackOffFromEdge(movement);
        }

        // 6. 记录移动前的位置（用于自动跳跃检测）
        Vector3 prevPos = m_position;

        // 7. 使用碰撞检测移动
        if (m_physicsEngine && (movement.x != 0.0f || movement.y != 0.0f || movement.z != 0.0f)) {
            Vector3 actualMovement = moveWithCollision(movement.x, movement.y, movement.z);

            // 8. 碰撞后重置速度（参考MC: Entity.move）
            // 飞行模式下碰撞时不重置水平速度（可以穿透方块边缘的感觉）
            if (!m_abilities.flying) {
                if (m_collidedHorizontally) {
                    m_velocity.x = 0.0f;
                    m_velocity.z = 0.0f;
                }
            }
            if (m_collidedVertically) {
                m_velocity.y = 0.0f;
            }
        } else if (!m_physicsEngine && (movement.x != 0.0f || movement.y != 0.0f || movement.z != 0.0f)) {
            move(movement.x, movement.y, movement.z);
        }

        // 9. 自动跳跃检测（在移动后）
        // MC 源码在 ClientPlayerEntity.move() 方法末尾调用 updateAutoJump
        if (m_autoJump.isEnabled() && !m_abilities.flying && m_onGround && !m_isSneaking) {
            // 计算实际移动距离
            Vector2 actualMovement(m_position.x - prevPos.x, m_position.z - prevPos.z);
            f32 moveDistSq = actualMovement.x * actualMovement.x + actualMovement.y * actualMovement.y;

            // 只有在确实移动了才检测
            if (moveDistSq > 0.0001f && m_physicsEngine) {
                auto result = m_autoJump.check(*this, *m_physicsEngine, actualMovement);
                if (result.shouldJump) {
                    jump();
                }
            }
        }
    }

    // 更新本地渲染与环境状态缓存
    updateEnvironmentState();
    updateSwimming();
    updateAirSupply();
    updateMoveDistance();

    // 10. 再次重置过小的速度
    clampMotion();
}

void Player::applyMovementSpeed(f32& speed, bool sneaking) const {
    if (m_abilities.flying) {
        speed = m_abilities.flySpeed;
    } else {
        speed = m_abilities.walkSpeed;
        if (m_isSprinting) {
            speed *= 1.3f;
        }
        if (sneaking) {
            speed *= 0.3f;
        }
    }
}

network::PlayerPosition Player::playerPosition() const {
    return network::PlayerPosition(
        static_cast<f64>(m_position.x),
        static_cast<f64>(m_position.y),
        static_cast<f64>(m_position.z),
        m_yaw,
        m_pitch,
        m_onGround
    );
}

i32 Player::armorValue() const {
    // TODO: 当护甲物品实现 getArmorValue() 后，计算总护甲值
    // 目前返回占位值0
    // 参考 MC: PlayerEntity.getTotalArmorValue()
    // 护甲值 = 头盔护甲值 + 胸甲护甲值 + 护腿护甲值 + 靴子护甲值
    return 0;
}

void Player::setCreativeModeInventory() {
    fillCreativeModeInventory(m_inventory);
}

void Player::respawn() {
    m_health = m_maxHealth;
    m_foodStats.setFoodLevel(20);
    m_foodStats.setSaturationLevel(5.0f);
    m_foodStats.setFoodTimer(0);
    m_deathTime = 0;
    m_hurtTime = 0;
    m_isSleeping = false;
    m_jumpTicks = 0;
    sleepTimer = 0;
    setPose(EntityPose::Standing);

    // 重置经验
    m_experienceManager->reset();
}

void Player::serialize(network::PacketSerializer& ser) const {
    // 基本信息
    ser.writeU64(m_playerId);
    ser.writeString(m_username);

    // 位置
    ser.writeF64(static_cast<f64>(m_position.x));
    ser.writeF64(static_cast<f64>(m_position.y));
    ser.writeF64(static_cast<f64>(m_position.z));
    ser.writeF32(m_yaw);
    ser.writeF32(m_pitch);

    // 状态
    ser.writeF32(m_health);
    ser.writeI32(static_cast<i32>(m_gameMode));
    ser.writeBool(m_onGround);
    ser.writeBool(m_isSprinting);
    ser.writeBool(m_isSneaking);

    // 饥饿
    m_foodStats.serialize(ser);

    // 经验（从 ExperienceManager 获取）
    ser.writeI32(m_experienceManager->getLevel());
    ser.writeF32(m_experienceManager->getProgress());
    ser.writeI32(m_experienceManager->getTotalExperience());
}

Result<std::unique_ptr<Player>> Player::deserialize(network::PacketDeserializer& deser) {
    // 读取基本信息
    auto idResult = deser.readU64();
    if (idResult.failed()) return idResult.error();
    PlayerId playerId = idResult.value();

    auto usernameResult = deser.readString();
    if (usernameResult.failed()) return usernameResult.error();
    String username = usernameResult.value();

    auto player = std::make_unique<Player>(static_cast<EntityId>(playerId), username);
    player->m_playerId = playerId;

    // 位置
    auto xResult = deser.readF64();
    if (xResult.failed()) return xResult.error();

    auto yResult = deser.readF64();
    if (yResult.failed()) return yResult.error();

    auto zResult = deser.readF64();
    if (zResult.failed()) return zResult.error();

    // 网络协议使用 f64，内部使用 f32
    player->setPosition(static_cast<f32>(xResult.value()),
                        static_cast<f32>(yResult.value()),
                        static_cast<f32>(zResult.value()));

    auto yawResult = deser.readF32();
    if (yawResult.failed()) return yawResult.error();
    player->m_yaw = yawResult.value();

    auto pitchResult = deser.readF32();
    if (pitchResult.failed()) return pitchResult.error();
    player->m_pitch = pitchResult.value();

    // 状态
    auto healthResult = deser.readF32();
    if (healthResult.failed()) return healthResult.error();
    player->m_health = healthResult.value();

    auto gameModeResult = deser.readI32();
    if (gameModeResult.failed()) return gameModeResult.error();
    player->m_gameMode = static_cast<GameMode>(gameModeResult.value());

    auto groundResult = deser.readBool();
    if (groundResult.failed()) return groundResult.error();
    player->m_onGround = groundResult.value();

    auto sprintResult = deser.readBool();
    if (sprintResult.failed()) return sprintResult.error();
    player->m_isSprinting = sprintResult.value();

    auto sneakResult = deser.readBool();
    if (sneakResult.failed()) return sneakResult.error();
    player->setSneaking(sneakResult.value());

    // 饥饿
    auto foodResult = FoodStats::deserialize(deser);
    if (foodResult.failed()) return foodResult.error();
    player->m_foodStats = foodResult.value();

    // 经验（设置到 ExperienceManager）
    auto levelResult = deser.readI32();
    if (levelResult.failed()) return levelResult.error();

    auto progressResult = deser.readF32();
    if (progressResult.failed()) return progressResult.error();

    auto totalResult = deser.readI32();
    if (totalResult.failed()) return totalResult.error();

    player->m_experienceManager->setExperience(
        levelResult.value(),
        progressResult.value(),
        totalResult.value()
    );

    return std::move(player);
}

// ============================================================================
// getHeldItem 实现
// ============================================================================

ItemStack Player::getHeldItem(Hand hand) const {
    if (hand == Hand::MainHand) {
        return m_inventory.getSelectedStack();
    } else {
        return m_inventory.getOffhandItem();
    }
}

ItemStack& Player::getHeldItem(Hand hand) {
    if (hand == Hand::MainHand) {
        return m_inventory.getSelectedStackRef();
    } else {
        return m_inventory.getOffhandItemRef();
    }
}

// ============================================================================
// 效果系统实现
// Player 不继承 LivingEntity，所以使用简化的效果管理
// ============================================================================

bool Player::addEffect(const entity::effect::EffectInstance& effect) {
    // 查找是否已存在相同类型的效果
    for (auto& existing : m_effects) {
        if (existing.type() == effect.type()) {
            // 已存在，尝试合并
            return existing.merge(effect);
        }
    }

    // 新效果，添加到列表
    m_effects.push_back(effect);
    return true;
}

void Player::removeEffect(entity::effect::EffectType type) {
    m_effects.erase(
        std::remove_if(m_effects.begin(), m_effects.end(),
            [type](const entity::effect::EffectInstance& e) { return e.type() == type; }),
        m_effects.end());
}

void Player::removeAllEffects() {
    m_effects.clear();
}

bool Player::hasEffect(entity::effect::EffectType type) const {
    for (const auto& effect : m_effects) {
        if (effect.type() == type) {
            return true;
        }
    }
    return false;
}

const entity::effect::EffectInstance* Player::getEffect(entity::effect::EffectType type) const {
    for (const auto& effect : m_effects) {
        if (effect.type() == type) {
            return &effect;
        }
    }
    return nullptr;
}

// ============================================================================
// 水中物理和游泳实现
// ============================================================================

bool Player::isActualSwimming() const {
    // 游泳条件（MC 1.16.5: LivingEntity.isActualySwimming()）
    // 需要: 眼睛在水中 && 身体在水中 && 不在飞行模式
    // MC: return this.eyesInWater && this.isInWater() && !this.abilities.isFlying;
    return areEyesInWater() && isInWater() && !m_abilities.flying;
}

void Player::updateSwimming() {
    // 更新游泳动画
    m_prevSwimAnimation = m_swimAnimation;

    bool isSwimmingNow = isActualSwimming();

    // 平滑过渡游泳动画
    // MC 1.16.5: swimAnimation 增加/减少 0.09f
    if (isSwimmingNow) {
        m_swimAnimation = std::min(1.0f, m_swimAnimation + 0.09f);
    } else {
        m_swimAnimation = std::max(0.0f, m_swimAnimation - 0.09f);
    }

    // 更新游泳状态
    setSwimming(isSwimmingNow);
}

void Player::updatePose() {
    // MC 1.16.5: PlayerEntity.updatePose()
    // 每帧自动判断正确姿态

    // 检查是否有足够的游泳空间（用于姿态切换的后备检查）
    // isPoseClear 在 MC 中检查指定姿态的碰撞箱是否与方块冲突
    auto isPoseClear = [this](EntityPose pose) -> bool {
        return canFitPose(pose);
    };

    // 如果姿态被禁止，不进行自动更新
    // MC: if (this.forcedPose != null) { this.setPose(this.forcedPose); return; }
    // 目前没有实现 forcedPose，直接进行姿态判断

    // 只有在游泳空间足够时才允许姿态切换
    if (!isPoseClear(EntityPose::Swimming)) {
        return;
    }

    // 按优先级判断目标姿态
    EntityPose targetPose = EntityPose::Standing;

    // 检查是否是旁观者模式
    bool isSpectatorMode = entity::GameModeUtils::isSpectator(m_gameMode);
    // 检查是否正在骑乘
    bool isRidingVehicle = isRiding();

    // 1. 鞘翅飞行（优先级最高）
    // TODO: 实现 isElytraFlying()
    // if (isElytraFlying()) {
    //     targetPose = EntityPose::FallFlying;
    // }
    // else

    // 2. 睡眠
    if (m_isSleeping) {
        targetPose = EntityPose::Sleeping;
    }
    // 3. 游泳
    else if (m_isSwimming) {
        targetPose = EntityPose::Swimming;
    }
    // 4. 三叉戟激流攻击
    // TODO: 实现 isSpinAttacking()
    // else if (isSpinAttacking()) {
    //     targetPose = EntityPose::SpinAttack;
    // }
    // 5. 潜行（非飞行模式）
    else if (m_isSneaking && !m_abilities.flying) {
        targetPose = EntityPose::Crouching;
    }
    // 6. 默认站立
    else {
        targetPose = EntityPose::Standing;
    }

    // 检查目标姿态是否可以容纳
    // MC: if (!this.isSpectator() && !this.isPassenger() && !this.isPoseClear(pose)) { ... }
    if (!isSpectatorMode && !isRidingVehicle && !isPoseClear(targetPose)) {
        // 目标姿态无法容纳，尝试后备姿态
        if (isPoseClear(EntityPose::Crouching)) {
            targetPose = EntityPose::Crouching;
        } else {
            targetPose = EntityPose::Swimming;
        }
    }

    // 设置姿态
    setPose(targetPose);
}

void Player::swimUp() {
    // 水中向上游泳
    if (isInWater() && !m_abilities.flying) {
        m_velocity.y += physics::SWIM_UP_SPEED;
    }
}

void Player::updateAirSupply() {
    // 参考 MC 1.16.5 LivingEntity.baseTick() 空气处理

    bool inWater = isInWater();
    bool inLava = isInLava();

    // 检查是否能水下呼吸
    // MC 1.16.5: EffectUtils.canBreatheUnderwater()
    // - 水下呼吸效果 (WaterBreathing)
    // - 潮涌能量效果 (ConduitPower)
    bool canBreatheUnderwater = hasEffect(entity::effect::EffectType::WaterBreathing) ||
                                  hasEffect(entity::effect::EffectType::ConduitPower);

    // 创造模式或旁观者模式下不消耗空气
    bool invulnerableToDrowning = m_abilities.invulnerable || canBreatheUnderwater;

    // 检测入水/出水状态变化（需要在空气处理之前）
    bool justEnteredWater = inWater && !m_wasInWater;
    bool justExitedWater = !inWater && m_wasInWater;

    if (inWater || inLava) {
        // 在水或岩浆中
        if (!invulnerableToDrowning) {
            // 空气消耗
            // MC 1.16.5: decreaseAirSupply() 会考虑水下呼吸附魔
            // 有水下呼吸附魔时，有概率不消耗空气
            // TODO: 实现水下呼吸附魔（Respiration enchantment）
            m_airSupply--;

            // 空气耗尽到 -20 时触发溺水
            // MC 1.16.5: getAir() == -20 时重置为 0 并造成伤害
            if (m_airSupply <= -20) {
                m_airSupply = 0;

                // 溺水伤害
                m_drownDamageTimer++;
                if (m_drownDamageTimer >= physics::DROWN_DAMAGE_INTERVAL) {
                    m_drownDamageTimer = 0;
                    // 造成溺水伤害
                    // MC 1.16.5: attackEntityFrom(DamageSource.DROWN, 2.0F)
                    damage(physics::DROWN_DAMAGE_AMOUNT);
                }

                // TODO: 生成气泡粒子
                // MC 1.16.5: for (int i = 0; i < 8; ++i) {
                //     world.addParticle(ParticleTypes.BUBBLE, ...);
                // }
            }
        }

        // 入水溅水声
        if (justEnteredWater) {
            // MC 1.16.5: this.world.playSound(null, this.getPosX(), this.getPosY(), this.getPosZ(),
            //              SoundEvents.ENTITY_PLAYER_SPLASH, this.getSoundCategory(), 1.0F, 1.0F);
            playSound(SoundEvents::ENTITY_PLAYER_SPLASH, 1.0f, 1.0f);
            // TODO: 触发入水粒子效果（水花飞溅）
        }
    } else {
        // 不在水中，恢复空气
        // MC 1.16.5: determineNextAir() 每tick恢复4点
        m_airSupply = std::min(m_airSupply + 4, physics::DEFAULT_MAX_AIR);
        m_drownDamageTimer = 0;

        // 出水声音（可选，MC 中没有专门的出水声）
        if (justExitedWater) {
            // MC 中出水没有特定声音，但可以添加轻微的声音效果
        }
    }

    // 更新上一帧状态
    m_wasInWater = inWater;
}

void Player::updateMoveDistance() {
    // 保存上一帧的距离
    m_prevMoveDistanceWalked = m_moveDistanceWalked;
    m_prevMoveDistanceSwam = m_moveDistanceSwam;

    // 只累计上次采样后的增量，避免 tick 和物理更新重复统计同一段位移
    f32 dx = m_position.x - m_moveDistanceSamplePosition.x;
    f32 dy = m_position.y - m_moveDistanceSamplePosition.y;
    f32 dz = m_position.z - m_moveDistanceSamplePosition.z;
    f32 distance = std::sqrt(dx * dx + dz * dz);  // 水平距离

    // 重置声音触发标志
    m_shouldPlayStepSound = false;
    m_shouldPlaySwimSound = false;

    // 根据 MC 的逻辑：
    // distanceWalkedOnStepModified = 距离 * 0.6 用于脚步声触发
    // 参考实体在地面且不在流体中时触发脚步声
    f32 stepDistance;

    if (isInWater()) {
        // 游泳距离包括垂直移动
        f32 swimDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
        m_moveDistanceSwam += swimDistance;

        // 游泳声音触发
        // MC: distanceWalkedOnStepModified += sqrt(motion.x^2 * 0.2 + motion.y^2 + motion.z^2 * 0.2) * 0.35
        stepDistance = std::sqrt(dx * dx * 0.2f + dy * dy + dz * dz * 0.2f) * 0.35f;
    } else {
        m_moveDistanceWalked += distance;
        // 脚步声距离乘以 0.6
        stepDistance = distance * 0.6f;
    }

    m_distanceWalkedOnStep += stepDistance;

    // 检查是否需要播放脚步声/游泳声
    // 参考 MC: if (distanceWalkedOnStepModified > nextStepDistance && !blockState.isAir())
    if (m_distanceWalkedOnStep > m_nextStepDistance && m_onGround) {
        m_nextStepDistance = std::floor(m_distanceWalkedOnStep) + 1.0f;

        if (isInWater()) {
            // 游泳声音量基于速度
            m_swimSoundVolume = std::min(1.0f, stepDistance / 0.35f);
            m_shouldPlaySwimSound = true;
        } else {
            // 记录脚下方块位置（用于获取正确的声音类型）
            m_stepSoundPos = BlockPos(
                static_cast<i32>(std::floor(m_position.x)),
                static_cast<i32>(std::floor(m_position.y - 0.2f)),  // 脚底位置
                static_cast<i32>(std::floor(m_position.z))
            );
            m_shouldPlayStepSound = true;
        }
    }

    m_moveDistanceSamplePosition = m_position;

    // 饥饿消耗（基于移动距离）
    // 只有生存模式和冒险模式才消耗饥饿
    if (m_gameMode == GameMode::Survival || m_gameMode == GameMode::Adventure) {
        if (isInWater()) {
            // 游泳消耗：每米 0.01
            f32 swimDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (swimDistance > 0.0f) {
                addExhaustion(EXHAUSTION_SWIM_PER_METER * swimDistance);
            }
        } else if (m_isSprinting && m_onGround) {
            // 疾跑消耗：每米 0.1
            if (distance > 0.0f) {
                addExhaustion(EXHAUSTION_SPRINT_PER_METER * distance);
            }
        }
        // 潜行、普通行走、攀爬不消耗饥饿
    }
}

void Player::playStepSound() {
    // 由客户端调用，在正确的位置播放声音
    // 此处仅标记需要播放，客户端负责实际播放
}

void Player::playSwimSound(f32 volume) {
    // 由客户端调用
    m_swimSoundVolume = volume;
    m_shouldPlaySwimSound = true;
}

void Player::addExhaustion(f32 exhaustion) {
    // 只有生存模式和冒险模式才消耗饥饿
    if (m_gameMode == GameMode::Survival || m_gameMode == GameMode::Adventure) {
        m_foodStats.addExhaustion(exhaustion);
    }
}

} // namespace mc
