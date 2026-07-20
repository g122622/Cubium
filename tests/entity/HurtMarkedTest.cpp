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
        : LivingEntity(id)
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
    TestHurtEntity entity(1);
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, MarkHurtSetsFlag)
{
    TestHurtEntity entity(1);
    entity.markHurt();
    EXPECT_TRUE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, ClearHurtMarkedResetsFlag)
{
    TestHurtEntity entity(1);
    entity.markHurt();
    EXPECT_TRUE(entity.isHurtMarked());
    entity.clearHurtMarked();
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, MultipleMarkHurtCallsAreIdempotent)
{
    // 多次 markHurt() 调用不应导致任何问题，标记仍然是 true
    TestHurtEntity entity(1);
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
    TestHurtEntity entity(1);

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

class HurtMarkedWorld final : public test::BaseTestWorld {
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
    TestHurtEntity entity(1);
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
    TestHurtEntity entity(1);
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
    TestHurtEntity entity(1);
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
    TestHurtEntity entity(1);
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
    TestHurtEntity entity(1);
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
    TestHurtEntity entity(1);
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
    TestHurtEntity entity(1);
    entity.addVelocity(1.0f, 0.5f, 1.0f);
    // addVelocity 不设置 hurtMarked
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, SetVelocityDoesNotSetMarkHurtFlag)
{
    // Entity::setVelocity() 不应自动设置 hurtMarked
    TestHurtEntity entity(1);
    entity.setVelocity(1.0f, 0.5f, 1.0f);
    EXPECT_FALSE(entity.isHurtMarked());
}

TEST(HurtMarkedTest, HurtResistantTimePreventsHurtAndMarkHurt)
{
    // 在无敌帧期间，hurt() 返回 false（isInvulnerableTo 检查优先于累积伤害逻辑），
    // 因此 hurtMarked 不会被设置
    HurtMarkedWorld world;
    TestHurtEntity entity(1);
    entity.setWorld(&world);
    entity.setPosition(Vector3(0.5f, 1.0f, 0.5f));
    entity.setOnGround(true);

    // 第一次受伤
    EntityDamageSource damageSource(DamageType::Generic, nullptr);
    entity.hurt(damageSource, 5.0f);
    EXPECT_TRUE(entity.isHurtMarked());
    entity.clearHurtMarked();

    // 在无敌帧期间再次受伤 — isInvulnerableTo 拦截，hurt() 返回 false
    bool hurt = entity.hurt(damageSource, 10.0f);
    EXPECT_FALSE(hurt);
    // hurtMarked 不应被设置
    EXPECT_FALSE(entity.isHurtMarked());
}
