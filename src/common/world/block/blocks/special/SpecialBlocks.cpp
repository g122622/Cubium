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

#include "SpecialBlocks.hpp"
#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../physics/PhysicsConstants.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/Vector3.hpp"
#include "../../../IWorld.hpp"
#include "../../../WorldEvents.hpp"
#include "../../../block/IBucketPickupHandler.hpp"
#include "../../../block/blocks/LiquidBlock.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../blockentity/redstone/CommandBlockEntity.hpp"
#include "../../../dimension/DimensionType.hpp"
#include "../../../fluid/FluidTags.hpp"
#include "../../../gen/jigsaw/JigsawOrientation.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <utility>

namespace mc {
namespace blocks {

// BlockPos 哈希别名
using BlockPosHash = std::hash<BlockPos>;

// ========== BarrierBlock ==========

BarrierBlock::BarrierBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 屏障没有状态属性
}

const CollisionShape& BarrierBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

// ========== StructureVoidBlock ==========

StructureVoidBlock::StructureVoidBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 结构空位没有状态属性
}

const CollisionShape& StructureVoidBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 结构空位有一个小的可见轮廓
    static CollisionShape shape = CollisionShape::box(0.375f, 0.375f, 0.375f, 0.625f, 0.625f, 0.625f);
    return shape;
}

const CollisionShape& StructureVoidBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== StructureBlock ==========

StructureBlock::StructureBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器，添加 MODE 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::STRUCTURE_MODE())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态：DATA 模式
    setDefaultState(defaultState().with(BlockStateProperties::STRUCTURE_MODE(), Mode::Data));
}

StructureBlock::Mode StructureBlock::getMode(const BlockState& state) const
{
    return state.get(BlockStateProperties::STRUCTURE_MODE());
}

BlockState StructureBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 放置时默认为 DATA 模式
    MC_UNUSED(context);
    return defaultState();
}

BlockActionResult StructureBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 游戏管理员方块需要权限才能交互
    if (!player.canUseGameMasterBlocks()) {
        return ActionResultType::Fail;
    }

    // TODO: 打开结构方块界面
    return ActionResultType::Success;
}

// ========== JigsawBlock ==========

JigsawBlock::JigsawBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器，添加 ORIENTATION 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::ORIENTATION())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态：NorthUp
    setDefaultState(
        defaultState().with(BlockStateProperties::ORIENTATION(), world::gen::jigsaw::JigsawOrientation::NorthUp));
}

const BlockState& JigsawBlock::rotate(const BlockState& state, Rotation rotation) const
{
    world::gen::jigsaw::JigsawOrientation orientation = state.get(BlockStateProperties::ORIENTATION());
    world::gen::jigsaw::JigsawOrientation newOrientation =
        world::gen::jigsaw::JigsawOrientations::rotate(orientation, rotation);
    return state.with(BlockStateProperties::ORIENTATION(), newOrientation);
}

const BlockState& JigsawBlock::mirror(const BlockState& state, Mirror mirror) const
{
    world::gen::jigsaw::JigsawOrientation orientation = state.get(BlockStateProperties::ORIENTATION());
    world::gen::jigsaw::JigsawOrientation newOrientation =
        world::gen::jigsaw::JigsawOrientations::mirror(orientation, mirror);
    return state.with(BlockStateProperties::ORIENTATION(), newOrientation);
}

BlockActionResult JigsawBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 游戏管理员方块需要权限才能交互
    if (!player.canUseGameMasterBlocks()) {
        return ActionResultType::Fail;
    }

    // TODO: 打开拼图方块界面
    return ActionResultType::Success;
}

// ========== CommandBlock ==========

CommandBlock::CommandBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
            .add(BlockStateProperties::CONDITIONAL())
            .add(BlockStateProperties::POWERED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::FACING(), Direction::North)
            .with(BlockStateProperties::CONDITIONAL(), false)
            .with(BlockStateProperties::POWERED(), false));
}

Direction CommandBlock::getFacing(const BlockState& state) const
{
    return state.get(BlockStateProperties::FACING());
}

bool CommandBlock::isConditional(const BlockState& state) const
{
    return state.get(BlockStateProperties::CONDITIONAL());
}

bool CommandBlock::isPowered(const BlockState& state) const
{
    return state.get(BlockStateProperties::POWERED());
}

BlockState CommandBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction facing = Directions::opposite(context.getClickedFace());
    return defaultState().with(BlockStateProperties::FACING(), facing);
}

void CommandBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 客户端不处理红石逻辑
    if (world.isClientSide()) {
        return;
    }

    // 获取方块实体
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity == nullptr || entity->getType() != BlockEntityType::CommandBlock) {
        return;
    }

    auto* commandEntity = static_cast<blockentity::CommandBlockEntity*>(entity);

    // 检测红石信号
    bool isPowered = world::redstone::RedstonePower::isPowered(world, pos);
    bool wasPowered = commandEntity->isPowered();

    // 更新供电状态
    commandEntity->setPowered(isPowered);

    // 获取命令方块模式
    blockentity::CommandBlockMode mode = commandEntity->getMode();

    // 只处理脉冲模式（REDSTONE）的红石上升沿触发
    // 循环模式（AUTO）和连锁模式（SEQUENCE）不通过红石直接触发
    if (mode == blockentity::CommandBlockMode::Redstone) {
        // 上升沿触发：从不供电变为供电
        if (isPowered && !wasPowered) {
            // 检查条件模式
            commandEntity->checkCondition(
                world, getFacing(*world.getBlockState(pos)), isConditional(*world.getBlockState(pos)));

            // 延迟 1 tick 后执行
            world.tickManager().scheduleBlockTick(pos, *this, 1, world::tick::TickPriority::High);
        }
    }

    // 循环模式：如果被供电或设置为自动执行，调度 tick
    if (mode == blockentity::CommandBlockMode::Auto) {
        if ((isPowered || commandEntity->isAuto()) && !world.tickManager().isBlockTickScheduled(pos, *this)) {
            world.tickManager().scheduleBlockTick(pos, *this, 1, world::tick::TickPriority::High);
        }
    }
}

i32 CommandBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 命令方块不输出信号
    return 0;
}

i32 CommandBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::CommandBlock) {
        auto* commandEntity = static_cast<blockentity::CommandBlockEntity*>(entity);
        return std::min(commandEntity->getSuccessCount(), 15);
    }
    return 0;
}

void CommandBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 获取方块实体
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity == nullptr || entity->getType() != BlockEntityType::CommandBlock) {
        return;
    }

    auto* commandEntity = static_cast<blockentity::CommandBlockEntity*>(entity);

    // 脉冲模式（REDSTONE）：由红石信号上升沿触发的 tick 执行
    if (commandEntity->getMode() == blockentity::CommandBlockMode::Redstone) {
        // 检查条件
        bool conditional = isConditional(state);
        if (!commandEntity->checkCondition(world, getFacing(state), conditional)) {
            commandEntity->setSuccessCount(0);
        } else {
            // 执行命令
            execute(world, pos, state, commandEntity);
        }

        // 无条件通知比较器更新信号（无论条件是否满足，成功计数可能已变化）
        world::redstone::RedstoneSystem::instance().updateComparators(world, pos);
    }
}

const BlockState& CommandBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

const BlockState& CommandBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

BlockActionResult CommandBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 游戏管理员方块需要权限才能交互
    if (!player.canUseGameMasterBlocks()) {
        return ActionResultType::Fail;
    }

    // TODO: 打开命令方块界面
    return ActionResultType::Success;
}

std::unique_ptr<BlockEntity> CommandBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::CommandBlockEntity>(pos, blockentity::CommandBlockMode::Redstone);
}

void CommandBlock::execute(
    IWorld& world, const BlockPos& pos, const BlockState& state, blockentity::CommandBlockEntity* commandEntity)
{
    if (commandEntity == nullptr) {
        return;
    }

    // 检查条件
    bool conditional = isConditional(state);
    if (!commandEntity->checkCondition(world, getFacing(state), conditional)) {
        // 条件不满足时重置成功计数
        commandEntity->setSuccessCount(0);
        return;
    }

    // 执行命令
    if (!commandEntity->getCommand().empty()) {
        commandEntity->trigger(world);
    }

    // 触发连锁命令方块
    executeChain(world, pos, getFacing(state));
}

void CommandBlock::executeChain(IWorld& world, const BlockPos& pos, Direction facing)
{
    // 沿着 FACING 方向查找并触发连锁命令方块

    // 最大链长度限制
    constexpr i32 MAX_CHAIN_LENGTH = 65536;

    BlockPos currentPos = pos;
    Direction currentFacing = facing;

    for (i32 i = 0; i < MAX_CHAIN_LENGTH; ++i) {
        // 移动到下一个位置
        currentPos = currentPos.offset(currentFacing);

        // 获取方块状态
        const BlockState* nextState = world.getBlockState(currentPos);
        if (nextState == nullptr) {
            break;
        }

        // 检查是否为连锁命令方块（通过检查是否有 ChainCommandBlock 类型的方法）
        const Block& nextBlock = nextState->getBlock();
        // 检查是否是 ChainCommandBlock（通过 dynamic_cast）
        const ChainCommandBlock* chainBlock = dynamic_cast<const ChainCommandBlock*>(&nextBlock);
        if (chainBlock == nullptr) {
            break;
        }

        // 获取方块实体
        BlockEntity* entity = world.getBlockEntity(currentPos);
        if (entity == nullptr || entity->getType() != BlockEntityType::CommandBlock) {
            break;
        }

        auto* chainEntity = static_cast<blockentity::CommandBlockEntity*>(entity);

        // 连锁模式必须是 SEQUENCE
        if (chainEntity->getMode() != blockentity::CommandBlockMode::Sequence) {
            break;
        }

        // 检查是否被供电或设置为自动执行
        if (!chainEntity->isPowered() && !chainEntity->isAuto()) {
            break;
        }

        // 检查条件
        Direction blockFacing = chainBlock->getFacing(*nextState);
        bool conditional = chainBlock->isConditional(*nextState);
        if (!chainEntity->checkCondition(world, blockFacing, conditional)) {
            // 条件不满足，继续链但不执行
            chainEntity->setSuccessCount(0);
            currentFacing = blockFacing;
            continue;
        }

        // 执行命令
        if (!chainEntity->getCommand().empty()) {
            if (!chainEntity->trigger(world)) {
                // 执行失败，停止链
                break;
            }
        }

        // 通知周围比较器更新信号（连锁命令方块的成功计数可作为比较器输入）
        world::redstone::RedstoneSystem::instance().updateComparators(world, currentPos);

        // 继续链
        currentFacing = blockFacing;
    }
}

// ========== RepeatingCommandBlock ==========

RepeatingCommandBlock::RepeatingCommandBlock(const BlockProperties& properties)
    : CommandBlock(properties)
{}

void RepeatingCommandBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 获取方块实体
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity == nullptr || entity->getType() != BlockEntityType::CommandBlock) {
        return;
    }

    auto* commandEntity = static_cast<blockentity::CommandBlockEntity*>(entity);

    // 循环模式（AUTO）：每 tick 执行
    if (commandEntity->getMode() == blockentity::CommandBlockMode::Auto) {
        // 检查条件
        bool conditional = isConditional(state);
        if (!commandEntity->checkCondition(world, getFacing(state), conditional)) {
            commandEntity->setSuccessCount(0);
        } else {
            // 执行命令
            execute(world, pos, state, commandEntity);
        }

        // 无条件通知比较器更新信号
        world::redstone::RedstoneSystem::instance().updateComparators(world, pos);

        // 如果仍然被供电或自动执行，重新调度下一 tick
        if (commandEntity->isPowered() || commandEntity->isAuto()) {
            world.tickManager().scheduleBlockTick(pos, *this, 1, world::tick::TickPriority::High);
        }
    }
}

std::unique_ptr<BlockEntity> RepeatingCommandBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::CommandBlockEntity>(pos, blockentity::CommandBlockMode::Auto);
}

// ========== ChainCommandBlock ==========

ChainCommandBlock::ChainCommandBlock(const BlockProperties& properties)
    : CommandBlock(properties)
{}

std::unique_ptr<BlockEntity> ChainCommandBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::CommandBlockEntity>(pos, blockentity::CommandBlockMode::Sequence);
}

// ========== SlimeBlock ==========

SlimeBlock::SlimeBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 史莱姆块滑度为 0.8
    m_slipperiness = physics::SLIPPERINESS_SLIME;
}

void SlimeBlock::onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 如果实体向下落，反弹
    // 反弹系数：LivingEntity 使用 1.0，其他实体使用 0.8
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    if (velocity.y < 0.0f) {
        // 反弹：Y速度取反并乘以弹跳系数
        // 使用非生物实体的弹跳系数（保守值），LivingEntity 会单独处理
        entity.setVelocity(velocity.x, -velocity.y * physics::SLIME_BLOCK_BOUNCE_FACTOR_NON_LIVING, velocity.z);
    } else {
        // 向上或静止时，Y速度归零
        entity.setVelocity(velocity.x, 0.0f, velocity.z);
    }
}

void SlimeBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 史莱姆块会减缓实体的Y轴速度（类似于蜘蛛网的效果，但更温和）
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 这个效果主要用于实体在史莱姆块内部时减速
    // 实际弹跳在 onLanded 中处理
}

Material::PushReaction SlimeBlock::getPushReaction(const BlockState& state) const
{
    MC_UNUSED(state);
    return Material::PushReaction::Normal;
}

bool SlimeBlock::isStickyBlock(const BlockState& state) const noexcept
{
    MC_UNUSED(state);
    return true;
}

bool SlimeBlock::canStickTo(const BlockState& state, const BlockState& other) const noexcept
{
    MC_UNUSED(state);
    // 史莱姆块可以粘住史莱姆块和蜂蜜块
    const Block& otherBlock = other.getBlock();
    return otherBlock.isStickyBlock(other);
}

// ========== HoneyBlock ==========

HoneyBlock::HoneyBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 蜂蜜块滑度为默认值 0.6（MC 中蜂蜜块不修改 friction）
    // 蜂蜜块的减速效果通过 speedFactor=0.4 和 jumpFactor=0.5 实现
    m_slipperiness = physics::SLIPPERINESS_HONEY;
    m_speedFactor = physics::HONEY_BLOCK_SPEED_FACTOR;
    m_jumpFactor = physics::HONEY_BLOCK_JUMP_FACTOR;

    // 蜂蜜块碰撞箱稍小
    m_collisionShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.9375f, 1.0f);
}

void HoneyBlock::onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 蜂蜜块消除摔落伤害，但不弹跳
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    // Y速度归零，但不反弹
    entity.setVelocity(velocity.x, 0.0f, velocity.z);
    // 重置摔落距离（消除摔落伤害）
    entity.setFallDistance(0.0f);
}

void HoneyBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 蜂蜜块减缓实体速度
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    // 水平速度减少 40%（乘以 0.4）
    // 垂直下落速度减少（下落时每 tick 减速）
    entity.setVelocity(velocity.x * 0.4f, velocity.y * 0.9f, velocity.z * 0.4f);
}

Material::PushReaction HoneyBlock::getPushReaction(const BlockState& state) const
{
    MC_UNUSED(state);
    return Material::PushReaction::Normal;
}

bool HoneyBlock::isStickyBlock(const BlockState& state) const noexcept
{
    MC_UNUSED(state);
    return true;
}

bool HoneyBlock::canStickTo(const BlockState& state, const BlockState& other) const noexcept
{
    MC_UNUSED(state);
    // 蜂蜜块只能粘住蜂蜜块（不能粘住史莱姆块）
    // 如果两个都是蜂蜜块，则可以粘连
    // 检查 other 方块是否是蜂蜜块
    return other.is(VanillaBlocks::HONEY_BLOCK);
}

const CollisionShape& HoneyBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_collisionShape;
}

// ========== SpongeBlock ==========

SpongeBlock::SpongeBlock(const BlockProperties& properties)
    : Block(properties)
{}

bool SpongeBlock::tryAbsorbWater(IWorld& world, const BlockPos& pos)
{
    i32 absorbedCount = absorb(world, pos);
    if (absorbedCount > 0) {
        // 将海绵变为湿润海绵
        const BlockState& wetSpongeState = VanillaBlocks::WET_SPONGE->defaultState();
        world.setBlockState(pos, &wetSpongeState, 3);

        // 播放水被吸收的视觉效果（事件 2001，data 为水的方块状态 ID）
        const BlockState& waterState = VanillaBlocks::WATER->defaultState();
        world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, pos, waterState.stateId());

        return true;
    }
    return false;
}

void SpongeBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    // 放置时尝试吸水
    tryAbsorbWater(world, pos);
}

void SpongeBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 邻居更新时尝试吸水
    tryAbsorbWater(world, pos);

    // 调用基类方法
    Block::neighborChanged(world, pos, neighborBlock, neighborPos, isMoving);
}

i32 SpongeBlock::absorb(IWorld& world, const BlockPos& pos)
{
    // 使用 BFS 搜索周围的水方块

    // 队列元素：(位置, 深度)
    std::queue<std::pair<BlockPos, i32>> queue;
    queue.push({pos, 0});

    i32 absorbedCount = 0;

    // 已访问的位置集合（用于避免重复处理）
    std::unordered_set<BlockPos, BlockPosHash> visited;
    visited.insert(pos);

    while (!queue.empty()) {
        auto [currentPos, depth] = queue.front();
        queue.pop();

        // 遍历六个方向
        for (Direction dir : Directions::all()) {
            BlockPos neighborPos = currentPos.offset(dir);

            // 获取流体状态
            const fluid::FluidState* fluidState = world.getFluidState(neighborPos);
            if (fluidState == nullptr || fluidState->isEmpty()) {
                continue;
            }

            // 检查是否为水
            if (!fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
                continue;
            }

            // 获取方块状态
            const BlockState* blockState = world.getBlockState(neighborPos);
            if (blockState == nullptr) {
                continue;
            }

            Block& block = blockState->getBlockMutable();

            // 情况1：可舀取的水源（如水源方块）
            IBucketPickupHandler* bucketPickup = dynamic_cast<IBucketPickupHandler*>(&block);
            if (bucketPickup != nullptr) {
                fluid::Fluid* pickedFluid = bucketPickup->pickupFluid(world, neighborPos, *blockState);
                if (pickedFluid != nullptr) {
                    ++absorbedCount;
                    if (depth < MAX_ABSORB_DEPTH && visited.find(neighborPos) == visited.end()) {
                        visited.insert(neighborPos);
                        queue.push({neighborPos, depth + 1});
                    }
                }
            }
            // 情况2：流动水方块
            else if (dynamic_cast<block::LiquidBlock*>(&block) != nullptr) {
                // 移除流动水方块，设置为空气
                const BlockState& airState = VanillaBlocks::AIR->defaultState();
                world.setBlockState(neighborPos, &airState, 3);
                ++absorbedCount;
                if (depth < MAX_ABSORB_DEPTH && visited.find(neighborPos) == visited.end()) {
                    visited.insert(neighborPos);
                    queue.push({neighborPos, depth + 1});
                }
            }
            // 情况3：海洋植物（海带、海带茎、海草、高海草）
            // 不要按 Material 判断，以避免误匹配海泡菜等其他 OCEAN_PLANT 方块。
            else if (blockState->is(VanillaBlocks::KELP) || blockState->is(VanillaBlocks::KELP_PLANT) ||
                blockState->is(VanillaBlocks::SEAGRASS) || blockState->is(VanillaBlocks::TALL_SEAGRASS)) {
                // 在移除方块之前生成掉落物品
                Block::dropResources(world, neighborPos, *blockState);

                const BlockState& airState = VanillaBlocks::AIR->defaultState();
                world.setBlockState(neighborPos, &airState, 3);
                ++absorbedCount;
                if (depth < MAX_ABSORB_DEPTH && visited.find(neighborPos) == visited.end()) {
                    visited.insert(neighborPos);
                    queue.push({neighborPos, depth + 1});
                }
            }

            // 超过最大吸收数量就停止
            if (absorbedCount >= MAX_ABSORB_COUNT) {
                return absorbedCount;
            }
        }
    }

    return absorbedCount;
}

// ========== WetSpongeBlock ==========

WetSpongeBlock::WetSpongeBlock(const BlockProperties& properties)
    : Block(properties)
{}

void WetSpongeBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 在下界（超热维度）放置时变干
    if (world.isUltraWarm()) {
        // 变为干海绵
        const BlockState& spongeState = VanillaBlocks::SPONGE->defaultState();
        world.setBlockState(pos, &spongeState, 3);

        // 播放蒸汽效果（事件 2009）
        world.playEvent(world::WorldEvents::WET_SPONGE_DRY, pos, 0);

        // 播放火焰熄灭音效
        world.playSound(ResourceLocation("minecraft", "block.fire.extinguish"),
            sound::SoundCategory::Blocks,
            pos.center(),
            1.0f,
            1.0f);
    }
}

// ========== WebBlock ==========

WebBlock::WebBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 蜘蛛网形状：完整方块，透明
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

const CollisionShape& WebBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

void WebBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 蜘蛛网大幅减缓实体速度
    // 实际效果是水平速度 * 0.025，垂直下落 * 0.05
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    entity.setVelocity(velocity.x * physics::COBWEB_SLOWDOWN_XZ,
        velocity.y < 0.0f ? velocity.y * physics::COBWEB_SLOWDOWN_Y : velocity.y, // 只减速下落
        velocity.z * physics::COBWEB_SLOWDOWN_XZ);
}

} // namespace blocks
} // namespace mc
