#include "MinecartEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../player/Player.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc {
namespace entity {

using namespace mc::math;

// ============================================================================
// AbstractMinecartEntity
// ============================================================================

AbstractMinecartEntity::AbstractMinecartEntity(Type type, EntityId id)
    : Entity(LegacyEntityType::Minecart, id)
    , m_type(type)
{
    // MC 1.16.5: 矿车默认属性
}

AbstractMinecartEntity::AbstractMinecartEntity(Type type)
    : Entity(LegacyEntityType::Minecart, EntityId(0))
    , m_type(type)
{
}

void AbstractMinecartEntity::tick() {
    // MC 1.16.5 AbstractMinecartEntity.tick()

    // 更新摇晃动画
    updateRollingAnimation();

    // 减少损坏值
    if (m_damage > 0) {
        m_damage--;
    }

    // 检查铁轨状态
    checkRailState();

    if (m_onRail) {
        // 在铁轨上移动
        // TODO: 获取铁轨方块状态
        // BlockState& state = world->getBlockState(m_railPos);
        // moveAlongTrack(m_railPos, state);
    } else {
        // 脱轨移动
        moveDerailedMinecart();
    }

    // 处理碰撞
    handleEntityCollisions();
    handleMinecartCollisions();

    // 更新朝向
    updateRotation();

    // 调用父类tick
    Entity::tick();
}

bool AbstractMinecartEntity::isOnRailAt(const BlockPos& pos) const {
    // TODO: 检查指定位置是否有铁轨
    // BlockState* state = world->getBlockState(pos);
    // return state && state->getBlock()->isRail();
    MC_UNUSED(pos);
    return false;
}

void AbstractMinecartEntity::checkRailState() {
    // MC 1.16.5: 检查当前位置或下方一格是否有铁轨
    BlockPos currentPos(
        static_cast<BlockCoord>(std::floor(x())),
        static_cast<BlockCoord>(std::floor(y())),
        static_cast<BlockCoord>(std::floor(z()))
    );

    // 检查当前方块
    if (isOnRailAt(currentPos)) {
        m_onRail = true;
        m_railPos = currentPos;
        // TODO: 获取铁轨形状
        // m_railShape = getRailShapeFromBlock(state);
        return;
    }

    // 检查下方一格
    BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
    if (isOnRailAt(belowPos)) {
        m_onRail = true;
        m_railPos = belowPos;
        // TODO: 获取铁轨形状
        return;
    }

    m_onRail = false;
}

void AbstractMinecartEntity::moveAlongTrack(const BlockPos& pos) {
    // MC 1.16.5: moveAlongTrack()
    MC_UNUSED(pos);

    // 获取铁轨形状和方向
    auto [dir1, dir2] = getRailDirectionVectors(m_railShape);

    // 计算速度分量
    f64 vx = velocityX();
    f64 vz = velocityZ();
    f64 speed = std::sqrt(vx * vx + vz * vz);

    // 斜坡调整
    if (m_railShape == RailShape::AscendingEast ||
        m_railShape == RailShape::AscendingWest ||
        m_railShape == RailShape::AscendingNorth ||
        m_railShape == RailShape::AscendingSouth) {
        // 斜坡上有额外的重力分量
        f64 adjustment = getSlopeAdjustment();
        switch (m_railShape) {
            case RailShape::AscendingEast:
                vx -= adjustment;
                break;
            case RailShape::AscendingWest:
                vx += adjustment;
                break;
            case RailShape::AscendingNorth:
                vz += adjustment;
                break;
            case RailShape::AscendingSouth:
                vz -= adjustment;
                break;
            default:
                break;
        }
    }

    // 速度限制
    f64 maxSpeed = static_cast<f64>(getMaxSpeed());
    if (speed > maxSpeed) {
        f64 scale = maxSpeed / speed;
        vx *= scale;
        vz *= scale;
    }

    // 动力铁轨加速/减速
    if (isPoweredRail(m_railPos)) {
        if (isRailPowered(m_railPos)) {
            // 充能的动力铁轨加速
            if (speed > 0.01) {
                vx += (vx / speed) * POWERED_RAIL_BOOST;
                vz += (vz / speed) * POWERED_RAIL_BOOST;
            } else {
                // 静止矿车寻找方向
                // TODO: 检查周围方块确定推动方向
            }
        } else {
            // 未充能的动力铁轨减速
            if (speed < UNPOWERED_RAIL_THRESHOLD) {
                vx = 0;
                vz = 0;
            } else {
                vx *= 0.5;
                vz *= 0.5;
            }
        }
    }

    // 激活铁轨
    if (isActivatorRail(m_railPos)) {
        onActivatorRailPass(m_railPos.x, m_railPos.y, m_railPos.z, isRailPowered(m_railPos));
    }

    // 探测铁轨
    // TODO: 探测铁轨检测矿车并通过红石信号输出

    // 应用摩擦力
    applyDrag();

    // 执行移动
    move(static_cast<f32>(vx), velocityY(), static_cast<f32>(vz));
}

void AbstractMinecartEntity::moveDerailedMinecart() {
    // MC 1.16.5: moveDerailedMinecart()
    f64 vx = velocityX();
    f64 vy = velocityY();
    f64 vz = velocityZ();

    // 重力
    vy -= 0.04;

    // 速度限制
    f64 maxSpeed = onGround() ? static_cast<f64>(getMaxSpeed()) : static_cast<f64>(getMaxSpeedAirLateral());
    vx = std::clamp(vx, -maxSpeed, maxSpeed);
    vz = std::clamp(vz, -maxSpeed, maxSpeed);

    // 地面摩擦
    if (onGround()) {
        vx *= 0.5;
        vz *= 0.5;
    }

    // 垂直速度限制
    f32 maxVert = getMaxSpeedAirVertical();
    if (maxVert > 0 && vy > maxVert) {
        if (std::abs(vx) < 0.3 && std::abs(vz) < 0.3) {
            vy = 0.15;
        } else {
            vy = maxVert;
        }
    }

    setVelocity(static_cast<f32>(vx), static_cast<f32>(vy), static_cast<f32>(vz));

    move(static_cast<f32>(vx), static_cast<f32>(vy), static_cast<f32>(vz));

    // 空气阻力
    if (!onGround()) {
        f32 drag = getDragAir();
        setVelocity(velocityX() * drag, velocityY(), velocityZ() * drag);
    }
}

std::pair<Vector3, Vector3> AbstractMinecartEntity::getRailDirectionVectors(RailShape shape) const {
    // MC 1.16.5: MATRIX 映射
    // 返回铁轨的两个端点方向
    switch (shape) {
        case RailShape::NorthSouth:
            return { Vector3(0, 0, -1), Vector3(0, 0, 1) };  // 南北
        case RailShape::EastWest:
            return { Vector3(-1, 0, 0), Vector3(1, 0, 0) };  // 东西
        case RailShape::AscendingEast:
            return { Vector3(-1, 0, 0), Vector3(1, 1, 0) };  // 向东上坡
        case RailShape::AscendingWest:
            return { Vector3(-1, 1, 0), Vector3(1, 0, 0) };  // 向西上坡
        case RailShape::AscendingNorth:
            return { Vector3(0, 0, -1), Vector3(0, 1, 1) };  // 向北上坡
        case RailShape::AscendingSouth:
            return { Vector3(0, 1, -1), Vector3(0, 0, 1) };  // 向南上坡
        case RailShape::SouthEast:
            return { Vector3(0, 0, 1), Vector3(1, 0, 0) };   // 东南弯道
        case RailShape::SouthWest:
            return { Vector3(-1, 0, 0), Vector3(0, 0, 1) };  // 西南弯道
        case RailShape::NorthWest:
            return { Vector3(-1, 0, 0), Vector3(0, 0, -1) }; // 西北弯道
        case RailShape::NorthEast:
            return { Vector3(0, 0, -1), Vector3(1, 0, 0) };  // 东北弯道
        default:
            return { Vector3(0, 0, -1), Vector3(0, 0, 1) };
    }
}

Vector3 AbstractMinecartEntity::getPosOnRail(f64 x, f64 y, f64 z) const {
    // MC 1.16.5: getPos()
    // 将矿车位置贴靠到铁轨中心线

    auto [dir1, dir2] = getRailDirectionVectors(m_railShape);

    // 铁轨中心点
    f64 railX = static_cast<f64>(m_railPos.x) + 0.5;
    f64 railY = static_cast<f64>(m_railPos.y);
    f64 railZ = static_cast<f64>(m_railPos.z) + 0.5;

    // 计算铁轨两端点
    f64 x1 = railX + dir1.x;
    f64 y1 = railY + dir1.y;
    f64 z1 = railZ + dir1.z;

    f64 x2 = railX + dir2.x;
    f64 y2 = railY + dir2.y;
    f64 z2 = railZ + dir2.z;

    // 投影到线段上
    f64 dx = x2 - x1;
    f64 dy = y2 - y1;
    f64 dz = z2 - z1;
    f64 lenSq = dx * dx + dy * dy + dz * dz;

    if (lenSq < 0.0001) {
        return Vector3(static_cast<f32>(railX), static_cast<f32>(railY), static_cast<f32>(railZ));
    }

    f64 t = ((x - x1) * dx + (y - y1) * dy + (z - z1) * dz) / lenSq;
    t = std::clamp(t, 0.0, 1.0);

    return Vector3(
        static_cast<f32>(x1 + t * dx),
        static_cast<f32>(y1 + t * dy),
        static_cast<f32>(z1 + t * dz)
    );
}

void AbstractMinecartEntity::applyDrag() {
    // MC 1.16.5: applyDrag()
    // 摩擦力取决于是否有乘客
    f64 drag = getPassengers().empty() ? EMPTY_DRAG : OCCUPIED_DRAG;
    setVelocity(
        velocityX() * static_cast<f32>(drag),
        velocityY(),
        velocityZ() * static_cast<f32>(drag)
    );
}

void AbstractMinecartEntity::handleEntityCollisions() {
    // MC 1.16.5: collideWithEntities()
    // 推动实体
    // TODO: 获取附近实体并推动
}

void AbstractMinecartEntity::handleMinecartCollisions() {
    // MC 1.16.5: 矿车间碰撞
    // TODO: 检测其他矿车并处理碰撞
}

void AbstractMinecartEntity::updateRollingAnimation() {
    // MC 1.16.5: 摇晃动画
    if (m_rollingAmplitude > 0) {
        m_rollingAmplitude--;
    }
}

void AbstractMinecartEntity::updateRotation() {
    // MC 1.16.5: 根据移动方向更新朝向
    f32 vx = velocityX();
    f32 vz = velocityZ();
    f32 speedSq = vx * vx + vz * vz;

    if (speedSq > 0.0001) {
        f32 targetYaw = std::atan2(vx, vz) * (180.0f / PI);

        // 处理180度转向
        f32 yawDiff = targetYaw - yaw();
        while (yawDiff < -180.0f) yawDiff += 360.0f;
        while (yawDiff > 180.0f) yawDiff -= 360.0f;

        // 检测是否需要翻转
        if (yawDiff > 90.0f || yawDiff < -90.0f) {
            m_flipped = !m_flipped;
        }

        setRotation(targetYaw, pitch());
    }
}

bool AbstractMinecartEntity::isPoweredRail(const BlockPos& pos) const {
    // TODO: 检查是否为动力铁轨
    MC_UNUSED(pos);
    return false;
}

bool AbstractMinecartEntity::isDetectorRail(const BlockPos& pos) const {
    // TODO: 检查是否为探测铁轨
    MC_UNUSED(pos);
    return false;
}

bool AbstractMinecartEntity::isActivatorRail(const BlockPos& pos) const {
    // TODO: 检查是否为激活铁轨
    MC_UNUSED(pos);
    return false;
}

bool AbstractMinecartEntity::isRailPowered(const BlockPos& pos) const {
    // TODO: 检查铁轨是否接收红石信号
    MC_UNUSED(pos);
    return false;
}

void AbstractMinecartEntity::dropItem() {
    // MC 1.16.5: killMinecart()
    // TODO: 根据类型掉落对应物品
    remove();
}

void AbstractMinecartEntity::activate() {
    // 默认无操作，子类重写
}

void AbstractMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) {
    // 默认无操作，子类重写
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);
    MC_UNUSED(powered);
}

void AbstractMinecartEntity::applyForce(f32 x, f32 z) {
    addVelocity(x, 0.0f, z);
}

// ============================================================================
// RideableMinecartEntity
// ============================================================================

void RideableMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) {
    // MC 1.16.5: 激活铁轨弹出乘客
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);

    if (powered) {
        // 弹出所有乘客
        // TODO: removePassengers();
    }
}

// ============================================================================
// ChestMinecartEntity
// ============================================================================

void ChestMinecartEntity::applyDrag() {
    // MC 1.16.5: 箱子矿车摩擦力
    // 根据红石信号强度增加摩擦
    f32 drag = 0.98f;

    // TODO: 计算红石信号强度
    // int signal = Container.calcRedstoneFromInventory(this);
    // drag += signal * 0.001f;

    setVelocity(
        velocityX() * drag,
        velocityY(),
        velocityZ() * drag
    );
}

// ============================================================================
// FurnaceMinecartEntity
// ============================================================================

void FurnaceMinecartEntity::tick() {
    AbstractMinecartEntity::tick();

    // MC 1.16.5: 燃料消耗
    if (m_fuel > 0) {
        m_fuel--;
    }
}

void FurnaceMinecartEntity::applyDrag() {
    // MC 1.16.5: 熔炉矿车摩擦力
    f64 pushMag = m_pushX * m_pushX + m_pushZ * m_pushZ;

    if (pushMag > 1.0e-7) {
        pushMag = std::sqrt(pushMag);
        m_pushX /= static_cast<f32>(pushMag);
        m_pushZ /= static_cast<f32>(pushMag);

        f32 drag = 0.8f;
        setVelocity(
            velocityX() * drag + m_pushX * 0.1f,
            velocityY(),
            velocityZ() * drag + m_pushZ * 0.1f
        );
    } else {
        f32 drag = 0.98f;
        setVelocity(
            velocityX() * drag,
            velocityY(),
            velocityZ() * drag
        );
    }

    AbstractMinecartEntity::applyDrag();
}

void FurnaceMinecartEntity::activate() {
    // MC 1.16.5: 添加燃料
    addFuel(3600); // 3分钟 = 180秒 * 20 ticks/秒
}

// ============================================================================
// TNTMinecartEntity
// ============================================================================

void TNTMinecartEntity::tick() {
    AbstractMinecartEntity::tick();

    // MC 1.16.5: TNT引信倒计时
    if (m_fuse > 0) {
        m_fuse--;
        if (m_fuse <= 0) {
            explode();
        }
    }
}

void TNTMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) {
    // MC 1.16.5: 激活铁轨点燃TNT
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);

    if (powered && m_fuse < 0) {
        prime();
    }
}

void TNTMinecartEntity::explode() {
    // TODO: 爆炸逻辑
    // world->createExplosion(position(), 4.0f);
    remove();
}

// ============================================================================
// HopperMinecartEntity
// ============================================================================

void HopperMinecartEntity::tick() {
    AbstractMinecartEntity::tick();

    // MC 1.16.5: 吸取物品冷却
    if (m_suckCooldown > 0) {
        m_suckCooldown--;
    }

    // TODO: 检测附近物品并吸取
    // TODO: 向下方的容器传输物品
}

// ============================================================================
// CommandBlockMinecartEntity
// ============================================================================

void CommandBlockMinecartEntity::tick() {
    AbstractMinecartEntity::tick();

    // MC 1.16.5: 命令方块矿车每tick执行命令（如果激活）
    // TODO: 检查是否激活并执行命令
}

void CommandBlockMinecartEntity::executeCommand() {
    // TODO: 执行命令
    // CommandSource source = CommandSource::fromEntity(this);
    // m_successCount = world->getServer()->getCommandManager()->execute(m_command, source);
}

} // namespace entity
} // namespace mc
