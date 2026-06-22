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

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockSoundType.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;

namespace {

// ============================================================================
// 测试用声音捕获世界
// ============================================================================

class StepSoundTestWorld final : public test::BaseTestWorld {
public:
    struct SoundRecord {
        ResourceLocation soundEventId;
        sound::SoundCategory category;
        f32 volume;
        f32 pitch;
    };

    void setBelowBlock(const BlockState* state) { m_belowBlock = state; }
    void setAboveBlock(const BlockState* state) { m_aboveBlock = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        // pos (y=0): 脚下方块；pos.y+1 (y=1): 脚上方方块
        if (x == 0 && z == 0) {
            if (y == 0) {
                return m_belowBlock;
            }
            if (y == 1) {
                return m_aboveBlock;
            }
        }
        return nullptr;
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& /*position*/,
        f32 volume,
        f32 pitch) override
    {
        m_sounds.push_back(SoundRecord{soundEventId, category, volume, pitch});
    }

    [[nodiscard]] const std::vector<SoundRecord>& sounds() const { return m_sounds; }
    void clearSounds() { m_sounds.clear(); }
    [[nodiscard]] size_t soundCount() const { return m_sounds.size(); }

    // TickManager 接口（桩实现）
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("StepSoundTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("StepSoundTestWorld::tickManager not implemented");
    }

private:
    const BlockState* m_belowBlock = nullptr;
    const BlockState* m_aboveBlock = nullptr;
    std::vector<SoundRecord> m_sounds;
};

// ============================================================================
// 测试实体 - 暴露 playStepSound 等受保护方法
// ============================================================================

class TestStepSoundEntity : public Entity {
public:
    TestStepSoundEntity(IWorld* world = nullptr)
        : Entity(EntityId(1), world)
    {}

    // 暴露 playStepSound 用于测试
    using Entity::getPrimaryStepSoundBlockPos;
    using Entity::playCombinationStepSounds;
    using Entity::playMuffledStepSound;
    using Entity::playStepSound;
    using Entity::shouldPlayAmethystStepSound;
};

} // namespace

// ============================================================================
// 测试夹具
// ============================================================================

class StepSoundTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }

    StepSoundTestWorld world;
    TestStepSoundEntity entity{&world};
};

// ============================================================================
// getPrimaryStepSoundBlockPos 测试
// ============================================================================

TEST_F(StepSoundTest, GetPrimaryStepSoundBlockPos_NormalBlock_ReturnsSamePos)
{
    // 石头在地面上，上方是空气 → 返回脚下方块位置
    world.setBelowBlock(&VanillaBlocks::STONE->defaultState());
    world.setAboveBlock(nullptr); // 空气

    BlockPos belowPos(0, 0, 0);
    BlockPos result = entity.getPrimaryStepSoundBlockPos(belowPos);
    EXPECT_EQ(result, belowPos);
}

TEST_F(StepSoundTest, GetPrimaryStepSoundBlockPos_CarpetAbove_ReturnsAbovePos)
{
    // 石头在地面上，上方是白色地毯（COMBINATION_STEP_SOUND_BLOCKS）→ 返回上方位置
    world.setBelowBlock(&VanillaBlocks::STONE->defaultState());
    world.setAboveBlock(&VanillaBlocks::WHITE_CARPET->defaultState());

    BlockPos belowPos(0, 0, 0);
    BlockPos abovePos(0, 1, 0);
    BlockPos result = entity.getPrimaryStepSoundBlockPos(belowPos);
    EXPECT_EQ(result, abovePos);
}

TEST_F(StepSoundTest, GetPrimaryStepSoundBlockPos_PowderSnowAbove_ReturnsAbovePos)
{
    // 石头在地面上，上方是细雪（INSIDE_STEP_SOUND_BLOCKS）→ 返回上方位置
    world.setBelowBlock(&VanillaBlocks::STONE->defaultState());
    world.setAboveBlock(&VanillaBlocks::POWDER_SNOW->defaultState());

    BlockPos belowPos(0, 0, 0);
    BlockPos abovePos(0, 1, 0);
    BlockPos result = entity.getPrimaryStepSoundBlockPos(belowPos);
    EXPECT_EQ(result, abovePos);
}

TEST_F(StepSoundTest, GetPrimaryStepSoundBlockPos_SnowAbove_ReturnsAbovePos)
{
    // 石头在地面上，上方是雪层（COMBINATION_STEP_SOUND_BLOCKS）→ 返回上方位置
    world.setBelowBlock(&VanillaBlocks::STONE->defaultState());
    world.setAboveBlock(&VanillaBlocks::SNOW->defaultState());

    BlockPos belowPos(0, 0, 0);
    BlockPos abovePos(0, 1, 0);
    BlockPos result = entity.getPrimaryStepSoundBlockPos(belowPos);
    EXPECT_EQ(result, abovePos);
}

// ============================================================================
// playStepSound 正常步声测试
// ============================================================================

TEST_F(StepSoundTest, NormalBlock_PlaysOwnStepSound)
{
    // 站在石头上，上方是空气 → 播放石头步声
    world.setBelowBlock(&VanillaBlocks::STONE->defaultState());
    world.setAboveBlock(nullptr);

    BlockPos belowPos(0, 0, 0);
    entity.playStepSound(belowPos, &VanillaBlocks::STONE->defaultState());

    // 应该只播放一个声音（石头步声）
    ASSERT_EQ(world.soundCount(), 1u);
    const auto& sound = world.sounds()[0];
    // 石头步声事件应该包含 "stone" 和 "step"
    EXPECT_NE(sound.soundEventId.toString().find("step"), std::string::npos);
    // 音量应该 > 0
    EXPECT_GT(sound.volume, 0.0f);
}

// ============================================================================
// COMBINATION_STEP_SOUND_BLOCKS 步声测试
// ============================================================================

TEST_F(StepSoundTest, CombinationStepSound_CarpetOnStone_PlaysTwoSounds)
{
    // 白色地毯在石头上方 → 播放两个声音：地毯正常步声 + 石头沉闷步声
    const BlockState* carpetState = &VanillaBlocks::WHITE_CARPET->defaultState();
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();

    world.setBelowBlock(stoneState);
    world.setAboveBlock(carpetState);

    BlockPos belowPos(0, 0, 0);
    entity.playStepSound(belowPos, stoneState);

    // 应该播放两个声音：上方正常步声 + 下方沉闷步声
    ASSERT_EQ(world.soundCount(), 2u);

    // 第一个声音：上方方块（地毯）正常步声
    const auto& aboveSound = world.sounds()[0];
    EXPECT_GT(aboveSound.volume, 0.0f);

    // 第二个声音：下方方块（石头）沉闷步声
    const auto& belowSound = world.sounds()[1];
    EXPECT_GT(belowSound.volume, 0.0f);
    // 沉闷步声音量应该更低（0.05x vs 0.15x）
    EXPECT_LT(belowSound.volume, aboveSound.volume);
}

TEST_F(StepSoundTest, CombinationStepSound_SnowOnStone_PlaysTwoSounds)
{
    // 雪层在石头上方 → 播放两个声音
    const BlockState* snowState = &VanillaBlocks::SNOW->defaultState();
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();

    world.setBelowBlock(stoneState);
    world.setAboveBlock(snowState);

    BlockPos belowPos(0, 0, 0);
    entity.playStepSound(belowPos, stoneState);

    ASSERT_EQ(world.soundCount(), 2u);
    // 第二个声音是沉闷步声，音量应该更低
    EXPECT_LT(world.sounds()[1].volume, world.sounds()[0].volume);
}

TEST_F(StepSoundTest, CombinationStepSound_MossCarpetOnDirt_PlaysTwoSounds)
{
    // 苔藓地毯在泥土上方 → 播放两个声音
    const BlockState* mossCarpetState = &VanillaBlocks::MOSS_CARPET->defaultState();
    const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();

    world.setBelowBlock(dirtState);
    world.setAboveBlock(mossCarpetState);

    BlockPos belowPos(0, 0, 0);
    entity.playStepSound(belowPos, dirtState);

    ASSERT_EQ(world.soundCount(), 2u);
    EXPECT_LT(world.sounds()[1].volume, world.sounds()[0].volume);
}

// ============================================================================
// INSIDE_STEP_SOUND_BLOCKS 步声测试
// ============================================================================

TEST_F(StepSoundTest, InsideStepSound_PowderSnowOnStone_PlaysOnlyAboveSound)
{
    // 细雪在石头上方 → 只播放细雪步声（替代石头步声）
    const BlockState* powderSnowState = &VanillaBlocks::POWDER_SNOW->defaultState();
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();

    world.setBelowBlock(stoneState);
    world.setAboveBlock(powderSnowState);

    BlockPos belowPos(0, 0, 0);
    entity.playStepSound(belowPos, stoneState);

    // INSIDE 只播放一个声音（上方方块的步声）
    ASSERT_EQ(world.soundCount(), 1u);
    const auto& sound = world.sounds()[0];
    EXPECT_GT(sound.volume, 0.0f);
}

TEST_F(StepSoundTest, InsideStepSound_SculkVeinOnStone_PlaysOnlyAboveSound)
{
    // 幽匿脉络在石头上方 → 只播放幽匿脉络步声
    if (!VanillaBlocks::SCULK_VEIN) {
        GTEST_SKIP() << "SCULK_VEIN not registered";
    }

    const BlockState* sculkVeinState = &VanillaBlocks::SCULK_VEIN->defaultState();
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();

    world.setBelowBlock(stoneState);
    world.setAboveBlock(sculkVeinState);

    BlockPos belowPos(0, 0, 0);
    entity.playStepSound(belowPos, stoneState);

    ASSERT_EQ(world.soundCount(), 1u);
    EXPECT_GT(world.sounds()[0].volume, 0.0f);
}

// ============================================================================
// 紫水晶共振铃声测试
// ============================================================================

TEST_F(StepSoundTest, AmethystChime_OnCrystalSoundBlock_PlaysChime)
{
    // 站在紫水晶块上，上方是空气 → 播放步声 + 紫水晶铃声
    const BlockState* amethystState = &VanillaBlocks::AMETHYST_BLOCK->defaultState();

    world.setBelowBlock(amethystState);
    world.setAboveBlock(nullptr);

    BlockPos belowPos(0, 0, 0);
    entity.playStepSound(belowPos, amethystState);

    // 应该播放两个声音：步声 + 紫水晶铃声
    ASSERT_EQ(world.soundCount(), 2u);

    // 第一个是正常步声
    EXPECT_NE(world.sounds()[0].soundEventId.toString().find("step"), std::string::npos);

    // 第二个是紫水晶铃声
    EXPECT_NE(world.sounds()[1].soundEventId.toString().find("chime"), std::string::npos);
}

TEST_F(StepSoundTest, AmethystChime_CarpetOnAmethyst_PlaysCombinationAndChime)
{
    // 白色地毯在紫水晶块上方 → 播放三个声音：
    // 1. 地毯正常步声（COMBINATION 第一个声音）
    // 2. 紫水晶沉闷步声（COMBINATION 第二个声音）
    // 3. 紫水晶铃声（CRYSTAL_SOUND_BLOCKS 检查脚下方块）
    const BlockState* carpetState = &VanillaBlocks::WHITE_CARPET->defaultState();
    const BlockState* amethystState = &VanillaBlocks::AMETHYST_BLOCK->defaultState();

    world.setBelowBlock(amethystState);
    world.setAboveBlock(carpetState);

    BlockPos belowPos(0, 0, 0);
    entity.playStepSound(belowPos, amethystState);

    // 三个声音：地毯步声 + 紫水晶沉闷步声 + 紫水晶铃声
    ASSERT_EQ(world.soundCount(), 3u);

    // 第二个声音是紫水晶沉闷步声（音量低）
    EXPECT_GT(world.sounds()[1].volume, 0.0f);

    // 第三个声音是紫水晶铃声
    EXPECT_NE(world.sounds()[2].soundEventId.toString().find("chime"), std::string::npos);
}

TEST_F(StepSoundTest, AmethystChime_InsideBlockOnAmethyst_PlaysAboveSoundAndChime)
{
    // 细雪在紫水晶块上方 → 播放两个声音：
    // 1. 细雪步声（INSIDE 只播放上方方块步声）
    // 2. 紫水晶铃声（检查脚下方块=紫水晶）
    const BlockState* powderSnowState = &VanillaBlocks::POWDER_SNOW->defaultState();
    const BlockState* amethystState = &VanillaBlocks::AMETHYST_BLOCK->defaultState();

    world.setBelowBlock(amethystState);
    world.setAboveBlock(powderSnowState);

    BlockPos belowPos(0, 0, 0);
    entity.playStepSound(belowPos, amethystState);

    // 两个声音：细雪步声 + 紫水晶铃声
    ASSERT_EQ(world.soundCount(), 2u);

    // 第二个声音是紫水晶铃声
    EXPECT_NE(world.sounds()[1].soundEventId.toString().find("chime"), std::string::npos);
}

TEST_F(StepSoundTest, NoAmethystChime_OnStoneWithCarpet)
{
    // 白色地毯在石头上方 → 播放两个声音（组合步声），无紫水晶铃声
    const BlockState* carpetState = &VanillaBlocks::WHITE_CARPET->defaultState();
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();

    world.setBelowBlock(stoneState);
    world.setAboveBlock(carpetState);

    BlockPos belowPos(0, 0, 0);
    entity.playStepSound(belowPos, stoneState);

    // 只有组合步声（2个声音），无紫水晶铃声
    ASSERT_EQ(world.soundCount(), 2u);
    // 没有任何声音包含 "chime"
    for (const auto& sound : world.sounds()) {
        EXPECT_EQ(sound.soundEventId.toString().find("chime"), std::string::npos);
    }
}

// ============================================================================
// playMuffledStepSound 测试
// ============================================================================

TEST_F(StepSoundTest, MuffledStepSound_LowVolumeAndPitch)
{
    // 沉闷步声应该以 0.05x 音量和 0.8x 音调播放
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();

    entity.playMuffledStepSound(*stoneState);

    ASSERT_EQ(world.soundCount(), 1u);
    const auto& sound = world.sounds()[0];
    // 石头声音类型默认音量 * 0.05
    f32 expectedVolume = stoneState->getSoundType().getVolume() * 0.05f;
    f32 expectedPitch = stoneState->getSoundType().getPitch() * 0.8f;
    EXPECT_FLOAT_EQ(sound.volume, expectedVolume);
    EXPECT_FLOAT_EQ(sound.pitch, expectedPitch);
}

// ============================================================================
// shouldPlayAmethystStepSound 测试
// ============================================================================

TEST_F(StepSoundTest, ShouldPlayAmethystStepSound_OnAmethystBlock_ReturnsTrue)
{
    const BlockState* amethystState = &VanillaBlocks::AMETHYST_BLOCK->defaultState();
    EXPECT_TRUE(entity.shouldPlayAmethystStepSound(*amethystState));
}

TEST_F(StepSoundTest, ShouldPlayAmethystStepSound_OnBuddingAmethyst_ReturnsTrue)
{
    if (!VanillaBlocks::BUDDING_AMETHYST) {
        GTEST_SKIP() << "BUDDING_AMETHYST not registered";
    }
    const BlockState* buddingState = &VanillaBlocks::BUDDING_AMETHYST->defaultState();
    EXPECT_TRUE(entity.shouldPlayAmethystStepSound(*buddingState));
}

TEST_F(StepSoundTest, ShouldPlayAmethystStepSound_OnStone_ReturnsFalse)
{
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(entity.shouldPlayAmethystStepSound(*stoneState));
}

TEST_F(StepSoundTest, ShouldPlayAmethystStepSound_OnCarpet_ReturnsFalse)
{
    const BlockState* carpetState = &VanillaBlocks::WHITE_CARPET->defaultState();
    EXPECT_FALSE(entity.shouldPlayAmethystStepSound(*carpetState));
}

// ============================================================================
// 空世界/空方块 边界测试
// ============================================================================

TEST_F(StepSoundTest, PlayStepSound_NullBlockState_DoesNothing)
{
    // 传入 nullptr blockState 不应崩溃
    BlockPos pos(0, 0, 0);
    entity.playStepSound(pos, nullptr);
    EXPECT_EQ(world.soundCount(), 0u);
}

TEST_F(StepSoundTest, PlayStepSound_AirAbove_NormalStepSound)
{
    // 上方是空气 → 正常步声（与 NormalBlock 测试类似，但显式验证）
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world.setBelowBlock(stoneState);
    world.setAboveBlock(nullptr);

    BlockPos belowPos(0, 0, 0);
    entity.playStepSound(belowPos, stoneState);

    // 只有步声，无紫水晶铃声
    ASSERT_EQ(world.soundCount(), 1u);
}
