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
#include "common/core/Result.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/redstone/ActivatorRailBlock.hpp"
#include "common/world/block/blocks/redstone/DetectorRailBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include "common/world/blockentity/transport/HopperEntity.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/redstone/RedstoneHelper.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <algorithm>
#include <cmath>

namespace mc {
namespace entity {

using namespace mc::math;
using blocks::AbstractRailBlock;
using blocks::ActivatorRailBlock;

namespace {
// 矿车常量
constexpr f64 MINECART_MAX_SPEED_ON_RAIL = 2.0;     // moveAlongTrack 中的最大速度
constexpr f64 RAIL_HEIGHT_OFFSET = 0.0625;          // 1/16 方块高度偏移
constexpr f64 RAIL_POSITION_SCALE = 2.0;            // 铁轨位置缩放因子
constexpr f64 UNPOWERED_RAIL_STOP_THRESHOLD = 0.03; // 未充能铁轨停止阈值
constexpr f64 PLAYER_PUSH_THRESHOLD = 0.01;         // 玩家推动阈值
constexpr f64 PLAYER_PUSH_FACTOR = 0.1;             // 玩家推动系数

} // namespace

// ========== 静态数据参数定义 ==========
entity::DataParameter<i32> AbstractMinecartEntity::DATA_ROLLING_AMPLITUDE_PARAM =
    entity::EntityDataManager::createKey<i32>();
entity::DataParameter<i32> AbstractMinecartEntity::DATA_ROLLING_DIRECTION_PARAM =
    entity::EntityDataManager::createKey<i32>();
entity::DataParameter<f32> AbstractMinecartEntity::DATA_DAMAGE_PARAM = entity::EntityDataManager::createKey<f32>();
entity::DataParameter<i32> AbstractMinecartEntity::DATA_DISPLAY_TILE_PARAM =
    entity::EntityDataManager::createKey<i32>();
entity::DataParameter<i32> AbstractMinecartEntity::DATA_DISPLAY_TILE_OFFSET_PARAM =
    entity::EntityDataManager::createKey<i32>();
entity::DataParameter<bool> AbstractMinecartEntity::DATA_SHOW_BLOCK_PARAM =
    entity::EntityDataManager::createKey<bool>();

// ============================================================================
// AbstractMinecartEntity
// ============================================================================

AbstractMinecartEntity::AbstractMinecartEntity(Type type, EntityInstanceId id)
    : Entity(id)
    , m_type(type)
{
    // 矿车默认属性
    registerData();
}

AbstractMinecartEntity::AbstractMinecartEntity(Type type)
    : Entity(EntityInstanceId(0))
    , m_type(type)
{
    registerData();
}

void AbstractMinecartEntity::registerData()
{
    Entity::registerData();

    m_dataManager.registerParam(DATA_ROLLING_AMPLITUDE_PARAM, 0);
    m_dataManager.registerParam(DATA_ROLLING_DIRECTION_PARAM, 1);
    m_dataManager.registerParam(DATA_DAMAGE_PARAM, 0.0f);
    m_dataManager.registerParam(DATA_DISPLAY_TILE_PARAM, 0); // 空气方块状态ID
    m_dataManager.registerParam(DATA_DISPLAY_TILE_OFFSET_PARAM, 6);
    m_dataManager.registerParam(DATA_SHOW_BLOCK_PARAM, false);
}

void AbstractMinecartEntity::tick()
{
    // 更新摇晃动画
    _updateRollingAnimation();

    // 减少损坏值
    if (m_damage > 0) {
        m_damage--;
    }

    // 检查铁轨状态
    _checkRailState();

    if (m_onRail) {
        // 在铁轨上移动
        _moveAlongTrack(m_railPos);
    } else {
        // 脱轨移动
        _moveDerailedMinecart();
    }

    // 处理碰撞
    _handleEntityCollisions();
    _handleMinecartCollisions();

    // 更新朝向
    _updateRotation();

    // 调用父类tick
    Entity::tick();
}

bool AbstractMinecartEntity::isOnRailAt(const BlockPos& pos) const
{
    // 检查指定位置是否有铁轨
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

void AbstractMinecartEntity::_checkRailState()
{
    // 检查当前位置或下方一格是否有铁轨
    BlockPos currentPos(static_cast<BlockCoord>(std::floor(x())),
        static_cast<BlockCoord>(std::floor(y())),
        static_cast<BlockCoord>(std::floor(z())));

    IWorld* worldPtr = Entity::world();

    // 检查当前方块
    if (isOnRailAt(currentPos)) {
        m_onRail = true;
        m_railPos = currentPos;

        // 从方块状态获取铁轨形状
        if (worldPtr) {
            const BlockState* state = worldPtr->getBlockState(currentPos);
            if (state) {
                const Block* block = &state->getBlock();
                const AbstractRailBlock* railBlock = dynamic_cast<const AbstractRailBlock*>(block);
                if (railBlock) {
                    m_railShape = railBlock->getRailShape(*state);
                }
            }
        }
        return;
    }

    // 检查下方一格
    BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
    if (isOnRailAt(belowPos)) {
        m_onRail = true;
        m_railPos = belowPos;

        // 从方块状态获取铁轨形状
        if (worldPtr) {
            const BlockState* state = worldPtr->getBlockState(belowPos);
            if (state) {
                const Block* block = &state->getBlock();
                const AbstractRailBlock* railBlock = dynamic_cast<const AbstractRailBlock*>(block);
                if (railBlock) {
                    m_railShape = railBlock->getRailShape(*state);
                }
            }
        }
        return;
    }

    m_onRail = false;
}

void AbstractMinecartEntity::_moveAlongTrack(const BlockPos& pos)
{
    // 矿车在铁轨上不会积累摔落伤害
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
    Vector3 railPos = _getPosOnRail(x(), y(), z());
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
    // 注意：动力铁轨和探测铁轨属于 isPoweredType()，激活铁轨属于 isStraight 但非 isPoweredType()
    // 此处暂不在此处做特殊区分，由 _isPoweredRail 和 _isRailPowered 方法处理

    // 检查是否为动力铁轨且被充能
    if (_isPoweredRail(m_railPos)) {
        isPoweredRailFlag = _isRailPowered(m_railPos);
        isUnpoweredRailFlag = !isPoweredRailFlag;
    }

    // 斜坡重力调整：根据 RailShape 调整速度和Y位置
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

    // 根据铁轨方向向量重新计算速度分量
    auto [dir1, dir2] = _getRailDirectionVectors(railshape);
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
    // 最大速度限制为 MINECART_MAX_SPEED_ON_RAIL
    f64 currentSpeed = std::sqrt(vx * vx + vz * vz);
    f64 d8 = std::min(MINECART_MAX_SPEED_ON_RAIL, currentSpeed);
    vx = d8 * d4 / d6;
    vz = d8 * d5 / d6;
    setVelocity(static_cast<f32>(vx), velocityY(), static_cast<f32>(vz));

    // 玩家推动检测：如果矿车几乎静止但有乘客在移动，则推动矿车
    const auto& passengers = getPassengers();
    if (!passengers.empty()) {
        Entity* passenger = worldPtr->getEntity(passengers[0]);
        if (passenger != nullptr && passenger->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
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

    // 动力铁轨减速（未充能的动力铁轨）
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

    // 计算精确位置贴靠
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

    // 在铁轨上移动
    _moveMinecartOnRail(pos);

    // 斜坡位置调整：检查是否需要上升到斜坡的顶端
    if (dir1.y != 0 && static_cast<BlockCoord>(std::floor(x())) - pos.x == dir1.x &&
        static_cast<BlockCoord>(std::floor(z())) - pos.z == dir1.z) {
        setPosition(x(), y() + static_cast<f64>(dir1.y), z());
    } else if (dir2.y != 0 && static_cast<BlockCoord>(std::floor(x())) - pos.x == dir2.x &&
        static_cast<BlockCoord>(std::floor(z())) - pos.z == dir2.z) {
        setPosition(x(), y() + static_cast<f64>(dir2.y), z());
    }

    // 应用摩擦力
    applyDrag();

    // Y坐标校正
    Vector3 vector3d3 = _getPosOnRail(x(), y(), z());
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

    // 区块边界处理
    BlockCoord j = static_cast<BlockCoord>(std::floor(x()));
    BlockCoord i = static_cast<BlockCoord>(std::floor(z()));
    if (j != pos.x || i != pos.z) {
        f64 d26 = std::sqrt(velocityX() * velocityX() + velocityZ() * velocityZ());
        setVelocity(static_cast<f32>(d26 * static_cast<f64>(j - pos.x)),
            velocityY(),
            static_cast<f32>(d26 * static_cast<f64>(i - pos.z)));
    }

    // 激活铁轨回调：矿车实体自行检测铁轨类型并调用 onActivatorRailPass，
    // 而不是通过方块回调。当矿车位于激活铁轨上时，根据充能状态触发回调。
    if (shouldDoRailFunctions() && abstractRailBlock != nullptr && railState->is(VanillaBlocks::ACTIVATOR_RAIL)) {
        bool powered = ActivatorRailBlock::isPowered(*railState);
        onActivatorRailPass(pos.x, pos.y, pos.z, powered);
    }

    // 动力铁轨加速
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
                if (_isNormalBlockAt(pos.offset(Direction::West))) {
                    d20 = 0.02;
                } else if (_isNormalBlockAt(pos.offset(Direction::East))) {
                    d20 = -0.02;
                }
            } else if (railshape == RailShape::NorthSouth) {
                if (_isNormalBlockAt(pos.offset(Direction::North))) {
                    d21 = 0.02;
                } else if (_isNormalBlockAt(pos.offset(Direction::South))) {
                    d21 = -0.02;
                }
            }
            setVelocity(static_cast<f32>(d20), velocityY(), static_cast<f32>(d21));
        }
    }
}

void AbstractMinecartEntity::_moveDerailedMinecart()
{
    // 脱轨移动处理
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

std::pair<Vector3, Vector3> AbstractMinecartEntity::_getRailDirectionVectors(RailShape shape) const
{
    // 方向向量映射
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

Vector3 AbstractMinecartEntity::_getPosOnRail(f64 x, f64 y, f64 z) const
{
    // 计算矿车在铁轨上的精确位置

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
    auto [vector3i, vector3i1] = _getRailDirectionVectors(railshape);

    // 关键的 Y 坐标计算
    // 注意: d1 = (double)j + RAIL_HEIGHT_OFFSET + (double)vector3i.getY() * 0.5
    // 这里有一个 1/16 方块的基础偏移
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
    // 摩擦力取决于是否有乘客
    f64 drag = getPassengers().empty() ? EMPTY_DRAG : OCCUPIED_DRAG;
    setVelocity(velocityX() * static_cast<f32>(drag), velocityY(), velocityZ() * static_cast<f32>(drag));
}

void AbstractMinecartEntity::_handleEntityCollisions()
{
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

        auto entityType = entity->entityType();

        // 跳过玩家（玩家碰撞由其他逻辑处理）
        if (entityType == entity::VanillaEntityTypeKeys::PLAYER) {
            continue;
        }

        // 矿车间碰撞处理
        if (entityType == entity::VanillaEntityTypeKeys::MINECART) {
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

void AbstractMinecartEntity::_handleMinecartCollisions()
{
    // 已在 _handleEntityCollisions() 中处理
    // 此方法保留为空以保持API兼容性
}

void AbstractMinecartEntity::_updateRollingAnimation()
{
    // 摇晃动画更新
    if (m_rollingAmplitude > 0) {
        m_rollingAmplitude--;
    }
}

void AbstractMinecartEntity::_updateRotation()
{
    // 根据移动方向更新朝向
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

bool AbstractMinecartEntity::_isPoweredRail(const BlockPos& pos) const
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

bool AbstractMinecartEntity::_isDetectorRail(const BlockPos& pos) const
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

bool AbstractMinecartEntity::_isActivatorRail(const BlockPos& pos) const
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

bool AbstractMinecartEntity::_isRailPowered(const BlockPos& pos)
{
    // 检查铁轨是否接收红石信号
    IWorld* worldPtr = Entity::world();
    if (!worldPtr) {
        return false;
    }

    return world::redstone::RedstonePower::isPowered(*worldPtr, pos);
}

bool AbstractMinecartEntity::_isNormalBlockAt(const BlockPos& pos) const
{
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
    return state->isSolid() && state->isOpaque() && !state->isAir();
}

void AbstractMinecartEntity::_moveMinecartOnRail(const BlockPos& pos)
{
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
    // 如果不在铁轨上，使用基础最大速度
    if (!m_onRail) {
        return getMaxSpeed();
    }

    // 使用 max_minecart_speed 游戏规则计算铁轨最大速度：
    //   速度 = 规则值（默认 8） * (在水中 ? 0.5 : 1.0) / 20.0
    // 默认值 8 / 20.0 = 0.4 方块/刻
    const IWorld* worldPtr = world();
    if (!worldPtr) {
        return getMaxSpeed();
    }

    i32 maxSpeedRule = worldPtr->getGameRules().getInt(world::gamerule::GameRuleKeys::MAX_MINECART_SPEED);
    // 限制范围为 [1, 1000]，与 MC Java 一致
    maxSpeedRule = std::clamp(maxSpeedRule, 1, 1000);

    f64 maxSpeed = static_cast<f64>(maxSpeedRule) / 20.0;
    if (isInWater()) {
        maxSpeed *= 0.5;
    }

    return static_cast<f32>(maxSpeed);
}

void AbstractMinecartEntity::dropItem(DamageSource* source)
{
    MC_UNUSED(source);
    // 根据矿车类型掉落对应物品

    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        remove();
        return;
    }

    // 检查游戏规则 doEntityDrops：当该规则为 false 时，矿车被摧毁不产生掉落物品
    // 参考 VehicleEntity.destroy() 中的 ENTITY_DROPS 检查
    if (!worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
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
    m_rollingDirection = -m_rollingDirection;

    // 5. 设置摇晃时间
    m_rollingAmplitude = 10;

    // 6. 标记速度已改变（用于同步到客户端）
    markHurt();

    // 7. 累积伤害
    m_damage += static_cast<i32>(amount * 10.0f);

    // 8. 检查攻击者是否为创造模式玩家
    bool isCreative = false;
    Entity* attacker = source.source();
    if (attacker != nullptr && attacker->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
        Player* player = static_cast<Player*>(attacker);
        isCreative = player->isCreative();
    }

    // 9. 检查是否应该摧毁矿车
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
    return std::make_unique<RideableMinecartEntity>(EntityInstanceId(0));
}

void RideableMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered)
{
    // 激活铁轨弹出乘客
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);

    if (powered) {
        // 弹出所有乘客
        const auto& passengerIds = getPassengers();
        IWorld* worldPtr = Entity::world();
        if (worldPtr) {
            for (EntityInstanceId passengerId : passengerIds) {
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
    return std::make_unique<ChestMinecartEntity>(EntityInstanceId(0));
}

ChestMinecartEntity::ChestMinecartEntity(EntityInstanceId id)
    : AbstractMinecartEntity(Type::Chest, id)
    , m_inventory(std::make_unique<blockentity::SimpleInventory>(INVENTORY_SIZE))
{}

void ChestMinecartEntity::applyDrag()
{
    // 箱子矿车摩擦力：根据红石信号强度增加摩擦
    f32 drag = 0.98f;

    // 根据容器红石信号强度增加摩擦力
    i32 signal = getComparatorOutput();
    drag -= static_cast<f32>(signal) * 0.001f;

    setVelocity(velocityX() * drag, velocityY(), velocityZ() * drag);
}

void ChestMinecartEntity::dropItem(DamageSource* source)
{
    MC_UNUSED(source);
    // 先掉落库存内容，再掉落矿车物品
    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        remove();
        return;
    }

    // 容器内容物掉落也受 doEntityDrops 游戏规则控制
    // 参考 ContainerEntity.chestVehicleDestroyed() 中的 ENTITY_DROPS 检查
    bool doEntityDrops = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS);

    if (doEntityDrops && m_inventory) {
        // 掉落所有库存物品
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

    // 调用父类方法掉落矿车物品（父类内部也会检查 doEntityDrops）
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

i32 ChestMinecartEntity::getComparatorOutput() const
{
    if (!m_inventory) {
        return 0;
    }
    return world::redstone::RedstoneHelper::calcRedstoneFromInventory(*m_inventory);
}

// ============================================================================
// FurnaceMinecartEntity
// ============================================================================

std::unique_ptr<Entity> FurnaceMinecartEntity::create(IWorld* /*world*/)
{
    return std::make_unique<FurnaceMinecartEntity>(EntityInstanceId(0));
}

void FurnaceMinecartEntity::tick()
{
    AbstractMinecartEntity::tick();

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

    // 燃烧时产生烟雾粒子
    // 激活状态下每 tick 有 1/4 概率产生大烟雾粒子
    if (isActivated() && worldPtr != nullptr) {
        math::Random& random = worldPtr->getRandom();
        if (random.nextInt(4) == 0) {
            using namespace particle;
            worldPtr->addParticle(ParticleTypeId::LargeSmoke, Vector3(x(), y() + 0.8, z()), Vector3(0.0, 0.0, 0.0));
        }
    }
}

void FurnaceMinecartEntity::updatePushDirection()
{
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
    // 熔炉矿车摩擦力计算
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
    // 燃料上限检查
    if (m_fuel + ticks <= MAX_FUEL) {
        m_fuel += ticks;
    } else {
        m_fuel = MAX_FUEL;
    }
}

void FurnaceMinecartEntity::activate()
{
    // 添加燃料（玩家交互时）
    addFuel(3600); // 3分钟 = 180秒 * 20 ticks/秒
}

void FurnaceMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered)
{
    // 激活铁轨可以改变熔炉矿车的推动方向
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
    // 先调用父类方法掉落矿车物品（父类内部会检查 doEntityDrops）
    AbstractMinecartEntity::dropItem(source);

    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        return;
    }

    // 如果不是爆炸伤害且游戏规则允许实体掉落，则掉落熔炉方块
    // 参考 MC 1.16.5 FurnaceMinecartEntity.killMinecart()：
    //   非爆炸销毁时额外掉落熔炉方块（MC 1.21.11 MinecartFurnace 已移除此行为，仅掉落 FURNACE_MINECART 物品）
    bool isExplosion = (source != nullptr && source->isExplosion());
    bool doEntityDrops = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS);

    if (!isExplosion && doEntityDrops) {
        // 通过 BlockItemRegistry 获取熔炉方块物品并掉落
        const BlockItem* furnaceBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::FURNACE);
        if (furnaceBlockItem != nullptr) {
            ItemStack stack(*furnaceBlockItem, 1);
            math::Random& rng = worldPtr->getRandom();
            ItemDropHelper::spawnItemEntity(worldPtr, stack, x(), y(), z(), rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
        }
    }
}

// ============================================================================
// TNTMinecartEntity
// ============================================================================

std::unique_ptr<Entity> TNTMinecartEntity::create(IWorld* /*world*/)
{
    return std::make_unique<TNTMinecartEntity>(EntityInstanceId(0));
}

void TNTMinecartEntity::tick()
{
    AbstractMinecartEntity::tick();

    // TNT引信倒计时
    if (m_fuse > 0) {
        m_fuse--;

        // 引信燃烧时产生烟雾粒子
        // 引信每 4 tick 产生一次烟雾
        if (m_fuse % 4 == 0) {
            IWorld* worldPtr = Entity::world();
            if (worldPtr) {
                using namespace particle;
                worldPtr->addParticle(ParticleTypeId::Smoke, Vector3(x(), y() + 0.5, z()), Vector3(0.0, 0.0, 0.0));
            }
        }

        if (m_fuse == 0) {
            // 引信归零时爆炸，传递引爆来源作为伤害归因
            f64 speedSq = velocityX() * velocityX() + velocityZ() * velocityZ();
            _explode(static_cast<f32>(std::sqrt(speedSq)), m_ignitionSource.get());
        }
    }

    // 检查火焰接触（在火焰/岩浆中自动点燃）
    _checkFireIgnition();

    // 水平碰撞检测（高速碰撞时爆炸）
    if (m_collidedHorizontally) {
        f64 speedSq = velocityX() * velocityX() + velocityZ() * velocityZ();
        if (speedSq >= 0.01) {
            _explode(static_cast<f32>(std::sqrt(speedSq)), m_ignitionSource.get());
        }
    }
}

void TNTMinecartEntity::_ignite(const DamageSource* source)
{
    // 对应 MC Java 的 MinecartTNT.primeFuse() 中的 GameRules.TNT_EXPLODES 检查
    // 如果 tntExplodes 游戏规则为 false，则不点燃
    IWorld* worldPtr = Entity::world();
    if (worldPtr && !worldPtr->isClientSide()) {
        if (!worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES)) {
            return;
        }
    }

    // 对应 MC Java 的 primeFuse(DamageSource) 中的 ignitionSource 设置逻辑
    // 首次点燃时记录引爆来源，后续不再覆盖
    if (source != nullptr && m_ignitionSource == nullptr) {
        // 将原始伤害源转换为爆炸伤害源：
        // directEntity = this（TNT矿车自身），causeEntity = 原始伤害的造成者
        Entity* causeEntity = source->getEntity();
        m_ignitionSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Explosion, causeEntity, this);
        static_cast<IndirectEntityDamageSource*>(m_ignitionSource.get())->setExplosion();
    }

    m_fuse = DEFAULT_FUSE; // 80 ticks = 4 seconds

    if (worldPtr && !worldPtr->isClientSide()) {
        // 广播实体状态 10，通知客户端 TNT 矿车已被引燃
        // 客户端收到 status 10 后设置 fuse 值以渲染闪烁效果
        worldPtr->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatusPacket::Status::EatBlock));

        // 播放 TNT 引燃音效
        playSound(SoundEvents::ENTITY_TNT_PRIMED, 1.0f, 1.0f);
    }
}

void TNTMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered)
{
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);

    // 激活铁轨充能时点燃TNT，无伤害来源（对应 MC Java 中 primeFuse(null)）
    if (powered && m_fuse < 0) {
        _ignite(nullptr);
    }
}

bool TNTMinecartEntity::hurt(DamageSource& source, f32 amount)
{
    // 检查是否为燃烧的箭矢
    Entity* directSource = source.directSource();
    if (directSource != nullptr) {
        // 检查是否为 AbstractArrowEntity（包括 ArrowEntity 和 SpectralArrowEntity）
        AbstractArrowEntity* arrow = dynamic_cast<AbstractArrowEntity*>(directSource);
        if (arrow != nullptr && arrow->isOnFire()) {
            // 检测燃烧箭矢引爆 TNT 矿车，使用箭矢的速度计算爆炸威力
            // 对应 MC Java 中 hurtServer() 的燃烧箭矢处理：
            // 直接爆炸（不经过点燃流程），将箭矢射手作为间接爆炸源
            Vector3 arrowVelocity = arrow->velocity();
            f64 speedSq = static_cast<f64>(arrowVelocity.x) * arrowVelocity.x +
                static_cast<f64>(arrowVelocity.y) * arrowVelocity.y +
                static_cast<f64>(arrowVelocity.z) * arrowVelocity.z;

            Entity* shooter = source.getEntity();
            auto explosionSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Explosion, shooter, this);
            explosionSource->setExplosion();
            _explode(static_cast<f32>(std::sqrt(speedSq)), explosionSource.get());
            return true;
        }

        // 兼容：检查其他带火焰的投射物（如火球）
        if (source.isProjectile() && source.isFire()) {
            f64 speedSq = velocityX() * velocityX() + velocityZ() * velocityZ();
            // 火焰投射物直接爆炸，将投射物的射击者作为间接爆炸源
            Entity* shooter = source.getEntity();
            auto explosionSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Explosion, shooter, this);
            explosionSource->setExplosion();
            _explode(static_cast<f32>(std::sqrt(speedSq)), explosionSource.get());
            return true;
        }
    }

    // 对应 MC Java 中 VehicleEntity.hurtServer() 的 shouldSourceDestroy 检查
    // TNT矿车重写了 shouldSourceDestroy：当伤害源能点燃TNT时，即使伤害未超过阈值也触发 destroy()
    // 这确保了火焰/爆炸伤害能立即点燃TNT矿车，而不需要累积足够伤害
    if (_damageSourceIgnitesTnt(source)) {
        removePassengers();
        dropItem(&source);
        return true;
    }

    return AbstractMinecartEntity::hurt(source, amount);
}

bool TNTMinecartEntity::onProjectileHit(DamageSource& source, f32 amount)
{
    // 燃烧箭矢命中时直接爆炸
    return hurt(source, amount);
}

void TNTMinecartEntity::dropItem(DamageSource* source)
{
    // 对应 MC Java 的 MinecartTNT.destroy() 方法
    f64 speedSq = velocityX() * velocityX() + velocityZ() * velocityZ();

    // 判断伤害类型是否能点燃TNT
    bool ignitesTnt = (source != nullptr && _damageSourceIgnitesTnt(*source));

    // 如果不能点燃且速度足够低，则正常掉落
    if (!ignitesTnt && speedSq < 0.01) {
        // 先掉落矿车物品（父类内部会检查 doEntityDrops）
        AbstractMinecartEntity::dropItem(source);

        // 如果不是爆炸伤害且游戏规则允许实体掉落，则额外掉落 TNT 方块
        bool isExplosion = (source != nullptr && source->isExplosion());
        if (!isExplosion) {
            IWorld* worldPtr = world();
            if (worldPtr && !worldPtr->isClientSide()) {
                if (worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
                    // 通过 BlockItemRegistry 获取 TNT 方块物品
                    const BlockItem* tntBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::TNT);
                    if (tntBlockItem != nullptr) {
                        ItemStack stack(*tntBlockItem, 1);
                        math::Random& rng = worldPtr->getRandom();
                        ItemDropHelper::spawnItemEntity(
                            worldPtr, stack, x(), y(), z(), rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
                    }
                }
            }
        }
    } else {
        // 火焰/爆炸伤害或高速碰撞时点燃 TNT 矿车
        // 对应 MC Java 中 destroy() 里的 primeFuse(damageSource)
        if (m_fuse < 0) {
            _ignite(source);
            // 随机点燃时间 0-38 ticks（对应 MC Java 的 random.nextInt(20) + random.nextInt(20)）
            IWorld* worldPtr = world();
            if (worldPtr) {
                math::Random& rng = worldPtr->getRandom();
                m_fuse = rng.nextInt(20) + rng.nextInt(20);
            } else {
                m_fuse = 20;
            }
        }
    }
}

void TNTMinecartEntity::_checkFireIgnition()
{
    // 在火焰或岩浆中自动点燃（无伤害来源，对应 MC Java 中红石/火焰直接点燃）
    if (m_fuse < 0) {
        if (isOnFire() || isInLava()) {
            _ignite(nullptr);
        }
    }
}

bool TNTMinecartEntity::_damageSourceIgnitesTnt(const DamageSource& source)
{
    // 对应 MC Java 的 MinecartTNT.damageSourceIgnitesTnt()
    // 判断逻辑：
    // 1. 如果直接实体是投射物且着火 → 能点燃
    // 2. 否则，如果伤害类型是火焰（IS_FIRE）→ 能点燃
    // 3. 否则，如果伤害类型是爆炸（IS_EXPLOSION）→ 能点燃
    Entity* directEntity = source.directSource();
    if (directEntity != nullptr) {
        auto* projectile = dynamic_cast<ProjectileEntity*>(directEntity);
        if (projectile != nullptr && projectile->isOnFire()) {
            return true;
        }
    }
    return source.isFire() || source.isExplosion();
}

void TNTMinecartEntity::_explode(f32 speedFactor, const DamageSource* damageSource)
{
    IWorld* worldPtr = Entity::world();
    if (!worldPtr || worldPtr->isClientSide()) {
        remove();
        return;
    }

    // 对应 MC Java 的 MinecartTNT.explode() 中的 GameRules.TNT_EXPLODES 检查
    // 如果 tntExplodes 游戏规则为 false，不产生爆炸；
    // 如果矿车已被点燃，则移除实体；否则实体保持不变
    if (!worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES)) {
        if (isPrimed()) {
            remove();
        }
        return;
    }

    // 爆炸威力：基础4.0，速度加成最大到5.0
    f64 d0 = static_cast<f64>(speedFactor);
    if (d0 > 5.0) {
        d0 = 5.0;
    }

    math::Random& rng = worldPtr->getRandom();
    f32 radius = static_cast<f32>(4.0 + rng.nextDouble() * 1.5 * d0);

    // 创建爆炸（TNT矿车爆炸时不破坏铁轨）
    // canExplosionDestroyBlock: 如果已点燃且目标方块是铁轨，返回 false
    // 使用 createExplosionWithSource 确保爆炸包广播给客户端，并传递自定义伤害来源
    // createExplosionWithSource 内部会 clone damageSource，此处无需 clone
    worldPtr->createExplosionWithSource(Vector3(static_cast<f32>(x()), static_cast<f32>(y()), static_cast<f32>(z())),
        radius,
        world::explosion::ExplosionMode::Break,
        false,         // 不产生火焰
        this,          // TNT矿车自身作为爆炸源
        damageSource); // 自定义伤害来源（可能为nullptr，使用默认爆炸伤害）

    remove();
}

// ============================================================================
// HopperMinecartEntity
// ============================================================================

std::unique_ptr<Entity> HopperMinecartEntity::create(IWorld* /*world*/)
{
    return std::make_unique<HopperMinecartEntity>(EntityInstanceId(0));
}

HopperMinecartEntity::HopperMinecartEntity(EntityInstanceId id)
    : AbstractMinecartEntity(Type::Hopper, id)
    , m_inventory(std::make_unique<blockentity::SimpleInventory>(INVENTORY_SIZE))
{}

void HopperMinecartEntity::tick()
{
    AbstractMinecartEntity::tick();

    // 冷却计时
    if (m_suckCooldown > 0) {
        m_suckCooldown--;
    }

    IWorld* worldPtr = Entity::world();
    if (!worldPtr || worldPtr->isClientSide()) {
        return;
    }

    // 漏斗矿车的启用/禁用状态由 onActivatorRailPass 回调控制：
    // - 充能的激活铁轨 → m_disabled = true（禁用吸取和传输）
    // - 未充能的激活铁轨 → m_disabled = false（启用）
    // - 不在激活铁轨上时，保持当前状态不变
    // 注意：漏斗矿车离开激活铁轨后不会自动重新启用，
    // 必须经过一个未充能的激活铁轨才会重新启用。

    // 如果被禁用，跳过吸取和传输
    if (m_disabled) {
        return;
    }

    // 尝试吸取物品
    if (canSuckItems()) {
        _suckItems();
    }

    // 尝试向下传输物品
    _transferItemsOut();
}

void HopperMinecartEntity::_suckItems()
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

void HopperMinecartEntity::_transferItemsOut()
{
    IWorld* worldPtr = Entity::world();
    if (!worldPtr || !m_inventory) {
        return;
    }

    // 获取下方容器
    BlockPos belowPos = getHopperPos().offset(Direction::Down);
    InventoryRef targetInventoryRef = blockentity::HopperEntity::getInventoryAtPosition(worldPtr, belowPos);
    IInventory* targetInventory = targetInventoryRef.get();

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

void HopperMinecartEntity::dropItem(DamageSource* source)
{
    MC_UNUSED(source);
    // 先掉落库存内容，再掉落矿车物品
    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        remove();
        return;
    }

    // 容器内容物掉落受 doEntityDrops 游戏规则控制
    // 参考 ContainerEntity.chestVehicleDestroyed() 中的 ENTITY_DROPS 检查
    bool doEntityDrops = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS);

    if (doEntityDrops && m_inventory) {
        // 掉落所有库存物品
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

    // 调用父类方法掉落矿车物品（父类内部也会检查 doEntityDrops）
    AbstractMinecartEntity::dropItem(source);
}

i32 HopperMinecartEntity::getComparatorOutput() const
{
    if (!m_inventory) {
        return 0;
    }
    return world::redstone::RedstoneHelper::calcRedstoneFromInventory(*m_inventory);
}

void HopperMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered)
{
    // 激活铁轨控制漏斗状态
    // 注意语义：
    // - isBlocked = true 表示漏斗可以工作（吸取/传输）
    // - isBlocked = false 表示漏斗被禁用
    //
    // 当激活铁轨充能时（powered = true）：
    //   flag = !true = false，setBlocked(false) -> 禁用漏斗
    // 当激活铁轨未充能时（powered = false）：
    //   flag = !false = true，setBlocked(true) -> 启用漏斗
    //
    // 我们使用 m_disabled：
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

    // 命令方块矿车不自动执行命令
    // 命令只在通过激活铁轨时执行
}

i32 CommandBlockMinecartEntity::getComparatorOutput() const
{
    return std::min(m_successCount, 15);
}

void CommandBlockMinecartEntity::onActivatorRailPass(i32 x, i32 y, i32 z, bool powered)
{
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);

    // 激活铁轨触发命令执行
    // 只在上升沿执行（从不激活变为激活）
    if (powered && !mPowered) {
        mPowered = true;
        _executeCommand();
    } else if (!powered) {
        mPowered = false;
    }
}

void CommandBlockMinecartEntity::_executeCommand()
{
    // 通过 IWorld 接口执行命令
    // 命令方块矿车的权限级别为 2（相当于 OP 级别）

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

// ============================================================================
// SpawnerMinecartEntity
// ============================================================================

SpawnerMinecartEntity::SpawnerMinecartEntity(EntityInstanceId id)
    : AbstractMinecartEntity(Type::Spawner, id)
{}

std::unique_ptr<Entity> SpawnerMinecartEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SpawnerMinecartEntity>(EntityInstanceId(0));
}

void SpawnerMinecartEntity::tick()
{
    // 先调用基类 tick（铁轨运动、碰撞等）
    AbstractMinecartEntity::tick();

    // 刷怪笼逻辑 tick
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    if (worldPtr->isClientSide()) {
        // 客户端：更新旋转动画
        m_spawnerLogic.clientTick(*worldPtr, x(), y(), z());
    } else {
        // 服务端：执行刷怪逻辑
        m_spawnerLogic.serverTick(*worldPtr, x(), y(), z(), [this](IWorld& w) {
            // 成功生成实体后广播刷怪笼粒子事件
            // 对应 MC Java BaseSpawner.serverTick() 中成功生成后调用
            // level.broadcastEntityEvent(MinecartSpawner.this, (byte)1)
            // 参考 EntityStatusPacket::Status::SpawnerEvent(1)
            w.broadcastEntityStatus(id(), static_cast<u8>(1));
        });
    }
}

void SpawnerMinecartEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    AbstractMinecartEntity::addAdditionalSaveData(tag);

    // 保存刷怪笼逻辑数据
    nbt::CompoundTag spawnerTag;
    m_spawnerLogic.saveToNBT(spawnerTag);

    // 将刷怪笼数据合并到实体标签中
    for (auto& [key, value] : spawnerTag.value) {
        tag.value.emplace(key, std::move(value));
    }
}

Result<void> SpawnerMinecartEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    auto result = AbstractMinecartEntity::readAdditionalSaveData(tag);
    if (!result.success()) {
        return result;
    }

    // 读取刷怪笼逻辑数据
    m_spawnerLogic.loadFromNBT(tag);

    return Result<void>();
}

} // namespace entity
} // namespace mc