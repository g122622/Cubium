#include "LeverBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../IWorld.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

// 使用 BlockStateProperties 中的 AttachFace
using AttachFace = BlockStateProperties::AttachFace;

LeverBlock::LeverBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::POWERED())
        .add(BlockStateProperties::ATTACH_FACE())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::POWERED(), false)
        .with(BlockStateProperties::ATTACH_FACE(), AttachFace::Wall));
}

bool LeverBlock::isPowered(const BlockState& state) {
    return state.get(BlockStateProperties::POWERED());
}

BlockState LeverBlock::withPowered(BlockState state, bool powered) {
    return state.with(BlockStateProperties::POWERED(), powered);
}

Direction LeverBlock::getFacing(const BlockState& state) {
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

void LeverBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 拉杆放置时不触发信号
}

void LeverBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                 const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
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
    const BlockState* supportState = world.getBlockState(supportPos.x, supportPos.y, supportPos.z);
    if (!supportState || supportState->isAir()) {
        // 拉杆掉落 - 设置为空气方块
        world.setBlockState(pos.x, pos.y, pos.z, nullptr, 2);
    }
}

BlockState LeverBlock::updatePostPlacement(
    const BlockState& state, Direction facing,
    const BlockState& facingState, IWorld& world,
    const BlockPos& currentPos, const BlockPos& facingPos) {
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    return state;
}

BlockState LeverBlock::toggle(IWorld& world, const BlockPos& pos, const BlockState& state) {
    bool newPowered = !isPowered(state);
    BlockState newState = withPowered(state, newPowered);
    world.setBlockState(pos.x, pos.y, pos.z, &newState, 2);

    // 播放音效
    playClickSound(world, pos, newPowered);

    // 通知相邻方块更新
    notifyNeighbors(world, pos, newState);

    return newState;
}

i32 LeverBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // MC Java: return blockState.get(POWERED) ? 15 : 0;
    // 拉杆开启时向所有方向输出弱信号
    return isPowered(state) ? world::redstone::RedstonePower::MAX_POWER : 0;
}

i32 LeverBlock::getStrongPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side
) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // MC Java: return blockState.get(POWERED) && getFacing(blockState) == side ? 15 : 0;
    // 只在朝向方向输出强信号
    if (!isPowered(state)) {
        return 0;
    }

    // 获取拉杆朝向（输出方向）
    Direction facing = getFacing(state);
    AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());

    Direction outputDir = Direction::North;  // 默认值
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

void LeverBlock::playClickSound(IWorld& world, const BlockPos& pos, bool powered) {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(powered);
    // TODO: 播放拉杆音效
    // world.playSound(pos, powered ? SoundEvents::BLOCK_LEVER_CLICK_ON
    //                              : SoundEvents::BLOCK_LEVER_CLICK_OFF,
    //                 0.3f, 0.6f);
}

void LeverBlock::notifyNeighbors(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 获取拉杆输出方向
    Direction facing = getFacing(state);
    AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());

    Direction outputDir = Direction::North;  // 默认值
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
    const Block& thisBlock = state.getBlock();

    // 通知输出方向的方块
    BlockPos outputPos = pos.offset(outputDir);
    const BlockState* outputState = world.getBlockState(outputPos.x, outputPos.y, outputPos.z);
    if (outputState && !outputState->isAir()) {
        Block& outputBlock = const_cast<Block&>(outputState->getBlock());
        outputBlock.neighborChanged(world, outputPos, const_cast<Block&>(thisBlock), pos, false);
    }

    // 通过支撑方块传递信号
    const BlockState* supportState = world.getBlockState(supportPos.x, supportPos.y, supportPos.z);
    if (supportState && !supportState->isAir()) {
        Block& supportBlock = const_cast<Block&>(supportState->getBlock());
        supportBlock.neighborChanged(world, supportPos, const_cast<Block&>(thisBlock), pos, false);
    }
}

} // namespace blocks
} // namespace mc
