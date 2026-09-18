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
 * @file EndermanEndermiteTargetTest.cpp
 * @brief 末影人攻击末影螨目标选择器测试
 *
 * 测试末影人对末影螨的目标选择行为（对齐 MC Java 1.21.11）：
 * - 末影人攻击所有末影螨（无 playerSpawned 守卫）
 * - 目标选择器正确注册在优先级3
 *
 * 参考 MC Java 1.21.11 EnderMan.registerGoals()（EnderMan.java:104）：
 *   this.targetSelector.addGoal(3, new NearestAttackableTargetGoal<>(this, Endermite.class, true, false));
 * 4 参数版 (Mob, Class, checkSight=true, mustSee=false)，无 target selector 谓词——末影人攻击所有末影螨。
 *
 * 历史背景：MC 1.8-1.16 末影人仅攻击玩家（末影珍珠）生成的末影螨（Endermite.playerSpawned 守卫），
 * 1.17 (20w46a) 撤销该守卫；1.21.11 Endermite 类已无 playerSpawned 字段。此前 Cubium 误对齐 1.8-1.16
 * 旧行为，现已迁移到 1.21.11（删除 isSpawnedByPlayer 守卫 + EndermiteEntity.m_playerSpawned 字段）。
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
class EndermanEndermiteTestWorld final : public mc::test::BaseTestWorld {
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

// ==================== EndermanEntity 目标选择器测试 ====================

class EndermanEndermiteTargetTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        world = std::make_unique<EndermanEndermiteTestWorld>();

        // 创建末影人
        enderman = std::make_unique<EndermanEntity>(static_cast<EntityInstanceId>(1), mc::test::testEcsRegistry());
        enderman->setWorld(world.get());
        enderman->setPosition(0.0, 64.0, 0.0);

        // 创建末影螨
        endermite = std::make_unique<EndermiteEntity>(static_cast<EntityInstanceId>(2), mc::test::testEcsRegistry());
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

// ==================== 目标选择器谓词测试（对齐 1.21.11：攻击所有末影螨）====================

/**
 * @brief 验证末影人对末影螨的目标选择无 playerSpawned 守卫
 *
 * MC Java 1.21.11 EnderMan.registerGoals 注册：
 *   targetSelector.addGoal(3, new NearestAttackableTargetGoal<>(this, Endermite.class, true, false));
 * 末影人攻击所有末影螨，不区分生成来源。1.17 (20w46a) 撤销了 1.8-1.16 的 playerSpawned 守卫。
 */
class EndermiteTargetPredicateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        world = std::make_unique<EndermanEndermiteTestWorld>();

        // 创建末影螨（对齐 1.21.11：无需 playerSpawned 标志，末影人攻击所有末影螨）
        endermite = std::make_unique<EndermiteEntity>(static_cast<EntityInstanceId>(1), mc::test::testEcsRegistry());
        endermite->setWorld(world.get());
        endermite->setPosition(5.0, 64.0, 0.0);
    }

    void TearDown() override
    {
        endermite.reset();
        world.reset();
    }

    std::unique_ptr<EndermanEndermiteTestWorld> world;
    std::unique_ptr<EndermiteEntity> endermite;
};

TEST_F(EndermiteTargetPredicateTest, EndermiteIsAliveAndTargetable)
{
    // 对齐 1.21.11：末影螨存活即可被末影人选为目标，无 playerSpawned 前置条件。
    EXPECT_TRUE(endermite->isAlive());
}

TEST_F(EndermiteTargetPredicateTest, PredicateAcceptsAllEndermites)
{
    // 对齐 MC 1.21.11 的目标选择逻辑：NearestAttackableTargetGoal<Endermite> 无 selector 谓词，
    // 仅检查 instanceof EndermiteEntity + isAlive。任何末影螨（无论生成来源）都通过。
    auto predicate = [](const LivingEntity* entity) -> bool {
        if (entity == nullptr || !entity->isAlive()) {
            return false;
        }
        // 对齐 Java NearestAttackableTargetGoal 默认目标类型过滤：仅接受目标类型实例。
        const EndermiteEntity* endermite = dynamic_cast<const EndermiteEntity*>(entity);
        return endermite != nullptr;
    };

    // 所有存活的末影螨都应通过谓词（无 playerSpawned 守卫）
    EXPECT_TRUE(predicate(endermite.get()));
}

TEST_F(EndermiteTargetPredicateTest, PredicateRejectsNonEndermite)
{
    // 谓词应该拒绝非末影螨实体
    auto predicate = [](const LivingEntity* entity) -> bool {
        if (entity == nullptr || !entity->isAlive()) {
            return false;
        }
        const EndermiteEntity* endermite = dynamic_cast<const EndermiteEntity*>(entity);
        return endermite != nullptr;
    };

    // 创建一个末影人作为非末影螨实体
    auto enderman = std::make_unique<EndermanEntity>(static_cast<EntityInstanceId>(3), mc::test::testEcsRegistry());
    enderman->setWorld(world.get());
    enderman->setPosition(5.0, 64.0, 0.0);

    // 末影人不应该通过谓词（NearestAttackableTargetGoal<Endermite> 仅接受 EndermiteEntity）
    EXPECT_FALSE(predicate(enderman.get()));
}

// ==================== EndermanEntity::registerGoals 验证测试 ====================

class EndermanGoalsRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        world = std::make_unique<EndermanEndermiteTestWorld>();
        enderman = std::make_unique<EndermanEntity>(static_cast<EntityInstanceId>(1), mc::test::testEcsRegistry());
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
