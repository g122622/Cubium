#include "Player.hpp"
#include "GameModeUtils.hpp"
#include "../../inventory/Slot.hpp"
#include "../../experience/ExperienceManager.hpp"
#include "../../attribute/EntityDefaultAttributes.hpp"
#include "../../combat/PlayerAttackHelper.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../../physics/PhysicsEngine.hpp"
#include "../../../physics/PhysicsConstants.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../inventory/CreativeInventory.hpp"
#include "../../experience/ExperienceDropHandler.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockSoundType.hpp"
#include "../../../item/enchantment/EnchantmentHelper.hpp"
#include "../../../item/enchantment/enchantments/AllEnchantments.hpp"
#include "../../../item/items/tool/SwordItem.hpp"
#include "../../../item/core/ActionResult.hpp"
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

Player::Player(EntityId id, const std::string& username)
    : LivingEntity(LegacyEntityType::Player, id)
    , m_username(username)
    , m_experienceManager(std::make_unique<entity::experience::ExperienceManager>(*this))
{
    // 注册玩家属性
    registerAttributes();

    // 生成随机XP seed
    math::Random rng(static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    m_experienceManager->resetXpSeed(rng);
}

Player::~Player() = default;

void Player::setPosition(f32 x, f32 y, f32 z) {
    Entity::setPosition(x, y, z);
    snapshotInterpolationState();

    // 外部改坐标时同步复位步距采样，避免沿用旧位移或旧脚步阈值
    m_moveDistanceSamplePosition = m_position;
    m_moveDistanceWalked = 0.0f;
    m_prevMoveDistanceWalked = 0.0f;
    m_moveDistanceSwam = 0.0f;
    m_prevMoveDistanceSwam = 0.0f;
    m_cameraYaw = 0.0f;
    m_prevCameraYaw = 0.0f;
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

    // 同步移动速度到属性系统
    // PlayerAbilities 是配置层，属性系统是计算层
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED,
                               static_cast<f64>(m_abilities.walkSpeed));
}

// ============================================================================
// 生命值管理（覆盖 LivingEntity 方法）
// ============================================================================

void Player::setHealth(f32 health) {
    // 直接调用父类方法
    LivingEntity::setHealth(health);
}

void Player::heal(f32 amount) {
    // 直接调用父类方法
    LivingEntity::heal(amount);
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
    LivingEntity::tick();

    // 更新 XP 冷却
    if (m_xpCooldown > 0) {
        m_xpCooldown--;
    }

    // 更新攻击冷却
    m_ticksSinceLastAttack++;

    // 更新物品冷却追踪器
    // 参考 MC 1.16.5 PlayerEntity.tick() -> cooldownTracker.tick()
    m_cooldownTracker.tick();

    // 世界边界伤害检测
    // 参考 MC 1.16.5 LivingEntity.baseTick() 第306-318行
    // 只有玩家会受到边界伤害（flag = this instanceof PlayerEntity）
    if (m_world != nullptr && !isSpectator() && !m_abilities.invulnerable) {
        const auto& border = m_world->worldBorder();

        // 检查玩家碰撞箱是否与边界相交
        if (!border.intersects(boundingBox())) {
            // 玩家在边界外，计算伤害
            // MC: distance = getClosestDistance(entity) + damageBuffer
            double distance = border.getClosestDistance(boundingBox()) + border.getDamageBuffer();

            // 距离为负表示超出缓冲区
            if (distance < 0.0 && border.getDamagePerBlock() > 0.0) {
                // 计算伤害：max(1, floor(-distance * damagePerBlock))
                i32 damage = std::max(1, static_cast<i32>(std::floor(-distance * border.getDamagePerBlock())));

                // 应用 IN_WALL 伤害
                auto damageSource = DamageSources::inWall();
                hurt(damageSource, static_cast<f32>(damage));
            }
        }
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

    // 检测与附近实体的碰撞（拾取物品、箭矢等）
    // 参考 MC 1.16.5 PlayerEntity.tick() 第531-547行
    checkEntityCollisions();
}

void Player::checkEntityCollisions() {
    // 参考 MC 1.16.5 PlayerEntity.tick() 第531-547行
    // 只在存活且非观察者模式时检测碰撞
    if (!isAlive() || isSpectator()) {
        return;
    }

    if (m_world == nullptr) {
        return;
    }

    // 创建搜索盒：玩家碰撞箱扩展1格（水平和垂直）
    // MC 1.16.5: this.getBoundingBox().grow(1.0D, 0.5D, 1.0D)
    AxisAlignedBB searchBox = boundingBox().expand(1.0f, 0.5f, 1.0f);

    // 获取搜索盒内的所有实体
    std::vector<Entity*> nearbyEntities = m_world->getEntitiesInAABB(searchBox, this);

    for (Entity* entity : nearbyEntities) {
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }
        // 调用实体的碰撞回调
        entity->onCollideWithPlayer(*this);
    }
}

bool Player::tickPortal() {
    // 玩家需要 80 tick (4秒) 在传送门中才能传送
    // 创造模式（无敌状态）只需要 1 tick
    // 参考 MC 1.16.5 PlayerEntity.tick()

    if (!m_inPortal) {
        if (m_portalTime > 0) {
            m_portalTime = std::max(0, m_portalTime - 4);
        }
        return false;
    }

    // 无论是否传送，都重置 inPortal
    m_inPortal = false;

    if (!canTeleport()) {
        return false;
    }

    // 递增计时并检查阈值
    m_portalTime++;

    const i32 maxPortalTime = getMaxInPortalTime();
    if (m_portalTime >= maxPortalTime) {
        m_portalTime = maxPortalTime;
        return true;
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
    m_inputForward = forward;
    m_inputStrafe = strafe;
    m_inputJumping = jumping;
    m_inputSneaking = sneaking;
    m_isJumping = jumping;
}

void Player::applyCachedMovementInput(f32 groundSlipperiness) {
    const f32 forward = m_inputForward;
    const f32 strafe = m_inputStrafe;
    const bool jumping = m_inputJumping;
    const bool sneaking = m_inputSneaking;

    // 更新跳跃状态（用于动画等）
    m_isJumping = jumping;
    const bool wantsSneaking = sneaking && !m_abilities.flying;
    if (wantsSneaking || m_isSneaking) {
        setSneaking(wantsSneaking);
    }

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
    if (!m_abilities.flying) {
        if (m_onGround) {
            speedFactor = physics::getGroundMoveFactor(static_cast<f32>(getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_abilities.walkSpeed)), groundSlipperiness);
        } else {
            speedFactor = m_isSprinting ? physics::SPRINT_JUMP_MOVEMENT_FACTOR : physics::JUMP_MOVEMENT_FACTOR;
        }
    }
    if (m_isSprinting && m_onGround && !m_abilities.flying) {
        speedFactor *= physics::SPRINT_SPEED_MULTIPLIER;
    }
    if (sneaking && !m_abilities.flying) {
        speedFactor *= physics::SNEAK_SPEED_MULTIPLIER;
    }
    if (m_abilities.flying) {
        speedFactor = m_abilities.flySpeed * (m_isSprinting ? physics::SPRINT_FLY_MULTIPLIER : 1.0f);
    }

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

        m_velocity.x += moveX * speedFactor;
        m_velocity.z += moveZ * speedFactor;
    }

    if (m_abilities.flying) {
        i32 verticalInput = 0;
        if (jumping) {
            verticalInput += 1;
        }
        if (sneaking) {
            verticalInput -= 1;
        }
        if (verticalInput != 0) {
            f32 verticalSpeed = m_abilities.flySpeed * physics::FLY_VERTICAL_INPUT_MULTIPLIER * (m_isSprinting ? physics::SPRINT_FLY_MULTIPLIER : 1.0f);
            m_velocity.y += static_cast<f32>(verticalInput) * verticalSpeed;
        }
    } else {
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
    i32 depthStriderLevel = getDepthStriderLevel();
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
    if (std::abs(m_velocity.x) < physics::MOTION_THRESHOLD) m_velocity.x = 0.0f;
    if (std::abs(m_velocity.y) < physics::MOTION_THRESHOLD) m_velocity.y = 0.0f;
    if (std::abs(m_velocity.z) < physics::MOTION_THRESHOLD) m_velocity.z = 0.0f;
}

f32 Player::groundSlipperiness() const {
    if (!m_onGround || m_world == nullptr) {
        return physics::SLIPPERINESS_DEFAULT;
    }

    BlockPos blockPos(
        static_cast<i32>(std::floor(m_position.x)),
        static_cast<i32>(std::floor(m_boundingBox.minY - 0.001f)),
        static_cast<i32>(std::floor(m_position.z))
    );
    const BlockState* blockState = m_world->getBlockState(blockPos);
    if (blockState == nullptr) {
        return physics::SLIPPERINESS_DEFAULT;
    }
    return blockState->getBlock().getSlipperiness(*blockState, m_world, &blockPos, this);
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
    snapshotInterpolationState();

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

    const f32 tickGroundSlipperiness = groundSlipperiness();
    applyCachedMovementInput(tickGroundSlipperiness);

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
            m_velocity.x *= physics::FLY_HORIZONTAL_DRAG;
            m_velocity.y *= physics::FLY_VERTICAL_DRAG;
            m_velocity.z *= physics::FLY_HORIZONTAL_DRAG;
        } else {
            const f32 horizontalDrag = m_onGround ? tickGroundSlipperiness * physics::DRAG_GROUND : physics::DRAG_GROUND;
            m_velocity.x *= horizontalDrag;
            m_velocity.y *= physics::DRAG_AIR;
            m_velocity.z *= horizontalDrag;
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
            speed *= physics::SPRINT_SPEED_MULTIPLIER;
        }
        if (sneaking) {
            speed *= physics::SNEAK_SPEED_MULTIPLIER;
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
    setHealth(maxHealth());
    m_foodStats.setFoodLevel(20);
    m_foodStats.setSaturationLevel(5.0f);
    m_foodStats.setFoodTimer(0);
    m_isSleeping = false;
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

    // 下界进度追踪位置（可选）
    ser.writeBool(m_enteredNetherPosition.has_value());
    if (m_enteredNetherPosition.has_value()) {
        ser.writeF64(m_enteredNetherPosition->x);
        ser.writeF64(m_enteredNetherPosition->y);
        ser.writeF64(m_enteredNetherPosition->z);
    }
}

Result<std::unique_ptr<Player>> Player::deserialize(network::PacketDeserializer& deser) {
    // 读取基本信息
    auto idResult = deser.readU64();
    if (idResult.failed()) return idResult.error();
    PlayerId playerId = idResult.value();

    auto usernameResult = deser.readString();
    if (usernameResult.failed()) return usernameResult.error();
    std::string username = usernameResult.value();

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

    // 下界进度追踪位置（可选）
    auto hasNetherPosResult = deser.readBool();
    if (hasNetherPosResult.failed()) return hasNetherPosResult.error();
    if (hasNetherPosResult.value()) {
        auto nxResult = deser.readF64();
        if (nxResult.failed()) return nxResult.error();
        auto nyResult = deser.readF64();
        if (nyResult.failed()) return nyResult.error();
        auto nzResult = deser.readF64();
        if (nzResult.failed()) return nzResult.error();
        player->m_enteredNetherPosition = Vector3d(nxResult.value(), nyResult.value(), nzResult.value());
    }

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
// 受伤/死亡（覆盖 LivingEntity 方法）
// ============================================================================

bool Player::hurt(DamageSource& source, f32 amount) {
    // 创造模式无敌检查
    if (m_abilities.invulnerable && !source.canDamageCreative()) {
        return false;
    }
    // 调用父类方法处理伤害
    return LivingEntity::hurt(source, amount);
}

void Player::die(DamageSource& cause) {
    // 调用父类方法处理死亡
    LivingEntity::die(cause);

    // 玩家特有：掉落经验
    dropExperience();
}

// ============================================================================
// 摔落伤害
// ============================================================================

void Player::handleFallDamage(f32 distance, f32 damageMultiplier) {
    // 调用父类处理摔落伤害计算（包含摔落音效播放）
    LivingEntity::handleFallDamage(distance, damageMultiplier);
}

// ============================================================================
// 受伤/死亡声音
// ============================================================================

std::optional<ResourceLocation> Player::getHurtSound(DamageSource& source) const {
    // 参考 MC 1.16.5: PlayerEntity.getHurtSound()
    // 根据伤害类型返回不同音效
    if (source.isFire()) {
        return SoundEvents::ENTITY_PLAYER_HURT_ON_FIRE;
    } else if (source.isDrown()) {
        return SoundEvents::ENTITY_PLAYER_HURT_DROWN;
    } else if (source.isSweetBerryBush()) {
        return SoundEvents::ENTITY_PLAYER_HURT_SWEET_BERRY_BUSH;
    }
    return SoundEvents::ENTITY_PLAYER_HURT;
}

std::optional<ResourceLocation> Player::getDeathSound() const {
    // 参考 MC 1.16.5: PlayerEntity.getDeathSound()
    return SoundEvents::ENTITY_PLAYER_DEATH;
}

std::optional<ResourceLocation> Player::getFallSound(i32 fallHeight) const {
    // 参考 MC 1.16.5: PlayerEntity.getFallSound()
    // 高空摔落 (>4格) 使用 big_fall，否则使用 small_fall
    if (fallHeight > 4) {
        return SoundEvents::ENTITY_PLAYER_BIG_FALL;
    }
    return SoundEvents::ENTITY_PLAYER_SMALL_FALL;
}

// ============================================================================
// 属性注册（覆盖 LivingEntity 方法）
// ============================================================================

void Player::registerAttributes() {
    // 先调用父类方法注册基础属性
    LivingEntity::registerAttributes();

    // 设置玩家特有属性值
    using namespace entity::attribute;
    m_attributes.setBaseValue(Attributes::MOVEMENT_SPEED, defaults::player::MOVEMENT_SPEED);
    m_attributes.setBaseValue(Attributes::ATTACK_DAMAGE, defaults::player::ATTACK_DAMAGE);
    m_attributes.setBaseValue(Attributes::ATTACK_SPEED, defaults::player::ATTACK_SPEED);
}

// ============================================================================
// 移动物理（覆盖 LivingEntity 方法）
// ============================================================================

void Player::travel(f32 strafing, f32 vertical, f32 forward) {
    // 飞行模式处理 - Player 特有
    if (m_abilities.flying && !isRiding()) {
        f32 prevJumpFactor = m_jumpMovementFactor;
        m_jumpMovementFactor = m_abilities.flySpeed * (m_isSprinting ? physics::SPRINT_FLY_MULTIPLIER : 1.0f);
        LivingEntity::travel(strafing, vertical, forward);
        m_velocity.y *= physics::FLY_VERTICAL_DRAG;
        m_jumpMovementFactor = prevJumpFactor;
        m_fallDistance = 0.0f;
    } else {
        LivingEntity::travel(strafing, vertical, forward);
    }

    updateMoveDistance();
}

void Player::aiStep() {
    // 玩家不使用 AI 步进，由 handleMovementInput 和 updatePhysics 处理
    // 仅更新跳跃冷却
    if (m_jumpTicks > 0) {
        m_jumpTicks--;
    }
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

i32 Player::getDepthStriderLevel() const {
    // MC 1.16.5: EnchantmentHelper.getDepthStriderModifier(this)
    // 检查靴子上的深度守卫附魔等级
    using namespace item::enchant;
    const ItemStack& boots = m_inventory.getBoots();
    if (boots.isEmpty()) {
        return 0;
    }
    return EnchantmentHelper::getEnchantmentLevel(boots, &AllEnchantments::DEPTH_STRIDER);
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
            // 水下呼吸附魔(Respiration): 每级有 1/(level+1) 的概率不消耗空气
            // 例如: I级=50%, II级=33%, III级=25% 概率不消耗
            // TODO: 实现水下呼吸附魔的空气节约概率 (需: RespirationEnchantment等级获取)
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
                    auto drownSource = DamageSources::drown();
                    hurt(drownSource, physics::DROWN_DAMAGE_AMOUNT);
                }

                // 溺水时生成气泡粒子
                // TODO: 生成气泡粒子 (依赖: ParticleSystem 实现)
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
    // distanceWalkedModified = 水平位移 * 0.6，用于 bobView 相位
    // distanceWalkedOnStepModified = 位移加权值，用于脚步声/游泳声触发
    f32 stepDistance;

    if (isInWater()) {
        // 游泳距离包括垂直移动
        f32 swimDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
        m_moveDistanceSwam += swimDistance;

        // 游泳声音触发
        // MC: distanceWalkedOnStepModified += sqrt(motion.x^2 * 0.2 + motion.y^2 + motion.z^2 * 0.2) * 0.35
        stepDistance = std::sqrt(dx * dx * 0.2f + dy * dy + dz * dz * 0.2f) * 0.35f;
    } else {
        m_moveDistanceWalked += distance * 0.6f;
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
    updateCameraYaw();

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

void Player::updateCameraYaw() {
    m_prevCameraYaw = m_cameraYaw;

    if (isRiding()) {
        m_cameraYaw = 0.0f;
        return;
    }

    f32 targetCameraYaw = 0.0f;
    if (m_onGround && !isDead() && !isSwimming()) {
        targetCameraYaw = std::min(0.1f, std::sqrt(m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z));
    }
    m_cameraYaw += (targetCameraYaw - m_cameraYaw) * 0.4f;
}

void Player::playStepSound() {
    // MC 1.16.5: Entity.playStepSound(BlockPos, BlockState)
    // 由客户端在 updateMoveDistance() 检测到 m_shouldPlayStepSound 后调用

    if (m_world == nullptr) {
        return;
    }

    // 获取脚下方块状态
    const BlockState* blockState = m_world->getBlockState(m_stepSoundPos);
    if (blockState == nullptr || blockState->isAir()) {
        return;
    }

    // 调用 Entity 基类方法播放脚步声
    Entity::playStepSound(m_stepSoundPos, blockState);
}

void Player::playSwimSound(f32 volume) {
    // MC 1.16.5: Entity.playSwimSound(float volume)
    // 播放游泳声音
    if (m_world == nullptr || isSilent()) {
        return;
    }

    // 使用实体ID和tick计数器生成伪随机音调
    u32 seed = static_cast<u32>(m_id) ^ static_cast<u32>(m_ticksExisted);
    f32 randomValue = static_cast<f32>((seed * 1103515245 + 12345) % 32768) / 32768.0f;
    f32 pitch = 0.8f + randomValue * 0.4f;  // 0.8-1.2 范围

    playSound(SoundEvents::ENTITY_PLAYER_SWIM, volume, pitch);
}

void Player::addExhaustion(f32 exhaustion) {
    // 只有生存模式和冒险模式才消耗饥饿
    if (m_gameMode == GameMode::Survival || m_gameMode == GameMode::Adventure) {
        m_foodStats.addExhaustion(exhaustion);
    }
}

bool Player::canEat(bool ignoreHunger) const {
    // MC 1.16.5: PlayerEntity.canEat(boolean ignoreHunger)
    // 创造模式或观察者模式不能进食
    if (isCreative() || isSpectator()) {
        return false;
    }
    // 如果忽略饥饿值检查，返回 true（如金苹果等特殊食物）
    if (ignoreHunger) {
        return true;
    }
    // 否则检查饥饿值是否小于 20
    return m_foodStats.needsFood();
}

// ========== 攻击冷却系统 ==========

f32 Player::getCooledAttackStrength(f32 adjustTicks) const {
    // MC 1.16.5: getCooledAttackStrength()
    // 冷却进度 = min(ticksSinceLastAttack + adjustTicks, cooldownPeriod) / cooldownPeriod
    // 冷却周期 = 20 / attackSpeed (ticks)
    f32 attackSpeed = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_SPEED, 4.0));
    if (attackSpeed <= 0.0f) {
        attackSpeed = 4.0f;  // 默认攻击速度
    }

    f32 cooldownPeriod = 20.0f / attackSpeed;  // 冷却周期（ticks）
    f32 adjustedTicks = static_cast<f32>(m_ticksSinceLastAttack) + adjustTicks;
    f32 progress = adjustedTicks / cooldownPeriod;

    return std::min(progress, 1.0f);
}

void Player::resetCooldown() {
    m_ticksSinceLastAttack = 0;
}

void Player::attack(Entity& target) {
    // MC 1.16.5: PlayerEntity.attackTargetEntityWithCurrentItem()
    // 完整的玩家攻击逻辑

    // 1. 检查目标是否可被攻击（创造/观察模式不能攻击）
    if (isSpectator()) {
        return;
    }

    // 2. 只能攻击生物实体
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(&target);
    if (!livingTarget) {
        return;
    }

    // 3. 获取基础攻击伤害
    f32 baseDamage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0));

    // 4. 获取附魔伤害加成
    f32 enchantDamage = 0.0f;
    const ItemStack& mainHand = getMainHandItem();

    if (!mainHand.isEmpty()) {
        enchantDamage = entity::combat::PlayerAttackHelper::getEnchantmentDamageBonus(
            mainHand, livingTarget->getCreatureAttribute());
    }

    // 5. 计算攻击冷却进度
    // MC 1.16.5: 使用 adjustTicks = 0.5F 获取冷却强度
    f32 cooldownProgress = getCooledAttackStrength(0.5f);

    // 6. 应用冷却伤害衰减
    // MC 1.16.5: 基础伤害使用二次冷却系数，附魔伤害使用线性冷却系数
    // 参考：PlayerEntity.attack() 中 f = f * (0.2 + f2*f2 * 0.8) 和 f1 = f1 * f2
    f32 quadraticCooldown = 0.2f + cooldownProgress * cooldownProgress * 0.8f;
    f32 linearCooldown = cooldownProgress;
    f32 damage = baseDamage * quadraticCooldown;
    enchantDamage *= linearCooldown;

    // 7. 重置攻击冷却
    resetCooldown();

    // 如果伤害为 0，不执行攻击
    if (damage <= 0.0f && enchantDamage <= 0.0f) {
        return;
    }

    // 8. 判断是否是完全冷却攻击
    bool isFullCooldown = cooldownProgress > 0.9f;

    // 9. 计算击退
    i32 knockbackLevel = 0;
    if (!mainHand.isEmpty()) {
        knockbackLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
            mainHand, &item::enchant::AllEnchantments::KNOCKBACK);
    }

    // 疾跑额外击退
    bool isSprintKnockback = false;
    if (isSprinting() && isFullCooldown) {
        knockbackLevel++;
        isSprintKnockback = true;
        // MC 1.16.5: 播放击退攻击音效
        playSound(SoundEvents::ENTITY_PLAYER_ATTACK_KNOCKBACK, 1.0f, 1.0f);
    }

    // 10. 暴击判定
    bool isCritical = entity::combat::PlayerAttackHelper::isCriticalHit(*this);

    // 11. 火焰附加
    i32 fireAspectLevel = 0;
    if (!mainHand.isEmpty()) {
        fireAspectLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
            mainHand, &item::enchant::AllEnchantments::FIRE_ASPECT);
    }

    // 攻击前点燃（用于燃烧传递）
    // MC 1.16.5: 如果目标未燃烧，先点燃 1 秒（用于燃烧效果传递判定）
    bool wasBurning = false;
    if (fireAspectLevel > 0 && !livingTarget->isOnFire()) {
        wasBurning = true;
        livingTarget->setFire(20);  // 1 秒 = 20 ticks
    }

    // 12. 应用暴击倍率
    if (isCritical) {
        damage *= 1.5f;  // MC 1.16.5: 暴击倍率 1.5
    }

    // 13. 合并伤害
    f32 totalDamage = damage + enchantDamage;

    // 14. 创建伤害来源并应用伤害
    EntityDamageSource damageSource = DamageSources::playerAttack(this);
    bool attacked = livingTarget->hurt(damageSource, totalDamage);

    // 用于跟踪是否播放了特定攻击音效
    bool playedAttackSound = false;

    if (attacked) {
        // 15. 应用击退
        if (knockbackLevel > 0) {
            entity::combat::PlayerAttackHelper::applyKnockback(
                *livingTarget, *this, static_cast<f32>(knockbackLevel));

            // 疾跑击退后停止疾跑并减少水平速度
            // MC 1.16.5: this.setMotion(this.getMotion().mul(0.6D, 1.0D, 0.6D));
            if (isSprintKnockback) {
                Vector3 vel = velocity();
                setVelocity(vel.x * 0.6f, vel.y, vel.z * 0.6f);
                setSprinting(false);
            }
        }

        // 16. 横扫攻击（MC 1.16.5: 仅当使用剑、冷却>90%、非暴击、非疾跑、在地面、且几乎静止时触发）
        // TODO: 添加 distanceWalkedModified 跟踪以检测玩家是否静止
        // MC 1.16.5 条件: distanceWalkedModified - prevDistanceWalkedModified < getAIMoveSpeed()
        bool canSweep = isFullCooldown && !isCritical && !isSprintKnockback && isOnGround();
        if (canSweep) {
            // 检查主手是否持有剑
            const item::tool::SwordItem* sword = dynamic_cast<const item::tool::SwordItem*>(mainHand.getItem());
            if (sword != nullptr) {
                f32 sweepRatio = item::enchant::EnchantmentHelper::getSweepingDamageRatio(mainHand);
                if (sweepRatio > 0.0f) {
                    // MC 1.16.5: sweepDamage = 1.0 + sweepRatio * baseDamage
                    // 其中 baseDamage 是冷却调整后的伤害（不含附魔伤害）
                    f32 sweepDamage = 1.0f + sweepRatio * damage;

                    // 扫描目标周围 1x0.25x1 范围内的实体
                    AxisAlignedBB sweepBox = livingTarget->boundingBox().expand(1.0f, 0.25f, 1.0f);
                    std::vector<Entity*> nearbyEntities = world()->getEntitiesInAABB(sweepBox, this);

                    for (Entity* entity : nearbyEntities) {
                        // 排除自身、目标和队友
                        if (entity == this || entity == livingTarget) {
                            continue;
                        }

                        // 只对生物实体生效
                        LivingEntity* nearbyLiving = dynamic_cast<LivingEntity*>(entity);
                        if (!nearbyLiving) {
                            continue;
                        }

                        // 检查距离（最大 3 格）
                        if (distanceSqTo(*entity) > 9.0) {  // 3^2 = 9
                            continue;
                        }

                        // TODO: 检查盔甲架标记 (ArmorStandEntity.hasMarker())
                        // TODO: 检查队友关系 (isOnSameTeam())

                        // 应用击退并造成伤害
                        // MC 1.16.5: 击退方向基于玩家朝向
                        f32 yawRad = math::toRadians(yaw());
                        f64 knockbackX = static_cast<f64>(std::sin(yawRad));
                        f64 knockbackZ = static_cast<f64>(-std::cos(yawRad));
                        nearbyLiving->applyKnockback(0.4f, knockbackX, knockbackZ);

                        // 造成横扫伤害
                        EntityDamageSource sweepSource = DamageSources::playerAttack(this);
                        nearbyLiving->hurt(sweepSource, sweepDamage);
                    }

                    // MC 1.16.5: 播放横扫攻击音效
                    playSound(SoundEvents::ENTITY_PLAYER_ATTACK_SWEEP, 1.0f, 1.0f);
                }
            }
        }

        // 17. 应用火焰附加
        if (fireAspectLevel > 0) {
            // MC 1.16.5: 火焰附加持续时间 = level * 4 秒
            livingTarget->setFire(fireAspectLevel * 4 * 20);  // 20 ticks per second
        }

        // 18. 设置最后攻击目标
        setLastHurtTarget(livingTarget);

        // 播放攻击音效
        // MC 1.16.5: 根据攻击类型播放不同音效
        if (isCritical) {
            // 暴击音效
            playSound(SoundEvents::ENTITY_PLAYER_ATTACK_CRIT, 1.0f, 1.0f);
            playedAttackSound = true;
        } else if (canSweep && !playedAttackSound) {
            // 横扫音效已在上面播放
            playedAttackSound = true;
        }

        // 如果没有播放特殊音效，根据冷却强度播放普通攻击音效
        if (!playedAttackSound) {
            if (isFullCooldown) {
                playSound(SoundEvents::ENTITY_PLAYER_ATTACK_STRONG, 1.0f, 1.0f);
            } else {
                playSound(SoundEvents::ENTITY_PLAYER_ATTACK_WEAK, 1.0f, 1.0f);
            }
        }

        // 荆棘附魔反伤处理
        // TODO: EnchantmentHelper.applyThornEnchantments(target, this);

        // 19. 武器损耗
        // TODO: mainHand.hitEntity(target, this);

        // 20. 饱食度消耗
        // MC 1.16.5: 攻击消耗 0.1 饱食度
        addExhaustion(EXHAUSTION_ATTACK);
    } else {
        // 攻击失败（被格挡等）
        // MC 1.16.5: 播放无伤害攻击音效
        playSound(SoundEvents::ENTITY_PLAYER_ATTACK_NODAMAGE, 1.0f, 1.0f);

        if (wasBurning) {
            livingTarget->setFire(0);  // 移除之前点燃的火焰
        }
    }
}

ActionResultType Player::interactOn(Entity& target, Hand hand) {
    // MC 1.16.5: PlayerEntity.interactOn()

    // 1. 旁观者模式：只能打开命名容器
    if (isSpectator()) {
        // TODO: 如果目标实现了 INamedContainerProvider，打开容器
        // if (auto* provider = dynamic_cast<INamedContainerProvider*>(&target)) {
        //     openContainer(provider);
        //     return ActionResultType::Success;
        // }
        return ActionResultType::Pass;
    }

    // 2. 获取手持物品
    ItemStack itemstack = getHeldItem(hand);
    ItemStack itemstackCopy = itemstack;  // 保存副本用于创造模式恢复

    // 3. 先调用实体的 processInitialInteract 方法
    // TODO: ActionResultType entityResult = target.processInitialInteract(*this, hand);
    // if (entityResult.isSuccessOrConsume()) {
    //     // 创造模式恢复物品数量
    //     if (isCreative() && itemstack.isEmpty()) {
    //         inventory().setItem(hand == Hand::MainHand ? 0 : 40, itemstackCopy);
    //     }
    //     return entityResult;
    // }

    // 4. 如果实体不处理，尝试物品的 interactWithEntity
    if (!itemstack.isEmpty()) {
        // 只有生物实体才支持物品交互
        LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(&target);
        if (livingTarget != nullptr) {
            // 创造模式使用物品副本，避免消耗
            if (isCreative()) {
                itemstack = itemstackCopy;
            }

            // 调用物品的 itemInteractionForEntity
            // 注意: ItemStack::getItem() 返回 const Item*，需要转换为非 const
            // 这是安全的，因为 itemInteractionForEntity 可能会修改 ItemStack
            Item* item = const_cast<Item*>(itemstack.getItem());
            if (item != nullptr) {
                bool success = item->itemInteractionForEntity(itemstack, *this, *livingTarget, hand);
                if (success) {
                    // 物品被消耗处理
                    if (!isCreative() && itemstack.isEmpty()) {
                        // TODO: 触发 PlayerDestroyItem 事件
                        inventory().setItem(hand == Hand::MainHand ? 0 : 40, ItemStack());
                    }
                    return ActionResultType::Success;
                }
            }
        }
    }

    return ActionResultType::Pass;
}

} // namespace mc
