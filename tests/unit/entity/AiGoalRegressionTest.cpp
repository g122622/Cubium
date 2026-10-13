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
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/EndermanGoals.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/mod/bedrock/addon/binding/ScriptHandleRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <algorithm>
#include <memory>
#include <optional>
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
    static LivingEntity* getAvoidTarget(const entity::ai::goal::AvoidEntityGoal& goal)
    {
        return goal.m_avoidTarget.get();
    }

    static void setAvoidTarget(entity::ai::goal::AvoidEntityGoal& goal, LivingEntity* target)
    {
        goal.m_avoidTarget = target;
    }
};

class EndermanFindPlayerGoalTestAccessor {
public:
    static void preparePromotion(entity::ai::goal::EndermanFindPlayerGoal& goal, Player* player)
    {
        goal.m_targetPlayer = player;
        goal.m_aggroTime = 1;
    }

    static Player* pendingPlayer(const entity::ai::goal::EndermanFindPlayerGoal& goal)
    {
        return goal.m_targetPlayer.get();
    }
};

} // namespace mc::test

namespace {

class _TestGoalWorld final : public mc::test::BaseTestWorld {
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

        return _contains(m_waterBlocks, pos.x, pos.y, pos.z);
    }

    [[nodiscard]] bool isLavaAt(const BlockPos& pos) const override
    {
        if (m_allLava) {
            return true;
        }

        return _contains(m_lavaBlocks, pos.x, pos.y, pos.z);
    }

    // TODO: 本夹具尚不模拟 TickManager，当前用例不经过世界 tick。
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("_TestGoalWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("_TestGoalWorld::tickManager not implemented");
    }

    // 水和熔岩查询只在本夹具内维护。
private:
    [[nodiscard]] static bool _contains(const std::vector<BlockPos>& blocks, i32 x, i32 y, i32 z)
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

class _TestPathNavigator final : public entity::ai::pathfinding::PathNavigator {
public:
    using PathNavigator::PathNavigator;

    void setActivePath()
    {
        m_path = std::make_unique<entity::ai::pathfinding::Path>(
            std::vector<entity::ai::pathfinding::PathPoint>{{0, 64, 0}, {5, 64, 0}});
    }
};

class _TestCreatureEntity final : public CreatureEntity {
public:
    _TestCreatureEntity()
        : CreatureEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    void setActivePath()
    {
        auto navigator = std::make_unique<_TestPathNavigator>(this);
        navigator->setActivePath();
        m_navigator = std::move(navigator);
    }
};

class _TestTemptItem final : public Item {
public:
    _TestTemptItem()
        : Item(ItemProperties().maxStackSize(64))
    {}
};

class _TestPlainItem final : public Item {
public:
    _TestPlainItem()
        : Item(ItemProperties().maxStackSize(64))
    {}
};

class _TestBowItem final : public Item {
public:
    _TestBowItem()
        : Item(ItemProperties().maxStackSize(1))
    {}

    [[nodiscard]] UseAction getUseAction(const ItemStack&) const override { return UseAction::Bow; }
};

class _ExposedPanicGoal final : public PanicGoal {
public:
    using PanicGoal::_getRandomWaterPosition;
    using PanicGoal::PanicGoal;
};

// 暴露缓存引用，以验证目标移除和析构后的读取行为。
class _ExposedRangedAttackGoal final : public RangedAttackGoal {
public:
    using RangedAttackGoal::RangedAttackGoal;

    [[nodiscard]] LivingEntity* cachedTarget() const { return m_target.get(); }
};

class _ExposedRangedBowAttackGoal final : public RangedBowAttackGoal {
public:
    using RangedBowAttackGoal::RangedBowAttackGoal;

    [[nodiscard]] LivingEntity* cachedTarget() const { return m_target.get(); }
};

class _ExposedMeleeAttackGoal final : public MeleeAttackGoal {
public:
    using MeleeAttackGoal::MeleeAttackGoal;

    void setCachedTarget(LivingEntity* target) { m_attackTarget = target; }
    [[nodiscard]] LivingEntity* cachedTarget() const { return m_attackTarget.get(); }
};

class _ExposedWaterAvoidingRandomWalkingGoal final : public WaterAvoidingRandomWalkingGoal {
public:
    using WaterAvoidingRandomWalkingGoal::isInWaterOrLava;
    using WaterAvoidingRandomWalkingGoal::WaterAvoidingRandomWalkingGoal;
};

} // namespace

TEST(AiGoalRegressionTest, TemptGoal_UsesTemptingPlayerHandItems)
{
    _TestGoalWorld world;
    _TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    _TestTemptItem temptItem;
    _TestPlainItem nonTemptItem;

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

    _TestGoalWorld world;
    world.setAllWater(true);
    _TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(10.0f, 64.0f, 10.0f);

    // 先记录火焰伤害再模拟着火，使恐慌伤害标签检查通过。
    auto onFireSource = DamageSources::onFire();
    creature.actuallyHurt(onFireSource, 1.0f);
    creature.setFire(40);

    _ExposedPanicGoal goal(&creature, 1.0);

    EXPECT_TRUE(goal.shouldExecute());
    const BlockPos waterPos = goal._getRandomWaterPosition(8, 4);
    EXPECT_NE(waterPos.x, 0);
    EXPECT_NE(waterPos.y, 0);
    EXPECT_NE(waterPos.z, 0);
}

TEST(AiGoalRegressionTest, WaterAvoidingRandomWalkingGoal_DetectsWaterAndLava)
{
    _TestGoalWorld world;
    _TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    _ExposedWaterAvoidingRandomWalkingGoal goal(&creature, 1.0, 1.0f);

    world.setWaterBlock(0, 64, 0);
    world.setLavaBlock(1, 64, 0);

    EXPECT_TRUE(goal.isInWaterOrLava(0.2, 64.1, 0.2));
    EXPECT_TRUE(goal.isInWaterOrLava(1.8, 64.0, 0.2));
    EXPECT_FALSE(goal.isInWaterOrLava(5.0, 64.0, 5.0));
    EXPECT_TRUE(goal.shouldExecute());
}

TEST(AiGoalRegressionTest, RangedBowAttackGoal_RequiresBowUseAction)
{
    _TestGoalWorld world;
    _TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    _TestCreatureEntity target;
    target.setWorld(&world);
    target.setPosition(10.0f, 64.0f, 0.0f);

    _TestBowItem bowItem;
    _TestPlainItem plainItem;

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
// 目标可能在同一 tick 的兄弟目标执行期间移除，也可能在拥有者暂停 tick 时析构。
// 既验证逻辑移除后的停止行为，也验证目标实际释放后的恢复路径。

TEST(AiGoalRegressionTest, RangedAttackGoal_ClearsRemovedTarget)
{
    _TestGoalWorld world;
    _TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    _TestCreatureEntity target;
    target.setWorld(&world);
    target.setPosition(5.0f, 64.0f, 0.0f);

    creature.setAttackTarget(&target);

    _ExposedRangedAttackGoal goal(&creature, 1.0, 20, 40, 15.0f);
    ASSERT_TRUE(goal.shouldExecute());
    ASSERT_EQ(goal.cachedTarget(), &target);

    // 导航仍有路径，使缺少目标存活检查的旧逻辑必然返回 true。
    creature.setActivePath();
    ASSERT_FALSE(creature.navigator()->noPath());
    ASSERT_TRUE(goal.shouldContinueExecuting());

    // 目标被移除（isAlive()==false）但对象仍存活（graveyard 延迟析构窗口）。
    target.discard();
    ASSERT_FALSE(target.isAlive());

    // 目标失效时，即使导航仍有路径也必须停止。
    EXPECT_FALSE(goal.shouldContinueExecuting());

    // 移除后读取引用返回空值，tick 不应再访问目标。
    goal.tick();
    EXPECT_EQ(goal.cachedTarget(), nullptr);
}

TEST(AiGoalRegressionTest, RangedBowAttackGoal_ClearsRemovedTarget)
{
    _TestGoalWorld world;
    _TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    _TestCreatureEntity target;
    target.setWorld(&world);
    target.setPosition(10.0f, 64.0f, 0.0f);

    _TestBowItem bowItem;
    creature.setMainHandItem(ItemStack(&bowItem, 1));
    creature.setAttackTarget(&target);

    _ExposedRangedBowAttackGoal goal(&creature, 1.0, 20, 40);
    ASSERT_TRUE(goal.shouldExecute());
    ASSERT_EQ(goal.cachedTarget(), &target);

    // 复现 shard 18 崩溃前提：目标已被移除，对象仍在 graveyard 中未析构。
    target.discard();
    ASSERT_FALSE(target.isAlive());

    // 失效引用不得在 tick 中再次解引用。
    goal.tick();
    EXPECT_EQ(goal.cachedTarget(), nullptr);
}

TEST(AiGoalRegressionTest, AvoidEntityGoal_ClearsRemovedAvoidTarget)
{
    _TestGoalWorld world;
    _TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    _TestCreatureEntity avoidTarget;
    avoidTarget.setWorld(&world);
    avoidTarget.setPosition(3.0f, 64.0f, 0.0f);

    AvoidEntityGoal goal(&creature, 6.0f, 1.0, 1.3);
    mc::test::AvoidEntityGoalTestAccessor::setAvoidTarget(goal, &avoidTarget);
    ASSERT_EQ(mc::test::AvoidEntityGoalTestAccessor::getAvoidTarget(goal), &avoidTarget);

    creature.setActivePath();
    ASSERT_FALSE(creature.navigator()->noPath());
    ASSERT_TRUE(goal.shouldContinueExecuting());

    // 复现 shard 23 崩溃前提：被躲避目标已被移除，对象仍在 graveyard 中未析构。
    avoidTarget.discard();
    ASSERT_FALSE(avoidTarget.isAlive());

    // shouldContinueExecuting() 必须放手。修复前本 goal 只看 nav->noPath()，
    // 目标死亡完全不影响其返回值。
    EXPECT_FALSE(goal.shouldContinueExecuting());

    // 失效引用不得在 tick 中再次解引用。
    goal.tick();
    EXPECT_EQ(mc::test::AvoidEntityGoalTestAccessor::getAvoidTarget(goal), nullptr);
    EXPECT_FALSE(goal.shouldContinueExecuting());
    goal.resetTask();
    EXPECT_TRUE(creature.navigator()->noPath());
}

TEST(AiGoalRegressionTest, EntityReference_ExpiresOnRemovalAndDestruction)
{
    EntityReference<LivingEntity> reference;
    EntityReference<LivingEntity> copy;
    {
        _TestCreatureEntity target;
        reference = &target;
        copy = reference;
        ASSERT_EQ(reference.get(), &target);
        ASSERT_EQ(copy.get(), &target);
        target.discard();
        EXPECT_EQ(reference.get(), nullptr);
        EXPECT_EQ(copy.get(), nullptr);
        reference = &target;
        EXPECT_EQ(reference.get(), nullptr);
    }
    EXPECT_EQ(reference.get(), nullptr);
    EXPECT_EQ(copy.get(), nullptr);

    // 直接析构也必须失效，且后续分配在相同地址的新实体不能复活旧引用。
    std::optional<_TestCreatureEntity> target(std::in_place);
    reference = &*target;
    copy = reference;
    target.reset();
    target.emplace();
    EXPECT_EQ(reference.get(), nullptr);
    EXPECT_EQ(copy.get(), nullptr);
    reference = &*target;
    EXPECT_EQ(reference.get(), &*target);
    EXPECT_EQ(copy.get(), nullptr);
}

TEST(AiGoalRegressionTest, RangedAttackGoals_ResumeAfterTargetDestruction)
{
    _TestGoalWorld world;
    _TestCreatureEntity creature;
    creature.setWorld(&world);
    auto target = std::make_unique<_TestCreatureEntity>();
    target->setWorld(&world);
    creature.setAttackTarget(target.get());

    _TestBowItem bow;
    creature.setMainHandItem(ItemStack(&bow, 1));
    _ExposedRangedAttackGoal ranged(&creature, 1.0, 20, 40, 15.0f);
    _ExposedRangedBowAttackGoal bowGoal(&creature, 1.0, 20, 40);
    ASSERT_TRUE(ranged.shouldExecute());
    ASSERT_TRUE(bowGoal.shouldExecute());
    creature.setActivePath();

    // 模拟拥有者暂停 tick：目标移除、延迟析构队列清空后，才恢复目标评估与 tick。
    target->discard();
    target.reset();
    EXPECT_EQ(creature.attackTarget(), nullptr);
    EXPECT_FALSE(ranged.shouldContinueExecuting());
    EXPECT_FALSE(bowGoal.shouldContinueExecuting());
    ranged.tick();
    bowGoal.tick();
    EXPECT_EQ(ranged.cachedTarget(), nullptr);
    EXPECT_EQ(bowGoal.cachedTarget(), nullptr);
}

TEST(AiGoalRegressionTest, AvoidEntityGoal_ResumesAfterTargetDestruction)
{
    _TestCreatureEntity creature;
    auto target = std::make_unique<_TestCreatureEntity>();
    AvoidEntityGoal goal(&creature, 6.0f, 1.0, 1.3);
    mc::test::AvoidEntityGoalTestAccessor::setAvoidTarget(goal, target.get());
    creature.setActivePath();
    ASSERT_TRUE(goal.shouldContinueExecuting());
    target->discard();
    target.reset();

    // 跳过 shouldContinueExecuting，直接复现恢复后的 tick 解引用路径。
    goal.tick();
    EXPECT_EQ(mc::test::AvoidEntityGoalTestAccessor::getAvoidTarget(goal), nullptr);
    EXPECT_FALSE(goal.shouldContinueExecuting());
    goal.resetTask();
    EXPECT_TRUE(creature.navigator()->noPath());
}

TEST(AiGoalRegressionTest, MeleeAttackGoal_ResumesAfterTargetDestruction)
{
    _TestCreatureEntity creature;
    auto target = std::make_unique<_TestCreatureEntity>();
    creature.setAttackTarget(target.get());
    _ExposedMeleeAttackGoal goal(&creature, 1.0, true);
    goal.setCachedTarget(target.get());
    target->discard();
    target.reset();
    goal.tick();
    EXPECT_EQ(goal.cachedTarget(), nullptr);
    EXPECT_EQ(creature.attackTarget(), nullptr);
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST(AiGoalRegressionTest, MeleeAttackGoal_ContinuesActivePathDuringStartupCooldown)
{
    _TestCreatureEntity creature;
    _TestCreatureEntity target;
    creature.setPosition(0.5f, 64.0f, 0.5f);
    target.setPosition(5.5f, 64.0f, 0.5f);
    creature.setAttackTarget(&target);
    creature.setActivePath();
    _ExposedMeleeAttackGoal goal(&creature, 1.0, false);
    goal.setCachedTarget(&target);
    EXPECT_TRUE(goal.shouldContinueExecuting());
    EXPECT_TRUE(goal.shouldContinueExecuting());
    creature.navigator()->clearPath();
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST(AiGoalRegressionTest, LastHurtBy_ExpiresAfterAttackerDestruction)
{
    _TestCreatureEntity creature;
    auto attacker = std::make_unique<_TestCreatureEntity>();
    creature.setLastHurtBy(attacker.get());
    ASSERT_EQ(creature.getLastHurtBy(), attacker.get());
    attacker.reset();
    EXPECT_EQ(creature.getLastHurtBy(), nullptr);
}

TEST(AiGoalRegressionTest, ScriptHandles_SharedEntityIdDoesNotInvalidateOtherObject)
{
    auto first = std::make_unique<_TestCreatureEntity>();
    auto second = std::make_unique<_TestCreatureEntity>();
    ASSERT_EQ(first->id(), second->id());

    using namespace mc::mod::bedrock::addon;
    ScriptObjectRegistry::ObjectData firstHandle{first.get(), false, "Entity", nullptr, first->id(), first.get()};
    ScriptObjectRegistry::ObjectData secondHandle{second.get(), false, "Entity", nullptr, second->id(), second.get()};
    ScriptObjectRegistry::ObjectData secondComponent{
        second.get(), false, "Health", nullptr, second->id(), second.get()};
    auto& handles = ScriptHandleRegistry::instance();
    handles.registerHandle(&firstHandle);
    handles.registerHandle(&secondHandle);
    handles.registerHandle(&secondComponent);

    // 不同维度和未入册的临时实体可以同号，销毁其中一个不能清空另一个的句柄。
    first.reset();
    EXPECT_EQ(firstHandle.ptr, nullptr);
    EXPECT_EQ(secondHandle.ptr, second.get());
    EXPECT_EQ(secondComponent.ptr, second.get());

    second.reset();
    EXPECT_EQ(secondHandle.ptr, nullptr);
    EXPECT_EQ(secondComponent.ptr, nullptr);
}

TEST(AiGoalRegressionTest, ScriptHandles_FinalizedHandleIsNotVisitedDuringDestruction)
{
    auto entity = std::make_unique<_TestCreatureEntity>();
    using namespace mc::mod::bedrock::addon;
    auto finalized = std::make_unique<ScriptObjectRegistry::ObjectData>(
        ScriptObjectRegistry::ObjectData{entity.get(), false, "Entity", nullptr, entity->id(), entity.get()});
    ScriptObjectRegistry::ObjectData surviving{entity.get(), false, "Health", nullptr, entity->id(), entity.get()};
    auto& handles = ScriptHandleRegistry::instance();
    handles.registerHandle(finalized.get());
    handles.registerHandle(&surviving);
    // 单对象失效会先清空 opaque 指针，随后 GC 仍必须注销这个记录。
    finalized->ptr = nullptr;
    handles.unregisterHandle(finalized.get());
    finalized.reset();

    entity.reset();
    EXPECT_EQ(surviving.ptr, nullptr);
    handles.unregisterHandle(&surviving);
}

TEST(AiGoalRegressionTest, EndermanFindPlayerGoal_DoesNotPromoteInvalidPlayer)
{
    EndermanEntity enderman(EntityInstanceId(10), mc::test::testEcsRegistry());
    EndermanFindPlayerGoal goal(&enderman);

    for (bool discard : {false, true}) {
        auto player = std::make_unique<Player>(20, "Target", mc::test::testEcsRegistry());
        player->setHealth(player->maxHealth());
        mc::test::EndermanFindPlayerGoalTestAccessor::preparePromotion(goal, player.get());
        if (discard) {
            player->discard();
            player.reset();
        } else {
            // 死亡但尚未逻辑移除时，生命周期标记仍有效，必须单独检查 isAlive。
            player->setHealth(0.0f);
        }
        goal.tick();
        EXPECT_EQ(enderman.attackTarget(), nullptr);
        EXPECT_EQ(mc::test::EndermanFindPlayerGoalTestAccessor::pendingPlayer(goal), nullptr);
        EXPECT_FALSE(goal.shouldContinueExecuting());
    }
}
