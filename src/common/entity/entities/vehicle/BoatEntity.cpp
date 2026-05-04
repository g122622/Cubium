#include "BoatEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../util/math/MathConstants.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../world/fluid/Fluid.hpp"
#include "../../../world/fluid/FluidTags.hpp"
#include <cmath>

namespace mc {
namespace entity {

// MC 1.16.5 常量
namespace {
    constexpr f32 BUOYANCY = 0.06153846f;  // 1/16.25 近似
    constexpr f32 GRAVITY = 0.04f;
    constexpr f32 WATER_FRICTION = 0.9f;
    constexpr f32 AIR_FRICTION = 0.9f;
    constexpr f32 LAND_FRICTION = 0.5f;
    constexpr f32 UNDERWATER_FRICTION = 0.45f;
    constexpr f32 UNDERWATER_GRAVITY = -0.0007f;  // -7.0e-4D
    constexpr f32 MAX_SPEED = 0.4f;
    constexpr f32 WATER_SPEED_MULT = 0.04f;
    constexpr i32 MAX_PASSENGERS = 2;
    constexpr f32 DAMAGE_THRESHOLD = 40.0f;
    constexpr i32 TIME_SINCE_HIT_DECAY = 10;
    constexpr i32 OUT_OF_CONTROL_THRESHOLD = 60;  // 水下60tick踢下船
    constexpr f32 PADDLE_SPEED = math::PI / 8.0f;  // 每tick 22.5度

    // 乘客位置常量 (MC 1.16.5)
    constexpr f32 PASSENGER_Y_OFFSET_MULT = 0.75f;  // 乘客Y偏移乘数
    constexpr f32 PASSENGER1_X_OFFSET = 0.2f;       // 第一乘客X偏移
    constexpr f32 PASSENGER2_X_OFFSET = -0.6f;      // 第二乘客X偏移
    constexpr f32 MAX_PASSENGER_ROTATION = 105.0f;  // 乘客相对船的最大旋转角度

    // 浮力计算常量
    constexpr f32 BUOYANCY_VELOCITY_MULT = 0.75f;  // 浮力速度乘数
    constexpr f32 WATER_CHECK_OFFSET = 0.001f;     // 水面检测偏移
}

BoatEntity::BoatEntity(Type type)
    : Entity(LegacyEntityType::Boat, EntityId(0))
    , m_type(type)
{
    // MC 1.16.5: preventEntitySpawning = true
    // 设置尺寸通过 width()/height()
}

void BoatEntity::tick() {
    // MC 1.16.5 BoatEntity.tick()

    // 更新损伤计时器
    if (m_timeSinceHit > 0) {
        m_timeSinceHit--;
    }
    if (m_damageTaken > 0.0f) {
        m_damageTaken -= 0.05f;
        if (m_damageTaken < 0.0f) {
            m_damageTaken = 0.0f;
        }
    }

    // 更新失控计时器
    BoatStatus prevStatus = m_status;
    updateStatus();

    // 检查摔落伤害
    f64 currentYd = static_cast<f64>(m_position.y) - static_cast<f64>(m_prevPosition.y);
    if (!m_onGround && currentYd < -0.5) {
        m_lastYd = currentYd;
    }

    // 更新状态
    m_previousStatus = prevStatus;

    // 处理气泡柱
    updateRocking();

    // 更新插值
    tickLerp();

    // 计算浮力 (MC 1.16.5: 在tickLerp之后调用)
    floatBoat();

    // 更新运动
    updateMotion();

    // 控制船
    controlBoat();

    // 更新乘客位置
    updatePassengerPosition();

    // 更新失控状态
    if (m_status == BoatStatus::UnderWater || m_status == BoatStatus::UnderFlowingWater) {
        m_outOfControlTicks++;
        if (m_outOfControlTicks >= OUT_OF_CONTROL_THRESHOLD && !m_passengers.empty()) {
            // 水下60tick踢下所有乘客
            // MC: this.removePassengers();
            // 需要世界引用来执行移除操作
        }
    } else {
        m_outOfControlTicks = 0;
    }

    // 更新桨动画
    for (i32 i = 0; i <= 1; ++i) {
        if (getPaddleState(i)) {
            m_paddlePositions[i] += PADDLE_SPEED;
        } else {
            m_paddlePositions[i] = 0.0f;
        }
    }

    // 调用父类tick
    Entity::tick();
}

void BoatEntity::handleInput(bool left, bool right, bool forward, bool backward) {
    // MC 1.16.5: 设置输入状态
    m_leftInputDown = left;
    m_rightInputDown = right;
    m_forwardInputDown = forward;
    m_backwardInputDown = backward;
}

void BoatEntity::dropItem() {
    // MC 1.16.5: 掉落对应类型的船物品
    // TODO: 根据类型掉落物品
    // ItemEntity* item = spawnItem(Items::getBoat(m_type));
}

void BoatEntity::updateMotion() {
    // MC 1.16.5 BoatEntity.updateMotion()
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
            if (m_forwardInputDown) {
                friction /= 2.0f;
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

void BoatEntity::floatBoat() {
    // MC 1.16.5: 计算浮力
    // 需要世界引用来获取流体状态
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
                if (fluid != nullptr && !fluid->isEmpty()) {
                    // 检查是否是水
                    // TODO: 使用FluidTags::WATER
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

void BoatEntity::controlBoat() {
    // MC 1.16.5 BoatEntity.controlBoat()
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
        m_deltaRotation -= 0.1f;
    }
    if (m_rightInputDown) {
        m_deltaRotation += 0.1f;
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
    setPaddleState(m_leftInputDown || m_forwardInputDown,
                   m_rightInputDown || m_forwardInputDown);
}

void BoatEntity::tickLerp() {
    // MC 1.16.5: 插值更新
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

void BoatEntity::updateStatus() {
    // MC 1.16.5: 更新船的状态
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

BoatStatus BoatEntity::getUnderwaterStatus() {
    // MC 1.16.5: 检测船是否在水下
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

bool BoatEntity::checkInWater() {
    // MC 1.16.5: 检测船是否在水中
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

f32 BoatEntity::getBoatGlide() {
    // MC 1.16.5: 计算地面滑动系数
    if (m_world == nullptr) {
        return 0.0f;
    }

    // 简化实现：返回默认陆地摩擦系数
    // 完整实现需要遍历碰撞箱内的方块并获取其滑度
    return LAND_FRICTION;
}

void BoatEntity::updatePassengerPosition() {
    // MC 1.16.5: 更新乘客位置
    if (m_passengers.empty() || m_world == nullptr) {
        return;
    }

    for (size_t i = 0; i < m_passengers.size(); ++i) {
        EntityId passengerId = m_passengers[i];
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
                offsetX = PASSENGER1_X_OFFSET;  // 第一个乘客靠前
            } else {
                offsetX = PASSENGER2_X_OFFSET;  // 第二个乘客靠后
            }
            // 动物额外偏移
            // if (passenger->isAnimal()) {
            //     offsetX += 0.2f;
            // }
        }

        // 根据船的朝向旋转偏移向量
        f32 yawRad = math::toRadians(-m_yaw) - math::PI / 2.0f;
        f32 rotatedX = offsetX * std::cos(yawRad);
        f32 rotatedZ = -offsetX * std::sin(yawRad);

        // 设置乘客位置
        passenger->setPosition(
            m_position.x + rotatedX,
            m_position.y + offsetY,
            m_position.z + rotatedZ
        );

        // 同步旋转
        passenger->setRotation(m_yaw + m_deltaRotation, passenger->pitch());

        // 应用朝向
        applyOrientationToEntity(*passenger);
    }
}

void BoatEntity::applyOrientationToEntity(Entity& passenger) {
    // MC 1.16.5: 将船的朝向应用到乘客
    passenger.setRotation(m_yaw, passenger.pitch());

    // 限制乘客相对船的旋转范围
    f32 angleDiff = math::wrapDegrees(passenger.yaw() - m_yaw);
    f32 clampedDiff = std::max(-MAX_PASSENGER_ROTATION, std::min(MAX_PASSENGER_ROTATION, angleDiff));
    passenger.setRotation(m_yaw + clampedDiff, passenger.pitch());
}

void BoatEntity::updateRocking() {
    // MC 1.16.5: 更新气泡柱摇晃
    if (m_rockingTicks > 0) {
        m_rockingTicks--;
        m_prevRockingAngle = m_rockingAngle;
        m_rockingAngle += m_rockingIntensity;
    }
}

f32 BoatEntity::getWaterLevelAbove() {
    // MC 1.16.5: 获取上方水面高度
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

f64 BoatEntity::getMountedYOffset() const {
    // MC 1.16.5: BoatEntity.getMountedYOffset() -> -0.1D
    return -0.1;
}

} // namespace entity
} // namespace mc
