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

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/MathUtils.hpp"

using namespace mc;

namespace {

// ============================================================================
// 测试辅助：创建测试用 LivingEntity
// ============================================================================

class TestLivingEntity : public LivingEntity {
public:
    explicit TestLivingEntity(EntityInstanceId id)
        : LivingEntity(id)
    {
        registerAttributes();
        // 设置基础属性值
        m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0);
        setHealth(20.0f);
    }

    static std::unique_ptr<Entity> create(IWorld* /*world*/) { return std::make_unique<TestLivingEntity>(0); }
};

class TestPlayer : public Player {
public:
    explicit TestPlayer(EntityInstanceId id)
        : Player(id, "TestPlayer")
    {
        registerAttributes();
        m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0);
        m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0);
        setHealth(20.0f);
    }

    // 重写 sendVelocityPacket，记录是否被调用
    [[nodiscard]] bool sendVelocityPacket() override
    {
        m_velocityPacketSent = true;
        return true;
    }

    bool wasVelocityPacketSent() const { return m_velocityPacketSent; }
    void resetVelocityPacketSent() { m_velocityPacketSent = false; }

private:
    bool m_velocityPacketSent = false;
};

// ============================================================================
// LivingEntity::causeExtraKnockback 测试
// ============================================================================

TEST(CauseExtraKnockbackTest, LivingEntityBaseKnockbackAppliesToTarget)
{
    // LivingEntity 基类的 causeExtraKnockback 应该对 LivingEntity 目标施加击退
    TestLivingEntity attacker(1);
    TestLivingEntity target(2);

    // 设置攻击者朝向 yaw=90（看向 -X 方向，sin(90)=1, -cos(90)≈0）
    // 这样击退主要沿 X 方向，容易检测
    attacker.setRotation(90.0f, 0.0f);
    attacker.setVelocity(1.0f, 0.0f, 1.0f);

    Vector3 preHurtVel = target.velocity();

    // 对目标施加额外击退
    attacker.causeExtraKnockback(target, 1.0f, preHurtVel);

    // 击退方向基于攻击者朝向 yaw=90：
    // sin(90°) ≈ 1.0, -cos(90°) ≈ 0
    // 目标应被推向 -X 方向（基于攻击者朝向）
    // 由于初始速度为 (0,0,0)，击退后 X 方向速度应 < 0
    EXPECT_LT(target.velocity().x, 0.0f);
}

TEST(CauseExtraKnockbackTest, LivingEntityBaseSlowsAttacker)
{
    // LivingEntity 基类的 causeExtraKnockback 应该减缓攻击者的水平速度
    TestLivingEntity attacker(1);
    TestLivingEntity target(2);

    attacker.setRotation(0.0f, 0.0f);
    attacker.setVelocity(1.0f, 0.0f, 1.0f);
    float initialVelX = attacker.velocity().x;
    float initialVelZ = attacker.velocity().z;

    Vector3 preHurtVel = target.velocity();
    attacker.causeExtraKnockback(target, 1.0f, preHurtVel);

    // 攻击者水平速度应减为 0.6 倍
    EXPECT_FLOAT_EQ(attacker.velocity().x, initialVelX * 0.6f);
    EXPECT_FLOAT_EQ(attacker.velocity().z, initialVelZ * 0.6f);
    // Y 速度不变
    EXPECT_FLOAT_EQ(attacker.velocity().y, 0.0f);
}

TEST(CauseExtraKnockbackTest, ZeroStrengthNoKnockback)
{
    // 击退强度为 0 时不应施加击退
    TestLivingEntity attacker(1);
    TestLivingEntity target(2);

    attacker.setRotation(0.0f, 0.0f);
    attacker.setVelocity(1.0f, 0.0f, 1.0f);
    float initialTargetVelX = target.velocity().x;
    float initialTargetVelZ = target.velocity().z;
    float initialAttackerVelX = attacker.velocity().x;

    Vector3 preHurtVel = target.velocity();
    attacker.causeExtraKnockback(target, 0.0f, preHurtVel);

    // 目标速度不变
    EXPECT_FLOAT_EQ(target.velocity().x, initialTargetVelX);
    EXPECT_FLOAT_EQ(target.velocity().z, initialTargetVelZ);
    // 攻击者速度也不变（strength <= 0 不进入击退逻辑）
    EXPECT_FLOAT_EQ(attacker.velocity().x, initialAttackerVelX);
}

TEST(CauseExtraKnockbackTest, PlayerStopsSprinting)
{
    // Player 的 causeExtraKnockback 应该在击退后停止疾跑
    TestPlayer attacker(1);
    TestLivingEntity target(2);

    attacker.setRotation(0.0f, 0.0f);
    attacker.setVelocity(1.0f, 0.0f, 1.0f);
    attacker.setSprinting(true);
    EXPECT_TRUE(attacker.isSprinting());

    Vector3 preHurtVel = target.velocity();
    attacker.causeExtraKnockback(target, 1.0f, preHurtVel);

    // 疾跑应该被停止
    EXPECT_FALSE(attacker.isSprinting());
}

TEST(CauseExtraKnockbackTest, PlayerSlowsAttacker)
{
    // Player 的 causeExtraKnockback 应该减缓攻击者的水平速度
    TestPlayer attacker(1);
    TestLivingEntity target(2);

    attacker.setRotation(0.0f, 0.0f);
    attacker.setVelocity(1.0f, 0.5f, 1.0f);

    Vector3 preHurtVel = target.velocity();
    attacker.causeExtraKnockback(target, 1.0f, preHurtVel);

    // 水平速度减为 0.6 倍，Y 速度不变
    EXPECT_FLOAT_EQ(attacker.velocity().x, 1.0f * 0.6f);
    EXPECT_FLOAT_EQ(attacker.velocity().y, 0.5f);
    EXPECT_FLOAT_EQ(attacker.velocity().z, 1.0f * 0.6f);
}

TEST(CauseExtraKnockbackTest, PlayerSprintKnockbackSavesPreHurtVelocity)
{
    // Player::attack() 应该在 hurt() 之前保存目标速度
    // 保存的速度通过 causeExtraKnockback 的 preHurtVelocity 参数传递
    // 此测试验证参数被正确传递（但不能在单元测试中测试 ServerPlayer 网络发送）
    TestPlayer attacker(1);
    TestPlayer target(2);

    attacker.setRotation(90.0f, 0.0f); // 朝向 -X
    attacker.setVelocity(1.0f, 0.0f, 0.0f);
    attacker.setSprinting(true);

    // 模拟 hurt 前的目标速度
    target.setVelocity(0.5f, 0.1f, -0.3f);
    Vector3 preHurtVel = target.velocity();

    // 标记目标为受伤状态（模拟 hurt() 的效果）
    target.markHurt();
    EXPECT_TRUE(target.isHurtMarked());

    // 改变目标速度（模拟 knockback 的效果）
    target.setVelocity(2.0f, 0.5f, -1.0f);

    // 调用 causeExtraKnockback
    attacker.causeExtraKnockback(target, 1.0f, preHurtVel);

    // TestPlayer::sendVelocityPacket 返回 true（模拟 ServerPlayer）
    // 所以应该执行 clearHurtMarked + setVelocity(preHurtVelocity)
    EXPECT_TRUE(target.wasVelocityPacketSent());
    EXPECT_FALSE(target.isHurtMarked()); // hurtMarked 应该被清除
    // 速度应该恢复到 hurt 前的值
    EXPECT_FLOAT_EQ(target.velocity().x, preHurtVel.x);
    EXPECT_FLOAT_EQ(target.velocity().y, preHurtVel.y);
    EXPECT_FLOAT_EQ(target.velocity().z, preHurtVel.z);
}

TEST(CauseExtraKnockbackTest, NonPlayerTargetNoVelocityFix)
{
    // 非Player目标不应执行 clearHurtMarked/setVelocity 修正
    // （因为 LivingEntity 没有 sendVelocityPacket，dynamic_cast<Player*> 返回 nullptr）
    TestPlayer attacker(1);
    TestLivingEntity target(2);

    attacker.setRotation(90.0f, 0.0f);
    attacker.setSprinting(true);

    // 标记目标为受伤状态
    target.markHurt();
    EXPECT_TRUE(target.isHurtMarked());

    // 记录目标受伤标记前的速度
    Vector3 preHurtVel = target.velocity();

    // 调用 causeExtraKnockback（目标是 LivingEntity，不是 Player）
    // 击退会被应用，但 hurtMarked 不应被清除
    attacker.causeExtraKnockback(target, 1.0f, preHurtVel);

    // LivingEntity 不是 Player，所以 dynamic_cast<Player*> 返回 nullptr
    // hurtMarked 不应被清除
    EXPECT_TRUE(target.isHurtMarked());
}

TEST(CauseExtraKnockbackTest, PlayerTargetNotHurtMarkedNoFix)
{
    // 当 strength <= 0 时，causeExtraKnockback 不会调用 applyKnockback，
    // 因此目标的 hurtMarked 不会被 applyKnockback 设置为 true，
    // 速度修正逻辑不应执行
    TestPlayer attacker(1);
    TestPlayer target(2);

    attacker.setRotation(90.0f, 0.0f);
    attacker.setSprinting(true);

    Vector3 preHurtVel = target.velocity();

    // 不标记 hurtMarked
    EXPECT_FALSE(target.isHurtMarked());

    // 使用 strength=0.0f，这样 applyKnockback 不会被调用，hurtMarked 不会被设置
    attacker.causeExtraKnockback(target, 0.0f, preHurtVel);

    // hurtMarked 为 false，不应发送速度包
    EXPECT_FALSE(target.wasVelocityPacketSent());
    // hurtMarked 应仍为 false
    EXPECT_FALSE(target.isHurtMarked());
}

TEST(CauseExtraKnockbackTest, KnockbackDirectionBasedOnAttackerYaw)
{
    // 击退方向应基于攻击者的朝向
    TestLivingEntity attacker(1);
    TestLivingEntity target(2);

    // 设置攻击者朝向 yaw=90（看向 -X 方向）
    attacker.setRotation(90.0f, 0.0f);
    attacker.setVelocity(1.0f, 0.0f, 1.0f);

    Vector3 preHurtVel = target.velocity();

    // 击退强度为 1.0
    attacker.causeExtraKnockback(target, 1.0f, preHurtVel);

    // 击退方向基于 yaw=90：
    // sin(90°) ≈ 1.0, -cos(90°) ≈ 0
    // 目标应被推向 -X 方向（基于攻击者朝向）
    // 由于 applyKnockback 归一化方向向量，实际值取决于归一化结果
    // 当 ratioX ≈ 1.0, ratioZ ≈ 0.0 时，归一化后方向基本不变
    // 击退后速度：velocity.x = velocity.x/2 - knockbackX
    // knockbackX = ratioX * strength = sin(90°) * 1.0 ≈ 1.0
    // knockbackZ = ratioZ * strength = -cos(90°) * 1.0 ≈ 0.0
    // 所以目标 X 方向速度应减少（被推向 -X）
    EXPECT_LT(target.velocity().x, 0.0f); // 被推向 -X 方向
}

TEST(CauseExtraKnockbackTest, NegativeStrengthNoKnockback)
{
    // 负击退强度不应施加击退
    TestPlayer attacker(1);
    TestLivingEntity target(2);

    attacker.setRotation(0.0f, 0.0f);
    attacker.setVelocity(1.0f, 0.0f, 1.0f);
    attacker.setSprinting(true);
    float initialVelX = attacker.velocity().x;

    Vector3 preHurtVel = target.velocity();
    attacker.causeExtraKnockback(target, -1.0f, preHurtVel);

    // 负强度被视为 <= 0，不进入击退逻辑
    EXPECT_FLOAT_EQ(attacker.velocity().x, initialVelX);
    EXPECT_TRUE(attacker.isSprinting()); // 疾跑不应被停止
}

} // namespace
