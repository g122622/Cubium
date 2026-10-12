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
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
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

namespace mc::test {

// ============================================================================
// 测试访问器：访问 AvoidEntityGoal 的 private 成员 m_avoidTarget
// ============================================================================
// AvoidEntityGoal 已在 AvoidEntityGoal.hpp 中声明
// friend class test::AvoidEntityGoalTestAccessor。

class AvoidEntityGoalTestAccessor {
public:
    static LivingEntity* getAvoidTarget(const entity::ai::goal::AvoidEntityGoal& goal) { return goal.m_avoidTarget; }

    static void setAvoidTarget(entity::ai::goal::AvoidEntityGoal& goal, LivingEntity* target)
    {
        goal.m_avoidTarget = target;
    }
};

} // namespace mc::test

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

// 暴露 protected 成员 m_target 以验证 UAF 守卫是否清空了缓存的裸指针。
// RangedAttackGoal 中 m_target 为 protected（RangedAttackGoals.hpp:75），
// 故可在派生类中直接读取。
class ExposedRangedAttackGoal final : public RangedAttackGoal {
public:
    using RangedAttackGoal::RangedAttackGoal;

    [[nodiscard]] LivingEntity* cachedTarget() const { return m_target; }
};

class ExposedRangedBowAttackGoal final : public RangedBowAttackGoal {
public:
    using RangedBowAttackGoal::RangedBowAttackGoal;

    [[nodiscard]] LivingEntity* cachedTarget() const { return m_target; }
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

// ============================================================================
// 目标实体裸指针 UAF 回归测试
// ============================================================================
//
// 背景：EntityManager::tick() 采用 graveyard 延迟析构——实体被 remove() 标记后，
// 对象要到下一 tick 的 `m_graveyard.clear()`（EntityManager.cpp:345）才真正析构。
// 因此存在整整一个 _tickEntities() 遍历周期，其间 isAlive()==false 但对象仍已分配。
// GoalSelector::tick() 的 Phase 1 先跑 shouldContinueExecuting()->resetTask()，
// Phase 3 再跑 tick()；但同一 Phase 3 循环内先执行的兄弟 goal 可能在本次 tick 内
// 移除目标，此时本 goal 的 Phase 1 已经跑过。故 tick() 内部必须再检查一次 isAlive()。
// 契约见 EntityManager.cpp:380-381。
//
// 下列用例直接复现 CI 崩溃路径（nightly run 38193815032 shard 18/23）：
//   shard 18: heap-use-after-free @ Entity.hpp:326 Entity::x()
//             <- RangedBowAttackGoal::tick() RangedAttackGoals.cpp:221
//   shard 23: heap-use-after-free @ Entity.hpp:409 Entity::distanceSqTo()
//             <- AvoidEntityGoal::tick() AvoidEntityGoal.cpp:121

TEST(AiGoalRegressionTest, RangedAttackGoal_ClearsRemovedTarget)
{
    TestGoalWorld world;
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    TestCreatureEntity target;
    target.setWorld(&world);
    target.setPosition(5.0f, 64.0f, 0.0f);

    creature.setAttackTarget(&target);

    ExposedRangedAttackGoal goal(&creature, 1.0, 20, 40, 15.0f);
    ASSERT_TRUE(goal.shouldExecute());
    ASSERT_EQ(goal.cachedTarget(), &target);

    // 目标被移除（isAlive()==false）但对象仍存活（graveyard 延迟析构窗口）。
    target.discard();
    ASSERT_FALSE(target.isAlive());

    // shouldContinueExecuting() 必须放手。修复前此处置为 true：shouldExecute() 返回
    // false（目标不存活）后，`|| (m_mob && !m_mob->navigator()->noPath())` 的第二个
    // 析取项在测试世界中因 noPath()==true 而恰好为 false，故本断言在测试环境不具区分度；
    // 真正的区分度由下方 tick() 清空指针的断言提供。
    EXPECT_FALSE(goal.shouldContinueExecuting());

    // 关键断言：tick() 必须自行检测目标已移除并清空缓存指针，绝不解引用它。
    // 修复前 tick() 会走到 m_target->x()（shard 18 崩溃点），
    // 修复后 m_target 被置空。
    goal.tick();
    EXPECT_EQ(goal.cachedTarget(), nullptr);
}

TEST(AiGoalRegressionTest, RangedBowAttackGoal_ClearsRemovedTarget)
{
    TestGoalWorld world;
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    TestCreatureEntity target;
    target.setWorld(&world);
    target.setPosition(10.0f, 64.0f, 0.0f);

    TestBowItem bowItem;
    creature.setMainHandItem(ItemStack(&bowItem, 1));
    creature.setAttackTarget(&target);

    ExposedRangedBowAttackGoal goal(&creature, 1.0, 20, 40);
    ASSERT_TRUE(goal.shouldExecute());
    ASSERT_EQ(goal.cachedTarget(), &target);

    // 复现 shard 18 崩溃前提：目标已被移除，对象仍在 graveyard 中未析构。
    target.discard();
    ASSERT_FALSE(target.isAlive());

    // 崩溃点原为 RangedAttackGoals.cpp:221 的 m_target->x()。
    // tick() 内的 isAlive() 守卫必须拦截并清空指针。
    goal.tick();
    EXPECT_EQ(goal.cachedTarget(), nullptr);
}

TEST(AiGoalRegressionTest, AvoidEntityGoal_ClearsRemovedAvoidTarget)
{
    TestGoalWorld world;
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    TestCreatureEntity avoidTarget;
    avoidTarget.setWorld(&world);
    avoidTarget.setPosition(3.0f, 64.0f, 0.0f);

    AvoidEntityGoal goal(&creature, 6.0f, 1.0, 1.3);
    mc::test::AvoidEntityGoalTestAccessor::setAvoidTarget(goal, &avoidTarget);
    ASSERT_EQ(mc::test::AvoidEntityGoalTestAccessor::getAvoidTarget(goal), &avoidTarget);

    // 复现 shard 23 崩溃前提：被躲避目标已被移除，对象仍在 graveyard 中未析构。
    avoidTarget.discard();
    ASSERT_FALSE(avoidTarget.isAlive());

    // shouldContinueExecuting() 必须放手。修复前本 goal 只看 nav->noPath()，
    // 目标死亡完全不影响其返回值。
    EXPECT_FALSE(goal.shouldContinueExecuting());

    // 崩溃点原为 AvoidEntityGoal.cpp:121 的 m_creature->distanceSqTo(*m_avoidTarget)。
    // tick() 内的 isAlive() 守卫必须拦截，并清空裸指针。
    goal.tick();
    EXPECT_EQ(mc::test::AvoidEntityGoalTestAccessor::getAvoidTarget(goal), nullptr);
}
