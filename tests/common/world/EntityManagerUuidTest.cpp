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
 * copies of substantial portions of the Software.
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
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/entity/EntityManager.hpp"
#include <memory>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

/**
 * @brief EntityManager UUID 索引功能测试
 *
 * 测试 EntityManager 的 UUID 索引相关功能：
 * - getEntityByUuid() 查找
 * - hasEntityWithUuid() 存在性检查
 * - addEntity 时维护 UUID 索引
 * - removeEntity 时清理 UUID 索引
 * - removeDeadEntities 时清理 UUID 索引
 * - UUID 冲突处理
 * - 空UUID处理
 */
class EntityManagerUuidTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaEntities::registerAll(); }

    void TearDown() override {}

    EntityManager m_manager;
};

// ============================================================================
// getEntityByUuid 基础查找测试
// ============================================================================

TEST_F(EntityManagerUuidTest, GetEntityByUuid_BasicLookup)
{
    // 创建一个实体并添加到管理器
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr);
    ASSERT_NE(pig, nullptr);

    // 记住UUID
    const std::string uuid = pig->uuid();
    EXPECT_FALSE(uuid.empty()) << "实体UUID不应为空";

    EntityId id = m_manager.addEntity(std::move(pig));
    EXPECT_NE(id, 0u);

    // 通过UUID查找
    Entity* found = m_manager.getEntityByUuid(uuid);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id(), id);
    EXPECT_EQ(found->uuid(), uuid);
}

TEST_F(EntityManagerUuidTest, GetEntityByUuid_NotFound)
{
    // 查找不存在的UUID
    Entity* found = m_manager.getEntityByUuid("nonexistent_uuid");
    EXPECT_EQ(found, nullptr);
}

TEST_F(EntityManagerUuidTest, GetEntityByUuid_ConstVersion)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr);
    const std::string uuid = pig->uuid();
    EntityId id = m_manager.addEntity(std::move(pig));

    // const版本查找
    const EntityManager& constManager = m_manager;
    const Entity* found = constManager.getEntityByUuid(uuid);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id(), id);
}

// ============================================================================
// hasEntityWithUuid 存在性检查测试
// ============================================================================

TEST_F(EntityManagerUuidTest, HasEntityWithUuid_ExistingEntity)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr);
    const std::string uuid = pig->uuid();
    m_manager.addEntity(std::move(pig));

    EXPECT_TRUE(m_manager.hasEntityWithUuid(uuid));
}

TEST_F(EntityManagerUuidTest, HasEntityWithUuid_NonExistingUuid)
{
    EXPECT_FALSE(m_manager.hasEntityWithUuid("nonexistent_uuid"));
}

// ============================================================================
// addEntity 时维护 UUID 索引测试
// ============================================================================

TEST_F(EntityManagerUuidTest, AddEntity_UpdatesUuidIndex)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加多个实体，UUID索引应全部正确
    std::vector<std::string> uuids;
    for (int i = 0; i < 5; ++i) {
        auto pig = pigType->create(nullptr);
        uuids.push_back(pig->uuid());
        m_manager.addEntity(std::move(pig));
    }

    // 验证所有UUID都能找到
    for (const auto& uuid : uuids) {
        EXPECT_TRUE(m_manager.hasEntityWithUuid(uuid));
        Entity* found = m_manager.getEntityByUuid(uuid);
        ASSERT_NE(found, nullptr);
        EXPECT_EQ(found->uuid(), uuid);
    }
}

TEST_F(EntityManagerUuidTest, AddEntity_DifferentTypesAllIndexed)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    const EntityType* cowType = EntityRegistry::instance().getType(EntityTypes::COW);
    const EntityType* sheepType = EntityRegistry::instance().getType(EntityTypes::SHEEP);
    ASSERT_NE(pigType, nullptr);
    ASSERT_NE(cowType, nullptr);
    ASSERT_NE(sheepType, nullptr);

    auto pig = pigType->create(nullptr);
    auto cow = cowType->create(nullptr);
    auto sheep = sheepType->create(nullptr);

    std::string pigUuid = pig->uuid();
    std::string cowUuid = cow->uuid();
    std::string sheepUuid = sheep->uuid();

    m_manager.addEntity(std::move(pig));
    m_manager.addEntity(std::move(cow));
    m_manager.addEntity(std::move(sheep));

    EXPECT_TRUE(m_manager.hasEntityWithUuid(pigUuid));
    EXPECT_TRUE(m_manager.hasEntityWithUuid(cowUuid));
    EXPECT_TRUE(m_manager.hasEntityWithUuid(sheepUuid));
}

// ============================================================================
// removeEntity 时清理 UUID 索引测试
// ============================================================================

TEST_F(EntityManagerUuidTest, RemoveEntity_ClearsUuidIndex)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr);
    const std::string uuid = pig->uuid();
    EntityId id = m_manager.addEntity(std::move(pig));

    // 确认UUID存在
    EXPECT_TRUE(m_manager.hasEntityWithUuid(uuid));
    EXPECT_NE(m_manager.getEntityByUuid(uuid), nullptr);

    // 移除实体
    auto removed = m_manager.removeEntity(id);
    ASSERT_NE(removed, nullptr);

    // UUID索引应被清理
    EXPECT_FALSE(m_manager.hasEntityWithUuid(uuid));
    EXPECT_EQ(m_manager.getEntityByUuid(uuid), nullptr);
}

TEST_F(EntityManagerUuidTest, RemoveEntity_OnlyRemovesOwnUuid)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加两个实体
    auto pig1 = pigType->create(nullptr);
    auto pig2 = pigType->create(nullptr);
    std::string uuid1 = pig1->uuid();
    std::string uuid2 = pig2->uuid();
    EntityId id1 = m_manager.addEntity(std::move(pig1));
    EntityId id2 = m_manager.addEntity(std::move(pig2));

    // 移除第一个实体
    m_manager.removeEntity(id1);

    // 第一个UUID应被清理
    EXPECT_FALSE(m_manager.hasEntityWithUuid(uuid1));
    EXPECT_EQ(m_manager.getEntityByUuid(uuid1), nullptr);

    // 第二个UUID应仍然存在
    EXPECT_TRUE(m_manager.hasEntityWithUuid(uuid2));
    Entity* found2 = m_manager.getEntityByUuid(uuid2);
    ASSERT_NE(found2, nullptr);
    EXPECT_EQ(found2->id(), id2);
}

TEST_F(EntityManagerUuidTest, RemoveEntity_NonexistentId_NoCrash)
{
    // 移除不存在的实体ID不应崩溃
    auto result = m_manager.removeEntity(99999u);
    EXPECT_EQ(result, nullptr);
}

// ============================================================================
// removeDeadEntities 时清理 UUID 索引测试
// ============================================================================

TEST_F(EntityManagerUuidTest, RemoveDeadEntities_ClearsUuidIndex)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加两个实体
    auto pig1 = pigType->create(nullptr);
    auto pig2 = pigType->create(nullptr);
    std::string uuid1 = pig1->uuid();
    std::string uuid2 = pig2->uuid();
    EntityId id1 = m_manager.addEntity(std::move(pig1));
    EntityId id2 = m_manager.addEntity(std::move(pig2));

    // 标记第一个实体为已移除
    Entity* entity1 = m_manager.getEntity(id1);
    ASSERT_NE(entity1, nullptr);
    entity1->remove();

    // 调用removeDeadEntities
    m_manager.removeDeadEntities();

    // 第一个实体的UUID索引应被清理
    EXPECT_FALSE(m_manager.hasEntityWithUuid(uuid1));
    EXPECT_EQ(m_manager.getEntityByUuid(uuid1), nullptr);

    // 第二个实体不受影响
    EXPECT_TRUE(m_manager.hasEntityWithUuid(uuid2));
    EXPECT_NE(m_manager.getEntityByUuid(uuid2), nullptr);

    // 实体计数应减1
    EXPECT_EQ(m_manager.entityCount(), 1u);
}

// ============================================================================
// UUID 冲突处理测试
// ============================================================================

TEST_F(EntityManagerUuidTest, AddEntity_DuplicateUuid_OverrideMapping)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    // 手动设置相同UUID的两个实体
    auto pig1 = pigType->create(nullptr);
    auto pig2 = pigType->create(nullptr);

    // 强制设置相同的UUID
    const std::string sharedUuid = "test_shared_uuid_1234567890abcdef";
    pig1->setUuid(sharedUuid);
    pig2->setUuid(sharedUuid);

    EntityId id1 = m_manager.addEntity(std::move(pig1));
    EntityId id2 = m_manager.addEntity(std::move(pig2));

    // UUID索引应指向最后添加的实体（覆盖行为）
    Entity* found = m_manager.getEntityByUuid(sharedUuid);
    ASSERT_NE(found, nullptr);
    // UUID索引指向后添加的实体
    EXPECT_EQ(found->id(), id2);

    // 两个实体都存在于ID索引中
    EXPECT_TRUE(m_manager.hasEntity(id1));
    EXPECT_TRUE(m_manager.hasEntity(id2));
}

TEST_F(EntityManagerUuidTest, RemoveEntity_DuplicateUuid_NoAccidentalDelete)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    // 手动设置相同UUID的两个实体
    auto pig1 = pigType->create(nullptr);
    auto pig2 = pigType->create(nullptr);

    const std::string sharedUuid = "test_shared_uuid_1234567890abcdef";
    pig1->setUuid(sharedUuid);
    pig2->setUuid(sharedUuid);

    EntityId id1 = m_manager.addEntity(std::move(pig1));
    EntityId id2 = m_manager.addEntity(std::move(pig2));

    // 移除第一个实体（UUID索引指向第二个实体）
    m_manager.removeEntity(id1);

    // UUID索引应仍然有效（指向第二个实体）
    Entity* found = m_manager.getEntityByUuid(sharedUuid);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id(), id2);

    // 第二个实体仍存在
    EXPECT_TRUE(m_manager.hasEntity(id2));
}

// ============================================================================
// 空UUID处理测试
// ============================================================================

TEST_F(EntityManagerUuidTest, AddEntity_EmptyUuid_NotIndexed)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr);
    // 强制设置空UUID
    pig->setUuid("");

    EntityId id = m_manager.addEntity(std::move(pig));

    // 实体应该被成功添加
    EXPECT_TRUE(m_manager.hasEntity(id));

    // 空UUID不应被索引
    EXPECT_FALSE(m_manager.hasEntityWithUuid(""));
    EXPECT_EQ(m_manager.getEntityByUuid(""), nullptr);
}

// ============================================================================
// getEntityByUuid 与 getEntity 一致性测试
// ============================================================================

TEST_F(EntityManagerUuidTest, GetEntityByUuid_ConsistentWithGetEntity)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加多个实体
    std::vector<EntityId> ids;
    std::vector<std::string> uuids;
    for (int i = 0; i < 10; ++i) {
        auto pig = pigType->create(nullptr);
        uuids.push_back(pig->uuid());
        ids.push_back(m_manager.addEntity(std::move(pig)));
    }

    // 验证两种查找方式返回相同的实体
    for (size_t i = 0; i < ids.size(); ++i) {
        Entity* byId = m_manager.getEntity(ids[i]);
        Entity* byUuid = m_manager.getEntityByUuid(uuids[i]);
        ASSERT_NE(byId, nullptr);
        ASSERT_NE(byUuid, nullptr);
        EXPECT_EQ(byId, byUuid) << "通过ID和UUID查找应返回相同的实体指针";
        EXPECT_EQ(byId->id(), ids[i]);
        EXPECT_EQ(byId->uuid(), uuids[i]);
    }
}

// ============================================================================
// 大量实体性能验证测试
// ============================================================================

TEST_F(EntityManagerUuidTest, GetEntityByUuid_ManyEntities)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加100个实体
    constexpr int ENTITY_COUNT = 100;
    std::vector<std::string> uuids;
    for (int i = 0; i < ENTITY_COUNT; ++i) {
        auto pig = pigType->create(nullptr);
        uuids.push_back(pig->uuid());
        m_manager.addEntity(std::move(pig));
    }

    EXPECT_EQ(m_manager.entityCount(), ENTITY_COUNT);

    // 验证所有UUID都能正确查找
    for (const auto& uuid : uuids) {
        Entity* found = m_manager.getEntityByUuid(uuid);
        ASSERT_NE(found, nullptr);
        EXPECT_EQ(found->uuid(), uuid);
    }
}

// ============================================================================
// 移除后重新添加的UUID索引恢复测试
// ============================================================================

TEST_F(EntityManagerUuidTest, RemoveAndReAdd_UuidIndexRestored)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypes::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加实体
    auto pig = pigType->create(nullptr);
    const std::string uuid = pig->uuid();
    EntityId id1 = m_manager.addEntity(std::move(pig));
    EXPECT_TRUE(m_manager.hasEntityWithUuid(uuid));

    // 移除实体
    m_manager.removeEntity(id1);
    EXPECT_FALSE(m_manager.hasEntityWithUuid(uuid));

    // 添加新实体（新UUID）
    auto pig2 = pigType->create(nullptr);
    const std::string uuid2 = pig2->uuid();
    EntityId id2 = m_manager.addEntity(std::move(pig2));
    EXPECT_TRUE(m_manager.hasEntityWithUuid(uuid2));

    // 原UUID仍不存在
    EXPECT_FALSE(m_manager.hasEntityWithUuid(uuid));
}
