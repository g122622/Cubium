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

#include "IDispenseItemBehavior.hpp"

#include "../../../core/Types.hpp"
#include "../../../entity/entities/item/ItemEntity.hpp"
#include "../../../entity/entities/projectile/AbstractArrowEntity.hpp"
#include "../../../entity/entities/projectile/OtherProjectiles.hpp"
#include "../../../entity/entities/projectile/ProjectileEntity.hpp"
#include "../../../entity/entities/projectile/ProjectileItemEntity.hpp"
#include "../../../entity/entities/vehicle/BoatEntity.hpp"
#include "../../../item/Items.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../IWorld.hpp"
#include "../../WorldEvents.hpp"
#include "../../fluid/Fluid.hpp"
#include "../../fluid/FluidRegistry.hpp"
#include "../../fluid/FluidTags.hpp"
#include "../Block.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// DefaultDispenseItemBehavior
// ============================================================================

ItemStack DefaultDispenseItemBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack)
{
    // 获取发射方向
    Direction direction = state.get(BlockStateProperties::FACING());

    // 执行投掷
    ItemStack result = doDispense(world, pos, state, stack, direction, 6.0f, 6.0f);

    // 播放音效和粒子
    playSound(world, pos);
    spawnParticles(world, pos, direction);

    return result;
}

ItemStack DefaultDispenseItemBehavior::doDispense(IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    ItemStack& stack,
    Direction direction,
    f32 speed,
    f32 inaccuracy)
{
    MC_UNUSED(state);
    MC_UNUSED(inaccuracy);

    if (stack.isEmpty()) {
        return stack;
    }

    // 分离出1个物品
    ItemStack dispensedStack = stack.split(1);
    if (dispensedStack.isEmpty()) {
        return stack;
    }

    // 计算发射位置（MC 1.16.5 算法）
    // 发射口位置 = 方块中心 + 方向偏移 * 0.7
    Vector3 dispensePos = getDispensePosition(pos, direction);

    // Y轴偏移调整（MC 1.16.5 DefaultDispenseItemBehavior.doDispense）
    // 使物品看起来从发射口出来
    f32 adjustedY = dispensePos.y;
    if (Directions::getAxis(direction) == Axis::Y) {
        adjustedY -= 0.125f; // 向上/向下时
    } else {
        adjustedY -= 0.15625f; // 水平方向时
    }

    // 获取随机数生成器
    math::Random& rng = world.getRandom();

    // 计算速度（MC 1.16.5 算法）
    // 基础速度 d3 = random(0.1) + 0.2，范围 [0.2, 0.3]
    f32 baseVelocity = static_cast<f32>(rng.nextDouble() * 0.1 + 0.2);

    // 速度计算：
    // vx = random.gaussian() * 0.0075 * speed + direction.xOffset * baseVelocity
    // vy = random.gaussian() * 0.0075 * speed + 0.2
    // vz = random.gaussian() * 0.0075 * speed + direction.zOffset * baseVelocity
    f32 gaussianFactor = 0.0075f * speed;
    f32 vx = static_cast<f32>(rng.nextGaussian()) * gaussianFactor +
        static_cast<f32>(Directions::xOffset(direction)) * baseVelocity;
    f32 vy = static_cast<f32>(rng.nextGaussian()) * gaussianFactor + 0.2f;
    f32 vz = static_cast<f32>(rng.nextGaussian()) * gaussianFactor +
        static_cast<f32>(Directions::zOffset(direction)) * baseVelocity;

    // 创建物品实体（ItemEntity在mc命名空间，不在mc::entity中）
    auto itemEntity = std::make_unique<ItemEntity>(EntityId(0), // ID由世界分配
        dispensedStack,
        dispensePos.x,
        adjustedY,
        dispensePos.z,
        vx,
        vy,
        vz);

    // 设置拾取延迟（发射器发射的物品不能立即被拾取）
    itemEntity->setPickupDelay(10);

    // 添加到世界
    world.spawnEntity(std::move(itemEntity));

    // 返回剩余物品
    return stack;
}

void DefaultDispenseItemBehavior::playSound(IWorld& world, const BlockPos& pos)
{
    // MC 1.16.5: 播放发射音效 (事件ID 1000)
    // 参考: DefaultDispenseItemBehavior.playSound()
    if (!world.isClientSide()) {
        world.playEvent(world::WorldEvents::DISPENSER_DISPENSE_SOUND, pos, 0);
    }
}

void DefaultDispenseItemBehavior::spawnParticles(IWorld& world, const BlockPos& pos, Direction direction)
{
    // MC 1.16.5: 生成发射烟雾粒子 (事件ID 2000，数据为方向索引)
    // 参考: DefaultDispenseItemBehavior.spawnDispenseParticles()
    if (!world.isClientSide()) {
        // 服务端通过世界事件广播粒子
        world.playEvent(world::WorldEvents::DISPENSER_SMOKE, pos, static_cast<i32>(direction));
    }
}

Vector3 DefaultDispenseItemBehavior::getDispensePosition(const BlockPos& pos, Direction direction)
{
    // 计算发射位置：从方块面中心稍微向外偏移
    // MC 1.16.5: DispenserBlock.getDispensePosition()
    f32 x = static_cast<f32>(pos.x) + 0.5f + static_cast<f32>(Directions::xOffset(direction)) * 0.7f;
    f32 y = static_cast<f32>(pos.y) + 0.5f + static_cast<f32>(Directions::yOffset(direction)) * 0.7f;
    f32 z = static_cast<f32>(pos.z) + 0.5f + static_cast<f32>(Directions::zOffset(direction)) * 0.7f;
    return Vector3(x, y, z);
}

// ============================================================================
// OptionalDispenseItemBehavior
// ============================================================================

void OptionalDispenseItemBehavior::playSound(IWorld& world, const BlockPos& pos)
{
    // MC 1.16.5: 根据成功/失败播放不同音效
    // 成功: 1000 (DISPENSER_DISPENSE_SOUND)
    // 失败: 1001 (DISPENSER_FAIL_SOUND)
    if (!world.isClientSide()) {
        i32 eventId =
            m_success ? world::WorldEvents::DISPENSER_DISPENSE_SOUND : world::WorldEvents::DISPENSER_FAIL_SOUND;
        world.playEvent(eventId, pos, 0);
    }
}

void OptionalDispenseItemBehavior::spawnParticles(IWorld& world, const BlockPos& pos, Direction direction)
{
    // 只有成功时才生成粒子
    if (m_success) {
        DefaultDispenseItemBehavior::spawnParticles(world, pos, direction);
    }
}

// ============================================================================
// ProjectileDispenseBehavior
// ============================================================================

ProjectileDispenseBehavior::ProjectileDispenseBehavior(
    std::function<std::unique_ptr<mc::Entity>(IWorld&, const Vector3&, const ItemStack&)> createProjectile,
    f32 velocity,
    f32 inaccuracy)
    : m_createProjectile(std::move(createProjectile))
    , m_velocity(velocity)
    , m_inaccuracy(inaccuracy)
{}

ItemStack ProjectileDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack)
{
    // 获取发射方向
    Direction direction = state.get(BlockStateProperties::FACING());

    // 计算发射位置
    Vector3 dispensePos = getDispensePosition(pos, direction);

    // 创建投掷物实体
    std::unique_ptr<mc::Entity> projectile = m_createProjectile(world, dispensePos, stack);
    if (!projectile) {
        // 创建失败，使用默认行为
        return DefaultDispenseItemBehavior::dispense(world, pos, state, stack);
    }

    // 设置投掷物的位置
    projectile->setPosition(dispensePos.x, dispensePos.y, dispensePos.z);

    // 转换为 ProjectileEntity 指针
    entity::ProjectileEntity* projectileEntity = dynamic_cast<entity::ProjectileEntity*>(projectile.get());
    if (!projectileEntity) {
        // 不是投掷物，使用默认行为
        return DefaultDispenseItemBehavior::dispense(world, pos, state, stack);
    }

    // 设置发射方向和速度（MC 1.16.5 算法）
    // shoot(x, y, z, velocity, inaccuracy)
    // Y方向额外+0.1使投掷物稍向上
    projectileEntity->shoot(static_cast<f32>(Directions::xOffset(direction)),
        static_cast<f32>(Directions::yOffset(direction)) + 0.1f,
        static_cast<f32>(Directions::zOffset(direction)),
        m_velocity,
        m_inaccuracy);

    // 添加到世界
    world.spawnEntity(std::move(projectile));

    // 减少物品数量
    stack.shrink(1);

    // 播放投掷物发射音效（事件ID 1002）
    if (!world.isClientSide()) {
        world.playEvent(world::WorldEvents::DISPENSER_LAUNCH_SOUND, pos, 0);
    }

    // 生成烟雾粒子
    spawnParticles(world, pos, direction);

    return stack.isEmpty() ? ItemStack() : stack;
}

// ============================================================================
// BoatDispenseBehavior
// ============================================================================

BoatDispenseBehavior::BoatDispenseBehavior(entity::BoatEntity::Type type)
    : m_boatType(type)
{}

ItemStack BoatDispenseBehavior::dispense(IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack)
{
    // 参考 MC 1.16.5: DispenseBoatBehavior
    Direction direction = state.get(BlockStateProperties::FACING());

    // 计算船的位置
    // MC 1.16.5: d0 = source.getX() + direction.getXOffset() * 1.125D
    f32 x = static_cast<f32>(pos.x) + 0.5f + static_cast<f32>(Directions::xOffset(direction)) * 1.125f;
    f32 y = static_cast<f32>(pos.y) + 0.5f + static_cast<f32>(Directions::yOffset(direction)) * 1.125f;
    f32 z = static_cast<f32>(pos.z) + 0.5f + static_cast<f32>(Directions::zOffset(direction)) * 1.125f;

    // 检查目标位置是否有水
    BlockPos targetPos = pos.offset(direction);
    const fluid::FluidState* fluidState = world.getFluidState(targetPos);

    // 检查是否是水
    bool isWater =
        fluidState != nullptr && !fluidState->isEmpty() && fluidState->getFluid().isIn(fluid::FluidTags::WATER());

    // 如果目标位置没有水，检查下方
    f32 waterLevel = 0.0f;
    if (isWater && (fluidState->isSource() || fluidState->getLevel() > 0)) {
        waterLevel = 1.0f; // 水面上方
    } else {
        // 检查下方是否有水
        BlockPos belowPos = targetPos.offset(Direction::Down);
        const fluid::FluidState* belowFluid = world.getFluidState(belowPos);
        bool isBelowWater =
            belowFluid != nullptr && !belowFluid->isEmpty() && belowFluid->getFluid().isIn(fluid::FluidTags::WATER());
        if (!isBelowWater || belowFluid->getLevel() == 0) {
            // 下方也没有水，作为普通物品发射
            return DefaultDispenseItemBehavior::dispense(world, pos, state, stack);
        }
        waterLevel = 0.0f; // 水位下方
    }

    // 创建船实体
    auto boatEntity = std::make_unique<entity::BoatEntity>(m_boatType);
    boatEntity->setPosition(x, y + waterLevel, z);

    // 添加到世界
    world.spawnEntity(std::move(boatEntity));

    // 减少物品数量
    stack.shrink(1);

    // 播放音效和粒子
    playSound(world, pos);
    spawnParticles(world, pos, direction);

    return stack.isEmpty() ? ItemStack() : stack;
}

// ============================================================================
// BucketDispenseBehavior
// ============================================================================

BucketDispenseBehavior::BucketDispenseBehavior(fluid::Fluid& fluid)
    : m_fluid(&fluid)
{}

ItemStack BucketDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack)
{
    // 参考 MC 1.16.5: BucketDispenseBehavior（放置流体）
    // TODO: 当 IWorld 支持放置流体后完善实现
    setSuccess(false);
    return stack;
}

// ============================================================================
// EmptyBucketDispenseBehavior
// ============================================================================

ItemStack EmptyBucketDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack)
{
    // 参考 MC 1.16.5: BucketDispenseBehavior（收集流体）
    // TODO: 当 IWorld 支持收取流体后完善实现
    setSuccess(false);
    return stack;
}

// ============================================================================
// FlintAndSteelDispenseBehavior
// ============================================================================

ItemStack FlintAndSteelDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack)
{
    // 参考 MC 1.16.5: FlintAndSteelDispenseBehavior
    // TODO: 完善 FlintAndSteelItem API 后完善实现
    setSuccess(false);
    return stack;
}

// ============================================================================
// BonemealDispenseBehavior
// ============================================================================

ItemStack BonemealDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack)
{
    // 参考 MC 1.16.5: BonemealDispenseBehavior
    // TODO: 完善 BoneMealItem API 后完善实现
    setSuccess(false);
    return stack;
}

} // namespace blocks
} // namespace mc
