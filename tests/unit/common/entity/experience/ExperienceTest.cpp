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

#include "entity/experience/ExperienceConstants.hpp"
#include "entity/experience/ExperienceUtils.hpp"
#include "util/math/random/Random.hpp"
#include <gtest/gtest.h>

// 使用完整命名空间
namespace xp = mc::entity::experience;
namespace xp_constants = mc::entity::experience::constants;
namespace xp_utils = mc::entity::experience::utils;
using mc::i32;
using mc::u32;
using mc::u8;
using mc::math::Random;

// ==================== ExperienceConstants Tests ====================

class ExperienceConstantsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ExperienceConstantsTest, OrbConstants)
{
    // 验证经验球常量
    EXPECT_EQ(xp_constants::MAX_ORB_AGE, 6000); // 5分钟 = 6000 ticks
    // 原版 MC 构造函数中不设置 pickupDelay，默认为 0
    EXPECT_EQ(xp_constants::DEFAULT_PICKUP_DELAY, 0);
    EXPECT_FLOAT_EQ(xp_constants::ORB_TRACKING_RANGE, 8.0f);
    EXPECT_EQ(xp_constants::MAX_ORB_VALUE, 2477);
    EXPECT_FLOAT_EQ(xp_constants::ORB_GRAVITY, 0.03f);
    EXPECT_FLOAT_EQ(xp_constants::ORB_GROUND_FRICTION, 0.98f);
}

TEST_F(ExperienceConstantsTest, XPSplitValues)
{
    // 验证经验分割表
    EXPECT_EQ(xp_constants::XP_SPLIT_COUNT, 11);
    EXPECT_EQ(xp_constants::XP_SPLIT_VALUES[0], 2477); // 最大
    EXPECT_EQ(xp_constants::XP_SPLIT_VALUES[10], 1);   // 最小

    // 验证分割表是递减的
    for (int i = 1; i < xp_constants::XP_SPLIT_COUNT; ++i) {
        EXPECT_GT(xp_constants::XP_SPLIT_VALUES[i - 1], xp_constants::XP_SPLIT_VALUES[i]);
    }
}

TEST_F(ExperienceConstantsTest, PlayerConstants)
{
    // 验证玩家经验常量
    EXPECT_EQ(xp_constants::PLAYER_XP_COOLDOWN, 2);
    EXPECT_EQ(xp_constants::MAX_DEATH_XP_DROP, 100);
    EXPECT_EQ(xp_constants::DEATH_XP_PER_LEVEL, 7);
    EXPECT_EQ(xp_constants::MAX_EXPERIENCE_LEVEL, 21862);
}

TEST_F(ExperienceConstantsTest, OreExperienceRanges)
{
    // 煤矿: 0-2
    EXPECT_LE(xp_constants::COAL_ORE_XP_MIN, xp_constants::COAL_ORE_XP_MAX);
    EXPECT_EQ(xp_constants::COAL_ORE_XP_MIN, 0);
    EXPECT_EQ(xp_constants::COAL_ORE_XP_MAX, 2);

    // 钻石矿: 3-7
    EXPECT_LE(xp_constants::DIAMOND_ORE_XP_MIN, xp_constants::DIAMOND_ORE_XP_MAX);
    EXPECT_EQ(xp_constants::DIAMOND_ORE_XP_MIN, 3);
    EXPECT_EQ(xp_constants::DIAMOND_ORE_XP_MAX, 7);

    // 绿宝石矿: 3-7
    EXPECT_LE(xp_constants::EMERALD_ORE_XP_MIN, xp_constants::EMERALD_ORE_XP_MAX);

    // 末影龙: 12000
    EXPECT_EQ(xp_constants::ENDER_DRAGON_XP, 12000);
}

// ==================== ExperienceUtils Tests ====================

class ExperienceUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ExperienceUtilsTest, GetXPSplit)
{
    // 测试经验分割
    // 小于等于1的值
    EXPECT_EQ(xp_utils::getXPSplit(1), 1);
    EXPECT_EQ(xp_utils::getXPSplit(0), 1);

    // 小值
    EXPECT_EQ(xp_utils::getXPSplit(2), 1);
    EXPECT_EQ(xp_utils::getXPSplit(3), 3);
    EXPECT_EQ(xp_utils::getXPSplit(5), 3);
    EXPECT_EQ(xp_utils::getXPSplit(7), 7);
    EXPECT_EQ(xp_utils::getXPSplit(10), 7);

    // 大值
    EXPECT_EQ(xp_utils::getXPSplit(100), 73);
    EXPECT_EQ(xp_utils::getXPSplit(500), 307);
    EXPECT_EQ(xp_utils::getXPSplit(1000), 617);
    EXPECT_EQ(xp_utils::getXPSplit(3000), 2477);
}

TEST_F(ExperienceUtilsTest, SplitExperience)
{
    std::vector<i32> result;

    // 测试小经验值
    // 5 = 3 + 1 + 1 (因为 getXPSplit(5)=3, getXPSplit(2)=1, getXPSplit(1)=1)
    xp_utils::splitExperience(5, result);
    EXPECT_EQ(result.size(), 3u); // 3个球
    i32 sum = 0;
    for (i32 v : result)
        sum += v;
    EXPECT_EQ(sum, 5);

    // 测试中等经验值
    xp_utils::splitExperience(100, result);
    sum = 0;
    for (i32 v : result)
        sum += v;
    EXPECT_EQ(sum, 100);

    // 测试大经验值
    xp_utils::splitExperience(5000, result);
    sum = 0;
    for (i32 v : result)
        sum += v;
    EXPECT_EQ(sum, 5000);

    // 验证每个分割值都是有效的
    for (i32 v : result) {
        bool found = false;
        for (int i = 0; i < xp_constants::XP_SPLIT_COUNT; ++i) {
            if (xp_constants::XP_SPLIT_VALUES[i] == v) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Invalid XP split value: " << v;
    }
}

TEST_F(ExperienceUtilsTest, GetOrbSize)
{
    // 测试经验球大小等级
    // 1-2: 等级 0
    EXPECT_EQ(xp_utils::getOrbSize(1), 0);
    EXPECT_EQ(xp_utils::getOrbSize(2), 0);

    // 3-6: 等级 1
    EXPECT_EQ(xp_utils::getOrbSize(3), 1);
    EXPECT_EQ(xp_utils::getOrbSize(6), 1);

    // 7-16: 等级 2
    EXPECT_EQ(xp_utils::getOrbSize(7), 2);
    EXPECT_EQ(xp_utils::getOrbSize(16), 2);

    // 大值
    EXPECT_EQ(xp_utils::getOrbSize(2477), 10);
    EXPECT_EQ(xp_utils::getOrbSize(2000), 9);
}

TEST_F(ExperienceUtilsTest, CalculateOrbColor)
{
    // 测试颜色计算（对齐 MC Java 版 ExperienceOrbRenderer.submit）
    // 在 time=0 时: sin(0) = 0, 所以 red = (0+1)*0.5*255 = 127.5, green = 255, alpha = 128
    mc::math::Vector4f color0 = xp_utils::calculateOrbColor(0.0);

    // 绿色分量应该为 1.0（固定满值）
    EXPECT_FLOAT_EQ(color0.y, 1.0f);

    // Alpha 应该为 128/255 ≈ 0.502
    EXPECT_NEAR(color0.w, 128.0f / 255.0f, 0.01f);

    // 红色分量在 time=0 时应该为 0.5（(sin(0)+1)*0.5 = 0.5）
    EXPECT_NEAR(color0.x, 0.5f, 0.01f);

    // 蓝色分量应该很小（系数 0.1）
    EXPECT_LT(color0.z, 0.2f);

    // 在不同时间点颜色应该不同（动画循环）
    mc::math::Vector4f colorPi = xp_utils::calculateOrbColor(3.14159265);
    // sin(PI/2) = sin(PI/2) ≈ 1.0, 所以 red 应该接近 1.0
    EXPECT_GT(colorPi.x, 0.8f);
}

TEST_F(ExperienceUtilsTest, CalculateOrbIconUV)
{
    // 测试图标 UV 计算对齐 MC Java 版 ExperienceOrbRenderer.submit()
    // 图集: 64x64, 4列×3行, 每个图标 16x16

    // 边界值: iconIndex=0 (左上角)
    mc::f64 u0, v0, u1, v1;
    xp_utils::calculateOrbIconUV(0, u0, v0, u1, v1);
    EXPECT_NEAR(u0, 0.0, 0.001);         // 列0, 起始U=0/64=0
    EXPECT_NEAR(v0, 0.0, 0.001);         // 行0, 起始V=0/64=0
    EXPECT_NEAR(u1, 16.0 / 64.0, 0.001); // 结束U=16/64=0.25
    EXPECT_NEAR(v1, 16.0 / 64.0, 0.001); // 结束V=16/64=0.25

    // iconIndex=1 (第0行第1列)
    xp_utils::calculateOrbIconUV(1, u0, v0, u1, v1);
    EXPECT_NEAR(u0, 16.0 / 64.0, 0.001); // 列1, 起始U=16/64=0.25
    EXPECT_NEAR(v0, 0.0, 0.001);         // 行0
    EXPECT_NEAR(u1, 32.0 / 64.0, 0.001); // 结束U=32/64=0.5
    EXPECT_NEAR(v1, 16.0 / 64.0, 0.001); // 结束V=16/64=0.25

    // iconIndex=4 (第1行第0列, 第二行开始)
    xp_utils::calculateOrbIconUV(4, u0, v0, u1, v1);
    EXPECT_NEAR(u0, 0.0, 0.001);         // 列0
    EXPECT_NEAR(v0, 16.0 / 64.0, 0.001); // 行1, 起始V=16/64=0.25
    EXPECT_NEAR(u1, 16.0 / 64.0, 0.001); // 结束U=16/64=0.25
    EXPECT_NEAR(v1, 32.0 / 64.0, 0.001); // 结束V=32/64=0.5

    // 边界值: iconIndex=10 (第2行第2列, 最后一个图标)
    xp_utils::calculateOrbIconUV(10, u0, v0, u1, v1);
    EXPECT_NEAR(u0, 32.0 / 64.0, 0.001); // 列2, 起始U=32/64=0.5
    EXPECT_NEAR(v0, 32.0 / 64.0, 0.001); // 行2, 起始V=32/64=0.5
    EXPECT_NEAR(u1, 48.0 / 64.0, 0.001); // 结束U=48/64=0.75
    EXPECT_NEAR(v1, 48.0 / 64.0, 0.001); // 结束V=48/64=0.75

    // 中间值: iconIndex=5 (第1行第1列)
    xp_utils::calculateOrbIconUV(5, u0, v0, u1, v1);
    EXPECT_NEAR(u0, 16.0 / 64.0, 0.001);
    EXPECT_NEAR(v0, 16.0 / 64.0, 0.001);
    EXPECT_NEAR(u1, 32.0 / 64.0, 0.001);
    EXPECT_NEAR(v1, 32.0 / 64.0, 0.001);

    // 验证所有图标的 UV 区域不超出图集范围
    for (i32 i = 0; i <= 10; ++i) {
        xp_utils::calculateOrbIconUV(i, u0, v0, u1, v1);
        EXPECT_GE(u0, 0.0);
        EXPECT_GE(v0, 0.0);
        EXPECT_LE(u1, 1.0 + 0.001);
        EXPECT_LE(v1, 1.0 + 0.001);
        EXPECT_LT(u0, u1); // U 必须递增
        EXPECT_LT(v0, v1); // V 必须递增
    }
}

TEST_F(ExperienceUtilsTest, RandomOreExperience)
{
    Random rng(12345); // 固定种子

    // 测试各种矿石
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 100; ++j) {
            i32 xp = xp_utils::randomOreExperience(rng, i);

            switch (i) {
                case 0: // 煤矿
                    EXPECT_GE(xp, xp_constants::COAL_ORE_XP_MIN);
                    EXPECT_LE(xp, xp_constants::COAL_ORE_XP_MAX);
                    break;
                case 1: // 钻石矿
                    EXPECT_GE(xp, xp_constants::DIAMOND_ORE_XP_MIN);
                    EXPECT_LE(xp, xp_constants::DIAMOND_ORE_XP_MAX);
                    break;
                case 2: // 绿宝石矿
                    EXPECT_GE(xp, xp_constants::EMERALD_ORE_XP_MIN);
                    EXPECT_LE(xp, xp_constants::EMERALD_ORE_XP_MAX);
                    break;
                    // ... 其他矿石
            }
        }
    }
}

TEST_F(ExperienceUtilsTest, RandomPassiveMobExperience)
{
    Random rng(12345);

    // 测试被动动物经验 (1-3)
    for (int i = 0; i < 100; ++i) {
        i32 xp = xp_utils::randomPassiveMobExperience(rng);
        EXPECT_GE(xp, xp_constants::PASSIVE_MOB_XP_MIN);
        EXPECT_LE(xp, xp_constants::PASSIVE_MOB_XP_MAX);
    }
}

TEST_F(ExperienceUtilsTest, RandomFishingExperience)
{
    Random rng(12345);

    // 测试钓鱼经验 (1-6)
    for (int i = 0; i < 100; ++i) {
        i32 xp = xp_utils::randomFishingExperience(rng);
        EXPECT_GE(xp, xp_constants::FISHING_XP_MIN);
        EXPECT_LE(xp, xp_constants::FISHING_XP_MAX);
    }
}

TEST_F(ExperienceUtilsTest, CalculateDeathDropXp)
{
    // 等级 0: 0 经验
    EXPECT_EQ(xp_utils::calculateDeathDropXp(0), 0);

    // 等级 1-14: level * 7
    EXPECT_EQ(xp_utils::calculateDeathDropXp(1), 7);
    EXPECT_EQ(xp_utils::calculateDeathDropXp(10), 70);
    EXPECT_EQ(xp_utils::calculateDeathDropXp(14), 98);

    // 等级 15+: 最大 100
    EXPECT_EQ(xp_utils::calculateDeathDropXp(15), 100);
    EXPECT_EQ(xp_utils::calculateDeathDropXp(100), 100);
    EXPECT_EQ(xp_utils::calculateDeathDropXp(1000), 100);
}

TEST_F(ExperienceUtilsTest, DurabilityToXp)
{
    // 每2点经验修复1点耐久
    EXPECT_EQ(xp_utils::durabilityToXp(1), 1);  // 1耐久需要1经验
    EXPECT_EQ(xp_utils::durabilityToXp(2), 1);  // 2耐久需要1经验
    EXPECT_EQ(xp_utils::durabilityToXp(3), 2);  // 3耐久需要2经验
    EXPECT_EQ(xp_utils::durabilityToXp(4), 2);  // 4耐久需要2经验
    EXPECT_EQ(xp_utils::durabilityToXp(10), 5); // 10耐久需要5经验
}

TEST_F(ExperienceUtilsTest, XpToDurability)
{
    // 每2点经验修复1点耐久
    EXPECT_EQ(xp_utils::xpToDurability(1), 2);
    EXPECT_EQ(xp_utils::xpToDurability(2), 4);
    EXPECT_EQ(xp_utils::xpToDurability(5), 10);
    EXPECT_EQ(xp_utils::xpToDurability(10), 20);
}

// ==================== Integration Tests ====================

class ExperienceIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ExperienceIntegrationTest, SplitExperienceLargeValues)
{
    std::vector<i32> result;

    // 测试末影龙经验
    // 12000 = 2477*4 + 1237 + 617 + 307 + 149 + 73 + 37 + 17 + 7 + 3 + 1*...
    // 实际上每次取最大可分割值，会生成多个球
    xp_utils::splitExperience(xp_constants::ENDER_DRAGON_XP, result);
    i32 sum = 0;
    for (i32 v : result)
        sum += v;
    EXPECT_EQ(sum, xp_constants::ENDER_DRAGON_XP);

    // 验证球的数量合理 (末影龙12000经验大约分成12个球)
    EXPECT_LT(result.size(), 15u);
    EXPECT_GT(result.size(), 5u);
}

TEST_F(ExperienceIntegrationTest, SplitExperienceSmallValues)
{
    std::vector<i32> result;

    // 测试各种小值
    for (i32 xp_val = 1; xp_val <= 20; ++xp_val) {
        xp_utils::splitExperience(xp_val, result);
        i32 sum = 0;
        for (i32 v : result)
            sum += v;
        EXPECT_EQ(sum, xp_val);
    }
}

TEST_F(ExperienceIntegrationTest, OrbSizeConsistency)
{
    // 验证球大小与分割值一致
    for (int i = 0; i < xp_constants::XP_SPLIT_COUNT; ++i) {
        i32 value = xp_constants::XP_SPLIT_VALUES[i];
        i32 size = xp_utils::getOrbSize(value);
        // 更大的值应该有更大的或相等的球大小
        if (i < xp_constants::XP_SPLIT_COUNT - 1) {
            i32 nextValue = xp_constants::XP_SPLIT_VALUES[i + 1];
            i32 nextSize = xp_utils::getOrbSize(nextValue);
            EXPECT_GE(size, nextSize);
        }
    }
}
