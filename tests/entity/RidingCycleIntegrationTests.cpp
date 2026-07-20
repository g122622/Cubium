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
 * @file RidingCycleIntegrationTests.cpp
 * @brief 带有 World 环境的骑乘循环检测集成测试
 *
 * 补充 RidingCycleDetectionTests.cpp 中因缺少 World 环境而无法测试的场景：
 * - 直接循环检测：A骑B后B骑A应被拒绝
 * - 间接循环检测：A骑B, B骑C, C骑A应被拒绝
 * - stopRiding 在有 World 时正确清理骑乘关系
 * - baseTick 中载具被移除时自动下骑
 * - getLowestRidingEntity 在有 World 时正确遍历载具链
 * - isRidingOrBeingRiddenBy 正确检测间接骑乘关系
 *
 * 这些测试需要 World 环境因为 Entity::startRiding 的循环检测、
 * Entity::dismount 的载具查找、Entity::removePassengers 的乘客遍历
 * 都依赖 m_world->getEntity() 来解析 EntityInstanceId 到 Entity*。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::test;

// ============================================================================
// 测试用 World：支持 getEntity / registerEntity
// ============================================================================

class RidingTestWorld final : public BaseTestWorld {
public:
    RidingTestWorld() = default;

    void registerEntity(Entity* entity)
    {
        if (entity != nullptr) {
            m_entities[entity->id()] = entity;
        }
    }

    void unregisterEntity(EntityInstanceId id) { m_entities.erase(id); }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second : nullptr;
    }

private:
    std::unordered_map<EntityInstanceId, Entity*> m_entities;
};

// ============================================================================
// 辅助：可容纳多乘客的测试实体（带 World 支持）
// ============================================================================

class MultiPassengerEntity : public Entity {
public:
    MultiPassengerEntity(EntityInstanceId id, IWorld* world = nullptr)
        : Entity(id, world)
    {}
    i32 getMaxPassengers() const override { return 2; }
};

// ============================================================================
// 测试夹具
// ============================================================================

class RidingCycleIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<RidingTestWorld>(); }
    void TearDown() override { m_world.reset(); }

    std::unique_ptr<RidingTestWorld> m_world;
};

// ============================================================================
// 1. 直接循环检测：A骑B后B骑A应被拒绝（需要 World）
//    对齐 MC Java Entity.startRiding 中的循环检测 for 循环
// ============================================================================

TEST_F(RidingCycleIntegrationTest, DirectCycle_RejectedWithWorld)
{
    // 创建两个实体并注册到世界
    Entity a(EntityInstanceId(1), m_world.get());
    Entity b(EntityInstanceId(2), m_world.get());
    a.setWorld(m_world.get());
    b.setWorld(m_world.get());
    m_world->registerEntity(&a);
    m_world->registerEntity(&b);

    // A 骑乘 B，应该成功
    EXPECT_TRUE(a.startRiding(b));
    EXPECT_EQ(a.getVehicle(), b.id());
    EXPECT_TRUE(b.isPassenger(a.id()));

    // B 尝试骑乘 A —— 循环检测应拒绝
    // a.startRiding(b) 之后：a.m_vehicle = b.id()
    // b.startRiding(a) 中，循环检测遍历：current = a, a.getVehicle() = b.id()
    // b.id() == b.m_id（b自身），循环检测到！
    EXPECT_FALSE(b.startRiding(a));

    // B 的 vehicle 应该未被设置
    EXPECT_EQ(b.getVehicle(), INVALID_ENTITY_ID);

    // A 的骑乘关系不受影响
    EXPECT_TRUE(a.isRiding());
    EXPECT_EQ(a.getVehicle(), b.id());
    EXPECT_TRUE(b.isPassenger(a.id()));
}

TEST_F(RidingCycleIntegrationTest, DirectCycle_ThreeEntities)
{
    // 三个实体的直接循环尝试
    Entity a(EntityInstanceId(1), m_world.get());
    Entity b(EntityInstanceId(2), m_world.get());
    Entity c(EntityInstanceId(3), m_world.get());
    for (auto* e : {&a, &b, &c}) {
        e->setWorld(m_world.get());
        m_world->registerEntity(e);
    }

    // A 骑乘 B
    EXPECT_TRUE(a.startRiding(b));
    EXPECT_EQ(a.getVehicle(), b.id());

    // B 骑乘 C
    // B 的 rideCooldown 为 0（rideCooldown 仅设给 passenger，即 a）
    EXPECT_TRUE(b.startRiding(c));
    EXPECT_EQ(b.getVehicle(), c.id());

    // 现在链是 A -> B -> C（A骑B，B骑C）
    // C 尝试骑乘 A：循环检测遍历 vehicle 链
    // current = A, A.getVehicle() = B, B != C -> 继续
    // current = B, B.getVehicle() = C, C == C(m_id) -> 循环检测到！
    EXPECT_FALSE(c.startRiding(a));

    // C 的 vehicle 应该未被设置
    EXPECT_EQ(c.getVehicle(), INVALID_ENTITY_ID);

    // 原有骑乘关系不受影响
    EXPECT_TRUE(a.isRiding());
    EXPECT_EQ(a.getVehicle(), b.id());
    EXPECT_TRUE(b.isRiding());
    EXPECT_EQ(b.getVehicle(), c.id());
}

// ============================================================================
// 2. 间接循环检测：更长的链
//    A骑B, B骑C, C骑D, D骑A 应被拒绝
// ============================================================================

TEST_F(RidingCycleIntegrationTest, IndirectCycle_FourEntities)
{
    Entity a(EntityInstanceId(1), m_world.get());
    Entity b(EntityInstanceId(2), m_world.get());
    Entity c(EntityInstanceId(3), m_world.get());
    Entity d(EntityInstanceId(4), m_world.get());
    for (auto* e : {&a, &b, &c, &d}) {
        e->setWorld(m_world.get());
        m_world->registerEntity(e);
    }

    // 建立链：A -> B -> C -> D（A骑B，B骑C，C骑D）
    EXPECT_TRUE(a.startRiding(b));
    EXPECT_TRUE(b.startRiding(c));
    EXPECT_TRUE(c.startRiding(d));

    // D 尝试骑乘 A：循环检测应该拒绝
    // 循环：current = A, A.getVehicle() = B
    //       B != D，继续
    //       current = B, B.getVehicle() = C
    //       C != D，继续
    //       current = C, C.getVehicle() = D
    //       D == D(m_id)! 循环检测到！
    EXPECT_FALSE(d.startRiding(a));
    EXPECT_EQ(d.getVehicle(), INVALID_ENTITY_ID);
}

// ============================================================================
// 3. 非循环的链应该允许
//    A骑B, B骑C 是合法的（不存在循环）
// ============================================================================

TEST_F(RidingCycleIntegrationTest, NonCycleChain_AllowedWithWorld)
{
    Entity a(EntityInstanceId(1), m_world.get());
    Entity b(EntityInstanceId(2), m_world.get());
    Entity c(EntityInstanceId(3), m_world.get());
    for (auto* e : {&a, &b, &c}) {
        e->setWorld(m_world.get());
        m_world->registerEntity(e);
    }

    // A 骑乘 B
    EXPECT_TRUE(a.startRiding(b));
    EXPECT_EQ(a.getVehicle(), b.id());

    // B 骑乘 C（合法，B 不是 A 的载具链的一部分）
    EXPECT_TRUE(b.startRiding(c));
    EXPECT_EQ(b.getVehicle(), c.id());

    // 验证链完整性
    EXPECT_TRUE(a.isRiding());
    EXPECT_TRUE(b.isRiding());
    EXPECT_FALSE(c.isRiding());

    EXPECT_TRUE(b.isPassenger(a.id()));
    EXPECT_TRUE(c.isPassenger(b.id()));
    EXPECT_FALSE(c.isPassenger(a.id()));
}

// ============================================================================
// 4. stopRiding 在有 World 时正确清理骑乘关系
//    对齐 MC Java Entity.removeVehicle() 的行为
// ============================================================================

TEST_F(RidingCycleIntegrationTest, StopRiding_WithWorld_CleansUpBothSides)
{
    Entity vehicle(EntityInstanceId(1), m_world.get());
    Entity rider(EntityInstanceId(2), m_world.get());
    vehicle.setWorld(m_world.get());
    rider.setWorld(m_world.get());
    m_world->registerEntity(&vehicle);
    m_world->registerEntity(&rider);

    // rider 骑乘 vehicle
    EXPECT_TRUE(rider.startRiding(vehicle));
    EXPECT_TRUE(rider.isRiding());
    EXPECT_EQ(rider.getVehicle(), vehicle.id());
    EXPECT_TRUE(vehicle.hasPassengers());
    EXPECT_TRUE(vehicle.isPassenger(rider.id()));

    // stopRiding 应该清理两侧
    rider.stopRiding();

    // rider 的 vehicle 引用被清空
    EXPECT_FALSE(rider.isRiding());
    EXPECT_EQ(rider.getVehicle(), INVALID_ENTITY_ID);

    // vehicle 的乘客列表中 rider 被移除（因为有 World，dismount 可以找到 vehicle）
    EXPECT_FALSE(vehicle.hasPassengers());
    EXPECT_FALSE(vehicle.isPassenger(rider.id()));
}

TEST_F(RidingCycleIntegrationTest, StopRiding_MiddleOfChain_CleansUpCorrectly)
{
    // 链：A -> B -> C（A骑B，B骑C）
    // B 下骑后，A 也应该被弹出（因为 B 不再是 C 的乘客，
    // 但 A 仍然骑乘 B，只是 B 不再骑乘 C）
    Entity a(EntityInstanceId(1), m_world.get());
    Entity b(EntityInstanceId(2), m_world.get());
    Entity c(EntityInstanceId(3), m_world.get());
    for (auto* e : {&a, &b, &c}) {
        e->setWorld(m_world.get());
        m_world->registerEntity(e);
    }

    EXPECT_TRUE(a.startRiding(b));
    EXPECT_TRUE(b.startRiding(c));

    // B 从 C 上下骑
    b.stopRiding();

    // B 不再骑乘 C
    EXPECT_FALSE(b.isRiding());
    EXPECT_EQ(b.getVehicle(), INVALID_ENTITY_ID);
    EXPECT_FALSE(c.isPassenger(b.id()));

    // A 仍然骑乘 B
    EXPECT_TRUE(a.isRiding());
    EXPECT_EQ(a.getVehicle(), b.id());
    EXPECT_TRUE(b.isPassenger(a.id()));
}

// ============================================================================
// 5. removePassengers 在有 World 时正确移除所有乘客
// ============================================================================

TEST_F(RidingCycleIntegrationTest, RemovePassengers_WithWorld)
{
    MultiPassengerEntity vehicle(EntityInstanceId(1), m_world.get());
    Entity rider1(EntityInstanceId(2), m_world.get());
    Entity rider2(EntityInstanceId(3), m_world.get());
    vehicle.setWorld(m_world.get());
    rider1.setWorld(m_world.get());
    rider2.setWorld(m_world.get());
    m_world->registerEntity(&vehicle);
    m_world->registerEntity(&rider1);
    m_world->registerEntity(&rider2);

    // 两个乘客骑乘
    EXPECT_TRUE(rider1.startRiding(vehicle));
    EXPECT_TRUE(rider2.startRiding(vehicle));
    EXPECT_EQ(vehicle.getPassengers().size(), 2u);

    // removePassengers 应该移除所有乘客
    vehicle.removePassengers();

    // 乘客列表被清空
    EXPECT_FALSE(vehicle.hasPassengers());
    EXPECT_EQ(vehicle.getPassengers().size(), 0u);

    // 乘客的 vehicle 引用被清空（因为有 World，stopRiding 可以找到 vehicle）
    EXPECT_FALSE(rider1.isRiding());
    EXPECT_EQ(rider1.getVehicle(), INVALID_ENTITY_ID);
    EXPECT_FALSE(rider2.isRiding());
    EXPECT_EQ(rider2.getVehicle(), INVALID_ENTITY_ID);
}

// ============================================================================
// 6. getLowestRidingEntity 在有 World 时正确遍历载具链
//    对齐 MC Java Entity.getRootVehicle()
// ============================================================================

TEST_F(RidingCycleIntegrationTest, GetLowestRidingEntity_ChainTraversal)
{
    Entity a(EntityInstanceId(1), m_world.get());
    Entity b(EntityInstanceId(2), m_world.get());
    Entity c(EntityInstanceId(3), m_world.get());
    for (auto* e : {&a, &b, &c}) {
        e->setWorld(m_world.get());
        m_world->registerEntity(e);
    }

    // 没有骑乘时，getLowestRidingEntity 返回自身
    EXPECT_EQ(a.getLowestRidingEntity(), &a);
    EXPECT_EQ(b.getLowestRidingEntity(), &b);

    // A 骑乘 B
    EXPECT_TRUE(a.startRiding(b));

    // A 的 lowest riding entity 是 B（B不骑乘任何实体）
    EXPECT_EQ(a.getLowestRidingEntity(), &b);
    EXPECT_EQ(b.getLowestRidingEntity(), &b);

    // B 骑乘 C
    EXPECT_TRUE(b.startRiding(c));

    // A 的 lowest riding entity 是 C（链的根）
    EXPECT_EQ(a.getLowestRidingEntity(), &c);
    // B 的 lowest riding entity 是 C
    EXPECT_EQ(b.getLowestRidingEntity(), &c);
    // C 的 lowest riding entity 是 C 自身
    EXPECT_EQ(c.getLowestRidingEntity(), &c);
}

TEST_F(RidingCycleIntegrationTest, GetLowestRidingEntity_NoRiding)
{
    Entity entity(EntityInstanceId(1), m_world.get());
    entity.setWorld(m_world.get());
    m_world->registerEntity(&entity);

    // 不在骑乘时，返回自身
    EXPECT_EQ(entity.getLowestRidingEntity(), &entity);
}

// ============================================================================
// 7. isRidingOrBeingRiddenBy 正确检测间接骑乘关系
// ============================================================================

TEST_F(RidingCycleIntegrationTest, IsRidingOrBeingRiddenBy_DirectRelation)
{
    Entity vehicle(EntityInstanceId(1), m_world.get());
    Entity rider(EntityInstanceId(2), m_world.get());
    vehicle.setWorld(m_world.get());
    rider.setWorld(m_world.get());
    m_world->registerEntity(&vehicle);
    m_world->registerEntity(&rider);

    // 骑乘前没有关系
    EXPECT_FALSE(rider.isRidingOrBeingRiddenBy(vehicle));
    EXPECT_FALSE(vehicle.isRidingOrBeingRiddenBy(rider));

    // rider 骑乘 vehicle
    EXPECT_TRUE(rider.startRiding(vehicle));

    // rider 骑乘 vehicle（rider 和 vehicle 有直接关系）
    EXPECT_TRUE(rider.isRidingOrBeingRiddenBy(vehicle));
    EXPECT_TRUE(vehicle.isRidingOrBeingRiddenBy(rider));
}

TEST_F(RidingCycleIntegrationTest, IsRidingOrBeingRiddenBy_IndirectRelation)
{
    // A -> B -> C（A骑B，B骑C）
    Entity a(EntityInstanceId(1), m_world.get());
    Entity b(EntityInstanceId(2), m_world.get());
    Entity c(EntityInstanceId(3), m_world.get());
    for (auto* e : {&a, &b, &c}) {
        e->setWorld(m_world.get());
        m_world->registerEntity(e);
    }

    EXPECT_TRUE(a.startRiding(b));
    EXPECT_TRUE(b.startRiding(c));

    // A 和 C 有间接骑乘关系
    EXPECT_TRUE(a.isRidingOrBeingRiddenBy(c));
    EXPECT_TRUE(c.isRidingOrBeingRiddenBy(a));

    // A 和 B 有直接骑乘关系
    EXPECT_TRUE(a.isRidingOrBeingRiddenBy(b));
    EXPECT_TRUE(b.isRidingOrBeingRiddenBy(a));

    // B 和 C 有直接骑乘关系
    EXPECT_TRUE(b.isRidingOrBeingRiddenBy(c));
    EXPECT_TRUE(c.isRidingOrBeingRiddenBy(b));
}

TEST_F(RidingCycleIntegrationTest, IsRidingOrBeingRiddenBy_NoRelation)
{
    Entity a(EntityInstanceId(1), m_world.get());
    Entity b(EntityInstanceId(2), m_world.get());
    Entity c(EntityInstanceId(3), m_world.get());
    for (auto* e : {&a, &b, &c}) {
        e->setWorld(m_world.get());
        m_world->registerEntity(e);
    }

    // A 骑乘 B，但 C 是独立的
    EXPECT_TRUE(a.startRiding(b));

    EXPECT_FALSE(a.isRidingOrBeingRiddenBy(c));
    EXPECT_FALSE(c.isRidingOrBeingRiddenBy(a));
    EXPECT_FALSE(b.isRidingOrBeingRiddenBy(c));
    EXPECT_FALSE(c.isRidingOrBeingRiddenBy(b));
}

// ============================================================================
// 8. isRidingSameEntity 正确判断是否骑乘同一根载具
//    对齐 MC Java Entity.isPassengerOfSameVehicle()
// ============================================================================

TEST_F(RidingCycleIntegrationTest, IsRidingSameEntity_SharedRoot)
{
    // A -> C, B -> C（A和B都骑乘C）
    MultiPassengerEntity c(EntityInstanceId(3), m_world.get());
    Entity a(EntityInstanceId(1), m_world.get());
    Entity b(EntityInstanceId(2), m_world.get());
    a.setWorld(m_world.get());
    b.setWorld(m_world.get());
    c.setWorld(m_world.get());
    m_world->registerEntity(&a);
    m_world->registerEntity(&b);
    m_world->registerEntity(&c);

    EXPECT_TRUE(a.startRiding(c));
    EXPECT_TRUE(b.startRiding(c));

    // A 和 B 的根载具都是 C
    EXPECT_TRUE(a.isRidingSameEntity(b));
    EXPECT_TRUE(b.isRidingSameEntity(a));
}

TEST_F(RidingCycleIntegrationTest, IsRidingSameEntity_DifferentRoot)
{
    // A -> B, C -> D（不同根载具）
    Entity a(EntityInstanceId(1), m_world.get());
    Entity b(EntityInstanceId(2), m_world.get());
    Entity c(EntityInstanceId(3), m_world.get());
    Entity d(EntityInstanceId(4), m_world.get());
    for (auto* e : {&a, &b, &c, &d}) {
        e->setWorld(m_world.get());
        m_world->registerEntity(e);
    }

    EXPECT_TRUE(a.startRiding(b));
    EXPECT_TRUE(c.startRiding(d));

    // A 和 C 的根载具不同
    EXPECT_FALSE(a.isRidingSameEntity(c));
    EXPECT_FALSE(c.isRidingSameEntity(a));
}

// ============================================================================
// 9. 完整的骑乘生命周期：mount -> verify -> dismount -> verify
//    对齐 MC Java 的完整行为
// ============================================================================

TEST_F(RidingCycleIntegrationTest, FullRidingLifecycle)
{
    Entity vehicle(EntityInstanceId(1), m_world.get());
    Entity rider(EntityInstanceId(2), m_world.get());
    vehicle.setWorld(m_world.get());
    rider.setWorld(m_world.get());
    m_world->registerEntity(&vehicle);
    m_world->registerEntity(&rider);

    // 1. 初始状态
    EXPECT_FALSE(rider.isRiding());
    EXPECT_FALSE(vehicle.isBeingRidden());
    EXPECT_FALSE(vehicle.hasPassengers());
    EXPECT_EQ(rider.getVehicle(), INVALID_ENTITY_ID);

    // 2. 骑乘
    EXPECT_TRUE(rider.startRiding(vehicle));
    EXPECT_TRUE(rider.isRiding());
    EXPECT_EQ(rider.getVehicle(), vehicle.id());
    EXPECT_TRUE(vehicle.isBeingRidden());
    EXPECT_TRUE(vehicle.hasPassengers());
    EXPECT_TRUE(vehicle.isPassenger(rider.id()));
    EXPECT_EQ(vehicle.getPassengers().size(), 1u);
    EXPECT_EQ(vehicle.getPassengers()[0], rider.id());

    // 3. 下骑
    rider.stopRiding();
    EXPECT_FALSE(rider.isRiding());
    EXPECT_EQ(rider.getVehicle(), INVALID_ENTITY_ID);
    EXPECT_FALSE(vehicle.isBeingRidden());
    EXPECT_FALSE(vehicle.hasPassengers());
    EXPECT_FALSE(vehicle.isPassenger(rider.id()));

    // 4. 骑乘冷却（rideCooldown 被设置为 60 tick）
    EXPECT_GT(rider.rideCooldown(), 0);
    EXPECT_FALSE(rider.canRide());

    // 5. 冷却期间不能再次骑乘
    EXPECT_FALSE(rider.startRiding(vehicle));
    EXPECT_FALSE(rider.isRiding());
}

// ============================================================================
// 10. 循环检测不影响非循环场景
//     验证循环检测不会误判正常的多层骑乘
// ============================================================================

TEST_F(RidingCycleIntegrationTest, CycleDetectionDoesNotBlockValidChains)
{
    // A -> B -> C -> D（四层链，无循环）
    Entity a(EntityInstanceId(1), m_world.get());
    Entity b(EntityInstanceId(2), m_world.get());
    Entity c(EntityInstanceId(3), m_world.get());
    Entity d(EntityInstanceId(4), m_world.get());
    for (auto* e : {&a, &b, &c, &d}) {
        e->setWorld(m_world.get());
        m_world->registerEntity(e);
    }

    EXPECT_TRUE(a.startRiding(b));
    EXPECT_TRUE(b.startRiding(c));
    EXPECT_TRUE(c.startRiding(d));

    // 所有骑乘关系正确建立
    EXPECT_EQ(a.getVehicle(), b.id());
    EXPECT_EQ(b.getVehicle(), c.id());
    EXPECT_EQ(c.getVehicle(), d.id());
    EXPECT_EQ(d.getVehicle(), INVALID_ENTITY_ID);

    // D 不骑乘任何实体
    EXPECT_FALSE(d.isRiding());

    // A 的根载具是 D
    EXPECT_EQ(a.getLowestRidingEntity(), &d);
}

// ============================================================================
// 11. 自骑乘在有 World 环境下仍然被拒绝
// ============================================================================

TEST_F(RidingCycleIntegrationTest, SelfRiding_RejectedWithWorld)
{
    Entity entity(EntityInstanceId(1), m_world.get());
    entity.setWorld(m_world.get());
    m_world->registerEntity(&entity);

    EXPECT_FALSE(entity.startRiding(entity));
    EXPECT_FALSE(entity.isRiding());
    EXPECT_FALSE(entity.hasPassengers());
}

// ============================================================================
// 12. 已骑乘同一载具时 startRiding 返回 false（有 World 环境）
// ============================================================================

TEST_F(RidingCycleIntegrationTest, AlreadyRidingSameVehicle_ReturnsFalse)
{
    Entity vehicle(EntityInstanceId(1), m_world.get());
    Entity rider(EntityInstanceId(2), m_world.get());
    vehicle.setWorld(m_world.get());
    rider.setWorld(m_world.get());
    m_world->registerEntity(&vehicle);
    m_world->registerEntity(&rider);

    EXPECT_TRUE(rider.startRiding(vehicle));
    EXPECT_FALSE(rider.startRiding(vehicle)); // 重复骑乘同一载具

    // 骑乘关系不变
    EXPECT_TRUE(rider.isRiding());
    EXPECT_EQ(rider.getVehicle(), vehicle.id());
    EXPECT_EQ(vehicle.getPassengers().size(), 1u);
}

// ============================================================================
// 13. addPassenger 在有 World 环境下的前置条件验证
// ============================================================================

TEST_F(RidingCycleIntegrationTest, AddPassenger_PassengerAlreadyBoundToOtherVehicle_Fails)
{
    Entity vehicle1(EntityInstanceId(1), m_world.get());
    Entity vehicle2(EntityInstanceId(2), m_world.get());
    Entity rider(EntityInstanceId(3), m_world.get());
    for (auto* e : {&vehicle1, &vehicle2, &rider}) {
        e->setWorld(m_world.get());
        m_world->registerEntity(e);
    }

    // rider 骑乘 vehicle1
    EXPECT_TRUE(rider.startRiding(vehicle1));
    EXPECT_EQ(rider.getVehicle(), vehicle1.id());

    // 尝试将 rider 添加到 vehicle2 的乘客列表（应失败）
    EXPECT_FALSE(vehicle2.addPassenger(rider));

    // rider 仍然骑乘 vehicle1
    EXPECT_EQ(rider.getVehicle(), vehicle1.id());
}

// ============================================================================
// 14. startRiding 失败时 m_vehicle 被回滚（有 World 环境）
// ============================================================================

TEST_F(RidingCycleIntegrationTest, StartRidingFailure_RollbackVehicle_WithWorld)
{
    Entity vehicle(EntityInstanceId(1), m_world.get());
    Entity rider1(EntityInstanceId(2), m_world.get());
    Entity rider2(EntityInstanceId(3), m_world.get());
    for (auto* e : {&vehicle, &rider1, &rider2}) {
        e->setWorld(m_world.get());
        m_world->registerEntity(e);
    }

    // 第一个乘客骑乘成功
    EXPECT_TRUE(rider1.startRiding(vehicle));

    // 第二个乘客骑乘失败（因为 vehicle 只能容纳 1 个乘客）
    EXPECT_FALSE(rider2.startRiding(vehicle));

    // rider2 的 m_vehicle 应该被回滚为 INVALID_ENTITY_ID
    EXPECT_EQ(rider2.getVehicle(), INVALID_ENTITY_ID);
    EXPECT_FALSE(rider2.isRiding());

    // rider1 的骑乘关系不受影响
    EXPECT_TRUE(rider1.isRiding());
    EXPECT_EQ(rider1.getVehicle(), vehicle.id());
}

// ============================================================================
// 15. detach 在有 World 时正确解除所有骑乘关系
// ============================================================================

TEST_F(RidingCycleIntegrationTest, Detach_WithWorld_CleansUpAllRelationships)
{
    Entity vehicle(EntityInstanceId(1), m_world.get());
    Entity rider(EntityInstanceId(2), m_world.get());
    vehicle.setWorld(m_world.get());
    rider.setWorld(m_world.get());
    m_world->registerEntity(&vehicle);
    m_world->registerEntity(&rider);

    // rider 骑乘 vehicle
    EXPECT_TRUE(rider.startRiding(vehicle));

    // detach 应该清除所有关系
    rider.detach();

    // rider 不再骑乘
    EXPECT_FALSE(rider.isRiding());
    EXPECT_EQ(rider.getVehicle(), INVALID_ENTITY_ID);

    // vehicle 不再有 rider 作为乘客
    EXPECT_FALSE(vehicle.isPassenger(rider.id()));
    EXPECT_FALSE(vehicle.hasPassengers());
}
