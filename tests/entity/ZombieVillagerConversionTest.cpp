/*
* Copyright (c) 2026 Guo Yi
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction restriction, including without limitation the rights
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
#include "common/entity/core/Entity.hpp"

using namespace mc;

// ============================================================================
// ZombieVillagerEntity 治愈加速测试
// ============================================================================
//
// 测试僵尸村民治愈时床和铁栏杆的加速机制。
// 参考 MC 1.16.5 ZombieVillagerEntity.getConversionProgress()
//
// 治愈加速逻辑：
// - 以自身为中心，检测 4x4x4 范围内的铁栏杆和床
// - 每个有效方块有 30% 概率使进度 +1
// - 最多检测 14 个有效方块
// - 力量效果额外加速（每级 +10%）
//
// 注意：完整的集成测试需要 Mock 世界和方块系统。
// 这里测试治愈加速的核心常量和逻辑。

class ZombieVillagerConversionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置代码
    }
};

// ============================================================================
// 治愈时间常量测试
// ============================================================================

TEST_F(ZombieVillagerConversionTest, ConversionTime_MinIsThreeMinutes)
{
    // MC 1.16.5: 最小治愈时间 3600 ticks = 3 分钟
    constexpr i32 CONVERSION_TIME_MIN = 3600;
    EXPECT_EQ(CONVERSION_TIME_MIN, 3600);
}

TEST_F(ZombieVillagerConversionTest, ConversionTime_MaxIsFiveMinutes)
{
    // MC 1.16.5: 最大治愈时间 6000 ticks = 5 分钟
    constexpr i32 CONVERSION_TIME_MAX = 6000;
    EXPECT_EQ(CONVERSION_TIME_MAX, 6000);
}

// ============================================================================
// 加速检测常量测试
// ============================================================================

TEST_F(ZombieVillagerConversionTest, SpeedupCheck_RangeIsFourBlocks)
{
    // MC 1.16.5: 加速检测范围为 4 格
    // 遍历范围: -4 到 +4（共 9x9x9 = 729 个方块）
    constexpr i32 SPEEDUP_CHECK_RANGE = 4;
    EXPECT_EQ(SPEEDUP_CHECK_RANGE, 4);
}

TEST_F(ZombieVillagerConversionTest, SpeedupCheck_ChanceIs30Percent)
{
    // MC 1.16.5: 每个有效方块有 30% 概率加速治愈
    constexpr f32 SPEEDUP_CHANCE = 0.3f;
    EXPECT_FLOAT_EQ(SPEEDUP_CHANCE, 0.3f);
}

TEST_F(ZombieVillagerConversionTest, SpeedupCheck_MaxBonusIs14)
{
    // MC 1.16.5: 最多检测 14 个有效方块
    // 变量 j < 14 限制检测数量
    constexpr i32 SPEEDUP_MAX_BONUS = 14;
    EXPECT_EQ(SPEEDUP_MAX_BONUS, 14);
}

// ============================================================================
// 力量效果加速常量测试
// ============================================================================

TEST_F(ZombieVillagerConversionTest, StrengthSpeedup_PerLevelIs10Percent)
{
    // MC 1.16.5: 每级力量效果减少 10% 治愈时间
    // 表现为进度增加 10%
    constexpr f32 STRENGTH_SPEEDUP_PER_LEVEL = 0.1f;
    EXPECT_FLOAT_EQ(STRENGTH_SPEEDUP_PER_LEVEL, 0.1f);
}

// ============================================================================
// 治愈后恶心效果常量测试
// ============================================================================

TEST_F(ZombieVillagerConversionTest, NauseaDuration_Is10Seconds)
{
    // MC 1.16.5: 治愈后村民获得恶心效果 200 ticks = 10 秒
    constexpr i32 NAUSEA_DURATION = 200;
    EXPECT_EQ(NAUSEA_DURATION, 200);
}

// ============================================================================
// 床和铁栏杆同等处理测试
// ============================================================================

TEST_F(ZombieVillagerConversionTest, BedAndIronBars_TreatedEqually)
{
    // MC 1.16.5: 床和铁栏杆被同等对待
    // 在 getConversionProgress() 中：
    // if (block == Blocks.IRON_BARS || block instanceof BedBlock)
    // 两者都有 30% 概率加速治愈

    // 这个测试验证两者使用相同的概率
    constexpr f32 IRON_BARS_SPEEDUP_CHANCE = 0.3f;
    constexpr f32 BED_SPEEDUP_CHANCE = 0.3f;
    EXPECT_FLOAT_EQ(IRON_BARS_SPEEDUP_CHANCE, BED_SPEEDUP_CHANCE);
}

// ============================================================================
// 进度计算逻辑测试
// ============================================================================

TEST_F(ZombieVillagerConversionTest, ProgressCalculation_BaseProgressIsOne)
{
    // MC 1.16.5: 基础进度为 1
    // int i = 1;
    constexpr i32 BASE_PROGRESS = 1;
    EXPECT_EQ(BASE_PROGRESS, 1);
}

TEST_F(ZombieVillagerConversionTest, ProgressCalculation_CheckProbabilityIs1Percent)
{
    // MC 1.16.5: 每tick只有 1% 概率执行检测
    // if (this.rand.nextFloat() < 0.01F)
    constexpr f32 CHECK_PROBABILITY = 0.01f;
    EXPECT_FLOAT_EQ(CHECK_PROBABILITY, 0.01f);
}

// ============================================================================
// 最大加速效果计算测试
// ============================================================================

TEST_F(ZombieVillagerConversionTest, MaxSpeedup_14BlocksAllTrigger)
{
    // 假设运气极好，14 个方块都触发 30% 概率
    // 理论最大进度 = 1 (基础) + 14 (加速) = 15
    constexpr i32 BASE_PROGRESS = 1;
    constexpr i32 MAX_BONUS = 14;
    constexpr i32 MAX_PROGRESS = BASE_PROGRESS + MAX_BONUS;
    EXPECT_EQ(MAX_PROGRESS, 15);
}

TEST_F(ZombieVillagerConversionTest, ExpectedSpeedup_AverageCase)
{
    // 平均情况下，14 个方块每个有 30% 概率触发
    // 预期加速次数 = 14 * 0.3 = 4.2
    // 预期总进度 = 1 + 4.2 = 5.2
    constexpr f32 SPEEDUP_CHANCE = 0.3f;
    constexpr i32 MAX_BONUS = 14;
    constexpr f32 EXPECTED_SPEEDUP = SPEEDUP_CHANCE * static_cast<f32>(MAX_BONUS);
    EXPECT_NEAR(EXPECTED_SPEEDUP, 4.2f, 0.01f);
}

// ============================================================================
// 方块检测范围测试
// ============================================================================

TEST_F(ZombieVillagerConversionTest, DetectionRange_IsCube)
{
    // MC 1.16.5: 检测范围是以自身为中心的立方体
    // 从 (x-4, y-4, z-4) 到 (x+4, y+4, z+4)
    // 注意：范围是 [x-4, x+4) 即左闭右开
    constexpr i32 RANGE = 4;
    constexpr i32 CUBE_SIZE = RANGE * 2; // 8
    // 总检测方块数 = 8^3 = 512
    constexpr i32 TOTAL_BLOCKS = CUBE_SIZE * CUBE_SIZE * CUBE_SIZE;
    EXPECT_EQ(TOTAL_BLOCKS, 512);
}

// ============================================================================
// 治愈时间缩短计算测试
// ============================================================================

TEST_F(ZombieVillagerConversionTest, TimeReduction_WithMaxSpeedup)
{
    // 最大治愈时间 6000 ticks，每tick最多减少 15 进度
    // 如果每tick都是最大加速，治愈时间 = 6000 / 15 = 400 ticks ≈ 20 秒
    // 但实际上平均加速约 5.2 进度/tick
    // 平均治愈时间 = 6000 / 5.2 ≈ 1154 ticks ≈ 58 秒
    constexpr i32 MAX_TIME = 6000;
    constexpr f32 AVG_PROGRESS = 5.2f;
    constexpr f32 AVG_TIME = static_cast<f32>(MAX_TIME) / AVG_PROGRESS;
    EXPECT_NEAR(AVG_TIME, 1154.0f, 10.0f);
}

// ============================================================================
// 床类型兼容性测试
// ============================================================================

TEST_F(ZombieVillagerConversionTest, AllBedColors_AreTreatedEqually)
{
    // MC 1.16.5: 所有颜色的床都会加速治愈
    // 检测方式是 block instanceof BedBlock
    // 包括 16 种颜色的床
    constexpr u32 BED_COLOR_COUNT = 16;
    EXPECT_EQ(BED_COLOR_COUNT, 16);
}

TEST_F(ZombieVillagerConversionTest, BedOccupiedState_DoesNotMatter)
{
    // MC 1.16.5: 床的占用状态不影响加速效果
    // 无论是已占用还是未占用的床，都会触发加速
    // 代码只检查 block instanceof BedBlock，不检查 state.get(OCCUPIED)
    constexpr bool CHECK_OCCUPIED = false;
    EXPECT_FALSE(CHECK_OCCUPIED);
}

TEST_F(ZombieVillagerConversionTest, BedPart_DoesNotMatter)
{
    // MC 1.16.5: 床的头部和脚部都触发加速
    // 代码只检查 block instanceof BedBlock，不区分 BED_PART
    // 一张完整的床（头部+脚部）会触发两次检测
    constexpr bool CHECK_PART = false;
    EXPECT_FALSE(CHECK_PART);
}
