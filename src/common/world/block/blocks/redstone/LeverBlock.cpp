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

#include "LeverBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// 使用 BlockStateProperties 中的 AttachFace
using AttachFace = BlockStateProperties::AttachFace;

LeverBlock::LeverBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::ATTACH_FACE())
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
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::ATTACH_FACE(), AttachFace::Wall));
}

BlockState LeverBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 朝向与附着面由玩家点击面决定（附墙方块语义，同按钮）：
    //   点击顶面 → ATTACH_FACE=Floor，facing=玩家水平朝向（horizontalDirection，仅 yaw）；
    //   点击底面 → ATTACH_FACE=Ceiling，facing=玩家水平朝向；
    //   点击墙面 → ATTACH_FACE=Wall，facing=点击面（水平四向）。
    // 此前未重写该方法，落回基类 Block::getStateForPlacement 返回 defaultState()（HORIZONTAL_FACING 恒
    // North、ATTACH_FACE 恒 Wall），与预期按点击面决定朝向/附着面的行为不一致。重写后修正。
    Direction clickedFace = context.getClickedFace();
    Direction horizontalFacing = context.horizontalDirection();

    AttachFace attachFace;
    Direction finalFacing = horizontalFacing;

    if (clickedFace == Direction::Up) {
        attachFace = AttachFace::Floor;
    } else if (clickedFace == Direction::Down) {
        attachFace = AttachFace::Ceiling;
    } else {
        attachFace = AttachFace::Wall;
        finalFacing = clickedFace;
    }

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), finalFacing)
        .with(BlockStateProperties::ATTACH_FACE(), attachFace);
}

bool LeverBlock::isPowered(const BlockState& state)
{
    return state.get(BlockStateProperties::POWERED());
}

BlockState LeverBlock::withPowered(BlockState state, bool powered)
{
    return state.with(BlockStateProperties::POWERED(), powered);
}

Direction LeverBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

void LeverBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 拉杆放置时不触发信号
}

void LeverBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检查支撑是否还在
    Direction facing = getFacing(*state);
    AttachFace attachFace = state->get(BlockStateProperties::ATTACH_FACE());

    // 计算支撑方块位置
    BlockPos supportPos;
    switch (attachFace) {
        case AttachFace::Floor:
            supportPos = pos.down();
            break;
        case AttachFace::Ceiling:
            supportPos = pos.up();
            break;
        case AttachFace::Wall:
            supportPos = pos.offset(Directions::opposite(facing));
            break;
    }

    // 如果支撑方块被移除，拉杆掉落
    const BlockState* supportState = world.getBlockState(supportPos);
    if (!supportState || supportState->isAir()) {
        // 拉杆掉落 - 设置为空气方块
        world.setBlockState(pos, nullptr, 2);
    }
}

BlockState LeverBlock::updatePostPlacement(const BlockState& state,
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

BlockState LeverBlock::toggle(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    bool newPowered = !isPowered(state);
    BlockState newState = withPowered(state, newPowered);
    world.setBlockState(pos, &newState, 2);

    // 播放音效
    _playClickSound(world, pos, newPowered);

    // 通知相邻方块更新
    _notifyNeighbors(world, pos, newState);

    return newState;
}

i32 LeverBlock::getWeakPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 拉杆开启时向所有方向输出弱信号
    return isPowered(state) ? world::redstone::RedstonePower::MAX_POWER : 0;
}

i32 LeverBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只在朝向方向输出强信号
    if (!isPowered(state)) {
        return 0;
    }

    // 获取拉杆朝向（输出方向）
    Direction facing = getFacing(state);
    AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());

    Direction outputDir = Direction::North; // 默认值
    switch (attachFace) {
        case AttachFace::Floor:
            outputDir = Direction::Up;
            break;
        case AttachFace::Ceiling:
            outputDir = Direction::Down;
            break;
        case AttachFace::Wall:
            outputDir = facing;
            break;
        default:
            break;
    }

    // 只在输出方向输出强信号
    if (side == outputDir) {
        return world::redstone::RedstonePower::MAX_POWER;
    }

    return 0;
}

void LeverBlock::_playClickSound(IWorld& world, const BlockPos& pos, bool powered)
{
    // 注意：拉杆使用木质按钮音效
    world.playSound(powered ? SoundEvents::BLOCK_WOODEN_BUTTON_CLICK_ON : SoundEvents::BLOCK_WOODEN_BUTTON_CLICK_OFF,
        sound::SoundCategory::Blocks,
        pos.center(),
        0.3f,
        0.6f);
}

void LeverBlock::_notifyNeighbors(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 获取拉杆输出方向
    Direction facing = getFacing(state);
    AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());

    Direction outputDir = Direction::North; // 默认值
    BlockPos supportPos = pos;
    switch (attachFace) {
        case AttachFace::Floor:
            outputDir = Direction::Up;
            supportPos = pos.down();
            break;
        case AttachFace::Ceiling:
            outputDir = Direction::Down;
            supportPos = pos.up();
            break;
        case AttachFace::Wall:
            outputDir = facing;
            supportPos = pos.offset(Directions::opposite(facing));
            break;
        default:
            break;
    }

    // 获取方块引用
    Block& thisBlock = state.getBlockMutable();

    // 通知输出方向的方块
    BlockPos outputPos = pos.offset(outputDir);
    const BlockState* outputState = world.getBlockState(outputPos);
    if (outputState && !outputState->isAir()) {
        Block& outputBlock = outputState->getBlockMutable();
        outputBlock.neighborChanged(world, outputPos, thisBlock, pos, false);
    }

    // 通过支撑方块传递信号
    const BlockState* supportState = world.getBlockState(supportPos);
    if (supportState && !supportState->isAir()) {
        Block& supportBlock = supportState->getBlockMutable();
        supportBlock.neighborChanged(world, supportPos, thisBlock, pos, false);
    }
}

} // namespace blocks
} // namespace mc
