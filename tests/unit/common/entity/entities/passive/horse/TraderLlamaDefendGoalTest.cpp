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

// ============================================================================
// TraderLlamaDefendWanderingTraderGoal 单元测试
//
// 覆盖：
// - 未拴绳 / 拴在栅栏 / 拴在非流浪商人实体 → 不触发；
// - 拴在流浪商人身上但商人从未受击 → 不触发；
// - 拴在流浪商人身上且商人受击（时间戳更新）→ 触发，startExecuting 设置攻击目标；
// - 时间戳未变化（同一次受击重复评估）→ 不重复触发；
// - 攻击者不合法（已死亡 / 被羊驼免疫的实体类型）→ 不触发；
// - 互斥 flag 仅含 TARGET（继承 TargetGoal），不占用 Move/Look/Jump。
// ============================================================================

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/passive/horse/TraderLlamaEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

/**
 * @brief 支持 UUID 反查与实体 tick 推进的最小测试世界
 *
 * TraderLlamaEntity::getLeashHolderEntity 经 IWorld::getEntityByUuid 解析拴绳持有者；
 * 基础测试世界默认返回 nullptr，故此处按实体 UUID 登记，语义与生产 EntityManager 一致。
 * 地面铺 AIR 且不做实体碰撞，使实体 tick 不产生额外移动。
 */
class TraderLlamaDefendTestWorld final : public test::BaseTestWorld {
public:
    void registerEntityByUuid(Entity* entity)
    {
        if (entity != nullptr) {
            m_uuidEntities[entity->uuid()] = entity;
        }
    }

    [[nodiscard]] Entity* getEntityByUuid(const std::string& uuid) override
    {
        const auto it = m_uuidEntities.find(uuid);
        return it != m_uuidEntities.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntityByUuid(const std::string& uuid) const override
    {
        const auto it = m_uuidEntities.find(uuid);
        return it != m_uuidEntities.end() ? it->second : nullptr;
    }

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override
    {
        return &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override
    {
        return true;
    }

private:
    std::unordered_map<std::string, Entity*> m_uuidEntities;
};

/**
 * @brief 推进实体 tick 若干次
 *
 * 实体的 ticksExisted 只在 Entity::tick 内自增，直接调用 hurt 不会推进
 * setLastHurtBy 记录的时间戳。时间戳"更新"语义因此须经真实 tick 推进后再受伤。
 */
void tickEntities(std::initializer_list<Entity*> entities, i32 times)
{
    for (i32 i = 0; i < times; ++i) {
        for (Entity* entity : entities) {
            entity->tick();
        }
    }
}

/**
 * @brief 商队羊驼保卫流浪商人目标的测试夹具
 */
class TraderLlamaDefendGoalTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            entity::VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), test::testEcsRegistry());
        m_trader = std::make_unique<entity::WanderingTraderEntity>(EntityInstanceId(2), test::testEcsRegistry());
        m_attacker = std::make_unique<ZombieEntity>(EntityInstanceId(3), test::testEcsRegistry());

        // 直接构造绕过 EntityType::create 工厂，m_typeId 为空会导致 entityType() 返回 nullptr，
        // canAttackType 判定失效（isSuitableTarget 静默拒绝）。显式补齐类型 id。
        m_llama->setTypeId(entity::EntityTypeKeys::TRADER_LLAMA);
        m_trader->setTypeId(entity::EntityTypeKeys::WANDERING_TRADER);
        m_attacker->setTypeId(entity::EntityTypeKeys::ZOMBIE);

        m_world.registerEntityByUuid(m_llama.get());
        m_world.registerEntityByUuid(m_trader.get());
        m_world.registerEntityByUuid(m_attacker.get());

        m_llama->setWorld(&m_world);
        m_trader->setWorld(&m_world);
        m_attacker->setWorld(&m_world);
    }

    void TearDown() override
    {
        m_attacker.reset();
        m_trader.reset();
        m_llama.reset();
    }

    /// 把羊驼拴到流浪商人身上（对齐 Leashable.setLeashedTo 的 UUID 绑定语义）。
    void leashLlamaToTrader()
    {
        m_llama->setLeashedToEntity(m_trader->uuid());
    }

    /// 让流浪商人"受到"某实体攻击：推进时间后记录攻击者及其时间戳。
    void hurtTraderBy(LivingEntity* attacker)
    {
        tickEntities({m_trader.get()}, 1);
        m_trader->setLastHurtBy(attacker);
    }

    TraderLlamaDefendTestWorld m_world;
    std::unique_ptr<TraderLlamaEntity> m_llama;
    std::unique_ptr<entity::WanderingTraderEntity> m_trader;
    std::unique_ptr<ZombieEntity> m_attacker;
};

// ============================================================================
// 触发条件
// ============================================================================

TEST_F(TraderLlamaDefendGoalTest, ShouldExecute_NotLeashed_ReturnsFalse)
{
    entity::ai::goal::TraderLlamaDefendWanderingTraderGoal goal(m_llama.get());
    hurtTraderBy(m_attacker.get());

    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TraderLlamaDefendGoalTest, ShouldExecute_LeashedToNonTrader_ReturnsFalse)
{
    entity::ai::goal::TraderLlamaDefendWanderingTraderGoal goal(m_llama.get());
    // 拴在普通生物（僵尸）身上：持有者不是流浪商人，即便该持有者受过攻击也不触发。
    m_llama->setLeashedToEntity(m_attacker->uuid());
    hurtTraderBy(m_attacker.get());

    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TraderLlamaDefendGoalTest, ShouldExecute_LeashedToFence_ReturnsFalse)
{
    entity::ai::goal::TraderLlamaDefendWanderingTraderGoal goal(m_llama.get());
    m_llama->setLeashedToFence(BlockPos(0, 0, 0));
    hurtTraderBy(m_attacker.get());

    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TraderLlamaDefendGoalTest, ShouldExecute_TraderNeverHurt_ReturnsFalse)
{
    entity::ai::goal::TraderLlamaDefendWanderingTraderGoal goal(m_llama.get());
    leashLlamaToTrader();

    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TraderLlamaDefendGoalTest, ShouldExecute_TraderHurt_ReturnsTrue)
{
    entity::ai::goal::TraderLlamaDefendWanderingTraderGoal goal(m_llama.get());
    leashLlamaToTrader();
    hurtTraderBy(m_attacker.get());

    EXPECT_TRUE(goal.shouldExecute());
}

// ============================================================================
// startExecuting
// ============================================================================

TEST_F(TraderLlamaDefendGoalTest, StartExecuting_SetsAttackTargetToAttacker)
{
    entity::ai::goal::TraderLlamaDefendWanderingTraderGoal goal(m_llama.get());
    leashLlamaToTrader();
    hurtTraderBy(m_attacker.get());

    ASSERT_TRUE(goal.shouldExecute());
    goal.startExecuting();

    EXPECT_EQ(m_llama->attackTarget(), static_cast<LivingEntity*>(m_attacker.get()));
}

TEST_F(TraderLlamaDefendGoalTest, ShouldExecute_SameTimestamp_DoesNotRetrigger)
{
    entity::ai::goal::TraderLlamaDefendWanderingTraderGoal goal(m_llama.get());
    leashLlamaToTrader();
    hurtTraderBy(m_attacker.get());

    ASSERT_TRUE(goal.shouldExecute());
    goal.startExecuting();

    // 同一次受击（商人时间戳未再推进）不应重复触发。
    EXPECT_FALSE(goal.shouldExecute());

    // 商人再次受击 → 时间戳更新 → 重新触发。
    hurtTraderBy(m_attacker.get());
    EXPECT_TRUE(goal.shouldExecute());
}

TEST_F(TraderLlamaDefendGoalTest, ShouldExecute_TargetIsOwnLlama_ReturnsFalse)
{
    entity::ai::goal::TraderLlamaDefendWanderingTraderGoal goal(m_llama.get());
    leashLlamaToTrader();
    // 商人被拴绳的羊驼本身"攻击"：不能把自身设为目标。
    hurtTraderBy(m_llama.get());

    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TraderLlamaDefendGoalTest, ShouldExecute_AttackerDead_ReturnsFalse)
{
    entity::ai::goal::TraderLlamaDefendWanderingTraderGoal goal(m_llama.get());
    leashLlamaToTrader();
    hurtTraderBy(m_attacker.get());
    m_attacker->setHealth(0.0f);
    ASSERT_TRUE(m_attacker->isDead());

    EXPECT_FALSE(goal.shouldExecute());
}

// ============================================================================
// Goal 元信息
// ============================================================================

TEST_F(TraderLlamaDefendGoalTest, MutexFlags_AreTargetOnly)
{
    entity::ai::goal::TraderLlamaDefendWanderingTraderGoal goal(m_llama.get());

    const auto& flags = goal.getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Target));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
}

TEST_F(TraderLlamaDefendGoalTest, GetTypeName_IsStable)
{
    entity::ai::goal::TraderLlamaDefendWanderingTraderGoal goal(m_llama.get());
    EXPECT_EQ(goal.getTypeName(), "TraderLlamaDefendWanderingTraderGoal");
}

} // namespace
} // namespace mc
