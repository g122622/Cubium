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
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/special/CatGoals.hpp"
#include "common/entity/entities/passive/tamable/CatEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

// ==================== 测试辅助 ====================

/**
 * @brief CatGoals 测试用简易猫实体
 *
 * 提供公开目标选择器的测试实体
 */
class TestCatEntity : public CatEntity {
public:
    explicit TestCatEntity(EntityId id)
        : CatEntity(id)
    {}

    entity::ai::GoalSelector& testGoalSelector() { return goalSelector(); }
};

// ==================== CatLieOnBedGoal 测试 ====================

class CatLieOnBedGoalTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(CatLieOnBedGoalTest, Construction_DoesNotCrash)
{
    TestCatEntity cat(EntityId(1));
    EXPECT_NO_THROW({ auto goal = std::make_unique<entity::ai::goal::CatLieOnBedGoal>(&cat, 1.1); });
}

TEST_F(CatLieOnBedGoalTest, TypeName)
{
    TestCatEntity cat(EntityId(1));
    auto goal = std::make_unique<entity::ai::goal::CatLieOnBedGoal>(&cat, 1.1);
    EXPECT_EQ(goal->getTypeName(), "CatLieOnBedGoal");
}

TEST_F(CatLieOnBedGoalTest, MutexFlags)
{
    TestCatEntity cat(EntityId(1));
    auto goal = std::make_unique<entity::ai::goal::CatLieOnBedGoal>(&cat, 1.1);

    // CatLieOnBedGoal 应该有 Jump 和 Move 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(CatLieOnBedGoalTest, ShouldExecute_ReturnsFalse_WhenNotTamed)
{
    // 未驯服的猫不应寻找床
    TestCatEntity cat(EntityId(1));
    // 默认未驯服
    auto goal = std::make_unique<entity::ai::goal::CatLieOnBedGoal>(&cat, 1.1);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(CatLieOnBedGoalTest, ShouldExecute_ReturnsFalse_WhenSitting)
{
    // 坐下的猫不应寻找床
    TestCatEntity cat(EntityId(1));
    cat.setTamed(true);
    cat.setSitting(true);
    auto goal = std::make_unique<entity::ai::goal::CatLieOnBedGoal>(&cat, 1.1);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(CatLieOnBedGoalTest, ShouldExecute_ReturnsFalse_WhenAlreadyLying)
{
    // 已躺下的猫不应重新寻找床
    TestCatEntity cat(EntityId(1));
    cat.setTamed(true);
    cat.setLieDown(true);
    auto goal = std::make_unique<entity::ai::goal::CatLieOnBedGoal>(&cat, 1.1);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(CatLieOnBedGoalTest, ShouldExecute_ReturnsFalse_NullCat)
{
    // 空指针猫不应崩溃
    auto goal = std::make_unique<entity::ai::goal::CatLieOnBedGoal>(nullptr, 1.1);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(CatLieOnBedGoalTest, ResetTask_ClearsLieDown)
{
    // resetTask 应清除躺下状态
    TestCatEntity cat(EntityId(1));
    cat.setTamed(true);
    cat.setLieDown(true);
    auto goal = std::make_unique<entity::ai::goal::CatLieOnBedGoal>(&cat, 1.1);
    goal->resetTask();
    EXPECT_FALSE(cat.isLieDown());
}

// ==================== CatRelaxOnOwnerGoal 测试 ====================

class CatRelaxOnOwnerGoalTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(CatRelaxOnOwnerGoalTest, Construction_DoesNotCrash)
{
    TestCatEntity cat(EntityId(1));
    EXPECT_NO_THROW({ auto goal = std::make_unique<entity::ai::goal::CatRelaxOnOwnerGoal>(&cat, 0.6); });
}

TEST_F(CatRelaxOnOwnerGoalTest, TypeName)
{
    TestCatEntity cat(EntityId(1));
    auto goal = std::make_unique<entity::ai::goal::CatRelaxOnOwnerGoal>(&cat, 0.6);
    EXPECT_EQ(goal->getTypeName(), "CatRelaxOnOwnerGoal");
}

TEST_F(CatRelaxOnOwnerGoalTest, MutexFlags)
{
    TestCatEntity cat(EntityId(1));
    auto goal = std::make_unique<entity::ai::goal::CatRelaxOnOwnerGoal>(&cat, 0.6);

    // CatRelaxOnOwnerGoal 应该有 Jump, Move, Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(CatRelaxOnOwnerGoalTest, ShouldExecute_ReturnsFalse_WhenNotTamed)
{
    TestCatEntity cat(EntityId(1));
    // 默认未驯服
    auto goal = std::make_unique<entity::ai::goal::CatRelaxOnOwnerGoal>(&cat, 0.6);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(CatRelaxOnOwnerGoalTest, ShouldExecute_ReturnsFalse_WhenSitting)
{
    TestCatEntity cat(EntityId(1));
    cat.setTamed(true);
    cat.setSitting(true);
    auto goal = std::make_unique<entity::ai::goal::CatRelaxOnOwnerGoal>(&cat, 0.6);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(CatRelaxOnOwnerGoalTest, ShouldExecute_ReturnsFalse_NullCat)
{
    auto goal = std::make_unique<entity::ai::goal::CatRelaxOnOwnerGoal>(nullptr, 0.6);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(CatRelaxOnOwnerGoalTest, ShouldContinueExecuting_ReturnsFalse_NullCat)
{
    auto goal = std::make_unique<entity::ai::goal::CatRelaxOnOwnerGoal>(nullptr, 0.6);
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(CatRelaxOnOwnerGoalTest, ResetTask_ClearsStates)
{
    // resetTask 应清除躺下和放松状态
    TestCatEntity cat(EntityId(1));
    cat.setTamed(true);
    cat.setLieDown(true);
    cat.setRelaxStateOne(true);
    auto goal = std::make_unique<entity::ai::goal::CatRelaxOnOwnerGoal>(&cat, 0.6);
    goal->resetTask();
    EXPECT_FALSE(cat.isLieDown());
    EXPECT_FALSE(cat.isRelaxStateOne());
}

// ==================== CatGoalConstants 测试 ====================

TEST_F(CatLieOnBedGoalTest, Constants_MatchMCValues)
{
    // 验证常量与 MC 原版一致
    using namespace entity::ai::goal::CatGoalConstants;
    EXPECT_EQ(LIE_ON_BED_SEARCH_RANGE, 6);
    EXPECT_EQ(LIE_ON_BED_VERTICAL_START, -2);
    EXPECT_EQ(RELAX_ON_OWNER_DELAY, 16);
    EXPECT_FLOAT_EQ(MORNING_GIFT_CHANCE, 0.7f);
    EXPECT_DOUBLE_EQ(RELAX_ON_OWNER_NEAR_DIST_SQ, 2.5);
    EXPECT_DOUBLE_EQ(RELAX_ON_OWNER_SEARCH_DIST_SQ, 10000.0);
    EXPECT_FLOAT_EQ(SPACE_OCCUPIED_CHECK_DIST, 2.0f);
    EXPECT_EQ(LIE_ON_BED_MOVE_INTERVAL, 40);
}

// ==================== CatEntity 目标注册验证 ====================

class CatGoalRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }
};

TEST_F(CatGoalRegistrationTest, CatHasCatLieOnBedGoal)
{
    // CatEntity::registerGoals() 应注册 CatLieOnBedGoal
    TestCatEntity cat(EntityId(1));

    bool hasLieOnBedGoal = false;
    for (const auto& pg : cat.testGoalSelector().getAllGoals()) {
        const auto* goal = pg.getGoal();
        if (dynamic_cast<const entity::ai::goal::CatLieOnBedGoal*>(goal) != nullptr) {
            hasLieOnBedGoal = true;
            break;
        }
    }
    EXPECT_TRUE(hasLieOnBedGoal) << "CatEntity should have CatLieOnBedGoal registered";
}

TEST_F(CatGoalRegistrationTest, CatHasCatRelaxOnOwnerGoal)
{
    // CatEntity::registerGoals() 应注册 CatRelaxOnOwnerGoal
    TestCatEntity cat(EntityId(1));

    bool hasRelaxGoal = false;
    for (const auto& pg : cat.testGoalSelector().getAllGoals()) {
        const auto* goal = pg.getGoal();
        if (dynamic_cast<const entity::ai::goal::CatRelaxOnOwnerGoal*>(goal) != nullptr) {
            hasRelaxGoal = true;
            break;
        }
    }
    EXPECT_TRUE(hasRelaxGoal) << "CatEntity should have CatRelaxOnOwnerGoal registered";
}

TEST_F(CatGoalRegistrationTest, CatLieOnBedGoalPriority)
{
    // CatLieOnBedGoal 应在优先级 5
    TestCatEntity cat(EntityId(1));

    bool found = false;
    for (const auto& pg : cat.testGoalSelector().getAllGoals()) {
        const auto* goal = pg.getGoal();
        if (dynamic_cast<const entity::ai::goal::CatLieOnBedGoal*>(goal) != nullptr) {
            EXPECT_EQ(pg.getPriority(), 5) << "CatLieOnBedGoal should have priority 5";
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(CatGoalRegistrationTest, CatRelaxOnOwnerGoalPriority)
{
    // CatRelaxOnOwnerGoal 应在优先级 3
    TestCatEntity cat(EntityId(1));

    bool found = false;
    for (const auto& pg : cat.testGoalSelector().getAllGoals()) {
        const auto* goal = pg.getGoal();
        if (dynamic_cast<const entity::ai::goal::CatRelaxOnOwnerGoal*>(goal) != nullptr) {
            EXPECT_EQ(pg.getPriority(), 3) << "CatRelaxOnOwnerGoal should have priority 3";
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// ==================== 晨礼后备物品列表测试 ====================

class CatMorningGiftItemsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(CatMorningGiftItemsTest, FallbackGiftItems_AllNonNull)
{
    // CatRelaxOnOwnerGoal::_giveMorningGift 的后备物品列表中
    // 所有 Items:: 指针必须非空，否则该物品会被跳过（权重计算时跳过 nullptr）
    ASSERT_NE(Items::RABBIT_HIDE, nullptr) << "Items::RABBIT_HIDE 应该已注册";
    ASSERT_NE(Items::RABBIT_FOOT, nullptr) << "Items::RABBIT_FOOT 应该已注册";
    ASSERT_NE(Items::CHICKEN, nullptr) << "Items::CHICKEN 应该已注册";
    ASSERT_NE(Items::FEATHER, nullptr) << "Items::FEATHER 应该已注册";
    ASSERT_NE(Items::ROTTEN_FLESH, nullptr) << "Items::ROTTEN_FLESH 应该已注册";
    ASSERT_NE(Items::STRING, nullptr) << "Items::STRING 应该已注册";
    ASSERT_NE(Items::PHANTOM_MEMBRANE, nullptr) << "Items::PHANTOM_MEMBRANE 应该已注册";
}

TEST_F(CatMorningGiftItemsTest, FallbackGiftItems_HaveCorrectResourceLocations)
{
    // 验证 Items:: 指针指向的物品拥有正确的 ResourceLocation，
    // 与 MC 原版 cat_morning_gift 战利品表中的物品 ID 一致
    EXPECT_EQ(Items::RABBIT_HIDE->itemLocation(), ResourceLocation("minecraft", "rabbit_hide"));
    EXPECT_EQ(Items::RABBIT_FOOT->itemLocation(), ResourceLocation("minecraft", "rabbit_foot"));
    EXPECT_EQ(Items::CHICKEN->itemLocation(), ResourceLocation("minecraft", "chicken"));
    EXPECT_EQ(Items::FEATHER->itemLocation(), ResourceLocation("minecraft", "feather"));
    EXPECT_EQ(Items::ROTTEN_FLESH->itemLocation(), ResourceLocation("minecraft", "rotten_flesh"));
    EXPECT_EQ(Items::STRING->itemLocation(), ResourceLocation("minecraft", "string"));
    EXPECT_EQ(Items::PHANTOM_MEMBRANE->itemLocation(), ResourceLocation("minecraft", "phantom_membrane"));
}

} // namespace
} // namespace mc
