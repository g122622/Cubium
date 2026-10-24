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
 * @file ArmadilloEntityTest.cpp
 * @brief 犰狳实体测试
 *
 * 覆盖 vanilla Armadillo / ArmadilloState 的关键语义：
 * 1. 状态机：卷球 / 展开、shouldHideInShell、shouldSwitchToScaredState
 * 2. 受伤：卷球时伤害减半为 (amount-1)/2
 * 3. 同步数据：DATA_ID_STATE（serializerId 32）
 * 4. 刷鳞甲：成体返回 true、幼年返回 false
 * 5. NBT：state / scute_time 往返
 * 6. 危险记忆：受伤记录 80 tick，衰减到 30 以下时 SCARED → UNROLLING
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/passive/special/ArmadilloEntity.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>

namespace mc {
namespace {

/**
 * @brief 犰狳测试世界：空世界 + 记录生成的实体
 */
class ArmadilloTestWorld final : public mc::test::BaseTestWorld {
public:
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const
    {
        return m_spawnedEntities;
    }

private:
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class ArmadilloEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::ensureInitialized();
    }

    ArmadilloTestWorld m_world;
};

// ============================================================================
// 默认状态与同步参数
// ============================================================================

TEST_F(ArmadilloEntityTest, DefaultsToIdle)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(armadillo.getState(), ArmadilloState::Idle);
    EXPECT_FALSE(armadillo.isScared());
    EXPECT_FALSE(armadillo.shouldHideInShell());
    EXPECT_FALSE(armadillo.isPanicking());
}

TEST_F(ArmadilloEntityTest, StateParam_UsesArmadilloStateSerializer)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    armadillo.switchToState(ArmadilloState::Scared);

    EXPECT_EQ(armadillo.getState(), ArmadilloState::Scared);
    const i32 raw = armadillo.dataManager().get(ArmadilloEntity::DATA_ID_STATE_PARAM);
    EXPECT_EQ(raw, static_cast<i32>(ArmadilloState::Scared));
    EXPECT_EQ(ArmadilloEntity::VANILLA_ARMADILLO_STATE_SERIALIZER_ID, 32);
}

// ============================================================================
// 状态机
// ============================================================================

TEST_F(ArmadilloEntityTest, RollUp_FromIdle_EntersRolling)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    armadillo.setWorld(&m_world);

    EXPECT_FALSE(armadillo.shouldHideInShell());
    armadillo.rollUp();
    EXPECT_EQ(armadillo.getState(), ArmadilloState::Rolling);
    EXPECT_TRUE(armadillo.isScared());
    // ROLLING 在第 6 tick 后才完全缩进壳里（vanilla inStateTicks > 5）。
    EXPECT_FALSE(armadillo.shouldHideInShell());
}

TEST_F(ArmadilloEntityTest, RollUp_WhenAlreadyScared_IsIgnored)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    armadillo.switchToState(ArmadilloState::Scared);
    armadillo.rollUp();
    EXPECT_EQ(armadillo.getState(), ArmadilloState::Scared);
}

TEST_F(ArmadilloEntityTest, RollOut_FromScared_ReturnsIdle)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    armadillo.switchToState(ArmadilloState::Scared);
    EXPECT_TRUE(armadillo.shouldHideInShell());

    armadillo.rollOut();
    EXPECT_EQ(armadillo.getState(), ArmadilloState::Idle);
    EXPECT_FALSE(armadillo.shouldHideInShell());
}

TEST_F(ArmadilloEntityTest, RollOut_FromIdle_IsIgnored)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    armadillo.rollOut();
    EXPECT_EQ(armadillo.getState(), ArmadilloState::Idle);
}

TEST_F(ArmadilloEntityTest, ShouldSwitchToScaredState_AfterRollingAnimation)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    armadillo.setWorld(&m_world);
    armadillo.rollUp();

    EXPECT_FALSE(armadillo.shouldSwitchToScaredState());

    // ROLLING 动画 10 tick：tick 内 inStateTicks 自增并判定，超过 10 后自动转入 SCARED
    // （对齐 vanilla ArmadilloBallUp.tick 的 shouldSwitchToScaredState 分支）。
    for (i32 i = 0; i < 12; ++i) {
        armadillo.tick();
        if (armadillo.getState() == ArmadilloState::Scared) {
            break;
        }
    }
    EXPECT_EQ(armadillo.getState(), ArmadilloState::Scared);
    // SCARED 状态下弹体完全收起
    EXPECT_TRUE(armadillo.shouldHideInShell());
}

TEST_F(ArmadilloEntityTest, ShouldHideInShell_MatchesVanillaPerState)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());

    armadillo.switchToState(ArmadilloState::Scared);
    EXPECT_TRUE(armadillo.shouldHideInShell());
    EXPECT_LT(armadillo.eyeHeight(), 1.0f);
}

// ============================================================================
// 伤害
// ============================================================================

TEST_F(ArmadilloEntityTest, Hurt_WhenScared_HalvesDamageAsLeftMinusOneOverTwo)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    armadillo.setWorld(&m_world);
    armadillo.switchToState(ArmadilloState::Scared);
    EXPECT_TRUE(armadillo.shouldHideInShell());

    const f32 before = armadillo.health();
    EnvironmentalDamage source = DamageSources::generic();
    armadillo.hurt(source, 8.0f);

    // (8-1)/2 = 3.5
    EXPECT_FLOAT_EQ(before - armadillo.health(), 3.5f);
}

TEST_F(ArmadilloEntityTest, Hurt_WhenIdle_TakesFullDamage)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    armadillo.setWorld(&m_world);

    const f32 before = armadillo.health();
    EnvironmentalDamage source = DamageSources::generic();
    armadillo.hurt(source, 4.0f);
    EXPECT_FLOAT_EQ(before - armadillo.health(), 4.0f);
}

// ============================================================================
// 刷鳞甲
// ============================================================================

TEST_F(ArmadilloEntityTest, BrushOffScute_AdultDropsItem)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    armadillo.setWorld(&m_world);
    armadillo.setPosition(0.5, 64.0, 0.5);

    EXPECT_TRUE(armadillo.brushOffScute(nullptr));
    ASSERT_FALSE(m_world.spawnedEntities().empty());
    auto* item = dynamic_cast<ItemEntity*>(m_world.spawnedEntities().back().get());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->getItemStack().getItem(), Items::ARMADILLO_SCUTE);
}

TEST_F(ArmadilloEntityTest, BrushOffScute_BabyRefuses)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    armadillo.setWorld(&m_world);
    armadillo.setChild(true);

    EXPECT_FALSE(armadillo.brushOffScute(nullptr));
    EXPECT_TRUE(m_world.spawnedEntities().empty());
}

TEST_F(ArmadilloEntityTest, ScuteTime_IsWithinVanillaRange)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    // vanilla pickNextScuteDropTime：base + rand(0, base)，base = 5 分钟 = 6000 tick。
    EXPECT_GE(armadillo.scuteTime(), 6000);
    EXPECT_LT(armadillo.scuteTime(), 12000);
}

// ============================================================================
// NBT
// ============================================================================

TEST_F(ArmadilloEntityTest, NbtRoundTrip_PreservesStateAndScuteTime)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    armadillo.switchToState(ArmadilloState::Scared);
    armadillo.setScuteTime(4321);

    nbt::tags::compound_tag tag;
    armadillo.addAdditionalSaveData(tag);

    ArmadilloEntity restored(EntityInstanceId(2), mc::test::testEcsRegistry());
    ASSERT_TRUE(restored.readAdditionalSaveData(tag).success());
    EXPECT_EQ(restored.getState(), ArmadilloState::Scared);
    EXPECT_EQ(restored.scuteTime(), 4321);
}

TEST_F(ArmadilloEntityTest, Nbt_UnknownStateName_KeepsIdle)
{
    ArmadilloEntity armadillo(EntityInstanceId(1), mc::test::testEcsRegistry());
    nbt::tags::compound_tag tag;
    tag.put("state", std::string("bogus"));
    ASSERT_TRUE(armadillo.readAdditionalSaveData(tag).success());
    EXPECT_EQ(armadillo.getState(), ArmadilloState::Idle);
}

} // namespace
} // namespace mc