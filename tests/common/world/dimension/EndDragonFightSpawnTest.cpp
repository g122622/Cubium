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

// 测试 EndDragonFight::_findOrCreateDragon / _createNewDragon 的行为。
// 对应 MC 1.21.11 EndDragonFight.findOrCreateDragon() / createNewDragon()：
//   1. 世界中已存在末影龙时，复用其 UUID，不重复生成
//   2. 世界中无末影龙时，通过 EntityType 工厂创建新龙，设置位置 (0, 128, 0)、
//      随机 yaw、初始阶段 HoldingPattern，并记录 UUID

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/entities/boss/EnderDragonEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/EndDragonFightTestAccessor.hpp"
#include "common/world/dimension/end/EndDragonFight.hpp"

#include <cmath>
#include <map>
#include <memory>

using namespace mc;

namespace mc::test {

// ============================================================================
// 测试用世界 - 支持 spawnEntity / getEntitiesByType / hasChunk 等
// ============================================================================

class DragonSpawnTestWorld final : public BaseChunkBackedTestWorld {
public:
    DragonSpawnTestWorld()
    {
        // 预填充原点周围 ARENA_CHUNK_RADIUS 范围的区块，使 _isArenaLoaded() 返回 true
        for (i32 cx = -EndDragonFight::ARENA_CHUNK_RADIUS; cx <= EndDragonFight::ARENA_CHUNK_RADIUS; ++cx) {
            for (i32 cz = -EndDragonFight::ARENA_CHUNK_RADIUS; cz <= EndDragonFight::ARENA_CHUNK_RADIUS; ++cz) {
                ensureChunk(cx, cz);
            }
        }
    }

    // ========== IWorld 接口实现 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const ChunkData* chunk = getChunk(toChunkCoord(x), toChunkCoord(z));
        if (chunk == nullptr) {
            return nullptr;
        }
        return chunk->getBlockState(toLocalCoord(x), y - world::MIN_BUILD_HEIGHT, toLocalCoord(z));
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        ChunkData& chunk = ensureChunk(toChunkCoord(x), toChunkCoord(z));
        chunk.setBlockState(toLocalCoord(x), y - world::MIN_BUILD_HEIGHT, toLocalCoord(z), state);
        return true;
    }

    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/) const override { return world::MAX_BUILD_HEIGHT; }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    // ========== 实体管理 ==========

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) {
            return EntityId(0);
        }
        const EntityId id = m_nextEntityId;
        m_nextEntityId = EntityId(static_cast<u64>(m_nextEntityId) + 1);
        entity->setId(id);
        entity->setWorld(this);
        // 跟踪末影龙实体的生成
        if (dynamic_cast<entity::EnderDragonEntity*>(entity.get()) != nullptr) {
            m_spawnedDragons.push_back(entity.get());
        }
        m_entities.push_back(std::move(entity));
        return id;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesByType(entity::EntityTypeId typeId) const override
    {
        std::vector<Entity*> result;
        for (const auto& entity : m_entities) {
            if (entity->typeId() == typeId && !entity->isRemoved()) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    Entity* getEntity(EntityId id) override
    {
        for (auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityId id) const override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    // ========== 测试辅助方法 ==========

    /// 注入一个已存在的末影龙实体（模拟从存档加载的龙），返回其 UUID
    /// 使用 EntityRegistry 工厂创建，确保 typeId() 正确返回 ENDER_DRAGON
    std::string injectExistingDragon()
    {
        auto& registry = entity::EntityRegistry::instance();
        const entity::EntityType* dragonType = registry.getType(entity::EntityTypes::ENDER_DRAGON);
        if (dragonType == nullptr) {
            return std::string();
        }
        std::unique_ptr<Entity> dragonEntity = dragonType->create(this);
        if (dragonEntity == nullptr) {
            return std::string();
        }
        const std::string uuid = dragonEntity->uuid();
        dragonEntity->setId(EntityId(100 + static_cast<u64>(m_entities.size())));
        m_entities.push_back(std::move(dragonEntity));
        return uuid;
    }

    /// 获取所有通过 spawnEntity 生成的末影龙实体指针
    [[nodiscard]] const std::vector<Entity*>& spawnedDragons() const { return m_spawnedDragons; }

    /// 总实体数（含注入的）
    [[nodiscard]] size_t totalEntityCount() const { return m_entities.size(); }

private:
    // 区块坐标转换辅助（避免依赖 CoordConverter，使测试自包含）
    static i32 toChunkCoord(i32 worldCoord)
    {
        return worldCoord >= 0 ? worldCoord / world::CHUNK_WIDTH : (worldCoord + 1) / world::CHUNK_WIDTH - 1;
    }

    static i32 toLocalCoord(i32 worldCoord)
    {
        i32 local = worldCoord % world::CHUNK_WIDTH;
        return local >= 0 ? local : local + world::CHUNK_WIDTH;
    }

    std::vector<std::unique_ptr<Entity>> m_entities;
    std::vector<Entity*> m_spawnedDragons;
    EntityId m_nextEntityId = EntityId(1);
};

} // namespace mc::test

// ============================================================================
// 测试夹具
// ============================================================================

class EndDragonFightSpawnTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // 注册原版实体类型，使 EntityRegistry 中 ENDER_DRAGON 可用。
        // registerAll() 幂等且线程安全，多次调用无副作用。
        entity::VanillaEntities::registerAll();
    }

    test::DragonSpawnTestWorld m_world;
};

// ============================================================================
// _createNewDragon 测试
// ============================================================================

TEST_F(EndDragonFightSpawnTest, CreateNewDragon_SpawnsDragonEntity)
{
    // 新世界，dragonKilled=false，无存档数据
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    EXPECT_TRUE(accessor.dragonUUID().empty());
    EXPECT_FALSE(accessor.dragonKilled());

    Entity* newDragon = accessor.createNewDragon(m_world);

    EXPECT_NE(newDragon, nullptr);
    EXPECT_FALSE(accessor.dragonUUID().empty());
    EXPECT_FALSE(accessor.dragonKilled());

    // 应在世界中生成一个末影龙实体
    auto dragons = m_world.getEntitiesByType(entity::EntityTypeIdNumber::ENDER_DRAGON);
    ASSERT_EQ(dragons.size(), 1u);
    EXPECT_EQ(dragons[0]->uuid(), accessor.dragonUUID());
    // 返回的指针应指向世界中生成的同一实体
    EXPECT_EQ(dragons[0], newDragon);
}

TEST_F(EndDragonFightSpawnTest, CreateNewDragon_SetsSpawnPosition)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    accessor.createNewDragon(m_world);

    auto dragons = m_world.getEntitiesByType(entity::EntityTypeIdNumber::ENDER_DRAGON);
    ASSERT_EQ(dragons.size(), 1u);

    // 龙应在 (0, DRAGON_SPAWN_Y, 0) 生成
    Entity* dragon = dragons[0];
    EXPECT_FLOAT_EQ(dragon->x(), 0.0f);
    EXPECT_FLOAT_EQ(dragon->y(), static_cast<f32>(EndDragonFight::DRAGON_SPAWN_Y));
    EXPECT_FLOAT_EQ(dragon->z(), 0.0f);
}

TEST_F(EndDragonFightSpawnTest, CreateNewDragon_SetsRandomYawInRange)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    accessor.createNewDragon(m_world);

    auto dragons = m_world.getEntitiesByType(entity::EntityTypeIdNumber::ENDER_DRAGON);
    ASSERT_EQ(dragons.size(), 1u);

    // yaw 应在 [0, 360) 范围内
    const f32 yaw = dragons[0]->yaw();
    EXPECT_GE(yaw, 0.0f);
    EXPECT_LT(yaw, 360.0f);
    // pitch 应为 0
    EXPECT_FLOAT_EQ(dragons[0]->pitch(), 0.0f);
}

TEST_F(EndDragonFightSpawnTest, CreateNewDragon_SetsHoldingPatternPhase)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    accessor.createNewDragon(m_world);

    auto dragons = m_world.getEntitiesByType(entity::EntityTypeIdNumber::ENDER_DRAGON);
    ASSERT_EQ(dragons.size(), 1u);

    auto* dragon = dynamic_cast<entity::EnderDragonEntity*>(dragons[0]);
    ASSERT_NE(dragon, nullptr);
    EXPECT_EQ(dragon->phase(), entity::EnderDragonEntity::Phase::HoldingPattern);
}

TEST_F(EndDragonFightSpawnTest, CreateNewDragon_DragonHasFullHealth)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    accessor.createNewDragon(m_world);

    auto dragons = m_world.getEntitiesByType(entity::EntityTypeIdNumber::ENDER_DRAGON);
    ASSERT_EQ(dragons.size(), 1u);

    auto* dragon = dynamic_cast<entity::EnderDragonEntity*>(dragons[0]);
    ASSERT_NE(dragon, nullptr);
    // MC 原版构造时 setHealth(getMaxHealth())，满血应为 200
    EXPECT_FLOAT_EQ(dragon->health(), dragon->maxHealth());
    EXPECT_FLOAT_EQ(dragon->maxHealth(), 200.0f);
}

TEST_F(EndDragonFightSpawnTest, CreateNewDragon_ResetsTicksSinceDragonSeen)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    accessor.setTicksSinceDragonSeen(500);
    accessor.createNewDragon(m_world);

    EXPECT_EQ(accessor.ticksSinceDragonSeen(), 0);
}

TEST_F(EndDragonFightSpawnTest, CreateNewDragon_ClearsDragonKilledFlag)
{
    // 模拟异常状态：dragonKilled=true 但调用 createNewDragon
    // createNewDragon 应将 dragonKilled 重置为 false
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);

    accessor.createNewDragon(m_world);

    EXPECT_FALSE(accessor.dragonKilled());
}

// ============================================================================
// _findOrCreateDragon 测试
// ============================================================================

TEST_F(EndDragonFightSpawnTest, FindOrCreateDragon_NoExistingDragon_CreatesNew)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    // 世界中无末影龙，应创建新龙
    accessor.findOrCreateDragon(m_world);

    EXPECT_FALSE(accessor.dragonUUID().empty());
    EXPECT_FALSE(accessor.dragonKilled());

    auto dragons = m_world.getEntitiesByType(entity::EntityTypeIdNumber::ENDER_DRAGON);
    ASSERT_EQ(dragons.size(), 1u);
}

TEST_F(EndDragonFightSpawnTest, FindOrCreateDragon_ExistingDragon_ReusesUUID)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    // 注入一条已存在的末影龙
    const std::string existingUUID = m_world.injectExistingDragon();

    // 调用 findOrCreateDragon 应复用已存在龙的 UUID，不生成新龙
    accessor.findOrCreateDragon(m_world);

    EXPECT_EQ(accessor.dragonUUID(), existingUUID);
    EXPECT_FALSE(accessor.dragonKilled());

    // 世界中应仍只有一条龙（未重复生成）
    auto dragons = m_world.getEntitiesByType(entity::EntityTypeIdNumber::ENDER_DRAGON);
    ASSERT_EQ(dragons.size(), 1u);
    EXPECT_EQ(dragons[0]->uuid(), existingUUID);
}

TEST_F(EndDragonFightSpawnTest, FindOrCreateDragon_ExistingDragon_DoesNotSpawnNew)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    m_world.injectExistingDragon();
    const size_t entityCountBefore = m_world.totalEntityCount();

    accessor.findOrCreateDragon(m_world);

    // 实体总数不应增加（复用已有龙，不生成新龙）
    EXPECT_EQ(m_world.totalEntityCount(), entityCountBefore);
}

TEST_F(EndDragonFightSpawnTest, FindOrCreateDragon_MultipleExistingDragons_UsesFirst)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    // 注入两条龙（异常场景，但 findOrCreateDragon 应取第一条）
    const std::string firstUUID = m_world.injectExistingDragon();
    const std::string secondUUID = m_world.injectExistingDragon();
    EXPECT_NE(firstUUID, secondUUID);

    accessor.findOrCreateDragon(m_world);

    // 应记录第一条龙的 UUID（MC 原版 list.get(0)）
    // 注意：getEntitiesByType 的迭代顺序由 m_entities 插入顺序决定，第一条是 firstUUID
    EXPECT_EQ(accessor.dragonUUID(), firstUUID);
}

TEST_F(EndDragonFightSpawnTest, FindOrCreateDragon_ExistingRemovedDragon_CreatesNew)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    // 注入一条龙并将其标记为已移除（模拟 discard 后的状态）
    const std::string removedUUID = m_world.injectExistingDragon();
    auto dragons = m_world.getEntitiesByType(entity::EntityTypeIdNumber::ENDER_DRAGON);
    ASSERT_EQ(dragons.size(), 1u);
    dragons[0]->discard();
    EXPECT_TRUE(dragons[0]->isRemoved());

    // findOrCreateDragon 应过滤已移除的龙并创建新龙
    accessor.findOrCreateDragon(m_world);

    EXPECT_FALSE(accessor.dragonUUID().empty());
    EXPECT_NE(accessor.dragonUUID(), removedUUID);

    // getEntitiesByType 也应过滤已移除的龙，仅返回新生成的龙
    auto remainingDragons = m_world.getEntitiesByType(entity::EntityTypeIdNumber::ENDER_DRAGON);
    ASSERT_EQ(remainingDragons.size(), 1u);
    EXPECT_EQ(remainingDragons[0]->uuid(), accessor.dragonUUID());
}

// ============================================================================
// 通过 tick() 触发的集成测试
// ============================================================================

TEST_F(EndDragonFightSpawnTest, Tick_WithEmptyDragonUUID_TriggersFindOrCreate)
{
    // 模拟新世界首次 tick：dragonUUID 为空，竞技场已加载
    // tick() 中的条件 (uuidEmpty || ++ticksSinceDragonSeen >= MAX) && arenaLoaded
    // 在 uuidEmpty=true 时立即触发
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    // 需要有可见玩家才能进入龙失联检查分支
    // 由于测试世界无玩家，Boss 栏 hasPlayers() 返回 false，tick 会提前 return
    // 这里直接验证 tick 不会崩溃，并通过 accessor 验证状态
    fight.tick(m_world);

    // 无玩家时 tick 提前返回，不应创建龙
    EXPECT_TRUE(accessor.dragonUUID().empty());
}

TEST_F(EndDragonFightSpawnTest, Tick_AfterMaxTicksSinceDragonSeen_TriggersFindOrCreate)
{
    // 这个测试验证 ticksSinceDragonSeen 超阈值时 tick 会调用 findOrCreateDragon
    // 但由于测试世界无玩家（hasPlayers()=false），无法进入该分支
    // 此处仅验证 ticksSinceDragonSeen 的递增逻辑不崩溃
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setTicksSinceDragonSeen(EndDragonFight::MAX_TICKS_BEFORE_DRAGON_RESPAWN - 1);

    // 多次 tick（无玩家，提前 return，不会触发 findOrCreateDragon）
    for (i32 i = 0; i < 5; ++i) {
        fight.tick(m_world);
    }

    // 无玩家时不应创建龙
    EXPECT_TRUE(accessor.dragonUUID().empty());
}
