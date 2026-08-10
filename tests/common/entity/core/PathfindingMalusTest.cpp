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
 * @file PathfindingMalusTest.cpp
 * @brief MobEntity::setPathfindingMalus / getPathfindingMalus 单元测试
 *
 * 覆盖以下场景：
 * - 默认值回退：未设置时返回 PathNodeType 默认代价（getPathCostPenalty）
 * - setPathfindingMalus 后 getPathfindingMalus 返回设置值
 * - 各类 PathNodeType（Water/DangerFire/DamageFire/DangerOther 等）的设置与查询
 * - shouldPassengersInheritMalus 默认返回 false
 * - CopperGolemEntity 构造函数对 DANGER_FIRE/DANGER_OTHER/DAMAGE_FIRE 的初始化
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/pathfinding/PathNodeType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <unordered_map>

using namespace mc;
using namespace mc::entity;
using namespace mc::entity::ai::pathfinding;

namespace {

// 测试用 MobEntity 子类，仅用于直接测试 MobEntity 的 malus 接口
class TestMobEntity : public MobEntity {
public:
    TestMobEntity()
        : MobEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
    {
        registerAttributes();
    }

    explicit TestMobEntity(EntityInstanceId id, ecs::EntityRegistry& registry = mc::test::testEcsRegistry())
        : MobEntity(id, registry)
    {
        registerAttributes();
    }

    // 暴露 protected setVehicle 供测试设置骑乘关系
    void setVehicleForTest(EntityInstanceId vehicle) { setVehicle(vehicle); }
};

} // namespace

// ============================================================================
// 默认值回退测试
// ============================================================================

TEST(PathfindingMalusTest, DefaultReturnsPathCostPenalty_Water)
{
    TestMobEntity mob;

    // Water 默认代价为 8.0
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::Water), 8.0f);
}

TEST(PathfindingMalusTest, DefaultReturnsPathCostPenalty_DangerFire)
{
    TestMobEntity mob;

    // DangerFire 默认代价为 8.0
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::DangerFire), 8.0f);
}

TEST(PathfindingMalusTest, DefaultReturnsPathCostPenalty_DamageFire)
{
    TestMobEntity mob;

    // DamageFire 默认代价为 16.0
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::DamageFire), 16.0f);
}

TEST(PathfindingMalusTest, DefaultReturnsPathCostPenalty_Walkable)
{
    TestMobEntity mob;

    // Walkable 默认代价为 0.0
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::Walkable), 0.0f);
}

TEST(PathfindingMalusTest, DefaultReturnsPathCostPenalty_Lava)
{
    TestMobEntity mob;

    // Lava 默认代价为 -1.0（不可通行）
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::Lava), -1.0f);
}

// ============================================================================
// 设置后查询测试
// ============================================================================

TEST(PathfindingMalusTest, SetMalus_OverridesDefault)
{
    TestMobEntity mob;

    mob.setPathfindingMalus(PathNodeType::Water, 0.0f);
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::Water), 0.0f);

    // 其他类型仍返回默认值
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::Lava), -1.0f);
}

TEST(PathfindingMalusTest, SetMalus_NegativeValue_BlocksPath)
{
    TestMobEntity mob;

    // 设置 DamageFire 为 -1.0 表示不可通行
    mob.setPathfindingMalus(PathNodeType::DamageFire, -1.0f);
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::DamageFire), -1.0f);
}

TEST(PathfindingMalusTest, SetMalus_PositiveValue_HighCost)
{
    TestMobEntity mob;

    // 设置 DangerFire 为 16.0 表示高代价但可通行
    mob.setPathfindingMalus(PathNodeType::DangerFire, 16.0f);
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::DangerFire), 16.0f);
}

TEST(PathfindingMalusTest, SetMalus_MultipleTypes_Independent)
{
    TestMobEntity mob;

    mob.setPathfindingMalus(PathNodeType::DangerFire, 16.0f);
    mob.setPathfindingMalus(PathNodeType::DangerOther, 16.0f);
    mob.setPathfindingMalus(PathNodeType::DamageFire, -1.0f);

    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::DangerFire), 16.0f);
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::DangerOther), 16.0f);
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::DamageFire), -1.0f);

    // 未设置的类型仍返回默认值
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::Water), 8.0f);
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::Walkable), 0.0f);
}

TEST(PathfindingMalusTest, SetMalus_OverwritePreviousValue)
{
    TestMobEntity mob;

    mob.setPathfindingMalus(PathNodeType::Water, 4.0f);
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::Water), 4.0f);

    mob.setPathfindingMalus(PathNodeType::Water, 0.0f);
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::Water), 0.0f);

    mob.setPathfindingMalus(PathNodeType::Water, -1.0f);
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::Water), -1.0f);
}

// ============================================================================
// shouldPassengersInheritMalus 默认值测试
// ============================================================================

TEST(PathfindingMalusTest, ShouldPassengersInheritMalus_DefaultFalse)
{
    TestMobEntity mob;
    EXPECT_FALSE(mob.shouldPassengersInheritMalus());
}

TEST(PathfindingMalusTest, ShouldPassengersInheritMalus_PigEntityDefaultFalse)
{
    // 通过具体实体类型验证默认行为
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(pig->shouldPassengersInheritMalus());
}

// ============================================================================
// CopperGolemEntity 构造函数初始化测试
// 对应 MC 1.21.11 CopperGolem.java:91-93
// ============================================================================

class CopperGolemMalusTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            VanillaBlocks::initialize();
            VanillaEntities::registerAll();
            s_initialized = true;
        }
    }
};

TEST_F(CopperGolemMalusTest, DangerFire_InitializedTo16)
{
    auto golem = std::make_unique<CopperGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(golem->getPathfindingMalus(PathNodeType::DangerFire), 16.0f);
}

TEST_F(CopperGolemMalusTest, DangerOther_InitializedTo16)
{
    auto golem = std::make_unique<CopperGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(golem->getPathfindingMalus(PathNodeType::DangerOther), 16.0f);
}

TEST_F(CopperGolemMalusTest, DamageFire_InitializedToNegative1)
{
    auto golem = std::make_unique<CopperGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(golem->getPathfindingMalus(PathNodeType::DamageFire), -1.0f);
}

TEST_F(CopperGolemMalusTest, OtherTypes_UseDefaultValues)
{
    auto golem = std::make_unique<CopperGolemEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 未显式设置的类型应保持默认代价
    EXPECT_FLOAT_EQ(golem->getPathfindingMalus(PathNodeType::Water), 8.0f);
    EXPECT_FLOAT_EQ(golem->getPathfindingMalus(PathNodeType::Walkable), 0.0f);
    EXPECT_FLOAT_EQ(golem->getPathfindingMalus(PathNodeType::Lava), -1.0f);
}

// ============================================================================
// 乘客继承 malus 测试
// ============================================================================

class InheritMalusTestWorld final : public mc::test::BaseTestWorld {
public:
    InheritMalusTestWorld() = default;

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }

    // 覆写 getEntity 以支持 getVehicle() → Entity* 解引用
    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        const auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second : nullptr;
    }
    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        const auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second : nullptr;
    }

    // 注册实体到世界中，便于后续通过 getEntity 查询
    void registerEntity(Entity* entity)
    {
        MC_ASSERT_RELEASE(entity != nullptr);
        m_entities[entity->id()] = entity;
    }

private:
    std::unordered_map<EntityInstanceId, Entity*> m_entities;
};

// 测试用 MobEntity 子类，重写 shouldPassengersInheritMalus 返回 true
class InheritMalusMob : public MobEntity {
public:
    InheritMalusMob()
        : MobEntity(EntityInstanceId(2), mc::test::testEcsRegistry())
    {
        registerAttributes();
        // 模拟炽足兽（Strider）：乘客继承 malus
        // 设置 LAVA 为 0.0（可在岩浆上寻路）
        setPathfindingMalus(PathNodeType::Lava, 0.0f);
    }

    [[nodiscard]] bool shouldPassengersInheritMalus() const noexcept override { return true; }
};

TEST(PathfindingMalusInheritTest, PassengerInheritsVehicleMalus_WhenInheritFlagTrue)
{
    // 此测试验证：当乘客骑乘在 shouldPassengersInheritMalus=true 的载具上时，
    // 乘客的 getPathfindingMalus 应返回载具的 malus 值。
    //
    // 由于 getPathfindingMalus 内部需要通过 world()->getEntity(getVehicle()) 解引用载具，
    // 我们使用 InheritMalusTestWorld 来提供实体查询能力。
    InheritMalusTestWorld world;

    TestMobEntity passenger(EntityInstanceId(1));
    passenger.setWorld(&world);

    InheritMalusMob vehicle;
    vehicle.setWorld(&world);

    world.registerEntity(&vehicle);

    // 模拟乘客骑上载具
    passenger.setVehicleForTest(vehicle.id());

    // 乘客未设置 LAVA malus，默认应为 -1.0；但载具设置了 0.0 且 shouldPassengersInheritMalus=true
    // 因此乘客查询 LAVA 时应返回载具的 0.0
    EXPECT_FLOAT_EQ(passenger.getPathfindingMalus(PathNodeType::Lava), 0.0f);
}

TEST(PathfindingMalusInheritTest, PassengerUsesOwnMalus_WhenInheritFlagFalse)
{
    // 当载具 shouldPassengersInheritMalus=false（默认）时，乘客使用自己的 malus
    InheritMalusTestWorld world;

    TestMobEntity passenger(EntityInstanceId(1));
    passenger.setWorld(&world);

    TestMobEntity vehicle(EntityInstanceId(2));
    vehicle.setWorld(&world);
    vehicle.setPathfindingMalus(PathNodeType::Lava, 0.0f); // 载具允许岩浆

    world.registerEntity(&vehicle);
    passenger.setVehicleForTest(vehicle.id());

    // 乘客未设置 LAVA，载具 shouldPassengersInheritMalus=false
    // 因此乘客使用自己的默认值 -1.0
    EXPECT_FLOAT_EQ(passenger.getPathfindingMalus(PathNodeType::Lava), -1.0f);
}

TEST(PathfindingMalusInheritTest, NoVehicle_UsesOwnMalus)
{
    // 没有骑乘时，使用自己的 malus
    TestMobEntity mob;
    mob.setPathfindingMalus(PathNodeType::Water, 4.0f);
    EXPECT_FLOAT_EQ(mob.getPathfindingMalus(PathNodeType::Water), 4.0f);
}
