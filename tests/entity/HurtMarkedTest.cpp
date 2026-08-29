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

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;

namespace {

// 简单的测试用 LivingEntity 子类，用于测试 hurtMarked 机制
class TestHurtEntity final : public LivingEntity {
public:
    explicit TestHurtEntity(EntityInstanceId id)
        : LivingEntity(id, nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

} // anonymous namespace

// ============================================================================
// Entity::hurtMarked 基础 API 测试
// ============================================================================

TEST(HurtMarkedTest, DefaultIsFalse)
{
    TestHurtEntity entity(EntityInstanceId(1));
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, MarkHurtSetsFlag)
{
    TestHurtEntity entity(EntityInstanceId(1));
    entity.markHurt();
    EXPECT_TRUE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, ClearHurtMarkedResetsFlag)
{
    TestHurtEntity entity(EntityInstanceId(1));
    entity.markHurt();
    EXPECT_TRUE(entity.isHurtMarked());
    entity.clearHurtMarked();
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, MultipleMarkHurtCallsAreIdempotent)
{
    // 多次 markHurt() 调用不应导致任何问题，标记仍然是 true
    TestHurtEntity entity(EntityInstanceId(1));
    entity.markHurt();
    entity.markHurt();
    entity.markHurt();
    EXPECT_TRUE(entity.isHurtMarked());
    // 一次 clearHurtMarked() 即可重置
    entity.clearHurtMarked();
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, MarkHurtClearCycle)
{
    // 模拟 EntityTracker 的 tick 循环：mark -> check -> clear
    TestHurtEntity entity(EntityInstanceId(1));

    // 初始状态
    EXPECT_FALSE(entity.isHurtMarked());

    // 受伤后标记
    entity.markHurt();
    EXPECT_TRUE(entity.isHurtMarked());

    // EntityTracker 检查并同步后清除
    entity.clearHurtMarked();
    EXPECT_FALSE(entity.isHurtMarked());

    // 下一 tick 没有受伤，标记仍为 false
    EXPECT_FALSE(entity.isHurtMarked());
}

// ============================================================================
// LivingEntity::hurt() 自动标记 hurtMarked 测试
// ============================================================================

class HurtMarkedWorld final : public mc::test::BaseTestWorld {
public:
    HurtMarkedWorld()
    {
        // 提供地面支持
        setSupportEnabled(true);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (m_supportEnabled && x == 0 && y == 0 && z == 0) {
            return &VanillaBlocks::STONE->defaultState();
        }
        return nullptr;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        if (!m_supportEnabled) {
            return false;
        }
        return box.maxX > 0.0f && box.minX < 1.0f && box.maxY > 0.0f && box.minY < 1.0f && box.maxZ > 0.0f &&
            box.minZ < 1.0f;
    }

    void setSupportEnabled(bool enabled) { m_supportEnabled = enabled; }

private:
    bool m_supportEnabled = true;
};

TEST(HurtMarkedTest, HurtSetsMarkHurtFlag)
{
    // LivingEntity::hurt() 成功造成伤害时应自动设置 hurtMarked
    HurtMarkedWorld world;
    TestHurtEntity entity(EntityInstanceId(1));
    entity.setWorld(&world);
    entity.setPosition(Vector3(0.5f, 1.0f, 0.5f));
    entity.setOnGround(true);

    EXPECT_FALSE(entity.isHurtMarked());

    EntityDamageSource damageSource(DamageType::Generic, nullptr);
    bool hurt = entity.hurt(damageSource, 5.0f);

    // 伤害应成功
    EXPECT_TRUE(hurt);
    // hurtMarked 应被自动设置
    EXPECT_TRUE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, HurtInvulnerableDoesNotSetMarkHurtFlag)
{
    // 无敌状态下 hurt() 返回 false，不应设置 hurtMarked
    HurtMarkedWorld world;
    TestHurtEntity entity(EntityInstanceId(1));
    entity.setWorld(&world);
    entity.setPosition(Vector3(0.5f, 1.0f, 0.5f));
    entity.setOnGround(true);
    entity.setInvulnerable(true);

    EXPECT_FALSE(entity.isHurtMarked());

    EntityDamageSource damageSource(DamageType::Generic, nullptr);
    bool hurt = entity.hurt(damageSource, 5.0f);

    // 无敌时伤害应失败
    EXPECT_FALSE(hurt);
    // hurtMarked 不应被设置
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, ApplyKnockbackSetsMarkHurtFlag)
{
    // applyKnockback() 应自动设置 hurtMarked
    HurtMarkedWorld world;
    TestHurtEntity entity(EntityInstanceId(1));
    entity.setWorld(&world);
    entity.setPosition(Vector3(0.0f, 1.0f, 0.0f));
    entity.setOnGround(true);

    EXPECT_FALSE(entity.isHurtMarked());

    // 施加击退
    entity.applyKnockback(1.0f, 1.0, 0.0);

    // hurtMarked 应被自动设置
    EXPECT_TRUE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, ApplyKnockbackWithZeroDirectionStillSetsFlag)
{
    // commit 3265b790d 起对齐 MC Java：零向量方向不提前返回，而是随机扰动
    // 方向（ratioX/Z = (random-random)*0.01）后继续应用击退，故仍设置 hurtMarked。
    HurtMarkedWorld world;
    TestHurtEntity entity(EntityInstanceId(1));
    entity.setWorld(&world);
    entity.setPosition(Vector3(0.0f, 1.0f, 0.0f));
    entity.setOnGround(true);

    EXPECT_FALSE(entity.isHurtMarked());

    // 零向量方向经随机扰动后仍应用击退
    entity.applyKnockback(1.0f, 0.0, 0.0);

    // 击退被应用，标记设置
    EXPECT_TRUE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, HurtThenKnockbackBothSetFlag)
{
    // hurt() 和 applyKnockback() 都设置标记，标记应保持为 true
    HurtMarkedWorld world;
    TestHurtEntity entity(EntityInstanceId(1));
    entity.setWorld(&world);
    entity.setPosition(Vector3(0.0f, 1.0f, 0.0f));
    entity.setOnGround(true);

    EntityDamageSource damageSource(DamageType::Generic, nullptr);
    entity.hurt(damageSource, 5.0f);
    EXPECT_TRUE(entity.isHurtMarked());

    // 击退也设置标记
    entity.applyKnockback(1.0f, 1.0, 0.0);
    EXPECT_TRUE(entity.isHurtMarked());

    // 一次清除即可
    entity.clearHurtMarked();
    EXPECT_FALSE(entity.isHurtMarked());
}

// ============================================================================
// 边界场景测试
// ============================================================================

TEST(HurtMarkedTest, NoTrackingPlayersClearIsSafe)
{
    // 没有追踪玩家时 clearHurtMarked() 应安全无副作用
    TestHurtEntity entity(EntityInstanceId(1));
    entity.markHurt();
    EXPECT_TRUE(entity.isHurtMarked());
    // 即使没有 EntityTracker 在追踪，清除标记也是安全的
    entity.clearHurtMarked();
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, AddVelocityDoesNotSetMarkHurtFlag)
{
    // Entity::addVelocity() 不应自动设置 hurtMarked
    // 与 MC Java 行为一致：addDeltaMovement 不设置 hurtMarked
    TestHurtEntity entity(EntityInstanceId(1));
    entity.addVelocity(1.0f, 0.5f, 1.0f);
    // addVelocity 不设置 hurtMarked
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, SetVelocityDoesNotSetMarkHurtFlag)
{
    // Entity::setVelocity() 不应自动设置 hurtMarked
    TestHurtEntity entity(EntityInstanceId(1));
    entity.setVelocity(1.0f, 0.5f, 1.0f);
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, HurtResistantTimePreventsHurtAndMarkHurt)
{
    // 无敌帧差额逻辑（对齐 vanilla LivingEntity.hurtServer:1191-1206，任务 #379 重构后）：
    //   invulnerableTime>10 && !BYPASSES_COOLDOWN 时：
    //     - amount <= lastHurt → return false（同额/更小伤害被吞，不设 hurtMarked）
    //     - amount >  lastHurt → 承受差额（amount-lastHurt），return true，设 hurtMarked
    // 此前 Cubium 在 isInvulnerableTo 内做无敌帧门控（>0 即免疫），致 hurt 主流程的差额逻辑成为
    // 死代码；#379 移除该门控后无敌帧完全交给 hurt 差额逻辑。本测试锚定新行为。
    HurtMarkedWorld world;
    TestHurtEntity entity(EntityInstanceId(1));
    entity.setWorld(&world);
    entity.setPosition(Vector3(0.5f, 1.0f, 0.5f));
    entity.setOnGround(true);

    // 第一次受伤（5 伤害）→ 进入无敌帧，hurt 返 true，设 hurtMarked
    EntityDamageSource damageSource(DamageType::Generic, nullptr);
    bool firstHurt = entity.hurt(damageSource, 5.0f);
    EXPECT_TRUE(firstHurt);
    EXPECT_TRUE(entity.isHurtMarked());
    entity.clearHurtMarked();

    // 无敌帧内再次受更小伤害（3 < lastHurt 5）→ 被吞，hurt 返 false，不设 hurtMarked
    bool smallerHurt = entity.hurt(damageSource, 3.0f);
    EXPECT_FALSE(smallerHurt) << "无敌帧内更小伤害应被吞（amount<=lastHurt）";
    EXPECT_FALSE(entity.isHurtMarked()) << "被吞的伤害不应设置 hurtMarked";
}

TEST(HurtMarkedTest, HurtResistantTimeLargerDamageDealsDifference)
{
    // 无敌帧差额逻辑：无敌帧内更大伤害承受差额（对齐 vanilla hurtServer:1191-1206）。
    // 第一次 5 伤害 → lastHurt=5、invulnerableTime=20。第二次 10 伤害（>5）→ 承受差额 5、
    // lastHurt 更新为 10、hurt 返 true、设 hurtMarked。
    HurtMarkedWorld world;
    TestHurtEntity entity(EntityInstanceId(1));
    entity.setWorld(&world);
    entity.setPosition(Vector3(0.5f, 1.0f, 0.5f));
    entity.setOnGround(true);

    EntityDamageSource damageSource(DamageType::Generic, nullptr);
    entity.hurt(damageSource, 5.0f);
    entity.clearHurtMarked();

    const f32 hpAfterFirst = entity.health();
    // 无敌帧内更大伤害（10 > lastHurt 5）→ 承受差额 5
    bool largerHurt = entity.hurt(damageSource, 10.0f);
    EXPECT_TRUE(largerHurt) << "无敌帧内更大伤害应承受差额（amount>lastHurt）";
    EXPECT_TRUE(entity.isHurtMarked()) << "承受差额的伤害应设置 hurtMarked";
    // 承受差额 5 伤害（10-5），血量应再降 5
    EXPECT_FLOAT_EQ(entity.health(), hpAfterFirst - 5.0f) << "应仅承受差额 5 伤害";
}
