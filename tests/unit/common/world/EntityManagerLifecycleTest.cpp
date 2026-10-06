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

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/RemovalReason.hpp"
#include "common/world/entity/EntityManager.hpp"
#include <memory>
#include <gtest/gtest.h>

using namespace mc;

namespace {

/**
 * @brief 生命周期探针实体
 *
 * 记录析构次数与 onRemoval 调用次数/原因，用于验证 EntityManager 的两阶段销毁
 * （逻辑移除 → graveyard 延迟物理析构）与 RemovalReason 传递语义。
 * tick() 空实现避免依赖 world（本测试用 nullptr world 构造）。
 */
class ProbeEntity final : public Entity {
public:
    ProbeEntity(EntityInstanceId id, ecs::EntityRegistry& registry, i32* destroyCounter, i32* removalCounter)
        : Entity(id, nullptr, registry)
        , m_destroyCounter(destroyCounter)
        , m_removalCounter(removalCounter)
    {}

    ~ProbeEntity() override
    {
        if (m_destroyCounter != nullptr) {
            ++*m_destroyCounter;
        }
    }

    void tick() override {}

    void onRemoval(RemovalReason reason) override
    {
        if (m_removalCounter != nullptr) {
            ++*m_removalCounter;
        }
        m_lastReason = reason;
        Entity::onRemoval(reason);
    }

    [[nodiscard]] std::optional<RemovalReason> lastReason() const { return m_lastReason; }

private:
    i32* m_destroyCounter = nullptr;
    i32* m_removalCounter = nullptr;
    std::optional<RemovalReason> m_lastReason;
};

} // namespace

/**
 * @brief EntityManager 实体销毁生命周期测试
 *
 * 覆盖统一销毁入口 destroyEntity / takeEntity 与 RemovalReason 语义：
 * 延迟析构（graveyard）、幂等标记、onRemoval 回调、原因传递。
 */
class EntityManagerLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override { m_manager.setSimulationDistance(32); }

    /// 创建并登记一个探针实体，返回其 id
    EntityInstanceId addProbe()
    {
        auto probe = std::make_unique<ProbeEntity>(EntityInstanceId(0), mc::test::testEcsRegistry(), &m_destroyCount,
            &m_removalCount);
        return m_manager.addEntity(std::move(probe));
    }

    EntityManager m_manager{mc::test::testEcsRegistry()};
    i32 m_destroyCount = 0;
    i32 m_removalCount = 0;
};

// ============================================================================
// RemovalReason 取值表
// ============================================================================

TEST_F(EntityManagerLifecycleTest, RemovalReason_ShouldDestroyAndShouldSave)
{
    // shouldDestroy：仅 Killed / Discarded（摧毁类副作用：掉落、断骑乘、容器掉落）
    EXPECT_TRUE(shouldDestroy(RemovalReason::Killed));
    EXPECT_TRUE(shouldDestroy(RemovalReason::Discarded));
    EXPECT_FALSE(shouldDestroy(RemovalReason::UnloadedToChunk));
    EXPECT_FALSE(shouldDestroy(RemovalReason::UnloadedWithPlayer));
    EXPECT_FALSE(shouldDestroy(RemovalReason::ChangedDimension));

    // shouldSave：仅 UnloadedToChunk（区块卸载需写盘）
    EXPECT_FALSE(shouldSave(RemovalReason::Killed));
    EXPECT_FALSE(shouldSave(RemovalReason::Discarded));
    EXPECT_TRUE(shouldSave(RemovalReason::UnloadedToChunk));
    EXPECT_FALSE(shouldSave(RemovalReason::UnloadedWithPlayer));
    EXPECT_FALSE(shouldSave(RemovalReason::ChangedDimension));
}

// ============================================================================
// destroyEntity 两阶段销毁
// ============================================================================

TEST_F(EntityManagerLifecycleTest, DestroyEntity_DefersDestructionUntilTick)
{
    EntityInstanceId id = addProbe();
    ASSERT_TRUE(m_manager.hasEntity(id));

    // 销毁：立即从 m_entities 摘除（不可再查询），但对象尚未析构（入 graveyard 延迟队列）。
    EXPECT_TRUE(m_manager.destroyEntity(id, RemovalReason::Killed));
    EXPECT_FALSE(m_manager.hasEntity(id));
    EXPECT_EQ(m_manager.entityCount(), 0u);
    EXPECT_EQ(m_destroyCount, 0); // 关键：延迟析构，本 tick 不 free
    EXPECT_EQ(m_removalCount, 1); // onRemoval 在标记时同步调用

    // tick() 末尾冲刷 graveyard，此时才真正物理析构。
    m_manager.tick();
    EXPECT_EQ(m_destroyCount, 1);
}

TEST_F(EntityManagerLifecycleTest, DestroyEntity_NonExistent_ReturnsFalse)
{
    EXPECT_FALSE(m_manager.destroyEntity(EntityInstanceId(99999), RemovalReason::Discarded));
    EXPECT_EQ(m_manager.entityCount(), 0u);
}

TEST_F(EntityManagerLifecycleTest, DestroyEntity_DoesNotRunOnRemovalTwice)
{
    EntityInstanceId id = addProbe();
    m_manager.destroyEntity(id, RemovalReason::Discarded);
    m_manager.tick();
    EXPECT_EQ(m_removalCount, 1);
    EXPECT_EQ(m_destroyCount, 1);
}

// ============================================================================
// takeEntity 所有权移交
// ============================================================================

TEST_F(EntityManagerLifecycleTest, TakeEntity_TransfersOwnershipWithoutDestruction)
{
    EntityInstanceId id = addProbe();

    auto entity = m_manager.takeEntity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->id(), id);
    EXPECT_FALSE(m_manager.hasEntity(id));
    EXPECT_EQ(m_destroyCount, 0); // 所有权交给调用者，未析构

    // 调用者持有 unique_ptr，析构时计数 +1（验证移交后对象仍存活）
    entity.reset();
    EXPECT_EQ(m_destroyCount, 1);
}

TEST_F(EntityManagerLifecycleTest, TakeEntity_NonExistent_ReturnsNullptr)
{
    EXPECT_EQ(m_manager.takeEntity(EntityInstanceId(99999)), nullptr);
}

// ============================================================================
// RemovalReason 传递与幂等
// ============================================================================

TEST_F(EntityManagerLifecycleTest, Remove_RecordsReasonAndIsIdempotent)
{
    EntityInstanceId id = addProbe();
    Entity* entity = m_manager.getEntity(id);
    ASSERT_NE(entity, nullptr);

    EXPECT_FALSE(entity->isRemoved());
    EXPECT_FALSE(entity->removalReason().has_value());

    // 首次移除记录原因
    entity->remove(RemovalReason::UnloadedToChunk);
    EXPECT_TRUE(entity->isRemoved());
    ASSERT_TRUE(entity->removalReason().has_value());
    EXPECT_EQ(entity->removalReason().value(), RemovalReason::UnloadedToChunk);
    EXPECT_EQ(m_removalCount, 1);

    // 二次移除：原因不被覆盖、onRemoval 不重复触发（对齐 vanilla setRemoved 的 removalReason==null 守卫）
    entity->remove(RemovalReason::Killed);
    EXPECT_EQ(entity->removalReason().value(), RemovalReason::UnloadedToChunk);
    EXPECT_EQ(m_removalCount, 1);

    // 标记后由 tick 收走（死亡实体路径）：本 tick 末尾 _removeDeadEntitiesInternal 收集入
    // graveyard，下一 tick 末尾才物理析构（两 tick 延迟，与 destroyEntity 的立即入队差一 tick）。
    m_manager.tick();
    EXPECT_FALSE(m_manager.hasEntity(id));
    EXPECT_EQ(m_destroyCount, 0); // 本 tick 仅入队，尚未析构
    m_manager.tick();
    EXPECT_EQ(m_destroyCount, 1);
}

TEST_F(EntityManagerLifecycleTest, Discard_IsDiscardedReason)
{
    EntityInstanceId id = addProbe();
    Entity* entity = m_manager.getEntity(id);
    ASSERT_NE(entity, nullptr);

    entity->discard();
    ASSERT_TRUE(entity->removalReason().has_value());
    EXPECT_EQ(entity->removalReason().value(), RemovalReason::Discarded);
}

TEST_F(EntityManagerLifecycleTest, RemoveDefault_IsKilledReason)
{
    EntityInstanceId id = addProbe();
    Entity* entity = m_manager.getEntity(id);
    ASSERT_NE(entity, nullptr);

    // 无参 remove() 默认 Killed（对齐死亡流程 tickDeath）
    entity->remove();
    ASSERT_TRUE(entity->removalReason().has_value());
    EXPECT_EQ(entity->removalReason().value(), RemovalReason::Killed);
}

// ============================================================================
// 无世界引用的健壮性
// ============================================================================

TEST_F(EntityManagerLifecycleTest, DestroyEntity_NullWorld_NoCrash)
{
    // 探针实体以 nullptr world 构造。remove() 内的断骑乘/乘客放下在无 world 时跳过，
    // 不应崩溃（单元测试夹具常见场景）。
    EntityInstanceId id = addProbe();
    EXPECT_NO_FATAL_FAILURE(m_manager.destroyEntity(id, RemovalReason::Killed));
    EXPECT_NO_FATAL_FAILURE(m_manager.tick());
    EXPECT_EQ(m_destroyCount, 1);
}

TEST_F(EntityManagerLifecycleTest, MultipleEntities_AllDeferredThenDestroyed)
{
    std::vector<EntityInstanceId> ids;
    for (int i = 0; i < 5; ++i) {
        ids.push_back(addProbe());
    }
    EXPECT_EQ(m_manager.entityCount(), 5u);

    for (EntityInstanceId id : ids) {
        EXPECT_TRUE(m_manager.destroyEntity(id, RemovalReason::Discarded));
    }
    EXPECT_EQ(m_manager.entityCount(), 0u);
    EXPECT_EQ(m_destroyCount, 0); // 全部延迟

    m_manager.tick();
    EXPECT_EQ(m_destroyCount, 5); // 一次性冲刷
}
