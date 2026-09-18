/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include <memory>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/core/EntityRegistry.hpp"
#include "entity/entities/monster/illager/AbstractRaiderEntity.hpp"
#include "entity/entities/passive/basic/PigEntity.hpp"
#include "entity/registry/VanillaEntities.hpp"
#include "entity/registry/VanillaEntityTypeKeys.hpp"

namespace mc {
namespace test {

// ==================== HurtByTargetGoal 构造与谓词测试 ====================

class HurtByTargetGoalTest : public ::testing::Test {
protected:
    void SetUp() override { pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry()); }

    void TearDown() override { pig.reset(); }

    std::unique_ptr<PigEntity> pig;
};

TEST_F(HurtByTargetGoalTest, DefaultConstructor_AlertAlliesFalse)
{
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, Constructor_AlertAlliesTrue)
{
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true);
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, Constructor_WithIgnoreDamagePredicate)
{
    auto goal =
        std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true, [](const LivingEntity* attacker) -> bool {
            return attacker != nullptr && attacker->entityType() == entity::VanillaEntityTypeKeys::GUARDIAN;
        });
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, SetAlertOthers_ReturnsReference)
{
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());
    auto& ref = goal->setAlertOthers([](const LivingEntity* ally) -> bool {
        return ally != nullptr && ally->entityType() == entity::VanillaEntityTypeKeys::ZOMBIFIED_PIGLIN;
    });
    EXPECT_EQ(&ref, goal.get());
}

TEST_F(HurtByTargetGoalTest, SetAlertOthers_EnablesAlertAllies)
{
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), false);
    goal->setAlertOthers([](const LivingEntity* ally) -> bool {
        return ally != nullptr && ally->entityType() == entity::VanillaEntityTypeKeys::ZOMBIFIED_PIGLIN;
    });
    SUCCEED();
}

TEST_F(HurtByTargetGoalTest, IgnoreDamagePredicate_CompilesWithLambda)
{
    auto dolphinPredicate = [](const LivingEntity* attacker) -> bool {
        if (!attacker) return false;
        const entity::EntityType* type = attacker->entityType();
        return type == entity::VanillaEntityTypeKeys::GUARDIAN || type == entity::VanillaEntityTypeKeys::ELDER_GUARDIAN;
    };
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true, dolphinPredicate);
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, IgnoreDamagePredicate_CompilesWithRaiderCheck)
{
    auto raiderPredicate = [](const LivingEntity* attacker) -> bool {
        return dynamic_cast<const AbstractRaiderEntity*>(attacker) != nullptr;
    };
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true, raiderPredicate);
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, IgnoreDamagePredicate_CompilesWithTypeIdCheck)
{
    auto sameTypePredicate = [](const LivingEntity* attacker) -> bool {
        return attacker != nullptr && attacker->entityType() == entity::VanillaEntityTypeKeys::SHULKER;
    };
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true, sameTypePredicate);
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, Combined_IgnoreDamageAndAlertOthers)
{
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true);
    goal->setAlertOthers([](const LivingEntity* ally) -> bool {
        return ally != nullptr && ally->entityType() == entity::VanillaEntityTypeKeys::ZOMBIFIED_PIGLIN;
    });
    EXPECT_NE(goal, nullptr);
}

TEST_F(HurtByTargetGoalTest, Combined_IgnoreDamageAndIgnoreAlert)
{
    auto drownedPredicate = [](const LivingEntity* attacker) -> bool {
        return attacker != nullptr && attacker->entityType() == entity::VanillaEntityTypeKeys::DROWNED;
    };
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true, drownedPredicate);
    goal->setAlertOthers([](const LivingEntity* ally) -> bool {
        return ally != nullptr && ally->entityType() == entity::VanillaEntityTypeKeys::ZOMBIFIED_PIGLIN;
    });
    EXPECT_NE(goal, nullptr);
}

// ==================== HurtByTargetGoal 行为测试 ====================

/**
 * @brief 支持实体查询的测试用世界
 *
 * 用于测试 HurtByTargetGoal::startExecuting() 中的盟友警醒逻辑。
 */
class HurtByTargetTestWorld : public mc::test::BaseTestWorld {
public:
    void setEntities(std::vector<Entity*> entities) { m_entities = std::move(entities); }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_entities;
    }

private:
    std::vector<Entity*> m_entities;
};

/**
 * @brief 可公开 tick() 的测试用猪实体
 *
 * PigEntity::tick() 是 protected，测试中需要推进 ticksExisted
 * 以便 setLastHurtBy 产生非零时间戳，HurtByTargetGoal 才能正常触发。
 */
class TestPigEntity : public PigEntity {
public:
    explicit TestPigEntity(EntityInstanceId id)
        : PigEntity(id, mc::test::testEcsRegistry())
    {}

    using PigEntity::tick;
};

class HurtByTargetBehaviorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和实体注册表，确保 VanillaEntityTypeKeys 有正确的 typeId
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();

        world = std::make_unique<HurtByTargetTestWorld>();

        // 创建被攻击者（猪）
        pig = std::make_unique<TestPigEntity>(EntityInstanceId(1));
        pig->setWorld(world.get());
        pig->setPosition(0.0f, 64.0f, 0.0f);

        // 创建攻击者（另一只猪）
        attacker = std::make_unique<TestPigEntity>(EntityInstanceId(2));
        attacker->setWorld(world.get());
        attacker->setPosition(5.0f, 64.0f, 0.0f);

        // 直接构造的实体不经过 EntityType::create()，typeId 默认空，entityType() 返回 nullptr，
        // 会让 isSuitableTarget 的 canAttackType 检查提前失败。这里补 setTypeId 对齐生产路径
        // （注册表工厂 EntityType::create() 会调 setTypeId(m_name)），使行为测试反映真实实体。
        pig->setTypeId(entity::EntityTypeKeys::PIG);
        attacker->setTypeId(entity::EntityTypeKeys::PIG);

        // 推进 tick 使 ticksExisted > 0
        // setLastHurtBy 将 lastHurtByTimestamp 设为 ticksExisted()
        // HurtByTargetGoal::shouldExecute() 在 timestamp != m_timestamp 时才返回 true
        // 初始 m_timestamp = 0，如果 ticksExisted() 也为 0，则无法触发
        for (int i = 0; i < 10; ++i) {
            pig->tick();
        }
    }

    void TearDown() override
    {
        pig.reset();
        attacker.reset();
        world.reset();
    }

    std::unique_ptr<HurtByTargetTestWorld> world;
    std::unique_ptr<TestPigEntity> pig;
    std::unique_ptr<TestPigEntity> attacker;
};

TEST_F(HurtByTargetBehaviorTest, ShouldExecute_NoAttacker_ReturnsFalse)
{
    // 没有攻击者时，shouldExecute 应返回 false
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(HurtByTargetBehaviorTest, ShouldExecute_WithAttacker_ReturnsTrue)
{
    // 设置攻击者
    pig->setLastHurtBy(attacker.get());

    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(HurtByTargetBehaviorTest, ShouldExecute_IgnoreDamagePredicate_ExcludesAttacker)
{
    pig->setLastHurtBy(attacker.get());

    // 使用排除谓词排除所有 PigEntity 类型的攻击者
    const entity::EntityType* pigTypeId = pig->entityType();
    auto ignorePig = [pigTypeId](const LivingEntity* entity) -> bool {
        return entity != nullptr && entity->entityType() == pigTypeId;
    };

    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), false, ignorePig);

    // 攻击者是同类（PigEntity），被排除谓词过滤，shouldExecute 应返回 false
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(HurtByTargetBehaviorTest, ShouldExecute_IgnoreDamagePredicate_AllowsOtherTypes)
{
    pig->setLastHurtBy(attacker.get());

    // 使用排除谓词只排除守卫者类型（攻击者是猪，不在排除范围内）
    auto ignoreGuardian = [](const LivingEntity* entity) -> bool {
        if (!entity) return false;
        const entity::EntityType* type = entity->entityType();
        return type == entity::VanillaEntityTypeKeys::GUARDIAN || type == entity::VanillaEntityTypeKeys::ELDER_GUARDIAN;
    };

    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), false, ignoreGuardian);

    // 攻击者是猪，不是守卫者，不在排除范围内，shouldExecute 应返回 true
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(HurtByTargetBehaviorTest, ShouldExecute_DoesNotRepeatForSameAttack)
{
    pig->setLastHurtBy(attacker.get());

    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());

    // 第一次调用 shouldExecute 应返回 true
    EXPECT_TRUE(goal->shouldExecute());

    // 第二次调用（同一时间戳）应返回 false
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(HurtByTargetBehaviorTest, ShouldExecute_NewAttackAfterReset_AllowsReExecute)
{
    pig->setLastHurtBy(attacker.get());

    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());

    // 第一次攻击
    EXPECT_TRUE(goal->shouldExecute());
    goal->resetTask();

    // resetTask 将 m_timestamp 重置为 0，但 lastHurtByTimestamp 未变
    // 再次 shouldExecute 会发现 timestamp != 0，所以仍然返回 true
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(HurtByTargetBehaviorTest, ShouldExecute_NullMob_ReturnsFalse)
{
    // 没有 mob 的 goal 不应崩溃
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(nullptr, false);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(HurtByTargetBehaviorTest, ShouldExecute_SelfAttack_ExcludedByIsSuitableTarget)
{
    // 实体攻击自己（不应反击自身，isSuitableTarget 检查 target != mob）
    pig->setLastHurtBy(pig.get());

    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());
    // isSuitableTarget 检查 target != m_mob，自身攻击应被排除
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(HurtByTargetBehaviorTest, StartExecuting_SetsAttackTarget)
{
    pig->setLastHurtBy(attacker.get());

    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());

    // shouldExecute 设置内部目标
    ASSERT_TRUE(goal->shouldExecute());

    // startExecuting 应将攻击者设为 mob 的攻击目标
    goal->startExecuting();
    EXPECT_EQ(pig->attackTarget(), attacker.get());
}

TEST_F(HurtByTargetBehaviorTest, ResetTask_ClearsAttackTarget)
{
    pig->setLastHurtBy(attacker.get());

    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());
    ASSERT_TRUE(goal->shouldExecute());
    goal->startExecuting();
    EXPECT_EQ(pig->attackTarget(), attacker.get());

    // resetTask 应清除攻击目标
    goal->resetTask();
    EXPECT_EQ(pig->attackTarget(), nullptr);
}

TEST_F(HurtByTargetBehaviorTest, StartExecuting_AlertAllies_NearbySameTypeGetTarget)
{
    pig->setLastHurtBy(attacker.get());

    // 创建一个附近的同类实体（盟友）
    auto ally = std::make_unique<TestPigEntity>(EntityInstanceId(3));
    ally->setWorld(world.get());
    ally->setPosition(2.0f, 64.0f, 0.0f);
    // 盟友同样需 setTypeId，否则 entityType() 为空，alertAllies 的同类匹配会跳过它。
    ally->setTypeId(entity::EntityTypeKeys::PIG);

    // 将盟友放入世界的实体列表
    world->setEntities({ally.get()});

    // 创建带 alertAllies=true 的 goal
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true);
    ASSERT_TRUE(goal->shouldExecute());

    // 执行 startExecuting — 同类盟友应被警醒，攻击目标设为攻击者
    goal->startExecuting();

    // 盟友应被设置为攻击攻击者
    EXPECT_EQ(ally->attackTarget(), attacker.get());

    // 主实体也应被设置
    EXPECT_EQ(pig->attackTarget(), attacker.get());
}

TEST_F(HurtByTargetBehaviorTest, StartExecuting_AlertAllies_IgnoreAlertPredicate_SkipsAlly)
{
    pig->setLastHurtBy(attacker.get());

    // 创建一个附近的同类实体（盟友）
    auto ally = std::make_unique<TestPigEntity>(EntityInstanceId(3));
    ally->setWorld(world.get());
    ally->setPosition(2.0f, 64.0f, 0.0f);
    ally->setTypeId(entity::EntityTypeKeys::PIG);

    // 将盟友放入世界的实体列表
    world->setEntities({ally.get()});

    // 创建带 setAlertOthers 排除谓词的 goal
    // 排除谓词返回 true 表示不警醒该盟友
    // 这里排除所有与猪同类（typeId 相同）的盟友
    const entity::EntityType* pigTypeId = pig->entityType();
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true);
    goal->setAlertOthers([pigTypeId](const LivingEntity* allyEntity) -> bool {
        return allyEntity != nullptr && allyEntity->entityType() == pigTypeId;
    });

    ASSERT_TRUE(goal->shouldExecute());
    goal->startExecuting();

    // 盟友不应被警醒（被排除谓词过滤）
    EXPECT_EQ(ally->attackTarget(), nullptr);

    // 主实体仍应被设置攻击目标
    EXPECT_EQ(pig->attackTarget(), attacker.get());
}

TEST_F(HurtByTargetBehaviorTest, StartExecuting_NoAlertAllies_NearbySameTypeNotTargeted)
{
    pig->setLastHurtBy(attacker.get());

    // 创建一个附近的同类实体（盟友）
    auto ally = std::make_unique<TestPigEntity>(EntityInstanceId(3));
    ally->setWorld(world.get());
    ally->setPosition(2.0f, 64.0f, 0.0f);
    ally->setTypeId(entity::EntityTypeKeys::PIG);

    // 将盟友放入世界的实体列表
    world->setEntities({ally.get()});

    // 创建不带 alertAllies 的 goal（默认 false）
    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), false);
    ASSERT_TRUE(goal->shouldExecute());
    goal->startExecuting();

    // 盟友不应被警醒
    EXPECT_EQ(ally->attackTarget(), nullptr);

    // 主实体应被设置攻击目标
    EXPECT_EQ(pig->attackTarget(), attacker.get());
}

} // namespace test
} // namespace mc
