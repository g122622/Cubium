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

#include "common/world/entity/spatial/EntitySpatialIndex.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include <algorithm>
#include <memory>
#include <unordered_set>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

namespace {

/// 轻量测试实体：构造后用 setTypeId 控制类型（PIG/COW/PLAYER），
/// 避免依赖具体生物实体构造副作用（finalizeSpawn 等）。
/// 构造时 m_pos=(0,0,0)，boundingBox() 懒初始化基于 m_pos 计算。
/// 自动分配递增唯一 id——EntitySpatialIndex 用 entity.id() 作 m_entitySection 键，
/// 多实体 id 相同会互相覆盖，故测试实体必须 id 唯一。
class TestEntity final : public Entity {
public:
    explicit TestEntity(ecs::EntityRegistry& registry, const std::string& typeId)
        : Entity(EntityInstanceId(0), nullptr, registry)
    {
        setTypeId(typeId);
        setId(nextId());
    }

private:
    static EntityInstanceId nextId()
    {
        static EntityInstanceId s_counter = 1;
        return s_counter++;
    }
};

/// 取 box 内的实体（全扫 oracle，线性遍历给定集合做 AABB 精筛）
std::vector<Entity*> oracleInAABB(
    const std::vector<std::unique_ptr<TestEntity>>& all, const AxisAlignedBB& box, const Entity* except)
{
    std::vector<Entity*> result;
    for (const auto& e : all) {
        if (e == nullptr || e.get() == except) {
            continue;
        }
        if (box.intersects(e->boundingBox())) {
            result.push_back(e.get());
        }
    }
    return result;
}

} // namespace

/**
 * @brief 3D section 空间索引单元测试
 *
 * 验证 EntitySpatialIndex 的分桶/迁移/查询/类型子列表/玩家专表/空桶回收/区块列语义，
 * 以及 1000 实体规模下相对全扫 oracle 的查询正确性。
 */
class EntitySpatialIndexTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaEntities::registerAll(); }

    void TearDown() override {}

    EntitySpatialIndex m_index;
};

// ============================================================================
// 基础 add/remove 与 section 计数
// ============================================================================
TEST_F(EntitySpatialIndexTest, AddRemoveUpdatesCounts)
{
    auto pig = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    TestEntity* pigPtr = pig.get();

    EXPECT_EQ(m_index.entityCount(), 0u);
    EXPECT_EQ(m_index.sectionCount(), 0u);

    m_index.addEntity(*pigPtr);
    EXPECT_EQ(m_index.entityCount(), 1u);
    EXPECT_EQ(m_index.sectionCount(), 1u);

    m_index.removeEntity(*pigPtr);
    EXPECT_EQ(m_index.entityCount(), 0u);
    // 空桶立即回收
    EXPECT_EQ(m_index.sectionCount(), 0u);
}

// ============================================================================
// AABB 查询：跨 section 与同 section 命中
// ============================================================================
TEST_F(EntitySpatialIndexTest, CollectEntitiesInAABB)
{
    // 两个实体分属不同 section（间距 32 > 16）
    auto a = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    a->setPosition(0.0f, 64.0f, 0.0f);
    auto b = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    b->setPosition(32.0f, 64.0f, 0.0f);
    m_index.addEntity(*a);
    m_index.addEntity(*b);

    // 小盒只覆盖 a
    AxisAlignedBB boxA(-1.0f, 63.0f, -1.0f, 1.0f, 65.0f, 1.0f);
    std::vector<Entity*> hit;
    m_index.collectEntitiesInAABB(boxA, nullptr, [&](Entity& e) -> bool {
        hit.push_back(&e);
        return true;
    });
    EXPECT_EQ(hit.size(), 1u);
    EXPECT_EQ(hit[0], a.get());

    // 大盒覆盖两者
    hit.clear();
    AxisAlignedBB boxAll(-5.0f, 63.0f, -5.0f, 40.0f, 65.0f, 5.0f);
    m_index.collectEntitiesInAABB(boxAll, nullptr, [&](Entity& e) -> bool {
        hit.push_back(&e);
        return true;
    });
    EXPECT_EQ(hit.size(), 2u);

    // except 跳过 a
    hit.clear();
    m_index.collectEntitiesInAABB(boxAll, a.get(), [&](Entity& e) -> bool {
        hit.push_back(&e);
        return true;
    });
    EXPECT_EQ(hit.size(), 1u);
    EXPECT_EQ(hit[0], b.get());
}

// ============================================================================
// 球查询：距离精筛（外接盒覆盖但距离超 range 的实体不命中）
// ============================================================================
TEST_F(EntitySpatialIndexTest, CollectEntitiesInRangeDistanceFilter)
{
    auto near_ = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    near_->setPosition(5.0f, 64.0f, 0.0f); // 距原点 5
    auto far = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    far->setPosition(20.0f, 64.0f, 0.0f); // 距原点 20
    m_index.addEntity(*near_);
    m_index.addEntity(*far);

    std::vector<Entity*> hit;
    m_index.collectEntitiesInRange(Vector3(0.0f, 64.0f, 0.0f), 10.0f, nullptr, [&](Entity& e) -> bool {
        hit.push_back(&e);
        return true;
    });
    EXPECT_EQ(hit.size(), 1u);
    EXPECT_EQ(hit[0], near_.get());
}

// ============================================================================
// 跨 section 迁移：onEntityPositionChanged 后查询读到新位置
// ============================================================================
TEST_F(EntitySpatialIndexTest, MoveAcrossSectionUpdatesIndex)
{
    auto pig = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_index.addEntity(*pig);

    // 迁移前：原点附近命中
    AxisAlignedBB boxOrigin(-1.0f, 63.0f, -1.0f, 1.0f, 65.0f, 1.0f);
    std::vector<Entity*> hit;
    m_index.collectEntitiesInAABB(boxOrigin, nullptr, [&](Entity& e) -> bool {
        hit.push_back(&e);
        return true;
    });
    EXPECT_EQ(hit.size(), 1u);

    // 跨 section 移动到 (32,64,0)
    pig->setPosition(32.0f, 64.0f, 0.0f);
    m_index.onEntityPositionChanged(*pig);

    // 迁移后：原点盒不再命中
    hit.clear();
    m_index.collectEntitiesInAABB(boxOrigin, nullptr, [&](Entity& e) -> bool {
        hit.push_back(&e);
        return true;
    });
    EXPECT_EQ(hit.size(), 0u);

    // 新位置盒命中
    hit.clear();
    AxisAlignedBB boxNew(31.0f, 63.0f, -1.0f, 33.0f, 65.0f, 1.0f);
    m_index.collectEntitiesInAABB(boxNew, nullptr, [&](Entity& e) -> bool {
        hit.push_back(&e);
        return true;
    });
    EXPECT_EQ(hit.size(), 1u);
    EXPECT_EQ(hit[0], pig.get());

    // section 计数仍为 1（旧空桶回收）
    EXPECT_EQ(m_index.sectionCount(), 1u);
}

// ============================================================================
// 同 section 内移动：不迁移，计数不变
// ============================================================================
TEST_F(EntitySpatialIndexTest, MoveWithinSectionNoMigration)
{
    auto pig = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_index.addEntity(*pig);
    EXPECT_EQ(m_index.sectionCount(), 1u);

    pig->setPosition(5.0f, 64.0f, 5.0f); // 仍在同一 section
    m_index.onEntityPositionChanged(*pig);
    EXPECT_EQ(m_index.sectionCount(), 1u);
    EXPECT_EQ(m_index.entityCount(), 1u);
}

// ============================================================================
// 类型分桶：collectEntitiesByType 只返回指定类型
// ============================================================================
TEST_F(EntitySpatialIndexTest, CollectEntitiesByType)
{
    auto pig = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    auto cow = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::COW);
    cow->setPosition(0.0f, 64.0f, 0.0f);
    auto pig2 = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    pig2->setPosition(0.0f, 64.0f, 0.0f);
    m_index.addEntity(*pig);
    m_index.addEntity(*cow);
    m_index.addEntity(*pig2);

    const EntityType* pigType = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    std::vector<Entity*> hit;
    m_index.collectEntitiesByType(pigType, [&](Entity& e) -> bool {
        hit.push_back(&e);
        return true;
    });
    EXPECT_EQ(hit.size(), 2u);
    for (Entity* e : hit) {
        EXPECT_EQ(e->entityType(), pigType);
    }

    // nullptr 类型返回空
    hit.clear();
    m_index.collectEntitiesByType(nullptr, [&](Entity& e) -> bool {
        hit.push_back(&e);
        return true;
    });
    EXPECT_TRUE(hit.empty());
}

// ============================================================================
// 玩家专表：add/remove 同步维护，getClosestPlayer 距离判定
// ============================================================================
TEST_F(EntitySpatialIndexTest, PlayerTableAndClosestPlayer)
{
    auto player = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PLAYER);
    player->setPosition(0.0f, 64.0f, 0.0f);
    auto pig = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_index.addEntity(*player);
    m_index.addEntity(*pig);

    // 玩家专表只含 player，不含 pig
    EXPECT_EQ(m_index.players().size(), 1u);
    EXPECT_EQ(m_index.players()[0], player.get());

    // getClosestPlayer 命中 player
    Entity* closest = m_index.getClosestPlayer(Vector3(1.0f, 64.0f, 0.0f), 10.0f, nullptr);
    EXPECT_EQ(closest, player.get());

    // 超出 maxDistance 返回 nullptr
    closest = m_index.getClosestPlayer(Vector3(100.0f, 64.0f, 0.0f), 10.0f, nullptr);
    EXPECT_EQ(closest, nullptr);

    // exclude 跳过 player
    closest = m_index.getClosestPlayer(Vector3(0.0f, 64.0f, 0.0f), 10.0f, player.get());
    EXPECT_EQ(closest, nullptr);

    // remove 后玩家专表清空
    m_index.removeEntity(*player);
    EXPECT_EQ(m_index.players().size(), 0u);
    EXPECT_EQ(m_index.getClosestPlayer(Vector3(0.0f, 64.0f, 0.0f), 100.0f, nullptr), nullptr);
}

// ============================================================================
// 空桶回收：remove 后空 section 立即 erase
// ============================================================================
TEST_F(EntitySpatialIndexTest, EmptySectionReclaimedAfterRemove)
{
    std::vector<std::unique_ptr<TestEntity>> entities;
    // 在 3 个不同 section 各放 1 个实体
    for (int i = 0; i < 3; ++i) {
        auto e = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
        e->setPosition(static_cast<f32>(i * 32), 64.0f, 0.0f);
        m_index.addEntity(*e);
        entities.push_back(std::move(e));
    }
    EXPECT_EQ(m_index.sectionCount(), 3u);

    // 移除中间一个，其 section 应回收
    m_index.removeEntity(*entities[1]);
    EXPECT_EQ(m_index.sectionCount(), 2u);
    EXPECT_EQ(m_index.entityCount(), 2u);
}

// ============================================================================
// 区块列取实体 ID：getEntityIdsInChunkColumn
// ============================================================================
TEST_F(EntitySpatialIndexTest, GetEntityIdsInChunkColumn)
{
    // (0,0) 列放一个，(1,0) 列放一个（不同 chunk）
    auto inCol = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    inCol->setPosition(0.0f, 64.0f, 0.0f); // chunk (0,0)
    m_index.addEntity(*inCol);
    auto otherCol = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    otherCol->setPosition(32.0f, 64.0f, 0.0f); // chunk (2,0)
    m_index.addEntity(*otherCol);
    auto inColHigh = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    inColHigh->setPosition(0.0f, 200.0f, 0.0f); // chunk (0,0) 但高 section
    m_index.addEntity(*inColHigh);

    auto ids = m_index.getEntityIdsInChunkColumn(0, 0);
    EXPECT_EQ(ids.size(), 2u); // inCol + inColHigh

    // 确认 ID 与预期实体一致（顺序无关）
    std::unordered_set<EntityInstanceId> idSet(ids.begin(), ids.end());
    EXPECT_TRUE(idSet.count(inCol->id()) > 0);
    EXPECT_TRUE(idSet.count(inColHigh->id()) > 0);
    EXPECT_TRUE(idSet.count(otherCol->id()) == 0);

    // 空列返回空
    EXPECT_TRUE(m_index.getEntityIdsInChunkColumn(5, 5).empty());
}

// ============================================================================
// 1000 实体规模：相对全扫 oracle 校验 AABB 查询正确性
// ============================================================================
TEST_F(EntitySpatialIndexTest, LargeScaleAABBMatchesOracle)
{
    // 用固定模式生成可复现的坐标（不允许 Math.random，用确定性公式）
    std::vector<std::unique_ptr<TestEntity>> entities;
    for (int i = 0; i < 1000; ++i) {
        auto e = std::make_unique<TestEntity>(
            mc::test::testEcsRegistry(), (i % 2 == 0) ? EntityTypeKeys::PIG : EntityTypeKeys::COW);
        // 坐标分布在 0..255 区间，负坐标边界也覆盖一部分
        const f32 x = static_cast<f32>((i * 37) % 256) - 8.0f;
        const f32 y = static_cast<f32>(64 + ((i * 13) % 64));
        const f32 z = static_cast<f32>((i * 71) % 256) - 8.0f;
        e->setPosition(x, y, z);
        m_index.addEntity(*e);
        entities.push_back(std::move(e));
    }
    EXPECT_EQ(m_index.entityCount(), 1000u);

    // 多个查询盒，逐个与 oracle 对比
    const AxisAlignedBB boxes[] = {
        AxisAlignedBB(0.0f, 60.0f, 0.0f, 50.0f, 100.0f, 50.0f),
        AxisAlignedBB(-10.0f, 64.0f, -10.0f, 10.0f, 80.0f, 10.0f),
        AxisAlignedBB(100.0f, 64.0f, 100.0f, 200.0f, 130.0f, 200.0f),
        AxisAlignedBB(-1000.0f, -1000.0f, -1000.0f, 1000.0f, 1000.0f, 1000.0f), // 全覆盖
        AxisAlignedBB(500.0f, 500.0f, 500.0f, 600.0f, 600.0f, 600.0f),          // 无命中
    };

    for (const AxisAlignedBB& box : boxes) {
        std::vector<Entity*> fromIndex;
        m_index.collectEntitiesInAABB(box, nullptr, [&](Entity& e) -> bool {
            fromIndex.push_back(&e);
            return true;
        });
        std::vector<Entity*> fromOracle = oracleInAABB(entities, box, nullptr);

        // 集合相等（顺序无关）
        std::unordered_set<Entity*> idxSet(fromIndex.begin(), fromIndex.end());
        std::unordered_set<Entity*> orcSet(fromOracle.begin(), fromOracle.end());
        EXPECT_EQ(idxSet, orcSet) << "box (" << box.minX << "," << box.minY << "," << box.minZ << ")-(" << box.maxX
                                  << "," << box.maxY << "," << box.maxZ << ") index size=" << fromIndex.size()
                                  << " oracle size=" << fromOracle.size();
    }
}

// ============================================================================
// 重复 remove 不崩溃（未登记实体直接 removeEntity）
// ============================================================================
TEST_F(EntitySpatialIndexTest, RemoveUnregisteredEntityIsSafe)
{
    auto pig = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    // 未 addEntity 直接 removeEntity：不应崩溃，计数保持 0
    EXPECT_NO_FATAL_FAILURE(m_index.removeEntity(*pig));
    EXPECT_EQ(m_index.entityCount(), 0u);

    // add 后双重 remove：第二次安全
    m_index.addEntity(*pig);
    m_index.removeEntity(*pig);
    EXPECT_NO_FATAL_FAILURE(m_index.removeEntity(*pig));
    EXPECT_EQ(m_index.entityCount(), 0u);
}

// ============================================================================
// 回调返回 false 中止遍历
// ============================================================================
TEST_F(EntitySpatialIndexTest, CallbackFalseStopsIteration)
{
    std::vector<std::unique_ptr<TestEntity>> entities;
    for (int i = 0; i < 10; ++i) {
        auto e = std::make_unique<TestEntity>(mc::test::testEcsRegistry(), EntityTypeKeys::PIG);
        e->setPosition(static_cast<f32>(i), 64.0f, 0.0f);
        m_index.addEntity(*e);
        entities.push_back(std::move(e));
    }

    // 大盒覆盖全部，但回调在第 3 个后返回 false
    int count = 0;
    AxisAlignedBB boxAll(-1.0f, 63.0f, -1.0f, 20.0f, 65.0f, 5.0f);
    m_index.collectEntitiesInAABB(boxAll, nullptr, [&](Entity&) -> bool {
        ++count;
        return count < 3; // 第 3 个返回 false 中止
    });
    EXPECT_EQ(count, 3);
}
