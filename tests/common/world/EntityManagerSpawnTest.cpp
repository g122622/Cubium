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

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/gen/spawn/WorldGenSpawner.hpp"
#include <memory>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

/**
 * @brief EntityManager 实体生成测试
 *
 * 测试实体管理器的添加、获取、移除功能，
 * 这是 IntegratedServer 实体处理的基础。
 */
class EntityManagerSpawnTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaEntities::registerAll(); }

    void TearDown() override {}

    EntityManager m_manager;
};

namespace {

class ReentrantQueryEntity final : public Entity {
public:
    explicit ReentrantQueryEntity(EntityManager* manager)
        : Entity(EntityId(0), nullptr)
        , m_manager(manager)
    {}

    void tick() override
    {
        Entity::tick();
        auto entities = m_manager->getEntitiesInRange(position(), 16.0f, this);
        m_lastQueryCount = entities.size();
    }

    [[nodiscard]] size_t lastQueryCount() const { return m_lastQueryCount; }

private:
    EntityManager* m_manager = nullptr;
    size_t m_lastQueryCount = 0;
};

} // namespace

// ============================================================================
// 基础实体添加测试
// ============================================================================

TEST_F(EntityManagerSpawnTest, AddEntity)
{
    // 创建一个猪实体
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr);
    ASSERT_NE(pig, nullptr);

    EntityId id = m_manager.addEntity(std::move(pig));
    EXPECT_NE(id, 0u);
    EXPECT_TRUE(m_manager.hasEntity(id));
}

TEST_F(EntityManagerSpawnTest, AddMultipleEntities)
{
    std::vector<EntityId> ids;
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    for (int i = 0; i < 5; ++i) {
        auto pig = pigType->create(nullptr);
        EntityId id = m_manager.addEntity(std::move(pig));
        ids.push_back(id);
    }

    EXPECT_EQ(m_manager.entityCount(), 5u);

    for (EntityId id : ids) {
        EXPECT_TRUE(m_manager.hasEntity(id));
    }
}

TEST_F(EntityManagerSpawnTest, AddDifferentEntityTypes)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    const EntityType* cowType = EntityRegistry::instance().getType(EntityTypes::COW);
    const EntityType* sheepType = EntityRegistry::instance().getType(EntityTypes::SHEEP);
    const EntityType* chickenType = EntityRegistry::instance().getType(EntityTypes::CHICKEN);

    ASSERT_NE(pigType, nullptr);
    ASSERT_NE(cowType, nullptr);
    ASSERT_NE(sheepType, nullptr);
    ASSERT_NE(chickenType, nullptr);

    auto pig = pigType->create(nullptr);
    auto cow = cowType->create(nullptr);
    auto sheep = sheepType->create(nullptr);
    auto chicken = chickenType->create(nullptr);

    EntityId pigId = m_manager.addEntity(std::move(pig));
    EntityId cowId = m_manager.addEntity(std::move(cow));
    EntityId sheepId = m_manager.addEntity(std::move(sheep));
    EntityId chickenId = m_manager.addEntity(std::move(chicken));

    EXPECT_EQ(m_manager.entityCount(), 4u);

    EXPECT_TRUE(m_manager.hasEntity(pigId));
    EXPECT_TRUE(m_manager.hasEntity(cowId));
    EXPECT_TRUE(m_manager.hasEntity(sheepId));
    EXPECT_TRUE(m_manager.hasEntity(chickenId));
}

// ============================================================================
// 实体获取测试
// ============================================================================

TEST_F(EntityManagerSpawnTest, GetEntity)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr);
    pig->setPosition(10.0f, 64.0f, 20.0f);

    EntityId id = m_manager.addEntity(std::move(pig));

    Entity* entity = m_manager.getEntity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_FLOAT_EQ(entity->x(), 10.0f);
    EXPECT_FLOAT_EQ(entity->y(), 64.0f);
    EXPECT_FLOAT_EQ(entity->z(), 20.0f);
}

TEST_F(EntityManagerSpawnTest, GetEntityNotFound)
{
    Entity* entity = m_manager.getEntity(99999);
    EXPECT_EQ(entity, nullptr);
}

TEST_F(EntityManagerSpawnTest, GetEntityByType)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    const EntityType* cowType = EntityRegistry::instance().getType(EntityTypes::COW);
    ASSERT_NE(pigType, nullptr);
    ASSERT_NE(cowType, nullptr);

    EntityId pigId = m_manager.addEntity(pigType->create(nullptr));
    EntityId cowId = m_manager.addEntity(cowType->create(nullptr));

    EXPECT_TRUE(m_manager.hasEntity(pigId));
    EXPECT_TRUE(m_manager.hasEntity(cowId));
    EXPECT_EQ(m_manager.entityCount(), 2u);
}

// ============================================================================
// 实体移除测试
// ============================================================================

TEST_F(EntityManagerSpawnTest, RemoveEntity)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr);
    EntityId id = m_manager.addEntity(std::move(pig));

    EXPECT_TRUE(m_manager.hasEntity(id));

    m_manager.removeEntity(id);

    EXPECT_FALSE(m_manager.hasEntity(id));
    EXPECT_EQ(m_manager.entityCount(), 0u);
}

TEST_F(EntityManagerSpawnTest, RemoveNonExistentEntity)
{
    // 移除不存在的实体不应崩溃
    m_manager.removeEntity(99999);
    EXPECT_EQ(m_manager.entityCount(), 0u);
}

TEST_F(EntityManagerSpawnTest, RemoveAndAddAgain)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    const EntityType* cowType = EntityRegistry::instance().getType(EntityTypes::COW);
    ASSERT_NE(pigType, nullptr);
    ASSERT_NE(cowType, nullptr);

    auto pig = pigType->create(nullptr);
    EntityId id1 = m_manager.addEntity(std::move(pig));
    EXPECT_TRUE(m_manager.hasEntity(id1));
    EXPECT_EQ(m_manager.entityCount(), 1u);

    m_manager.removeEntity(id1);
    EXPECT_FALSE(m_manager.hasEntity(id1));
    EXPECT_EQ(m_manager.entityCount(), 0u);

    auto cow = cowType->create(nullptr);
    EntityId id2 = m_manager.addEntity(std::move(cow));
    EXPECT_TRUE(m_manager.hasEntity(id2));
    EXPECT_EQ(m_manager.entityCount(), 1u);
}

// ============================================================================
// 实体属性测试
// ============================================================================

TEST_F(EntityManagerSpawnTest, EntityPositionAfterSpawn)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr);
    pig->setPosition(100.5f, 65.0f, -50.5f);
    pig->setRotation(90.0f, 45.0f);

    EntityId id = m_manager.addEntity(std::move(pig));

    Entity* entity = m_manager.getEntity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_FLOAT_EQ(entity->x(), 100.5f);
    EXPECT_FLOAT_EQ(entity->y(), 65.0f);
    EXPECT_FLOAT_EQ(entity->z(), -50.5f);
    EXPECT_FLOAT_EQ(entity->yaw(), 90.0f);
    EXPECT_FLOAT_EQ(entity->pitch(), 45.0f);
}

// ============================================================================
// SpawnedEntityData 集成测试
// ============================================================================

TEST_F(EntityManagerSpawnTest, SpawnFromSpawnedEntityData)
{
    // 模拟从区块生成获取的 SpawnedEntityData
    SpawnedEntityData data;
    data.entityTypeId = EntityTypes::COW;
    data.x = 50.0f;
    data.y = 64.0f;
    data.z = 100.0f;

    // 通过类型创建实体
    auto* entityType = EntityRegistry::instance().getType(data.entityTypeId);
    ASSERT_NE(entityType, nullptr);
    EXPECT_TRUE(entityType->canSummon());

    auto entity = entityType->create(nullptr);
    ASSERT_NE(entity, nullptr);

    entity->setPosition(data.x, data.y, data.z);

    EntityId id = m_manager.addEntity(std::move(entity));

    Entity* spawned = m_manager.getEntity(id);
    ASSERT_NE(spawned, nullptr);
    EXPECT_FLOAT_EQ(spawned->x(), 50.0f);
    EXPECT_FLOAT_EQ(spawned->y(), 64.0f);
    EXPECT_FLOAT_EQ(spawned->z(), 100.0f);
}

TEST_F(EntityManagerSpawnTest, BatchSpawnFromSpawnedEntityData)
{
    std::vector<SpawnedEntityData> spawnedEntities;

    // 创建多个实体数据
    for (int i = 0; i < 3; ++i) {
        SpawnedEntityData data;
        data.entityTypeId = EntityTypes::CHICKEN;
        data.x = static_cast<f32>(i * 10);
        data.y = 64.0f;
        data.z = static_cast<f32>(i * 10);
        spawnedEntities.push_back(data);
    }

    // 批量生成实体
    std::vector<EntityId> ids;
    for (const auto& data : spawnedEntities) {
        auto* entityType = EntityRegistry::instance().getType(data.entityTypeId);
        if (entityType && entityType->canSummon()) {
            auto entity = entityType->create(nullptr);
            entity->setPosition(data.x, data.y, data.z);
            ids.push_back(m_manager.addEntity(std::move(entity)));
        }
    }

    EXPECT_EQ(ids.size(), 3u);
    EXPECT_EQ(m_manager.entityCount(), 3u);
}

// ============================================================================
// MobEntity 特定测试
// ============================================================================

TEST_F(EntityManagerSpawnTest, MobEntityIsLivingEntity)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);

    EntityId id = m_manager.addEntity(std::move(pig));

    Entity* entity = m_manager.getEntity(id);
    ASSERT_NE(entity, nullptr);

    // 检查是否是 MobEntity
    auto* mob = dynamic_cast<MobEntity*>(entity);
    ASSERT_NE(mob, nullptr);
}

TEST_F(EntityManagerSpawnTest, AnimalEntityIsMobEntity)
{
    const EntityType* cowType = EntityRegistry::instance().getType(EntityTypes::COW);
    ASSERT_NE(cowType, nullptr);

    auto cow = cowType->create(nullptr);
    EntityId id = m_manager.addEntity(std::move(cow));

    Entity* entity = m_manager.getEntity(id);
    ASSERT_NE(entity, nullptr);

    // AnimalEntity 应该可以转换为 MobEntity
    auto* mob = dynamic_cast<MobEntity*>(entity);
    EXPECT_NE(mob, nullptr);

    // AnimalEntity 应该可以转换为 LivingEntity
    auto* living = dynamic_cast<LivingEntity*>(entity);
    EXPECT_NE(living, nullptr);
}

// ============================================================================
// 清空测试
// ============================================================================

TEST_F(EntityManagerSpawnTest, RemoveMultipleEntities)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加多个实体并记录ID
    std::vector<EntityId> ids;
    for (int i = 0; i < 5; ++i) {
        auto pig = pigType->create(nullptr);
        ids.push_back(m_manager.addEntity(std::move(pig)));
    }

    EXPECT_EQ(m_manager.entityCount(), 5u);

    // 逐个移除
    for (EntityId id : ids) {
        EXPECT_TRUE(m_manager.hasEntity(id));
        m_manager.removeEntity(id);
        EXPECT_FALSE(m_manager.hasEntity(id));
    }

    EXPECT_EQ(m_manager.entityCount(), 0u);
}

TEST_F(EntityManagerSpawnTest, TickAllowsReentrantRangeQuery)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto queryEntity = std::make_unique<ReentrantQueryEntity>(&m_manager);
    queryEntity->setPosition(0.0f, 64.0f, 0.0f);
    EntityId queryEntityId = m_manager.addEntity(std::move(queryEntity));

    auto pig = pigType->create(nullptr);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(1.0f, 64.0f, 1.0f);
    m_manager.addEntity(std::move(pig));

    EXPECT_NO_FATAL_FAILURE(m_manager.tick());

    auto* updatedEntity = dynamic_cast<ReentrantQueryEntity*>(m_manager.getEntity(queryEntityId));
    ASSERT_NE(updatedEntity, nullptr);
    EXPECT_EQ(updatedEntity->lastQueryCount(), 1u);
}

TEST_F(EntityManagerSpawnTest, ForEachAllowsReentrantQueries)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr);
    ASSERT_NE(pig, nullptr);
    EntityId id = m_manager.addEntity(std::move(pig));

    bool callbackInvoked = false;
    m_manager.forEachEntity([this, id, &callbackInvoked](Entity*) {
        callbackInvoked = true;
        EXPECT_TRUE(m_manager.hasEntity(id));
        EXPECT_EQ(m_manager.getEntitiesInRange(Vector3(0.0f, 0.0f, 0.0f), 128.0f).size(), 1u);
        return true;
    });

    EXPECT_TRUE(callbackInvoked);
}
