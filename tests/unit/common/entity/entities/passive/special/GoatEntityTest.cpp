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
 * @file GoatEntityTest.cpp
 * @brief 山羊实体测试
 *
 * 覆盖 vanilla Goat 的关键语义：
 * 1. 默认状态：非尖叫、双角齐全
 * 2. 尖叫山羊切换与冲撞冷却区间（尖叫 100~300 / 普通 600~6000）
 * 3. 掉落山羊角：优先掉剩下的那只角，两只都在时随机，无角时不可掉
 * 4. 摔落伤害减免 10
 * 5. NBT：IsScreamingGoat / HasLeftHorn / HasRightHorn 往返
 * 6. 低头计时：setLoweringHead 后每 tick +1，取消后每 tick -2（0..20 夹取）
 * 7. 长跳：抛射初速度计算、长跳中姿态碰撞箱缩放、长跳目标已注册
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/special/GoatLongJumpGoal.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/passive/special/GoatEntity.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <string>

namespace mc {
namespace {

class GoatTestWorld final : public mc::test::BaseTestWorld {
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

class GoatEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::ensureInitialized();
    }

    GoatTestWorld m_world;
};

TEST_F(GoatEntityTest, DefaultsToRegularGoatWithBothHorns)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(goat.isScreamingGoat());
    EXPECT_TRUE(goat.hasLeftHorn());
    EXPECT_TRUE(goat.hasRightHorn());
    EXPECT_FALSE(goat.isLoweringHead());
}

TEST_F(GoatEntityTest, ScreamingGoat_SyncAndCooldownRange)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_GE(goat.ramCooldown(), GoatEntity::RAM_COOLDOWN_MIN);
    EXPECT_LE(goat.ramCooldown(), GoatEntity::RAM_COOLDOWN_MAX);

    goat.setScreamingGoat(true);
    EXPECT_TRUE(goat.isScreamingGoat());
    EXPECT_TRUE(goat.dataManager().get(GoatEntity::DATA_IS_SCREAMING_GOAT_PARAM));

    for (i32 i = 0; i < 32; ++i) {
        const i32 cooldown = goat.pickNextRamCooldown();
        EXPECT_GE(cooldown, GoatEntity::RAM_COOLDOWN_SCREAMER_MIN);
        EXPECT_LE(cooldown, GoatEntity::RAM_COOLDOWN_SCREAMER_MAX);
    }
}

TEST_F(GoatEntityTest, DropHorn_PrefersRemainingHorn)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    goat.setWorld(&m_world);
    goat.setPosition(0.5, 64.0, 0.5);

    // 左角已被移除 → 只可能掉右角
    goat.removeHorns();
    goat.dataManager().set(GoatEntity::DATA_HAS_RIGHT_HORN_PARAM, true);

    EXPECT_TRUE(goat.dropHorn());
    EXPECT_TRUE(goat.hasLeftHorn() == false);
    EXPECT_FALSE(goat.hasRightHorn());
    ASSERT_FALSE(m_world.spawnedEntities().empty());
    auto* item = dynamic_cast<ItemEntity*>(m_world.spawnedEntities().back().get());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->getItemStack().getItem(), Items::GOAT_HORN);
}

TEST_F(GoatEntityTest, DropHorn_WithoutHornsFails)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    goat.setWorld(&m_world);
    goat.removeHorns();

    EXPECT_FALSE(goat.dropHorn());
    EXPECT_TRUE(m_world.spawnedEntities().empty());
}

TEST_F(GoatEntityTest, AddHorns_RestoresBoth)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    goat.removeHorns();
    EXPECT_FALSE(goat.hasLeftHorn());
    EXPECT_FALSE(goat.hasRightHorn());
    goat.addHorns();
    EXPECT_TRUE(goat.hasLeftHorn());
    EXPECT_TRUE(goat.hasRightHorn());
}

TEST_F(GoatEntityTest, FallDamageReduction_IsTen)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(goat.fallDamageReduction(), 10);
}

TEST_F(GoatEntityTest, NbtRoundTrip_PreservesScreamingAndHorns)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    goat.setScreamingGoat(true);
    goat.dataManager().set(GoatEntity::DATA_HAS_LEFT_HORN_PARAM, false);

    nbt::tags::compound_tag tag;
    goat.addAdditionalSaveData(tag);

    GoatEntity restored(EntityInstanceId(2), mc::test::testEcsRegistry());
    ASSERT_TRUE(restored.readAdditionalSaveData(tag).success());
    EXPECT_TRUE(restored.isScreamingGoat());
    EXPECT_FALSE(restored.hasLeftHorn());
    EXPECT_TRUE(restored.hasRightHorn());
}

TEST_F(GoatEntityTest, LowerHeadTick_RisesAndFallsAtVanillaRates)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    goat.setWorld(&m_world);

    goat.setLoweringHead(true);
    for (i32 i = 0; i < 5; ++i) {
        goat.tick();
    }
    EXPECT_EQ(goat.lowerHeadTick(), 5);

    goat.setLoweringHead(false);
    for (i32 i = 0; i < 10; ++i) {
        goat.tick();
    }
    // 每 tick -2，夹取到 0
    EXPECT_EQ(goat.lowerHeadTick(), 0);
}

TEST_F(GoatEntityTest, LowerHeadTick_ClampsAtTwenty)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    goat.setWorld(&m_world);
    goat.setLoweringHead(true);
    for (i32 i = 0; i < 40; ++i) {
        goat.tick();
    }
    EXPECT_EQ(goat.lowerHeadTick(), GoatEntity::RAM_PREPARE_TIME);
}

// ============================================================================
// 山羊角乐器
// ============================================================================

TEST_F(GoatEntityTest, CreateHorn_CarriesInstrumentComponent)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    goat.setUuid("00000000-0000-0000-0000-000000000001");

    const ItemStack horn = goat.createHorn();
    ASSERT_FALSE(horn.isEmpty());
    // 对齐 vanilla Goat.createHorn：从乐器标签随机取音色写入 instrument 组件
    const std::string instrument = horn.getInstrument();
    EXPECT_FALSE(instrument.empty());
    EXPECT_NE(instrument.find("minecraft:"), std::string::npos);
}

TEST_F(GoatEntityTest, CreateHorn_InstrumentsDifferByScreaming)
{
    GoatEntity regular(EntityInstanceId(1), mc::test::testEcsRegistry());
    GoatEntity screaming(EntityInstanceId(2), mc::test::testEcsRegistry());
    screaming.setScreamingGoat(true);
    regular.setUuid("11111111-2222-3333-4444-555555555555");
    screaming.setUuid("11111111-2222-3333-4444-555555555555");

    const std::string regularInstrument = regular.createHorn().getInstrument();
    const std::string screamingInstrument = screaming.createHorn().getInstrument();
    ASSERT_FALSE(regularInstrument.empty());
    ASSERT_FALSE(screamingInstrument.empty());

    // 两个标签互斥：普通山羊角音色 {ponder,sing,seek,feel}，尖叫 {admire,call,yearn,dream}
    static const char* const kRegular[] = {
        "minecraft:ponder_goat_horn",
        "minecraft:sing_goat_horn",
        "minecraft:seek_goat_horn",
        "minecraft:feel_goat_horn",
    };
    static const char* const kScreaming[] = {
        "minecraft:admire_goat_horn",
        "minecraft:call_goat_horn",
        "minecraft:yearn_goat_horn",
        "minecraft:dream_goat_horn",
    };
    EXPECT_TRUE(std::find(std::begin(kRegular), std::end(kRegular), regularInstrument) != std::end(kRegular));
    EXPECT_TRUE(std::find(std::begin(kScreaming), std::end(kScreaming), screamingInstrument) != std::end(kScreaming));
}

// ============================================================================
// 长跳
// ============================================================================

TEST_F(GoatEntityTest, LongJumpGoal_RegisteredWithLowestPriority)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    bool found = false;
    for (const auto& pg : goat.goalSelector().getAllGoals()) {
        if (pg.getGoal() != nullptr && pg.getGoal()->getTypeName() == "GoatLongJumpGoal") {
            found = true;
            // 长跳优先级最低（9），不抢占冲撞/繁殖/漫步
            EXPECT_EQ(pg.getPriority(), 9);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(GoatEntityTest, LongJump_DimensionsShrinkToSeventyPercent)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    const entity::EntitySize standing = goat.getDimensions(EntityPose::Standing);
    const entity::EntitySize jumping = goat.getDimensions(EntityPose::LongJumping);

    // 对齐 vanilla Goat.LONG_JUMPING_DIMENSIONS = 0.9x1.3 scale 0.7
    EXPECT_NEAR(jumping.width(), standing.width() * GoatEntity::LONG_JUMPING_SCALE, 1e-5f);
    EXPECT_NEAR(jumping.height(), standing.height() * GoatEntity::LONG_JUMPING_SCALE, 1e-5f);
}

TEST_F(GoatEntityTest, LongJumpVector_SolvesFlatShortJump)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    entity::ai::goal::GoatLongJumpGoal goal(&goat);

    // 水平 3 格、等高的小跳：应能求出向上且朝 +X 的初速度
    const Vector3 from(0.0, 64.0, 0.0);
    const Vector3 to(3.5, 64.0, 0.5);
    const auto velocity = goal.calculateJumpVector(from, to, 3.5714288, 65);

    ASSERT_TRUE(velocity.has_value());
    EXPECT_GT(velocity->y, 0.0);
    EXPECT_GT(velocity->x, 0.0);
    // 初速度不超过 JUMP_STRENGTH * 倍率
    const f64 speed = std::sqrt(velocity->x * velocity->x + velocity->y * velocity->y + velocity->z * velocity->z);
    EXPECT_LE(speed, 3.5714288 + 1e-6);
}

TEST_F(GoatEntityTest, LongJumpVector_RejectsUnreachableDistance)
{
    GoatEntity goat(EntityInstanceId(1), mc::test::testEcsRegistry());
    entity::ai::goal::GoatLongJumpGoal goal(&goat);

    // 低速上限（0.1）下 3 格小跳也无解，验证"求解速度 > 上限即拒绝"
    const Vector3 from(0.0, 64.0, 0.0);
    const Vector3 to(3.5, 64.0, 0.5);
    EXPECT_FALSE(goal.calculateJumpVector(from, to, 0.1, 65).has_value());
}

} // namespace
} // namespace mc