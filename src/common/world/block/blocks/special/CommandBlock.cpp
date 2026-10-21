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
#include "common/world/gamerule/GameRules.hpp"
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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::FACING(), Direction::North)
            .with(BlockStateProperties::CONDITIONAL(), false));
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

    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity == nullptr || entity->getType() != BlockEntityType::CommandBlock) {
        return;
    }

    // 对齐 vanilla CommandBlock.neighborChanged → setPoweredAndUpdate：
    // 仅供电状态变化时处理；上升沿时自动执行（循环）与连锁模式不调度（前者由自身方块刻自驱，
    // 后者只由 executeChain 驱动），其余模式刷新条件并在 1 tick 后执行。
    setPoweredAndUpdate(world, pos, *static_cast<blockentity::CommandBlockEntity*>(entity));
}

void CommandBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* placer, const ItemStack& stack)
{
    MC_UNUSED(state);
    MC_UNUSED(placer);

    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity == nullptr || entity->getType() != BlockEntityType::CommandBlock) {
        return;
    }
    auto& commandEntity = *static_cast<blockentity::CommandBlockEntity*>(entity);

    // 对齐 vanilla CommandBlock.setPlacedBy：物品未携带 block_entity_data 时，用游戏规则与方块类型
    // 自身的自动执行标志初始化实体。
    // TODO(command-block-item-data): 项目 ItemStack 尚未暴露 block_entity_data 组件查询，
    //   且放置流程先于 onBlockPlacedBy 应用方块实体 NBT；此处暂按"无组件"处理，
    //   待组件 API 补齐后改为读取组件存在性，避免覆盖物品携带的 auto / TrackOutput。
    commandEntity.setTrackOutput(world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::SEND_COMMAND_FEEDBACK));
    commandEntity.reapplyAutomatic();

    // 放置时若已被邻居供电，按供电上升沿处理（与 neighborChanged 一致）。
    commandEntity.setPowered(false);
    setPoweredAndUpdate(world, pos, commandEntity);
}

void CommandBlock::setPoweredAndUpdate(IWorld& world, const BlockPos& pos, blockentity::CommandBlockEntity& entity)
{
    // 对齐 vanilla CommandBlock.setPoweredAndUpdate（CommandBlock.java:113-125）。
    const bool powered = world::redstone::RedstonePower::isPowered(world, pos);
    const bool wasPowered = entity.isPowered();
    if (powered == wasPowered) {
        return;
    }
    entity.setPowered(powered);
    if (!powered) {
        return;
    }
    if (entity.isAuto() || entity.getMode() == blockentity::CommandBlockMode::Sequence) {
        return;
    }
    entity.markConditionMet(world);
    world.tickManager().scheduleBlockTick(pos, *this, 1, world::tick::TickPriority::High);
}

void CommandBlock::onUpdated(
    IWorld& world, const BlockPos& pos, const BlockState& state, blockentity::CommandBlockEntity& entity)
{
    MC_UNUSED(state);
    // 对齐 vanilla CommandBlock.onUpdated（CommandBlock.java:onUpdated）：
    // 重新按方块类型的自动执行标志设置实体，再按"循环模式或自动执行"决定是否重新调度方块刻。
    entity.setAuto(defaultAutomatic());
    if (entity.getMode() == blockentity::CommandBlockMode::Auto || entity.isAuto()) {
        entity.markConditionMet(world);
        world.tickManager().scheduleBlockTick(pos, *this, 1, world::tick::TickPriority::High);
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

    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity == nullptr || entity->getType() != BlockEntityType::CommandBlock) {
        return;
    }
    auto& commandEntity = *static_cast<blockentity::CommandBlockEntity*>(entity);

    // 对齐 vanilla CommandBlock.tick（CommandBlock.java:127-153）：
    //   循环模式：先刷新条件，再以"调度前的条件结果"决定是否执行；执行后若仍被供电/自动则继续调度下一刻。
    //   脉冲模式：以调度前的条件结果决定是否执行。
    //   连锁模式：由 executeChain 直接驱动，不单独调度方块刻。
    const blockentity::CommandBlockMode mode = commandEntity.getMode();
    const bool conditionMet = commandEntity.markConditionMet(world);

    auto executeOrReset = [&]() {
        if (!conditionMet) {
            // 条件不满足：条件方块清空成功计数（对齐 vanilla execute 的 setSuccessCount(0) 分支）
            commandEntity.setSuccessCount(0);
            return;
        }
        execute(world, pos, state, &commandEntity);
    };

    if (mode == blockentity::CommandBlockMode::Auto) {
        // 循环方块只在被供电或开启自动执行时运行；两者都不满足时既空闲也不续期。
        const bool active = commandEntity.isPowered() || commandEntity.isAuto();
        if (!active) {
            // 空闲：仅条件方块需要清空成功计数，避免比较器残留信号。
            if (commandEntity.isConditional()) {
                commandEntity.setSuccessCount(0);
            }
        } else {
            if (conditionMet) {
                execute(world, pos, state, &commandEntity);
            } else if (commandEntity.isConditional()) {
                commandEntity.setSuccessCount(0);
            }
            world.tickManager().scheduleBlockTick(pos, *this, 1, world::tick::TickPriority::High);
        }
    } else if (mode == blockentity::CommandBlockMode::Redstone) {
        executeOrReset();
    }

    // 通知比较器更新信号（成功计数变化；无论何种模式，vanilla 都会更新输出信号）
    world::redstone::RedstoneSystem::instance().updateComparators(world, pos);
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

BlockActionResult CommandBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(hit);

    // 对齐 vanilla CommandBlock.useWithoutItem：命令方块被 commandBlocksWork 游戏规则禁用时直接 PASS；
    // 存在命令方块实体且玩家拥有 GameMaster 权限时打开命令方块界面并返回 SUCCESS，否则返回 PASS。
    if (!world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::COMMAND_BLOCKS_WORK)) {
        return ActionResultType::Pass;
    }
    const BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::CommandBlock && player.canUseGameMasterBlocks()) {
        player.openCommandBlock(pos);
        return ActionResultType::Success;
    }
    return ActionResultType::Pass;
}

std::unique_ptr<BlockEntity> CommandBlock::createBlockEntity(const BlockPos& pos)
{
    auto entity = std::make_unique<blockentity::CommandBlockEntity>(pos, blockentity::CommandBlockMode::Redstone);
    // 对齐 vanilla CommandBlock.newBlockEntity：创建实体时按方块类型设定自动执行标志。
    entity->setAuto(defaultAutomatic());
    return entity;
}

void CommandBlock::execute(
    IWorld& world, const BlockPos& pos, const BlockState& state, blockentity::CommandBlockEntity* commandEntity)
{
    if (commandEntity == nullptr) {
        return;
    }

    // 对齐 vanilla CommandBlock.execute：条件结果由调用方（tick/executeChain）先行刷新，
    // 此处只负责执行命令（空命令视为失败但继续链）并在执行后驱动连锁。
    if (!commandEntity->getCommand().empty()) {
        commandEntity->trigger(world);
    } else {
        commandEntity->setSuccessCount(0);
    }

    executeChain(world, pos, state.get(BlockStateProperties::FACING()));
}

// executeChain 保持成员函数形态：与 Block 家族其它 tick 侧辅助方法一致，便于后续按方块类型覆写。
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void CommandBlock::executeChain(IWorld& world, const BlockPos& pos, Direction facing)
{
    // 对齐 vanilla CommandBlock.executeChain：沿 FACING 逐格查找连锁命令方块，
    // 每格以自身 FACING 决定下一步方向；条件不满足时清空成功计数并继续，命令失败时中断整条链。
    BlockPos currentPos = pos;
    Direction currentFacing = facing;

    // 链长度上限由 maxCommandChainLength 游戏规则决定（默认 65536）。
    i32 remaining = world.getGameRules().getInt(world::gamerule::GameRuleKeys::MAX_COMMAND_CHAIN_LENGTH);
    while (remaining-- > 0) {
        currentPos = currentPos.offset(currentFacing);

        const BlockState* nextState = world.getBlockState(currentPos);
        if (nextState == nullptr) {
            break;
        }
        const auto* chainBlock = dynamic_cast<const ChainCommandBlock*>(&nextState->getBlock());
        if (chainBlock == nullptr) {
            break;
        }

        BlockEntity* entity = world.getBlockEntity(currentPos);
        if (entity == nullptr || entity->getType() != BlockEntityType::CommandBlock) {
            break;
        }
        auto& chainEntity = *static_cast<blockentity::CommandBlockEntity*>(entity);
        if (chainEntity.getMode() != blockentity::CommandBlockMode::Sequence) {
            break;
        }
        // 未被供电且非自动执行的连锁方块不参与执行（对齐 vanilla isPowered() || isAutomatic()）。
        if (!chainEntity.isPowered() && !chainEntity.isAuto()) {
            break;
        }

        const Direction blockFacing = nextState->get(BlockStateProperties::FACING());
        if (chainEntity.markConditionMet(world)) {
            if (!chainEntity.getCommand().empty() && !chainEntity.trigger(world)) {
                // 命令执行失败：中断整条链（vanilla performCommand 返回 false 时 break）。
                break;
            }
            world::redstone::RedstoneSystem::instance().updateComparators(world, currentPos);
        } else if (chainEntity.isConditional()) {
            chainEntity.setSuccessCount(0);
        }

        currentFacing = blockFacing;
    }
}

} // namespace blocks
} // namespace mc
