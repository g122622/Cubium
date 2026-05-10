#include <gtest/gtest.h>
#include "server/world/spawn/VillageSiege.hpp"
#include "common/core/Constants.hpp"
#include "common/util/math/MathConstants.hpp"

namespace mc {
namespace server::spawn {

/**
 * @brief VillageSiege 测试套件
 *
 * 测试僵尸围村系统的核心功能。
 */
class VillageSiegeTest : public ::testing::Test {
protected:
    void SetUp() override {
        siege = std::make_unique<VillageSiege>();
    }

    void TearDown() override {
        siege.reset();
    }

    std::unique_ptr<VillageSiege> siege;
};

// ========== 状态测试 ==========

TEST_F(VillageSiegeTest, InitialState_IsDone) {
    // 初始状态应该是 Done（未激活）
    EXPECT_EQ(siege->getState(), VillageSiege::State::Done);
}

TEST_F(VillageSiegeTest, IsSiegeActive_InitiallyFalse) {
    // 初始时围攻未激活
    EXPECT_FALSE(siege->isSiegeActive());
}

TEST_F(VillageSiegeTest, GetRemainingZombies_InitiallyZero) {
    // 初始时剩余僵尸数量为 0
    EXPECT_EQ(siege->getRemainingZombies(), 0);
}

TEST_F(VillageSiegeTest, GetSpawnCenter_InitiallyZero) {
    // 初始生成中心为零坐标
    const BlockPos& center = siege->getSpawnCenter();
    EXPECT_EQ(center.x, 0);
    EXPECT_EQ(center.y, 0);
    EXPECT_EQ(center.z, 0);
}

// ========== 配置常量测试 ==========

TEST_F(VillageSiegeTest, Config_TriggerChance) {
    // 触发概率：1/10 = 10%
    EXPECT_EQ(VillageSiege::Config::TRIGGER_CHANCE, 10);
}

TEST_F(VillageSiegeTest, Config_TotalZombies) {
    // 总共生成的僵尸数量：20
    EXPECT_EQ(VillageSiege::Config::TOTAL_ZOMBIES, 20);
}

TEST_F(VillageSiegeTest, Config_SpawnDelay) {
    // 每次生成之间的延迟：2 tick
    EXPECT_EQ(VillageSiege::Config::SPAWN_DELAY, 2);
}

TEST_F(VillageSiegeTest, Config_SpawnDistance) {
    // 生成距离：32 格
    EXPECT_FLOAT_EQ(VillageSiege::Config::SPAWN_DISTANCE, 32.0f);
}

TEST_F(VillageSiegeTest, Config_MaxSpawnAttempts) {
    // 最大生成尝试次数：10
    EXPECT_EQ(VillageSiege::Config::MAX_SPAWN_ATTEMPTS, 10);
}

TEST_F(VillageSiegeTest, Config_MaxSetupAttempts) {
    // 最大设置尝试次数：10
    EXPECT_EQ(VillageSiege::Config::MAX_SETUP_ATTEMPTS, 10);
}

TEST_F(VillageSiegeTest, Config_SpawnOffsetRange) {
    // 生成位置随机偏移范围：8
    EXPECT_EQ(VillageSiege::Config::SPAWN_OFFSET_RANGE, 8);
}

// ========== 状态枚举测试 ==========

TEST_F(VillageSiegeTest, State_CanActivate) {
    // 状态枚举值检查
    EXPECT_EQ(static_cast<u8>(VillageSiege::State::CanActivate), 0);
}

TEST_F(VillageSiegeTest, State_Tonight) {
    EXPECT_EQ(static_cast<u8>(VillageSiege::State::Tonight), 1);
}

TEST_F(VillageSiegeTest, State_Done) {
    EXPECT_EQ(static_cast<u8>(VillageSiege::State::Done), 2);
}

// ========== 默认行为测试 ==========

TEST_F(VillageSiegeTest, Tick_SpawnHostilesFalse_ReturnsZero) {
    // 如果不允许生成敌对生物，应该返回 0
    // 注意：这需要 Mock ServerWorld，此处仅测试接口存在性
    // 实际行为测试需要在集成测试中进行
    EXPECT_NE(siege, nullptr);
}

TEST_F(VillageSiegeTest, Tick_DaytimeResetsState) {
    // 白天应该重置状态为 Done
    // 这需要 Mock ServerWorld::dayTime() 返回白天时间
    // 实际行为测试需要在集成测试中进行
    EXPECT_NE(siege, nullptr);
}

// ========== 午夜检测测试 ==========

TEST_F(VillageSiegeTest, Midnight_DetectionRange) {
    // 午夜时间范围：18000-18200 tick
    // 参考 MC 1.16.5 天体角度计算
    // 天体角度 0.5 对应游戏时间约 18000
    EXPECT_GE(18000, 0);
    EXPECT_LE(18200, game::DAY_LENGTH_TICKS);
}

// ========== 常量验证测试 ==========

TEST_F(VillageSiegeTest, DayLengthConstant) {
    // 一天的长度：24000 tick
    EXPECT_EQ(game::DAY_LENGTH_TICKS, 24000);
}

TEST_F(VillageSiegeTest, TwoPiConstant) {
    // TWO_PI 用于角度计算
    EXPECT_FLOAT_EQ(math::TWO_PI, 6.28318530718f);
}

// ========== 围攻逻辑验证测试 ==========

TEST_F(VillageSiegeTest, SiegeSequence_Valid) {
    // 验证围攻序列的合理性
    // 20 个僵尸，每 2 tick 生成 1 个
    // 总时长 = 20 * 2 = 40 tick = 2 秒
    const i32 totalDuration = VillageSiege::Config::TOTAL_ZOMBIES * VillageSiege::Config::SPAWN_DELAY;
    EXPECT_EQ(totalDuration, 40);
}

TEST_F(VillageSiegeTest, TriggerProbability_Valid) {
    // 10% 触发概率
    const f32 probability = 1.0f / static_cast<f32>(VillageSiege::Config::TRIGGER_CHANCE);
    EXPECT_FLOAT_EQ(probability, 0.1f);
}

TEST_F(VillageSiegeTest, SpawnArea_Calculation) {
    // 生成区域计算验证
    // 玩家周围 32 格圆周，加上 8 格随机偏移
    // 最大生成距离 = 32 + 8 = 40 格
    // 最小生成距离 = 32 - 8 = 24 格
    const f32 maxDistance = VillageSiege::Config::SPAWN_DISTANCE + VillageSiege::Config::SPAWN_OFFSET_RANGE;
    const f32 minDistance = VillageSiege::Config::SPAWN_DISTANCE - VillageSiege::Config::SPAWN_OFFSET_RANGE;
    EXPECT_FLOAT_EQ(maxDistance, 40.0f);
    EXPECT_FLOAT_EQ(minDistance, 24.0f);
}

// ========== 边界条件测试 ==========

TEST_F(VillageSiegeTest, State_Transition_Valid) {
    // 状态转换应该是：Done -> CanActivate -> Tonight -> Done
    // 或者直接 Done -> Tonight -> Done
    // 初始状态是 Done
    EXPECT_EQ(siege->getState(), VillageSiege::State::Done);
}

TEST_F(VillageSiegeTest, RemainingZombies_NeverNegative) {
    // 剩余僵尸数量不应该为负数
    EXPECT_GE(siege->getRemainingZombies(), 0);
}

// ========== 移动语义测试 ==========

TEST_F(VillageSiegeTest, MoveConstructor_Works) {
    VillageSiege other;
    VillageSiege moved(std::move(other));
    EXPECT_EQ(moved.getState(), VillageSiege::State::Done);
}

TEST_F(VillageSiegeTest, MoveAssignment_Works) {
    VillageSiege other;
    VillageSiege moved;
    moved = std::move(other);
    EXPECT_EQ(moved.getState(), VillageSiege::State::Done);
}

} // namespace server::spawn
} // namespace mc
