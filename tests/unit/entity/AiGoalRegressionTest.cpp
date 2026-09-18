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
#include "common/core/Constants.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <algorithm>
#include <utility>

using namespace mc;
using namespace mc::entity::ai::goal;

namespace {

class TestGoalWorld final : public mc::test::BaseTestWorld {
public:
    void setEntities(std::vector<Entity*> entities) { m_entities = std::move(entities); }

    void setAllWater(bool enabled) { m_allWater = enabled; }

    void setAllLava(bool enabled) { m_allLava = enabled; }

    void setWaterBlock(i32 x, i32 y, i32 z) { m_waterBlocks.push_back(BlockPos(x, y, z)); }

    void setLavaBlock(i32 x, i32 y, i32 z) { m_lavaBlocks.push_back(BlockPos(x, y, z)); }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity* except) const override
    {
        std::vector<Entity*> result;
        for (Entity* entity : m_entities) {
            if (entity == except) {
                continue;
            }
            result.push_back(entity);
        }
        return result;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(
        const Vector3& pos, f32 range, const Entity* except) const override
    {
        std::vector<Entity*> result;
        const f32 rangeSq = range * range;

        for (Entity* entity : m_entities) {
            if (entity == except) {
                continue;
            }

            if (pos.distanceSquared(entity->position()) <= rangeSq) {
                result.push_back(entity);
            }
        }

        return result;
    }

    [[nodiscard]] bool isWaterAt(const BlockPos& pos) const override
    {
        if (m_allWater) {
            return true;
        }

        return contains(m_waterBlocks, pos.x, pos.y, pos.z);
    }

    [[nodiscard]] bool isLavaAt(const BlockPos& pos) const override
    {
        if (m_allLava) {
            return true;
        }

        return contains(m_lavaBlocks, pos.x, pos.y, pos.z);
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("TestGoalWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("TestGoalWorld::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
private:
    [[nodiscard]] static bool contains(const std::vector<BlockPos>& blocks, i32 x, i32 y, i32 z)
    {
        const auto it = std::find_if(blocks.begin(), blocks.end(), [x, y, z](const BlockPos& pos) {
            return pos.x == x && pos.y == y && pos.z == z;
        });
        return it != blocks.end();
    }

    bool m_allWater = false;
    bool m_allLava = false;
    std::vector<Entity*> m_entities;
    std::vector<BlockPos> m_waterBlocks;
    std::vector<BlockPos> m_lavaBlocks;
};

class TestCreatureEntity final : public CreatureEntity {
public:
    TestCreatureEntity()
        : CreatureEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

class TestTemptItem final : public Item {
public:
    TestTemptItem()
        : Item(ItemProperties().maxStackSize(64))
    {}
};

class TestPlainItem final : public Item {
public:
    TestPlainItem()
        : Item(ItemProperties().maxStackSize(64))
    {}
};

class TestBowItem final : public Item {
public:
    TestBowItem()
        : Item(ItemProperties().maxStackSize(1))
    {}

    [[nodiscard]] UseAction getUseAction(const ItemStack&) const override { return UseAction::Bow; }
};

class ExposedPanicGoal final : public PanicGoal {
public:
    using PanicGoal::_getRandomWaterPosition;
    using PanicGoal::PanicGoal;
};

class ExposedWaterAvoidingRandomWalkingGoal final : public WaterAvoidingRandomWalkingGoal {
public:
    using WaterAvoidingRandomWalkingGoal::isInWaterOrLava;
    using WaterAvoidingRandomWalkingGoal::WaterAvoidingRandomWalkingGoal;
};

} // namespace

TEST(AiGoalRegressionTest, TemptGoal_UsesTemptingPlayerHandItems)
{
    TestGoalWorld world;
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    TestTemptItem temptItem;
    TestPlainItem nonTemptItem;

    Player closePlayer(2, "ClosePlayer", mc::test::testEcsRegistry());
    closePlayer.setWorld(&world);
    closePlayer.setPosition(2.0f, 64.0f, 0.0f);
    closePlayer.setHealth(closePlayer.maxHealth());
    closePlayer.getHeldItem(Hand::MainHand) = ItemStack(&nonTemptItem, 1);
    closePlayer.getHeldItem(Hand::OffHand) = ItemStack(&nonTemptItem, 1);

    Player temptingPlayer(3, "TemptingPlayer", mc::test::testEcsRegistry());
    temptingPlayer.setWorld(&world);
    temptingPlayer.setPosition(6.0f, 64.0f, 0.0f);
    temptingPlayer.setHealth(temptingPlayer.maxHealth());
    temptingPlayer.getHeldItem(Hand::MainHand) = ItemStack(&nonTemptItem, 1);
    temptingPlayer.getHeldItem(Hand::OffHand) = ItemStack(&temptItem, 1);

    world.setEntities({&closePlayer, &temptingPlayer});

    TemptGoal goal(&creature, 1.0, [&temptItem](const ItemStack& stack) { return stack.getItem() == &temptItem; });

    EXPECT_TRUE(goal.shouldExecute());
    goal.startExecuting();
    EXPECT_TRUE(goal.shouldContinueExecuting());

    temptingPlayer.getHeldItem(Hand::OffHand) = ItemStack(&nonTemptItem, 1);
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST(AiGoalRegressionTest, PanicGoal_FindsNearbyWaterWhenBurning)
{
    // 标签成员集在 DamageTypeTags::initialize() 注册。此回归测试为 TEST（无 fixture SetUp），
    // 须显式初始化一次（进程级幂等）。未初始化时 PANIC_CAUSES 成员集为空，source.is(PANIC_CAUSES)
    // 恒返 false，shouldPanic 恒 false。
    DamageTypeTags::initialize();

    TestGoalWorld world;
    world.setAllWater(true);
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(10.0f, 64.0f, 10.0f);

    // 对齐 vanilla PanicGoal 语义：shouldExecute 先调 shouldPanic（lastDamageSource.is(PANIC_CAUSES)），
    // 通过后才在着火时找水。真实游戏中实体着火总因受 fire 类伤害（InFire/Lava/OnFire 均在
    // PANIC_CAUSES），故此处先 apply 一次 OnFire 伤害设 lastDamageSource=OnFire，使 shouldPanic=true，
    // 再 setFire 模拟着火状态进入找水分支。单纯 setFire(40) 不设 lastDamageSource（vanilla 亦然），
    // shouldPanic=false 不会触发恐慌——这正是修复后的正确语义。
    auto onFireSource = DamageSources::onFire();
    creature.actuallyHurt(onFireSource, 1.0f);
    creature.setFire(40);

    ExposedPanicGoal goal(&creature, 1.0);

    EXPECT_TRUE(goal.shouldExecute());
    const BlockPos waterPos = goal._getRandomWaterPosition(8, 4);
    EXPECT_NE(waterPos.x, 0);
    EXPECT_NE(waterPos.y, 0);
    EXPECT_NE(waterPos.z, 0);
}

TEST(AiGoalRegressionTest, WaterAvoidingRandomWalkingGoal_DetectsWaterAndLava)
{
    TestGoalWorld world;
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    ExposedWaterAvoidingRandomWalkingGoal goal(&creature, 1.0, 1.0f);

    world.setWaterBlock(0, 64, 0);
    world.setLavaBlock(1, 64, 0);

    EXPECT_TRUE(goal.isInWaterOrLava(0.2, 64.1, 0.2));
    EXPECT_TRUE(goal.isInWaterOrLava(1.8, 64.0, 0.2));
    EXPECT_FALSE(goal.isInWaterOrLava(5.0, 64.0, 5.0));
    EXPECT_TRUE(goal.shouldExecute());
}

TEST(AiGoalRegressionTest, RangedBowAttackGoal_RequiresBowUseAction)
{
    TestGoalWorld world;
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    TestCreatureEntity target;
    target.setWorld(&world);
    target.setPosition(10.0f, 64.0f, 0.0f);

    TestBowItem bowItem;
    TestPlainItem plainItem;

    creature.setMainHandItem(ItemStack(&plainItem, 1));
    creature.setAttackTarget(&target);

    RangedBowAttackGoal goal(&creature, 1.0, 20, 40);
    EXPECT_FALSE(goal.shouldExecute());

    creature.setMainHandItem(ItemStack(&bowItem, 1));
    EXPECT_TRUE(goal.shouldExecute());
}
