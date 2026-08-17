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

#include "PistonBlock.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/interactive/PistonBlockEntity.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "MovingPistonBlock.hpp"
#include "PistonHeadBlock.hpp"
#include "PistonStructureHelper.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

PistonBlock::PistonBlock(const BlockProperties& properties, bool sticky)
    : Block(properties)
    , m_sticky(sticky)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
            .add(BlockStateProperties::EXTENDED())
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
            .with(BlockStateProperties::EXTENDED(), false));
}

BlockState PistonBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 朝向玩家视线最近方向的反方向（活塞头朝向玩家）。与 MC 1.21.11 PistonBaseBlock.getStateForPlacement
    // 对齐：facing = getNearestLookingDirection().getOpposite()，extended = false（放置时未伸出）。
    // getNearestLookingDirection = orderedByNearest(yaw, pitch)[0]，六向含 Up/Down（由俯仰决定）。
    // 此前未重写该方法，落回基类 Block::getStateForPlacement 返回 defaultState()（FACING 恒 North），
    // 与 vanilla 严重分歧（vanilla 由玩家视线决定）。修复后与 vanilla 严格对齐。
    // 粘性活塞与普通活塞共用本类（m_sticky 区分），继承本方法自动获得正确朝向。
    Direction facing = Directions::opposite(context.getNearestLookingDirection());
    return defaultState().with(BlockStateProperties::FACING(), facing).with(BlockStateProperties::EXTENDED(), false);
}

bool PistonBlock::isExtended(const BlockState& state)
{
    return state.get(BlockStateProperties::EXTENDED());
}

const BlockState& PistonBlock::withExtended(const BlockState& state, bool extended)
{
    return state.with(BlockStateProperties::EXTENDED(), extended);
}

Direction PistonBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::FACING());
}

void PistonBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 放置时检查是否需要伸出
    _checkForMove(world, pos, state);
}

void PistonBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 邻居变化时检查是否需要改变状态
    const BlockState* state = world.getBlockState(pos);
    if (state) {
        _checkForMove(world, pos, *state);
    }
}

void PistonBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    // 活塞动画由 PistonBlockEntity 处理
}

BlockState PistonBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    return state;
}

bool PistonBlock::shouldBeExtended(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    Direction facing = getFacing(state);

    // 检查活塞本体除前面外5个方向是否被充能
    // 注意：不检查活塞朝向方向（前面）的信号
    for (Direction dir : Directions::all()) {
        if (dir == facing) {
            // 不检查活塞朝向方向（前面）
            continue;
        }

        BlockPos neighborPos = pos.offset(dir);
        // 检查相邻方块在该方向是否被充能（从该方向接收强信号）
        if (world::redstone::RedstonePower::isSidePowered(world, neighborPos, dir)) {
            return true;
        }
    }

    // 额外检查活塞上方位置的水平信号
    if (world::redstone::RedstonePower::isSidePowered(world, pos, Direction::Down)) {
        return true;
    }

    BlockPos abovePos = pos.up();
    for (Direction dir : Directions::all()) {
        if (dir != Direction::Down) {
            BlockPos checkPos = abovePos.offset(dir);
            if (world::redstone::RedstonePower::isSidePowered(world, checkPos, dir)) {
                return true;
            }
        }
    }

    return false;
}

bool PistonBlock::canPush(const BlockState& blockState,
    IWorld& world,
    const BlockPos& pos,
    Direction facing,
    bool destroyBlocks,
    Direction direction)
{
    // 检查高度限制
    if (pos.y < 0 || pos.y >= world.getHeight(pos.x, pos.z)) {
        return false;
    }

    // 检查世界边界
    if (!world.worldBorder().contains(pos)) {
        return false;
    }

    // 空气可以推动
    if (blockState.isAir()) {
        return true;
    }

    // 不可推动的方块
    if (blockState.is(VanillaBlocks::OBSIDIAN) || blockState.is(VanillaBlocks::CRYING_OBSIDIAN) ||
        blockState.is(VanillaBlocks::RESPAWN_ANCHOR)) {
        return false;
    }

    // 检查高度边界（向下推时检查底部，向上推时检查顶部）
    if (facing == Direction::Down && pos.y == 0) {
        return false;
    }
    if (facing == Direction::Up && pos.y >= world.getHeight(pos.x, pos.z) - 1) {
        return false;
    }

    // 检查活塞本身
    if (blockState.is(VanillaBlocks::PISTON) || blockState.is(VanillaBlocks::STICKY_PISTON)) {
        // 已伸出的活塞不能被推动
        if (isExtended(blockState)) {
            return false;
        }
    } else {
        // 检查硬度和推动反应
        if (blockState.hardness() < 0.0f) {
            // 不可破坏的方块
            return false;
        }

        Material::PushReaction reaction = blockState.getMaterial().getPushReaction();
        switch (reaction) {
            case Material::PushReaction::Block:
                return false;
            case Material::PushReaction::Destroy:
                return destroyBlocks;
            case Material::PushReaction::PushOnly:
                // 只能被活塞面推动
                return facing == direction;
            default:
                break;
        }
    }

    // 有方块实体的方块不能被推动
    const Block& block = blockState.getBlock();
    if (block.hasBlockEntity()) {
        return false;
    }

    return true;
}

void PistonBlock::_checkForMove(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    Direction facing = getFacing(state);
    bool shouldExtend = shouldBeExtended(world, pos, state);
    bool isCurrentlyExtended = isExtended(state);

    if (shouldExtend != isCurrentlyExtended) {
        if (shouldExtend) {
            // 应该伸出
            PistonStructureHelper helper(world, pos, facing, true);
            if (helper.canMove()) {
                extend(world, pos, state);
            }
        } else {
            // 应该收回
            retract(world, pos, state);
        }
    }
}

bool PistonBlock::extend(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    Direction facing = getFacing(state);

    // 使用 PistonStructureHelper 计算推动链
    PistonStructureHelper helper(world, pos, facing, true);
    if (!helper.canMove()) {
        return false;
    }

    // 执行移动
    if (!_doMove(world, pos, facing, true)) {
        return false;
    }

    // 更新活塞状态为伸出
    const BlockState& newState = withExtended(state, true);
    world.setBlockState(pos, &newState, 67);

    return true;
}

bool PistonBlock::retract(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    Direction facing = getFacing(state);

    // 更新活塞状态为收回
    const BlockState& newState = withExtended(state, false);
    world.setBlockState(pos, &newState, 67);

    if (m_sticky) {
        // 粘性活塞：尝试拉回方块
        BlockPos frontPos = pos.offset(facing);     // 活塞头位置
        BlockPos pullPos = frontPos.offset(facing); // 活塞头前面的方块

        const BlockState* pullState = world.getBlockState(pullPos);
        if (pullState && !pullState->isAir()) {
            // 检查方块是否可以被拉回
            if (canPush(*pullState, world, pullPos, Directions::opposite(facing), false, facing)) {
                Material::PushReaction reaction = pullState->getMaterial().getPushReaction();
                if (reaction == Material::PushReaction::Normal || pullState->is(VanillaBlocks::PISTON) ||
                    pullState->is(VanillaBlocks::STICKY_PISTON)) {
                    // 执行拉回
                    _doMove(world, pos, facing, false);
                    return true;
                }
            }
        }

        // 移除活塞头
        world.setBlockState(frontPos, nullptr, 20);
    } else {
        // 普通活塞：移除活塞头
        BlockPos frontPos = pos.offset(facing);
        world.setBlockState(frontPos, nullptr, 20);
    }

    return true;
}

bool PistonBlock::_doMove(IWorld& world, const BlockPos& pos, Direction facing, bool extending)
{
    BlockPos frontPos = pos.offset(facing);

    // 收回时先清除活塞头
    if (!extending) {
        const BlockState* headState = world.getBlockState(frontPos);
        if (headState && headState->is(VanillaBlocks::PISTON_HEAD)) {
            world.setBlockState(frontPos, nullptr, 20);
        }
    }

    PistonStructureHelper helper(world, pos, facing, extending);
    if (!helper.canMove()) {
        return false;
    }

    // 获取要移动和要破坏的方块
    const std::vector<BlockPos>& toMove = helper.getBlocksToMove();
    const std::vector<BlockPos>& toDestroy = helper.getBlocksToDestroy();

    // 先破坏需要破坏的方块（从远到近）
    for (auto it = toDestroy.rbegin(); it != toDestroy.rend(); ++it) {
        const BlockPos& destroyPos = *it;
        const BlockState* destroyState = world.getBlockState(destroyPos);
        if (destroyState && !destroyState->isAir()) {
            // 破坏方块时掉落物品
            const Block* destroyBlock = &destroyState->getBlock();
            if (destroyBlock != nullptr) {
                const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*destroyBlock);
                if (blockItem != nullptr) {
                    ItemStack dropStack(blockItem, 1);
                    math::Random rng;
                    ItemDropHelper::spawnItemEntity(&world,
                        dropStack,
                        static_cast<f64>(destroyPos.x) + 0.5,
                        static_cast<f64>(destroyPos.y) + 0.5,
                        static_cast<f64>(destroyPos.z) + 0.5,
                        rng);
                }
            }
            world.setBlockState(destroyPos, nullptr, 18);
            destroyBlock->spawnAfterBreak(world, destroyPos, *destroyState, nullptr, false);
        }
    }

    // 移动方块（从远到近）
    Direction moveDir = extending ? facing : Directions::opposite(facing);
    for (auto it = toMove.rbegin(); it != toMove.rend(); ++it) {
        const BlockPos& movePos = *it;
        const BlockState* moveState = world.getBlockState(movePos);
        if (!moveState) continue;

        BlockPos newPos = movePos.offset(moveDir);

        // 创建移动活塞方块
        const BlockState& movingState =
            VanillaBlocks::MOVING_PISTON->defaultState().with(BlockStateProperties::FACING(), facing);
        world.setBlockState(newPos, &movingState, 68);

        // 创建 PistonBlockEntity
        auto entity = std::make_unique<blockentity::PistonBlockEntity>(newPos, moveState, facing, extending, false);
        world.setBlockEntity(newPos, entity.release());

        // 清除原位置
        world.setBlockState(movePos, nullptr, 68);
    }

    // 如果是伸出，在活塞位置创建移动活塞（用于活塞头动画）
    if (extending) {
        // 创建活塞头状态（使用引用类型获取持久化的 BlockState）
        // StateHolder::with() 返回 const BlockState&，指向 StateContainer 中预分配的状态
        const BlockState& pistonHeadState =
            VanillaBlocks::PISTON_HEAD->defaultState()
                .with(BlockStateProperties::FACING(), facing)
                .with(PistonHeadBlock::getTypeProperty(),
                    m_sticky ? PistonHeadBlock::Type::Sticky : PistonHeadBlock::Type::Normal);

        // 创建移动活塞方块
        const BlockState& movingState =
            VanillaBlocks::MOVING_PISTON->defaultState()
                .with(BlockStateProperties::FACING(), facing)
                .with(PistonHeadBlock::getTypeProperty(),
                    m_sticky ? PistonHeadBlock::Type::Sticky : PistonHeadBlock::Type::Normal);

        world.setBlockState(pos, &movingState, 68);

        // 创建 PistonBlockEntity 用于活塞头
        // pistonHeadState 是持久化引用，可以安全获取其指针
        // 参数：pos, pistonState（活塞头状态）, facing, extending, shouldRenderHead
        auto entity = std::make_unique<blockentity::PistonBlockEntity>(pos, &pistonHeadState, facing, true, true);
        world.setBlockEntity(pos, entity.release());
    }

    return true;
}

Material::PushReaction PistonBlock::_getBlockPushReaction(const BlockState& state) const
{
    // 已伸出的活塞不能被推动
    if (isExtended(state)) {
        return Material::PushReaction::Block;
    }
    return Material::PushReaction::Normal;
}

} // namespace blocks
} // namespace mc
