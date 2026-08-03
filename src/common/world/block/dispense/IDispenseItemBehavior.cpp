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
#include "../../../entity/core/EntityRegistry.hpp"
#include "../../../entity/entities/item/ItemEntity.hpp"
#include "../../../entity/entities/misc/MiscEntities.hpp"
#include "../../../entity/entities/projectile/AbstractArrowEntity.hpp"
#include "../../../entity/entities/projectile/ProjectileEntity.hpp"
#include "../../../entity/entities/vehicle/BoatEntity.hpp"
#include "../../../entity/inventory/IInventory.hpp"
#include "../../../item/Items.hpp"
#include "../../../item/core/ProjectileItem.hpp"
#include "../../../item/items/special/BoneMealItem.hpp"
#include "../../../item/items/special/BucketItem.hpp"
#include "../../../item/items/special/FlintAndSteelItem.hpp"
#include "../../../item/items/special/PowderSnowBucketItem.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../IWorld.hpp"
#include "../../WorldEvents.hpp"
#include "../../block/Block.hpp"
#include "../../block/IBucketPickupHandler.hpp"
#include "../../block/ILiquidContainer.hpp"
#include "../../block/blocks/nether/FireBlock.hpp"
#include "../../block/blocks/redstone/TNTBlock.hpp"
#include "../../block/registry/VanillaBlocks.hpp"
#include "../../fluid/Fluid.hpp"
#include "../../fluid/FluidTags.hpp"
#include "../../gamerule/GameRules.hpp"
#include "../../tick/manager/TickManager.hpp"
#include "../Block.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <functional>
#include <memory>
#include <utility>

namespace mc {
namespace blocks {

// ============================================================================
// DefaultDispenseItemBehavior
// ============================================================================

ItemStack DefaultDispenseItemBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack, IInventory* dispenserInventory)
{
    MC_UNUSED(dispenserInventory);
    // 获取发射方向
    Direction direction = state.get(BlockStateProperties::FACING());

    // 执行投掷
    ItemStack result = _doDispense(world, pos, state, stack, direction, 6.0f, 6.0f);

    // 播放音效和粒子
    _playSound(world, pos);
    _spawnParticles(world, pos, direction);

    return result;
}

ItemStack DefaultDispenseItemBehavior::_doDispense(IWorld& world,
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

    // 计算发射位置：方块中心 + 方向偏移
    Vector3 dispensePos = getDispensePosition(pos, direction);

    // Y轴偏移调整，使物品看起来从发射口出来
    f32 adjustedY = dispensePos.y;
    if (Directions::getAxis(direction) == Axis::Y) {
        adjustedY -= Y_AXIS_OFFSET;
    } else {
        adjustedY -= HORIZONTAL_Y_OFFSET;
    }

    // 获取随机数生成器
    math::Random& rng = world.getRandom();

    // 计算速度：基础速度范围 [BASE_VELOCITY_MIN, BASE_VELOCITY_MIN + BASE_VELOCITY_RANGE]
    f32 baseVelocity = static_cast<f32>(rng.nextDouble() * BASE_VELOCITY_RANGE + BASE_VELOCITY_MIN);

    // 速度计算：方向偏移 * 基础速度 + 高斯扰动
    f32 gaussianFactor = GAUSSIAN_FACTOR * speed;
    f32 vx = static_cast<f32>(rng.nextGaussian()) * gaussianFactor +
        static_cast<f32>(Directions::xOffset(direction)) * baseVelocity;
    f32 vy = static_cast<f32>(rng.nextGaussian()) * gaussianFactor + Y_VELOCITY_BASE;
    f32 vz = static_cast<f32>(rng.nextGaussian()) * gaussianFactor +
        static_cast<f32>(Directions::zOffset(direction)) * baseVelocity;

    // 创建物品实体
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(0), // ID由世界分配
        dispensedStack,
        dispensePos.x,
        adjustedY,
        dispensePos.z,
        vx,
        vy,
        vz);

    // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
    itemEntity->setTypeId(entity::EntityTypeKeys::ITEM);

    // 设置拾取延迟（发射器发射的物品不能立即被拾取）
    itemEntity->setPickupDelay(DEFAULT_PICKUP_DELAY);

    // 添加到世界
    world.spawnEntity(std::move(itemEntity));

    // 返回剩余物品
    return stack;
}

void DefaultDispenseItemBehavior::_playSound(IWorld& world, const BlockPos& pos)
{
    // 播放发射音效（事件ID 1000）
    if (!world.isClientSide()) {
        world.playEvent(world::WorldEvents::DISPENSER_DISPENSE_SOUND, pos, 0);
    }
}

void DefaultDispenseItemBehavior::_spawnParticles(IWorld& world, const BlockPos& pos, Direction direction)
{
    // 生成发射烟雾粒子（事件ID 2000，数据为方向索引）
    if (!world.isClientSide()) {
        // 服务端通过世界事件广播粒子
        world.playEvent(world::WorldEvents::DISPENSER_SMOKE, pos, static_cast<i32>(direction));
    }
}

void DefaultDispenseItemBehavior::_spawnItemEntity(
    IWorld& world, const BlockPos& pos, Direction direction, const ItemStack& itemStack)
{
    Vector3 dispensePos = getDispensePosition(pos, direction);

    // Y轴偏移调整
    f32 adjustedY = dispensePos.y;
    if (Directions::getAxis(direction) == Axis::Y) {
        adjustedY -= Y_AXIS_OFFSET;
    } else {
        adjustedY -= HORIZONTAL_Y_OFFSET;
    }

    // 计算速度
    math::Random& rng = world.getRandom();
    f32 baseVelocity = static_cast<f32>(rng.nextDouble() * BASE_VELOCITY_RANGE + BASE_VELOCITY_MIN);
    f32 gaussianFactor = GAUSSIAN_FACTOR * 6.0f;
    f32 vx = static_cast<f32>(rng.nextGaussian()) * gaussianFactor +
        static_cast<f32>(Directions::xOffset(direction)) * baseVelocity;
    f32 vy = static_cast<f32>(rng.nextGaussian()) * gaussianFactor + Y_VELOCITY_BASE;
    f32 vz = static_cast<f32>(rng.nextGaussian()) * gaussianFactor +
        static_cast<f32>(Directions::zOffset(direction)) * baseVelocity;

    auto itemEntity = std::make_unique<ItemEntity>(
        EntityInstanceId(0), itemStack, dispensePos.x, adjustedY, dispensePos.z, vx, vy, vz);

    // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
    itemEntity->setTypeId(entity::EntityTypeKeys::ITEM);

    itemEntity->setPickupDelay(DEFAULT_PICKUP_DELAY);
    world.spawnEntity(std::move(itemEntity));
}

// ============================================================================
// consumeWithRemainder & addToInventoryOrDispense
// ============================================================================

ItemStack DefaultDispenseItemBehavior::consumeWithRemainder(IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    ItemStack& original,
    const ItemStack& replacement,
    IInventory* dispenserInventory)
{
    // 将原始物品减一
    original.shrink(1);

    if (original.isEmpty()) {
        // 原始物品只有1个，直接返回替换物品
        // 调用者会将替换物品写回发射器原槽位
        return replacement;
    }

    // 原始物品还有剩余，尝试将替换物品放回发射器库存
    addToInventoryOrDispense(world, pos, state, replacement, dispenserInventory);

    // 返回剩余的原始物品
    return original;
}

void DefaultDispenseItemBehavior::addToInventoryOrDispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack, IInventory* dispenserInventory)
{
    if (stack.isEmpty()) {
        return;
    }

    // 尝试将替换物品插入发射器库存
    if (dispenserInventory != nullptr) {
        ItemStack remainder = dispenserInventory->addItem(stack);
        if (remainder.isEmpty()) {
            // 全部放入库存成功，无需弹出
            return;
        }
        // 库存放不下，将剩余物品弹出到世界中
        Direction direction = state.get(BlockStateProperties::FACING());
        _spawnItemEntity(world, pos, direction, remainder);
        return;
    }

    // 没有库存指针，直接弹出到世界
    Direction direction = state.get(BlockStateProperties::FACING());
    _spawnItemEntity(world, pos, direction, stack);
}

Vector3 DefaultDispenseItemBehavior::getDispensePosition(const BlockPos& pos, Direction direction)
{
    // 计算发射位置：从方块面中心稍微向外偏移
    f32 x = static_cast<f32>(pos.x) + 0.5f + static_cast<f32>(Directions::xOffset(direction)) * DISPENSE_OFFSET;
    f32 y = static_cast<f32>(pos.y) + 0.5f + static_cast<f32>(Directions::yOffset(direction)) * DISPENSE_OFFSET;
    f32 z = static_cast<f32>(pos.z) + 0.5f + static_cast<f32>(Directions::zOffset(direction)) * DISPENSE_OFFSET;
    return Vector3(x, y, z);
}

// ============================================================================
// OptionalDispenseItemBehavior
// ============================================================================

void OptionalDispenseItemBehavior::_playSound(IWorld& world, const BlockPos& pos)
{
    // 根据成功/失败播放不同音效
    // 成功: 1000 (DISPENSER_DISPENSE_SOUND)
    // 失败: 1001 (DISPENSER_FAIL_SOUND)
    if (!world.isClientSide()) {
        i32 eventId =
            m_success ? world::WorldEvents::DISPENSER_DISPENSE_SOUND : world::WorldEvents::DISPENSER_FAIL_SOUND;
        world.playEvent(eventId, pos, 0);
    }
}

void OptionalDispenseItemBehavior::_spawnParticles(IWorld& world, const BlockPos& pos, Direction direction)
{
    // 只有成功时才生成粒子
    if (m_success) {
        DefaultDispenseItemBehavior::_spawnParticles(world, pos, direction);
    }
}

// ============================================================================
// ProjectileDispenseBehavior
// ============================================================================

ProjectileDispenseBehavior::ProjectileDispenseBehavior(const item::ProjectileItem& projectileItem)
    : m_projectileItem(projectileItem)
{}

ItemStack ProjectileDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack, IInventory* dispenserInventory)
{
    MC_UNUSED(dispenserInventory);
    // 获取发射方向
    Direction direction = state.get(BlockStateProperties::FACING());

    // 计算发射位置
    Vector3 dispensePos = getDispensePosition(pos, direction);

    // 从 ProjectileItem 接口获取发射配置
    auto config = m_projectileItem.getDispenseConfig();
    f32 dirX = static_cast<f32>(Directions::xOffset(direction));
    f32 dirY = static_cast<f32>(Directions::yOffset(direction));
    f32 dirZ = static_cast<f32>(Directions::zOffset(direction));

    // 通过 ProjectileItem 接口创建弹射物实体
    std::unique_ptr<entity::ProjectileEntity> projectile =
        m_projectileItem.asProjectile(world, dispensePos, stack, dirX, dirY + 0.1f, dirZ);
    if (!projectile) {
        // 创建失败，使用默认行为
        return DefaultDispenseItemBehavior::dispense(world, pos, state, stack, dispenserInventory);
    }

    // 设置弹射物位置
    projectile->setPosition(dispensePos.x, dispensePos.y, dispensePos.z);

    // 通过 ProjectileItem 接口发射弹射物
    m_projectileItem.shoot(*projectile, dirX, dirY + 0.1f, dirZ, config.power, config.uncertainty);

    // 添加到世界
    world.spawnEntity(std::move(projectile));

    // 减少物品数量
    stack.shrink(1);

    // 播放投掷物发射音效（事件ID 1002）
    if (!world.isClientSide()) {
        world.playEvent(world::WorldEvents::DISPENSER_LAUNCH_SOUND, pos, 0);
    }

    // 生成烟雾粒子
    _spawnParticles(world, pos, direction);

    return stack.isEmpty() ? ItemStack() : stack;
}

// ============================================================================
// BoatDispenseBehavior
// ============================================================================

BoatDispenseBehavior::BoatDispenseBehavior(entity::BoatEntity::Type type)
    : m_boatType(type)
{}

ItemStack BoatDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack, IInventory* dispenserInventory)
{
    MC_UNUSED(dispenserInventory);
    Direction direction = state.get(BlockStateProperties::FACING());

    // 计算船的位置
    static constexpr f32 BOAT_OFFSET = 1.125f;
    f32 x = static_cast<f32>(pos.x) + 0.5f + static_cast<f32>(Directions::xOffset(direction)) * BOAT_OFFSET;
    f32 y = static_cast<f32>(pos.y) + 0.5f + static_cast<f32>(Directions::yOffset(direction)) * BOAT_OFFSET;
    f32 z = static_cast<f32>(pos.z) + 0.5f + static_cast<f32>(Directions::zOffset(direction)) * BOAT_OFFSET;

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
            return DefaultDispenseItemBehavior::dispense(world, pos, state, stack, dispenserInventory);
        }
        waterLevel = 0.0f; // 水位下方
    }

    // 创建船实体
    auto boatEntity = std::make_unique<entity::BoatEntity>(m_boatType);
    boatEntity->setTypeId(entity::EntityTypeKeys::BOAT);
    boatEntity->setPosition(x, y + waterLevel, z);

    // 添加到世界
    world.spawnEntity(std::move(boatEntity));

    // 减少物品数量
    stack.shrink(1);

    // 播放音效和粒子
    _playSound(world, pos);
    _spawnParticles(world, pos, direction);

    return stack.isEmpty() ? ItemStack() : stack;
}

// ============================================================================
// BucketDispenseBehavior
// ============================================================================

BucketDispenseBehavior::BucketDispenseBehavior(fluid::Fluid& fluid)
    : m_fluid(&fluid)
{}

ItemStack BucketDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack, IInventory* dispenserInventory)
{
    // 获取发射方向和目标位置
    Direction direction = state.get(BlockStateProperties::FACING());
    BlockPos targetPos = pos.offset(direction);

    // 尝试放置流体
    // 1. 检查目标方块是否实现 ILiquidContainer（如炼药锅），向容器注水
    const BlockState* targetState = world.getBlockState(targetPos);
    if (targetState != nullptr) {
        Block* targetBlock = Block::getBlock(targetState->blockId());
        if (targetBlock != nullptr) {
            auto* liquidContainer = dynamic_cast<ILiquidContainer*>(targetBlock);
            if (liquidContainer != nullptr &&
                liquidContainer->canContainFluid(world, targetPos, *targetState, *m_fluid)) {
                fluid::FluidState fluidState = m_fluid->defaultState();
                if (liquidContainer->receiveFluid(world, targetPos, *targetState, fluidState)) {
                    // 成功向容器注水
                    _setSuccess(true);
                    _playSound(world, pos);
                    _spawnParticles(world, pos, direction);

                    // 消耗一个满桶，尝试将空桶放回发射器库存
                    BucketItem* emptyBucket = BucketItem::getEmptyBucket();
                    ItemStack replacement = (emptyBucket != nullptr) ? emptyBucket->getDefaultInstance() : ItemStack();
                    return consumeWithRemainder(world, pos, state, stack, replacement, dispenserInventory);
                }
            }
        }
    }

    // 2. 尝试直接放置流体方块
    //    检查目标位置是否可以放置流体（空气、可替换方块、可被流体替换的方块）
    if (targetState != nullptr) {
        bool canPlace = false;

        // 空气或可替换方块
        Block* targetBlock = Block::getBlock(targetState->blockId());
        if (targetBlock == nullptr || targetBlock->isAir(*targetState)) {
            canPlace = true;
        } else if (targetState->canBeReplacedByFluid()) {
            canPlace = true;
        }

        if (canPlace) {
            // 获取流体对应的方块状态
            fluid::FluidState fluidState = m_fluid->defaultState();
            const BlockState* fluidBlockState = fluidState.getBlockState();
            if (fluidBlockState == nullptr) {
                if (m_fluid->isIn(fluid::FluidTags::WATER())) {
                    fluidBlockState = VanillaBlocks::getState(VanillaBlocks::WATER);
                } else if (m_fluid->isIn(fluid::FluidTags::LAVA())) {
                    fluidBlockState = VanillaBlocks::getState(VanillaBlocks::LAVA);
                }
            }

            if (fluidBlockState != nullptr) {
                // 在下界等维度中，水会蒸发
                if (m_fluid->isIn(fluid::FluidTags::WATER()) && world.isUltraWarm()) {
                    // 水在超热维度中蒸发，播放熄灭音效和粒子
                    world.playEvent(world::WorldEvents::FIRE_EXTINGUISH_SOUND, targetPos, 0);
                    _setSuccess(true);
                    _playSound(world, pos);
                    _spawnParticles(world, pos, direction);

                    // 消耗一个满桶，尝试将空桶放回发射器库存
                    BucketItem* emptyBucket = BucketItem::getEmptyBucket();
                    ItemStack replacement = (emptyBucket != nullptr) ? emptyBucket->getDefaultInstance() : ItemStack();
                    return consumeWithRemainder(world, pos, state, stack, replacement, dispenserInventory);
                }

                // 放置流体方块
                world.setBlockState(targetPos, fluidBlockState, 3);

                // 通过世界调度器调度流体 tick，而非直接调用
                world.tickManager().scheduleFluidTick(targetPos, *m_fluid, m_fluid->getTickDelay(world));

                _setSuccess(true);
                _playSound(world, pos);
                _spawnParticles(world, pos, direction);

                // 消耗一个满桶，尝试将空桶放回发射器库存
                BucketItem* emptyBucket = BucketItem::getEmptyBucket();
                ItemStack replacement = (emptyBucket != nullptr) ? emptyBucket->getDefaultInstance() : ItemStack();
                return consumeWithRemainder(world, pos, state, stack, replacement, dispenserInventory);
            }
        }
    }

    // 放置失败，回退到默认投掷行为
    _setSuccess(false);
    return DefaultDispenseItemBehavior::dispense(world, pos, state, stack, dispenserInventory);
}

// ============================================================================
// EmptyBucketDispenseBehavior
// ============================================================================

ItemStack EmptyBucketDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack, IInventory* dispenserInventory)
{
    // 获取发射方向和目标位置
    Direction direction = state.get(BlockStateProperties::FACING());
    BlockPos targetPos = pos.offset(direction);

    // 检查目标方块是否实现 IBucketPickupHandler
    const BlockState* targetState = world.getBlockState(targetPos);
    if (targetState != nullptr) {
        Block* targetBlock = Block::getBlock(targetState->blockId());
        if (targetBlock != nullptr) {
            auto* pickupHandler = dynamic_cast<IBucketPickupHandler*>(targetBlock);
            if (pickupHandler != nullptr) {
                // 首先尝试拾取流体（水、岩浆等）
                fluid::Fluid* pickedFluid = pickupHandler->pickupFluid(world, targetPos, *targetState);
                if (pickedFluid != nullptr) {
                    // 成功拾取流体，获取对应满桶
                    BucketItem* filledBucket = BucketItem::getFilledBucket(*pickedFluid);
                    if (filledBucket != nullptr) {
                        _setSuccess(true);
                        _playSound(world, pos);
                        _spawnParticles(world, pos, direction);

                        // 消耗一个空桶，尝试将满桶放回发射器库存
                        ItemStack replacement = filledBucket->getDefaultInstance();
                        return consumeWithRemainder(world, pos, state, stack, replacement, dispenserInventory);
                    }
                }

                // 如果流体拾取返回 nullptr，尝试非流体拾取（细雪等）
                const Item* pickedItem = pickupHandler->pickupItem(world, targetPos, *targetState);
                if (pickedItem != nullptr) {
                    // 播放拾取音效（使用方块指定的音效或默认音效）
                    const ResourceLocation* pickupSound = pickupHandler->getPickupSound(world, targetPos, *targetState);
                    if (pickupSound != nullptr) {
                        Vector3 soundPos(static_cast<f32>(targetPos.x) + 0.5f,
                            static_cast<f32>(targetPos.y) + 0.5f,
                            static_cast<f32>(targetPos.z) + 0.5f);
                        world.playSound(*pickupSound, sound::SoundCategory::Blocks, soundPos, 1.0f, 1.0f);
                    }

                    _setSuccess(true);
                    _playSound(world, pos);
                    _spawnParticles(world, pos, direction);

                    // 消耗一个空桶，尝试将拾取到的物品桶放回发射器库存
                    ItemStack replacement = pickedItem->getDefaultInstance();
                    return consumeWithRemainder(world, pos, state, stack, replacement, dispenserInventory);
                }
            }
        }
    }

    // 目标不是可拾取流体的方块，回退到默认投掷行为
    _setSuccess(false);
    return DefaultDispenseItemBehavior::dispense(world, pos, state, stack, dispenserInventory);
}

// ============================================================================
// PowderSnowBucketDispenseBehavior
// ============================================================================

ItemStack PowderSnowBucketDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack, IInventory* dispenserInventory)
{
    // 获取发射方向和目标位置
    Direction direction = state.get(BlockStateProperties::FACING());
    BlockPos targetPos = pos.offset(direction);

    // 获取 PowderSnowBucketItem 并调用 emptyContents 放置细雪
    const auto* powderSnowBucket = dynamic_cast<const item::PowderSnowBucketItem*>(stack.getItem());
    if (powderSnowBucket != nullptr && powderSnowBucket->emptyContents(nullptr, world, targetPos)) {
        // 成功放置细雪，消耗细雪桶并返回空桶
        _setSuccess(true);
        _playSound(world, pos);
        _spawnParticles(world, pos, direction);

        // 消耗一个细雪桶，尝试将空桶放回发射器库存
        ItemStack replacement;
        if (Items::BUCKET != nullptr) {
            replacement = Items::BUCKET->getDefaultInstance();
        }
        return consumeWithRemainder(world, pos, state, stack, replacement, dispenserInventory);
    }

    // 放置失败，回退到默认投掷行为
    _setSuccess(false);
    return DefaultDispenseItemBehavior::dispense(world, pos, state, stack, dispenserInventory);
}

// ============================================================================
// FlintAndSteelDispenseBehavior
// ============================================================================

ItemStack FlintAndSteelDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack, IInventory* dispenserInventory)
{
    MC_UNUSED(dispenserInventory);
    // 获取发射方向和目标位置
    Direction direction = state.get(BlockStateProperties::FACING());
    BlockPos targetPos = pos.offset(direction);

    _setSuccess(true);

    const BlockState* targetState = world.getBlockState(targetPos);
    if (targetState == nullptr) {
        _setSuccess(false);
        _playSound(world, pos);
        _spawnParticles(world, pos, direction);
        return stack;
    }

    // 情况1：可以在目标位置放置火焰
    if (item::tool::FlintAndSteelItem::canLightBlock(world, targetPos)) {
        Block* fireBlock = item::tool::FlintAndSteelItem::getFireForPlacement(world, targetPos);
        if (fireBlock != nullptr) {
            const BlockState& fireState = fireBlock->getDefaultState();
            world.setBlockState(targetPos, &fireState, 11);
        }
    }
    // 情况2：目标是可点燃的方块（有 LIT 属性且当前为 false）
    else if (targetState->hasProperty(BlockStateProperties::LIT()) && !targetState->get(BlockStateProperties::LIT())) {
        BlockState newState = targetState->with(BlockStateProperties::LIT(), true);
        world.setBlockState(targetPos, &newState, 11);
    }
    // 情况3：目标是 TNT 方块
    else if (targetState->is(VanillaBlocks::TNT)) {
        Block* tntBlock = Block::getBlock(targetState->blockId());
        if (tntBlock != nullptr) {
            auto* tnt = static_cast<TNTBlock*>(tntBlock);
            // ignite() 返回 false 表示 tntExplodes 游戏规则禁止点燃
            if (!tnt->ignite(world, targetPos, *targetState)) {
                _setSuccess(false);
            }
        }
    }
    // 无法点燃
    else {
        _setSuccess(false);
    }

    // 成功时消耗耐久
    if (isSuccess()) {
        stack.attemptDamageItem(1);
    }

    _playSound(world, pos);
    _spawnParticles(world, pos, direction);

    return stack.isEmpty() ? ItemStack() : stack;
}

// ============================================================================
// BonemealDispenseBehavior
// ============================================================================

ItemStack BonemealDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack, IInventory* dispenserInventory)
{
    MC_UNUSED(dispenserInventory);
    // 获取发射方向和目标位置
    Direction direction = state.get(BlockStateProperties::FACING());
    BlockPos targetPos = pos.offset(direction);

    _setSuccess(true);

    // 先尝试对 IGrowable 方块使用骨粉
    bool applied = item::items::BoneMealItem::applyBonemeal(stack, world, targetPos, nullptr);

    // 如果 IGrowable 路径失败，尝试在水中生成海草
    if (!applied) {
        const fluid::FluidState* fluidState = world.getFluidState(targetPos);
        if (fluidState != nullptr && !fluidState->isEmpty() && fluidState->getFluid().isIn(fluid::FluidTags::WATER()) &&
            fluidState->isSource()) {
            const u64 seed = world.seed() ^ static_cast<u64>(std::hash<BlockPos>{}(targetPos));
            math::Random random(seed);
            applied = item::items::BoneMealItem::growSeagrass(world, targetPos, random);
            if (applied) {
                // growSeagrass 不消耗物品，需要手动消耗
                stack.shrink(1);
            }
        }
    }

    if (!applied) {
        _setSuccess(false);
    } else {
        // 播放植物生长效果（粒子 + 音效）
        if (!world.isClientSide()) {
            world.playEvent(world::WorldEvents::PLANT_GROWTH_EFFECT, targetPos, 15);
        }
    }

    _playSound(world, pos);
    _spawnParticles(world, pos, direction);

    return stack.isEmpty() ? ItemStack() : stack;
}

// ============================================================================
// TNTDispenseBehavior
// ============================================================================

ItemStack TNTDispenseBehavior::dispense(
    IWorld& world, const BlockPos& pos, const BlockState& state, ItemStack& stack, IInventory* dispenserInventory)
{
    MC_UNUSED(dispenserInventory);

    // 对应 MC Java 的 DispenseItemBehavior 中 Blocks.TNT 的发射行为
    // 如果 tntExplodes 游戏规则为 false，发射失败（物品不被消耗）
    if (!world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES)) {
        _setSuccess(false);
        return stack;
    }

    Direction direction = state.get(BlockStateProperties::FACING());
    BlockPos targetPos = pos.offset(direction);

    // 生成点燃的 TNT 实体
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* tntType = registry.getType(entity::EntityTypeKeys::TNT);

    if (tntType != nullptr && tntType->isValid()) {
        auto tntEntity = tntType->create(&world);
        if (tntEntity != nullptr) {
            // 设置 TNT 位置：在发射器前方偏移
            static constexpr f32 TNT_OFFSET = 1.125f;
            f32 x = static_cast<f32>(pos.x) + 0.5f + static_cast<f32>(Directions::xOffset(direction)) * TNT_OFFSET;
            f32 y = static_cast<f32>(pos.y) + 0.5f + static_cast<f32>(Directions::yOffset(direction)) * TNT_OFFSET;
            f32 z = static_cast<f32>(pos.z) + 0.5f + static_cast<f32>(Directions::zOffset(direction)) * TNT_OFFSET;

            auto* tnt = dynamic_cast<entity::TNTEntity*>(tntEntity.get());
            if (tnt != nullptr) {
                tnt->setPosition(x, y, z);

                // 设置发射方向的速度
                f32 vx = static_cast<f32>(Directions::xOffset(direction)) * 0.5f;
                f32 vy = static_cast<f32>(Directions::yOffset(direction)) * 0.5f + 0.1f;
                f32 vz = static_cast<f32>(Directions::zOffset(direction)) * 0.5f;
                tnt->setVelocity(Vector3(vx, vy, vz));

                // 点燃 TNT
                tnt->ignite();
            }

            world.spawnEntity(std::move(tntEntity));
        }
    }

    // 播放 TNT 引燃音效
    world.playSound(SoundEvents::ENTITY_TNT_PRIMED,
        sound::SoundCategory::Blocks,
        Vector3(
            static_cast<f32>(targetPos.x) + 0.5f, static_cast<f32>(targetPos.y), static_cast<f32>(targetPos.z) + 0.5f),
        1.0f,
        1.0f);

    // 消耗一个 TNT 物品
    stack.shrink(1);
    _setSuccess(true);

    _playSound(world, pos);
    _spawnParticles(world, pos, direction);

    return stack.isEmpty() ? ItemStack() : stack;
}

} // namespace blocks
} // namespace mc
