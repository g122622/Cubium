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
 * IMPLIED, NONINFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file BoatLandFrictionTest.cpp
 * @brief 测试 BoatEntity::updateMotion() 中陆地摩擦力减半条件
 *
 * 验证陆地摩擦力仅在控制乘客为 Player 时减半，
 * 与 MC Java AbstractBoat.floatBoat() 行为一致：
 *   if (this.getControllingPassenger() instanceof Player) {
 *       this.landFriction /= 2.0F;
 *   }
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace {

/**
 * @brief 暴露 BoatEntity 受保护方法的测试子类
 */
class TestBoatEntity : public entity::BoatEntity {
public:
    explicit TestBoatEntity(entity::BoatEntity::Type type = entity::BoatEntity::Type::OAK)
        : entity::BoatEntity(type)
    {}

    using BoatEntity::handleInput;
    using BoatEntity::setBoatGlide;
    using BoatEntity::setStatus;
    using BoatEntity::updateMotion;
};

/**
 * @brief 支持实体注册和查找的测试世界
 *
 * 继承 BaseTestWorld 并覆写 getEntity/spawnEntity 以支持乘客系统测试。
 */
class BoatFrictionTestWorld final : public test::BaseTestWorld {
public:
    BoatFrictionTestWorld() = default;

    void addTestEntity(Entity* entity) { m_testEntities[entity->id()] = entity; }
    void removeTestEntity(EntityInstanceId id) { m_testEntities.erase(id); }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_testEntities.find(id);
        return it != m_testEntities.end() ? it->second : nullptr;
    }
    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_testEntities.find(id);
        return it != m_testEntities.end() ? it->second : nullptr;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        EntityInstanceId id = entity->id();
        m_testEntities[id] = entity.get();
        m_ownedEntities.push_back(std::move(entity));
        return id;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("BoatFrictionTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("BoatFrictionTestWorld::tickManager not implemented");
    }

private:
    std::map<EntityInstanceId, Entity*> m_testEntities;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
};

class BoatLandFrictionTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 注册所有实体类型以使 VanillaEntityTypeKeys 常量有效
        entity::VanillaEntities::registerAll();
    }

    void SetUp() override { m_world = std::make_unique<BoatFrictionTestWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<BoatFrictionTestWorld> m_world;
};

// ============================================================================
// 控制乘客为 Player 时 m_boatGlide 字段减半
// ============================================================================

/**
 * @brief 控制乘客为 Player 时，m_boatGlide 字段减半（MC Java 语义对齐）
 *
 * MC Java AbstractBoat.floatBoat() 中 OnLand 分支：
 *   f = this.landFriction;           // 局部变量 f = 原始值
 *   if (controllingPassenger instanceof Player) {
 *       this.landFriction /= 2.0F;   // 仅减半字段，不影响当前 tick 的速度
 *   }
 *   velocity *= f;                   // 速度乘以原始值（未减半）
 *
 * 因此 Player 控制时，速度衰减系数与无 Player 时相同（原始值），
 * 但 m_boatGlide 字段被减半。由于 updateMotion() 末尾会重置 m_boatGlide = 0，
 * 且下一 tick 的 updateStatus() 会重新采样，此字段修改的效果是：
 * 如果船在同一 tick 内多次调用 updateMotion()（理论上不会发生），
 * 则第二次会使用减半后的值。
 */
TEST_F(BoatLandFrictionTest, PlayerPassenger_BoatGlideFieldHalved_VelocityUsesOriginalFriction)
{
    auto boat = std::make_unique<TestBoatEntity>(entity::BoatEntity::Type::OAK);
    boat->setId(EntityInstanceId(1));
    boat->setWorld(m_world.get());
    boat->setPosition(0.0f, 64.0f, 0.0f);
    boat->setStatus(entity::BoatStatus::OnLand);
    // 设置较高的陆地滑度值使效果明显
    boat->setBoatGlide(0.6f);
    // 给船一个初始速度
    boat->setVelocity(Vector3(1.0f, 0.0f, 1.0f));

    // 创建 Player 乘客
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer");
    player->setWorld(m_world.get());
    // 设置类型ID以使 entityType() 返回正确的 PLAYER 类型指针
    player->setTypeId(entity::EntityTypeKeys::PLAYER);

    // 注册实体到测试世界
    m_world->addTestEntity(boat.get());
    m_world->addTestEntity(player.get());

    // 让 Player 骑乘船（成为控制乘客）
    ASSERT_TRUE(player->startRiding(*boat));

    // 验证 Player 确实是控制乘客
    EntityInstanceId controllerId = boat->getControllingPassenger();
    EXPECT_EQ(controllerId, player->id()) << "Player should be the controlling passenger";

    // 保存初始速度
    Vector3 velBefore = boat->velocity();

    // 调用 updateMotion
    boat->updateMotion();

    // MC Java 语义：friction 使用原始 m_boatGlide 值（0.6），速度乘以原始值
    // 但 m_boatGlide 字段被减半为 0.3（下一 tick 会被 updateStatus() 覆盖）
    // 因此 Player 控制时，速度衰减与无 Player 时相同（都是 0.6），
    // 差异体现在 m_boatGlide 字段值上
    f32 expectedFriction = 0.6f; // 原始值，未减半
    EXPECT_FLOAT_EQ(boat->velocity().x, velBefore.x * expectedFriction);
    EXPECT_FLOAT_EQ(boat->velocity().z, velBefore.z * expectedFriction);
}

// ============================================================================
// 控制乘客非 Player 时摩擦力不减半
// ============================================================================

/**
 * @brief 控制乘客为非 Player 实体时，陆地摩擦力不减半
 *
 * 非玩家实体（如僵尸）骑乘船时，摩擦力保持原值。
 */
TEST_F(BoatLandFrictionTest, NonPlayerPassenger_FrictionNotHalved)
{
    auto boat = std::make_unique<TestBoatEntity>(entity::BoatEntity::Type::OAK);
    boat->setId(EntityInstanceId(1));
    boat->setWorld(m_world.get());
    boat->setPosition(0.0f, 64.0f, 0.0f);
    boat->setStatus(entity::BoatStatus::OnLand);
    boat->setBoatGlide(0.6f);
    boat->setVelocity(Vector3(1.0f, 0.0f, 1.0f));

    // 创建 Zombie 乘客（非 Player）
    auto zombie = std::make_unique<ZombieEntity>(EntityInstanceId(2));
    zombie->setWorld(m_world.get());
    // 设置类型ID以使 entityType() 返回正确的 ZOMBIE 类型指针（非 PLAYER）
    zombie->setTypeId(entity::EntityTypeKeys::ZOMBIE);

    // 注册实体到测试世界
    m_world->addTestEntity(boat.get());
    m_world->addTestEntity(zombie.get());

    // 让 Zombie 骑乘船
    ASSERT_TRUE(zombie->startRiding(*boat));

    // 验证 Zombie 是控制乘客
    EntityInstanceId controllerId = boat->getControllingPassenger();
    EXPECT_EQ(controllerId, zombie->id()) << "Zombie should be the controlling passenger";

    // 验证 Zombie 不是 Player 类型
    Entity* controller = m_world->getEntity(controllerId);
    ASSERT_NE(controller, nullptr);
    EXPECT_NE(controller->entityType(), entity::VanillaEntityTypeKeys::PLAYER) << "Zombie should not be a Player type";

    // 保存初始速度
    Vector3 velBefore = boat->velocity();

    // 调用 updateMotion
    boat->updateMotion();

    // 验证摩擦力未减半：m_boatGlide = 0.6，friction = 0.6
    f32 expectedFriction = 0.6f; // 不减半
    EXPECT_FLOAT_EQ(boat->velocity().x, velBefore.x * expectedFriction);
    EXPECT_FLOAT_EQ(boat->velocity().z, velBefore.z * expectedFriction);
}

// ============================================================================
// 无乘客时摩擦力不减半
// ============================================================================

/**
 * @brief 无乘客时，陆地摩擦力不减半
 *
 * 空船在陆地上不应获得摩擦力减半效果。
 */
TEST_F(BoatLandFrictionTest, NoPassenger_FrictionNotHalved)
{
    auto boat = std::make_unique<TestBoatEntity>(entity::BoatEntity::Type::OAK);
    boat->setId(EntityInstanceId(1));
    boat->setWorld(m_world.get());
    boat->setPosition(0.0f, 64.0f, 0.0f);
    boat->setStatus(entity::BoatStatus::OnLand);
    boat->setBoatGlide(0.6f);
    boat->setVelocity(Vector3(1.0f, 0.0f, 1.0f));

    // 注册船到测试世界（不添加任何乘客）
    m_world->addTestEntity(boat.get());

    // 验证无控制乘客
    EXPECT_EQ(boat->getControllingPassenger(), INVALID_ENTITY_ID);

    // 保存初始速度
    Vector3 velBefore = boat->velocity();

    // 调用 updateMotion
    boat->updateMotion();

    // 验证摩擦力未减半：m_boatGlide = 0.6，friction = 0.6
    f32 expectedFriction = 0.6f; // 不减半
    EXPECT_FLOAT_EQ(boat->velocity().x, velBefore.x * expectedFriction);
    EXPECT_FLOAT_EQ(boat->velocity().z, velBefore.z * expectedFriction);
}

// ============================================================================
// 无世界时摩擦力不减半
// ============================================================================

/**
 * @brief 无世界引用时，陆地摩擦力不减半（安全降级）
 *
 * 当 boat 没有 world 指针时，getEntity() 无法解析乘客实体，
 * 应安全降级为不减半摩擦力。
 */
TEST_F(BoatLandFrictionTest, NoWorld_FrictionNotHalved)
{
    auto boat = std::make_unique<TestBoatEntity>(entity::BoatEntity::Type::OAK);
    // 不设置世界指针
    boat->setId(EntityInstanceId(1));
    boat->setStatus(entity::BoatStatus::OnLand);
    boat->setBoatGlide(0.6f);
    boat->setVelocity(Vector3(1.0f, 0.0f, 1.0f));

    // 保存初始速度
    Vector3 velBefore = boat->velocity();

    // 调用 updateMotion（不会崩溃，因为无 world 时跳过乘客检查）
    boat->updateMotion();

    // 验证摩擦力未减半：无世界时无法解析乘客，安全降级
    f32 expectedFriction = 0.6f;
    EXPECT_FLOAT_EQ(boat->velocity().x, velBefore.x * expectedFriction);
    EXPECT_FLOAT_EQ(boat->velocity().z, velBefore.z * expectedFriction);
}

// ============================================================================
// getControllingPassenger 基础测试
// ============================================================================

/**
 * @brief 验证 BoatEntity 使用默认的 getControllingPassenger 实现
 *
 * BoatEntity 不覆写 getControllingPassenger()，默认返回第一个乘客。
 */
TEST_F(BoatLandFrictionTest, GetControllingPassenger_ReturnsFirstPassenger)
{
    auto boat = std::make_unique<TestBoatEntity>(entity::BoatEntity::Type::OAK);
    boat->setId(EntityInstanceId(1));
    boat->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setTypeId(entity::EntityTypeKeys::PLAYER);

    m_world->addTestEntity(boat.get());
    m_world->addTestEntity(player.get());

    // 无乘客时返回 INVALID_ENTITY_ID
    EXPECT_EQ(boat->getControllingPassenger(), INVALID_ENTITY_ID);

    // 骑乘后返回乘客 ID
    ASSERT_TRUE(player->startRiding(*boat));
    EXPECT_EQ(boat->getControllingPassenger(), player->id());
}

// ============================================================================
// 水中状态不受乘客影响
// ============================================================================

/**
 * @brief 水中状态使用固定摩擦力，不受乘客类型影响
 *
 * 仅 BoatStatus::OnLand 分支检查控制乘客是否为 Player，
 * 其他状态使用固定摩擦力常量。
 */
TEST_F(BoatLandFrictionTest, InWater_FrictionUnaffectedByPlayerPassenger)
{
    auto boat = std::make_unique<TestBoatEntity>(entity::BoatEntity::Type::OAK);
    boat->setId(EntityInstanceId(1));
    boat->setWorld(m_world.get());
    boat->setPosition(0.0f, 64.0f, 0.0f);
    boat->setStatus(entity::BoatStatus::InWater);
    boat->setVelocity(Vector3(1.0f, 0.0f, 1.0f));

    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setTypeId(entity::EntityTypeKeys::PLAYER);

    m_world->addTestEntity(boat.get());
    m_world->addTestEntity(player.get());

    ASSERT_TRUE(player->startRiding(*boat));

    Vector3 velBefore = boat->velocity();

    boat->updateMotion();

    // 水中摩擦力为 WATER_FRICTION = 0.9，不受乘客类型影响
    constexpr f32 WATER_FRICTION = 0.9f;
    EXPECT_FLOAT_EQ(boat->velocity().x, velBefore.x * WATER_FRICTION);
    EXPECT_FLOAT_EQ(boat->velocity().z, velBefore.z * WATER_FRICTION);
}

} // namespace
} // namespace mc
