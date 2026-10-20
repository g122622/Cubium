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

// 盾牌方向校验测试：来袭方向与头部水平朝向夹角超过 90° 时不格挡（vanilla
// BlocksAttacks.DamageReduction 默认 horizontal_blocking_angle=90）。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;

namespace {

/// 测试玩家：注册属性并补满生命值，作为盾牌持有者或攻击者
class ShieldTestPlayer final : public Player {
public:
    ShieldTestPlayer()
        : Player(EntityInstanceId(1), "ShieldTestPlayer", mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

} // namespace

class ShieldBlockingAngleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        DamageTypeTags::initialize();
    }

    /// 举盾：主手放置盾牌并进入使用状态
    static void _equipRaisedShield(Player& player)
    {
        player.getHeldItem(Hand::MainHand) = ItemStack(*Items::SHIELD, 1);
        player.setActiveHand(Hand::MainHand);
    }

    // 与 ArmorStandDamageTest 一致：BaseTestWorld 默认构造为 protected，派生后作为夹具成员。
    class TestWorld final : public mc::test::BaseTestWorld {
    public:
        TestWorld() = default;
    };

    TestWorld m_world;
};

TEST_F(ShieldBlockingAngleTest, EnemyInFrontIsBlocked)
{
    ShieldTestPlayer player;
    player.setWorld(&m_world);
    player.setPosition(0.0f, 64.0f, 0.0f);
    // yaw=0 朝向 +X
    player.setRotation(0.0f, 0.0f);
    player.setRotationYawHead(0.0f);
    _equipRaisedShield(player);

    ShieldTestPlayer attacker;
    attacker.setWorld(&m_world);
    attacker.setPosition(3.0f, 64.0f, 0.0f);

    auto source = DamageSources::playerAttack(&attacker);
    ASSERT_TRUE(player.canBlockDamageSource(source));
    const auto angle = player.blockingSourceAngle(source);
    ASSERT_TRUE(angle.has_value());
    EXPECT_NEAR(*angle, 0.0f, 1.0e-4f);
    player.actuallyHurt(source, 4.0f);
    EXPECT_FLOAT_EQ(player.health(), player.maxHealth());
}

TEST_F(ShieldBlockingAngleTest, EnemyBehindBreaksThrough)
{
    ShieldTestPlayer player;
    player.setWorld(&m_world);
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setRotation(0.0f, 0.0f); // 朝 +X
    player.setRotationYawHead(0.0f);
    _equipRaisedShield(player);
    const f32 healthBefore = player.health();

    ShieldTestPlayer attacker;
    attacker.setWorld(&m_world);
    attacker.setPosition(-3.0f, 64.0f, 0.0f); // 正后方

    auto source = DamageSources::playerAttack(&attacker);
    ASSERT_TRUE(player.canBlockDamageSource(source));
    const auto angle = player.blockingSourceAngle(source);
    ASSERT_TRUE(angle.has_value());
    EXPECT_NEAR(*angle, static_cast<f32>(math::PI), 1.0e-4f);
    // 角度超出 90° 阻挡角：伤害照常结算，盾牌不扣耐久
    player.actuallyHurt(source, 4.0f);
    EXPECT_LT(player.health(), healthBefore);
}

TEST_F(ShieldBlockingAngleTest, EnemySidewaysIsBlocked)
{
    ShieldTestPlayer player;
    player.setWorld(&m_world);
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setRotation(0.0f, 0.0f); // 朝 +X
    player.setRotationYawHead(0.0f);
    _equipRaisedShield(player);

    ShieldTestPlayer attacker;
    attacker.setWorld(&m_world);
    attacker.setPosition(0.0f, 64.0f, 3.0f); // 正侧方，夹角 90°

    auto source = DamageSources::playerAttack(&attacker);
    const auto angle = player.blockingSourceAngle(source);
    ASSERT_TRUE(angle.has_value());
    EXPECT_NEAR(*angle, static_cast<f32>(math::PI / 2.0), 1.0e-4f);
    player.actuallyHurt(source, 4.0f);
    EXPECT_FLOAT_EQ(player.health(), player.maxHealth());
}

TEST_F(ShieldBlockingAngleTest, NoSourcePositionFallsBackToPiAngle)
{
    ShieldTestPlayer player;
    player.setWorld(&m_world);
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setRotationYawHead(0.0f);
    _equipRaisedShield(player);

    // 无来源实体的伤害：sourcePosition 缺失 → 夹角按 vanilla 取 PI，任何阻挡角下都不减伤。
    // 选用 mobAttack（在 CAN_BREAK 标签范围内，不被 BYPASSES_SHIELD 直接排除）。
    auto source = DamageSources::mobAttack(nullptr);
    EXPECT_TRUE(player.canBlockDamageSource(source));
    EXPECT_FALSE(player.blockingSourceAngle(source).has_value());

    const f32 healthBefore = player.health();
    player.actuallyHurt(source, 4.0f);
    EXPECT_LT(player.health(), healthBefore);
}
