#include "MinecartEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/blocks/redstone/DetectorRailBlock.hpp"
#include "../../../world/block/blocks/redstone/ActivatorRailBlock.hpp"
#include "../../../world/blockentity/core/SimpleInventory.hpp"
#include "../../../world/blockentity/transport/HopperEntity.hpp"
#include "../../../world/explosion/Explosion.hpp"
#include "../../../world/explosion/ExplosionMode.hpp"
#include "../../../world/redstone/RedstonePower.hpp"
#include "../../../item/Items.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../player/Player.hpp"
#include "../item/ItemEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../inventory/IInventory.hpp"
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
        moveAlongTrack(m_railPos);
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
    // MC 1.16.5: 检查指定位置是否有铁轨
    const IWorld* worldPtr = world();
    if (!worldPtr) {
        return false;
    }

    const BlockState* state = worldPtr->getBlockState(pos);
    if (!state) {
        return false;
    }

    // 检查是否为铁轨类方块
    return state->is(VanillaBlocks::RAIL) ||
           state->is(VanillaBlocks::POWERED_RAIL) ||
           state->is(VanillaBlocks::DETECTOR_RAIL) ||
           state->is(VanillaBlocks::ACTIVATOR_RAIL);
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

        // 获取铁轨形状
        IWorld* worldPtr = Entity::world();
        if (worldPtr) {
            const BlockState* state = worldPtr->getBlockState(currentPos);
            if (state) {
                // 从方块状态获取铁轨形状
                // 这里简化处理，实际需要从 AbstractRailBlock 获取
                // m_railShape = getRailShapeFromState(state);
            }
        }
        return;
    }

    // 检查下方一格
    BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
    if (isOnRailAt(belowPos)) {
        m_onRail = true;
        m_railPos = belowPos;
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
    if (blocks::isAscending(m_railShape)) {
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
                // 检查周围方块确定推动方向
                IWorld* worldPtr = Entity::world();
                if (worldPtr) {
                    // 简化处理：根据铁轨方向推动
                    vx = dir2.x * 0.1;
                    vz = dir2.z * 0.1;
                }
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
    // 探测铁轨检测矿车并通过红石信号输出（由方块本身处理）

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
    // 同时处理实体推动和矿车间碰撞，避免重复查询

    IWorld* worldPtr = Entity::world();
    if (!worldPtr) {
        return;
    }

    // 获取矿车碰撞箱（仅查询一次）
    AxisAlignedBB box = boundingBox().expand(0.2f, 0.0f, 0.2f);
    std::vector<Entity*> entities = worldPtr->getEntitiesInAABB(box, this);

    for (Entity* entity : entities) {
        if (!entity || entity->isRemoved()) {
            continue;
        }

        LegacyEntityType entityType = entity->legacyType();

        // 跳过玩家（玩家碰撞由其他逻辑处理）
        if (entityType == LegacyEntityType::Player) {
            continue;
        }

        // 矿车间碰撞处理
        if (entityType == LegacyEntityType::Minecart) {
            AbstractMinecartEntity* otherCart = static_cast<AbstractMinecartEntity*>(entity);

            // 计算碰撞方向
            f32 dx = static_cast<f32>(otherCart->x() - x());
            f32 dz = static_cast<f32>(otherCart->z() - z());
            f32 dist = std::sqrt(dx * dx + dz * dz);

            if (dist > 0.0f) {
                // 归一化方向
                dx /= dist;
                dz /= dist;

                // 检查是否平行（用于判断是追尾还是侧面碰撞）
                f32 dotProduct = velocityX() * otherCart->velocityX() + velocityZ() * otherCart->velocityZ();
                bool parallel = dotProduct > 0.8f; // 大致同向

                if (parallel) {
                    // 追尾碰撞：平均速度
                    f32 avgVx = (velocityX() + otherCart->velocityX()) * 0.5f;
                    f32 avgVz = (velocityZ() + otherCart->velocityZ()) * 0.5f;
                    setVelocity(avgVx, velocityY(), avgVz);
                    otherCart->setVelocity(avgVx, otherCart->velocityY(), avgVz);
                } else {
                    // 侧面碰撞：推开
                    constexpr f32 COLLISION_STRENGTH = 0.2f;
                    setVelocity(-dx * COLLISION_STRENGTH, velocityY(), -dz * COLLISION_STRENGTH);
                    otherCart->setVelocity(dx * COLLISION_STRENGTH, otherCart->velocityY(), dz * COLLISION_STRENGTH);
                }
            }
            continue;
        }

        // 其他实体：推动
        f32 dx = static_cast<f32>(entity->x() - x());
        f32 dz = static_cast<f32>(entity->z() - z());
        f32 dist = std::sqrt(dx * dx + dz * dz);

        if (dist > 0.0f) {
            dx /= dist;
            dz /= dist;

            // 推动速度
            constexpr f32 PUSH_STRENGTH = 0.1f;
            entity->addVelocity(dx * PUSH_STRENGTH, 0.0f, dz * PUSH_STRENGTH);
        }
    }
}

void AbstractMinecartEntity::handleMinecartCollisions() {
    // 已在 handleEntityCollisions() 中处理
    // 此方法保留为空以保持API兼容性
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
    // 检查是否为动力铁轨
    const IWorld* worldPtr = world();
    if (!worldPtr) {
        return false;
    }

    const BlockState* state = worldPtr->getBlockState(pos);
    if (!state) {
        return false;
    }

    return state->is(VanillaBlocks::POWERED_RAIL);
}

bool AbstractMinecartEntity::isDetectorRail(const BlockPos& pos) const {
    // 检查是否为探测铁轨
    const IWorld* worldPtr = world();
    if (!worldPtr) {
        return false;
    }

    const BlockState* state = worldPtr->getBlockState(pos);
    if (!state) {
        return false;
    }

    return state->is(VanillaBlocks::DETECTOR_RAIL);
}

bool AbstractMinecartEntity::isActivatorRail(const BlockPos& pos) const {
    // 检查是否为激活铁轨
    const IWorld* worldPtr = world();
    if (!worldPtr) {
        return false;
    }

    const BlockState* state = worldPtr->getBlockState(pos);
    if (!state) {
        return false;
    }

    return state->is(VanillaBlocks::ACTIVATOR_RAIL);
}

bool AbstractMinecartEntity::isRailPowered(const BlockPos& pos) {
    // 检查铁轨是否接收红石信号
    IWorld* worldPtr = Entity::world();
    if (!worldPtr) {
        return false;
    }

    return world::redstone::RedstonePower::isPowered(*worldPtr, pos);
}

void AbstractMinecartEntity::dropItem() {
    // MC 1.16.5: killMinecart()
    // 根据矿车类型掉落对应物品

    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        remove();
        return;
    }

    // 获取对应的矿车物品
    const Item* minecartItem = nullptr;
    switch (m_type) {
        case Type::Rideable:
            minecartItem = Items::MINECART;
            break;
        case Type::Chest:
            minecartItem = Items::CHEST_MINECART;
            break;
        case Type::Furnace:
            minecartItem = Items::FURNACE_MINECART;
            break;
        case Type::TNT:
            minecartItem = Items::TNT_MINECART;
            break;
        case Type::Hopper:
            minecartItem = Items::HOPPER_MINECART;
            break;
        case Type::CommandBlock:
            minecartItem = Items::COMMAND_BLOCK_MINECART;
            break;
        case Type::Spawner:
            // Spawner minecart 不掉落物品（MC 1.16.5）
            remove();
            return;
    }

    if (minecartItem == nullptr) {
        remove();
        return;
    }

    // 创建物品堆
    ItemStack stack(*minecartItem, 1);

    // 如果矿车有自定义名称，设置到物品上
    if (hasCustomName()) {
        stack.setCustomName(customNameText());
    }

    // 创建物品实体
    // MC 1.16.5: entityDropItem(stack) - 在实体位置生成物品
    thread_local math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
    f32 vx = rng.nextFloat(-0.1f, 0.1f);
    f32 vy = 0.2f;  // 轻微向上
    f32 vz = rng.nextFloat(-0.1f, 0.1f);

    auto itemEntity = std::make_unique<ItemEntity>(
        EntityId(0),
        stack,
        x(), y(), z(),
        vx, vy, vz
    );

    // 设置拾取延迟，防止立即被玩家拾取
    itemEntity->setPickupDelay(10);

    // 生成实体
    worldPtr->spawnEntity(std::move(itemEntity));

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
        const auto& passengerIds = getPassengers();
        IWorld* worldPtr = Entity::world();
        if (worldPtr) {
            for (EntityId passengerId : passengerIds) {
                Entity* passenger = worldPtr->getEntity(passengerId);
                if (passenger) {
                    // 停止骑行
                    passenger->stopRiding();
                }
            }
        }
    }
}

// ============================================================================
// ChestMinecartEntity
// ============================================================================

ChestMinecartEntity::ChestMinecartEntity(EntityId id)
    : AbstractMinecartEntity(Type::Chest, id)
    , m_inventory(std::make_unique<blockentity::SimpleInventory>(INVENTORY_SIZE))
{
}

void ChestMinecartEntity::applyDrag() {
    // MC 1.16.5: 箱子矿车摩擦力
    // 根据红石信号强度增加摩擦
    f32 drag = 0.98f;

    // MC 1.16.5: 根据容器红石信号强度增加摩擦力
    // int signal = Container.calcRedstoneFromInventory(this);
    // drag -= signal * 0.001f;
    // 目前简化处理，使用固定摩擦力

    setVelocity(
        velocityX() * drag,
        velocityY(),
        velocityZ() * drag
    );
}

void ChestMinecartEntity::dropItem() {
    // MC 1.16.5: 先掉落库存内容，再掉落矿车物品
    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        remove();
        return;
    }

    // 掉落所有库存物品
    if (m_inventory) {
        for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
            ItemStack stack = m_inventory->getItem(i);
            if (!stack.isEmpty()) {
                // 在矿车位置生成物品实体
                thread_local math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
                f32 vx = rng.nextFloat(-0.1f, 0.1f);
                f32 vy = 0.2f;
                f32 vz = rng.nextFloat(-0.1f, 0.1f);

                auto itemEntity = std::make_unique<ItemEntity>(
                    EntityId(0),
                    stack,
                    x(), y(), z(),
                    vx, vy, vz
                );
                itemEntity->setPickupDelay(10);
                worldPtr->spawnEntity(std::move(itemEntity));
            }
        }
    }

    // 调用父类方法掉落矿车物品
    AbstractMinecartEntity::dropItem();
}

i32 ChestMinecartEntity::getContainerSize() const {
    return m_inventory ? m_inventory->getContainerSize() : 0;
}

bool ChestMinecartEntity::isInventoryEmpty() const {
    return m_inventory ? m_inventory->isEmpty() : true;
}

ItemStack ChestMinecartEntity::getInventoryItem(i32 slot) const {
    return m_inventory ? m_inventory->getItem(slot) : ItemStack();
}

void ChestMinecartEntity::setInventoryItem(i32 slot, const ItemStack& stack) {
    if (m_inventory) {
        m_inventory->setItem(slot, stack);
    }
}

ItemStack ChestMinecartEntity::removeInventoryItem(i32 slot, i32 count) {
    return m_inventory ? m_inventory->removeItem(slot, count) : ItemStack();
}

void ChestMinecartEntity::clearInventory() {
    if (m_inventory) {
        m_inventory->clear();
    }
}

IInventory* ChestMinecartEntity::getInventory() {
    return m_inventory.get();
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
    // MC 1.16.5: 添加燃料（玩家交互时）
    addFuel(3600); // 3分钟 = 180秒 * 20 ticks/秒
}

void FurnaceMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) {
    // MC 1.16.5: 激活铁轨可以改变熔炉矿车的推动方向
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);

    if (powered) {
        // 根据当前移动方向更新推动方向
        f32 vx = velocityX();
        f32 vz = velocityZ();
        f32 speed = std::sqrt(vx * vx + vz * vz);
        if (speed > 0.01f) {
            m_pushX = vx / speed;
            m_pushZ = vz / speed;
        }
    }
}

// ============================================================================
// TNTMinecartEntity
// ============================================================================

void TNTMinecartEntity::tick() {
    AbstractMinecartEntity::tick();

    // MC 1.16.5: TNT引信倒计时
    if (m_fuse > 0) {
        m_fuse--;

        // 引信燃烧时产生烟雾粒子
        if (m_fuse % 5 == 0) {
            IWorld* worldPtr = Entity::world();
            if (worldPtr) {
                // TODO: 添加烟雾粒子效果
                // worldPtr->addParticle(ParticleTypeId::SMOKE, Vector3(x(), y() + 0.5, z()), Vector3(0, 0.05, 0));
            }
        }

        if (m_fuse <= 0) {
            // 根据速度计算爆炸威力
            f32 speed = std::sqrt(velocityX() * velocityX() + velocityZ() * velocityZ());
            f32 speedFactor = std::min(speed / 0.5f, 2.0f);  // 最大2倍
            explode(speedFactor);
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

void TNTMinecartEntity::explode(f32 speedFactor) {
    // MC 1.16.5: TNT矿车爆炸
    IWorld* worldPtr = Entity::world();
    if (!worldPtr || worldPtr->isClientSide()) {
        remove();
        return;
    }

    // 爆炸威力：基础4.0，速度加成最大到5.0
    f32 radius = 4.0f * speedFactor;
    radius = std::min(radius, 5.0f);  // MC 1.16.5 最大半径限制

    // 创建爆炸
    worldPtr->createExplosion(
        Vector3(static_cast<f32>(x()), static_cast<f32>(y()), static_cast<f32>(z())),
        radius,
        world::explosion::ExplosionMode::Break,
        false,  // 不产生火焰
        this
    );

    remove();
}

// ============================================================================
// HopperMinecartEntity
// ============================================================================

HopperMinecartEntity::HopperMinecartEntity(EntityId id)
    : AbstractMinecartEntity(Type::Hopper, id)
    , m_inventory(std::make_unique<blockentity::SimpleInventory>(INVENTORY_SIZE))
{
}

void HopperMinecartEntity::tick() {
    AbstractMinecartEntity::tick();

    // MC 1.16.5: 冷却计时
    if (m_suckCooldown > 0) {
        m_suckCooldown--;
    }

    IWorld* worldPtr = Entity::world();
    if (!worldPtr || worldPtr->isClientSide()) {
        return;
    }

    // MC 1.16.5: 检查是否被红石信号禁用
    // 漏斗矿车在充能的探测铁轨上会被禁用
    if (isOnRail()) {
        BlockPos railPos = getRailPosition();
        const BlockState* railState = worldPtr->getBlockState(railPos);
        if (railState && railState->is(VanillaBlocks::DETECTOR_RAIL)) {
            // 探测铁轨被充能时禁用漏斗
            m_disabled = blocks::DetectorRailBlock::isPowered(*railState);
        } else if (railState && railState->is(VanillaBlocks::ACTIVATOR_RAIL)) {
            // 激活铁轨被充能时也禁用漏斗
            m_disabled = blocks::ActivatorRailBlock::isPowered(*railState);
        } else {
            m_disabled = false;
        }
    } else {
        m_disabled = false;
    }

    // 如果被禁用，跳过吸取和传输
    if (m_disabled) {
        return;
    }

    // 尝试吸取物品
    if (canSuckItems()) {
        suckItems();
    }

    // 尝试向下传输物品
    transferItemsOut();
}

void HopperMinecartEntity::suckItems() {
    IWorld* worldPtr = Entity::world();
    if (!worldPtr || !m_inventory) {
        return;
    }

    // 获取收集区域
    AxisAlignedBB collectionArea = blockentity::IHopper::getCollectionArea(*this);

    // 获取区域内的物品实体
    std::vector<Entity*> entities = worldPtr->getEntitiesInAABB(collectionArea, nullptr);

    for (Entity* entity : entities) {
        if (!entity || entity->isRemoved()) {
            continue;
        }

        // 检查是否为物品实体
        ItemEntity* itemEntity = dynamic_cast<ItemEntity*>(entity);
        if (!itemEntity || !itemEntity->canBePickedUp()) {
            continue;
        }

        // 尝试将物品放入漏斗库存
        ItemStack stack = itemEntity->getItemStack().copy();
        ItemStack remaining = m_inventory->addItem(stack);

        if (remaining.isEmpty()) {
            // 完全吸收
            itemEntity->remove();
            m_suckCooldown = TRANSFER_COOLDOWN;
            return;  // 每tick只处理一个物品
        } else if (remaining.getCount() < stack.getCount()) {
            // 部分吸收
            itemEntity->setItemStack(remaining);
            m_suckCooldown = TRANSFER_COOLDOWN;
            return;
        }
    }
}

void HopperMinecartEntity::transferItemsOut() {
    IWorld* worldPtr = Entity::world();
    if (!worldPtr || !m_inventory) {
        return;
    }

    // 获取下方容器
    BlockPos belowPos = getHopperPos().offset(Direction::Down);
    IInventory* targetInventory = blockentity::HopperEntity::getInventoryAtPosition(worldPtr, belowPos);

    if (!targetInventory) {
        return;
    }

    // 检查目标库存是否已满
    bool targetFull = true;
    for (i32 i = 0; i < targetInventory->getContainerSize(); ++i) {
        if (targetInventory->getItem(i).isEmpty()) {
            targetFull = false;
            break;
        }
    }
    if (targetFull) {
        return;
    }

    // 遍历漏斗库存，尝试输出物品
    for (i32 slot = 0; slot < INVENTORY_SIZE; ++slot) {
        ItemStack stack = m_inventory->getItem(slot);
        if (stack.isEmpty()) {
            continue;
        }

        // 尝试将物品放入目标库存
        ItemStack toTransfer = stack.split(1);
        ItemStack remaining = blockentity::HopperEntity::putStackInInventoryAllSlots(
            m_inventory.get(), targetInventory, toTransfer, Direction::Up);

        if (remaining.isEmpty()) {
            // 传输成功
            m_inventory->setItem(slot, stack);
            m_suckCooldown = TRANSFER_COOLDOWN;
            return;
        } else {
            // 传输失败，恢复物品
            stack.grow(1);
            m_inventory->setItem(slot, stack);
        }
    }
}

i32 HopperMinecartEntity::getContainerSize() const {
    return m_inventory ? m_inventory->getContainerSize() : 0;
}

bool HopperMinecartEntity::isInventoryEmpty() const {
    return m_inventory ? m_inventory->isEmpty() : true;
}

ItemStack HopperMinecartEntity::getInventoryItem(i32 slot) const {
    return m_inventory ? m_inventory->getItem(slot) : ItemStack();
}

void HopperMinecartEntity::setInventoryItem(i32 slot, const ItemStack& stack) {
    if (m_inventory) {
        m_inventory->setItem(slot, stack);
    }
}

ItemStack HopperMinecartEntity::removeInventoryItem(i32 slot, i32 count) {
    return m_inventory ? m_inventory->removeItem(slot, count) : ItemStack();
}

void HopperMinecartEntity::clearInventory() {
    if (m_inventory) {
        m_inventory->clear();
    }
}

IInventory* HopperMinecartEntity::getInventory() {
    return m_inventory.get();
}

// ============================================================================
// CommandBlockMinecartEntity
// ============================================================================

void CommandBlockMinecartEntity::tick() {
    AbstractMinecartEntity::tick();

    // MC 1.16.5: 命令方块矿车不自动执行命令
    // 命令只在通过激活铁轨时执行
}

void CommandBlockMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered) {
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);

    // MC 1.16.5: 激活铁轨触发命令执行
    // 只在上升沿执行（从不激活变为激活）
    if (powered && !mPowered) {
        mPowered = true;
        executeCommand();
    } else if (!powered) {
        mPowered = false;
    }
}

void CommandBlockMinecartEntity::executeCommand() {
    // MC 1.16.5: 执行命令
    if (m_command.empty()) {
        return;
    }

    IWorld* worldPtr = Entity::world();
    if (!worldPtr) {
        return;
    }

    // TODO: 当命令系统完全集成后执行命令
    // 伪代码：
    // auto* server = worldPtr->getServer();
    // if (server) {
    //     CommandSource source = CommandSource::fromEntity(*this);
    //     m_successCount = server->getCommandManager().execute(m_command, source);
    // }

    // 当前实现：设置成功次数为0
    m_successCount = 0;
    m_lastOutput = "Command execution not yet implemented";
}

} // namespace entity
} // namespace mc
