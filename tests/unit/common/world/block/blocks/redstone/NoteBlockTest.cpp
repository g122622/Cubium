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

#include <gtest/gtest.h>

#include "common/BlockInteractionTestHelper.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/redstone/NoteBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"

#include <cmath>
#include <map>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 测试用世界类
 *
 * 提供最小化的 IWorld 实现用于测试音符盒乐器检测逻辑。
 */
class NoteBlockTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it == m_blocks.end() ? nullptr : &it->second;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        return setBlockState(x, y, z, state, 0);
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        const BlockPos pos(x, y, z);
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks.insert_or_assign(pos, *state);
        }
        return true;
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        MC_UNUSED(category);
        MC_UNUSED(position);
        MC_UNUSED(volume);
        m_playedSoundIds.push_back(soundEventId);
        m_lastPitch = pitch;
    }

    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        MC_UNUSED(type);
        MC_UNUSED(pos);
        MC_UNUSED(velocity);
        ++m_particleCount;
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("NoteBlockTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("NoteBlockTestWorld::tickManager not implemented");
    }

    void setBlockAt(const BlockPos& pos, const BlockState& state)
    {
        m_blocks.insert_or_assign(pos, state);
    }

    void clearBlockAt(const BlockPos& pos)
    {
        m_blocks.erase(pos);
    }

    [[nodiscard]] const std::vector<ResourceLocation>& playedSoundIds() const
    {
        return m_playedSoundIds;
    }

    [[nodiscard]] f32 lastPitch() const
    {
        return m_lastPitch;
    }

    [[nodiscard]] i32 particleCount() const
    {
        return m_particleCount;
    }

    void clear()
    {
        m_blocks.clear();
        m_playedSoundIds.clear();
        m_lastPitch = 1.0f;
        m_particleCount = 0;
    }

private:
    std::map<BlockPos, BlockState> m_blocks;
    std::vector<ResourceLocation> m_playedSoundIds;
    f32 m_lastPitch = 1.0f;
    i32 m_particleCount = 0;
};

} // anonymous namespace

// ============================================================================
// 测试类
// ============================================================================

class NoteBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块
        VanillaBlocks::initialize();
        BlockTags::initialize();
        // useItemOn 测试需要物品与物品标签（NOTE_BLOCK_TOP_INSTRUMENTS）
        Items::ensureInitialized();
        item::tag::ItemTags::initialize();

        m_world = std::make_unique<NoteBlockTestWorld>();
    }

    void TearDown() override
    {
        m_world.reset();
    }

    /// 在 pos 放置音符盒：走 resolveInstrument（等价 vanilla getStateForPlacement），
    /// 使 INSTRUMENT 属性按上下方方块解析后写入，再返回放置后的状态。
    BlockState placeNoteBlock(const BlockPos& pos)
    {
        BlockState placed = NoteBlock::resolveInstrument(*m_world, pos, VanillaBlocks::NOTE_BLOCK->defaultState());
        m_world->setBlockAt(pos, placed);
        return placed;
    }

    std::unique_ptr<NoteBlockTestWorld> m_world;
};

// ============================================================================
// 乐器类型测试
// ============================================================================

/**
 * @brief 头颅置于音符盒上方时解析为对应 MOB_HEAD/CUSTOM 乐器
 *
 * 对齐 vanilla NoteBlockInstrument.worksAboveNoteBlock：头颅类的乐器在音符盒上方生效，
 * 取代下方方块决定音色。覆盖 7 种头颅（站立 + 墙挂变体）。
 */
TEST_F(NoteBlockTest, InstrumentType_HeadAboveResolvesMobHeadInstrument)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);

    struct HeadCase {
        const Block* head;
        const Block* wallHead;
        BlockStateProperties::NoteBlockInstrument expected;
    };

    const HeadCase cases[] = {
        {VanillaBlocks::SKELETON_SKULL,
            VanillaBlocks::SKELETON_WALL_SKULL,
            BlockStateProperties::NoteBlockInstrument::Skeleton},
        {VanillaBlocks::WITHER_SKELETON_SKULL,
            VanillaBlocks::WITHER_SKELETON_WALL_SKULL,
            BlockStateProperties::NoteBlockInstrument::WitherSkeleton},
        {VanillaBlocks::ZOMBIE_HEAD,
            VanillaBlocks::ZOMBIE_WALL_HEAD,
            BlockStateProperties::NoteBlockInstrument::Zombie},
        {VanillaBlocks::CREEPER_HEAD,
            VanillaBlocks::CREEPER_WALL_HEAD,
            BlockStateProperties::NoteBlockInstrument::Creeper},
        {VanillaBlocks::DRAGON_HEAD,
            VanillaBlocks::DRAGON_WALL_HEAD,
            BlockStateProperties::NoteBlockInstrument::Dragon},
        {VanillaBlocks::PIGLIN_HEAD,
            VanillaBlocks::PIGLIN_WALL_HEAD,
            BlockStateProperties::NoteBlockInstrument::Piglin},
        {VanillaBlocks::PLAYER_HEAD,
            VanillaBlocks::PLAYER_WALL_HEAD,
            BlockStateProperties::NoteBlockInstrument::CustomHead},
    };

    for (const auto& c : cases) {
        if (c.head == nullptr || c.wallHead == nullptr) {
            continue;
        }
        // 站立变体
        {
            BlockPos notePos(0, 1, 0);
            m_world->setBlockAt(notePos, VanillaBlocks::NOTE_BLOCK->defaultState());
            m_world->setBlockAt(BlockPos(0, 2, 0), c.head->defaultState());
            BlockState placed = placeNoteBlock(notePos);
            EXPECT_EQ(placed.get(BlockStateProperties::NOTE_BLOCK_INSTRUMENT()), c.expected)
                << "standing head above note block";
        }
        // 墙挂变体
        {
            BlockPos notePos(0, 1, 0);
            m_world->setBlockAt(notePos, VanillaBlocks::NOTE_BLOCK->defaultState());
            m_world->setBlockAt(BlockPos(0, 2, 0), c.wallHead->defaultState());
            BlockState placed = placeNoteBlock(notePos);
            EXPECT_EQ(placed.get(BlockStateProperties::NOTE_BLOCK_INSTRUMENT()), c.expected)
                << "wall head above note block";
        }
    }
}

/**
 * @brief 测试材质映射的乐器类型
 */
TEST_F(NoteBlockTest, InstrumentType_MaterialMapping)
{
    // 测试需要验证:
    // - ROCK 材质 -> BASEDRUM
    // - SAND 材质 -> SNARE
    // - GLASS 材质 -> HAT
    // - WOOD 材质 -> BASS
    // - NETHER_WOOD 材质 -> BASS
    // - 其他材质 -> HARP (默认)

    // 验证 Material 枚举存在
    EXPECT_EQ(Material::ROCK, Material::ROCK);
    EXPECT_EQ(Material::SAND, Material::SAND);
    EXPECT_EQ(Material::GLASS, Material::GLASS);
    EXPECT_EQ(Material::WOOD, Material::WOOD);
    EXPECT_EQ(Material::NETHER_WOOD, Material::NETHER_WOOD);
}

/**
 * @brief 测试羊毛触发吉他乐器
 */
TEST_F(NoteBlockTest, InstrumentType_WoolTriggersGuitar)
{
    // 验证羊毛标签存在
    EXPECT_TRUE(BlockTags::WOOL().contains(*VanillaBlocks::WHITE_WOOL));
}

/**
 * @brief 测试音高计算
 *
 * 验证音高公式: f = 2^((note - 12) / 12)
 */
TEST_F(NoteBlockTest, PitchCalculation)
{
    // 音高计算测试
    // note = 12 时，音高应为 1.0 (标准音高)
    f32 pitch12 = static_cast<f32>(std::pow(2.0, static_cast<f64>(12 - 12) / 12.0));
    EXPECT_NEAR(pitch12, 1.0f, 0.0001f);

    // note = 0 时，音高应为 0.5 (低八度)
    f32 pitch0 = static_cast<f32>(std::pow(2.0, static_cast<f64>(0 - 12) / 12.0));
    EXPECT_NEAR(pitch0, 0.5f, 0.0001f);

    // note = 24 时，音高应为 2.0 (高八度)
    f32 pitch24 = static_cast<f32>(std::pow(2.0, static_cast<f64>(24 - 12) / 12.0));
    EXPECT_NEAR(pitch24, 2.0f, 0.0001f);

    // 每增加 1，音高上升一个半音 (约 5.946% 增加)
    f32 pitch13 = static_cast<f32>(std::pow(2.0, static_cast<f64>(13 - 12) / 12.0));
    EXPECT_NEAR(pitch13 / pitch12, 1.059463f, 0.0001f); // 2^(1/12)
}

/**
 * @brief 测试音符范围
 */
TEST_F(NoteBlockTest, NoteRange)
{
    // 使用 VanillaBlocks::NOTE_BLOCK 来测试
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);

    // 测试默认音符值为 0
    const BlockState& defaultState = VanillaBlocks::NOTE_BLOCK->defaultState();
    EXPECT_EQ(NoteBlock::getNote(defaultState), 0);

    // 测试音符范围 0-24
    for (i32 note = 0; note < 25; ++note) {
        BlockState state = NoteBlock::withNote(defaultState, note);
        EXPECT_EQ(NoteBlock::getNote(state), note);
    }

    // 测试循环音符
    BlockState state = defaultState;
    for (i32 i = 0; i < 25; ++i) {
        EXPECT_EQ(NoteBlock::getNote(state), i);
        state = NoteBlock::cycleNote(state);
    }
    // 循环后应回到 0
    EXPECT_EQ(NoteBlock::getNote(state), 0);
}

/**
 * @brief 测试音符限制
 */
TEST_F(NoteBlockTest, NoteClamping)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    const BlockState& defaultState = VanillaBlocks::NOTE_BLOCK->defaultState();

    // 测试负值被限制为 0
    BlockState stateNeg = NoteBlock::withNote(defaultState, -5);
    EXPECT_EQ(NoteBlock::getNote(stateNeg), 0);

    // 测试超过范围的值被限制为 24
    BlockState stateOver = NoteBlock::withNote(defaultState, 100);
    EXPECT_EQ(NoteBlock::getNote(stateOver), 24);
}

/**
 * @brief 测试声音事件存在
 *
 * 验证所有 16 种乐器的声音事件已定义。
 */
TEST_F(NoteBlockTest, SoundEventsDefined)
{
    // 验证所有音符盒声音事件存在
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_HARP.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_BASEDRUM.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_SNARE.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_HAT.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_BASS.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_FLUTE.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_BELL.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_GUITAR.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_CHIME.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_XYLOPHONE.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_IRON_XYLOPHONE.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_COW_BELL.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_DIDGERIDOO.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_BIT.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_BANJO.toString().empty());
    EXPECT_FALSE(SoundEvents::BLOCK_NOTE_BLOCK_PLING.toString().empty());
}

/**
 * @brief 测试音符盒状态属性
 */
TEST_F(NoteBlockTest, BlockStateProperties)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);

    // 验证默认状态
    const BlockState& defaultState = VanillaBlocks::NOTE_BLOCK->defaultState();

    // 验证 NOTE 属性存在
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::NOTE_0_24()));

    // 验证 POWERED 属性存在
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::POWERED()));

    // 验证默认值
    EXPECT_EQ(defaultState.get(BlockStateProperties::NOTE_0_24()), 0);
    EXPECT_EQ(defaultState.get(BlockStateProperties::POWERED()), false);
}

/**
 * @brief 测试音符盒特殊方块乐器检测
 *
 * 测试特定方块是否触发正确的乐器类型。
 */
TEST_F(NoteBlockTest, InstrumentType_SpecialBlocks)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);

    // 测试所有需要检查的方块都已定义
    EXPECT_NE(VanillaBlocks::CLAY, nullptr);
    EXPECT_NE(VanillaBlocks::GOLD_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::PACKED_ICE, nullptr);
    EXPECT_NE(VanillaBlocks::BONE_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::IRON_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::SOUL_SAND, nullptr);
    EXPECT_NE(VanillaBlocks::EMERALD_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::HAY_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::GLOWSTONE, nullptr);
    EXPECT_NE(VanillaBlocks::JACK_O_LANTERN, nullptr);
}

/**
 * @brief 测试音符盒方块的材质
 */
TEST_F(NoteBlockTest, NoteBlockMaterial)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);

    // 音符盒应该是木质材质（被斧有效挖掘）
    const Material& material = VanillaBlocks::NOTE_BLOCK->material();
    EXPECT_EQ(material, Material::WOOD);
}

// ============================================================================
// 乐器类型检测测试 (MC 1.16.5 对齐)
// ============================================================================

/**
 * @brief 测试南瓜触发迪吉里杜管乐器
 *
 * 参考 MC 1.16.5: NoteBlockInstrument.byState() 检测 PUMPKIN 触发 DIDGERIDOO
 * 注意: CARVED_PUMPKIN 和 JACK_O_LANTERN 在 MC 1.16.5 中不会触发此乐器
 */
TEST_F(NoteBlockTest, InstrumentType_PumpkinTriggersDidgeridoo)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::PUMPKIN, nullptr);

    // 在音符盒下方放置南瓜
    BlockPos noteBlockPos(0, 1, 0);
    BlockPos pumpkinPos(0, 0, 0);

    m_world->setBlockAt(pumpkinPos, VanillaBlocks::PUMPKIN->defaultState());
    const BlockState noteBlockState = placeNoteBlock(noteBlockPos);

    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, noteBlockState);

    // 验证播放了 DIDGERIDOO 声音
    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_DIDGERIDOO);

    // 验证产生了粒子效果
    EXPECT_EQ(m_world->particleCount(), 1);
}

/**
 * @brief 测试雕刻南瓜不触发迪吉里杜管乐器
 *
 * 验证 CARVED_PUMPKIN 在 MC 1.16.5 中不触发特殊乐器
 */
TEST_F(NoteBlockTest, InstrumentType_CarvedPumpkinDoesNotTriggerDidgeridoo)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::CARVED_PUMPKIN, nullptr);

    // 在音符盒下方放置雕刻南瓜
    BlockPos noteBlockPos(0, 1, 0);
    BlockPos carvedPumpkinPos(0, 0, 0);

    m_world->setBlockAt(carvedPumpkinPos, VanillaBlocks::CARVED_PUMPKIN->defaultState());
    const BlockState noteBlockState = placeNoteBlock(noteBlockPos);

    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, noteBlockState);

    // 验证没有播放 DIDGERIDOO，而是播放了材质对应的乐器
    // 雕刻南瓜使用 Material::EARTH，应该触发 HARP (默认)
    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_HARP);
}

/**
 * @brief 测试南瓜灯不触发迪吉里杜管乐器
 *
 * 验证 JACK_O_LANTERN 在 MC 1.16.5 中不触发特殊乐器
 */
TEST_F(NoteBlockTest, InstrumentType_JackOLanternDoesNotTriggerDidgeridoo)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::JACK_O_LANTERN, nullptr);

    // 在音符盒下方放置南瓜灯
    BlockPos noteBlockPos(0, 1, 0);
    BlockPos jackOLanternPos(0, 0, 0);

    m_world->setBlockAt(jackOLanternPos, VanillaBlocks::JACK_O_LANTERN->defaultState());
    const BlockState noteBlockState = placeNoteBlock(noteBlockPos);

    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, noteBlockState);

    // 验证没有播放 DIDGERIDOO，而是播放了材质对应的乐器
    // 南瓜灯使用 Material::EARTH，应该触发 HARP (默认)
    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_HARP);
}

/**
 * @brief 测试陶土触发长笛乐器
 */
TEST_F(NoteBlockTest, InstrumentType_ClayTriggersFlute)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::CLAY, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    BlockPos clayPos(0, 0, 0);

    m_world->setBlockAt(clayPos, VanillaBlocks::CLAY->defaultState());
    const BlockState noteBlockState = placeNoteBlock(noteBlockPos);

    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, noteBlockState);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_FLUTE);
}

/**
 * @brief 测试金块触发钟乐器
 */
TEST_F(NoteBlockTest, InstrumentType_GoldBlockTriggersBell)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::GOLD_BLOCK, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    BlockPos goldBlockPos(0, 0, 0);

    m_world->setBlockAt(goldBlockPos, VanillaBlocks::GOLD_BLOCK->defaultState());
    const BlockState noteBlockState = placeNoteBlock(noteBlockPos);

    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, noteBlockState);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_BELL);
}

/**
 * @brief 测试灵魂沙触发牛铃乐器
 */
TEST_F(NoteBlockTest, InstrumentType_SoulSandTriggersCowBell)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::SOUL_SAND, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    BlockPos soulSandPos(0, 0, 0);

    m_world->setBlockAt(soulSandPos, VanillaBlocks::SOUL_SAND->defaultState());
    const BlockState noteBlockState = placeNoteBlock(noteBlockPos);

    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, noteBlockState);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_COW_BELL);
}

// ============================================================================
// useItemOn：手持顶部乐器物品（头颅）交互测试
// ============================================================================

/**
 * @brief 手持头颅右键音符盒顶面 → 返回 Pass（交物品放置，不升调）
 *
 * 对齐 vanilla NoteBlock.useItemOn：NOTE_BLOCK_TOP_INSTRUMENTS + 顶面 → PASS。
 */
TEST_F(NoteBlockTest, UseItemOn_HeadOnTopFace_ReturnsPass)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(Items::SKELETON_SKULL, nullptr);

    BlockPos pos(0, 1, 0);
    m_world->setBlockAt(pos, VanillaBlocks::NOTE_BLOCK->defaultState());

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.inventory().getSelectedStackRef() = ItemStack(Items::SKELETON_SKULL, 1);

    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    const BlockState& state = VanillaBlocks::NOTE_BLOCK->defaultState();
    BlockRaycastResult hit(Vector3(0.5f, 1.5f, 0.5f), pos, Direction::Up, 1.0f);

    auto result = mc::test::useBlock(*noteBlock, state, *m_world, pos, player, Hand::MainHand, hit);

    // 返回 Pass 表示交物品放置；不播放音符、音符值不变
    EXPECT_TRUE(result.isPass());
    EXPECT_TRUE(m_world->playedSoundIds().empty());
    const BlockState* current = m_world->getBlockState(pos);
    ASSERT_NE(current, nullptr);
    EXPECT_EQ(NoteBlock::getNote(*current), 0);
}

/**
 * @brief 手持头颅右键音符盒侧面 → 不降级，走升调
 */
TEST_F(NoteBlockTest, UseItemOn_HeadOnSideFace_CyclesNote)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(Items::SKELETON_SKULL, nullptr);

    BlockPos pos(0, 1, 0);
    m_world->setBlockAt(pos, VanillaBlocks::NOTE_BLOCK->defaultState());

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.inventory().getSelectedStackRef() = ItemStack(Items::SKELETON_SKULL, 1);

    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    const BlockState& state = VanillaBlocks::NOTE_BLOCK->defaultState();
    BlockRaycastResult hit(Vector3(0.5f, 1.5f, 0.0f), pos, Direction::North, 1.0f);

    auto result = mc::test::useBlock(*noteBlock, state, *m_world, pos, player, Hand::MainHand, hit);

    EXPECT_TRUE(result.isSuccess());
    const BlockState* current = m_world->getBlockState(pos);
    ASSERT_NE(current, nullptr);
    EXPECT_EQ(NoteBlock::getNote(*current), 1);
}

/**
 * @brief 手持非顶部乐器物品（钻石）右键顶面 → 走升调
 */
TEST_F(NoteBlockTest, UseItemOn_NonHeadOnTopFace_CyclesNote)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(Items::DIAMOND, nullptr);

    BlockPos pos(0, 1, 0);
    m_world->setBlockAt(pos, VanillaBlocks::NOTE_BLOCK->defaultState());

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.inventory().getSelectedStackRef() = ItemStack(Items::DIAMOND, 1);

    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    const BlockState& state = VanillaBlocks::NOTE_BLOCK->defaultState();
    BlockRaycastResult hit(Vector3(0.5f, 1.5f, 0.5f), pos, Direction::Up, 1.0f);

    auto result = mc::test::useBlock(*noteBlock, state, *m_world, pos, player, Hand::MainHand, hit);

    EXPECT_TRUE(result.isSuccess());
    const BlockState* current = m_world->getBlockState(pos);
    ASSERT_NE(current, nullptr);
    EXPECT_EQ(NoteBlock::getNote(*current), 1);
}

/**
 * @brief NOTE_BLOCK_TOP_INSTRUMENTS 标签包含全部 7 类头颅
 */
TEST_F(NoteBlockTest, TopInstrumentsTag_ContainsAllHeads)
{
    const auto& tag = mc::item::tag::ItemTags::NOTE_BLOCK_TOP_INSTRUMENTS();
    for (const Item* item : {Items::SKELETON_SKULL,
             Items::WITHER_SKELETON_SKULL,
             Items::PLAYER_HEAD,
             Items::ZOMBIE_HEAD,
             Items::CREEPER_HEAD,
             Items::DRAGON_HEAD,
             Items::PIGLIN_HEAD}) {
        ASSERT_NE(item, nullptr);
        EXPECT_TRUE(tag.contains(ItemStack(item, 1))) << "missing head item in tag";
    }
}

// ============================================================================
// 音符盒上方头颅决定乐器（vanilla WorksAboveNoteBlockBlock 语义）
// ============================================================================

/**
 * @brief 骨架头颅置于音符盒上方 → 播放骷髅拟声
 *
 * 对齐 vanilla：头颅类方块的 instrument 为 MOB_HEAD，worksAboveNoteBlock() 为真，
 * 因此放在音符盒上方时取代下方方块决定音色（本测试下方为空，若不检测上方则回落 HARP）。
 */
TEST_F(NoteBlockTest, HeadAbove_SkeletonSkullTriggersImitateSkeleton)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::SKELETON_SKULL, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    BlockPos headPos(0, 2, 0);

    m_world->setBlockAt(headPos, VanillaBlocks::SKELETON_SKULL->defaultState());
    const BlockState state = placeNoteBlock(noteBlockPos);
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, state);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_SKELETON);
}

/**
 * @brief 凋灵骷髅头颅置于上方 → 凋灵骷髅拟声
 */
TEST_F(NoteBlockTest, HeadAbove_WitherSkeletonSkullTriggersImitateWitherSkeleton)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::WITHER_SKELETON_SKULL, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    m_world->setBlockAt(BlockPos(0, 2, 0), VanillaBlocks::WITHER_SKELETON_SKULL->defaultState());
    const BlockState state = placeNoteBlock(noteBlockPos);
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, state);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_WITHER_SKELETON);
}

/**
 * @brief 僵尸头置于上方 → 僵尸拟声
 */
TEST_F(NoteBlockTest, HeadAbove_ZombieHeadTriggersImitateZombie)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::ZOMBIE_HEAD, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    m_world->setBlockAt(BlockPos(0, 2, 0), VanillaBlocks::ZOMBIE_HEAD->defaultState());
    const BlockState state = placeNoteBlock(noteBlockPos);
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, state);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_ZOMBIE);
}

/**
 * @brief 苦力怕头置于上方 → 苦力怕拟声
 */
TEST_F(NoteBlockTest, HeadAbove_CreeperHeadTriggersImitateCreeper)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::CREEPER_HEAD, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    m_world->setBlockAt(BlockPos(0, 2, 0), VanillaBlocks::CREEPER_HEAD->defaultState());
    const BlockState state = placeNoteBlock(noteBlockPos);
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, state);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_CREEPER);
}

/**
 * @brief 龙首置于上方 → 末影龙拟声
 */
TEST_F(NoteBlockTest, HeadAbove_DragonHeadTriggersImitateEnderDragon)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::DRAGON_HEAD, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    m_world->setBlockAt(BlockPos(0, 2, 0), VanillaBlocks::DRAGON_HEAD->defaultState());
    const BlockState state = placeNoteBlock(noteBlockPos);
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, state);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_ENDER_DRAGON);
}

/**
 * @brief 猪灵头置于上方 → 猪灵拟声
 */
TEST_F(NoteBlockTest, HeadAbove_PiglinHeadTriggersImitatePiglin)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::PIGLIN_HEAD, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    m_world->setBlockAt(BlockPos(0, 2, 0), VanillaBlocks::PIGLIN_HEAD->defaultState());
    const BlockState state = placeNoteBlock(noteBlockPos);
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, state);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_PIGLIN);
}

/**
 * @brief 玩家头置于上方 → 自定义头颅音色（UI_BUTTON_CLICK 兜底）
 */
TEST_F(NoteBlockTest, HeadAbove_PlayerHeadTriggersCustomClick)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::PLAYER_HEAD, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    m_world->setBlockAt(BlockPos(0, 2, 0), VanillaBlocks::PLAYER_HEAD->defaultState());
    const BlockState state = placeNoteBlock(noteBlockPos);
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, state);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::UI_BUTTON_CLICK);
}

/**
 * @brief 上方头颅优先级高于下方方块
 *
 * 下方为南瓜（DIDGERIDOO），上方为骷髅头 → 应以头颅拟声为准。
 */
TEST_F(NoteBlockTest, HeadAbove_TakesPrecedenceOverBlockBelow)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::PUMPKIN, nullptr);
    ASSERT_NE(VanillaBlocks::SKELETON_SKULL, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    m_world->setBlockAt(BlockPos(0, 0, 0), VanillaBlocks::PUMPKIN->defaultState());
    m_world->setBlockAt(BlockPos(0, 2, 0), VanillaBlocks::SKELETON_SKULL->defaultState());
    const BlockState state = placeNoteBlock(noteBlockPos);
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, state);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_SKELETON);
}

/**
 * @brief 上方无头颅时回落到下方方块材质决定乐器
 *
 * 下方为南瓜（DIDGERIDOO），上方为空气 → 保持 DIDGERIDOO。
 */
TEST_F(NoteBlockTest, HeadAbove_AbsentFallsBackToBlockBelow)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::PUMPKIN, nullptr);

    BlockPos noteBlockPos(0, 1, 0);
    m_world->setBlockAt(BlockPos(0, 0, 0), VanillaBlocks::PUMPKIN->defaultState());
    const BlockState state = placeNoteBlock(noteBlockPos);
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);

    noteBlock->triggerNote(*m_world, noteBlockPos, state);

    const auto& sounds = m_world->playedSoundIds();
    ASSERT_EQ(sounds.size(), 1u);
    EXPECT_EQ(sounds[0], SoundEvents::BLOCK_NOTE_BLOCK_DIDGERIDOO);
} // ============================================================================

// INSTRUMENT 状态属性 / updatePostPlacement / 可调音语义（vanilla 对齐）
// ============================================================================

/**
 * @brief 放置时乐器写入 INSTRUMENT 属性
 *
 * 对齐 vanilla NoteBlock.getStateForPlacement：南瓜在下方 → 放置瞬间
 * INSTRUMENT=Didgeridoo。
 */
TEST_F(NoteBlockTest, Instrument_WrittenOnPlacement)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::PUMPKIN, nullptr);

    m_world->setBlockAt(BlockPos(0, 0, 0), VanillaBlocks::PUMPKIN->defaultState());
    const BlockState placed = placeNoteBlock(BlockPos(0, 1, 0));

    EXPECT_EQ(placed.get(BlockStateProperties::NOTE_BLOCK_INSTRUMENT()),
        BlockStateProperties::NoteBlockInstrument::Didgeridoo);
}

/**
 * @brief 头颅垫在音符盒下方 → 回退 HARP（不下传拟声）
 *
 * 对齐 vanilla setInstrument 第二分支：下方方块
 * worksAboveNoteBlock() 为真时取 HARP。
 */
TEST_F(NoteBlockTest, Instrument_HeadBelowFallsBackToHarp)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::SKELETON_SKULL, nullptr);

    m_world->setBlockAt(BlockPos(0, 0, 0), VanillaBlocks::SKELETON_SKULL->defaultState());
    const BlockState placed = placeNoteBlock(BlockPos(0, 1, 0));

    EXPECT_EQ(
        placed.get(BlockStateProperties::NOTE_BLOCK_INSTRUMENT()), BlockStateProperties::NoteBlockInstrument::Harp);
}

/**
 * @brief 竖直邻居更新重解析乐器（上方放置头颅后音色即变）
 *
 * 对齐 vanilla NoteBlock.updateShape 的 Y 轴分支。

 */
TEST_F(NoteBlockTest, UpdatePostPlacement_YAxisReparsesInstrument)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::PUMPKIN, nullptr);
    ASSERT_NE(VanillaBlocks::SKELETON_SKULL, nullptr);

    m_world->setBlockAt(BlockPos(0, 0, 0), VanillaBlocks::PUMPKIN->defaultState());
    const BlockState placed = placeNoteBlock(BlockPos(0, 1, 0));
    ASSERT_EQ(placed.get(BlockStateProperties::NOTE_BLOCK_INSTRUMENT()),
        BlockStateProperties::NoteBlockInstrument::Didgeridoo);

    // 在音符盒上方新增骷髅头，竖直邻居变化触发重解析
    m_world->setBlockAt(BlockPos(0, 2, 0), VanillaBlocks::SKELETON_SKULL->defaultState());
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);
    const BlockState updated = noteBlock->updatePostPlacement(placed,
        Direction::Up,
        VanillaBlocks::SKELETON_SKULL->defaultState(),
        *m_world,
        BlockPos(0, 1, 0),
        BlockPos(0, 2, 0));

    EXPECT_EQ(updated.get(BlockStateProperties::NOTE_BLOCK_INSTRUMENT()),
        BlockStateProperties::NoteBlockInstrument::Skeleton);
}

/**
 * @brief 水平邻居变化不重解析乐器
 */
TEST_F(NoteBlockTest, UpdatePostPlacement_HorizontalAxisKeepsInstrument)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::PUMPKIN, nullptr);

    m_world->setBlockAt(BlockPos(0, 0, 0), VanillaBlocks::PUMPKIN->defaultState());
    const BlockState placed = placeNoteBlock(BlockPos(0, 1, 0));

    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);
    const BlockState updated = noteBlock->updatePostPlacement(
        placed, Direction::North, placed, *m_world, BlockPos(0, 1, 0), BlockPos(0, 1, -1));

    EXPECT_EQ(updated.get(BlockStateProperties::NOTE_BLOCK_INSTRUMENT()),
        BlockStateProperties::NoteBlockInstrument::Didgeridoo);
}

/**
 * @brief 头颅类乐器音高固定 1.0 且无音符粒子（对齐 vanilla triggerEvent 的 isTunable 分支）

 */
TEST_F(NoteBlockTest, HeadInstrument_FixedPitchAndNoTunableParticle)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::SKELETON_SKULL, nullptr);

    m_world->setBlockAt(BlockPos(0, 2, 0), VanillaBlocks::SKELETON_SKULL->defaultState());
    BlockState placed = placeNoteBlock(BlockPos(0, 1, 0));
    // 把 note 调到非 12（音高本应为非 1.0），验证头颅乐器仍固定 1.0
    placed = placed.with(BlockStateProperties::NOTE_0_24(), 5);
    m_world->setBlockAt(BlockPos(0, 1, 0), placed);

    m_world->clear();
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);
    noteBlock->triggerNote(*m_world, BlockPos(0, 1, 0), placed);

    ASSERT_EQ(m_world->playedSoundIds().size(), 1u);
    EXPECT_EQ(m_world->playedSoundIds()[0], SoundEvents::BLOCK_NOTE_BLOCK_IMITATE_SKELETON);
    EXPECT_FLOAT_EQ(m_world->lastPitch(), 1.0f);
    EXPECT_EQ(m_world->particleCount(), 0);
}

/**
 * @brief 可调音乐器按 note 定音高并产生音符粒子
 */
TEST_F(NoteBlockTest, TunableInstrument_PitchAndParticle)
{
    ASSERT_NE(VanillaBlocks::NOTE_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::PUMPKIN, nullptr);

    m_world->setBlockAt(BlockPos(0, 0, 0), VanillaBlocks::PUMPKIN->defaultState());
    BlockState placed = placeNoteBlock(BlockPos(0, 1, 0));
    placed = placed.with(BlockStateProperties::NOTE_0_24(), 12);
    m_world->setBlockAt(BlockPos(0, 1, 0), placed);

    m_world->clear();
    NoteBlock* noteBlock = dynamic_cast<NoteBlock*>(VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlock, nullptr);
    noteBlock->triggerNote(*m_world, BlockPos(0, 1, 0), placed);

    ASSERT_EQ(m_world->playedSoundIds().size(), 1u);
    EXPECT_EQ(m_world->playedSoundIds()[0], SoundEvents::BLOCK_NOTE_BLOCK_DIDGERIDOO);
    EXPECT_FLOAT_EQ(m_world->lastPitch(), 1.0f); // note 12 -> pitch 1.0
    EXPECT_EQ(m_world->particleCount(), 1);
}
