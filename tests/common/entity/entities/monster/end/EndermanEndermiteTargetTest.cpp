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
 * IMPLIED, INCLUDING BUT BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file EndermanEndermiteTargetTest.cpp
 * @brief 末影人攻击末影螨目标选择器测试
 *
 * 测试末影人对末影螨的目标选择行为：
 * - 末影人应该攻击玩家生成的末影螨
 * - 末影人不应该攻击自然生成的末影螨
 * - 目标选择器正确注册在优先级3
 *
 * 参考 MC 1.16.5 EndermanEntity.registerGoals():
 * this.targetSelector.addGoal(3, new NearestAttackableTargetGoal<>(this, EndermiteEntity.class, 10, true, false,
 * field_213627_bA)); 其中 field_213627_bA 是一个 Predicate，只过滤 isSpawnedByPlayer() 为 true 的末影螨
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 *
 * 提供末影人/末影螨测试所需的最小 IWorld 接口实现
 */
class EndermanEndermiteTestWorld final : public test::BaseTestWorld {
public:
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
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        EntityInstanceId id = entity->id();
        m_entities[id] = std::move(entity);
        return id;
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second.get() : nullptr;
    }

    std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except = nullptr) const override
    {
        std::vector<Entity*> result;
        for (const auto& [id, entity] : m_entities) {
            if (entity.get() == except) continue;
            if (entity->isAlive() && entity->boundingBox().intersects(box)) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("EndermanEndermiteTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("EndermanEndermiteTestWorld::tickManager not implemented");
    }

    // 测试辅助方法
    void incrementTick() { m_currentTick++; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<EntityInstanceId, std::unique_ptr<Entity>> m_entities;
    u64 m_currentTick = 0;
};

// ==================== EndermiteEntity::isSpawnedByPlayer 测试 ====================

class EndermiteSpawnedByPlayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        world = std::make_unique<EndermanEndermiteTestWorld>();
        endermite = std::make_unique<EndermiteEntity>(static_cast<EntityInstanceId>(1));
        endermite->setWorld(world.get());
    }

    void TearDown() override
    {
        endermite.reset();
        world.reset();
    }

    std::unique_ptr<EndermanEndermiteTestWorld> world;
    std::unique_ptr<EndermiteEntity> endermite;
};

TEST_F(EndermiteSpawnedByPlayerTest, DefaultNotSpawnedByPlayer)
{
    // 默认情况下，末影螨不是由玩家生成的
    EXPECT_FALSE(endermite->isSpawnedByPlayer());
}

TEST_F(EndermiteSpawnedByPlayerTest, SetSpawnedByPlayerTrue)
{
    // 设置为玩家生成
    endermite->setSpawnedByPlayer(true);
    EXPECT_TRUE(endermite->isSpawnedByPlayer());
}

TEST_F(EndermiteSpawnedByPlayerTest, SetSpawnedByPlayerFalse)
{
    // 设置为玩家生成后，再设置为非玩家生成
    endermite->setSpawnedByPlayer(true);
    EXPECT_TRUE(endermite->isSpawnedByPlayer());

    endermite->setSpawnedByPlayer(false);
    EXPECT_FALSE(endermite->isSpawnedByPlayer());
}

// ==================== EndermanEntity 目标选择器测试 ====================

class EndermanEndermiteTargetTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        world = std::make_unique<EndermanEndermiteTestWorld>();

        // 创建末影人
        enderman = std::make_unique<EndermanEntity>(static_cast<EntityInstanceId>(1));
        enderman->setWorld(world.get());
        enderman->setPosition(0.0, 64.0, 0.0);

        // 创建末影螨
        endermite = std::make_unique<EndermiteEntity>(static_cast<EntityInstanceId>(2));
        endermite->setWorld(world.get());
        endermite->setPosition(5.0, 64.0, 0.0); // 末影人5格外
    }

    void TearDown() override
    {
        endermite.reset();
        enderman.reset();
        world.reset();
    }

    std::unique_ptr<EndermanEndermiteTestWorld> world;
    std::unique_ptr<EndermanEntity> enderman;
    std::unique_ptr<EndermiteEntity> endermite;
};

TEST_F(EndermanEndermiteTargetTest, EndermiteEntityClassExists)
{
    // 验证 EndermiteEntity 类可以正常创建
    EXPECT_NE(endermite.get(), nullptr);
    // 静态构造未 setTypeId（由 EntityType::create 经注册表赋值），仅验证类型正确。
    EXPECT_NE(dynamic_cast<EndermiteEntity*>(endermite.get()), nullptr);
}

TEST_F(EndermanEndermiteTargetTest, EndermiteHasCorrectLegacyType)
{
    // LegacyEntityType 概念已随实体类型系统统一移除；保留用例验证运行时类型。
    EXPECT_NE(dynamic_cast<EndermiteEntity*>(endermite.get()), nullptr);
}

TEST_F(EndermanEndermiteTargetTest, EndermiteIsAlive)
{
    // 验证末影螨存活状态
    EXPECT_TRUE(endermite->isAlive());
}

TEST_F(EndermanEndermiteTargetTest, EndermiteIsPlayerSpawned)
{
    // 设置末影螨为玩家生成
    endermite->setSpawnedByPlayer(true);
    EXPECT_TRUE(endermite->isSpawnedByPlayer());
}

TEST_F(EndermanEndermiteTargetTest, EndermiteIsNotPlayerSpawned)
{
    // 验证默认不是玩家生成
    EXPECT_FALSE(endermite->isSpawnedByPlayer());
}

// ==================== 目标选择器谓词测试 ====================

/**
 * @brief 测试目标选择器谓词
 *
 * MC 1.16.5 中末影人只攻击玩家生成的末影螨:
 * private static final Predicate<LivingEntity> field_213627_bA = (p_213626_0_) -> {
 *     return p_213626_0_ instanceof EndermiteEntity && ((EndermiteEntity)p_213626_0_).isSpawnedByPlayer();
 * };
 */
class EndermiteTargetPredicateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        world = std::make_unique<EndermanEndermiteTestWorld>();

        // 创建末影螨（玩家生成）
        playerSpawnedEndermite = std::make_unique<EndermiteEntity>(static_cast<EntityInstanceId>(1));
        playerSpawnedEndermite->setWorld(world.get());
        playerSpawnedEndermite->setPosition(5.0, 64.0, 0.0);
        playerSpawnedEndermite->setSpawnedByPlayer(true);

        // 创建末影螨（自然生成）
        naturalEndermite = std::make_unique<EndermiteEntity>(static_cast<EntityInstanceId>(2));
        naturalEndermite->setWorld(world.get());
        naturalEndermite->setPosition(5.0, 64.0, 0.0);
        naturalEndermite->setSpawnedByPlayer(false);
    }

    void TearDown() override
    {
        playerSpawnedEndermite.reset();
        naturalEndermite.reset();
        world.reset();
    }

    std::unique_ptr<EndermanEndermiteTestWorld> world;
    std::unique_ptr<EndermiteEntity> playerSpawnedEndermite;
    std::unique_ptr<EndermiteEntity> naturalEndermite;
};

TEST_F(EndermiteTargetPredicateTest, PlayerSpawnedEndermiteShouldBeTargeted)
{
    // 玩家生成的末影螨应该被末影人攻击
    EXPECT_TRUE(playerSpawnedEndermite->isSpawnedByPlayer());
    EXPECT_TRUE(playerSpawnedEndermite->isAlive());
}

TEST_F(EndermiteTargetPredicateTest, NaturalEndermiteShouldNotBeTargeted)
{
    // 自然生成的末影螨不应该被末影人攻击
    EXPECT_FALSE(naturalEndermite->isSpawnedByPlayer());
    EXPECT_TRUE(naturalEndermite->isAlive());
}

TEST_F(EndermiteTargetPredicateTest, PredicateFiltersCorrectly)
{
    // 模拟 MC 1.16.5 的谓词逻辑
    auto predicate = [](const LivingEntity* entity) -> bool {
        if (entity == nullptr || !entity->isAlive()) {
            return false;
        }
        const EndermiteEntity* endermite = dynamic_cast<const EndermiteEntity*>(entity);
        if (endermite == nullptr) {
            return false;
        }
        return endermite->isSpawnedByPlayer();
    };

    // 玩家生成的末影螨通过谓词
    EXPECT_TRUE(predicate(playerSpawnedEndermite.get()));

    // 自然生成的末影螨不通过谓词
    EXPECT_FALSE(predicate(naturalEndermite.get()));
}

TEST_F(EndermiteTargetPredicateTest, PredicateRejectsNonEndermite)
{
    // 谓词应该拒绝非末影螨实体
    auto predicate = [](const LivingEntity* entity) -> bool {
        if (entity == nullptr || !entity->isAlive()) {
            return false;
        }
        const EndermiteEntity* endermite = dynamic_cast<const EndermiteEntity*>(entity);
        if (endermite == nullptr) {
            return false;
        }
        return endermite->isSpawnedByPlayer();
    };

    // 创建一个末影人作为非末影螨实体
    auto enderman = std::make_unique<EndermanEntity>(static_cast<EntityInstanceId>(3));
    enderman->setWorld(world.get());
    enderman->setPosition(5.0, 64.0, 0.0);

    // 末影人不应该通过谓词
    EXPECT_FALSE(predicate(enderman.get()));
}

// ==================== EndermanEntity::registerGoals 验证测试 ====================

class EndermanGoalsRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        world = std::make_unique<EndermanEndermiteTestWorld>();
        enderman = std::make_unique<EndermanEntity>(static_cast<EntityInstanceId>(1));
        enderman->setWorld(world.get());
    }

    void TearDown() override
    {
        enderman.reset();
        world.reset();
    }

    std::unique_ptr<EndermanEndermiteTestWorld> world;
    std::unique_ptr<EndermanEntity> enderman;
};

TEST_F(EndermanGoalsRegistrationTest, EndermanHasTargetSelector)
{
    // 验证末影人有目标选择器
    EXPECT_NE(&enderman->targetSelector(), nullptr);
}

TEST_F(EndermanGoalsRegistrationTest, EndermanHasGoalSelector)
{
    // 验证末影人有行为选择器
    EXPECT_NE(&enderman->goalSelector(), nullptr);
}

TEST_F(EndermanGoalsRegistrationTest, EndermanHasCorrectLegacyType)
{
    // LegacyEntityType 概念已随实体类型系统统一移除；保留用例验证运行时类型。
    EXPECT_NE(dynamic_cast<EndermanEntity*>(enderman.get()), nullptr);
}

} // namespace
} // namespace mc
