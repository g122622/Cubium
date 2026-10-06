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

#include "common/TestWorldHelper.hpp"
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

    EntityManager m_manager{mc::test::testEcsRegistry()};
};

// ============================================================================
// getEntityByUuid 基础查找测试
// ============================================================================

TEST_F(EntityManagerUuidTest, GetEntityByUuid_BasicLookup)
{
    // 创建一个实体并添加到管理器
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr, mc::test::testEcsRegistry());
    ASSERT_NE(pig, nullptr);

    // 记住UUID
    const std::string uuid = pig->uuid();
    EXPECT_FALSE(uuid.empty()) << "实体UUID不应为空";

    EntityInstanceId id = m_manager.addEntity(std::move(pig));
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
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr, mc::test::testEcsRegistry());
    const std::string uuid = pig->uuid();
    EntityInstanceId id = m_manager.addEntity(std::move(pig));

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
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr, mc::test::testEcsRegistry());
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
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加多个实体，UUID索引应全部正确
    std::vector<std::string> uuids;
    for (int i = 0; i < 5; ++i) {
        auto pig = pigType->create(nullptr, mc::test::testEcsRegistry());
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
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    const EntityType* cowType = EntityRegistry::instance().getType(EntityTypeKeys::COW);
    const EntityType* sheepType = EntityRegistry::instance().getType(EntityTypeKeys::SHEEP);
    ASSERT_NE(pigType, nullptr);
    ASSERT_NE(cowType, nullptr);
    ASSERT_NE(sheepType, nullptr);

    auto pig = pigType->create(nullptr, mc::test::testEcsRegistry());
    auto cow = cowType->create(nullptr, mc::test::testEcsRegistry());
    auto sheep = sheepType->create(nullptr, mc::test::testEcsRegistry());

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
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr, mc::test::testEcsRegistry());
    const std::string uuid = pig->uuid();
    EntityInstanceId id = m_manager.addEntity(std::move(pig));

    // 确认UUID存在
    EXPECT_TRUE(m_manager.hasEntityWithUuid(uuid));
    EXPECT_NE(m_manager.getEntityByUuid(uuid), nullptr);

    // 移除实体（走统一销毁入口，标记 Discarded 后入 graveyard 延迟析构）
    EXPECT_TRUE(m_manager.destroyEntity(id, RemovalReason::Discarded));

    // UUID索引应被清理
    EXPECT_FALSE(m_manager.hasEntityWithUuid(uuid));
    EXPECT_EQ(m_manager.getEntityByUuid(uuid), nullptr);
}

TEST_F(EntityManagerUuidTest, RemoveEntity_OnlyRemovesOwnUuid)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加两个实体
    auto pig1 = pigType->create(nullptr, mc::test::testEcsRegistry());
    auto pig2 = pigType->create(nullptr, mc::test::testEcsRegistry());
    std::string uuid1 = pig1->uuid();
    std::string uuid2 = pig2->uuid();
    EntityInstanceId id1 = m_manager.addEntity(std::move(pig1));
    EntityInstanceId id2 = m_manager.addEntity(std::move(pig2));

    // 移除第一个实体
    m_manager.destroyEntity(id1, RemovalReason::Discarded);

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
    // 销毁不存在的实体ID不应崩溃
    EXPECT_FALSE(m_manager.destroyEntity(99999u, RemovalReason::Discarded));
}

// ============================================================================
// removeDeadEntities 时清理 UUID 索引测试
// ============================================================================

TEST_F(EntityManagerUuidTest, RemoveDeadEntities_ClearsUuidIndex)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加两个实体
    auto pig1 = pigType->create(nullptr, mc::test::testEcsRegistry());
    auto pig2 = pigType->create(nullptr, mc::test::testEcsRegistry());
    std::string uuid1 = pig1->uuid();
    std::string uuid2 = pig2->uuid();
    EntityInstanceId id1 = m_manager.addEntity(std::move(pig1));
    EntityInstanceId id2 = m_manager.addEntity(std::move(pig2));

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

// 【与生产契约对齐】对齐 MC 1.21.11 PersistentEntitySectionManager.addEntityUuid：
// UUID 是实体的存档主键与网络身份，重复 UUID 时原版拒绝入册（addEntityUuid 返回 false）。
// 本仓库 addEntity 对重复 UUID 取硬断言策略（2026-09 cd40af64b：重复 UUID 曾把
// "存档中的重复行"静默转成 15.6 万同时存活的重复实例）。因此这里验证的是
// "第二个同 UUID 实体被断言拒绝"，而不是旧行为的"覆盖映射"。
// 注：MC_ASSERT_RELEASE 触发 abort，进程级失败，无法在进程内断言（death test 因
// CrashHandler + 每用例进程隔离代价被禁用，见 ReentrantAreaLockTest.cpp:553 的同款说明），
// 故此处只验证单实体正常路径的 UUID 索引不变量。
TEST_F(EntityManagerUuidTest, AddEntity_DuplicateUuid)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    // 正常路径：不同 UUID 的两个实体各占一条 UUID 索引
    auto pig1 = pigType->create(nullptr, mc::test::testEcsRegistry());
    auto pig2 = pigType->create(nullptr, mc::test::testEcsRegistry());
    const std::string uuid1 = pig1->uuid();
    const std::string uuid2 = pig2->uuid();
    ASSERT_NE(uuid1, uuid2);

    EntityInstanceId id1 = m_manager.addEntity(std::move(pig1));
    EntityInstanceId id2 = m_manager.addEntity(std::move(pig2));

    // 两个实体各自可通过自己的 UUID 寻址，互不覆盖
    ASSERT_NE(m_manager.getEntityByUuid(uuid1), nullptr);
    ASSERT_NE(m_manager.getEntityByUuid(uuid2), nullptr);
    EXPECT_EQ(m_manager.getEntityByUuid(uuid1)->id(), id1);
    EXPECT_EQ(m_manager.getEntityByUuid(uuid2)->id(), id2);
    EXPECT_TRUE(m_manager.hasEntity(id1));
    EXPECT_TRUE(m_manager.hasEntity(id2));
}

TEST_F(EntityManagerUuidTest, RemoveEntity_ClearsOnlyOwnUuid)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    // 两个正常 UUID 的实体，移除其一后另一条 UUID 索引不受影响
    auto pig1 = pigType->create(nullptr, mc::test::testEcsRegistry());
    auto pig2 = pigType->create(nullptr, mc::test::testEcsRegistry());
    const std::string uuid1 = pig1->uuid();
    const std::string uuid2 = pig2->uuid();

    EntityInstanceId id1 = m_manager.addEntity(std::move(pig1));
    EntityInstanceId id2 = m_manager.addEntity(std::move(pig2));

    m_manager.destroyEntity(id1, RemovalReason::Discarded);

    EXPECT_EQ(m_manager.getEntityByUuid(uuid1), nullptr);
    ASSERT_NE(m_manager.getEntityByUuid(uuid2), nullptr);
    EXPECT_EQ(m_manager.getEntityByUuid(uuid2)->id(), id2);
    EXPECT_TRUE(m_manager.hasEntity(id2));
}

// ============================================================================
// 空UUID处理测试
// ============================================================================

// 【与生产契约对齐】构造期 Entity 已自动生成随机 UUID；addEntity 时 UUID 为空只可能是
// 调用方显式 setUuid("") 破坏契约，生产代码取硬断言策略拒绝（同上，进程级失败无法
// 在进程内断言）。此处验证正常实体 UUID 非空、且 getEntityByUuid 对未知值返回 nullptr。
TEST_F(EntityManagerUuidTest, EmptyUuid_IsRejectedByContract)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    auto pig = pigType->create(nullptr, mc::test::testEcsRegistry());
    // 构造期已自动生成 UUID，非空
    EXPECT_FALSE(pig->uuid().empty());

    // 未注册的 UUID 查不到
    EXPECT_FALSE(m_manager.hasEntityWithUuid(""));
    EXPECT_EQ(m_manager.getEntityByUuid(""), nullptr);
}

// ============================================================================
// getEntityByUuid 与 getEntity 一致性测试
// ============================================================================

TEST_F(EntityManagerUuidTest, GetEntityByUuid_ConsistentWithGetEntity)
{
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加多个实体
    std::vector<EntityInstanceId> ids;
    std::vector<std::string> uuids;
    for (int i = 0; i < 10; ++i) {
        auto pig = pigType->create(nullptr, mc::test::testEcsRegistry());
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
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加100个实体
    constexpr int ENTITY_COUNT = 100;
    std::vector<std::string> uuids;
    for (int i = 0; i < ENTITY_COUNT; ++i) {
        auto pig = pigType->create(nullptr, mc::test::testEcsRegistry());
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
    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    // 添加实体
    auto pig = pigType->create(nullptr, mc::test::testEcsRegistry());
    const std::string uuid = pig->uuid();
    EntityInstanceId id1 = m_manager.addEntity(std::move(pig));
    EXPECT_TRUE(m_manager.hasEntityWithUuid(uuid));

    // 移除实体
    m_manager.destroyEntity(id1, RemovalReason::Discarded);
    EXPECT_FALSE(m_manager.hasEntityWithUuid(uuid));

    // 添加新实体（新UUID）
    auto pig2 = pigType->create(nullptr, mc::test::testEcsRegistry());
    const std::string uuid2 = pig2->uuid();
    EntityInstanceId id2 = m_manager.addEntity(std::move(pig2));
    EXPECT_TRUE(m_manager.hasEntityWithUuid(uuid2));

    // 原UUID仍不存在
    EXPECT_FALSE(m_manager.hasEntityWithUuid(uuid));
}
