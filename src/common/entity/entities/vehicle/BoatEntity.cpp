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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "BoatEntity.hpp"
#include "../../../item/Items.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../util/math/MathConstants.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/BlockState.hpp"
#include "../../../world/block/registry/NaturalBlocks.hpp"
#include "../../../world/fluid/Fluid.hpp"
#include "../../../world/fluid/FluidTags.hpp"
#include "../../../world/gamerule/GameRules.hpp"
#include "../../core/DataParameter.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../entities/player/Player.hpp"
#include "../../registry/VanillaEntityTypeKeys.hpp"
#include "../../utils/ItemDropHelper.hpp"
#include "../passive/basic/AnimalEntity.hpp"
#include <cmath>

namespace mc {
namespace entity {

namespace {
constexpr f32 BUOYANCY = 0.06153846f; // 1/16.25 近似
constexpr f32 GRAVITY = 0.04f;
constexpr f32 WATER_FRICTION = 0.9f;
constexpr f32 AIR_FRICTION = 0.9f;
constexpr f32 UNDERWATER_FRICTION = 0.45f;
constexpr f32 UNDERWATER_GRAVITY = -0.0007f; // -7.0e-4D
constexpr f32 MAX_SPEED = 0.4f;
constexpr f32 WATER_SPEED_MULT = 0.04f;
constexpr i32 MAX_PASSENGERS = 2;
constexpr f32 DAMAGE_THRESHOLD = 40.0f;
constexpr i32 TIME_SINCE_HIT_DECAY = 10;
constexpr i32 OUT_OF_CONTROL_THRESHOLD = 60;  // 水下60tick踢下船
constexpr f32 PADDLE_SPEED = math::PI / 8.0f; // 每tick 22.5度

// 乘客位置常量
constexpr f32 PASSENGER_Y_OFFSET_MULT = 0.75f; // 乘客Y偏移乘数
constexpr f32 PASSENGER1_X_OFFSET = 0.2f;      // 第一乘客X偏移
constexpr f32 PASSENGER2_X_OFFSET = -0.6f;     // 第二乘客X偏移
constexpr f32 MAX_PASSENGER_ROTATION = 105.0f; // 乘客相对船的最大旋转角度

// 浮力计算常量
constexpr f32 BUOYANCY_VELOCITY_MULT = 0.75f; // 浮力速度乘数
constexpr f32 WATER_CHECK_OFFSET = 0.001f;    // 水面检测偏移
} // namespace

// ============================================================================
// 静态数据参数定义（通过 createKey 自动分配唯一 ID，避免跨类 ID 冲突）
// ============================================================================

DataParameter<i32> BoatEntity::DATA_TIME_SINCE_HIT_PARAM = EntityDataManager::createKey<i32>();
DataParameter<i32> BoatEntity::DATA_FORWARD_DIRECTION_PARAM = EntityDataManager::createKey<i32>();
DataParameter<f32> BoatEntity::DATA_DAMAGE_TAKEN_PARAM = EntityDataManager::createKey<f32>();
DataParameter<bool> BoatEntity::DATA_LEFT_PADDLE_PARAM = EntityDataManager::createKey<bool>();
DataParameter<bool> BoatEntity::DATA_RIGHT_PADDLE_PARAM = EntityDataManager::createKey<bool>();
DataParameter<i32> BoatEntity::DATA_BUBBLE_TIME_PARAM = EntityDataManager::createKey<i32>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = Entity::classInfo()）
// ============================================================================
const entity::EntityClassInfo& BoatEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"BoatEntity", &Entity::classInfo()};
    return s_classInfo;
}

std::unique_ptr<Entity> BoatEntity::create(IWorld* /*world*/)
{
    return std::make_unique<BoatEntity>();
}

BoatEntity::BoatEntity(Type type)
    : Entity(EntityInstanceId(0))
    , m_type(type)
{
    // 设置尺寸通过 width()/height()
    registerData();
}

void BoatEntity::registerData()
{
    Entity::registerData();

    // 标记当前正在注册 BoatEntity 类的字段，使 registerParam 沿继承链分配 id
    // （续接 Entity 的 id 7 之后，从 8 起）。字段顺序/类型对齐 vanilla 1.21.11
    // AbstractBoat/VehicleEntity：HURT(8,Int)/HURTDIR(9,Int)/DAMAGE(10,Float)/
    // PADDLE_LEFT(11,Bool)/PADDLE_RIGHT(12,Bool)/BUBBLE_TIME(13,Int)。
    // 船类型（m_type）由 EntityType 区分，非同步字段，不入 synched data。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    m_dataManager.registerParam(DATA_TIME_SINCE_HIT_PARAM, 0);    // HURT, 默认 0
    m_dataManager.registerParam(DATA_FORWARD_DIRECTION_PARAM, 1); // HURTDIR, 默认 1
    m_dataManager.registerParam(DATA_DAMAGE_TAKEN_PARAM, 0.0f);   // DAMAGE, 默认 0.0F
    m_dataManager.registerParam(DATA_LEFT_PADDLE_PARAM, false);   // PADDLE_LEFT
    m_dataManager.registerParam(DATA_RIGHT_PADDLE_PARAM, false);  // PADDLE_RIGHT
    m_dataManager.registerParam(DATA_BUBBLE_TIME_PARAM, 0);       // BUBBLE_TIME, 默认 0
}

ActionResultType BoatEntity::processInitialInteract(Player& player, Hand hand)
{
    // 先调用基类交互（处理拴绳等）
    ActionResultType result = Entity::processInitialInteract(player, hand);
    if (result != ActionResultType::Pass) {
        return result;
    }

    // 蹲下时不乘坐
    if (player.isSneaking()) {
        return ActionResultType::Pass;
    }

    // 船失控（水下超时）时不乘坐
    if (m_outOfControlTicks >= OUT_OF_CONTROL_THRESHOLD) {
        return ActionResultType::Pass;
    }

    // 服务端尝试让玩家乘坐
    if (m_world && !m_world->isClientSide()) {
        if (!player.startRiding(*this)) {
            return ActionResultType::Pass;
        }
    }

    return ActionResultType::Success;
}

void BoatEntity::tick()
{
    // 更新 previousStatus 和 status
    m_previousStatus = m_status;
    updateStatus();

    // 更新失控计时器
    if (m_status == BoatStatus::UnderWater || m_status == BoatStatus::UnderFlowingWater) {
        m_outOfControlTicks++;
        // 水下60tick踢下所有乘客
        if (m_outOfControlTicks >= OUT_OF_CONTROL_THRESHOLD && !m_passengers.empty()) {
            removePassengers();
        }
    } else {
        m_outOfControlTicks = 0;
    }

    // 更新损伤计时器
    if (m_timeSinceHit > 0) {
        m_timeSinceHit--;
    }
    // damageTaken 每tick减1.0
    if (m_damageTaken > 0.0f) {
        m_damageTaken -= 1.0f;
        if (m_damageTaken < 0.0f) {
            m_damageTaken = 0.0f;
        }
    }

    // 检查摔落伤害
    f64 currentYd = static_cast<f64>(m_position.y) - static_cast<f64>(m_prevPosition.y);
    if (!m_onGround && currentYd < -0.5) {
        m_lastYd = currentYd;
    }

    // 调用父类tick
    Entity::tick();

    // 更新插值
    tickLerp();

    if (canPassengerSteer()) {
        // 没有乘客或非玩家控制者时，不划桨
        if (m_passengers.empty() || (m_world && !m_world->isClientSide())) {
            setPaddleState(false, false);
        }

        // 更新运动
        updateMotion();

        // 服务端：由 PlayerInputPacket 触发 handleInput 后调用 controlBoat
        // 客户端通过 ClientApplication 直接发送 SteerBoatPacket
        if (m_world && !m_world->isClientSide()) {
            controlBoat();
        }

        // 执行移动
        move(MoverType::Self, velocity());
    } else {
        // 不可控制时，清零速度
        setVelocity(Vector3(0.0f, velocityY(), 0.0f));
    }

    // 处理气泡柱
    updateRocking();

    // 更新乘客位置
    updateAllPassengerPositions();

    // 由于 canTriggerWalking() 返回 false，需要手动调用 doBlockCollisions()
    // 用于处理气泡柱、仙人掌、甜浆果丛等方块的碰撞效果
    doBlockCollisions();

    // 更新桨动画
    for (i32 i = 0; i <= 1; ++i) {
        if (getPaddleState(i)) {
            m_paddlePositions[i] += PADDLE_SPEED;
        } else {
            m_paddlePositions[i] = 0.0f;
        }
    }
}

void BoatEntity::handleInput(bool left, bool right, bool forward, bool backward)
{
    m_leftInputDown = left;
    m_rightInputDown = right;
    m_forwardInputDown = forward;
    m_backwardInputDown = backward;
}

void BoatEntity::updateMotion()
{
    f64 gravity = -GRAVITY;
    f32 friction = 0.05f;
    f64 buoyancy = 0.0;

    switch (m_status) {
        case BoatStatus::InWater:
            friction = WATER_FRICTION;
            // 计算浮力: d2 = (waterLevel - posY) / height
            if (m_waterLevel > static_cast<f64>(m_position.y)) {
                buoyancy = (m_waterLevel - static_cast<f64>(m_position.y)) / static_cast<f64>(height());
            }
            break;
        case BoatStatus::UnderWater:
        case BoatStatus::UnderFlowingWater:
            gravity = UNDERWATER_GRAVITY;
            friction = UNDERWATER_FRICTION;
            buoyancy = 0.01;
            break;
        case BoatStatus::OnLand:
            friction = m_boatGlide;
            // 当控制乘客是玩家时，对 m_boatGlide 字段减半（非局部变量 friction）
            // MC Java 中 floatBoat() 先将 landFriction 赋给局部变量 f，再对字段 landFriction /= 2，
            // 速度乘以的是原始 f（未减半），字段减半的效果在下一 tick 被 getStatus() 覆盖。
            // 此处保持语义一致：friction 保持原始值用于速度衰减，m_boatGlide 减半作为字段副作用。
            {
                EntityInstanceId controllerId = getControllingPassenger();
                if (controllerId != INVALID_ENTITY_ID && m_world) {
                    Entity* controller = m_world->getEntity(controllerId);
                    if (controller != nullptr && controller->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
                        m_boatGlide /= 2.0f;
                    }
                }
            }
            break;
        case BoatStatus::InAir:
            friction = AIR_FRICTION;
            break;
    }

    // 应用摩擦和重力
    Vector3 vel = velocity();
    vel.x *= friction;
    vel.y += static_cast<f32>(gravity);
    vel.z *= friction;
    setVelocity(vel);

    // 应用浮力
    if (buoyancy > 0.0) {
        setVelocity(velocity().x,
            static_cast<f32>((static_cast<f64>(velocity().y) + buoyancy * BUOYANCY) * BUOYANCY_VELOCITY_MULT),
            velocity().z);
    }

    // 重置船滑行系数
    m_boatGlide = 0.0f;
}

void BoatEntity::floatBoat()
{
    // 计算浮力，需要世界引用来获取流体状态
    if (m_world == nullptr) {
        return;
    }

    // 计算水面高度
    m_waterLevel = static_cast<f64>(m_position.y);

    // 获取船的碰撞箱
    AxisAlignedBB box = boundingBox();

    // 遍历碰撞箱范围内的方块
    BlockCoord minX = static_cast<BlockCoord>(std::floor(box.minX));
    BlockCoord maxX = static_cast<BlockCoord>(std::ceil(box.maxX));
    BlockCoord minY = static_cast<BlockCoord>(std::floor(box.minY));
    BlockCoord maxY = static_cast<BlockCoord>(std::ceil(box.maxY));
    BlockCoord minZ = static_cast<BlockCoord>(std::floor(box.minZ));
    BlockCoord maxZ = static_cast<BlockCoord>(std::ceil(box.maxZ));

    for (BlockCoord x = minX; x < maxX; ++x) {
        for (BlockCoord y = minY; y < maxY; ++y) {
            for (BlockCoord z = minZ; z < maxZ; ++z) {
                BlockPos pos(x, y, z);
                const fluid::FluidState* fluid = m_world->getFluidState(pos);
                if (fluid != nullptr && !fluid->isEmpty() && fluid->getFluid().isIn(fluid::FluidTags::WATER())) {
                    // 检查是否是水
                    f32 fluidHeight = fluid->getActualHeight(*m_world, pos);
                    f32 waterY = static_cast<f32>(y) + fluidHeight;
                    if (waterY > m_waterLevel) {
                        m_waterLevel = static_cast<f64>(waterY);
                    }
                }
            }
        }
    }

    // 根据水位调整船的位置
    if (m_waterLevel > static_cast<f64>(m_position.y)) {
        // 船在水面以下，上浮
        f64 diff = m_waterLevel - static_cast<f64>(m_position.y);
        f64 lift = diff * BUOYANCY;
        setVelocity(velocity().x, velocity().y + static_cast<f32>(lift), velocity().z);
    }
}

void BoatEntity::controlBoat()
{
    if (m_passengers.empty()) {
        return;
    }

    // 前进/后退
    if (m_forwardInputDown) {
        m_speed += WATER_SPEED_MULT;
    }
    if (m_backwardInputDown) {
        m_speed -= 0.005f;
    }

    // 转向
    if (m_leftInputDown) {
        m_deltaRotation -= 1.0f;
    }
    if (m_rightInputDown) {
        m_deltaRotation += 1.0f;
    }

    // 没有前进输入时减速
    if (!m_forwardInputDown && !m_backwardInputDown) {
        m_speed *= 0.95f;
    }

    // 限制速度
    m_speed = std::max(-MAX_SPEED, std::min(MAX_SPEED, m_speed));

    // 应用转向
    m_yaw += m_deltaRotation;
    m_deltaRotation *= 0.8f;

    // 应用速度
    f32 yawRad = math::toRadians(m_yaw);
    f32 vx = -std::sin(yawRad) * m_speed;
    f32 vz = std::cos(yawRad) * m_speed;
    setVelocity(vx, velocityY(), vz);

    // 更新桨状态
    setPaddleState(m_leftInputDown || m_forwardInputDown, m_rightInputDown || m_forwardInputDown);
}

void BoatEntity::tickLerp()
{
    // 插值更新
    if (m_interpolationSteps > 0) {
        f64 lerpFactor = 1.0 / static_cast<f64>(m_interpolationSteps);
        f64 dx = m_interpolationX - static_cast<f64>(m_position.x);
        f64 dy = m_interpolationY - static_cast<f64>(m_position.y);
        f64 dz = m_interpolationZ - static_cast<f64>(m_position.z);
        f64 dYaw = m_interpolationYaw - static_cast<f64>(m_yaw);
        f64 dPitch = m_interpolationPitch - static_cast<f64>(m_pitch);

        setPosition(static_cast<f32>(static_cast<f64>(m_position.x) + dx * lerpFactor),
            static_cast<f32>(static_cast<f64>(m_position.y) + dy * lerpFactor),
            static_cast<f32>(static_cast<f64>(m_position.z) + dz * lerpFactor));
        Entity::setRotation(static_cast<f32>(static_cast<f64>(m_yaw) + dYaw * lerpFactor),
            static_cast<f32>(static_cast<f64>(m_pitch) + dPitch * lerpFactor));
        m_interpolationSteps--;
    } else {
        setVelocity(velocity());
    }
}

void BoatEntity::updateStatus()
{
    if (m_world == nullptr) {
        m_status = BoatStatus::InAir;
        return;
    }

    // 首先检查是否在水下
    BoatStatus underwaterStatus = getUnderwaterStatus();
    if (underwaterStatus != BoatStatus::InWater) {
        m_status = underwaterStatus;
        return;
    }

    // 检查是否在水面上
    if (checkInWater()) {
        m_status = BoatStatus::InWater;
        return;
    }

    // 检查是否在陆地上
    m_boatGlide = getBoatGlide();
    if (m_boatGlide > 0.0f) {
        m_status = BoatStatus::OnLand;
        return;
    }

    // 否则在空中
    m_status = BoatStatus::InAir;
}

BoatStatus BoatEntity::getUnderwaterStatus()
{
    // 检测船是否在水下
    if (m_world == nullptr) {
        return BoatStatus::InWater;
    }

    // 检查船顶是否有水
    AxisAlignedBB box = boundingBox();
    f64 checkHeight = static_cast<f64>(box.maxY) + WATER_CHECK_OFFSET;

    BlockCoord minX = static_cast<BlockCoord>(std::floor(box.minX));
    BlockCoord maxX = static_cast<BlockCoord>(std::ceil(box.maxX));
    BlockCoord minY = static_cast<BlockCoord>(std::floor(box.maxY));
    BlockCoord maxY = static_cast<BlockCoord>(std::ceil(checkHeight));
    BlockCoord minZ = static_cast<BlockCoord>(std::floor(box.minZ));
    BlockCoord maxZ = static_cast<BlockCoord>(std::ceil(box.maxZ));

    bool isSource = false;
    for (BlockCoord x = minX; x < maxX; ++x) {
        for (BlockCoord y = minY; y < maxY; ++y) {
            for (BlockCoord z = minZ; z < maxZ; ++z) {
                BlockPos pos(x, y, z);
                const fluid::FluidState* fluid = m_world->getFluidState(pos);
                if (fluid != nullptr && !fluid->isEmpty()) {
                    f32 fluidHeight = fluid->getActualHeight(*m_world, pos);
                    f32 waterY = static_cast<f32>(y) + fluidHeight;
                    if (static_cast<f64>(waterY) > checkHeight) {
                        if (!fluid->isSource()) {
                            return BoatStatus::UnderFlowingWater;
                        }
                        isSource = true;
                    }
                }
            }
        }
    }

    return isSource ? BoatStatus::UnderWater : BoatStatus::InWater;
}

bool BoatEntity::checkInWater()
{
    // 检测船是否在水中
    if (m_world == nullptr) {
        return false;
    }

    AxisAlignedBB box = boundingBox();
    bool inWater = false;

    BlockCoord minX = static_cast<BlockCoord>(std::floor(box.minX));
    BlockCoord maxX = static_cast<BlockCoord>(std::ceil(box.maxX));
    BlockCoord minY = static_cast<BlockCoord>(std::floor(box.minY));
    BlockCoord maxY = static_cast<BlockCoord>(std::floor(box.minY + WATER_CHECK_OFFSET));
    BlockCoord minZ = static_cast<BlockCoord>(std::floor(box.minZ));
    BlockCoord maxZ = static_cast<BlockCoord>(std::ceil(box.maxZ));

    for (BlockCoord x = minX; x < maxX; ++x) {
        for (BlockCoord y = minY; y <= maxY; ++y) {
            for (BlockCoord z = minZ; z < maxZ; ++z) {
                BlockPos pos(x, y, z);
                const fluid::FluidState* fluid = m_world->getFluidState(pos);
                if (fluid != nullptr && !fluid->isEmpty()) {
                    f32 fluidHeight = fluid->getActualHeight(*m_world, pos);
                    f32 waterY = static_cast<f32>(y) + fluidHeight;
                    if (waterY > m_waterLevel) {
                        m_waterLevel = static_cast<f64>(waterY);
                    }
                    if (box.minY < static_cast<f64>(waterY)) {
                        inWater = true;
                    }
                }
            }
        }
    }

    return inWater;
}

f32 BoatEntity::getBoatGlide()
{
    // 对应 MC Java AbstractBoat.getGroundFriction()
    // 遍历船底下方薄碰撞箱范围内的方块，计算平均滑度
    if (m_world == nullptr) {
        return 0.0f;
    }

    // 创建船底下方薄碰撞箱（Y 方向向下扩展 0.001）
    AxisAlignedBB boatBox = boundingBox();
    AxisAlignedBB checkBox(boatBox.minX, boatBox.minY - 0.001, boatBox.minZ, boatBox.maxX, boatBox.minY, boatBox.maxZ);

    // 扩展搜索范围：每个方向扩展 1 格
    i32 minX = static_cast<i32>(std::floor(checkBox.minX)) - 1;
    i32 maxX = static_cast<i32>(std::ceil(checkBox.maxX)) + 1;
    i32 minY = static_cast<i32>(std::floor(checkBox.minY)) - 1;
    i32 maxY = static_cast<i32>(std::ceil(checkBox.maxY)) + 1;
    i32 minZ = static_cast<i32>(std::floor(checkBox.minZ)) - 1;
    i32 maxZ = static_cast<i32>(std::ceil(checkBox.maxZ)) + 1;

    f32 totalSlipperiness = 0.0f;
    i32 count = 0;

    for (i32 x = minX; x < maxX; ++x) {
        for (i32 z = minZ; z < maxZ; ++z) {
            // 跳过角落列（MC Java 的优化：角列只检查非角落行）
            bool isEdgeX = (x == minX || x == maxX - 1);
            bool isEdgeZ = (z == minZ || z == maxZ - 1);
            if (isEdgeX && isEdgeZ) {
                continue; // 4 个角落列跳过
            }

            for (i32 y = minY; y < maxY; ++y) {
                // 角列内部的边缘行也跳过
                if (isEdgeX && (y == minY || y == maxY - 1)) {
                    continue;
                }
                if (isEdgeZ && (y == minY || y == maxY - 1)) {
                    continue;
                }

                BlockPos pos(x, y, z);
                const BlockState* state = m_world->getBlockState(pos);
                if (state == nullptr) {
                    continue;
                }

                // 跳过睡莲（MC Java 排除 WaterlilyBlock）
                if (state->is(block_registry::NaturalBlocks::LILY_PAD)) {
                    continue;
                }

                // 检查方块碰撞箱是否与船底薄碰撞箱相交
                if (!state->getCollisionShape().intersects(checkBox, x, y, z)) {
                    continue;
                }

                // 累加滑度值
                totalSlipperiness += state->getBlock().getSlipperiness(*state);
                ++count;
            }
        }
    }

    // 返回平均滑度；若无相交方块则返回 0（表示船在空中）
    return count > 0 ? totalSlipperiness / static_cast<f32>(count) : 0.0f;
}

void BoatEntity::updatePassengerPosition(Entity& passenger)
{
    // 更新单个乘客位置，委托给内部辅助方法
    updateAllPassengerPositions();
}

void BoatEntity::updateAllPassengerPositions()
{
    if (m_passengers.empty() || m_world == nullptr) {
        return;
    }

    for (size_t i = 0; i < m_passengers.size(); ++i) {
        EntityInstanceId passengerId = m_passengers[i];
        Entity* passenger = m_world->getEntity(passengerId);
        if (passenger == nullptr) {
            continue;
        }

        // 计算乘客偏移
        f32 offsetX = 0.0f;
        f32 offsetY = static_cast<f32>(static_cast<f64>(height()) * PASSENGER_Y_OFFSET_MULT + passenger->getYOffset());

        // 多乘客时位置偏移
        if (m_passengers.size() > 1) {
            if (i == 0) {
                offsetX = PASSENGER1_X_OFFSET; // 第一个乘客靠前
            } else {
                offsetX = PASSENGER2_X_OFFSET; // 第二个乘客靠后
            }
            // 动物额外偏移 +0.2D
            if (dynamic_cast<const AnimalEntity*>(passenger) != nullptr) {
                offsetX += 0.2f;
            }
        }

        // 根据船的朝向旋转偏移向量
        f32 yawRad = math::toRadians(-m_yaw) - math::PI / 2.0f;
        f32 rotatedX = offsetX * std::cos(yawRad);
        f32 rotatedZ = -offsetX * std::sin(yawRad);

        // 设置乘客位置
        passenger->setPosition(m_position.x + rotatedX, m_position.y + offsetY, m_position.z + rotatedZ);

        // 同步旋转
        passenger->setRotation(m_yaw + m_deltaRotation, passenger->pitch());

        // 应用朝向
        applyOrientationToEntity(*passenger);
    }
}

void BoatEntity::applyOrientationToEntity(Entity& passenger)
{
    // 将船的朝向应用到乘客
    passenger.setRotation(m_yaw, passenger.pitch());

    // 限制乘客相对船的旋转范围
    f32 angleDiff = math::wrapDegrees(passenger.yaw() - m_yaw);
    f32 clampedDiff = std::max(-MAX_PASSENGER_ROTATION, std::min(MAX_PASSENGER_ROTATION, angleDiff));
    passenger.setRotation(m_yaw + clampedDiff, passenger.pitch());
}

void BoatEntity::updateRocking()
{
    // 更新气泡柱摇晃
    if (m_rockingTicks > 0) {
        m_rockingTicks--;
        m_prevRockingAngle = m_rockingAngle;
        m_rockingAngle += m_rockingIntensity;
    }
}

f32 BoatEntity::getWaterLevelAbove()
{
    // 获取上方水面高度
    if (m_world == nullptr) {
        return m_position.y + 1.0f;
    }

    AxisAlignedBB box = boundingBox();
    f64 maxY = static_cast<f64>(box.maxY) - m_lastYd;
    f64 minY = static_cast<f64>(box.minY);

    for (f64 y = maxY; y >= minY; y -= 1.0) {
        BlockCoord blockY = static_cast<BlockCoord>(std::floor(y));
        f32 maxFluidHeight = 0.0f;

        BlockCoord minX = static_cast<BlockCoord>(std::floor(box.minX));
        BlockCoord maxX = static_cast<BlockCoord>(std::floor(box.maxX));
        BlockCoord minZ = static_cast<BlockCoord>(std::floor(box.minZ));
        BlockCoord maxZ = static_cast<BlockCoord>(std::floor(box.maxZ));

        for (BlockCoord x = minX; x <= maxX; ++x) {
            for (BlockCoord z = minZ; z <= maxZ; ++z) {
                BlockPos pos(x, blockY, z);
                const fluid::FluidState* fluid = m_world->getFluidState(pos);
                if (fluid != nullptr && !fluid->isEmpty()) {
                    f32 fluidHeight = fluid->getActualHeight(*m_world, pos);
                    if (fluidHeight > maxFluidHeight) {
                        maxFluidHeight = fluidHeight;
                    }
                }
            }
        }

        if (maxFluidHeight < 1.0f) {
            return static_cast<f32>(y) + maxFluidHeight;
        }
    }

    return m_position.y + 1.0f;
}

f64 BoatEntity::getMountedYOffset() const
{
    return -0.1;
}

bool BoatEntity::hurt(DamageSource& source, f32 amount)
{
    // 检查是否对伤害类型免疫
    if (isInvulnerable()) {
        return false;
    }

    // 只在服务端处理
    if (isRemoved()) {
        return true;
    }

    // 设置受击动画
    m_forwardDirection = -m_forwardDirection;

    // 设置受击时间
    m_timeSinceHit = 10;

    // 标记受伤（需要同步速度到客户端，对应 MC Java Boat.hurt() 中的 markHurt()）
    markHurt();

    // 累积伤害
    m_damageTaken += amount * 10.0f;

    // 检查是否应该摧毁船
    bool isCreative = false;
    Player* attacker = dynamic_cast<Player*>(source.source());
    if (attacker != nullptr) {
        isCreative = attacker->isCreative();
    }

    // 超过伤害阈值或创造模式时摧毁船
    if (isCreative || m_damageTaken > DAMAGE_THRESHOLD) {
        // 移除所有乘客
        removePassengers();

        // 掉落船物品
        if (!isCreative) {
            dropItem();
        }

        // 移除船
        remove();
    }

    return true;
}

void BoatEntity::updateFallState(f64 y, bool onGround)
{
    // 船没有摔落伤害，但需要更新 lastYd 用于水面检测
    MC_UNUSED(onGround);

    // 注意：BoatEntity.canTriggerWalking() 返回 false
    // 因此 BoatEntity::tick() 中需要手动调用 doBlockCollisions()

    // 如果在水中，重置摔落距离
    if (isInWater()) {
        setFallDistance(0.0f);
    }

    m_lastYd = y;
}

const Item* BoatEntity::getBoatItem() const
{
    // 返回对应木材类型的普通船物品
    // 箱子船物品由 ChestBoatEntity::getBoatItem() 重写返回
    switch (m_type) {
        case Type::OAK:
            return Items::OAK_BOAT;
        case Type::SPRUCE:
            return Items::SPRUCE_BOAT;
        case Type::BIRCH:
            return Items::BIRCH_BOAT;
        case Type::JUNGLE:
            return Items::JUNGLE_BOAT;
        case Type::ACACIA:
            return Items::ACACIA_BOAT;
        case Type::DARK_OAK:
            return Items::DARK_OAK_BOAT;
        case Type::MANGROVE:
            return Items::MANGROVE_BOAT;
        case Type::CHERRY:
            return Items::CHERRY_BOAT;
        case Type::PALE_OAK:
            return Items::PALE_OAK_BOAT;
        case Type::BAMBOO:
            return Items::BAMBOO_RAFT;
        default:
            return Items::OAK_BOAT;
    }
}

void BoatEntity::dropItem()
{
    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        return;
    }

    // 检查游戏规则 doEntityDrops：当该规则为 false 时，船被摧毁不产生掉落物品
    // 参考 VehicleEntity.destroy() 中的 ENTITY_DROPS 检查
    if (!worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
        return;
    }

    // 获取对应的船物品
    const Item* boatItem = getBoatItem();
    if (boatItem == nullptr) {
        return;
    }

    // 创建物品堆
    ItemStack stack(*boatItem, 1);

    // 如果船有自定义名称，设置到物品上
    if (hasCustomName()) {
        stack.setCustomName(customNameText());
    }

    // 使用 ItemDropHelper 在船的位置生成物品实体
    math::Random& rng = worldPtr->getRandom();
    ItemDropHelper::spawnItemEntity(worldPtr, stack, x(), y(), z(), rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
}

void BoatEntity::dropItemWithDamage()
{
    // 掉落船物品（带伤害倍率）
    dropItem();
}

} // namespace entity
} // namespace mc
