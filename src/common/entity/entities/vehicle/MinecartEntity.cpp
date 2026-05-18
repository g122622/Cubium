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

#include "MinecartEntity.hpp"
#include "../../../item/Items.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/blocks/redstone/ActivatorRailBlock.hpp"
#include "../../../world/block/blocks/redstone/DetectorRailBlock.hpp"
#include "../../../world/blockentity/core/SimpleInventory.hpp"
#include "../../../world/blockentity/transport/HopperEntity.hpp"
#include "../../../world/explosion/Explosion.hpp"
#include "../../../world/explosion/ExplosionMode.hpp"
#include "../../../world/redstone/RedstonePower.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../inventory/IInventory.hpp"
#include "../../utils/ItemDropHelper.hpp"
#include "../item/ItemEntity.hpp"
#include "../player/Player.hpp"
#include "../projectile/AbstractArrowEntity.hpp"
#include <cmath>

namespace mc {
namespace entity {

using namespace mc::math;
using blocks::AbstractRailBlock;

namespace {
// MC 1.16.5 矿车常量
constexpr f64 MINECART_MAX_SPEED_ON_RAIL = 2.0;     // moveAlongTrack 中的最大速度
constexpr f64 RAIL_HEIGHT_OFFSET = 0.0625;          // 1/16 方块高度偏移
constexpr f64 RAIL_POSITION_SCALE = 2.0;            // 铁轨位置缩放因子
constexpr f64 UNPOWERED_RAIL_STOP_THRESHOLD = 0.03; // 未充能铁轨停止阈值
constexpr f64 PLAYER_PUSH_THRESHOLD = 0.01;         // 玩家推动阈值
constexpr f64 PLAYER_PUSH_FACTOR = 0.1;             // 玩家推动系数

// MC 1.16.5 AbstractMinecartEntity 数据参数
entity::DataParameter<i32> ROLLING_AMPLITUDE_PARAM{0};
entity::DataParameter<i32> ROLLING_DIRECTION_PARAM{1};
entity::DataParameter<f32> DAMAGE_PARAM{2};
entity::DataParameter<i32> DISPLAY_TILE_PARAM{3};
entity::DataParameter<i32> DISPLAY_TILE_OFFSET_PARAM{4};
entity::DataParameter<bool> SHOW_BLOCK_PARAM{5};
} // namespace

// ============================================================================
// AbstractMinecartEntity
// ============================================================================

AbstractMinecartEntity::AbstractMinecartEntity(Type type, EntityId id)
    : Entity(id)
    , m_type(type)
{
    // MC 1.16.5: 矿车默认属性
    registerData();
}

AbstractMinecartEntity::AbstractMinecartEntity(Type type)
    : Entity(EntityId(0))
    , m_type(type)
{
    registerData();
}

void AbstractMinecartEntity::registerData()
{
    // MC 1.16.5 AbstractMinecartEntity.registerData()
    Entity::registerData();

    m_dataManager.registerParam(ROLLING_AMPLITUDE_PARAM, 0);
    m_dataManager.registerParam(ROLLING_DIRECTION_PARAM, 1);
    m_dataManager.registerParam(DAMAGE_PARAM, 0.0f);
    m_dataManager.registerParam(DISPLAY_TILE_PARAM, 0); // 空气方块状态ID
    m_dataManager.registerParam(DISPLAY_TILE_OFFSET_PARAM, 6);
    m_dataManager.registerParam(SHOW_BLOCK_PARAM, false);
}

void AbstractMinecartEntity::tick()
{
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

bool AbstractMinecartEntity::isOnRailAt(const BlockPos& pos) const
{
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
    return state->is(VanillaBlocks::RAIL) || state->is(VanillaBlocks::POWERED_RAIL) ||
        state->is(VanillaBlocks::DETECTOR_RAIL) || state->is(VanillaBlocks::ACTIVATOR_RAIL);
}

void AbstractMinecartEntity::checkRailState()
{
    // MC 1.16.5: 检查当前位置或下方一格是否有铁轨
    BlockPos currentPos(static_cast<BlockCoord>(std::floor(x())),
        static_cast<BlockCoord>(std::floor(y())),
        static_cast<BlockCoord>(std::floor(z())));

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

void AbstractMinecartEntity::moveAlongTrack(const BlockPos& pos)
{
    // MC 1.16.5 AbstractMinecartEntity.moveAlongTrack()
    // 行431: 矿车在铁轨上不会积累摔落伤害
    setFallDistance(0.0f);

    // 获取世界指针
    IWorld* worldPtr = Entity::world();
    if (!worldPtr) {
        return;
    }

    // 获取当前速度
    f64 vx = velocityX();
    f64 vy = velocityY();
    f64 vz = velocityZ();

    // 获取精确的铁轨位置贴靠点
    // MC 1.16.5 行435: Vector3d vector3d = this.getPos(d0, d1, d2);
    Vector3 railPos = getPosOnRail(x(), y(), z());
    f64 d1 = static_cast<f64>(pos.y);

    // 获取铁轨状态
    const BlockState* railState = worldPtr->getBlockState(pos);
    if (!railState) {
        return;
    }

    // 获取铁轨方块
    const Block* railBlock = &railState->getBlock();
    if (railBlock == nullptr) {
        return;
    }

    // 获取铁轨形状
    RailShape railshape = m_railShape;
    const AbstractRailBlock* abstractRailBlock = dynamic_cast<const AbstractRailBlock*>(railBlock);
    if (abstractRailBlock) {
        railshape = abstractRailBlock->getRailShape(*railState);
        m_railShape = railshape; // 更新缓存
    }

    // 检查是否为动力铁轨（非激活铁轨）
    bool isPoweredRailFlag = false;
    bool isUnpoweredRailFlag = false;
    if (abstractRailBlock && abstractRailBlock->isPowered() && !abstractRailBlock->isPowered()) {
        // 这里需要特殊处理动力铁轨 vs 激活铁轨
        // MC 1.16.5: PoweredRailBlock 有 isActivatorRail() 方法
        // 普通动力铁轨返回 false，激活铁轨返回 true
    }

    // 检查是否为动力铁轨且被充能
    if (isPoweredRail(m_railPos)) {
        isPoweredRailFlag = isRailPowered(m_railPos);
        isUnpoweredRailFlag = !isPoweredRailFlag;
    }

    // MC 1.16.5 行445-464: 斜坡重力调整
    // 根据 RailShape 调整速度和Y位置
    switch (railshape) {
        case RailShape::AscendingEast:
            // 向东上升：西端低，东端高
            // 向东移动时Y增加，向西移动时Y减少
            setVelocity(vx - getSlopeAdjustment(), static_cast<f32>(vy), static_cast<f32>(vz));
            d1 += 1.0;
            break;
        case RailShape::AscendingWest:
            // 向西上升：东端低，西端高
            setVelocity(vx + getSlopeAdjustment(), static_cast<f32>(vy), static_cast<f32>(vz));
            d1 += 1.0;
            break;
        case RailShape::AscendingNorth:
            // 向北上升：南端低，北端高
            setVelocity(vx, static_cast<f32>(vy), vz + getSlopeAdjustment());
            d1 += 1.0;
            break;
        case RailShape::AscendingSouth:
            // 向南上升：北端低，南端高
            setVelocity(vx, static_cast<f32>(vy), vz - getSlopeAdjustment());
            d1 += 1.0;
            break;
        default:
            break;
    }

    // 更新速度
    vx = velocityX();
    vz = velocityZ();

    // MC 1.16.5 行466-481: 根据铁轨方向向量重新计算速度分量
    auto [dir1, dir2] = getRailDirectionVectors(railshape);
    f64 d4 = dir2.x - dir1.x;              // X方向差值
    f64 d5 = dir2.z - dir1.z;              // Z方向差值
    f64 d6 = std::sqrt(d4 * d4 + d5 * d5); // 方向向量长度

    // 计算当前速度在铁轨方向上的投影
    f64 d7 = vx * d4 + vz * d5;
    if (d7 < 0.0) {
        // 如果速度方向与铁轨方向相反，翻转方向向量
        d4 = -d4;
        d5 = -d5;
    }

    // 缓存速度值，避免重复计算 sqrt
    // MC 1.16.5 行479: 最大速度限制为 MINECART_MAX_SPEED_ON_RAIL
    f64 currentSpeed = std::sqrt(vx * vx + vz * vz);
    f64 d8 = std::min(MINECART_MAX_SPEED_ON_RAIL, currentSpeed);
    vx = d8 * d4 / d6;
    vz = d8 * d5 / d6;
    setVelocity(static_cast<f32>(vx), velocityY(), static_cast<f32>(vz));

    // MC 1.16.5 行482-491: 玩家推动检测
    // 如果矿车几乎静止但有乘客在移动，则推动矿车
    const auto& passengers = getPassengers();
    if (!passengers.empty()) {
        Entity* passenger = worldPtr->getEntity(passengers[0]);
        if (passenger != nullptr && passenger->typeId() == entity::EntityTypeIdNumber::PLAYER) {
            // 获取乘客的移动输入速度
            f64 passengerSpeedSq =
                passenger->velocityX() * passenger->velocityX() + passenger->velocityZ() * passenger->velocityZ();
            f64 minecartSpeedSq = vx * vx + vz * vz;
            if (passengerSpeedSq > 1.0e-4 && minecartSpeedSq < PLAYER_PUSH_THRESHOLD) {
                // 玩家在移动但矿车几乎静止，推动矿车
                setVelocity(velocityX() + static_cast<f32>(passenger->velocityX() * PLAYER_PUSH_FACTOR),
                    velocityY(),
                    velocityZ() + static_cast<f32>(passenger->velocityZ() * PLAYER_PUSH_FACTOR));
                isUnpoweredRailFlag = false; // 取消减速
            }
        }
    }

    // MC 1.16.5 行493-500: 动力铁轨减速（未充能的动力铁轨）
    if (isUnpoweredRailFlag && shouldDoRailFunctions()) {
        // 重用当前速度值
        f64 speedForRail = std::sqrt(velocityX() * velocityX() + velocityZ() * velocityZ());
        if (speedForRail < UNPOWERED_RAIL_STOP_THRESHOLD) {
            // 速度太低，完全停止
            setVelocity(0.0f, velocityY(), 0.0f);
        } else {
            // 减半速度
            setVelocity(velocityX() * 0.5f, velocityY(), velocityZ() * 0.5f);
        }
    }

    // MC 1.16.5 行502-521: 计算精确位置贴靠
    // 计算铁轨两端点
    f64 d23 = static_cast<f64>(pos.x) + 0.5 + static_cast<f64>(dir1.x) * 0.5;
    f64 d10 = static_cast<f64>(pos.z) + 0.5 + static_cast<f64>(dir1.z) * 0.5;
    f64 d12 = static_cast<f64>(pos.x) + 0.5 + static_cast<f64>(dir2.x) * 0.5;
    f64 d13 = static_cast<f64>(pos.z) + 0.5 + static_cast<f64>(dir2.z) * 0.5;

    d4 = d12 - d23;
    d5 = d13 - d10;
    f64 d14;

    // 计算矿车在铁轨上的位置参数
    if (d4 == 0.0) {
        d14 = z() - static_cast<f64>(pos.z);
    } else if (d5 == 0.0) {
        d14 = x() - static_cast<f64>(pos.x);
    } else {
        f64 d15 = x() - d23;
        f64 d16 = z() - d10;
        d14 = (d15 * d4 + d16 * d5) * 2.0;
    }

    // 设置精确位置
    setPosition(d23 + d4 * d14, d1, d10 + d5 * d14);

    // MC 1.16.5 行522: 调用 moveMinecartOnRail
    moveMinecartOnRail(pos);

    // MC 1.16.5 行523-527: 斜坡位置调整
    // 检查是否需要上升到斜坡的顶端
    if (dir1.y != 0 && static_cast<BlockCoord>(std::floor(x())) - pos.x == dir1.x &&
        static_cast<BlockCoord>(std::floor(z())) - pos.z == dir1.z) {
        setPosition(x(), y() + static_cast<f64>(dir1.y), z());
    } else if (dir2.y != 0 && static_cast<BlockCoord>(std::floor(x())) - pos.x == dir2.x &&
        static_cast<BlockCoord>(std::floor(z())) - pos.z == dir2.z) {
        setPosition(x(), y() + static_cast<f64>(dir2.y), z());
    }

    // MC 1.16.5 行529: 应用摩擦力
    applyDrag();

    // MC 1.16.5 行530-540: Y坐标校正
    Vector3 vector3d3 = getPosOnRail(x(), y(), z());
    if (vector3d3.x != 0.0f || vector3d3.y != 0.0f || vector3d3.z != 0.0f) {
        if (railPos.x != 0.0f || railPos.y != 0.0f || railPos.z != 0.0f) {
            f64 d17 = (railPos.y - vector3d3.y) * 0.05;
            f64 d18 = std::sqrt(velocityX() * velocityX() + velocityZ() * velocityZ());
            if (d18 > 0.0) {
                setVelocity(velocityX() * static_cast<f32>((d18 + d17) / d18),
                    velocityY(),
                    velocityZ() * static_cast<f32>((d18 + d17) / d18));
            }
            setPosition(x(), vector3d3.y, z());
        }
    }

    // MC 1.16.5 行542-548: 区块边界处理
    BlockCoord j = static_cast<BlockCoord>(std::floor(x()));
    BlockCoord i = static_cast<BlockCoord>(std::floor(z()));
    if (j != pos.x || i != pos.z) {
        f64 d26 = std::sqrt(velocityX() * velocityX() + velocityZ() * velocityZ());
        setVelocity(static_cast<f32>(d26 * static_cast<f64>(j - pos.x)),
            velocityY(),
            static_cast<f32>(d26 * static_cast<f64>(i - pos.z)));
    }

    // MC 1.16.5 行550-551: 调用 AbstractRailBlock.onMinecartPass()
    if (shouldDoRailFunctions() && abstractRailBlock) {
        // TODO: 当 onMinecartPass 方法实现后调用
        // abstractRailBlock->onMinecartPass(*railState, *worldPtr, pos, *this);
    }

    // MC 1.16.5 行553-583: 动力铁轨加速
    if (isPoweredRailFlag && shouldDoRailFunctions()) {
        f64 d27 = std::sqrt(velocityX() * velocityX() + velocityZ() * velocityZ());
        if (d27 > 0.01) {
            // 已有速度，加速
            setVelocity(velocityX() + static_cast<f32>(velocityX() / d27 * 0.06),
                velocityY(),
                velocityZ() + static_cast<f32>(velocityZ() / d27 * 0.06));
        } else {
            // 静止矿车，根据铁轨方向选择初始推动方向
            f64 d20 = velocityX();
            f64 d21 = velocityZ();
            if (railshape == RailShape::EastWest) {
                if (isNormalBlockAt(pos.offset(Direction::West))) {
                    d20 = 0.02;
                } else if (isNormalBlockAt(pos.offset(Direction::East))) {
                    d20 = -0.02;
                }
            } else if (railshape == RailShape::NorthSouth) {
                if (isNormalBlockAt(pos.offset(Direction::North))) {
                    d21 = 0.02;
                } else if (isNormalBlockAt(pos.offset(Direction::South))) {
                    d21 = -0.02;
                }
            }
            setVelocity(static_cast<f32>(d20), velocityY(), static_cast<f32>(d21));
        }
    }
}

void AbstractMinecartEntity::moveDerailedMinecart()
{
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

std::pair<Vector3, Vector3> AbstractMinecartEntity::getRailDirectionVectors(RailShape shape) const
{
    // MC 1.16.5: MATRIX 映射 (AbstractMinecartEntity.java 行60-79)
    // 方向向量定义（Vector3i整数向量）：
    // WEST = (-1, 0, 0), EAST = (1, 0, 0)
    // NORTH = (0, 0, -1), SOUTH = (0, 0, 1)
    // WEST_DOWN = WEST.down() = (-1, -1, 0)
    // EAST_DOWN = EAST.down() = (1, -1, 0)
    // NORTH_DOWN = NORTH.down() = (0, -1, -1)
    // SOUTH_DOWN = SOUTH.down() = (0, -1, 1)
    // 注意：斜坡的Y值为-1（向下），表示矿车在该端点时会下降

    switch (shape) {
        case RailShape::NorthSouth:
            // NORTH_SOUTH: Pair.of(NORTH, SOUTH) = (0, 0, -1), (0, 0, 1)
            return {Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.0f, 1.0f)};

        case RailShape::EastWest:
            // EAST_WEST: Pair.of(WEST, EAST) = (-1, 0, 0), (1, 0, 0)
            return {Vector3(-1.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f)};

        case RailShape::AscendingEast:
            // ASCENDING_EAST: Pair.of(WEST_DOWN, EAST) = (-1, -1, 0), (1, 0, 0)
            // 矿车从西向东上坡：西端点低（Y=-1），东端点高（Y=0）
            return {Vector3(-1.0f, -1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f)};

        case RailShape::AscendingWest:
            // ASCENDING_WEST: Pair.of(WEST, EAST_DOWN) = (-1, 0, 0), (1, -1, 0)
            // 矿车从东向西上坡：东端点低（Y=-1），西端点高（Y=0）
            return {Vector3(-1.0f, 0.0f, 0.0f), Vector3(1.0f, -1.0f, 0.0f)};

        case RailShape::AscendingNorth:
            // ASCENDING_NORTH: Pair.of(NORTH, SOUTH_DOWN) = (0, 0, -1), (0, -1, 1)
            // 矿车从南向北上坡：南端点低（Y=-1），北端点高（Y=0）
            return {Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, -1.0f, 1.0f)};

        case RailShape::AscendingSouth:
            // ASCENDING_SOUTH: Pair.of(NORTH_DOWN, SOUTH) = (0, -1, -1), (0, 0, 1)
            // 矿车从北向南上坡：北端点低（Y=-1），南端点高（Y=0）
            return {Vector3(0.0f, -1.0f, -1.0f), Vector3(0.0f, 0.0f, 1.0f)};

        case RailShape::SouthEast:
            // SOUTH_EAST: Pair.of(SOUTH, EAST) = (0, 0, 1), (1, 0, 0)
            return {Vector3(0.0f, 0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f)};

        case RailShape::SouthWest:
            // SOUTH_WEST: Pair.of(SOUTH, WEST) = (0, 0, 1), (-1, 0, 0)
            return {Vector3(0.0f, 0.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f)};

        case RailShape::NorthWest:
            // NORTH_WEST: Pair.of(NORTH, WEST) = (0, 0, -1), (-1, 0, 0)
            return {Vector3(0.0f, 0.0f, -1.0f), Vector3(-1.0f, 0.0f, 0.0f)};

        case RailShape::NorthEast:
            // NORTH_EAST: Pair.of(NORTH, EAST) = (0, 0, -1), (1, 0, 0)
            return {Vector3(0.0f, 0.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f)};

        default:
            return {Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.0f, 1.0f)};
    }
}

Vector3 AbstractMinecartEntity::getPosOnRail(f64 x, f64 y, f64 z) const
{
    // MC 1.16.5 AbstractMinecartEntity.getPos()
    // 行637-684: 计算矿车在铁轨上的精确位置

    BlockCoord i = static_cast<BlockCoord>(std::floor(x));
    BlockCoord j = static_cast<BlockCoord>(std::floor(y));
    BlockCoord k = static_cast<BlockCoord>(std::floor(z));

    // 检查下方一格是否有铁轨
    if (isOnRailAt(BlockPos(i, j - 1, k))) {
        --j;
    }

    const IWorld* worldPtr = world();
    if (!worldPtr) {
        return Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    }

    BlockPos railPos(i, j, k);
    const BlockState* railState = worldPtr->getBlockState(railPos);
    if (!railState) {
        return Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    }

    const Block* railBlock = &railState->getBlock();
    const AbstractRailBlock* abstractRailBlock = dynamic_cast<const AbstractRailBlock*>(railBlock);
    if (!abstractRailBlock) {
        return Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    }

    RailShape railshape = abstractRailBlock->getRailShape(*railState);
    auto [vector3i, vector3i1] = getRailDirectionVectors(railshape);

    // MC 1.16.5 行651-656: 关键的 Y 坐标计算
    // 注意: d1 = (double)j + RAIL_HEIGHT_OFFSET + (double)vector3i.getY() * 0.5
    // 这里有一个 1/16 方块的基础偏移！
    f64 d0 = static_cast<f64>(i) + 0.5 + static_cast<f64>(vector3i.x) * 0.5;
    f64 d1 = static_cast<f64>(j) + RAIL_HEIGHT_OFFSET + static_cast<f64>(vector3i.y) * 0.5;
    f64 d2 = static_cast<f64>(k) + 0.5 + static_cast<f64>(vector3i.z) * 0.5;
    f64 d3 = static_cast<f64>(i) + 0.5 + static_cast<f64>(vector3i1.x) * 0.5;
    f64 d4 = static_cast<f64>(j) + RAIL_HEIGHT_OFFSET + static_cast<f64>(vector3i1.y) * 0.5;
    f64 d5 = static_cast<f64>(k) + 0.5 + static_cast<f64>(vector3i1.z) * 0.5;

    f64 d6 = d3 - d0;
    f64 d7 = (d4 - d1) * RAIL_POSITION_SCALE; // Y差值乘以2（因为斜坡上升1格）
    f64 d8 = d5 - d2;
    f64 d9;

    // 计算矿车在铁轨线段上的参数位置
    if (d6 == 0.0) {
        d9 = z - static_cast<f64>(k);
    } else if (d8 == 0.0) {
        d9 = x - static_cast<f64>(i);
    } else {
        f64 d10 = x - d0;
        f64 d11 = z - d2;
        d9 = (d10 * d6 + d11 * d8) * RAIL_POSITION_SCALE;
    }

    x = d0 + d6 * d9;
    y = d1 + d7 * d9;
    z = d2 + d8 * d9;

    // 斜坡Y坐标校正
    if (d7 < 0.0) {
        ++y;
    } else if (d7 > 0.0) {
        y += 0.5;
    }

    return Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
}

void AbstractMinecartEntity::applyDrag()
{
    // MC 1.16.5: applyDrag()
    // 摩擦力取决于是否有乘客
    f64 drag = getPassengers().empty() ? EMPTY_DRAG : OCCUPIED_DRAG;
    setVelocity(velocityX() * static_cast<f32>(drag), velocityY(), velocityZ() * static_cast<f32>(drag));
}

void AbstractMinecartEntity::handleEntityCollisions()
{
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

        auto entityType = entity->typeId();

        // 跳过玩家（玩家碰撞由其他逻辑处理）
        if (entityType == entity::EntityTypeIdNumber::PLAYER) {
            continue;
        }

        // 矿车间碰撞处理
        if (entityType == entity::EntityTypeIdNumber::MINECART) {
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

void AbstractMinecartEntity::handleMinecartCollisions()
{
    // 已在 handleEntityCollisions() 中处理
    // 此方法保留为空以保持API兼容性
}

void AbstractMinecartEntity::updateRollingAnimation()
{
    // MC 1.16.5: 摇晃动画
    if (m_rollingAmplitude > 0) {
        m_rollingAmplitude--;
    }
}

void AbstractMinecartEntity::updateRotation()
{
    // MC 1.16.5: 根据移动方向更新朝向
    f32 vx = velocityX();
    f32 vz = velocityZ();
    f32 speedSq = vx * vx + vz * vz;

    if (speedSq > 0.0001) {
        f32 targetYaw = std::atan2(vx, vz) * (180.0f / PI);

        // 处理180度转向
        f32 yawDiff = targetYaw - yaw();
        while (yawDiff < -180.0f)
            yawDiff += 360.0f;
        while (yawDiff > 180.0f)
            yawDiff -= 360.0f;

        // 检测是否需要翻转
        if (yawDiff > 90.0f || yawDiff < -90.0f) {
            m_flipped = !m_flipped;
        }

        setRotation(targetYaw, pitch());
    }
}

bool AbstractMinecartEntity::isPoweredRail(const BlockPos& pos) const
{
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

bool AbstractMinecartEntity::isDetectorRail(const BlockPos& pos) const
{
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

bool AbstractMinecartEntity::isActivatorRail(const BlockPos& pos) const
{
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

bool AbstractMinecartEntity::isRailPowered(const BlockPos& pos)
{
    // 检查铁轨是否接收红石信号
    IWorld* worldPtr = Entity::world();
    if (!worldPtr) {
        return false;
    }

    return world::redstone::RedstonePower::isPowered(*worldPtr, pos);
}

bool AbstractMinecartEntity::isNormalBlockAt(const BlockPos& pos) const
{
    // MC 1.16.5: func_213900_a()
    // 检查指定位置是否为完整方块（用于动力铁轨方向判断）
    const IWorld* worldPtr = world();
    if (!worldPtr) {
        return false;
    }

    const BlockState* state = worldPtr->getBlockState(pos);
    if (!state) {
        return false;
    }

    // 检查是否为完整方块（实体方块）
    // MC 1.16.5: isNormalCube() = isSolid() && isOpaque() && !isAir()
    return state->isSolid() && state->isOpaque() && !state->isAir();
}

void AbstractMinecartEntity::moveMinecartOnRail(const BlockPos& pos)
{
    // MC 1.16.5: moveMinecartOnRail()
    // 在铁轨上移动矿车，应用速度限制

    // 获取速度限制系数（有乘客时为 0.75，无乘客时为 1.0）
    f64 d24 = getPassengers().empty() ? 1.0 : 0.75;

    // 获取最大速度
    f64 d25 = static_cast<f64>(getMaxSpeedWithRail());

    // 获取当前速度
    f64 vx = velocityX();
    f64 vz = velocityZ();

    // 限制速度
    vx = std::clamp(d24 * vx, -d25, d25);
    vz = std::clamp(d24 * vz, -d25, d25);

    // 执行移动
    move(static_cast<f32>(vx), 0.0f, static_cast<f32>(vz));
}

f32 AbstractMinecartEntity::getMaxSpeedWithRail() const
{
    // MC 1.16.5 Forge: getMaxSpeedWithRail()
    // 如果不在铁轨上，使用基础最大速度
    if (!m_onRail) {
        return getMaxSpeed();
    }

    // 获取铁轨方块并检查其最大速度限制
    const IWorld* worldPtr = world();
    if (!worldPtr) {
        return getMaxSpeed();
    }

    const BlockState* state = worldPtr->getBlockState(m_railPos);
    if (!state) {
        return getMaxSpeed();
    }

    // TODO: 当 AbstractRailBlock::getRailMaxSpeed 实现后调用
    // 目前使用默认最大速度
    return getMaxSpeed();
}

void AbstractMinecartEntity::dropItem(DamageSource* source)
{
    MC_UNUSED(source);
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

    // 使用 ItemDropHelper 在实体位置生成物品
    // 参考 MC 1.16.5: entityDropItem(stack)
    math::Random& rng = worldPtr->getRandom();
    ItemDropHelper::spawnItemEntity(worldPtr, stack, x(), y(), z(), rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);

    remove();
}

void AbstractMinecartEntity::activate()
{
    // 默认无操作，子类重写
}

void AbstractMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered)
{
    // 默认无操作，子类重写
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);
    MC_UNUSED(powered);
}

void AbstractMinecartEntity::applyForce(f32 x, f32 z)
{
    addVelocity(x, 0.0f, z);
}

bool AbstractMinecartEntity::hurt(DamageSource& source, f32 amount)
{
    // MC 1.16.5: AbstractMinecartEntity.attackEntityFrom()
    // 矿车不继承 LivingEntity，所以这不是 override

    // 1. 检查无敌状态
    if (isInvulnerable()) {
        return false;
    }

    // 2. 只在服务端处理
    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        return true;
    }

    // 3. 已移除的实体不再处理伤害
    if (isRemoved()) {
        return true;
    }

    // 4. 设置摇晃动画
    // this.setRollingDirection(-this.getRollingDirection());
    m_rollingDirection = -m_rollingDirection;

    // 5. 设置摇晃时间
    // this.setRollingAmplitude(10);
    m_rollingAmplitude = 10;

    // 6. 标记速度已改变（用于同步）
    // this.markVelocityChanged();
    // TODO: 当网络同步实现后设置

    // 7. 累积伤害
    // this.setDamage(this.getDamage() + amount * 10.0F);
    m_damage += static_cast<i32>(amount * 10.0f);

    // 8. 检查攻击者是否为创造模式玩家
    // MC 1.16.5: boolean flag = source.getTrueSource() instanceof PlayerEntity &&
    //     ((PlayerEntity)source.getTrueSource()).abilities.isCreativeMode;
    bool isCreative = false;
    Entity* attacker = source.source();
    if (attacker != nullptr && attacker->typeId() == entity::EntityTypeIdNumber::PLAYER) {
        Player* player = static_cast<Player*>(attacker);
        isCreative = player->isCreative();
    }

    // 9. 检查是否应该摧毁矿车
    // if (flag || this.getDamage() > 40.0F)
    if (isCreative || m_damage > static_cast<i32>(DAMAGE_THRESHOLD)) {
        // 移除所有乘客
        removePassengers();

        if (isCreative && !hasCustomName()) {
            // 创造模式玩家攻击且无自定义名称：直接移除，不掉落物品
            remove();
        } else {
            // 掉落矿车物品，传递伤害源以便子类判断
            dropItem(&source);
        }
    }

    return true;
}

// ============================================================================
// RideableMinecartEntity
// ============================================================================

std::unique_ptr<Entity> RideableMinecartEntity::create(IWorld* /*world*/)
{
    return std::make_unique<RideableMinecartEntity>(EntityId(0));
}

void RideableMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered)
{
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

std::unique_ptr<Entity> ChestMinecartEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ChestMinecartEntity>(EntityId(0));
}

ChestMinecartEntity::ChestMinecartEntity(EntityId id)
    : AbstractMinecartEntity(Type::Chest, id)
    , m_inventory(std::make_unique<blockentity::SimpleInventory>(INVENTORY_SIZE))
{}

void ChestMinecartEntity::applyDrag()
{
    // MC 1.16.5: 箱子矿车摩擦力
    // 根据红石信号强度增加摩擦
    f32 drag = 0.98f;

    // MC 1.16.5: 根据容器红石信号强度增加摩擦力
    // int signal = Container.calcRedstoneFromInventory(this);
    // drag -= signal * 0.001f;
    // 目前简化处理，使用固定摩擦力

    setVelocity(velocityX() * drag, velocityY(), velocityZ() * drag);
}

void ChestMinecartEntity::dropItem(DamageSource* source)
{
    MC_UNUSED(source);
    // MC 1.16.5: 先掉落库存内容，再掉落矿车物品
    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        remove();
        return;
    }

    // 掉落所有库存物品
    if (m_inventory) {
        math::Random& rng = worldPtr->getRandom();
        for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
            ItemStack stack = m_inventory->getItem(i);
            if (!stack.isEmpty()) {
                // 使用 ItemDropHelper 在矿车位置生成物品实体
                ItemDropHelper::spawnItemEntity(
                    worldPtr, stack, x(), y(), z(), rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
            }
        }
    }

    // 调用父类方法掉落矿车物品
    AbstractMinecartEntity::dropItem(source);
}

i32 ChestMinecartEntity::getContainerSize() const
{
    return m_inventory ? m_inventory->getContainerSize() : 0;
}

bool ChestMinecartEntity::isInventoryEmpty() const
{
    return m_inventory ? m_inventory->isEmpty() : true;
}

ItemStack ChestMinecartEntity::getInventoryItem(i32 slot) const
{
    return m_inventory ? m_inventory->getItem(slot) : ItemStack();
}

void ChestMinecartEntity::setInventoryItem(i32 slot, const ItemStack& stack)
{
    if (m_inventory) {
        m_inventory->setItem(slot, stack);
    }
}

ItemStack ChestMinecartEntity::removeInventoryItem(i32 slot, i32 count)
{
    return m_inventory ? m_inventory->removeItem(slot, count) : ItemStack();
}

void ChestMinecartEntity::clearInventory()
{
    if (m_inventory) {
        m_inventory->clear();
    }
}

IInventory* ChestMinecartEntity::getInventory()
{
    return m_inventory.get();
}

// ============================================================================
// FurnaceMinecartEntity
// ============================================================================

std::unique_ptr<Entity> FurnaceMinecartEntity::create(IWorld* /*world*/)
{
    return std::make_unique<FurnaceMinecartEntity>(EntityId(0));
}

void FurnaceMinecartEntity::tick()
{
    AbstractMinecartEntity::tick();

    // MC 1.16.5 FurnaceMinecartEntity.tick() 行54-72
    IWorld* worldPtr = Entity::world();
    if (worldPtr && !worldPtr->isClientSide()) {
        // 消耗燃料
        if (m_fuel > 0) {
            m_fuel--;
        }

        // 燃料耗尽时清除推动方向
        if (m_fuel <= 0) {
            m_pushX = 0.0f;
            m_pushZ = 0.0f;
        }
    }

    // MC 1.16.5: 燃烧时产生烟雾粒子
    // if (isActivated() && rand.nextInt(4) == 0) {
    //     world.addParticle(ParticleTypes.LARGE_SMOKE, x, y + 0.8, z, 0, 0, 0);
    // }
}

void FurnaceMinecartEntity::updatePushDirection()
{
    // MC 1.16.5 FurnaceMinecartEntity.moveAlongTrack() 行90-104
    // 根据当前速度更新推动方向
    f64 vx = velocityX();
    f64 vz = velocityZ();
    f64 speedSq = vx * vx + vz * vz;
    f64 pushSq = m_pushX * m_pushX + m_pushZ * m_pushZ;

    if (pushSq > 1.0e-4 && speedSq > 0.001) {
        f64 speed = std::sqrt(speedSq);
        f64 pushMag = std::sqrt(pushSq);
        m_pushX = static_cast<f32>(vx / speed * pushMag);
        m_pushZ = static_cast<f32>(vz / speed * pushMag);
    }
}

void FurnaceMinecartEntity::applyDrag()
{
    // MC 1.16.5: 熔炉矿车摩擦力
    f64 pushMag = m_pushX * m_pushX + m_pushZ * m_pushZ;

    if (pushMag > 1.0e-7) {
        pushMag = std::sqrt(pushMag);
        m_pushX /= static_cast<f32>(pushMag);
        m_pushZ /= static_cast<f32>(pushMag);

        f32 drag = 0.8f;
        setVelocity(velocityX() * drag + m_pushX * 0.1f, velocityY(), velocityZ() * drag + m_pushZ * 0.1f);
    } else {
        f32 drag = 0.98f;
        setVelocity(velocityX() * drag, velocityY(), velocityZ() * drag);
    }

    AbstractMinecartEntity::applyDrag();
}

void FurnaceMinecartEntity::addFuel(i32 ticks)
{
    // MC 1.16.5: 燃料上限检查
    // if (this.fuel + 3600 <= 32000) { this.fuel += 3600; }
    if (m_fuel + ticks <= MAX_FUEL) {
        m_fuel += ticks;
    } else {
        m_fuel = MAX_FUEL;
    }
}

void FurnaceMinecartEntity::activate()
{
    // MC 1.16.5: 添加燃料（玩家交互时）
    addFuel(3600); // 3分钟 = 180秒 * 20 ticks/秒
}

void FurnaceMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered)
{
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

void FurnaceMinecartEntity::dropItem(DamageSource* source)
{
    // MC 1.16.5 FurnaceMinecartEntity.killMinecart() 行82-88
    // 先调用父类方法掉落矿车物品
    AbstractMinecartEntity::dropItem(source);

    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        return;
    }

    // MC 1.16.5: 如果不是爆炸伤害且游戏规则允许实体掉落，则掉落熔炉方块
    // if (!source.isExplosion() && this.world.getGameRules().getBoolean(GameRules.DO_ENTITY_DROPS))
    // 参考 MC 1.16.5 FurnaceMinecartEntity.killMinecart()
    bool isExplosion = (source != nullptr && source->isExplosion());
    // TODO: 当 GameRules 系统完善后检查 DO_ENTITY_DROPS 规则
    // bool doEntityDrops = worldPtr->gameRules().getBoolean(GameRules::DO_ENTITY_DROPS);

    if (!isExplosion) {
        // 掉落熔炉方块
        // 参考 MC 1.16.5: this.entityDropItem(Blocks.FURNACE);
        // 通过 BlockItemRegistry 获取熔炉方块物品
        // TODO: 当 FURNACE 方块注册到 VanillaBlocks 后使用 BlockItemRegistry
        // 目前暂不实现熔炉方块掉落，因为 FURNACE 方块尚未完全实现
        // const BlockItem* furnaceBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::FURNACE);
    }
}

// ============================================================================
// TNTMinecartEntity
// ============================================================================

std::unique_ptr<Entity> TNTMinecartEntity::create(IWorld* /*world*/)
{
    return std::make_unique<TNTMinecartEntity>(EntityId(0));
}

void TNTMinecartEntity::tick()
{
    AbstractMinecartEntity::tick();

    // MC 1.16.5 TNTMinecartEntity.tick() 行46-62
    // TNT引信倒计时
    if (m_fuse > 0) {
        m_fuse--;

        // 引信燃烧时产生烟雾粒子
        if (m_fuse % 4 == 0) {
            IWorld* worldPtr = Entity::world();
            if (worldPtr) {
                // MC 1.16.5: 每tick有1/4概率产生烟雾
                // worldPtr->addParticle(ParticleTypes::SMOKE, x, y + 0.5, z, 0, 0, 0);
            }
        }

        if (m_fuse == 0) {
            // MC 1.16.5 行51-53: 引信归零时爆炸
            f64 speedSq = velocityX() * velocityX() + velocityZ() * velocityZ();
            explode(static_cast<f32>(std::sqrt(speedSq)));
        }
    }

    // MC 1.16.5 行55-61: 检查火焰接触（在火焰/岩浆中自动点燃）
    checkFireIgnition();

    // MC 1.16.5 行55-61: 水平碰撞检测（高速碰撞时爆炸）
    if (m_collidedHorizontally) {
        f64 speedSq = velocityX() * velocityX() + velocityZ() * velocityZ();
        if (speedSq >= 0.01) {
            explode(static_cast<f32>(std::sqrt(speedSq)));
        }
    }
}

void TNTMinecartEntity::ignite()
{
    // MC 1.16.5 TNTMinecartEntity.ignite() 行147-156
    m_fuse = DEFAULT_FUSE; // 80 ticks = 4 seconds

    IWorld* worldPtr = Entity::world();
    if (worldPtr && !worldPtr->isClientSide()) {
        // MC 1.16.5: 发送状态更新
        // worldPtr->setEntityState(this, (byte)10);

        // MC 1.16.5: 播放点燃音效
        // if (!isSilent()) {
        //     worldPtr->playSound(nullptr, x(), y(), z(), SoundEvents::ENTITY_TNT_PRIMED,
        //     SoundCategory::BLOCKS, 1.0f, 1.0f);
        // }
    }
}

void TNTMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered)
{
    // MC 1.16.5 TNTMinecartEntity.onActivatorRailPass() 行124-128
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);

    if (powered && m_fuse < 0) {
        ignite();
    }
}

bool TNTMinecartEntity::hurt(DamageSource& source, f32 amount)
{
    // MC 1.16.5 TNTMinecartEntity.attackEntityFrom() 行67-77
    // 检查是否为燃烧的箭矢
    Entity* directSource = source.directSource();
    if (directSource != nullptr) {
        // 检查是否为 AbstractArrowEntity（包括 ArrowEntity 和 SpectralArrowEntity）
        // 使用 dynamic_cast 检测箭矢实体
        AbstractArrowEntity* arrow = dynamic_cast<AbstractArrowEntity*>(directSource);
        if (arrow != nullptr && arrow->isOnFire()) {
            // [已完成] 2026/05/17 - 检测燃烧箭矢引爆 TNT 矿车
            // MC 1.16.5: 使用箭矢的速度计算爆炸威力
            Vector3 arrowVelocity = arrow->velocity();
            f64 speedSq = static_cast<f64>(arrowVelocity.x) * arrowVelocity.x
                        + static_cast<f64>(arrowVelocity.y) * arrowVelocity.y
                        + static_cast<f64>(arrowVelocity.z) * arrowVelocity.z;
            explode(static_cast<f32>(std::sqrt(speedSq)));
            return true;
        }

        // 兼容：检查其他带火焰的投射物（如火球）
        if (source.isProjectile() && source.isFire()) {
            f64 speedSq = velocityX() * velocityX() + velocityZ() * velocityZ();
            explode(static_cast<f32>(std::sqrt(speedSq)));
            return true;
        }
    }

    return AbstractMinecartEntity::hurt(source, amount);
}

bool TNTMinecartEntity::onProjectileHit(DamageSource& source, f32 amount)
{
    // MC 1.16.5: 燃烧箭矢命中时直接爆炸
    return hurt(source, amount);
}

void TNTMinecartEntity::dropItem(DamageSource* source)
{
    // MC 1.16.5 TNTMinecartEntity.killMinecart() 行79-94
    // 火焰或爆炸伤害时点燃而非爆炸掉落
    f64 speedSq = velocityX() * velocityX() + velocityZ() * velocityZ();

    // 判断伤害类型
    bool isFire = (source != nullptr && source->isFire());
    bool isExplosion = (source != nullptr && source->isExplosion());

    // MC 1.16.5: if (!source.isFireDamage() && !source.isExplosion() && !(d0 >= 0.01D))
    // 如果不是火焰伤害、不是爆炸伤害、且速度足够低，则正常掉落
    if (!isFire && !isExplosion && speedSq < 0.01) {
        // 先掉落矿车物品
        AbstractMinecartEntity::dropItem(source);

        // MC 1.16.5: if (!source.isExplosion() && world.getGameRules().getBoolean(GameRules.DO_ENTITY_DROPS))
        // 如果不是爆炸伤害且游戏规则允许实体掉落，则额外掉落 TNT 方块
        if (!isExplosion) {
            // TODO: 当 GameRules 系统完善后检查 DO_ENTITY_DROPS 规则
            IWorld* worldPtr = world();
            if (worldPtr && !worldPtr->isClientSide()) {
                // 通过 BlockItemRegistry 获取 TNT 方块物品
                // 参考 MC 1.16.5: this.entityDropItem(Blocks.TNT);
                const BlockItem* tntBlockItem = BlockItemRegistry::instance().getBlockItem(
                    *VanillaBlocks::TNT);
                if (tntBlockItem != nullptr) {
                    ItemStack stack(*tntBlockItem, 1);
                    math::Random& rng = worldPtr->getRandom();
                    ItemDropHelper::spawnItemEntity(worldPtr, stack, x(), y(), z(), rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
                }
            }
        }
    } else {
        // MC 1.16.5: 火焰或爆炸伤害时点燃 TNT 矿车
        if (m_fuse < 0) {
            ignite();
            // 随机点燃时间 0-40 ticks
            // MC 1.16.5: this.minecartTNTFuse = this.rand.nextInt(20) + this.rand.nextInt(20);
            IWorld* worldPtr = world();
            if (worldPtr) {
                math::Random& rng = worldPtr->getRandom();
                m_fuse = rng.nextInt(20) + rng.nextInt(20);
            } else {
                m_fuse = 20; // 默认点燃时间
            }
        }
    }
}

void TNTMinecartEntity::checkFireIgnition()
{
    // MC 1.16.5: 在火焰或岩浆中自动点燃
    if (m_fuse < 0) {
        if (isOnFire() || isInLava()) {
            ignite();
        }
    }
}

void TNTMinecartEntity::explode(f32 speedFactor)
{
    // MC 1.16.5 TNTMinecartEntity.explodeCart() 行99-109
    IWorld* worldPtr = Entity::world();
    if (!worldPtr || worldPtr->isClientSide()) {
        remove();
        return;
    }

    // 爆炸威力：基础4.0，速度加成最大到5.0
    f64 d0 = static_cast<f64>(speedFactor);
    if (d0 > 5.0) {
        d0 = 5.0;
    }

    // MC 1.16.5: 4.0 + random * 1.5 * speedFactor
    math::Random& rng = worldPtr->getRandom();
    f32 radius = static_cast<f32>(4.0 + rng.nextDouble() * 1.5 * d0);

    // 创建爆炸
    // MC 1.16.5: TNT矿车爆炸时不破坏铁轨
    // canExplosionDestroyBlock: 如果已点燃且目标方块是铁轨，返回 false
    worldPtr->createExplosion(Vector3(static_cast<f32>(x()), static_cast<f32>(y()), static_cast<f32>(z())),
        radius,
        world::explosion::ExplosionMode::Break,
        false, // 不产生火焰
        this);

    remove();
}

// ============================================================================
// HopperMinecartEntity
// ============================================================================

std::unique_ptr<Entity> HopperMinecartEntity::create(IWorld* /*world*/)
{
    return std::make_unique<HopperMinecartEntity>(EntityId(0));
}

HopperMinecartEntity::HopperMinecartEntity(EntityId id)
    : AbstractMinecartEntity(Type::Hopper, id)
    , m_inventory(std::make_unique<blockentity::SimpleInventory>(INVENTORY_SIZE))
{}

void HopperMinecartEntity::tick()
{
    AbstractMinecartEntity::tick();

    // MC 1.16.5: 冷却计时
    if (m_suckCooldown > 0) {
        m_suckCooldown--;
    }

    IWorld* worldPtr = Entity::world();
    if (!worldPtr || worldPtr->isClientSide()) {
        return;
    }

    // MC 1.16.5 HopperMinecartEntity.tick() 行108-126
    // 检查是否被红石信号禁用
    // 注意：MC 中的 isBlocked 语义是反转的！
    // isBlocked = true 表示"可以工作"，isBlocked = false 表示"被禁用"
    // 这是 MC 源码中的命名问题，我们用 m_disabled 来表示更清晰的语义
    //
    // 漏斗矿车在充能的激活铁轨上会被禁用
    // 激活铁轨被充能时，漏斗矿车停止工作
    if (isOnRail()) {
        BlockPos railPos = getRailPosition();
        const BlockState* railState = worldPtr->getBlockState(railPos);
        if (railState && railState->is(VanillaBlocks::ACTIVATOR_RAIL)) {
            // MC 1.16.5: 激活铁轨充能时禁用漏斗
            // onActivatorRailPass: flag = !receivingPower; setBlocked(flag)
            // receivingPower = isPowered -> flag = !isPowered
            // isBlocked = flag = !isPowered
            // 所以: isBlocked = true (可工作) 当铁轨未充能
            //       isBlocked = false (禁用) 当铁轨充能
            // m_disabled = !isBlocked，所以：
            // m_disabled = isPowered
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

void HopperMinecartEntity::suckItems()
{
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
            return; // 每tick只处理一个物品
        } else if (remaining.getCount() < stack.getCount()) {
            // 部分吸收
            itemEntity->setItemStack(remaining);
            m_suckCooldown = TRANSFER_COOLDOWN;
            return;
        }
    }
}

void HopperMinecartEntity::transferItemsOut()
{
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

i32 HopperMinecartEntity::getContainerSize() const
{
    return m_inventory ? m_inventory->getContainerSize() : 0;
}

bool HopperMinecartEntity::isInventoryEmpty() const
{
    return m_inventory ? m_inventory->isEmpty() : true;
}

ItemStack HopperMinecartEntity::getInventoryItem(i32 slot) const
{
    return m_inventory ? m_inventory->getItem(slot) : ItemStack();
}

void HopperMinecartEntity::setInventoryItem(i32 slot, const ItemStack& stack)
{
    if (m_inventory) {
        m_inventory->setItem(slot, stack);
    }
}

ItemStack HopperMinecartEntity::removeInventoryItem(i32 slot, i32 count)
{
    return m_inventory ? m_inventory->removeItem(slot, count) : ItemStack();
}

void HopperMinecartEntity::clearInventory()
{
    if (m_inventory) {
        m_inventory->clear();
    }
}

IInventory* HopperMinecartEntity::getInventory()
{
    return m_inventory.get();
}

void HopperMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered)
{
    // MC 1.16.5 HopperMinecartEntity.onActivatorRailPass() 行55-61
    // boolean flag = !receivingPower;
    // if (flag != this.getBlocked()) { this.setBlocked(flag); }
    //
    // 注意 MC 中 isBlocked 的语义：
    // - isBlocked = true 表示漏斗可以工作（吸取/传输）
    // - isBlocked = false 表示漏斗被禁用
    //
    // 当激活铁轨充能时（powered = true）：
    //   flag = !true = false，setBlocked(false) -> 禁用漏斗
    // 当激活铁轨未充能时（powered = false）：
    //   flag = !false = true，setBlocked(true) -> 启用漏斗
    //
    // 我们使用 m_disabled 来表示更清晰的语义：
    // m_disabled = true -> 漏斗被禁用
    // m_disabled = false -> 漏斗可工作
    //
    // 所以：m_disabled = powered
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);

    m_disabled = powered;
}

// ============================================================================
// CommandBlockMinecartEntity
// ============================================================================

void CommandBlockMinecartEntity::tick()
{
    AbstractMinecartEntity::tick();

    // MC 1.16.5: 命令方块矿车不自动执行命令
    // 命令只在通过激活铁轨时执行
}

void CommandBlockMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered)
{
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

void CommandBlockMinecartEntity::executeCommand()
{
    // MC 1.16.5: CommandBlockMinecartEntity.executeCommand()
    // 参考 CommandBlockLogic.trigger() 和 Commands.handleCommand()

    if (m_command.empty()) {
        return;
    }

    IWorld* worldPtr = Entity::world();
    if (!worldPtr) {
        return;
    }

    // 通过 IWorld 接口执行命令
    // 命令方块矿车的权限级别为 2（相当于 OP 级别）
    // 参考 MC 1.16.5: CommandSource(permissionLevel=2)
    Vector3d position(x(), y(), z());
    m_successCount = worldPtr->executeCommand(m_command, position, 2);

    // 设置最后输出（成功或失败）
    if (m_successCount > 0) {
        m_lastOutput = "Command executed successfully";
    } else {
        m_lastOutput = "Command execution failed";
    }
}

} // namespace entity
} // namespace mc
