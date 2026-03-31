#include "Player.hpp"
#include "GameModeUtils.hpp"
#include "../../inventory/Slot.hpp"
#include "../../../physics/PhysicsEngine.hpp"
#include "../../../physics/PhysicsConstants.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../item/core/ItemRegistry.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace mc {

// ============================================================================
// Player 实现
// ============================================================================

Player::Player(EntityId id, const String& username)
    : Entity(LegacyEntityType::Player, id)
    , m_username(username)
{
    // 生成随机XP seed
    math::Random rng(static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    m_xpSeed = rng.nextInt();
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
        m_health = 0.0f;
        deathTime = 0;
    } else {
        hurtTime = 10;
    }
}

void Player::addExperience(i32 amount) {
    m_totalExperience += amount;

    while (amount > 0) {
        i32 capacity = experienceBarCapacity();
        f32 needed = static_cast<f32>(capacity) - m_experienceProgress * capacity;

        if (amount >= static_cast<i32>(needed)) {
            amount -= static_cast<i32>(needed);
            m_experienceLevel++;
            m_experienceProgress = 0.0f;
        } else {
            m_experienceProgress += static_cast<f32>(amount) / capacity;
            amount = 0;
        }
    }
}

void Player::setExperienceLevel(i32 level) {
    m_experienceLevel = std::max(0, level);
    m_experienceProgress = 0.0f;
}

i32 Player::experienceBarCapacity() const {
    // 经验条容量公式
    if (m_experienceLevel < 15) {
        return 7 + m_experienceLevel * 2;
    } else if (m_experienceLevel < 30) {
        return 37 + (m_experienceLevel - 15) * 5;
    } else {
        return 112 + (m_experienceLevel - 30) * 9;
    }
}

void Player::setSprinting(bool sprinting) {
    m_isSprinting = sprinting;
    if (sprinting) {
        addFlag(EntityFlags::Sprinting);
    } else {
        removeFlag(EntityFlags::Sprinting);
    }
}

void Player::setSneaking(bool sneaking) {
    m_isSneaking = sneaking;
    if (sneaking) {
        addFlag(EntityFlags::Crouching);
        setPose(EntityPose::Crouching);
    } else {
        removeFlag(EntityFlags::Crouching);
        setPose(EntityPose::Standing);
    }
}

void Player::setSwimming(bool swimming) {
    m_isSwimming = swimming;
    if (swimming) {
        addFlag(EntityFlags::Swimming);
        setPose(EntityPose::Swimming);
    } else {
        removeFlag(EntityFlags::Swimming);
        setPose(EntityPose::Standing);
    }
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
    } else {
        setPose(EntityPose::Standing);
    }
}

f32 Player::height() const {
    switch (m_pose) {
        case EntityPose::Sleeping:
            return 0.2f;
        case EntityPose::Swimming:
        case EntityPose::FallFlying:
        case EntityPose::SpinAttack:
            return PLAYER_SWIM_HEIGHT;
        case EntityPose::Crouching:
            return PLAYER_CROUCH_HEIGHT;
        default:
            return PLAYER_HEIGHT;
    }
}

f32 Player::eyeHeight() const {
    switch (m_pose) {
        case EntityPose::Sleeping:
            return 0.2f;
        case EntityPose::Swimming:
        case EntityPose::FallFlying:
        case EntityPose::SpinAttack:
            return 0.4f;
        case EntityPose::Crouching:
            return 1.27f;
        default:
            return PLAYER_EYE_HEIGHT;
    }
}

void Player::tick() {
    Entity::tick();

    // 更新受伤/死亡计时器
    if (hurtTime > 0) {
        hurtTime--;
    }

    if (isDead()) {
        deathTime++;
    }

    // 睡眠计时器
    if (m_isSleeping) {
        sleepTimer++;
    } else {
        sleepTimer = 0;
    }

    // 饥饿系统
    if (m_gameMode == GameMode::Survival && !isDead()) {
        // 简化的饥饿消耗
        if (m_foodStats.exhaustionLevel >= 4.0f) {
            m_foodStats.addExhaustion(0.0f); // 触发消耗
        }
    }
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

    // 更新跳跃状态（用于动画等）
    m_isJumping = jumping;

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

void Player::jump() {
    if (m_onGround && m_jumpTicks == 0) {
        m_velocity.y = physics::JUMP_VELOCITY;
        m_onGround = false;
        m_jumpTicks = JUMP_COOLDOWN; // 设置跳跃冷却
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

    // 2. 应用重力（飞行时不应用重力）
    if (!m_abilities.flying) {
        if (!m_onGround) {
            m_velocity.y -= physics::GRAVITY;
        }
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
    // 清空当前背包
    m_inventory.clear();

    i32 slot = 0;

    // 首先放置工作台在第一格（slot 0）
    Item* craftingTableItem = ItemRegistry::instance().getItem(
        ResourceLocation("minecraft:crafting_table"));
    BlockItem* craftingTableBlockItem = dynamic_cast<BlockItem*>(craftingTableItem);
    if (craftingTableBlockItem != nullptr && slot < PlayerInventory::TOTAL_SIZE) {
        m_inventory.setItem(slot, ItemStack(*craftingTableBlockItem, 64));
        slot++;
    }

    // 然后遍历所有其他注册的方块物品，添加到背包
    BlockItemRegistry::instance().forEachBlockItem([&](const BlockItem& item) {
        // 跳过工作台（已经放在第一格了）
        if (craftingTableBlockItem != nullptr && &item == craftingTableBlockItem) {
            return;
        }
        if (slot < PlayerInventory::TOTAL_SIZE) {
            // 创造模式下每个物品堆叠数量为64
            m_inventory.setItem(slot, ItemStack(item, 64));
            slot++;
        }
    });
}

void Player::respawn() {
    m_health = m_maxHealth;
    m_foodStats.foodLevel = 20;
    m_foodStats.saturationLevel = 5.0f;
    m_foodStats.exhaustionLevel = 0.0f;
    deathTime = 0;
    hurtTime = 0;
    m_isSleeping = false;
    m_jumpTicks = 0;
    sleepTimer = 0;
    setPose(EntityPose::Standing);
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

    // 经验
    ser.writeI32(m_experienceLevel);
    ser.writeF32(m_experienceProgress);
    ser.writeI32(m_totalExperience);
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
    player->m_isSneaking = sneakResult.value();

    // 饥饿
    auto foodResult = FoodStats::deserialize(deser);
    if (foodResult.failed()) return foodResult.error();
    player->m_foodStats = foodResult.value();

    // 经验
    auto levelResult = deser.readI32();
    if (levelResult.failed()) return levelResult.error();
    player->m_experienceLevel = levelResult.value();

    auto progressResult = deser.readF32();
    if (progressResult.failed()) return progressResult.error();
    player->m_experienceProgress = progressResult.value();

    auto totalResult = deser.readI32();
    if (totalResult.failed()) return totalResult.error();
    player->m_totalExperience = totalResult.value();

    return player;
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

} // namespace mc
