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

#include "entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/experience/ExperienceConstants.hpp"
#include "entity/experience/ExperienceUtils.hpp"
#include "util/math/random/Random.hpp"
#include <gtest/gtest.h>

using namespace mc;
using mc::i32;
using mc::math::Random;

// 命名空间别名
namespace xp_constants = mc::entity::experience::constants;

// ==================== ExperienceOrbEntity Tests ====================

class ExperienceOrbEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        orb = std::make_unique<ExperienceOrbEntity>(10, mc::test::testEcsRegistry()); // 10 XP orb
    }

    void TearDown() override { orb.reset(); }

    std::unique_ptr<ExperienceOrbEntity> orb;
};

// ========== 构造函数测试 ==========

TEST_F(ExperienceOrbEntityTest, DefaultConstruction)
{
    ExperienceOrbEntity defaultOrb(1, mc::test::testEcsRegistry());
    EXPECT_EQ(defaultOrb.getXpValue(), 1);
    EXPECT_EQ(defaultOrb.getAge(), 0);
    // 原版 MC：构造函数不设置 pickupDelay，默认为 0
    EXPECT_EQ(defaultOrb.getPickupDelay(), 0);
    EXPECT_FALSE(defaultOrb.isRemoved());
}

TEST_F(ExperienceOrbEntityTest, ConstructionWithXpValue)
{
    ExperienceOrbEntity orb50(50, mc::test::testEcsRegistry());
    EXPECT_EQ(orb50.getXpValue(), 50);
}

TEST_F(ExperienceOrbEntityTest, ConstructionWithMaxValue)
{
    // 值应该被限制在 MAX_ORB_SIZE
    ExperienceOrbEntity largeOrb(5000, mc::test::testEcsRegistry());
    EXPECT_EQ(largeOrb.getXpValue(), ExperienceOrbEntity::MAX_ORB_SIZE);
    EXPECT_EQ(largeOrb.getXpValue(), 2477);
}

TEST_F(ExperienceOrbEntityTest, ConstructionWithZeroValue)
{
    // 最小值应该是 1
    ExperienceOrbEntity zeroOrb(0, mc::test::testEcsRegistry());
    EXPECT_EQ(zeroOrb.getXpValue(), 1);
}

TEST_F(ExperienceOrbEntityTest, ConstructionWithNegativeValue)
{
    ExperienceOrbEntity negativeOrb(-10, mc::test::testEcsRegistry());
    EXPECT_EQ(negativeOrb.getXpValue(), 1);
}

// ========== 属性测试 ==========

TEST_F(ExperienceOrbEntityTest, Dimensions)
{
    EXPECT_FLOAT_EQ(orb->width(), 0.5f);
    EXPECT_FLOAT_EQ(orb->height(), 0.5f);
}

TEST_F(ExperienceOrbEntityTest, SetXpValue)
{
    orb->setXpValue(100);
    EXPECT_EQ(orb->getXpValue(), 100);

    orb->setXpValue(5000);
    EXPECT_EQ(orb->getXpValue(), ExperienceOrbEntity::MAX_ORB_SIZE);

    orb->setXpValue(0);
    EXPECT_EQ(orb->getXpValue(), 1);
}

TEST_F(ExperienceOrbEntityTest, Age)
{
    EXPECT_EQ(orb->getAge(), 0);

    orb->setAge(100);
    EXPECT_EQ(orb->getAge(), 100);
}

TEST_F(ExperienceOrbEntityTest, PickupDelay)
{
    // 原版 MC：构造函数不设置 pickupDelay，默认为 0
    EXPECT_EQ(orb->getPickupDelay(), 0);
    EXPECT_TRUE(orb->canBePickedUp()); // 默认可拾取

    orb->setPickupDelay(10); // 设置延迟
    EXPECT_EQ(orb->getPickupDelay(), 10);
    EXPECT_FALSE(orb->canBePickedUp());

    orb->setPickupDelay(0);
    EXPECT_EQ(orb->getPickupDelay(), 0);
    EXPECT_TRUE(orb->canBePickedUp());

    orb->setPickupDelay(100);
    EXPECT_FALSE(orb->canBePickedUp());
}

// ========== 经验球大小测试 ==========

TEST_F(ExperienceOrbEntityTest, GetOrbSize)
{
    // 根据经验值获取球大小等级
    // 1-2: 等级 0
    // 3-6: 等级 1
    // 7-16: 等级 2
    // ...

    ExperienceOrbEntity orb1(1, mc::test::testEcsRegistry());
    EXPECT_EQ(orb1.getOrbSize(), 0);

    ExperienceOrbEntity orb2(2, mc::test::testEcsRegistry());
    EXPECT_EQ(orb2.getOrbSize(), 0);

    ExperienceOrbEntity orb3(3, mc::test::testEcsRegistry());
    EXPECT_EQ(orb3.getOrbSize(), 1);

    ExperienceOrbEntity orb7(7, mc::test::testEcsRegistry());
    EXPECT_EQ(orb7.getOrbSize(), 2);

    ExperienceOrbEntity orb17(17, mc::test::testEcsRegistry());
    EXPECT_EQ(orb17.getOrbSize(), 3);

    ExperienceOrbEntity orbMax(2477, mc::test::testEcsRegistry());
    EXPECT_EQ(orbMax.getOrbSize(), 10); // 最大球大小
}

// ========== 静态方法测试 ==========

TEST_F(ExperienceOrbEntityTest, GetXPSplit)
{
    // 静态方法测试经验分割
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(1), 1);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(2), 1);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(3), 3);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(7), 7);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(10), 7);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(100), 73);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(1000), 617);
    EXPECT_EQ(ExperienceOrbEntity::getXPSplit(3000), 2477); // 最大分割值
}

// ========== 合并测试 ==========

TEST_F(ExperienceOrbEntityTest, CanMergeWithSelf)
{
    // 不能和自己合并
    EXPECT_FALSE(orb->canMergeWith(*orb));
}

TEST_F(ExperienceOrbEntityTest, CanMergeWithDifferentValues)
{
    ExperienceOrbEntity orb1(10, mc::test::testEcsRegistry());
    ExperienceOrbEntity orb2(20, mc::test::testEcsRegistry());

    // 在同一位置应该可以合并
    orb1.setPosition(0, 0, 0);
    orb2.setPosition(0, 0, 0);

    // 由于距离检查，需要足够近
    // MERGE_DISTANCE_SQ = 0.5 * 0.5 = 0.25
    // 但 canMergeWith 需要距离检查，而且还要检查合并后是否超过最大值

    // 总值 10 + 20 = 30，小于 MAX_ORB_SIZE (2477)
    EXPECT_TRUE(orb1.canMergeWith(orb2));
}

TEST_F(ExperienceOrbEntityTest, CanMergeWithExceedsMax)
{
    ExperienceOrbEntity orb1(2000, mc::test::testEcsRegistry());
    ExperienceOrbEntity orb2(1000, mc::test::testEcsRegistry());

    orb1.setPosition(0, 0, 0);
    orb2.setPosition(0, 0, 0);

    // 2000 + 1000 = 3000 > 2477，不应该合并
    EXPECT_FALSE(orb1.canMergeWith(orb2));
}

TEST_F(ExperienceOrbEntityTest, CanMergeWithDistance)
{
    ExperienceOrbEntity orb1(10, mc::test::testEcsRegistry());
    ExperienceOrbEntity orb2(20, mc::test::testEcsRegistry());

    orb1.setPosition(0, 0, 0);
    orb2.setPosition(100, 0, 0); // 距离太远

    EXPECT_FALSE(orb1.canMergeWith(orb2));
}

TEST_F(ExperienceOrbEntityTest, TryMerge)
{
    ExperienceOrbEntity orb1(10, mc::test::testEcsRegistry());
    ExperienceOrbEntity orb2(20, mc::test::testEcsRegistry());

    orb1.setPosition(0, 0, 0);
    orb2.setPosition(0, 0, 0);

    bool merged = orb1.tryMergeWith(orb2);

    EXPECT_TRUE(merged);
    EXPECT_EQ(orb1.getXpValue(), 30); // 合并后的值
    EXPECT_TRUE(orb2.isRemoved());    // 被合并的球应该被移除
}

// ========== 常量验证测试 ==========

TEST_F(ExperienceOrbEntityTest, ConstantsValidation)
{
    // 验证经验球常量与 ExperienceConstants 一致
    EXPECT_EQ(ExperienceOrbEntity::MAX_ORB_SIZE, xp_constants::MAX_ORB_VALUE);
    EXPECT_EQ(ExperienceOrbEntity::MAX_AGE, xp_constants::MAX_ORB_AGE);
    EXPECT_EQ(ExperienceOrbEntity::DEFAULT_PICKUP_DELAY, xp_constants::DEFAULT_PICKUP_DELAY);
    EXPECT_EQ(ExperienceOrbEntity::TRACKING_RANGE, xp_constants::ORB_TRACKING_RANGE);
}

// ========== 追踪玩家测试 ==========

TEST_F(ExperienceOrbEntityTest, TrackingPlayer)
{
    // 初始状态不追踪任何玩家
    EXPECT_FALSE(orb->isBeingTracked());
    EXPECT_EQ(orb->getTrackingPlayer(), nullptr);
}

// ========== 实体类型测试 ==========

TEST_F(ExperienceOrbEntityTest, EntityType)
{
    EXPECT_NE(dynamic_cast<ExperienceOrbEntity*>(orb.get()), nullptr);
}

// ==================== ExperienceOrbEntity Integration Tests ====================

class ExperienceOrbEntityIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ExperienceOrbEntityIntegrationTest, XPSplitConsistency)
{
    // 验证分割值和球大小一致性
    for (int xp = 1; xp <= 100; ++xp) {
        i32 split = ExperienceOrbEntity::getXPSplit(xp);

        // 分割值必须是有效的大小
        bool validSplit = false;
        for (int i = 0; i < xp_constants::XP_SPLIT_COUNT; ++i) {
            if (xp_constants::XP_SPLIT_VALUES[i] == split) {
                validSplit = true;
                break;
            }
        }
        EXPECT_TRUE(validSplit) << "Invalid split for XP " << xp << ": " << split;

        // 分割值不能超过原始值
        EXPECT_LE(split, xp);

        // 创建对应大小的球
        ExperienceOrbEntity orb(split, mc::test::testEcsRegistry());
        EXPECT_EQ(orb.getXpValue(), split);
    }
}

TEST_F(ExperienceOrbEntityIntegrationTest, OrbSizeConsistency)
{
    // 验证每个分割值对应的球大小
    for (int i = 0; i < xp_constants::XP_SPLIT_COUNT; ++i) {
        i32 value = xp_constants::XP_SPLIT_VALUES[i];
        ExperienceOrbEntity orb(value, mc::test::testEcsRegistry());
        i32 size = orb.getOrbSize();

        // 更大的分割值应该有更大的或相等的球大小
        if (i < xp_constants::XP_SPLIT_COUNT - 1) {
            i32 nextValue = xp_constants::XP_SPLIT_VALUES[i + 1];
            ExperienceOrbEntity nextOrb(nextValue, mc::test::testEcsRegistry());
            i32 nextSize = nextOrb.getOrbSize();

            EXPECT_GE(size, nextSize) << "Inconsistent orb sizes: " << value << " has size " << size << ", "
                                      << nextValue << " has size " << nextSize;
        }
    }
}

TEST_F(ExperienceOrbEntityIntegrationTest, MergeSimulation)
{
    // 模拟多个经验球合并
    std::vector<std::unique_ptr<ExperienceOrbEntity>> orbs;

    // 创建多个小经验球
    for (int i = 0; i < 10; ++i) {
        orbs.push_back(std::make_unique<ExperienceOrbEntity>(10, mc::test::testEcsRegistry())); // 每个10点经验
        orbs.back()->setPosition(0, 0, 0);
    }

    // 尝试合并
    i32 totalMerged = orbs[0]->getXpValue();
    for (size_t i = 1; i < orbs.size(); ++i) {
        if (orbs[0]->canMergeWith(*orbs[i])) {
            orbs[0]->tryMergeWith(*orbs[i]);
            totalMerged += 10;
        }
    }

    // 验证合并后的值
    EXPECT_EQ(orbs[0]->getXpValue(), totalMerged);
    EXPECT_EQ(orbs[0]->getXpValue(), 100); // 10个球，每个10点
}

TEST_F(ExperienceOrbEntityIntegrationTest, EnderDragonXP)
{
    // 末影龙掉落12000经验
    // 验证分割后球的值总和正确
    i32 totalXP = xp_constants::ENDER_DRAGON_XP;
    std::vector<i32> orbs;

    i32 remaining = totalXP;
    while (remaining > 0) {
        i32 split = ExperienceOrbEntity::getXPSplit(remaining);
        orbs.push_back(split);
        remaining -= split;
    }

    // 验证总和
    i32 sum = 0;
    for (i32 v : orbs) {
        sum += v;
    }
    EXPECT_EQ(sum, totalXP);

    // 验证每个球的值有效
    for (i32 v : orbs) {
        EXPECT_LE(v, ExperienceOrbEntity::MAX_ORB_SIZE);
        EXPECT_GE(v, 1);
    }

    // 球的数量应该合理
    EXPECT_LT(orbs.size(), 20u); // 12000经验应该分成约10个球
    EXPECT_GT(orbs.size(), 5u);
}

// ============================================================================
// ExperienceOrbEntity::hurt 测试
// ============================================================================

/**
 * @brief ExperienceOrbEntity hurt 测试用的 Mock World
 *
 * 支持 GameEvent 捕获，用于验证 ENTITY_DAMAGE 事件派发。
 */
class ExperienceOrbHurtTestWorld : public mc::test::BaseTestWorld {
public:
    ExperienceOrbHurtTestWorld() = default;

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_lastGameEventId = event.id();
        m_lastGameEventPos = pos;
        m_lastGameEventSourceEntity = context.sourceEntity();
        m_gameEventCount++;
    }

    [[nodiscard]] i32 gameEventCount() const { return m_gameEventCount; }
    [[nodiscard]] const std::string& lastGameEventId() const { return m_lastGameEventId; }
    [[nodiscard]] const BlockPos& lastGameEventPos() const { return m_lastGameEventPos; }
    [[nodiscard]] const Entity* lastGameEventSourceEntity() const { return m_lastGameEventSourceEntity; }

    void clearGameState()
    {
        m_gameEventCount = 0;
        m_lastGameEventId.clear();
        m_lastGameEventPos = BlockPos(0, 0, 0);
        m_lastGameEventSourceEntity = nullptr;
    }

private:
    i32 m_gameEventCount = 0;
    std::string m_lastGameEventId;
    BlockPos m_lastGameEventPos{0, 0, 0};
    const Entity* m_lastGameEventSourceEntity = nullptr;
};

class ExperienceOrbHurtTest : public ::testing::Test {
protected:
    void SetUp() override { m_world.clearGameState(); }

    ExperienceOrbHurtTestWorld m_world;
};

TEST_F(ExperienceOrbHurtTest, InvulnerableSource_ReturnsFalse_DoesNotMarkHurt)
{
    // 无敌状态下，hurt() 应返回 false 且不调用 markHurt()
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    orb.setInvulnerable(true);
    EXPECT_FALSE(orb.isHurtMarked());

    auto source = DamageSources::generic();
    EXPECT_FALSE(orb.hurt(source, 3.0f));
    EXPECT_FALSE(orb.isHurtMarked());
    EXPECT_FALSE(orb.isRemoved());
}

TEST_F(ExperienceOrbHurtTest, InvulnerableSource_VoidDamageBypasses_ReturnsTrue)
{
    // 虚空伤害绕过无敌，hurt() 应返回 true
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    orb.setInvulnerable(true);

    auto voidSource = DamageSources::outOfWorld();
    EXPECT_TRUE(orb.hurt(voidSource, 100.0f));
    EXPECT_TRUE(orb.isRemoved());
}

TEST_F(ExperienceOrbHurtTest, NormalDamage_ReducesHealth_MarksHurt_ReturnsTrue)
{
    // 正常伤害减少生命值，标记 hurtMarked，返回 true
    // ExperienceOrbEntity 默认 m_health = 5
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    orb.setWorld(&m_world);

    auto source = DamageSources::generic();
    bool result = orb.hurt(source, 3.0f);
    EXPECT_TRUE(result);
    EXPECT_TRUE(orb.isHurtMarked());
    EXPECT_FALSE(orb.isRemoved()); // 5 - 3 = 2 > 0，不会销毁
}

TEST_F(ExperienceOrbHurtTest, SmallDamage_DoesNotDiscard)
{
    // 3 点伤害：5 - 3 = 2，生命值 > 0，不会调用 discard()
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    orb.setWorld(&m_world);

    auto source = DamageSources::generic();
    orb.hurt(source, 3.0f);
    EXPECT_FALSE(orb.isRemoved());
}

TEST_F(ExperienceOrbHurtTest, ExactHealthDamage_DiscardsEntity)
{
    // 5 点伤害：5 - 5 = 0，生命值 <= 0，调用 discard()
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    orb.setWorld(&m_world);

    auto source = DamageSources::generic();
    orb.hurt(source, 5.0f);
    EXPECT_TRUE(orb.isRemoved());
}

TEST_F(ExperienceOrbHurtTest, OverkillDamage_DiscardsEntity)
{
    // 10 点伤害：5 - 10 = -5，生命值 < 0，调用 discard()
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    orb.setWorld(&m_world);

    auto source = DamageSources::generic();
    orb.hurt(source, 10.0f);
    EXPECT_TRUE(orb.isRemoved());
}

TEST_F(ExperienceOrbHurtTest, MultipleHitsAccumulateDamage)
{
    // 多次攻击累积伤害直到销毁
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    orb.setWorld(&m_world);

    auto source = DamageSources::generic();

    // 第一击：5 - 2 = 3
    EXPECT_TRUE(orb.hurt(source, 2.0f));
    EXPECT_FALSE(orb.isRemoved());

    // 第二击：3 - 2 = 1
    EXPECT_TRUE(orb.hurt(source, 2.0f));
    EXPECT_FALSE(orb.isRemoved());

    // 第三击：1 - 2 = -1，销毁
    EXPECT_TRUE(orb.hurt(source, 2.0f));
    EXPECT_TRUE(orb.isRemoved());
}

TEST_F(ExperienceOrbHurtTest, EntityDamageGameEvent_IsEmitted)
{
    // hurt() 成功时应派发 ENTITY_DAMAGE 游戏事件
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    orb.setPosition(5.0f, 64.0f, 10.0f);
    orb.setWorld(&m_world);

    auto source = DamageSources::generic();
    EXPECT_TRUE(orb.hurt(source, 1.0f));

    EXPECT_EQ(m_world.gameEventCount(), 1);
    EXPECT_EQ(m_world.lastGameEventId(), "entity_damage");
}

TEST_F(ExperienceOrbHurtTest, EntityDamageGameEvent_PositionMatchesEntityBlockPos)
{
    // 游戏事件位置应与实体的方块坐标一致
    // 位置 (5.5, 64.3, 10.7) -> BlockPos (5, 64, 10)
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    orb.setPosition(5.5f, 64.3f, 10.7f);
    orb.setWorld(&m_world);

    auto source = DamageSources::generic();
    orb.hurt(source, 1.0f);

    EXPECT_EQ(m_world.lastGameEventPos(), BlockPos(5, 64, 10));
}

TEST_F(ExperienceOrbHurtTest, EntityDamageGameEvent_NullSourceForEnvironmentalDamage)
{
    // 环境伤害的 sourceEntity 应为 nullptr
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    orb.setPosition(5.0f, 64.0f, 10.0f);
    orb.setWorld(&m_world);

    auto source = DamageSources::generic();
    orb.hurt(source, 1.0f);

    EXPECT_EQ(m_world.lastGameEventSourceEntity(), nullptr);
}

TEST_F(ExperienceOrbHurtTest, InvulnerableSource_DoesNotDispatchGameEvent)
{
    // 无敌状态下不派发游戏事件
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    orb.setPosition(5.0f, 64.0f, 10.0f);
    orb.setWorld(&m_world);
    orb.setInvulnerable(true);

    auto source = DamageSources::generic();
    EXPECT_FALSE(orb.hurt(source, 1.0f));
    EXPECT_EQ(m_world.gameEventCount(), 0);
}

TEST_F(ExperienceOrbHurtTest, NoWorld_DoesNotDispatchGameEvent)
{
    // 没有 world 时不会派发游戏事件，也不会崩溃
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());
    // 不设置 world

    auto source = DamageSources::generic();
    EXPECT_TRUE(orb.hurt(source, 1.0f));
    EXPECT_FALSE(orb.isRemoved()); // 5 - 1 = 4 > 0
}

TEST_F(ExperienceOrbHurtTest, ClearHurtMarked_ResetsFlag)
{
    // clearHurtMarked() 应重置 hurtMarked 标志
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());

    auto source = DamageSources::generic();
    orb.hurt(source, 1.0f);
    EXPECT_TRUE(orb.isHurtMarked());

    orb.clearHurtMarked();
    EXPECT_FALSE(orb.isHurtMarked());
}

TEST_F(ExperienceOrbHurtTest, MarkHurtCalledBeforeDiscard)
{
    // 即使伤害导致 discard()，markHurt 也应被调用
    ExperienceOrbEntity orb(10, mc::test::testEcsRegistry());

    auto source = DamageSources::generic();
    orb.hurt(source, 5.0f);
    EXPECT_TRUE(orb.isHurtMarked());
    EXPECT_TRUE(orb.isRemoved());
}
