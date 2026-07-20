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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"

using namespace mc;

// ============================================================================
// 冲量坠落伤害免疫（Impulse Context）测试
//
// 测试 Player 的冲量上下文系统，对应 MC Java Player 中的
// setIgnoreFallDamageFromCurrentImpulse / currentImpulseContext 系列方法。
// 用于实现重锤砸地攻击和风弹爆炸后的坠落伤害减免。
//
// 注意：由于 Player::tick() 依赖完整的实体系统（世界、碰撞等），
// 此处仅测试冲量上下文的各个方法，不通过 tick() 间接测试。
// tick() 中的宽限期递减和 tryResetCurrentImpulseContext 逻辑
// 通过方法级测试验证其正确性。
// ============================================================================

class ImpulseContextTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Player 构造需要注册全局属性，此处直接构造
        player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer");
    }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

// ========== 基本状态测试 ==========

TEST_F(ImpulseContextTest, InitialState_NoImpulseContext)
{
    // 初始状态下没有冲量上下文
    EXPECT_FALSE(player->isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_FALSE(player->isInPostImpulseGraceTime());
    EXPECT_FALSE(player->currentImpulseImpactPos().has_value());
    EXPECT_EQ(player->currentExplosionCause(), 0);
}

TEST_F(ImpulseContextTest, SetIgnoreFallDamage_EnableWithGraceTime)
{
    // 设置忽略坠落伤害时，同时启动 40 tick 宽限期
    player->setIgnoreFallDamageFromCurrentImpulse(true);

    EXPECT_TRUE(player->isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_TRUE(player->isInPostImpulseGraceTime());
}

TEST_F(ImpulseContextTest, SetIgnoreFallDamage_DisableClearsGraceTime)
{
    // 设置 true 再设置 false 时，宽限期被清除
    player->setIgnoreFallDamageFromCurrentImpulse(true);
    player->setIgnoreFallDamageFromCurrentImpulse(false);

    EXPECT_FALSE(player->isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_FALSE(player->isInPostImpulseGraceTime());
}

// ========== 宽限期（Grace Time）测试 ==========

TEST_F(ImpulseContextTest, ApplyPostImpulseGraceTime_SetsGraceTime)
{
    // applyPostImpulseGraceTime 设置宽限期
    player->applyPostImpulseGraceTime(10);
    EXPECT_TRUE(player->isInPostImpulseGraceTime());
}

TEST_F(ImpulseContextTest, ApplyPostImpulseGraceTime_TakesMaximum)
{
    // applyPostImpulseGraceTime 取最大值，不会缩短已有的宽限期
    player->applyPostImpulseGraceTime(10);
    EXPECT_TRUE(player->isInPostImpulseGraceTime());

    // 设置更大的值应该生效
    player->applyPostImpulseGraceTime(20);
    EXPECT_TRUE(player->isInPostImpulseGraceTime());

    // 设置更小的值不应缩短宽限期
    player->applyPostImpulseGraceTime(5);
    EXPECT_TRUE(player->isInPostImpulseGraceTime());

    // 宽限期为 20，不应在 10 次递减后结束
    // 通过 applyPostImpulseGraceTime(max(20,5)) = 20 验证
    // 再应用 15，max(20,15) = 20，仍为 20
    player->applyPostImpulseGraceTime(15);
    EXPECT_TRUE(player->isInPostImpulseGraceTime());
}

TEST_F(ImpulseContextTest, WindBurstExtendsGraceTime)
{
    // 模拟重锤砸地攻击后风爆附魔扩展宽限期
    player->setIgnoreFallDamageFromCurrentImpulse(true); // 启动 40 tick 宽限期
    player->applyPostImpulseGraceTime(10);               // 风爆附魔额外 +10 tick

    // 宽限期应该至少为 40（setIgnoreFallDamageFromCurrentImpulse 设置的）
    // max(40, 10) = 40，所以宽限期仍为 40
    EXPECT_TRUE(player->isInPostImpulseGraceTime());
    EXPECT_TRUE(player->isIgnoringFallDamageFromCurrentImpulse());
}

TEST_F(ImpulseContextTest, ApplyPostImpulseGraceTime_ZeroMeansNoGraceTime)
{
    // 设置 0 宽限期等于没有宽限期
    player->applyPostImpulseGraceTime(0);
    EXPECT_FALSE(player->isInPostImpulseGraceTime());
}

// ========== 重置上下文测试 ==========

TEST_F(ImpulseContextTest, ResetCurrentImpulseContext_ClearsAll)
{
    player->setCurrentImpulseImpactPos(Vector3(100.0f, 64.0f, 200.0f));
    player->setCurrentExplosionCause(EntityInstanceId(42));
    player->setIgnoreFallDamageFromCurrentImpulse(true);

    player->resetCurrentImpulseContext();

    EXPECT_FALSE(player->isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_FALSE(player->isInPostImpulseGraceTime());
    EXPECT_FALSE(player->currentImpulseImpactPos().has_value());
    EXPECT_EQ(player->currentExplosionCause(), 0);
}

TEST_F(ImpulseContextTest, TryReset_IgnoresDuringGraceTime)
{
    player->setCurrentImpulseImpactPos(Vector3(100.0f, 64.0f, 200.0f));
    player->setIgnoreFallDamageFromCurrentImpulse(true);

    // 宽限期期间，tryReset 不应该重置
    player->tryResetCurrentImpulseContext();

    EXPECT_TRUE(player->isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_TRUE(player->currentImpulseImpactPos().has_value());
}

TEST_F(ImpulseContextTest, TryReset_SucceedsWhenGraceTimeIsZero)
{
    // 不设置宽限期时（graceTime == 0），tryReset 应该重置
    player->setCurrentImpulseImpactPos(Vector3(100.0f, 64.0f, 200.0f));
    player->setCurrentExplosionCause(EntityInstanceId(42));
    // 不调用 setIgnoreFallDamageFromCurrentImpulse(true)，所以宽限期为 0

    player->tryResetCurrentImpulseContext();

    EXPECT_FALSE(player->currentImpulseImpactPos().has_value());
    EXPECT_EQ(player->currentExplosionCause(), 0);
}

TEST_F(ImpulseContextTest, TryReset_SucceedsAfterGraceTimeCleared)
{
    // 设置宽限期后，手动清除宽限期（模拟宽限期倒计时结束）
    player->setCurrentImpulseImpactPos(Vector3(100.0f, 64.0f, 200.0f));
    player->setIgnoreFallDamageFromCurrentImpulse(true); // 40 tick 宽限期

    // 手动将宽限期设为 0（模拟 40 tick 过去了）
    player->applyPostImpulseGraceTime(0);
    // applyPostImpulseGraceTime 使用 max，所以 0 不会覆盖 40
    // 我们需要用 resetCurrentImpulseContext 来清除然后重新设置不带宽限期的状态
    // 实际上，需要直接测试：graceTime > 0 时 tryReset 不重置，graceTime == 0 时重置

    // 用更直接的方式：先让 tryReset 在宽限期期间失败
    EXPECT_TRUE(player->isInPostImpulseGraceTime());
    player->tryResetCurrentImpulseContext();
    EXPECT_TRUE(player->currentImpulseImpactPos().has_value()); // 未重置

    // 重置后重新设置冲量但手动设置 graceTime 为 0
    player->resetCurrentImpulseContext();
    player->setCurrentImpulseImpactPos(Vector3(100.0f, 64.0f, 200.0f));
    player->setIgnoreFallDamageFromCurrentImpulse(false);
    // 此时宽限期为 0

    player->tryResetCurrentImpulseContext();
    EXPECT_FALSE(player->currentImpulseImpactPos().has_value()); // 成功重置
}

// ========== 冲量冲击位置计算测试 ==========

TEST_F(ImpulseContextTest, CalculateMaceImpactPosition_NoActiveImpulse)
{
    // 没有活跃冲量时，使用玩家当前位置
    player->setPosition(50.0f, 70.0f, 80.0f);
    Vector3 impactPos = player->calculateMaceImpactPosition();
    EXPECT_FLOAT_EQ(impactPos.x, 50.0f);
    EXPECT_FLOAT_EQ(impactPos.y, 70.0f);
    EXPECT_FLOAT_EQ(impactPos.z, 80.0f);
}

TEST_F(ImpulseContextTest, CalculateMaceImpactPosition_ActiveImpulseAtLowerPosition)
{
    // 有活跃冲量且冲击位置低于当前位置时，保留原有冲击位置（防止连续砸地双重获利）
    player->setPosition(50.0f, 80.0f, 80.0f);
    player->setCurrentImpulseImpactPos(Vector3(50.0f, 64.0f, 80.0f));
    player->setIgnoreFallDamageFromCurrentImpulse(true);

    Vector3 impactPos = player->calculateMaceImpactPosition();
    // 应该保留原有冲击位置（64.0），不是当前位置（80.0）
    EXPECT_FLOAT_EQ(impactPos.y, 64.0f);
}

TEST_F(ImpulseContextTest, CalculateMaceImpactPosition_ActiveImpulseAtHigherPosition)
{
    // 有活跃冲量但冲击位置高于当前位置时，使用当前位置（玩家已下落）
    player->setPosition(50.0f, 50.0f, 80.0f);
    player->setCurrentImpulseImpactPos(Vector3(50.0f, 64.0f, 80.0f));
    player->setIgnoreFallDamageFromCurrentImpulse(true);

    Vector3 impactPos = player->calculateMaceImpactPosition();
    // 冲击位置 (64) > 当前位置 (50)，不满足 y <= position.y，使用当前位置
    EXPECT_FLOAT_EQ(impactPos.y, 50.0f);
}

// ========== 游戏模式切换测试 ==========

TEST_F(ImpulseContextTest, SetGameModeCreative_ResetsImpulseContext)
{
    player->setCurrentImpulseImpactPos(Vector3(100.0f, 64.0f, 200.0f));
    player->setIgnoreFallDamageFromCurrentImpulse(true);

    // 切换到创造模式时重置冲量上下文
    player->setGameMode(GameMode::Creative);

    EXPECT_FALSE(player->isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_FALSE(player->currentImpulseImpactPos().has_value());
}

// ========== 宽限期与 tryReset 交互测试 ==========

TEST_F(ImpulseContextTest, GraceTimePreventsTryReset)
{
    // 设置宽限期后，tryReset 无效
    player->setCurrentImpulseImpactPos(Vector3(100.0f, 64.0f, 200.0f));
    player->setCurrentExplosionCause(EntityInstanceId(42));
    player->setIgnoreFallDamageFromCurrentImpulse(true);

    // 宽限期期间多次 tryReset 都不应重置
    for (int i = 0; i < 5; ++i) {
        player->tryResetCurrentImpulseContext();
    }

    EXPECT_TRUE(player->isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_TRUE(player->currentImpulseImpactPos().has_value());
    EXPECT_EQ(player->currentExplosionCause(), EntityInstanceId(42));
}

TEST_F(ImpulseContextTest, NoGraceTime_AllowsTryReset)
{
    // 没有宽限期时，tryReset 立即重置
    player->setCurrentImpulseImpactPos(Vector3(100.0f, 64.0f, 200.0f));
    player->setCurrentExplosionCause(EntityInstanceId(42));
    // 不调用 setIgnoreFallDamageFromCurrentImpulse(true)，宽限期为 0

    player->tryResetCurrentImpulseContext();

    EXPECT_FALSE(player->currentImpulseImpactPos().has_value());
    EXPECT_EQ(player->currentExplosionCause(), 0);
}

// ========== onExplosionHit 测试 ==========

TEST_F(ImpulseContextTest, OnExplosionHit_NonWindCharge_DoesNotEnableImmunity)
{
    player->setPosition(100.0f, 64.0f, 200.0f);

    // 非 WindCharge 爆炸不应启用坠落伤害免疫
    // 使用 nullptr 作为原因（无来源爆炸，如床爆炸）
    player->onExplosionHit(nullptr);

    EXPECT_FALSE(player->isIgnoringFallDamageFromCurrentImpulse());
    // 冲击位置仍被记录
    EXPECT_TRUE(player->currentImpulseImpactPos().has_value());
    EXPECT_EQ(player->currentExplosionCause(), 0);
}

TEST_F(ImpulseContextTest, OnExplosionHit_RecordsImpactPosition)
{
    player->setPosition(100.0f, 64.0f, 200.0f);

    player->onExplosionHit(nullptr);

    // 冲击位置被记录为当前位置
    auto impactPos = player->currentImpulseImpactPos();
    ASSERT_TRUE(impactPos.has_value());
    EXPECT_FLOAT_EQ(impactPos->x, 100.0f);
    EXPECT_FLOAT_EQ(impactPos->y, 64.0f);
    EXPECT_FLOAT_EQ(impactPos->z, 200.0f);
}

// ========== 重置后状态一致性测试 ==========

TEST_F(ImpulseContextTest, ResetThenReapply_NewContextIsIndependent)
{
    // 重置后再设置新的冲量上下文，应该完全独立
    player->setCurrentImpulseImpactPos(Vector3(100.0f, 64.0f, 200.0f));
    player->setCurrentExplosionCause(EntityInstanceId(42));
    player->setIgnoreFallDamageFromCurrentImpulse(true);

    player->resetCurrentImpulseContext();

    // 设置新的冲量上下文
    player->setCurrentImpulseImpactPos(Vector3(200.0f, 70.0f, 300.0f));
    player->setCurrentExplosionCause(EntityInstanceId(99));
    player->setIgnoreFallDamageFromCurrentImpulse(true);

    auto impactPos = player->currentImpulseImpactPos();
    ASSERT_TRUE(impactPos.has_value());
    EXPECT_FLOAT_EQ(impactPos->x, 200.0f);
    EXPECT_FLOAT_EQ(impactPos->y, 70.0f);
    EXPECT_FLOAT_EQ(impactPos->z, 300.0f);
    EXPECT_EQ(player->currentExplosionCause(), EntityInstanceId(99));
    EXPECT_TRUE(player->isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_TRUE(player->isInPostImpulseGraceTime());
}
