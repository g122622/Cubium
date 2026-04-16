#include "NoteBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../IWorld.hpp"
#include "../../../../util/property/Properties.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

NoteBlock::NoteBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::NOTE_0_24())
        .add(BlockStateProperties::POWERED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::NOTE_0_24(), 0)
        .with(BlockStateProperties::POWERED(), false));
}

i32 NoteBlock::getNote(const BlockState& state) {
    return state.get(BlockStateProperties::NOTE_0_24());
}

BlockState NoteBlock::withNote(BlockState state, i32 note) {
    // 确保在范围内
    note = std::max(0, std::min(note, NOTE_RANGE - 1));
    return state.with(BlockStateProperties::NOTE_0_24(), note);
}

BlockState NoteBlock::cycleNote(BlockState state) {
    i32 currentNote = getNote(state);
    i32 nextNote = (currentNote + 1) % NOTE_RANGE;
    return withNote(state, nextNote);
}

void NoteBlock::neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                                 const BlockPos& neighborPos, bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    bool shouldPower = world::redstone::RedstonePower::isPowered(world, pos);
    bool isPowered = state->get(BlockStateProperties::POWERED());

    if (shouldPower != isPowered) {
        if (shouldPower) {
            // 被激活时播放音符
            triggerNote(world, pos, *state);
        }
        BlockState newState = state->with(BlockStateProperties::POWERED(), shouldPower);
        world.setBlockState(pos, &newState, 3);
    }
}

BlockState NoteBlock::updatePostPlacement(
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

void NoteBlock::triggerNote(IWorld& world, const BlockPos& pos, const BlockState& state) {
    i32 note = getNote(state);
    i32 instrument = getInstrumentType(world, pos);

    // 播放音符
    playNote(world, pos, instrument, note);
}

i32 NoteBlock::getInstrumentType(IWorld& world, const BlockPos& pos) const {
    // 根据音符盒下方的方块类型确定乐器
    // 参考: net.minecraft.block.NoteBlock.getInstrument

    BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);

    if (!belowState) {
        return 0; // 默认: 钢琴 (harp)
    }

    // TODO: 根据方块材质返回对应的乐器类型
    // 0: 钢琴 - 默认
    // 1: 贝斯 - 木质
    // 2: 鼓 - 沙子类
    // 3: 架子鼓 - 玻璃
    // 4: 长笛 - 圆石类
    // 5: 吉他 - 沙砾类
    // 6: 管钟 - 铁块
    // 7: 木琴 - 金块
    // 8: 铁片琴 - 冰
    // 9: 牛铃 - 粘土
    // 10: 迪吉里杜管 - 浮冰
    // 11: 鼓 - 浮冰
    // 12: 铜钹 - 浮冰
    // 13: 电钢琴 - 羊毛
    // 14: 钢鼓 - 羊毛
    // 15: 口风琴 - 羊毛
    // 16: 大键琴 - 羊毛

    MC_UNUSED(belowState);
    return 0;
}

void NoteBlock::playNote(IWorld& world, const BlockPos& pos, i32 instrument, i32 note) {
    // TODO: 播放音符声音
    // 根据乐器和音调播放对应的声音
    // world.playSound(pos, SoundEvents::getNoteSound(instrument), 1.0f, getNotePitch(note));

    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(instrument);
    MC_UNUSED(note);
}

} // namespace blocks
} // namespace mc
