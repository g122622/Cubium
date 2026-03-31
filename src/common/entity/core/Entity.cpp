#include "Entity.hpp"
#include "../../world/IWorld.hpp"
#include "../../physics/PhysicsEngine.hpp"
#include "../../physics/PhysicsConstants.hpp"
#include "../../util/math/random/Random.hpp"
#include "../../util/math/MathUtils.hpp"
#include "../../world/block/Block.hpp"
#include <algorithm>
#include <sstream>
#include <chrono>
#include <cmath>

namespace mc {

// ============================================================================
// 静态数据参数定义
// ============================================================================

namespace {
    // 数据参数 ID 生成器
    entity::DataParameter<i8> FLAGS_PARAM{0};
    entity::DataParameter<i32> AIR_PARAM{1};
    entity::DataParameter<String> CUSTOM_NAME_PARAM{2};
    entity::DataParameter<bool> CUSTOM_NAME_VISIBLE_PARAM{3};
    entity::DataParameter<bool> SILENT_PARAM{4};
    entity::DataParameter<bool> NO_GRAVITY_PARAM{5};
    entity::DataParameter<i8> POSE_PARAM{6};
}

// ============================================================================
// Entity 实现
// ============================================================================

Entity::Entity(LegacyEntityType type, EntityId id, IWorld* world)
    : m_id(id)
    , m_legacyType(type)
    , m_world(world)
    , m_position(0.0f, 0.0f, 0.0f)
    , m_prevPosition(0.0f, 0.0f, 0.0f)
    , m_velocity(0.0f, 0.0f, 0.0f)
{
    // 生成随机UUID
    u64 seed = static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    math::Random rng(seed);

    std::stringstream ss;
    ss << std::hex << rng.nextU64() << rng.nextU64();
    m_uuid = ss.str();

    // 注册数据参数
    registerData();
}

void Entity::registerData() {
    // 注册基础数据参数
    m_dataManager.registerParam(FLAGS_PARAM, static_cast<i8>(0));
    m_dataManager.registerParam(AIR_PARAM, maxAir());
    m_dataManager.registerParam(CUSTOM_NAME_PARAM, String{});
    m_dataManager.registerParam(CUSTOM_NAME_VISIBLE_PARAM, false);
    m_dataManager.registerParam(SILENT_PARAM, false);
    m_dataManager.registerParam(NO_GRAVITY_PARAM, false);
    m_dataManager.registerParam(POSE_PARAM, static_cast<i8>(EntityPose::Standing));
}

String Entity::getTypeId() const {
    // 根据旧类型返回类型字符串
    switch (m_legacyType) {
        case LegacyEntityType::Player: return "minecraft:player";
        case LegacyEntityType::Item: return "minecraft:item";
        default: return "minecraft:unknown";
    }
}

void Entity::setPosition(f32 x, f32 y, f32 z) {
    m_prevPosition = m_position;
    m_position = Vector3(x, y, z);
}

void Entity::setRotation(f32 yaw, f32 pitch) {
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_yaw = yaw;
    m_pitch = pitch;
}

void Entity::setVelocity(f32 x, f32 y, f32 z) {
    m_velocity = Vector3(x, y, z);
}

void Entity::tick() {
    m_ticksExisted++;

    // 基础 tick
    baseTick();
}

void Entity::baseTick() {
    // 更新前一帧位置
    m_prevPosition = m_position;
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;

    // 更新传送冷却
    if (m_portalCooldown > 0) {
        m_portalCooldown--;
    }

    // 如果不在传送门中，重置传送门计时
    if (!m_inPortal) {
        m_portalTime = 0;
    }

    // 处理着火
    if (m_fire > 0) {
        if (isInWater() || isInLava()) {
            m_fire = 0;
        } else {
            m_fire--;
        }
    }

    // 处理空气值
    if (isInWater() || isInLava()) {
        if (!m_invulnerable) {
            m_air--;
            if (m_air <= -20) {
                m_air = 0;
                // TODO: 处理溺水伤害
            }
        }
    } else {
        m_air = maxAir();
    }

    // 更新环境状态
    updateEnvironmentState();
}

bool Entity::tickPortal() {
    // 基类默认不触发传送
    // Player 会重写此方法，需要 80 tick (4秒)
    // 其他实体需要约 1 tick
    return false;
}

void Entity::updateEnvironmentState() {
    // 如果有世界引用，检测水中/岩浆中状态
    if (m_world) {
        // 检测实体眼睛高度位置是否在液体中
        // 参考 MC 1.16.5 Entity.updateFluidOnEyes()
        const f32 eyeY = m_position.y + eyeHeight();

        // 检测眼睛位置是否在水中或岩浆中
        m_inWater = m_world->isWaterAt(
            static_cast<i32>(std::floor(m_position.x)),
            static_cast<i32>(std::floor(eyeY)),
            static_cast<i32>(std::floor(m_position.z))
        );

        m_inLava = m_world->isLavaAt(
            static_cast<i32>(std::floor(m_position.x)),
            static_cast<i32>(std::floor(eyeY)),
            static_cast<i32>(std::floor(m_position.z))
        );
    }
}

void Entity::updateFallDistance() {
    // 更新摔落距离
    if (!m_onGround && m_velocity.y < 0.0f) {
        m_fallDistance -= m_velocity.y;
    } else if (m_onGround && m_fallDistance > 0.0f) {
        handleFallDamage(m_fallDistance, 1.0f);
        m_fallDistance = 0.0f;
    }
}

void Entity::handleFallDamage(f32 /* distance */, f32 /* damageMultiplier */) {
    // 基础实体不处理摔落伤害
    // LivingEntity 会重写此方法
}

void Entity::update() {
    // 保存上一帧位置
    m_prevPosition = m_position;
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
}

void Entity::move(f32 dx, f32 dy, f32 dz) {
    m_prevPosition = m_position;
    m_position.x += dx;
    m_position.y += dy;
    m_position.z += dz;
}

void Entity::rotate(f32 deltaYaw, f32 deltaPitch) {
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_yaw += deltaYaw;
    m_pitch += deltaPitch;

    // 限制俯仰角范围
    m_pitch = std::clamp(m_pitch, -90.0f, 90.0f);

    // 规范化偏航角到 [0, 360) 范围
    m_yaw = math::wrapDegreesPositive(m_yaw);
}

/**
 * @brief 带碰撞检测的移动
 *
 * 参考MC的Entity.move()实现。
 * 核心流程：
 * 1. 使用物理引擎执行碰撞检测和移动
 * 2. 更新实体位置（从碰撞箱计算）
 * 3. 更新碰撞状态和地面状态
 * 4. 更新坠落距离
 */
Vector3 Entity::moveWithCollision(f32 dx, f32 dy, f32 dz) {
    Vector3 desiredMovement(dx, dy, dz);

    // 重置碰撞状态
    m_collidedHorizontally = false;
    m_collidedVertically = false;

    // 优先使用 World 的物理引擎
    PhysicsEngine* physics = physicsEngine();

    if (!physics) {
        // 无物理引擎，直接移动
        move(dx, dy, dz);
        // 尝试通过 World 检测地面
        checkOnGround();
        return desiredMovement;
    }

    // 获取当前碰撞箱
    AxisAlignedBB entityBox = boundingBox();

    // 使用物理引擎执行碰撞检测移动
    // 参考MC: Entity.move() -> getAllowedMovement()
    Vector3 actualMovement = physics->moveEntity(entityBox, desiredMovement, stepHeight());

    // 从碰撞箱更新位置
    // 实体位置 = 碰撞箱底部中心
    m_position = Vector3(
        (entityBox.minX + entityBox.maxX) / 2.0f,  // 中心X
        entityBox.minY,                             // 底部Y
        (entityBox.minZ + entityBox.maxZ) / 2.0f   // 中心Z
    );

    // 更新碰撞状态（从物理引擎获取）
    m_collidedHorizontally = physics->collidedHorizontally();
    m_collidedVertically = physics->collidedVertically();

    // 更新地面状态
    // 优先使用”向下移动时发生垂直碰撞”的判定，避免纯接触检测抖动。
    bool groundedByCollision = m_collidedVertically && desiredMovement.y < 0.0f;
    bool groundedByContact = physics->isOnGround(entityBox);
    m_onGround = groundedByCollision || groundedByContact;

    // 更新摔落距离并处理摔落伤害
    updateFallDistance();

    return actualMovement;
}

PhysicsEngine* Entity::physicsEngine() {
    // 优先使用 World 的物理引擎
    if (m_world) {
        PhysicsEngine* engine = m_world->physicsEngine();
        if (engine) return engine;
    }
    // 备用：显式设置的物理引擎（客户端兼容）
    return m_physicsEngine;
}

const PhysicsEngine* Entity::physicsEngine() const {
    // 优先使用 World 的物理引擎
    if (m_world) {
        const PhysicsEngine* engine = m_world->physicsEngine();
        if (engine) return engine;
    }
    // 备用：显式设置的物理引擎（客户端兼容）
    return m_physicsEngine;
}

void Entity::checkOnGround() {
    if (!m_world) {
        m_onGround = false;
        return;
    }

    // 检测实体下方是否有方块
    AxisAlignedBB box = boundingBox();
    box.minY -= 0.1f;  // 向下延伸一点
    box.maxY = box.minY + 0.1f;  // 扁平的检测区域

    m_onGround = m_world->hasBlockCollision(box);
}

void Entity::applyPhysics(f32 deltaTime) {
    // 应用重力
    if (!m_onGround) {
        // 重力加速度（MC使用固定值，与deltaTime无关）
        // 但这里我们使用deltaTime来平滑
        m_velocity.y -= physics::GRAVITY;
    }

    // 应用空气阻力
    m_velocity.x *= physics::DRAG_AIR;
    m_velocity.y *= physics::DRAG_AIR;
    m_velocity.z *= physics::DRAG_AIR;

    // 如果在地面，停止Y方向速度
    if (m_onGround && m_velocity.y < 0.0f) {
        m_velocity.y = 0.0f;
    }

    (void)deltaTime; // MC物理是基于tick的，不使用deltaTime
}

// ============================================================================
// 乘客/骑乘系统
// ============================================================================

bool Entity::isPassenger(EntityId entityId) const {
    for (EntityId passenger : m_passengers) {
        if (passenger == entityId) {
            return true;
        }
    }
    return false;
}

bool Entity::addPassenger(Entity& passenger) {
    // 检查是否已经是乘客
    if (isPassenger(passenger.id())) {
        return false;
    }

    // 如果乘客正在骑乘其他实体，先停止
    if (passenger.isRiding()) {
        passenger.stopRiding();
    }

    // 添加乘客
    m_passengers.push_back(passenger.id());
    passenger.setVehicle(m_id);

    return true;
}

void Entity::removePassenger(Entity& passenger) {
    // 查找并移除乘客
    auto it = std::find(m_passengers.begin(), m_passengers.end(), passenger.id());
    if (it != m_passengers.end()) {
        m_passengers.erase(it);
        passenger.setVehicle(INVALID_ENTITY_ID);
    }
}

bool Entity::startRiding(Entity& vehicle) {
    // 不能骑乘自己
    if (vehicle.id() == m_id) {
        return false;
    }

    // 如果已经在骑乘，先停止
    if (isRiding()) {
        stopRiding();
    }

    // 添加到车辆的乘客列表
    return vehicle.addPassenger(*this);
}

void Entity::stopRiding(bool clearVehicle) {
    if (!isRiding()) {
        return;
    }

    // 从车辆中移除自己
    if (m_world && clearVehicle) {
        // TODO: 通过世界获取车辆实体并移除
        // 目前只清除自己的车辆引用
    }

    m_vehicle = INVALID_ENTITY_ID;
}

Vector3 Entity::getRidingPosition() const {
    // 默认骑乘位置在实体顶部中心
    return Vector3(0.0f, height(), 0.0f);
}

bool Entity::canSee(const Entity& other) const {
    // TODO: 使用射线检测实现视线检查
    // 当前简化实现：只检查距离和基本条件
    // 完整实现需要使用世界的射线追踪功能

    // 检查目标是否存活
    if (!other.isAlive()) {
        return false;
    }

    // 计算到目标的距离
    f32 distSq = distanceSqTo(other);

    // 如果距离超过视线范围（64格），返回false
    constexpr f32 SIGHT_RANGE_SQ = 64.0f * 64.0f;
    if (distSq > SIGHT_RANGE_SQ) {
        return false;
    }

    // TODO: 使用世界射线追踪检查视线是否被方块阻挡
    // Vector3 eyePos = Vector3(x(), y() + eyeHeight(), z());
    // Vector3 targetEyePos = Vector3(other.x(), other.y() + other.eyeHeight(), other.z());
    // return m_world && !m_world->raycastBlocks(eyePos, targetEyePos);

    return true;
}

} // namespace mc
