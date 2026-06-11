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

#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"

using namespace mc::entity::combat;
using namespace mc;

// ============================================================================
// DifficultyInstance 简化构造函数测试
// ============================================================================

TEST(DifficultyInstanceTest, SimplifiedConstructor_Peaceful)
{
    DifficultyInstance inst(Difficulty::Peaceful);
    EXPECT_EQ(inst.getDifficulty(), Difficulty::Peaceful);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 0.0f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

TEST(DifficultyInstanceTest, SimplifiedConstructor_Easy)
{
    DifficultyInstance inst(Difficulty::Easy);
    EXPECT_EQ(inst.getDifficulty(), Difficulty::Easy);
    // effective = base * id = 0.75 * 1 = 0.75
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 0.75f);
    // 0.75 < 2.0 -> special = 0.0
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

TEST(DifficultyInstanceTest, SimplifiedConstructor_Normal)
{
    DifficultyInstance inst(Difficulty::Normal);
    EXPECT_EQ(inst.getDifficulty(), Difficulty::Normal);
    // effective = base * id = 1.0 * 2 = 2.0
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 2.0f);
    // 2.0 不小于 2.0，但 (2.0 - 2.0) / 2.0 = 0.0
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

TEST(DifficultyInstanceTest, SimplifiedConstructor_Hard)
{
    DifficultyInstance inst(Difficulty::Hard);
    EXPECT_EQ(inst.getDifficulty(), Difficulty::Hard);
    // effective = base * id = 1.0 * 3 = 3.0
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 3.0f);
    // (3.0 - 2.0) / 2.0 = 0.5
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.5f);
}

// ============================================================================
// DifficultyInstance 完整构造函数测试
// ============================================================================

TEST(DifficultyInstanceTest, FullConstructor_PeacefulAlwaysZero)
{
    // Peaceful 难度下无论时间多长，有效难度始终为 0
    DifficultyInstance inst(Difficulty::Peaceful, 1000000, 1000000, 1.0f);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 0.0f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

TEST(DifficultyInstanceTest, FullConstructor_PeacefulZeroTime)
{
    DifficultyInstance inst(Difficulty::Peaceful, 0, 0, 0.0f);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 0.0f);
}

TEST(DifficultyInstanceTest, FullConstructor_LongWorldTimeIncreasesDifficulty)
{
    // 同一难度下，世界时间越长有效难度越高
    DifficultyInstance instEarly(Difficulty::Normal, 0, 0, 0.0f);
    DifficultyInstance instLate(Difficulty::Normal, 1440000, 0, 0.0f);
    EXPECT_GT(instLate.getEffectiveDifficulty(), instEarly.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, FullConstructor_HighChunkInhabitedTimeIncreasesDifficulty)
{
    // 高区块居住时间增加难度
    DifficultyInstance instLow(Difficulty::Normal, 0, 0, 0.0f);
    DifficultyInstance instHigh(Difficulty::Normal, 0, 3600000, 0.0f);
    EXPECT_GT(instHigh.getEffectiveDifficulty(), instLow.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, FullConstructor_EasyChunkFactorHalved)
{
    // Easy 难度下区块因子减半
    // 使用相同的非零时间参数，Easy 的有效难度增长比 Normal 慢
    // 因为 chunkFactor 在 Easy 下乘以 0.5
    DifficultyInstance instEasy(Difficulty::Easy, 1440000, 3600000, 1.0f);
    DifficultyInstance instNormal(Difficulty::Normal, 1440000, 3600000, 1.0f);

    // 两者都应有非零有效难度，但 Normal 应更高
    EXPECT_GT(instEasy.getEffectiveDifficulty(), 0.0f);
    EXPECT_GT(instNormal.getEffectiveDifficulty(), instEasy.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, FullConstructor_MoonPhaseAddsToChunkFactor)
{
    // 月相因子增加难度（需要足够的世界时间才能生效）
    // worldTime=72000 -> timeGlobalFactor=0, 月相被夹到0，无效果
    // 使用更大的 worldTime 使月相因子生效
    DifficultyInstance instNoMoon(Difficulty::Normal, 720000, 0, 0.0f);
    DifficultyInstance instFullMoon(Difficulty::Normal, 720000, 0, 1.0f);
    EXPECT_GT(instFullMoon.getEffectiveDifficulty(), instNoMoon.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, FullConstructor_HardUsesFullChunkFactor)
{
    // Hard 难度使用完整的区块因子（1.0），而不是像 Easy/Normal 那样减半（0.75）
    DifficultyInstance instHard(Difficulty::Hard, 1440000, 3600000, 1.0f);
    DifficultyInstance instNormal(Difficulty::Normal, 1440000, 3600000, 1.0f);

    // Hard 的区块因子系数为 1.0，Normal 为 0.75
    EXPECT_GT(instHard.getEffectiveDifficulty(), instNormal.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, FullConstructor_AllMaxFactors)
{
    // 所有因子拉满：最大世界时间 + 最大区块居住时间 + 满月
    DifficultyInstance inst(Difficulty::Hard, 1440000, 3600000, 1.0f);

    // timeGlobalFactor = clamp((1440000 - 72000) / 1440000, 0, 1) = clamp(0.95, 0, 1) = 0.95
    // f = 0.75 + 0.95 * 0.25 = 0.75 + 0.2375 = 0.9875
    // chunkFactor = 1.0 * 1.0 = 1.0 (Hard)
    // moonFactor = clamp(1.0 * 0.25, 0, 0.95) = 0.25
    // chunkFactor += 0.25 = 1.25
    // f += 1.25 = 2.2375
    // effective = 3 * 2.2375 = 6.7125
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 6.7125f);
    // 6.7125 > 4.0 -> special = 1.0
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 1.0f);
}

TEST(DifficultyInstanceTest, FullConstructor_ZeroTimeZeroChunk)
{
    // 初始状态：世界时间0、区块居住时间0、月相0
    DifficultyInstance inst(Difficulty::Normal, 0, 0, 0.0f);

    // timeGlobalFactor: clamp((-72000 + 0) / 1440000, 0, 1) = clamp(-0.05, 0, 1) = 0
    // chunkFactor: clamp(0 / 3600000, 0, 1) * 0.75 = 0
    // moonFactor: clamp(0 * 0.25, 0, 0) = 0
    // f = 0.75 + 0 * 0.25 + 0 + 0 = 0.75
    // effective = 2 * 0.75 = 1.5
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 1.5f);
}

// ============================================================================
// getSpecialMultiplier() 边界测试
// ============================================================================

TEST(DifficultyInstanceTest, SpecialMultiplier_BelowTwoReturnsZero)
{
    // effectiveDifficulty < 2.0 -> 0.0
    DifficultyInstance inst(Difficulty::Easy, 0, 0, 0.0f);
    // Easy: effective = 0.75 * 1 = 0.75 < 2.0
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

TEST(DifficultyInstanceTest, SpecialMultiplier_AboveFourReturnsOne)
{
    // effectiveDifficulty > 4.0 -> 1.0
    // 需要 effective > 4.0，使用 Hard + 充足时间和区块居住时间
    DifficultyInstance inst(Difficulty::Hard, 1440000, 3600000, 1.0f);
    EXPECT_GT(inst.getEffectiveDifficulty(), 4.0f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 1.0f);
}

TEST(DifficultyInstanceTest, SpecialMultiplier_ExactlyThreeReturnsHalf)
{
    // 简化构造 Hard: effective = 3.0
    // (3.0 - 2.0) / 2.0 = 0.5
    DifficultyInstance inst(Difficulty::Hard);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 3.0f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.5f);
}

TEST(DifficultyInstanceTest, SpecialMultiplier_ExactlyTwoReturnsZero)
{
    // 简化构造 Normal: effective = 2.0
    // 2.0 不小于 2.0，但 (2.0 - 2.0) / 2.0 = 0.0
    DifficultyInstance inst(Difficulty::Normal);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 2.0f);
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 0.0f);
}

// ============================================================================
// isHard() 测试
// ============================================================================

TEST(DifficultyInstanceTest, IsHard_EffectiveBelowThree)
{
    // Peaceful 和 Easy 的有效难度 < 3.0
    DifficultyInstance peaceful(Difficulty::Peaceful);
    EXPECT_FALSE(peaceful.isHard());

    DifficultyInstance easy(Difficulty::Easy);
    EXPECT_FALSE(easy.isHard());
}

TEST(DifficultyInstanceTest, IsHard_NormalIsNotHard)
{
    // Normal 简化构造: effective = 2.0 < 3.0
    DifficultyInstance inst(Difficulty::Normal);
    EXPECT_FALSE(inst.isHard());
}

TEST(DifficultyInstanceTest, IsHard_HardSimplifiedIsHard)
{
    // Hard 简化构造: effective = 3.0 >= 3.0
    DifficultyInstance inst(Difficulty::Hard);
    EXPECT_TRUE(inst.isHard());
}

TEST(DifficultyInstanceTest, IsHard_BoundaryAtThree)
{
    // 当 effectiveDifficulty >= 3.0 时返回 true
    // 使用完整构造函数让 effective 恰好 >= 3.0
    // Hard + 部分时间因子
    DifficultyInstance inst(Difficulty::Hard, 72000, 0, 0.0f);
    // 如果 effective >= 3.0 则 isHard() = true
    // timeGlobalFactor: clamp((-72000 + 72000) / 1440000, 0, 1) = clamp(0, 0, 1) = 0
    // chunkFactor = 0, moonFactor = 0
    // f = 0.75 + 0 + 0 + 0 = 0.75
    // effective = 3 * 0.75 = 2.25 < 3.0 -> NOT hard
    EXPECT_FALSE(inst.isHard());
}

// ============================================================================
// isHarderThan() 测试
// ============================================================================

TEST(DifficultyInstanceTest, IsHarderThan_PeacefulNotHarderThanZero)
{
    DifficultyInstance inst(Difficulty::Peaceful);
    // effective = 0.0, 0.0 > 0.0 is false
    EXPECT_FALSE(inst.isHarderThan(0.0f));
}

TEST(DifficultyInstanceTest, IsHarderThan_EasyHarderThanZero)
{
    DifficultyInstance inst(Difficulty::Easy);
    // effective = 0.75 > 0.0
    EXPECT_TRUE(inst.isHarderThan(0.0f));
}

TEST(DifficultyInstanceTest, IsHarderThan_EasyNotHarderThanOne)
{
    DifficultyInstance inst(Difficulty::Easy);
    // effective = 0.75, 0.75 > 1.0 is false
    EXPECT_FALSE(inst.isHarderThan(1.0f));
}

TEST(DifficultyInstanceTest, IsHarderThan_NormalHarderThanEasy)
{
    DifficultyInstance normal(Difficulty::Normal);
    DifficultyInstance easy(Difficulty::Easy);
    EXPECT_TRUE(normal.isHarderThan(easy.getEffectiveDifficulty()));
}

TEST(DifficultyInstanceTest, IsHarderThan_HardHarderThanNormal)
{
    DifficultyInstance hard(Difficulty::Hard);
    DifficultyInstance normal(Difficulty::Normal);
    EXPECT_TRUE(hard.isHarderThan(normal.getEffectiveDifficulty()));
}

TEST(DifficultyInstanceTest, IsHarderThan_NotHarderThanSelf)
{
    DifficultyInstance inst(Difficulty::Normal);
    // effective = 2.0, 2.0 > 2.0 is false (strictly greater)
    EXPECT_FALSE(inst.isHarderThan(inst.getEffectiveDifficulty()));
}

TEST(DifficultyInstanceTest, IsHarderThan_NegativeThreshold)
{
    DifficultyInstance inst(Difficulty::Peaceful);
    // effective = 0.0, 0.0 > -1.0 is true
    EXPECT_TRUE(inst.isHarderThan(-1.0f));
}

// ============================================================================
// calculateDifficulty() 边界条件测试
// ============================================================================

TEST(DifficultyInstanceTest, Calculate_PeacefulIgnoresAllFactors)
{
    // 即使所有因子拉满，Peaceful 始终为 0
    DifficultyInstance zero(Difficulty::Peaceful, 0, 0, 0.0f);
    DifficultyInstance max(Difficulty::Peaceful, 1440000, 3600000, 1.0f);
    EXPECT_FLOAT_EQ(zero.getEffectiveDifficulty(), 0.0f);
    EXPECT_FLOAT_EQ(max.getEffectiveDifficulty(), 0.0f);
}

TEST(DifficultyInstanceTest, Calculate_NegativeWorldTimeBeforeOffset)
{
    // worldTime < 72000 时，timeGlobalFactor = clamp((-72000 + worldTime) / 1440000, 0, 1) = 0
    // 因为 -72000 + worldTime < 0
    DifficultyInstance inst(Difficulty::Normal, 0, 0, 0.0f);
    // f = 0.75 + 0 + 0 + 0 = 0.75, effective = 2 * 0.75 = 1.5
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 1.5f);
}

TEST(DifficultyInstanceTest, Calculate_ExactlyAtTimeGlobalOffset)
{
    // worldTime = 72000 -> (-72000 + 72000) / 1440000 = 0.0
    DifficultyInstance inst(Difficulty::Normal, 72000, 0, 0.0f);
    // timeGlobalFactor = 0, f = 0.75, effective = 1.5
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 1.5f);
}

TEST(DifficultyInstanceTest, Calculate_MidWorldTime)
{
    // worldTime = 720000 -> (-72000 + 720000) / 1440000 = 648000 / 1440000 = 0.45
    DifficultyInstance inst(Difficulty::Normal, 720000, 0, 0.0f);
    // timeGlobalFactor = 0.45, f = 0.75 + 0.45 * 0.25 + 0 + 0 = 0.75 + 0.1125 = 0.8625
    // effective = 2 * 0.8625 = 1.725
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 1.725f);
}

TEST(DifficultyInstanceTest, Calculate_MaxWorldTimeClampsToOne)
{
    // worldTime 远超 MAX_DIFFICULTY_TIME_GLOBAL 时，timeGlobalFactor 被夹到 1.0
    DifficultyInstance inst(Difficulty::Normal, 9999999, 0, 0.0f);
    // timeGlobalFactor = clamp((-72000 + 9999999) / 1440000, 0, 1) = 1.0
    // f = 0.75 + 1.0 * 0.25 + 0 + 0 = 1.0
    // effective = 2 * 1.0 = 2.0
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 2.0f);
}

TEST(DifficultyInstanceTest, Calculate_MaxChunkInhabitedTime)
{
    // chunkInhabitedTime = MAX_DIFFICULTY_TIME_LOCAL = 3600000
    // Normal: chunkFactor = clamp(1.0, 0, 1) * 0.75 = 0.75
    DifficultyInstance inst(Difficulty::Normal, 0, 3600000, 0.0f);
    // timeGlobalFactor = 0, chunkFactor = 0.75, moonFactor = 0
    // f = 0.75 + 0 + 0.75 + 0 = 1.5
    // effective = 2 * 1.5 = 3.0
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 3.0f);
}

TEST(DifficultyInstanceTest, Calculate_ChunkTimeExceedsMaxClamped)
{
    // chunkInhabitedTime 远超 MAX_DIFFICULTY_TIME_LOCAL，被夹到 1.0
    DifficultyInstance inst(Difficulty::Normal, 0, 9999999, 0.0f);
    // 和最大值的结果相同
    DifficultyInstance instMax(Difficulty::Normal, 0, 3600000, 0.0f);
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), instMax.getEffectiveDifficulty());
}

TEST(DifficultyInstanceTest, Calculate_MoonPhaseClampedByTimeGlobalFactor)
{
    // 月相因子 = clamp(moonPhaseFactor * 0.25, 0, timeGlobalFactor)
    // 当 timeGlobalFactor = 0 时，月相因子也为 0
    DifficultyInstance noTime(Difficulty::Normal, 0, 0, 1.0f);
    // timeGlobalFactor = 0, moonFactor = clamp(0.25, 0, 0) = 0
    // f = 0.75 + 0 + 0 + 0 = 0.75, effective = 1.5
    EXPECT_FLOAT_EQ(noTime.getEffectiveDifficulty(), 1.5f);
}

TEST(DifficultyInstanceTest, Calculate_MoonPhaseWithNonZeroTimeGlobalFactor)
{
    // worldTime = 720000 -> timeGlobalFactor = 0.45
    // moonPhaseFactor = 1.0 -> moonFactor = clamp(0.25, 0, 0.45) = 0.25
    DifficultyInstance inst(Difficulty::Normal, 720000, 0, 1.0f);
    // timeGlobalFactor = 0.45
    // chunkFactor = 0 (no chunk time)
    // moonFactor = 0.25
    // chunkFactor + moonFactor = 0 + 0.25 = 0.25
    // f = 0.75 + 0.45 * 0.25 + 0.25 = 0.75 + 0.1125 + 0.25 = 1.1125
    // effective = 2 * 1.1125 = 2.225
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 2.225f);
}

TEST(DifficultyInstanceTest, Calculate_MoonPhaseClampedWhenExceedsTimeGlobalFactor)
{
    // timeGlobalFactor = 0.1 (small), moonPhaseFactor = 1.0
    // moonFactor = clamp(0.25, 0, 0.1) = 0.1
    // 计算 worldTime 使得 timeGlobalFactor = 0.1
    // (-72000 + worldTime) / 1440000 = 0.1 -> worldTime = 72000 + 144000 = 216000
    DifficultyInstance inst(Difficulty::Normal, 216000, 0, 1.0f);
    // timeGlobalFactor = 0.1
    // chunkFactor = 0, moonFactor = clamp(0.25, 0, 0.1) = 0.1
    // chunkFactor + moonFactor = 0.1
    // f = 0.75 + 0.1 * 0.25 + 0.1 = 0.75 + 0.025 + 0.1 = 0.875
    // effective = 2 * 0.875 = 1.75
    EXPECT_FLOAT_EQ(inst.getEffectiveDifficulty(), 1.75f);
}

TEST(DifficultyInstanceTest, Calculate_EasyDifficultyChunkFactorHalved)
{
    // Easy 下 chunkFactor *= 0.5
    // worldTime = 1440000 -> timeGlobalFactor = (1440000 - 72000) / 1440000 = 0.95
    // chunkInhabitedTime = 3600000 (max chunk factor = 1.0)
    // moonPhaseFactor = 1.0
    DifficultyInstance inst(Difficulty::Easy, 1440000, 3600000, 1.0f);
    // timeGlobalFactor = 0.95
    // chunkFactor = 1.0 * 0.75 = 0.75 (Easy uses 0.75 for non-Hard)
    // moonFactor = clamp(0.25, 0, 0.95) = 0.25
    // chunkFactor += moonFactor = 0.75 + 0.25 = 1.0
    // Easy: chunkFactor *= 0.5 -> 0.5
    // f = 0.75 + 0.95 * 0.25 + 0.5 = 0.75 + 0.2375 + 0.5 = 1.4875
    // effective = 1 * 1.4875 = 1.4875
    EXPECT_NEAR(inst.getEffectiveDifficulty(), 1.4875f, 0.001f);
}

TEST(DifficultyInstanceTest, Calculate_NormalDifficultyFullFactors)
{
    DifficultyInstance inst(Difficulty::Normal, 1440000, 3600000, 1.0f);
    // timeGlobalFactor = 0.95
    // chunkFactor = 1.0 * 0.75 = 0.75 (Normal uses 0.75 for non-Hard)
    // moonFactor = clamp(0.25, 0, 0.95) = 0.25
    // chunkFactor += moonFactor = 0.75 + 0.25 = 1.0
    // Normal: chunkFactor NOT halved
    // f = 0.75 + 0.95 * 0.25 + 1.0 = 0.75 + 0.2375 + 1.0 = 1.9875
    // effective = 2 * 1.9875 = 3.975
    EXPECT_NEAR(inst.getEffectiveDifficulty(), 3.975f, 0.001f);
    // effective = 3.975 >= 2.0, (3.975 - 2.0) / 2.0 = 0.9875
    EXPECT_NEAR(inst.getSpecialMultiplier(), 0.9875f, 0.001f);
}

TEST(DifficultyInstanceTest, Calculate_HardDifficultyFullFactors)
{
    DifficultyInstance inst(Difficulty::Hard, 1440000, 3600000, 1.0f);
    // timeGlobalFactor = 0.95
    // chunkFactor = 1.0 * 1.0 = 1.0 (Hard uses 1.0)
    // moonFactor = clamp(0.25, 0, 0.95) = 0.25
    // chunkFactor += moonFactor = 1.0 + 0.25 = 1.25
    // Hard: chunkFactor NOT halved
    // f = 0.75 + 0.95 * 0.25 + 1.25 = 0.75 + 0.2375 + 1.25 = 2.2375
    // effective = 3 * 2.2375 = 6.7125
    EXPECT_NEAR(inst.getEffectiveDifficulty(), 6.7125f, 0.001f);
    // 6.7125 > 4.0 -> special = 1.0
    EXPECT_FLOAT_EQ(inst.getSpecialMultiplier(), 1.0f);
}
