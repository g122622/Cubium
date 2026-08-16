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

#include "NoteBlock.hpp"
#include "common/core/Types.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// 音符盒乐器映射
// ============================================================================

namespace {

/**
 * @brief 乐器类型枚举别名
 *
 * 为 NoteBlockInstrument 枚举类型创建简短的别名。
 */
using Instrument = BlockStateProperties::NoteBlockInstrument;

/**
 * @brief 获取声音事件资源位置
 *
 * 根据 NoteBlockInstrument 枚举值返回对应的声音事件。
 */
const ResourceLocation& getSoundEventForInstrument(Instrument instrument)
{
    switch (instrument) {
        case Instrument::Harp:
            return SoundEvents::BLOCK_NOTE_BLOCK_HARP;
        case Instrument::Basedrum:
            return SoundEvents::BLOCK_NOTE_BLOCK_BASEDRUM;
        case Instrument::Snare:
            return SoundEvents::BLOCK_NOTE_BLOCK_SNARE;
        case Instrument::Hat:
            return SoundEvents::BLOCK_NOTE_BLOCK_HAT;
        case Instrument::Bass:
            return SoundEvents::BLOCK_NOTE_BLOCK_BASS;
        case Instrument::Flute:
            return SoundEvents::BLOCK_NOTE_BLOCK_FLUTE;
        case Instrument::Bell:
            return SoundEvents::BLOCK_NOTE_BLOCK_BELL;
        case Instrument::Guitar:
            return SoundEvents::BLOCK_NOTE_BLOCK_GUITAR;
        case Instrument::Chime:
            return SoundEvents::BLOCK_NOTE_BLOCK_CHIME;
        case Instrument::Xylophone:
            return SoundEvents::BLOCK_NOTE_BLOCK_XYLOPHONE;
        case Instrument::IronXylophone:
            return SoundEvents::BLOCK_NOTE_BLOCK_IRON_XYLOPHONE;
        case Instrument::CowBell:
            return SoundEvents::BLOCK_NOTE_BLOCK_COW_BELL;
        case Instrument::Didgeridoo:
            return SoundEvents::BLOCK_NOTE_BLOCK_DIDGERIDOO;
        case Instrument::Bit:
            return SoundEvents::BLOCK_NOTE_BLOCK_BIT;
        case Instrument::Banjo:
            return SoundEvents::BLOCK_NOTE_BLOCK_BANJO;
        case Instrument::Pling:
            return SoundEvents::BLOCK_NOTE_BLOCK_PLING;
        default:
            return SoundEvents::BLOCK_NOTE_BLOCK_HARP;
    }
}

/**
 * @brief 根据音符值计算音高
 *
 * 音高计算公式: f = 2^((note - 12) / 12)
 * - 音符范围: 0-24 (共25个音高，对应两个八度)
 * - 基准音高: note = 12 时, f = 1.0 (标准音高)
 * - 每增加 1 个音符值，音高上升一个半音
 * - 每增加 12 个音符值，音高上升一个八度 (频率翻倍)
 */
f32 calculatePitch(i32 note)
{
    return static_cast<f32>(std::pow(2.0, static_cast<f64>(note - 12) / 12.0));
}

/**
 * @brief 检查方块是否为指定方块的实例
 *
 * 使用方块指针比较，避免字符串比较。
 */
bool isBlock(const BlockState* state, Block* targetBlock)
{
    if (state == nullptr || targetBlock == nullptr) {
        return false;
    }
    return &state->getBlock() == targetBlock;
}

} // anonymous namespace

// ============================================================================
// NoteBlock 实现
// ============================================================================

NoteBlock::NoteBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    // vanilla 1.21.11 note_block 属性：instrument + note + powered
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::NOTE_BLOCK_INSTRUMENT())
            .add(BlockStateProperties::NOTE_0_24())
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
            .with(BlockStateProperties::NOTE_BLOCK_INSTRUMENT(), BlockStateProperties::NoteBlockInstrument::Harp)
            .with(BlockStateProperties::NOTE_0_24(), 0)
            .with(BlockStateProperties::POWERED(), false));
}

i32 NoteBlock::getNote(const BlockState& state)
{
    return state.get(BlockStateProperties::NOTE_0_24());
}

BlockState NoteBlock::withNote(BlockState state, i32 note)
{
    // 确保在范围内
    note = std::max(0, std::min(note, NOTE_RANGE - 1));
    return state.with(BlockStateProperties::NOTE_0_24(), note);
}

BlockState NoteBlock::cycleNote(BlockState state)
{
    i32 currentNote = getNote(state);
    i32 nextNote = (currentNote + 1) % NOTE_RANGE;
    return withNote(state, nextNote);
}

void NoteBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
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

BlockActionResult NoteBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    // 对齐 vanilla NoteBlock.useWithoutItem（1.21.11）：右键升调——cycle(NOTE) 循环升半音（0→1→...→24→0）+
    //   setBlockState 写回 + playNote 播放新音高 → return SUCCESS。不检查手持物（空手/任意物品右键均升调），
    //   也不检查 mayBuild（vanilla useWithoutItem 无建造权限守卫，空手即可升调）。
    //   注：vanilla useItemOn 对 NOTE_BLOCK_TOP_INSTRUMENTS 物品（头颅等，放上方决定乐器）+ 顶面点击返
    //   PASS 交物品放置；Cubium 此处简化为统一升调（TODO: 待 NOTE_BLOCK_TOP_INSTRUMENTS 标签与朝向守卫
    //   完善后对齐 useItemOn 的 PASS 分支）。
    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 服务端逻辑（vanilla !isClientSide 守卫）。GameTest 服务端跑，走此分支。
    BlockState newState = cycleNote(state); // note (cur+1) % 25 循环升半音
    world.setBlockState(pos, &newState, 3);
    triggerNote(world, pos, newState); // 用新 note 播放音符

    return ActionResultType::Success;
}

BlockState NoteBlock::updatePostPlacement(const BlockState& state,
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

void NoteBlock::triggerNote(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    i32 note = getNote(state);
    i32 instrument = _getInstrumentType(world, pos);

    // 播放音符
    _playNote(world, pos, instrument, note);
}

i32 NoteBlock::_getInstrumentType(IWorld& world, const BlockPos& pos) const
{
    // 根据音符盒下方的方块类型确定乐器

    BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);

    if (!belowState) {
        return static_cast<i32>(Instrument::Harp);
    }

    // ========================================================================
    // 特定方块检测（优先级从高到低）
    // ========================================================================

    // 陶土 -> 长笛 (FLUTE)
    if (isBlock(belowState, VanillaBlocks::CLAY)) {
        return static_cast<i32>(Instrument::Flute);
    }

    // 金块 -> 钟 (BELL)
    if (isBlock(belowState, VanillaBlocks::GOLD_BLOCK)) {
        return static_cast<i32>(Instrument::Bell);
    }

    // 羊毛 (任意颜色) -> 吉他 (GUITAR)
    if (BlockTags::WOOL().contains(*belowState)) {
        return static_cast<i32>(Instrument::Guitar);
    }

    // 浮冰 -> 管钟 (CHIME)
    if (isBlock(belowState, VanillaBlocks::PACKED_ICE)) {
        return static_cast<i32>(Instrument::Chime);
    }

    // 骨块 -> 木琴 (XYLOPHONE)
    if (isBlock(belowState, VanillaBlocks::BONE_BLOCK)) {
        return static_cast<i32>(Instrument::Xylophone);
    }

    // 铁块 -> 铁片琴 (IRON_XYLOPHONE)
    if (isBlock(belowState, VanillaBlocks::IRON_BLOCK)) {
        return static_cast<i32>(Instrument::IronXylophone);
    }

    // 灵魂沙 -> 牛铃 (COW_BELL)
    if (isBlock(belowState, VanillaBlocks::SOUL_SAND)) {
        return static_cast<i32>(Instrument::CowBell);
    }

    // 南瓜 -> 迪吉里杜管 (DIDGERIDOO)
    // 注意: CARVED_PUMPKIN 和 JACK_O_LANTERN 不会触发此乐器
    if (isBlock(belowState, VanillaBlocks::PUMPKIN)) {
        return static_cast<i32>(Instrument::Didgeridoo);
    }

    // 绿宝石块 -> 电子音 (BIT)
    if (isBlock(belowState, VanillaBlocks::EMERALD_BLOCK)) {
        return static_cast<i32>(Instrument::Bit);
    }

    // 干草块 -> 班卓琴 (BANJO)
    if (isBlock(belowState, VanillaBlocks::HAY_BLOCK)) {
        return static_cast<i32>(Instrument::Banjo);
    }

    // 荧石 -> 电钢琴 (PLING)
    if (isBlock(belowState, VanillaBlocks::GLOWSTONE)) {
        return static_cast<i32>(Instrument::Pling);
    }

    // ========================================================================
    // 材质类型检测（次优先级）
    // ========================================================================

    const Material& material = belowState->getBlock().material();

    // 石头类材质 -> 底鼓 (BASEDRUM)
    if (material == Material::ROCK) {
        return static_cast<i32>(Instrument::Basedrum);
    }

    // 沙子类材质 -> 军鼓 (SNARE)
    if (material == Material::SAND) {
        return static_cast<i32>(Instrument::Snare);
    }

    // 玻璃材质 -> 踩镲 (HAT)
    if (material == Material::GLASS) {
        return static_cast<i32>(Instrument::Hat);
    }

    // 木头材质（包括下界木） -> 贝斯 (BASS)
    if (material == Material::WOOD || material == Material::NETHER_WOOD) {
        return static_cast<i32>(Instrument::Bass);
    }

    // 默认 -> 钢琴 (HARP)
    return static_cast<i32>(Instrument::Harp);
}

void NoteBlock::_playNote(IWorld& world, const BlockPos& pos, i32 instrument, i32 note)
{
    // 转换乐器类型
    auto instrumentEnum = static_cast<Instrument>(instrument);

    // 获取对应的声音事件
    const ResourceLocation& soundEvent = getSoundEventForInstrument(instrumentEnum);

    // 计算音高 (基于音符值 0-24)
    f32 pitch = calculatePitch(note);

    // 播放声音（音量固定为 3.0，声音类别为 RECORDS）
    Vector3 soundPos = pos.center();
    world.playSound(soundEvent,
        sound::SoundCategory::Records,
        soundPos,
        3.0f, // 音量
        pitch // 音高
    );

    // 生成音符粒子效果
    // 粒子类型: NOTE
    // 位置: 方块上方中心
    // 颜色数据: note / 24.0 (用于确定粒子颜色)
    world.addParticle(particle::ParticleTypeId::Note,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 1.2f, static_cast<f32>(pos.z) + 0.5f),
        Vector3(static_cast<f32>(note) / 24.0f, // 颜色数据
            0.0f,
            0.0f));
}

} // namespace blocks
} // namespace mc
