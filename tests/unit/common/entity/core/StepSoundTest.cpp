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

#include <cmath>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockSoundType.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;

namespace {

// ============================================================================
// 测试用声音捕获世界
// ============================================================================

class StepSoundTestWorld final : public mc::test::BaseTestWorld {
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
        : Entity(EntityInstanceId(1), world, mc::test::testEcsRegistry())
    {}

    // 暴露 playStepSound 等受保护方法
    using Entity::getPrimaryStepSoundBlockPos;
    using Entity::playAmethystStepSound;
    using Entity::playCombinationStepSounds;
    using Entity::playMuffledStepSound;
    using Entity::playStepSound;
    using Entity::shouldPlayAmethystStepSound;

    // 暴露紫水晶步声相关字段，用于测试强度累积和冷却
    using Entity::m_crystalSoundIntensity;
    using Entity::m_lastCrystalSoundPlayTick;
    using Entity::m_ticksExisted;
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

    // 满足 20 tick 冷却条件
    entity.m_ticksExisted = 20;

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

    // 满足 20 tick 冷却条件
    entity.m_ticksExisted = 20;

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

    // 满足 20 tick 冷却条件
    entity.m_ticksExisted = 20;

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
    // 需要满足冷却条件：ticksExisted >= lastCrystalSoundPlayTick + 20
    entity.m_ticksExisted = 20;
    EXPECT_TRUE(entity.shouldPlayAmethystStepSound(*amethystState));
}

TEST_F(StepSoundTest, ShouldPlayAmethystStepSound_OnBuddingAmethyst_ReturnsTrue)
{
    if (!VanillaBlocks::BUDDING_AMETHYST) {
        GTEST_SKIP() << "BUDDING_AMETHYST not registered";
    }
    const BlockState* buddingState = &VanillaBlocks::BUDDING_AMETHYST->defaultState();
    entity.m_ticksExisted = 20;
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

// ============================================================================
// shouldPlayAmethystStepSound 冷却测试
// ============================================================================

TEST_F(StepSoundTest, ShouldPlayAmethystStepSound_CooldownNotElapsed_ReturnsFalse)
{
    // 初始状态：lastCrystalSoundPlayTick = 0, ticksExisted = 0
    // ticksExisted(0) >= lastCrystalSoundPlayTick(0) + 20 不成立
    const BlockState* amethystState = &VanillaBlocks::AMETHYST_BLOCK->defaultState();

    // 模拟刚播放过铃声（设置 lastCrystalSoundPlayTick）
    entity.m_lastCrystalSoundPlayTick = 0;
    entity.m_ticksExisted = 10; // 10 < 0 + 20
    EXPECT_FALSE(entity.shouldPlayAmethystStepSound(*amethystState));
}

TEST_F(StepSoundTest, ShouldPlayAmethystStepSound_CooldownExactlyElapsed_ReturnsTrue)
{
    const BlockState* amethystState = &VanillaBlocks::AMETHYST_BLOCK->defaultState();

    entity.m_lastCrystalSoundPlayTick = 0;
    entity.m_ticksExisted = 20; // 20 >= 0 + 20，冷却刚好结束
    EXPECT_TRUE(entity.shouldPlayAmethystStepSound(*amethystState));
}

TEST_F(StepSoundTest, ShouldPlayAmethystStepSound_CooldownAfterPlayback_ReturnsFalse)
{
    const BlockState* amethystState = &VanillaBlocks::AMETHYST_BLOCK->defaultState();

    // 模拟在 tick 100 播放了铃声
    entity.m_lastCrystalSoundPlayTick = 100;
    entity.m_ticksExisted = 110; // 110 < 100 + 20
    EXPECT_FALSE(entity.shouldPlayAmethystStepSound(*amethystState));

    entity.m_ticksExisted = 119; // 119 < 100 + 20
    EXPECT_FALSE(entity.shouldPlayAmethystStepSound(*amethystState));

    entity.m_ticksExisted = 120; // 120 >= 100 + 20，冷却结束
    EXPECT_TRUE(entity.shouldPlayAmethystStepSound(*amethystState));
}

TEST_F(StepSoundTest, ShouldPlayAmethystStepSound_NonCrystalBlock_IgnoresCooldown)
{
    // 即使冷却已过，非紫水晶方块也不应触发
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();

    entity.m_lastCrystalSoundPlayTick = 0;
    entity.m_ticksExisted = 100; // 冷却早已结束
    EXPECT_FALSE(entity.shouldPlayAmethystStepSound(*stoneState));
}

// ============================================================================
// playAmethystStepSound 强度累积测试
// ============================================================================

TEST_F(StepSoundTest, PlayAmethystStepSound_InitialIntensity_StartsAtZeroPlus007)
{
    // 初次播放：intensity = 0 * 0.997^0 = 0, 然后 min(1.0, 0 + 0.07) = 0.07
    entity.m_crystalSoundIntensity = 0.0f;
    entity.m_lastCrystalSoundPlayTick = 0;
    entity.m_ticksExisted = 20; // 满足冷却条件

    entity.playAmethystStepSound();

    // 验证强度增加到 0.07
    EXPECT_FLOAT_EQ(entity.m_crystalSoundIntensity, 0.07f);
    // 验证 lastCrystalSoundPlayTick 更新
    EXPECT_EQ(entity.m_lastCrystalSoundPlayTick, 20);
    // 验证播放了声音
    ASSERT_EQ(world.soundCount(), 1u);
    // 音量 = 0.1 + 0.07 * 1.2 = 0.184
    EXPECT_FLOAT_EQ(world.sounds()[0].volume, 0.1f + 0.07f * 1.2f);
}

TEST_F(StepSoundTest, PlayAmethystStepSound_RepeatedPlayback_AccumulatesIntensity)
{
    // 连续播放：每次增加 0.07，强度累积
    const BlockState* amethystState = &VanillaBlocks::AMETHYST_BLOCK->defaultState();
    world.setBelowBlock(amethystState);
    world.setAboveBlock(nullptr);

    // 第一次播放（tick 20）
    entity.m_ticksExisted = 20;
    entity.playStepSound(BlockPos(0, 0, 0), amethystState);
    world.clearSounds();
    f32 intensityAfterFirst = entity.m_crystalSoundIntensity;
    EXPECT_FLOAT_EQ(intensityAfterFirst, 0.07f);

    // 第二次播放（tick 40，间隔 20 tick）
    entity.m_ticksExisted = 40;
    entity.playStepSound(BlockPos(0, 0, 0), amethystState);
    world.clearSounds();

    // 衰减：0.07 * 0.997^20 ≈ 0.07 * 0.9418 ≈ 0.0659
    // 累加：0.0659 + 0.07 ≈ 0.1359
    f32 decayFactor = std::pow(0.997f, 20.0f);
    f32 expectedDecayed = 0.07f * decayFactor;
    f32 expectedIntensity = expectedDecayed + 0.07f;
    EXPECT_NEAR(entity.m_crystalSoundIntensity, expectedIntensity, 0.001f);
    EXPECT_GT(entity.m_crystalSoundIntensity, intensityAfterFirst);
}

TEST_F(StepSoundTest, PlayAmethystStepSound_IntensityClampedToOne)
{
    // 多次播放后强度应趋近 1.0 但不超过 1.0
    entity.m_lastCrystalSoundPlayTick = 0;

    // 反复播放直到强度饱和
    for (i32 tick = 20; tick <= 2000; tick += 20) {
        entity.m_ticksExisted = tick;
        entity.playAmethystStepSound();
        if (entity.m_crystalSoundIntensity >= 1.0f) {
            break;
        }
    }

    // 最终强度应该恰好是 1.0（被钳制）
    EXPECT_FLOAT_EQ(entity.m_crystalSoundIntensity, 1.0f);

    // 验证继续播放后强度不会超过 1.0
    i32 lastTick = entity.m_lastCrystalSoundPlayTick;
    entity.m_ticksExisted = lastTick + 20;
    entity.playAmethystStepSound();
    EXPECT_FLOAT_EQ(entity.m_crystalSoundIntensity, 1.0f);
}

TEST_F(StepSoundTest, PlayAmethystStepSound_DecayOverLongGap)
{
    // 长时间不播放后，强度应该衰减到接近零
    entity.m_crystalSoundIntensity = 0.8f;
    entity.m_lastCrystalSoundPlayTick = 0;
    entity.m_ticksExisted = 1000; // 1000 tick 后

    entity.playAmethystStepSound();

    // 0.8 * 0.997^1000 ≈ 0.8 * 0.0498 ≈ 0.0398, +0.07 ≈ 0.1098
    f32 decayFactor = std::pow(0.997f, 1000.0f);
    f32 expectedDecayed = 0.8f * decayFactor;
    f32 expectedIntensity = std::min(1.0f, expectedDecayed + 0.07f);
    EXPECT_NEAR(entity.m_crystalSoundIntensity, expectedIntensity, 0.01f);
    // 强度应该明显低于初始的 0.8
    EXPECT_LT(entity.m_crystalSoundIntensity, 0.2f);
}

TEST_F(StepSoundTest, PlayAmethystStepSound_VolumeAndPitchFormula)
{
    // 验证音量和音调公式：volume = 0.1 + intensity * 1.2, pitch = 0.5 + intensity * random * 1.2
    entity.m_crystalSoundIntensity = 0.5f;
    entity.m_lastCrystalSoundPlayTick = 0;
    entity.m_ticksExisted = 20;

    entity.playAmethystStepSound();

    ASSERT_EQ(world.soundCount(), 1u);

    // 衰减：0.5 * 0.997^20 ≈ 0.5 * 0.9418 ≈ 0.4709
    f32 expectedDecayed = 0.5f * std::pow(0.997f, 20.0f);
    f32 expectedIntensity = std::min(1.0f, expectedDecayed + 0.07f);
    f32 expectedVolume = 0.1f + expectedIntensity * 1.2f;

    // 音量应该是确定性的，可以直接验证
    EXPECT_NEAR(world.sounds()[0].volume, expectedVolume, 0.01f);

    // 音调 = 0.5 + intensity * random * 1.2，random 是 [0,1) 范围
    // 验证音调在合理范围内
    EXPECT_GE(world.sounds()[0].pitch, 0.5f);               // 下界
    EXPECT_LE(world.sounds()[0].pitch, 0.5f + 1.0f * 1.2f); // 上界
}

TEST_F(StepSoundTest, PlayAmethystStepSound_UpdatesLastPlayTick)
{
    entity.m_lastCrystalSoundPlayTick = 50;
    entity.m_ticksExisted = 70;

    entity.playAmethystStepSound();

    EXPECT_EQ(entity.m_lastCrystalSoundPlayTick, 70);
}

TEST_F(StepSoundTest, PlayAmethystStepSound_SoundEventId)
{
    entity.m_ticksExisted = 20;

    entity.playAmethystStepSound();

    ASSERT_EQ(world.soundCount(), 1u);
    // 验证播放的是紫水晶铃声事件
    EXPECT_EQ(world.sounds()[0].soundEventId, SoundEvents::BLOCK_AMETHYST_BLOCK_CHIME);
}

// ============================================================================
// playStepSound 中紫水晶铃声冷却集成测试
// ============================================================================

TEST_F(StepSoundTest, AmethystChime_RespectsCooldown)
{
    // 第一次播放应该触发铃声
    const BlockState* amethystState = &VanillaBlocks::AMETHYST_BLOCK->defaultState();
    world.setBelowBlock(amethystState);
    world.setAboveBlock(nullptr);

    entity.m_ticksExisted = 20;
    entity.playStepSound(BlockPos(0, 0, 0), amethystState);
    // 应该有 2 个声音：步声 + 铃声
    EXPECT_EQ(world.soundCount(), 2u);

    // 5 tick 后（冷却未过），不应再触发铃声
    world.clearSounds();
    entity.m_ticksExisted = 25;
    entity.playStepSound(BlockPos(0, 0, 0), amethystState);
    // 应该只有 1 个声音：步声（铃声被冷却抑制）
    EXPECT_EQ(world.soundCount(), 1u);
    EXPECT_EQ(world.sounds()[0].soundEventId.toString().find("chime"), std::string::npos);

    // 20 tick 后（冷却结束），应再次触发铃声
    world.clearSounds();
    entity.m_ticksExisted = 40;
    entity.playStepSound(BlockPos(0, 0, 0), amethystState);
    // 应该有 2 个声音：步声 + 铃声
    EXPECT_EQ(world.soundCount(), 2u);
    EXPECT_NE(world.sounds()[1].soundEventId.toString().find("chime"), std::string::npos);
}

TEST_F(StepSoundTest, AmethystChime_IntensityAccumulatesWithRepeatedSteps)
{
    // 多次踩紫水晶块，强度应逐步增加（由于 0.997^20 衰减，累积较慢）
    const BlockState* amethystState = &VanillaBlocks::AMETHYST_BLOCK->defaultState();
    world.setBelowBlock(amethystState);
    world.setAboveBlock(nullptr);

    f32 previousIntensity = 0.0f;

    // 播放 50 次（每 20 tick 一次），足够让强度接近 1.0
    for (u32 tick = 20; tick <= 1000; tick += 20) {
        entity.m_ticksExisted = tick;
        entity.playStepSound(BlockPos(0, 0, 0), amethystState);
        // 强度应该单调递增（因为衰减不足以抵消 +0.07 的累加）
        EXPECT_GE(entity.m_crystalSoundIntensity, previousIntensity)
            << "Intensity should not decrease at tick " << tick;
        previousIntensity = entity.m_crystalSoundIntensity;
        world.clearSounds();
    }

    // 50 次播放后强度应接近 1.0
    EXPECT_NEAR(entity.m_crystalSoundIntensity, 1.0f, 0.05f);
}

TEST_F(StepSoundTest, AmethystChime_IntensityDecaysAfterLongGap)
{
    // 先累积一定强度
    const BlockState* amethystState = &VanillaBlocks::AMETHYST_BLOCK->defaultState();
    world.setBelowBlock(amethystState);
    world.setAboveBlock(nullptr);

    entity.m_ticksExisted = 20;
    entity.playStepSound(BlockPos(0, 0, 0), amethystState);
    f32 intensityAfterFirst = entity.m_crystalSoundIntensity;
    world.clearSounds();

    // 长时间不踩，强度衰减
    entity.m_ticksExisted = 1000;
    entity.playStepSound(BlockPos(0, 0, 0), amethystState);
    // 衰减后的强度（加上 0.07）应低于之前的强度加 0.07
    // 但由于衰减，最终强度可能低于或高于之前的强度
    // 关键验证：衰减确实发生了
    f32 expectedDecayed = intensityAfterFirst * std::pow(0.997f, 1000.0f - 20.0f);
    f32 expectedAfterDecay = std::min(1.0f, expectedDecayed + 0.07f);
    EXPECT_NEAR(entity.m_crystalSoundIntensity, expectedAfterDecay, 0.01f);
}
