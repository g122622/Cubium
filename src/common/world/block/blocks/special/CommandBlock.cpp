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

#include "CommandBlock.hpp"

#include "ChainCommandBlock.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/redstone/CommandBlockEntity.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== CommandBlock ==========

CommandBlock::CommandBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    // vanilla 1.21.11 命令方块仅有 conditional + facing，不含 powered（powered 由 BE 持有）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
            .add(BlockStateProperties::CONDITIONAL())
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
            .with(BlockStateProperties::CONDITIONAL(), false));
}

Direction CommandBlock::getFacing(const BlockState& state) const
{
    return state.get(BlockStateProperties::FACING());
}

bool CommandBlock::isConditional(const BlockState& state) const
{
    return state.get(BlockStateProperties::CONDITIONAL());
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

} // namespace blocks
} // namespace mc
