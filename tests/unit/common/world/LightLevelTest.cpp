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

#include "common/TestWorldHelper.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/lighting/InternalLightUtils.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace test {

namespace {

/**
 * @brief 用于测试光照计算的 Mock World 实现
 *
 * 允许自定义天空光照、方块光照、时间、天气等参数。
 */
class MockWorldForLightLevel final : public BaseTestWorld {
public:
    void setSkyLightValue(u8 value) { m_skyLightValue = value; }
    void setBlockLightValue(u8 value) { m_blockLightValue = value; }
    void setDayTime(i64 value) { m_dayTime = value; }
    void setRaining(bool value) { m_raining = value; }
    void setThundering(bool value) { m_thundering = value; }
    void setHasSkyLight(bool value) { m_hasSkyLight = value; }

    // ========== IWorld 接口实现 ==========

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return m_skyLightValue; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return m_blockLightValue; }
    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    [[nodiscard]] bool isRaining() const override { return m_raining; }
    [[nodiscard]] bool isThundering() const override { return m_thundering; }
    [[nodiscard]] bool hasSkyLight() const override { return m_hasSkyLight; }

private:
    u8 m_skyLightValue = 15;
    u8 m_blockLightValue = 0;
    i64 m_dayTime = 6000; // 正午
    bool m_raining = false;
    bool m_thundering = false;
    bool m_hasSkyLight = true;
};

} // namespace

/**
 * @brief 光照等级测试套件
 *
 * 测试 IWorld 中新增的光照计算方法：
 * - getNeighborAwareLightSubtracted
 * - getLight
 * - getSkyDarkening
 */
class LightLevelTest : public ::testing::Test {
protected:
    MockWorldForLightLevel world;
};

// ============================================================================
// getLightSubtracted 测试
// ============================================================================

TEST_F(LightLevelTest, GetLightSubtracted_ReturnsBlockLightWhenHigher)
{
    // 方块光照 > 天空光照-衰减
    world.setBlockLightValue(10);
    world.setSkyLightValue(5);

    // 衰减 0：max(10, 5) = 10
    EXPECT_EQ(world.getLightSubtracted(BlockPos(0, 64, 0), 0), 10);

    // 衰减 2：max(10, 5-2) = max(10, 3) = 10
    EXPECT_EQ(world.getLightSubtracted(BlockPos(0, 64, 0), 2), 10);
}

TEST_F(LightLevelTest, GetLightSubtracted_ReturnsSkyLightWhenHigher)
{
    // 天空光照-衰减 > 方块光照
    world.setBlockLightValue(3);
    world.setSkyLightValue(15);

    // 衰减 0：max(3, 15) = 15
    EXPECT_EQ(world.getLightSubtracted(BlockPos(0, 64, 0), 0), 15);

    // 衰减 5：max(3, 15-5) = max(3, 10) = 10
    EXPECT_EQ(world.getLightSubtracted(BlockPos(0, 64, 0), 5), 10);
}

TEST_F(LightLevelTest, GetLightSubtracted_SkyLightDoesNotGoNegative)
{
    world.setBlockLightValue(0);
    world.setSkyLightValue(5);

    // 衰减 10：天空光照 5-10 = -5，但会被限制为 0
    // max(0, 0) = 0
    EXPECT_EQ(world.getLightSubtracted(BlockPos(0, 64, 0), 10), 0);
}

// ============================================================================
// getNeighborAwareLightSubtracted 测试
// ============================================================================

TEST_F(LightLevelTest, GetNeighborAwareLightSubtracted_ValidPosition)
{
    // 有效坐标范围内
    world.setBlockLightValue(5);
    world.setSkyLightValue(15);

    // 应该调用 getLightSubtracted
    u8 result = world.getNeighborAwareLightSubtracted(BlockPos(0, 64, 0), 10);
    EXPECT_EQ(result, 5); // max(5, 15-10) = 5
}

TEST_F(LightLevelTest, GetNeighborAwareLightSubtracted_OutsideWorldBounds)
{
    // 超出世界边界（坐标 >= 30000000 或 < -30000000）
    // MC 1.16.5: 有效范围是 x >= -30000000 && x < 30000000
    // 所以 -30000000 是有效的（边界上），30000000 是无效的
    // -30000001 是无效的（超出边界）

    // 无效坐标应该返回 15（最大亮度）
    EXPECT_EQ(world.getNeighborAwareLightSubtracted(BlockPos(30000000, 64, 0), 10), 15);
    EXPECT_EQ(world.getNeighborAwareLightSubtracted(BlockPos(-30000001, 64, 0), 10), 15);
    EXPECT_EQ(world.getNeighborAwareLightSubtracted(BlockPos(0, 64, 30000000), 10), 15);
    EXPECT_EQ(world.getNeighborAwareLightSubtracted(BlockPos(0, 64, -30000001), 10), 15);

    // 有效边界坐标应该返回正常光照计算结果
    // -30000000 是有效的边界坐标
    world.setBlockLightValue(5);
    world.setSkyLightValue(15);
    EXPECT_EQ(world.getNeighborAwareLightSubtracted(BlockPos(-30000000, 64, 0), 10), 5); // max(5, 15-10) = 5
    EXPECT_EQ(world.getNeighborAwareLightSubtracted(BlockPos(0, 64, -30000000), 10), 5);
}

// ============================================================================
// getSkyDarkening 测试
// ============================================================================

TEST_F(LightLevelTest, GetSkyDarkening_Noon_NoWeather)
{
    // 正午 (dayTime = 6000)，无天气
    world.setDayTime(6000);
    world.setRaining(false);
    world.setThundering(false);

    i32 darkening = world.getSkyDarkening();
    EXPECT_EQ(darkening, 0); // 正午时天空减暗因子为 0
}

TEST_F(LightLevelTest, GetSkyDarkening_Midnight_NoWeather)
{
    // 午夜 (dayTime = 18000)，无天气
    world.setDayTime(18000);
    world.setRaining(false);
    world.setThundering(false);

    i32 darkening = world.getSkyDarkening();
    EXPECT_GE(darkening, 10); // 午夜时天空减暗因子接近最大值
}

TEST_F(LightLevelTest, GetSkyDarkening_Raining_Increases)
{
    // 正午下雨
    world.setDayTime(6000);
    world.setRaining(true);
    world.setThundering(false);

    i32 rainyDarkening = world.getSkyDarkening();
    EXPECT_GE(rainyDarkening, 3); // 下雨至少增加 3
}

TEST_F(LightLevelTest, GetSkyDarkening_Thundering_IncreasesMore)
{
    // 正午雷暴
    world.setDayTime(6000);
    world.setRaining(false);
    world.setThundering(true);

    i32 stormyDarkening = world.getSkyDarkening();
    EXPECT_GE(stormyDarkening, 10); // 雷暴至少增加 10
}

TEST_F(LightLevelTest, GetSkyDarkening_ThunderingAtNoon_IsHigh)
{
    // 正午雷暴：天空减暗因子应该很高
    // 这允许敌对生物在白天生成
    world.setDayTime(6000);
    world.setRaining(false);
    world.setThundering(true);

    i32 darkening = world.getSkyDarkening();
    // 雷暴时天空减暗至少 10，这样露天位置的光照约为 15-10=5
    // 这足够暗，敌对生物可以生成
    EXPECT_GE(darkening, 10);
}

// ============================================================================
// getLight 测试
// ============================================================================

TEST_F(LightLevelTest, GetLight_UsesCurrentSkyDarkening)
{
    // 设置一个位置
    BlockPos pos(0, 64, 0);

    // 正午，天空光照 15
    world.setDayTime(6000);
    world.setSkyLightValue(15);
    world.setBlockLightValue(0);
    world.setRaining(false);
    world.setThundering(false);

    // 正午天空减暗为 0，所以光照 = max(0, 15-0) = 15
    u8 noonLight = world.getLight(pos);
    EXPECT_EQ(noonLight, 15);

    // 午夜，天空光照 15
    world.setDayTime(18000);
    // 午夜天空减暗约为 11，所以光照 = max(0, 15-11) = 4
    u8 midnightLight = world.getLight(pos);
    EXPECT_LE(midnightLight, 5); // 天空减暗因子约 11
}

TEST_F(LightLevelTest, GetLight_ThunderingAtNoon_IsDarkEnoughForSpawning)
{
    // 正午雷暴：测试敌对生物是否可以在露天位置生成
    BlockPos pos(0, 64, 0);

    world.setDayTime(6000);
    world.setSkyLightValue(15);
    world.setBlockLightValue(0);
    world.setRaining(false);
    world.setThundering(true);

    // 正午雷暴时，天空减暗因子 >= 10
    // 光照 = max(0, 15-10) = 5 或更低
    u8 light = world.getLight(pos);

    // 光照应该 <= 7，敌对生物可以生成
    // 根据 MC 1.16.5 isValidLightLevel：light <= random.nextInt(8)
    // 所以光照 <= 7 的位置有概率可以生成
    EXPECT_LE(light, 7);
}

// ============================================================================
// MC 1.16.5 对齐测试
// ============================================================================

TEST_F(LightLevelTest, MC116_IsValidLightLevel_ThunderingAllowsSpawning)
{
    /**
     * MC 1.16.5 MonsterEntity.isValidLightLevel() 逻辑：
     *
     * 1. 天空光照检查：
     *    if (skyLight > random.nextInt(32)) return false;
     *
     * 2. 综合光照检查：
     *    if (isThundering()) {
     *        light = getNeighborAwareLightSubtracted(pos, 10);
     *    } else {
     *        light = getLight(pos);
     *    }
     *    return light <= random.nextInt(8);
     *
     * 关键点：雷暴时使用固定的天空减暗值 10，即使白天也可以生成敌对生物
     */

    // 测试场景：正午雷暴，露天位置
    BlockPos pos(0, 64, 0);
    world.setDayTime(6000);
    world.setSkyLightValue(15); // 露天
    world.setBlockLightValue(0);
    world.setThundering(true);

    // 使用 getNeighborAwareLightSubtracted(pos, 10)
    u8 thunderLight = world.getNeighborAwareLightSubtracted(pos, 10);

    // 光照 = max(0, 15-10) = 5
    // 这足够暗（<= 7），敌对生物有概率生成
    EXPECT_LE(thunderLight, 7);
    EXPECT_EQ(thunderLight, 5); // 精确值

    // 对比：非雷暴的正午
    world.setThundering(false);
    u8 normalLight = world.getLight(pos);

    // 正午正常光照 = 15
    EXPECT_EQ(normalLight, 15);
    // 太亮，敌对生物无法生成
    EXPECT_GT(normalLight, 7);
}

TEST_F(LightLevelTest, MC116_SkyLightCheck_PassesWhenDark)
{
    /**
     * MC 1.16.5 第一阶段检查：skyLight > random.nextInt(32)
     * 如果天空光照 <= 随机阈值（0-31），则通过
     *
     * 天空光照范围是 0-15，所以：
     * - 天空光照 = 0 时，总是通过（0 <= 任何 0-31 的数）
     * - 天空光照 = 15 时，有 15/32 ≈ 47% 概率被拒绝
     */

    // 室内位置，天空光照 = 0
    world.setSkyLightValue(0);
    // 第一阶段总是通过（0 <= random.nextInt(32)）
    // 然后进入第二阶段检查

    // 方块光照 = 7（临界值）
    world.setBlockLightValue(7);
    world.setDayTime(6000);

    // 光照 = max(7, 0) = 7
    // 7 <= random.nextInt(8) 有 50% 概率通过
    u8 light = world.getLight(BlockPos(0, 64, 0));
    EXPECT_EQ(light, 7);
}

} // namespace test
} // namespace mc
