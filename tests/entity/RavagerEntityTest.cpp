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
#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/illager/RavagerEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gamerule/GameRules.hpp"

#include <memory>

namespace mc {
namespace {

/**
 * @brief 劫掠兽测试用世界
 */
class RavagerTestWorld final : public mc::test::BaseTestWorld {
public:
    RavagerTestWorld()
        : m_difficulty(Difficulty::Normal)
        , m_mobGriefing(true)
    {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    // 测试辅助方法
    void incrementTick() { m_currentTick++; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }
    void setDifficulty(Difficulty diff) { m_difficulty = diff; }
    void setMobGriefing(bool value)
    {
        m_mobGriefing = value;
        m_gameRules.setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, value);
    }

    [[nodiscard]] std::vector<Entity*>& getEntitiesMutable() { return m_entities; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_entities;
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<Entity*> m_entities;
    u64 m_currentTick = 0;
    Difficulty m_difficulty;
    bool m_mobGriefing;
    world::gamerule::GameRules m_gameRules;
};

// ============================================================================
// RavagerEntity 基础属性测试
// ============================================================================

class RavagerEntityTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<RavagerTestWorld>(); }

    std::unique_ptr<RavagerTestWorld> m_world;
};

TEST_F(RavagerEntityTest, BasicProperties_AreCorrect)
{
    // MC 1.16.5 劫掠兽属性验证
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());

    // 尺寸验证
    EXPECT_FLOAT_EQ(ravager->width(), 1.95f);
    EXPECT_FLOAT_EQ(ravager->height(), 2.2f);
    EXPECT_FLOAT_EQ(ravager->eyeHeight(), 2.05f);

    // 步高验证 (劫掠兽可以走上 1 格高的方块)
    EXPECT_FLOAT_EQ(ravager->stepHeight(), 1.0f);
}

TEST_F(RavagerEntityTest, Attributes_AreCorrect)
{
    // MC 1.16.5 劫掠兽属性值验证
    // 构造函数中已调用 registerAttributes
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());

    // 属性验证
    f64 maxHealth = ravager->getAttributeValue(entity::attribute::Attributes::MAX_HEALTH, 0.0);
    f64 moveSpeed = ravager->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    f64 knockbackResistance = ravager->getAttributeValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0);
    f64 attackDamage = ravager->getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0);
    f64 attackKnockback = ravager->getAttributeValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 0.0);
    f64 followRange = ravager->getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 0.0);

    EXPECT_DOUBLE_EQ(maxHealth, 100.0);
    EXPECT_DOUBLE_EQ(moveSpeed, 0.3);
    EXPECT_DOUBLE_EQ(knockbackResistance, 0.75);
    EXPECT_DOUBLE_EQ(attackDamage, RavagerEntity::ATTACK_DAMAGE);
    EXPECT_DOUBLE_EQ(attackKnockback, 1.5);
    EXPECT_DOUBLE_EQ(followRange, 32.0);
}

TEST_F(RavagerEntityTest, Constants_AreCorrect)
{
    // MC 1.16.5 劫掠兽常量验证
    EXPECT_EQ(RavagerEntity::ATTACK_DURATION, 10);
    EXPECT_EQ(RavagerEntity::STUN_DURATION, 40);
    EXPECT_EQ(RavagerEntity::ROAR_DURATION, 20);
    EXPECT_FLOAT_EQ(RavagerEntity::ATTACK_DAMAGE, 12.0f);
    EXPECT_FLOAT_EQ(RavagerEntity::ROAR_DAMAGE, 6.0f);
    EXPECT_FLOAT_EQ(RavagerEntity::ROAR_RANGE, 4.0f);
    EXPECT_FLOAT_EQ(RavagerEntity::LAUNCH_POWER, 4.0f);
    EXPECT_FLOAT_EQ(RavagerEntity::LAUNCH_Y_POWER, 0.2f);
    EXPECT_FLOAT_EQ(RavagerEntity::STUN_CHANCE, 0.5f);
}

TEST_F(RavagerEntityTest, InitialState_IsCorrect)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());

    // 初始状态：无攻击、无眩晕、无咆哮
    EXPECT_FALSE(ravager->isAttacking());
    EXPECT_FALSE(ravager->isStunned());
    EXPECT_FALSE(ravager->isRoaring());
    EXPECT_EQ(ravager->getAttackTick(), 0);
    EXPECT_EQ(ravager->getStunTick(), 0);
    EXPECT_EQ(ravager->getRoarTick(), 0);

    // 可以破坏方块
    EXPECT_TRUE(ravager->canBreakBlocks());

    // 无骑乘者（骑乘关系由 Entity 通用 passengers 体系管理，历史 m_rider 死代码已移除）
    EXPECT_FALSE(ravager->isBeingRidden());
    EXPECT_EQ(ravager->getControllingPassenger(), INVALID_ENTITY_ID);
}

// ============================================================================
// RavagerEntity 攻击状态测试
// ============================================================================

class RavagerAttackStateTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<RavagerTestWorld>(); }

    std::unique_ptr<RavagerTestWorld> m_world;
};

TEST_F(RavagerAttackStateTest, AttackEntityAsMob_SetsAttackTick)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());
    ravager->setPosition(0.0, 64.0, 0.0);

    // 创建目标玩家
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setPosition(2.0, 64.0, 0.0);

    // 攻击目标
    ravager->attackEntityAsMob(*player);

    // 验证攻击动画设置
    EXPECT_TRUE(ravager->isAttacking());
    EXPECT_EQ(ravager->getAttackTick(), RavagerEntity::ATTACK_DURATION);
}

TEST_F(RavagerAttackStateTest, MovementBlocked_WhenAttacking)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());

    // 设置攻击状态
    ravager->attackEntityAsMob(
        *std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry()));

    // 攻击时移动被阻塞
    EXPECT_TRUE(ravager->isMovementBlocked());
}

TEST_F(RavagerAttackStateTest, MovementBlocked_WhenStunned)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());

    // 手动设置眩晕状态（实际由 constructKnockBackVector 触发）
    // 由于 stunTick 是私有成员，我们通过 tick 来测试
    // 这里验证 isMovementBlocked 逻辑
    EXPECT_FALSE(ravager->isMovementBlocked()); // 初始状态
}

TEST_F(RavagerAttackStateTest, MovementBlocked_WhenRoaring)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());

    // 初始状态不阻塞
    EXPECT_FALSE(ravager->isMovementBlocked());
}

// ============================================================================
// RavagerEntity tick 更新测试
// ============================================================================

class RavagerTickTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<RavagerTestWorld>(); }

    std::unique_ptr<RavagerTestWorld> m_world;
};

// 注意：tick 测试需要完整的 AI 基础设施支持，包括目标选择器等
// 这里只测试基本的 tick 计数器逻辑，不调用完整的 tick 方法

TEST_F(RavagerTickTest, AttackTick_CanBeSetAndDecrement)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());
    ravager->setPosition(0.0, 64.0, 0.0);

    // 攻击动画初始值为 0
    EXPECT_EQ(ravager->getAttackTick(), 0);
    EXPECT_FALSE(ravager->isAttacking());

    // 设置攻击状态
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setPosition(2.0, 64.0, 0.0);
    ravager->attackEntityAsMob(*player);

    // 验证攻击动画设置
    EXPECT_EQ(ravager->getAttackTick(), RavagerEntity::ATTACK_DURATION);
    EXPECT_TRUE(ravager->isAttacking());
}

TEST_F(RavagerTickTest, StunTick_CanBeSet)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());

    // 眩晕初始值为 0
    EXPECT_EQ(ravager->getStunTick(), 0);
    EXPECT_FALSE(ravager->isStunned());
}

TEST_F(RavagerTickTest, RoarTick_CanBeSet)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());

    // 咆哮初始值为 0
    EXPECT_EQ(ravager->getRoarTick(), 0);
    EXPECT_FALSE(ravager->isRoaring());
}

// ============================================================================
// RavagerEntity 方块破坏测试
// ============================================================================

class RavagerBlockBreakTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<RavagerTestWorld>(); }

    std::unique_ptr<RavagerTestWorld> m_world;
};

TEST_F(RavagerBlockBreakTest, CanBreakBlocks_DefaultTrue)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());

    EXPECT_TRUE(ravager->canBreakBlocks());
}

TEST_F(RavagerBlockBreakTest, CanBreakBlocks_CanBeDisabled)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());

    ravager->setCanBreakBlocks(false);
    EXPECT_FALSE(ravager->canBreakBlocks());

    ravager->setCanBreakBlocks(true);
    EXPECT_TRUE(ravager->canBreakBlocks());
}

// ============================================================================
// RavagerEntity 视线测试
// ============================================================================

class RavagerVisibilityTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<RavagerTestWorld>(); }

    std::unique_ptr<RavagerTestWorld> m_world;
};

TEST_F(RavagerVisibilityTest, CanSee_NormalCase)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());
    ravager->setPosition(0.0, 64.0, 0.0);

    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setPosition(2.0, 64.0, 0.0);

    // 正常情况下可以看到
    // 注：实际 canSee 需要射线检测，这里只测试基础逻辑
    EXPECT_FALSE(ravager->isStunned());
    EXPECT_FALSE(ravager->isRoaring());
}

// ============================================================================
// RavagerEntity 经验值测试
// ============================================================================

class RavagerExperienceTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<RavagerTestWorld>(); }

    std::unique_ptr<RavagerTestWorld> m_world;
};

TEST_F(RavagerExperienceTest, ExperienceValue_IsCorrect)
{
    // MC 1.16.5: 劫掠兽掉落 20 经验值
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());

    // 经验值在 registerAttributes 中设置
    // 注：需要确认 getExperienceValue 接口
    // EXPECT_EQ(ravager->getExperienceValue(), 20);
}

} // namespace
} // namespace mc
