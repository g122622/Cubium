#include "Entity.hpp"
#include "../utils/EntityUtils.hpp"
#include "../entities/player/Player.hpp"
#include "../../world/IWorld.hpp"
#include "../../physics/PhysicsEngine.hpp"
#include "../../physics/PhysicsConstants.hpp"
#include "../../util/math/random/Random.hpp"
#include "../../util/math/MathUtils.hpp"
#include "../../world/block/Block.hpp"
#include "../../world/block/BlockPos.hpp"
#include "../../world/fluid/Fluid.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../util/text/StringTextComponent.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <sstream>
#include <chrono>
#include <cmath>
#include <unordered_set>

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
    , m_position(0.0f, 0.0f, 0.0f)
    , m_prevPosition(0.0f, 0.0f, 0.0f)
    , m_velocity(0.0f, 0.0f, 0.0f)
    , m_world(world)
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

entity::EntitySize Entity::getDimensions(EntityPose pose) const {
    (void)pose;
    return entity::EntitySize(width(), height(), eyeHeight(), false);
}

void Entity::refreshDimensions() {
    m_dimensions = getDimensions(m_pose);
    m_dimensionsInitialized = true;
    reapplyPosition();
}

void Entity::reapplyPosition() {
    if (!m_dimensionsInitialized) {
        m_dimensions = getDimensions(m_pose);
        m_dimensionsInitialized = true;
    }

    m_boundingBox = m_dimensions.makeBoundingBox(m_position.x, m_position.y, m_position.z);
}

void Entity::setPose(EntityPose pose) {
    if (m_pose == pose) {
        return;
    }

    m_pose = pose;
    m_dataManager.set(POSE_PARAM, static_cast<i8>(pose));
    refreshDimensions();
}

void Entity::setFlags(EntityFlags flags) {
    m_flags = flags;
    m_dataManager.set(FLAGS_PARAM, static_cast<i8>(static_cast<u8>(flags)));
}

void Entity::addFlag(EntityFlags flag) {
    m_flags = m_flags | flag;
    m_dataManager.set(FLAGS_PARAM, static_cast<i8>(static_cast<u8>(m_flags)));
}

void Entity::removeFlag(EntityFlags flag) {
    m_flags = static_cast<EntityFlags>(static_cast<u8>(m_flags) & ~static_cast<u8>(flag));
    m_dataManager.set(FLAGS_PARAM, static_cast<i8>(static_cast<u8>(m_flags)));
}

void Entity::setAir(i32 air) {
    m_air = air;
    m_dataManager.set(AIR_PARAM, m_air);
}

void Entity::setCustomName(const String& name) {
    if (name.empty()) {
        m_customName = nullptr;
        m_dataManager.set(CUSTOM_NAME_PARAM, String(""));
    } else {
        m_customName = std::make_unique<text::StringTextComponent>(name);
        m_dataManager.set(CUSTOM_NAME_PARAM, name);
    }
}

void Entity::setCustomNameComponent(std::unique_ptr<text::ITextComponent> name) {
    m_customName = std::move(name);
    // 数据管理器仍然存储纯文本用于网络同步
    m_dataManager.set(CUSTOM_NAME_PARAM, m_customName ? m_customName->getUnformattedText() : String(""));
}

std::unique_ptr<text::ITextComponent> Entity::getDisplayName() const {
    if (m_customName) {
        return m_customName->deepCopy();
    }
    // 返回默认名称
    return std::make_unique<text::StringTextComponent>("entity");
}

void Entity::setCustomNameVisible(bool visible) {
    m_customNameVisible = visible;
    m_dataManager.set(CUSTOM_NAME_VISIBLE_PARAM, m_customNameVisible);
}

void Entity::setSilent(bool silent) {
    m_silent = silent;
    m_dataManager.set(SILENT_PARAM, m_silent);
}

void Entity::setNoGravity(bool noGravity) {
    m_noGravity = noGravity;
    m_dataManager.set(NO_GRAVITY_PARAM, m_noGravity);
}

String Entity::getTypeId() const {
    if (!m_typeId.empty()) {
        return m_typeId;
    }

    return EntityUtils::legacyTypeToTypeId(m_legacyType);
}

std::optional<ResourceLocation> Entity::makeSoundEventId(StringView suffix) const {
    const String typeId = getTypeId();
    const size_t separatorPos = typeId.find(':');
    if (separatorPos == String::npos || separatorPos + 1 >= typeId.size()) {
        return std::nullopt;
    }

    const String typePath = typeId.substr(separatorPos + 1);
    if (typePath.empty() || typePath == "unknown") {
        return std::nullopt;
    }

    String soundId = "minecraft:entity.";
    soundId += typePath;
    soundId += '.';
    soundId += String(suffix);
    return ResourceLocation(soundId);
}

void Entity::playSound(const ResourceLocation& soundEventId, f32 volume, f32 pitch) const {
    if (m_world == nullptr || isSilent()) {
        return;
    }

    m_world->playSound(soundEventId, getSoundCategory(), m_position, volume, pitch);
}

void Entity::playStepSound(const BlockPos& pos, const BlockState* blockState) {
    // MC 1.16.5: Entity.playStepSound(BlockPos, BlockState)
    // 默认实现使用脚下方块的声音类型播放脚步声
    // 子类可以重写以自定义声音（如蜜蜂不播放脚步声）

    if (blockState == nullptr || m_world == nullptr) {
        return;
    }

    // 获取方块的声音类型
    // 注意：完整的实现需要检查上方是否有雪层
    // 如果上方是雪层，则使用雪的声音类型
    // 这里暂时简化实现

    // TODO: 获取 BlockSoundType 并播放 step 声音
    // 需要 BlockState::getSoundType() 方法返回 BlockSoundType
    // 然后调用 getStepSound() 获取声音事件

    // 暂时使用空实现，子类会提供具体实现
    (void)pos;
    (void)blockState;
}

void Entity::setPosition(f32 x, f32 y, f32 z) {
    m_prevPosition = m_position;
    m_position = Vector3(x, y, z);
    reapplyPosition();
}

void Entity::snapshotInterpolationState() {
    m_prevPosition = m_position;
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
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

    // 处理传送门逻辑
    // 参考 MC 1.16.5 Entity.tick() 末尾的传送门处理
    if (tickPortal()) {
        onPortalTriggered();
    }
}

void Entity::baseTick() {
    // 更新前一帧位置
    m_prevPosition = m_position;
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;

    // MC 1.16.5: 检查车辆是否被移除
    // 如果正在骑乘且车辆已被移除，则下车
    if (isRiding()) {
        if (m_world) {
            Entity* vehicle = m_world->getEntity(m_vehicle);
            if (vehicle == nullptr || vehicle->isRemoved()) {
                stopRiding();
            }
        } else {
            stopRiding();
        }
    }

    // 更新传送冷却
    // 参考 MC 1.16.5 Entity.baseTick() 中的 timeUntilPortal 递减
    if (m_portalCooldown > 0) {
        m_portalCooldown--;
    }

    // 更新骑乘冷却
    // MC 1.16.5: rideCooldown 递减
    if (m_rideCooldown > 0) {
        m_rideCooldown--;
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
            setAir(m_air - 1);
            if (m_air <= -20) {
                setAir(0);
                // TODO: 处理溺水伤害
            }
        }
    } else {
        setAir(maxAir());
    }

    // 更新环境状态
    updateEnvironmentState();

    // 重新探测地面状态，避免实体在脚下方块被移除后仍然沿用旧的 onGround 缓存。
    checkOnGround();

    // MC 1.16.5: 如果是乘客，调用 updateRidden()
    // 注意：这里需要子类重写以实现乘客逻辑
    // 在 Vehicle/tick() 中应该调用 updatePassengers()
}

bool Entity::tickPortal() {
    // 参考 MC 1.16.5 Entity.updatePortal()
    // 关键行为：inPortal 每帧重置，由 NetherPortalBlock.onEntityCollision 重新设置
    // 冷却递减在 baseTick() 的 decrementTimeUntilPortal() 中处理

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

    // 递增计时并检查阈值（MC: portalCounter++ >= i）
    // 非玩家：i=0，第一次进入时 1 >= 0 成立
    // 玩家：i=80，第 80 tick 时 80 >= 80 成立
    m_portalTime++;

    const i32 maxPortalTime = getMaxInPortalTime();
    if (m_portalTime >= maxPortalTime) {
        m_portalTime = maxPortalTime;
        return true;
    }

    return false;
}

bool Entity::onPortalTriggered() {
    // 基类实现：默认不做任何事
    // 子类（如 ServerPlayer）可重写此方法以实现实际的维度切换逻辑
    // MC 1.16.5 中，此方法会调用 changeDimension

    // 重置传送门状态
    m_inPortal = false;
    m_portalTime = 0;
    triggerPortalCooldown();

    return false;
}

bool Entity::isOnLadder() const {
    // 缓存结果，避免每帧多次查询
    // 在 updateEnvironmentState() 中更新缓存

    if (m_world == nullptr) {
        return false;
    }

    // 检查实体碰撞箱内的方块
    // MC 1.16.5: 检查碰撞箱覆盖的所有方块位置
    const AxisAlignedBB box = boundingBox();

    i32 minX = static_cast<i32>(std::floor(box.minX));
    i32 maxX = static_cast<i32>(std::floor(box.maxX));
    i32 minY = static_cast<i32>(std::floor(box.minY));
    i32 maxY = static_cast<i32>(std::floor(box.maxY));
    i32 minZ = static_cast<i32>(std::floor(box.minZ));
    i32 maxZ = static_cast<i32>(std::floor(box.maxZ));

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                const BlockState* blockState = m_world->getBlockState(x, y, z);
                if (blockState != nullptr) {
                    const Block& block = blockState->getBlock();
                    BlockPos pos(x, y, z);
                    if (block.isLadder(*blockState, m_world, &pos, this)) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void Entity::updateEnvironmentState() {
    // 参考 MC 1.16.5 Entity.func_233566_aG_() 和 func_233567_aH_()
    // 需要遍历碰撞箱内的所有方块，计算流体浸入高度

    // 眼睛位置（用于判断眼睛是否在水下）
    const f32 eyeY = m_position.y + eyeHeight();
    const i32 eyeBlockY = static_cast<i32>(std::floor(eyeY));

    // 重置流体状态
    m_inWater = false;
    m_inLava = false;
    m_waterHeight = 0.0f;
    m_lavaHeight = 0.0f;
    m_eyesInWater = false;
    m_eyesInLava = false;

    if (m_world == nullptr && m_physicsEngine == nullptr) {
        return;
    }

    // 获取碰撞箱并收缩一点以避免边界问题
    AxisAlignedBB box = boundingBox().shrink(0.001);

    // 计算碰撞箱覆盖的方块范围
    const i32 minX = static_cast<i32>(std::floor(box.minX));
    const i32 maxX = static_cast<i32>(std::floor(box.maxX));
    const i32 minY = static_cast<i32>(std::floor(box.minY));
    const i32 maxY = static_cast<i32>(std::floor(box.maxY));
    const i32 minZ = static_cast<i32>(std::floor(box.minZ));
    const i32 maxZ = static_cast<i32>(std::floor(box.maxZ));

    // 遍历碰撞箱内的所有方块
    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                // 获取流体状态
                const fluid::FluidState* fluidState = nullptr;
                if (m_world) {
                    fluidState = m_world->getFluidState(x, y, z);
                } else if (m_physicsEngine) {
                    const ICollisionWorld* collisionWorld = m_physicsEngine->getWorld();
                    if (collisionWorld) {
                        const BlockState* blockState = collisionWorld->getBlockState(x, y, z);
                        fluidState = blockState != nullptr ? blockState->getFluidState() : nullptr;
                    }
                }

                if (fluidState == nullptr || fluidState->isEmpty()) {
                    continue;
                }

                // 计算流体高度
                // MC: (float)y + fluidState.getActualHeight()
                f32 fluidTopY = static_cast<f32>(y) + fluidState->getHeight();

                // 检查流体是否在碰撞箱内
                if (fluidTopY > box.minY) {
                    // 计算浸入高度
                    f32 submergedHeight = fluidTopY - box.minY;

                    // 判断流体类型
                    const ResourceLocation& fluidId = fluidState->getFluid().fluidLocation();
                    bool isWater = fluidId.namespace_() == "minecraft" &&
                                   (fluidId.path() == "water" || fluidId.path() == "flowing_water");
                    bool isLava = fluidId.namespace_() == "minecraft" &&
                                  (fluidId.path() == "lava" || fluidId.path() == "flowing_lava");

                    if (isWater) {
                        m_inWater = true;
                        m_waterHeight = std::max(m_waterHeight, submergedHeight);

                        // 检查眼睛是否在水下
                        // MC: 眼睛位置稍微下移 0.11111111 来检测
                        constexpr f32 EYE_OFFSET = 0.11111111f;
                        f32 adjustedEyeY = eyeY - EYE_OFFSET;
                        if (fluidTopY > adjustedEyeY) {
                            m_eyesInWater = true;
                        }
                    } else if (isLava) {
                        m_inLava = true;
                        m_lavaHeight = std::max(m_lavaHeight, submergedHeight);

                        // 检查眼睛是否在岩浆中
                        constexpr f32 EYE_OFFSET = 0.11111111f;
                        f32 adjustedEyeY = eyeY - EYE_OFFSET;
                        if (fluidTopY > adjustedEyeY) {
                            m_eyesInLava = true;
                        }
                    }
                }
            }
        }
    }

    // 兼容旧代码：设置 m_fluidHeight
    m_fluidHeight = std::max(m_waterHeight, m_lavaHeight);
}

bool Entity::isInRain() const {
    // MC 1.16.5: Entity.isInRain()
    // 需要：世界存在 + 正在下雨 + 实体位置可以降雨
    if (m_world == nullptr) {
        return false;
    }

    // 检查世界是否正在下雨
    if (!m_world->isRaining()) {
        return false;
    }

    // 检查实体位置是否可以降雨
    // 使用实体脚部位置
    BlockPos pos(static_cast<i32>(std::floor(m_position.x)),
                 static_cast<i32>(std::floor(m_position.y)),
                 static_cast<i32>(std::floor(m_position.z)));
    return m_world->canRainAt(pos);
}

void Entity::syncMetadataFromDataManager() {
    m_flags = static_cast<EntityFlags>(static_cast<u8>(m_dataManager.get<i8>(FLAGS_PARAM)));
    m_air = m_dataManager.get<i32>(AIR_PARAM);
    // 从数据管理器同步名称（纯文本）
    {
        const String& nameText = m_dataManager.get<String>(CUSTOM_NAME_PARAM);
        if (nameText.empty()) {
            m_customName = nullptr;
        } else {
            m_customName = std::make_unique<text::StringTextComponent>(nameText);
        }
    }
    m_customNameVisible = m_dataManager.get<bool>(CUSTOM_NAME_VISIBLE_PARAM);
    m_silent = m_dataManager.get<bool>(SILENT_PARAM);
    m_noGravity = m_dataManager.get<bool>(NO_GRAVITY_PARAM);
    m_pose = static_cast<EntityPose>(m_dataManager.get<i8>(POSE_PARAM));
    refreshDimensions();
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
    reapplyPosition();
}

void Entity::move(entity::MoverType type, const Vector3& delta) {
    MC_UNUSED(type);
    // MC 1.16.5: 移动类型用于区分移动来源
    // 目前简单委托给无碰撞版本，后续可添加碰撞检测
    move(delta.x, delta.y, delta.z);
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
    reapplyPosition();

    // 更新碰撞状态（从物理引擎获取）
    m_collidedHorizontally = physics->collidedHorizontally();
    m_collidedVertically = physics->collidedVertically();

    // 更新地面状态
    // 优先使用”向下移动时发生垂直碰撞”的判定，避免纯接触检测抖动。
    bool groundedByCollision = m_collidedVertically && desiredMovement.y < 0.0f;
    bool groundedByContact = physics->isOnGround(entityBox);
    m_onGround = groundedByCollision || groundedByContact;

    // MC 1.16.5: 碰撞后速度重置
    // 参考Entity.move() 行601-608
    // 如果某轴发生碰撞（实际移动 != 期望移动），清零该轴速度
    // 注意：使用 MC 的 MathHelper.epsilonEquals 比较，阈值约 1e-7
    constexpr f32 EPSILON = 1.0e-7f;
    if (std::abs(desiredMovement.x - actualMovement.x) > EPSILON) {
        // X轴碰撞，清零X速度
        m_velocity.x = 0.0f;
    }
    if (std::abs(desiredMovement.y - actualMovement.y) > EPSILON) {
        // Y轴碰撞，清零Y速度
        m_velocity.y = 0.0f;
    }
    if (std::abs(desiredMovement.z - actualMovement.z) > EPSILON) {
        // Z轴碰撞，清零Z速度
        m_velocity.z = 0.0f;
    }

    // MC 1.16.5: 方块碰撞回调
    // 参考 Entity.move() 行 610-616
    doBlockCollisions(actualMovement, desiredMovement);

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
    const AxisAlignedBB box = boundingBox();

    if (m_world) {
        // 检测实体下方是否有方块
        AxisAlignedBB groundProbe = box;
        groundProbe.minY -= 0.1f;  // 向下延伸一点
        groundProbe.maxY = groundProbe.minY + 0.1f;  // 扁平的检测区域

        m_onGround = m_world->hasBlockCollision(groundProbe);
        return;
    }

    if (m_physicsEngine) {
        m_onGround = m_physicsEngine->isOnGround(box);
        return;
    }

    m_onGround = false;
}

void Entity::doBlockCollisions(const Vector3& actualMovement, const Vector3& desiredMovement) {
    // MC 1.16.5: Entity.move() 中的方块回调处理
    // 参考: Entity.java 行 610-616

    if (m_world == nullptr) {
        return;
    }

    // 获取实体脚下所在的方块位置
    // 使用碰撞箱底部的中心坐标
    BlockPos blockPos(
        static_cast<i32>(std::floor(m_position.x)),
        static_cast<i32>(std::floor(m_boundingBox.minY - 0.001f)),  // 稍微向下偏移
        static_cast<i32>(std::floor(m_position.z))
    );

    // 获取方块状态
    const BlockState* blockState = m_world->getBlockState(blockPos);
    if (blockState == nullptr) {
        return;
    }

    const Block& block = blockState->getBlock();

    // 1. onLanded 回调 - 当垂直位置发生变化时
    // MC: if (pos.y != vector3d.y) { block.onLanded(this.world, this); }
    if (std::abs(desiredMovement.y - actualMovement.y) > 1.0e-7f) {
        // Y轴发生了碰撞，说明着陆了
        block.onLanded(*blockState, *m_world, blockPos, *this);
    }

    // 2. onEntityWalk 回调 - 当在地面行走时
    // MC: if (this.onGround && !this.isSteppingCarefully()) { block.onEntityWalk(this.world, blockpos, this); }
    if (m_onGround && !isSteppingCarefully()) {
        block.onEntityWalk(*blockState, *m_world, blockPos, *this);
    }
}

void Entity::applyPhysics(f32 /*deltaTime*/) {
    // MC 1.16.5: Entity 物理更新
    // 注意：重力应该始终应用（除非 noGravity），碰撞检测会处理停止
    // 参考 Entity.move() 中的物理处理

    // 重力始终应用（除非 noGravity 标志为 true）
    // MC 1.16.5: if (!this.hasNoGravity()) { this.setMotion(...) }
    if (!m_noGravity) {
        m_velocity.y -= physics::GRAVITY;
    }

    // 应用空气阻力
    // MC 1.16.5: 空气阻力在移动后应用
    m_velocity.x *= physics::DRAG_AIR;
    m_velocity.y *= physics::DRAG_AIR;
    m_velocity.z *= physics::DRAG_AIR;

    // 注意：MC 物理是基于 tick 的，deltaTime 参数被忽略
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
    // MC 1.16.5: Entity.addPassenger()

    // 检查是否已经是乘客
    if (isPassenger(passenger.id())) {
        return false;
    }

    // MC 1.16.5: 如果 passenger.getRidingEntity() != this，抛出异常
    // 这里我们改为：如果乘客正在骑乘其他实体，先停止
    // 注意：这与MC略有不同，MC期望调用者先设置ridingEntity
    // 我们的实现：调用者应该使用 startRiding() 而不是直接调用 addPassenger()

    // 循环检测：防止A骑B，B骑A等循环
    // MC: for(Entity entity = entityIn; entity.ridingEntity != null; entity = entity.ridingEntity)
    //     if (entity.ridingEntity == this) return false;
    if (m_world) {
        Entity* current = &passenger;
        while (current != nullptr) {
            EntityId currentVehicle = current->getVehicle();
            if (currentVehicle == INVALID_ENTITY_ID) {
                break;
            }
            if (currentVehicle == m_id) {
                // 检测到循环：该实体已经是我们的乘客链中的一员
                return false;
            }
            current = m_world->getEntity(currentVehicle);
        }
    }

    // 检查乘客数量限制
    if (!canFitPassenger()) {
        return false;
    }

    // 如果乘客正在骑乘其他实体，先停止
    if (passenger.isRiding()) {
        passenger.stopRiding();
    }

    // 设置乘客姿态为站立
    // MC: passenger.setPose(Pose.STANDING);
    passenger.setPose(EntityPose::Standing);

    // MC 1.16.5: 服务端玩家优先插入列表头部
    // if (!this.world.isRemote && passenger instanceof PlayerEntity && !(this.getControllingPassenger() instanceof PlayerEntity)) {
    //     this.passengers.add(0, passenger);
    // } else {
    //     this.passengers.add(passenger);
    // }
    bool isServerSide = m_world && !m_world->isClientSide();
    bool isPlayer = passenger.legacyType() == LegacyEntityType::Player;
    EntityId controllingId = getControllingPassenger();
    bool controllingIsPlayer = false;
    if (controllingId != INVALID_ENTITY_ID && m_world) {
        Entity* controlling = m_world->getEntity(controllingId);
        controllingIsPlayer = controlling != nullptr && controlling->legacyType() == LegacyEntityType::Player;
    }

    if (isServerSide && isPlayer && !controllingIsPlayer) {
        // 服务端：玩家优先插入头部，成为控制者
        m_passengers.insert(m_passengers.begin(), passenger.id());
    } else {
        // 其他情况：追加到末尾
        m_passengers.push_back(passenger.id());
    }
    passenger.setVehicle(m_id);

    // 设置骑乘冷却
    // MC: passenger.rideCooldown = 60;
    passenger.m_rideCooldown = 60;

    // 触发回调
    // MC: this.onAddedPassenger(passenger);
    // 子类可重写 onAddedPassenger() 来处理特殊逻辑

    return true;
}

void Entity::removePassenger(Entity& passenger) {
    // MC 1.16.5: Entity.removePassenger()

    // 验证乘客是否确实骑乘此载具
    if (passenger.getVehicle() != m_id) {
        // MC: throw new IllegalStateException("Use x.stopRiding(y), not y.removePassenger(x)");
        return;
    }

    // 查找并移除乘客
    auto it = std::find(m_passengers.begin(), m_passengers.end(), passenger.id());
    if (it != m_passengers.end()) {
        m_passengers.erase(it);
        passenger.setVehicle(INVALID_ENTITY_ID);

        // 设置骑乘冷却（MC 1.16.5: 60 tick = 3秒）
        passenger.m_rideCooldown = 60;

        // 触发回调
        // MC: this.onRemovedPassenger(passenger);
    }
}

bool Entity::startRiding(Entity& vehicle) {
    // MC 1.16.5: Entity.startRiding(Entity, boolean force)

    // 不能骑乘自己
    if (vehicle.id() == m_id) {
        return false;
    }

    // MC 1.16.5: 循环检测
    // for(Entity entity = entityIn; entity.ridingEntity != null; entity = entity.ridingEntity) {
    //     if (entity.ridingEntity == this) return false;
    // }
    if (m_world) {
        Entity* current = &vehicle;
        while (current != nullptr) {
            EntityId currentVehicle = current->getVehicle();
            if (currentVehicle == INVALID_ENTITY_ID) {
                break;
            }
            if (currentVehicle == m_id) {
                // 检测到循环：车辆正在骑乘我们（直接或间接）
                return false;
            }
            current = m_world->getEntity(currentVehicle);
        }
    }

    // MC 1.16.5: 检查是否可骑乘
    // if (force || this.canBeRidden(entityIn) && entityIn.canFitPassenger(this))
    // 注意：canBeRidden() 检查的是乘客是否可以骑乘（潜行状态、冷却等）
    // 这里我们检查乘客的 canBeRidden()，而不是车辆的
    if (!canBeRidden(vehicle)) {
        return false;
    }

    if (!vehicle.canFitPassenger()) {
        return false;
    }

    // 如果已经在骑乘，先停止
    if (isRiding()) {
        stopRiding();
    }

    // 设置姿态为站立
    setPose(EntityPose::Standing);

    // 设置骑乘的车辆引用
    m_vehicle = vehicle.id();

    // 添加到车辆的乘客列表
    // 注意：MC中是先设置ridingEntity，再调用addPassenger
    // addPassenger会验证ridingEntity是否正确
    if (!vehicle.addPassenger(*this)) {
        // 添加失败，清空引用
        m_vehicle = INVALID_ENTITY_ID;
        return false;
    }

    return true;
}

void Entity::stopRiding() {
    // MC 1.16.5: Entity.stopRiding()
    dismount();
}

void Entity::dismount() {
    // MC 1.16.5: Entity.dismount()
    if (!isRiding()) {
        return;
    }

    // 获取车辆实体
    if (m_world) {
        Entity* vehicle = m_world->getEntity(m_vehicle);
        if (vehicle != nullptr) {
            // MC: this.ridingEntity = null;
            //     entity.removePassenger(this);
            // 注意：先清空vehicle引用，再调用removePassenger
            m_vehicle = INVALID_ENTITY_ID;
            vehicle->removePassenger(*this);
        }
    } else {
        m_vehicle = INVALID_ENTITY_ID;
    }
}

void Entity::removePassengers() {
    // MC 1.16.5: Entity.removePassengers()
    // 没有世界引用时无法操作乘客
    if (m_world == nullptr) {
        return;
    }

    // 从后向前遍历，避免索引问题
    for (i32 i = static_cast<i32>(m_passengers.size()) - 1; i >= 0; --i) {
        EntityId passengerId = m_passengers[i];
        Entity* passenger = m_world->getEntity(passengerId);
        if (passenger != nullptr) {
            passenger->stopRiding();
        }
    }
}

bool Entity::canBeRidden(const Entity& vehicle) const {
    // MC 1.16.5: Entity.canBeRidden(Entity)
    // 参数 vehicle 用于检查是否可以骑乘特定载具
    // 目前只检查基本条件，未来可扩展检查载具类型等
    MC_UNUSED(vehicle);
    // 默认检查：不在潜行状态 + 骑乘冷却为0
    return !isSneaking() && m_rideCooldown <= 0;
}

bool Entity::isRidingSameEntity(const Entity& other) const {
    // MC 1.16.5: Entity.isRidingSameEntity(Entity)
    return getLowestRidingEntity() == other.getLowestRidingEntity();
}

Entity* Entity::getLowestRidingEntity() {
    // MC 1.16.5: Entity.getLowestRidingEntity()
    Entity* entity = this;
    while (entity->isRiding()) {
        EntityId vehicleId = entity->getVehicle();
        if (vehicleId == INVALID_ENTITY_ID) {
            break;
        }
        // 使用当前实体的世界指针，而不是起始实体的
        IWorld* world = entity->world();
        if (world == nullptr) {
            break;
        }
        Entity* vehicle = world->getEntity(vehicleId);
        if (vehicle == nullptr) {
            break;
        }
        entity = vehicle;
    }
    return entity;
}

const Entity* Entity::getLowestRidingEntity() const {
    // MC 1.16.5: Entity.getLowestRidingEntity()
    const Entity* entity = this;
    while (entity->isRiding()) {
        EntityId vehicleId = entity->getVehicle();
        if (vehicleId == INVALID_ENTITY_ID) {
            break;
        }
        // 使用当前实体的世界指针，而不是起始实体的
        const IWorld* world = entity->world();
        if (world == nullptr) {
            break;
        }
        const Entity* vehicle = world->getEntity(vehicleId);
        if (vehicle == nullptr) {
            break;
        }
        entity = vehicle;
    }
    return entity;
}

bool Entity::isRidingOrBeingRiddenBy(const Entity& other) const {
    // MC 1.16.5: Entity.isRidingOrBeingRiddenBy(Entity)
    // 使用迭代方式避免递归栈溢出
    if (m_world == nullptr) {
        return false;
    }

    // 使用向量作为栈进行广度优先搜索
    std::vector<EntityId> toCheck(m_passengers.begin(), m_passengers.end());
    std::unordered_set<EntityId> visited;  // 防止循环

    while (!toCheck.empty()) {
        EntityId passengerId = toCheck.back();
        toCheck.pop_back();

        if (passengerId == other.id()) {
            return true;
        }

        // 防止重复检查
        if (visited.count(passengerId) > 0) {
            continue;
        }
        visited.insert(passengerId);

        // 获取乘客并检查其乘客列表
        const Entity* passenger = m_world->getEntity(passengerId);
        if (passenger != nullptr) {
            const auto& subPassengers = passenger->getPassengers();
            toCheck.insert(toCheck.end(), subPassengers.begin(), subPassengers.end());
        }
    }

    return false;
}

void Entity::detach() {
    // MC 1.16.5: Entity.detach()
    // 先移除所有乘客，再下车
    if (isBeingRidden()) {
        removePassengers();
    }
    if (isRiding()) {
        stopRiding();
    }
}

f64 Entity::getMountedYOffset() const {
    // MC 1.16.5: Entity.getMountedYOffset() -> height * 0.75D
    return static_cast<f64>(height()) * 0.75;
}

Vector3 Entity::getRidingPosition() const {
    // 默认骑乘位置在实体顶部中心
    return Vector3(m_position.x, m_position.y + static_cast<f32>(getMountedYOffset()), m_position.z);
}

void Entity::updatePassengers() {
    // MC 1.16.5: Entity.updatePassengers()
    // 效率优化：提前检查世界指针，避免在循环内重复检查
    if (m_world == nullptr) {
        return;
    }

    // 遍历所有乘客并更新位置
    for (EntityId passengerId : m_passengers) {
        Entity* passenger = m_world->getEntity(passengerId);
        if (passenger == nullptr) {
            continue;
        }

        // 更新单个乘客位置
        positionRider(*passenger);
    }
}

void Entity::positionRider(Entity& passenger) {
    // MC 1.16.5: Entity.positionRider(Entity, MoveCallback)

    if (!isPassenger(passenger.id())) {
        return;
    }

    // 计算骑乘位置
    // MC: double d0 = this.getPosY() + this.getMountedYOffset() + passenger.getYOffset();
    f64 y = static_cast<f64>(m_position.y) + getMountedYOffset() + passenger.getYOffset();

    // 设置乘客位置
    passenger.setPosition(m_position.x, static_cast<f32>(y), m_position.z);
}

void Entity::updateRidden() {
    // MC 1.16.5: Entity.updateRidden()

    // 设置速度为零
    setVelocity(Vector3(0.0f, 0.0f, 0.0f));

    // 执行tick
    if (canUpdate()) {
        tick();
    }

    // 如果还在骑乘，更新位置
    if (isRiding()) {
        Entity* vehicle = m_world ? m_world->getEntity(m_vehicle) : nullptr;
        if (vehicle != nullptr) {
            vehicle->updatePassengerPosition(*this);
        }
    }
}

void Entity::updatePassengerPosition(Entity& passenger) {
    // MC 1.16.5: Entity.updatePassenger(Entity)
    positionRider(passenger);
}

void Entity::applyOrientationToEntity(Entity& passenger) {
    // MC 1.16.5: Entity.applyOrientationToEntity(Entity)
    // 默认实现：同步旋转
    // 子类（如BoatEntity）可以重写此方法以限制旋转范围

    passenger.setRotation(m_yaw, passenger.pitch());
}

bool Entity::canPassengerSteer() const {
    // MC 1.16.5: Entity.canPassengerSteer()
    // Entity entity = this.getControllingPassenger();
    // if (entity instanceof PlayerEntity) {
    //     return ((PlayerEntity)entity).isUser();
    // } else {
    //     return !this.world.isRemote;
    // }
    EntityId controllerId = getControllingPassenger();
    if (controllerId == INVALID_ENTITY_ID) {
        return false;
    }

    // 如果有世界引用，获取控制者并检查
    if (m_world) {
        Entity* controller = m_world->getEntity(controllerId);
        if (controller != nullptr) {
            // 检查控制者是否是玩家
            if (controller->legacyType() == LegacyEntityType::Player) {
                // MC 1.16.5: return ((PlayerEntity)entity).isUser();
                // 玩家需要检查是否是本地玩家
                Player* player = dynamic_cast<Player*>(controller);
                if (player != nullptr) {
                    return player->isLocalPlayer();
                }
            }
            // 非玩家实体：服务端可以控制，客户端不能
            return !m_world->isClientSide();
        }
    }

    return false;
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

String Entity::toString() const {
    std::stringstream ss;
    ss << "Entity{id=" << m_id
       << ", type=" << getTypeId()
       << ", uuid=" << m_uuid
       << ", position=(" << m_position.x << ", " << m_position.y << ", " << m_position.z << ")"
       << ", velocity=(" << m_velocity.x << ", " << m_velocity.y << ", " << m_velocity.z << ")"
       << ", onGround=" << m_onGround
       << ", inWater=" << m_inWater
       << ", inLava=" << m_inLava
       << ", flags=" << static_cast<u32>(m_flags)
       << ", air=" << m_air
       << ", customName=\"" << (m_customName ? m_customName->getUnformattedText() : "") << "\""
       << ", customNameVisible=" << m_customNameVisible
       << ", silent=" << m_silent
       << ", noGravity=" << m_noGravity
       << ", pose=" << static_cast<u32>(m_pose)
       << "}";
    return ss.str();
}


} // namespace mc
