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
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
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
#include "common/world/block/BlockUpdateFlags.hpp"
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
        // 头颅拟声（MOB_HEAD/CUSTOM 类）
        case Instrument::Zombie:
            return SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_ZOMBIE;
        case Instrument::Skeleton:
            return SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_SKELETON;
        case Instrument::Creeper:
            return SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_CREEPER;
        case Instrument::Dragon:
            return SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_ENDER_DRAGON;
        case Instrument::WitherSkeleton:
            return SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_WITHER_SKELETON;
        case Instrument::Piglin:
            return SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_PIGLIN;
        case Instrument::CustomHead:
            // 玩家头颅：vanilla 用头颅 BlockEntity 的自定义音符盒音效（UI_BUTTON_CLICK 兜底）
            return SoundEvents::UI_BUTTON_CLICK;
        default:
            return SoundEvents::BLOCK_NOTE_BLOCK_HARP;
    }
}

// 方块对音符盒的有效乐器：若方块显式设置了 instrument 属性则直接使用，否则在
// NoteBlock::blockInstrumentByType 的默认规则中按方块类型查找（对齐 vanilla 1.20 及以前的
// NoteBlockInstrument.byState；1.21.11 起同样效果改由各方块注册期显式给出）。
BlockProperties::Instrument effectiveInstrument(const BlockState& state)
{
    if (state.getBlock().hasExplicitInstrument()) {
        return state.getBlock().getInstrument();
    }
    return NoteBlock::blockInstrumentByType(&state.getBlock());
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
            .with(BlockStateProperties::NOTE_BLOCK_INSTRUMENT(), BlockStateProperties::NoteBlockInstrument::Harp)
            .with(BlockStateProperties::NOTE_0_24(), 0)
            .with(BlockStateProperties::POWERED(), false));
}

i32 NoteBlock::getNote(const BlockState& state)
{
    return state.get(BlockStateProperties::NOTE_0_24());
}

BlockState NoteBlock::withNote(const BlockState& state, i32 note)
{
    // 确保在范围内
    note = std::max(0, std::min(note, NOTE_RANGE - 1));
    return state.with(BlockStateProperties::NOTE_0_24(), note);
}

BlockState NoteBlock::cycleNote(const BlockState& state)
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
        world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_ALL);
    }
}

BlockActionResult NoteBlock::useItemOn(ItemStack& stack,
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    // 对齐 vanilla NoteBlock.useItemOn（1.21.11）：
    //   手持 NOTE_BLOCK_TOP_INSTRUMENTS 标签物品（各类头颅）且点击顶面 → 返回 Pass，交物品放置
    //   （使头颅落在音符盒上方以决定乐器音色）；否则走默认（→ 驱动 useWithoutItem 升调）。
    if (item::tag::ItemTags::NOTE_BLOCK_TOP_INSTRUMENTS().contains(stack) && hit.face() == Direction::Up) {
        return BlockActionResult::pass();
    }
    return Block::useItemOn(stack, state, world, pos, player, hand, hit);
}

BlockActionResult NoteBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    // 对齐 vanilla NoteBlock.useWithoutItem（1.21.11）：右键升调——cycle(NOTE) 循环升半音（0→1→...→24→0）+
    //   setBlockState 写回 + playNote 播放新音高 → return SUCCESS。不检查手持物（空手/任意物品右键均升调），
    //   也不检查 mayBuild（vanilla useWithoutItem 无建造权限守卫，空手即可升调）。
    //   手持头颅类物品点击顶面的降级分支由 useItemOn 处理（见上）。
    MC_UNUSED(player);
    MC_UNUSED(hit);

    // 服务端逻辑（vanilla !isClientSide 守卫）。GameTest 服务端跑，走此分支。
    BlockState newState = cycleNote(state); // note (cur+1) % 25 循环升半音
    world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_ALL);
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
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 对齐 vanilla NoteBlock.updateShape：仅 Y 轴邻居变化（上下方方块增删）会改变乐器，
    //   此时重解析 INSTRUMENT；水平邻居变化不改乐器。
    if (getAxis(facing) == Axis::Y) {
        return resolveInstrument(world, currentPos, state);
    }
    return state;
}

BlockState NoteBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 对齐 vanilla NoteBlock.getStateForPlacement：放置即依据上下方方块写入乐器属性，
    //   故放置瞬间音色即正确，无需等到首次播放。
    return resolveInstrument(context.world(), context.placementPos(), defaultState());
}

BlockProperties::Instrument NoteBlock::blockInstrumentByType(const Block* block)
{
    using Instrument = BlockProperties::Instrument;
    if (block == nullptr) {
        return Instrument::Harp;
    }
    const auto is = [block](const Block* target) {
        return target != nullptr && block == target;
    };

    // 陶土 -> 长笛（FLUTE）
    if (is(VanillaBlocks::CLAY)) {
        return Instrument::Flute;
    }
    // 金块 -> 钟（BELL）
    if (is(VanillaBlocks::GOLD_BLOCK)) {
        return Instrument::Bell;
    }
    // 羊毛（任意颜色）-> 吉他（GUITAR）
    if (BlockTags::WOOL().contains(*block)) {
        return Instrument::Guitar;
    }
    // 浮冰 -> 风铃（CHIME）
    if (is(VanillaBlocks::PACKED_ICE)) {
        return Instrument::Chime;
    }
    // 骨块 -> 木琴（XYLOPHONE）
    if (is(VanillaBlocks::BONE_BLOCK)) {
        return Instrument::Xylophone;
    }
    // 铁块 -> 铁琴（IRON_XYLOPHONE）
    if (is(VanillaBlocks::IRON_BLOCK)) {
        return Instrument::IronXylophone;
    }
    // 灵魂沙 -> 牛铃（COW_BELL）
    if (is(VanillaBlocks::SOUL_SAND)) {
        return Instrument::CowBell;
    }
    // 南瓜 -> 迪吉里杜管（DIDGERIDOO）；雕刻南瓜与南瓜灯不触发。
    if (is(VanillaBlocks::PUMPKIN)) {
        return Instrument::Didgeridoo;
    }
    if (is(VanillaBlocks::CARVED_PUMPKIN) || is(VanillaBlocks::JACK_O_LANTERN)) {
        return Instrument::Harp;
    }
    // 绿宝石块 -> 电子音（BIT）
    if (is(VanillaBlocks::EMERALD_BLOCK)) {
        return Instrument::Bit;
    }
    // 干草块 -> 班卓琴（BANJO）
    if (is(VanillaBlocks::HAY_BLOCK)) {
        return Instrument::Banjo;
    }
    // 荧石 -> 电钢琴（PLING）
    if (is(VanillaBlocks::GLOWSTONE)) {
        return Instrument::Pling;
    }
    // 泥土类 -> 长笛（FLUTE）
    if (block->material() == Material::EARTH) {
        return Instrument::Flute;
    }
    // 石头类 -> 底鼓（BASEDRUM）
    if (block->material() == Material::ROCK) {
        return Instrument::BaseDrum;
    }
    // 沙子类 -> 军鼓（SNARE）
    if (block->material() == Material::SAND) {
        return Instrument::Snare;
    }
    // 玻璃类 -> 踩镲（HAT）
    if (block->material() == Material::GLASS) {
        return Instrument::Hat;
    }
    // 木质类 -> 贝斯（BASS）
    if (block->material() == Material::WOOD || block->material() == Material::NETHER_WOOD) {
        return Instrument::Bass;
    }
    return Instrument::Harp;
}

BlockState NoteBlock::resolveInstrument(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 对齐 vanilla NoteBlock.setInstrument：
    //   ① 上方方块的乐器若 worksAboveNoteBlock()（MOB_HEAD / CUSTOM 类头颅）→ 直接采纳；
    //   ② 否则取下方方块的乐器，但下方若自身 worksAboveNoteBlock() → 回退 HARP
    //      （头颅垫在音符盒下方时不传出拟声）。
    // 乐器来自方块的 instrument 属性（Block::getInstrument，注册期由各方块设定，头颅按种类设定），
    // 与 vanilla BlockState.instrument() 语义一致。
    const BlockState* aboveState = world.getBlockState(pos.up());
    if (aboveState != nullptr) {
        const auto above =
            static_cast<BlockStateProperties::NoteBlockInstrument>(static_cast<u8>(effectiveInstrument(*aboveState)));
        if (BlockStateProperties::noteBlockInstrumentWorksAboveNoteBlock(above)) {
            return state.with(BlockStateProperties::NOTE_BLOCK_INSTRUMENT(), above);
        }
    }

    const BlockState* belowState = world.getBlockState(pos.down());
    const auto below = belowState != nullptr
        ? static_cast<BlockStateProperties::NoteBlockInstrument>(static_cast<u8>(effectiveInstrument(*belowState)))
        : BlockStateProperties::NoteBlockInstrument::Harp;
    const auto instrument = BlockStateProperties::noteBlockInstrumentWorksAboveNoteBlock(below) && belowState != nullptr
        ? BlockStateProperties::NoteBlockInstrument::Harp
        : below;
    return state.with(BlockStateProperties::NOTE_BLOCK_INSTRUMENT(), instrument);
}

void NoteBlock::triggerNote(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    i32 note = getNote(state);
    i32 instrument = _getInstrumentType(world, pos, state);

    // 播放音符
    _playNote(world, pos, instrument, note);
}

i32 NoteBlock::_getInstrumentType(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 乐器在放置/竖直邻居更新时已由 resolveInstrument 写入 INSTRUMENT 属性（对齐 vanilla
    //   NoteBlock.updateShape/getStateForPlacement）。播放时直接读取该属性即可——
    //   这样头颅置于上方 / 头颅垫在下方回退 HARP 等语义都由属性承载，播放路径无需再判定。
    return static_cast<i32>(state.get(BlockStateProperties::NOTE_BLOCK_INSTRUMENT()));
}

bool NoteBlock::_isTunableInstrument(i32 instrument)
{
    return BlockStateProperties::noteBlockInstrumentIsTunable(static_cast<Instrument>(instrument));
}

void NoteBlock::_playNote(IWorld& world, const BlockPos& pos, i32 instrument, i32 note)
{
    // 转换乐器类型
    auto instrumentEnum = static_cast<Instrument>(instrument);

    // 获取对应的声音事件
    const ResourceLocation& soundEvent = getSoundEventForInstrument(instrumentEnum);

    // 对齐 vanilla NoteBlock.triggerEvent：仅可调音乐器（BASE_BLOCK 类）按音符值定音高并产生
    //   音符粒子；头颅类（MOB_HEAD/CUSTOM）音高固定 1.0 且不产生调音粒子。
    f32 pitch = 1.0f;
    if (_isTunableInstrument(instrument)) {
        pitch = calculatePitch(note);

        // 生成音符粒子：位置为方块上方中心，颜色数据 note / 24.0
        world.addParticle(particle::ParticleTypeId::Note,
            Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 1.2f, static_cast<f32>(pos.z) + 0.5f),
            Vector3(static_cast<f32>(note) / 24.0f, // 颜色数据
                0.0f,
                0.0f));
    }

    // 播放声音（音量固定为 3.0，声音类别为 RECORDS）
    Vector3 soundPos = pos.center();
    world.playSound(soundEvent,
        sound::SoundCategory::Records,
        soundPos,
        3.0f, // 音量
        pitch // 音高
    );
}

} // namespace blocks
} // namespace mc
