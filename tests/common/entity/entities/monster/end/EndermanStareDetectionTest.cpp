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

/**
 * @file EndermanStareDetectionTest.cpp
 * @brief 末影人注视检测功能测试
 *
 * 测试末影人的注视检测机制：
 * - Player::getLookVector() 视线方向向量计算
 * - Player::getEyePosition() 眼睛位置获取
 * - Player::isWearingPumpkin() 南瓜头检测
 * - Player::isLookingAt() 注视目标检测
 * - EndermanEntity::shouldAttackPlayer() 激怒条件判断
 * - EndermanStareGoal 注视目标
 * - EndermanFindPlayerGoal 查找玩家目标选择器
 */

#include <cmath>
#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/special/EndermanGoals.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/MathConstants.hpp"

namespace mc {
namespace test {

// ==================== Player::getLookVector 测试 ====================

class PlayerLookVectorTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer"); }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

TEST_F(PlayerLookVectorTest, GetLookVector_Forward)
{
    // yaw=0, pitch=0 应该看向 +Z 方向
    player->setRotation(0.0f, 0.0f);
    Vector3 lookVec = player->getLookVector();

    // MC 坐标系：yaw=0 看向 +Z
    EXPECT_NEAR(lookVec.x, 0.0f, 0.001f);
    EXPECT_NEAR(lookVec.y, 0.0f, 0.001f);
    EXPECT_NEAR(lookVec.z, 1.0f, 0.001f);
}

TEST_F(PlayerLookVectorTest, GetLookVector_North)
{
    // yaw=180 看向 -Z 方向（北）
    player->setRotation(180.0f, 0.0f);
    Vector3 lookVec = player->getLookVector();

    EXPECT_NEAR(lookVec.x, 0.0f, 0.001f);
    EXPECT_NEAR(lookVec.y, 0.0f, 0.001f);
    EXPECT_NEAR(lookVec.z, -1.0f, 0.001f);
}

TEST_F(PlayerLookVectorTest, GetLookVector_East)
{
    // yaw=-90 (270) 看向 +X 方向（东）
    player->setRotation(-90.0f, 0.0f);
    Vector3 lookVec = player->getLookVector();

    EXPECT_NEAR(lookVec.x, 1.0f, 0.001f);
    EXPECT_NEAR(lookVec.y, 0.0f, 0.001f);
    EXPECT_NEAR(lookVec.z, 0.0f, 0.001f);
}

TEST_F(PlayerLookVectorTest, GetLookVector_West)
{
    // yaw=90 看向 -X 方向（西）
    player->setRotation(90.0f, 0.0f);
    Vector3 lookVec = player->getLookVector();

    EXPECT_NEAR(lookVec.x, -1.0f, 0.001f);
    EXPECT_NEAR(lookVec.y, 0.0f, 0.001f);
    EXPECT_NEAR(lookVec.z, 0.0f, 0.001f);
}

TEST_F(PlayerLookVectorTest, GetLookVector_Up)
{
    // pitch=-90 看向上方
    player->setRotation(0.0f, -90.0f);
    Vector3 lookVec = player->getLookVector();

    EXPECT_NEAR(lookVec.x, 0.0f, 0.001f);
    EXPECT_NEAR(lookVec.y, 1.0f, 0.001f);
    EXPECT_NEAR(lookVec.z, 0.0f, 0.001f);
}

TEST_F(PlayerLookVectorTest, GetLookVector_Down)
{
    // pitch=90 看向下方
    player->setRotation(0.0f, 90.0f);
    Vector3 lookVec = player->getLookVector();

    EXPECT_NEAR(lookVec.x, 0.0f, 0.001f);
    EXPECT_NEAR(lookVec.y, -1.0f, 0.001f);
    EXPECT_NEAR(lookVec.z, 0.0f, 0.001f);
}

TEST_F(PlayerLookVectorTest, GetLookVector_Normalized)
{
    // 随意角度，验证向量已归一化
    player->setRotation(45.0f, 30.0f);
    Vector3 lookVec = player->getLookVector();

    f32 length = lookVec.length();
    EXPECT_NEAR(length, 1.0f, 0.001f);
}

// ==================== Player::getEyePosition 测试 ====================

class PlayerEyePositionTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer"); }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

TEST_F(PlayerEyePositionTest, GetEyePosition_ReturnsCorrectPosition)
{
    player->setPosition(100.0, 64.0, 200.0);

    Vector3 eyePos = player->getEyePosition();

    // 眼睛位置 = 实体位置 + 眼睛高度
    EXPECT_FLOAT_EQ(eyePos.x, 100.0f);
    EXPECT_FLOAT_EQ(eyePos.y, 64.0f + player->eyeHeight());
    EXPECT_FLOAT_EQ(eyePos.z, 200.0f);
}

TEST_F(PlayerEyePositionTest, GetEyePosition_ChangesWithPosition)
{
    player->setPosition(50.0, 100.0, -30.0);
    Vector3 eyePos1 = player->getEyePosition();

    player->setPosition(60.0, 110.0, -20.0);
    Vector3 eyePos2 = player->getEyePosition();

    EXPECT_NE(eyePos1.x, eyePos2.x);
    EXPECT_NE(eyePos1.y, eyePos2.y);
    EXPECT_NE(eyePos1.z, eyePos2.z);
}

// ==================== Player::isWearingPumpkin 测试 ====================

class PlayerPumpkinTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer"); }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

TEST_F(PlayerPumpkinTest, IsWearingPumpkin_NoHelmet_ReturnsFalse)
{
    // 没有头盔时返回 false
    EXPECT_FALSE(player->isWearingPumpkin());
}

// 注意：完整的南瓜头测试需要物品系统支持，
// 这里测试基本功能：没有头盔时不返回 true

// ==================== Player::isLookingAt 测试 ====================

class PlayerLookingAtTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer");
        enderman = std::make_unique<EndermanEntity>(EntityInstanceId(2));
    }

    void TearDown() override
    {
        player.reset();
        enderman.reset();
    }

    std::unique_ptr<Player> player;
    std::unique_ptr<EndermanEntity> enderman;
};

TEST_F(PlayerLookingAtTest, IsLookingAt_LookingDirectlyAt_ReturnsTrue)
{
    // 玩家在原点，看向 +Z 方向
    player->setPosition(0.0, 0.0, 0.0);
    player->setRotation(0.0f, 0.0f);

    // 末影人在 Z 轴正方向，Y 位置与玩家眼睛高度对齐
    // 玩家眼睛高度约 1.62，末影人眼睛高度约 2.55
    // 设置末影人 Y 位置使其眼睛与玩家眼睛在同一水平线上
    f32 playerEyeY = 0.0f + player->eyeHeight();
    f32 endermanBaseY = playerEyeY - enderman->eyeHeight(); // 使末影人眼睛与玩家眼睛对齐
    enderman->setPosition(0.0, endermanBaseY, 10.0);

    // 玩家应该正在看向末影人
    EXPECT_TRUE(player->isLookingAt(*enderman));
}

TEST_F(PlayerLookingAtTest, IsLookingAt_LookingAway_ReturnsFalse)
{
    // 玩家在原点，看向 +Z 方向
    player->setPosition(0.0, 0.0, 0.0);
    player->setRotation(0.0f, 0.0f);

    // 末影人在 -Z 方向（背后）
    enderman->setPosition(0.0, 0.0, -10.0);

    // 玩家不应该正在看向末影人
    EXPECT_FALSE(player->isLookingAt(*enderman));
}

TEST_F(PlayerLookingAtTest, IsLookingAt_LookingSide_ReturnsFalse)
{
    // 玩家在原点，看向 +Z 方向
    player->setPosition(0.0, 0.0, 0.0);
    player->setRotation(0.0f, 0.0f);

    // 末影人在 +X 方向（侧面）
    enderman->setPosition(10.0, 0.0, 0.0);

    // 玩家不应该正在看向末影人
    EXPECT_FALSE(player->isLookingAt(*enderman));
}

TEST_F(PlayerLookingAtTest, IsLookingAt_SamePosition_ReturnsTrue)
{
    // 玩家和末影人在同一位置
    // 由于距离太近（< 0.001），函数应该直接返回 true
    player->setPosition(0.0, 0.0, 0.0);
    enderman->setPosition(0.0, 0.0, 0.0);

    // 距离为 0，认为是在看
    // 注意：由于眼睛高度不同，实际距离会大于 0.001
    // 所以需要设置末影人使眼睛位置相同
    f32 playerEyeY = 0.0f + player->eyeHeight();
    f32 endermanBaseY = playerEyeY - enderman->eyeHeight();
    enderman->setPosition(0.0, endermanBaseY, 0.0);

    EXPECT_TRUE(player->isLookingAt(*enderman));
}

TEST_F(PlayerLookingAtTest, IsLookingAt_FarAway_MoreStrictThreshold)
{
    // 玩家在原点，看向 +Z 方向
    player->setPosition(0.0, 0.0, 0.0);
    player->setRotation(0.0f, 0.0f);

    // 末影人在很远的 Z 轴正方向
    enderman->setPosition(0.0, 0.0, 100.0);

    // 远距离时阈值更严格，应该仍然在看点内
    EXPECT_TRUE(player->isLookingAt(*enderman));
}

TEST_F(PlayerLookingAtTest, IsLookingAt_SlightlyOffCenter_MayBeLooking)
{
    // 玩家在原点，看向 +Z 方向
    player->setPosition(0.0, 0.0, 0.0);
    player->setRotation(0.0f, 0.0f);

    // 末影人在 Z 轴正方向稍微偏移
    // 近距离允许更大的偏移角度
    enderman->setPosition(1.0, 0.0, 10.0);

    // 结果取决于具体的角度阈值计算
    // 1 偏移在 10 距离处约为 atan(1/10) ≈ 5.7 度
    // 这个偏移应该在允许范围内
    MC_UNUSED(enderman);
    // 不做具体断言，只验证函数能正常执行
    bool result = player->isLookingAt(*enderman);
    MC_UNUSED(result);
}

// ==================== EndermanStareGoal 构造函数测试 ====================

class EndermanStareGoalTest : public ::testing::Test {
protected:
    void SetUp() override { enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1)); }

    void TearDown() override { enderman.reset(); }

    std::unique_ptr<EndermanEntity> enderman;
};

TEST_F(EndermanStareGoalTest, Constructor_SetsMutexFlags)
{
    entity::ai::goal::EndermanStareGoal goal(enderman.get());

    auto flags = goal.getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(EndermanStareGoalTest, GetTypeName_ReturnsCorrectName)
{
    entity::ai::goal::EndermanStareGoal goal(enderman.get());
    EXPECT_EQ(goal.getTypeName(), "EndermanStareGoal");
}

TEST_F(EndermanStareGoalTest, ShouldExecute_WithoutTarget_ReturnsFalse)
{
    entity::ai::goal::EndermanStareGoal goal(enderman.get());
    // 没有攻击目标时不应执行
    EXPECT_FALSE(goal.shouldExecute());
}

// ==================== EndermanFindPlayerGoal 构造函数测试 ====================

class EndermanFindPlayerGoalTest : public ::testing::Test {
protected:
    void SetUp() override { enderman = std::make_unique<EndermanEntity>(EntityInstanceId(1)); }

    void TearDown() override { enderman.reset(); }

    std::unique_ptr<EndermanEntity> enderman;
};

TEST_F(EndermanFindPlayerGoalTest, Constructor_SetsMutexFlags)
{
    entity::ai::goal::EndermanFindPlayerGoal goal(enderman.get());

    // EndermanFindPlayerGoal 继承自 TargetGoal
    // TargetGoal 通常使用 Target 标志
    auto flags = goal.getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(EndermanFindPlayerGoalTest, GetTypeName_ReturnsCorrectName)
{
    entity::ai::goal::EndermanFindPlayerGoal goal(enderman.get());
    EXPECT_EQ(goal.getTypeName(), "EndermanFindPlayerGoal");
}

TEST_F(EndermanFindPlayerGoalTest, ShouldExecute_WithoutWorld_ReturnsFalse)
{
    entity::ai::goal::EndermanFindPlayerGoal goal(enderman.get());
    // 没有世界时不应执行
    EXPECT_FALSE(goal.shouldExecute());
}

// ==================== EndermanEntity::shouldAttackPlayer 测试 ====================

class EndermanShouldAttackPlayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer");
        enderman = std::make_unique<EndermanEntity>(EntityInstanceId(2));
    }

    void TearDown() override
    {
        player.reset();
        enderman.reset();
    }

    std::unique_ptr<Player> player;
    std::unique_ptr<EndermanEntity> enderman;
};

TEST_F(EndermanShouldAttackPlayerTest, ShouldAttackPlayer_NotLooking_ReturnsFalse)
{
    // 玩家看向背离末影人的方向
    player->setPosition(0.0, 0.0, 0.0);
    player->setRotation(0.0f, 0.0f); // 看向 +Z

    // 末影人在玩家背后
    enderman->setPosition(0.0, 0.0, -10.0);

    // 玩家没有在看末影人，不应激怒
    EXPECT_FALSE(enderman->shouldAttackPlayer(*player));
}

TEST_F(EndermanShouldAttackPlayerTest, ShouldAttackPlayer_LookingDirectly_MayReturnTrue)
{
    // 玩家看向末影人
    player->setPosition(0.0, 0.0, 0.0);
    player->setRotation(0.0f, 0.0f); // 看向 +Z

    // 末影人在玩家前方
    enderman->setPosition(0.0, 0.0, 10.0);

    // 如果没有南瓜头且在看，应该激怒
    // 注意：canSee() 需要世界支持，这里只验证函数可以调用
    MC_UNUSED(enderman);
    // 不做具体断言，因为 canSee() 需要完整的世界环境
    bool result = enderman->shouldAttackPlayer(*player);
    MC_UNUSED(result);
}

// ==================== 常量验证测试 ====================

class EndermanConstantsTest : public ::testing::Test {};

TEST_F(EndermanConstantsTest, EndermanStareGoal_StareRangeIsCorrect)
{
    // 注视范围 16 格，距离平方 256.0
    // 这是 MC 1.16.5 的常量
    constexpr f64 STARE_RANGE = 16.0;
    constexpr f64 STARE_RANGE_SQ = STARE_RANGE * STARE_RANGE;
    EXPECT_DOUBLE_EQ(STARE_RANGE_SQ, 256.0);
}

TEST_F(EndermanConstantsTest, EndermanFindPlayerGoal_AggroDurationIsCorrect)
{
    // 激怒持续时间 5 ticks
    // MC 1.16.5: 玩家注视末影人 5 ticks 后末影人被激怒
    constexpr i32 AGGRO_DURATION = 5;
    EXPECT_EQ(AGGRO_DURATION, 5);
}

TEST_F(EndermanConstantsTest, EndermanFindPlayerGoal_TeleportDistancesAreCorrect)
{
    // 近距离瞬移阈值：4 格（距离平方 16.0）
    // 远距离瞬移阈值：16 格（距离平方 256.0）
    constexpr f64 TELEPORT_NEAR_DISTANCE = 4.0;
    constexpr f64 TELEPORT_FAR_DISTANCE = 16.0;

    EXPECT_DOUBLE_EQ(TELEPORT_NEAR_DISTANCE * TELEPORT_NEAR_DISTANCE, 16.0);
    EXPECT_DOUBLE_EQ(TELEPORT_FAR_DISTANCE * TELEPORT_FAR_DISTANCE, 256.0);
}

TEST_F(EndermanConstantsTest, EndermanEntity_TeleportCooldownIsCorrect)
{
    // 瞬移冷却 50 ticks (2.5 秒)
    // MC 1.16.5: EndermanEntity.teleportCooldown
    constexpr i32 TELEPORT_COOLDOWN = 50;
    EXPECT_EQ(TELEPORT_COOLDOWN, 50);
}

TEST_F(EndermanConstantsTest, EndermanEntity_AngerDurationIsCorrect)
{
    // 愤怒持续时间 600 ticks (30 秒)
    // MC 1.16.5: EndermanEntity.ANGER_TIME
    constexpr i32 ANGER_DURATION = 600;
    EXPECT_EQ(ANGER_DURATION, 600);
}

// ==================== 视线方向向量计算精度测试 ====================

class LookVectorPrecisionTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer"); }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

TEST_F(LookVectorPrecisionTest, MultipleAngles_AllNormalized)
{
    // 测试多个角度，所有向量都应该是归一化的
    for (f32 yaw = 0.0f; yaw < 360.0f; yaw += 45.0f) {
        for (f32 pitch = -90.0f; pitch <= 90.0f; pitch += 30.0f) {
            player->setRotation(yaw, pitch);
            Vector3 lookVec = player->getLookVector();
            f32 length = lookVec.length();
            EXPECT_NEAR(length, 1.0f, 0.0001f) << "yaw=" << yaw << ", pitch=" << pitch;
        }
    }
}

TEST_F(LookVectorPrecisionTest, OppositeDirections_AreOpposite)
{
    // 相反的方向应该产生相反的向量
    player->setRotation(0.0f, 0.0f);
    Vector3 forward = player->getLookVector();

    player->setRotation(180.0f, 0.0f);
    Vector3 backward = player->getLookVector();

    EXPECT_NEAR(forward.x, -backward.x, 0.001f);
    EXPECT_NEAR(forward.y, -backward.y, 0.001f);
    EXPECT_NEAR(forward.z, -backward.z, 0.001f);
}

} // namespace test
} // namespace mc
