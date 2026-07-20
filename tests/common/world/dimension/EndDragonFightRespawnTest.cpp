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

// 测试末影龙重生序列（DragonRespawnAnimation 5 阶段 + tryRespawn + onCrystalDestroyed）。
// 对应 MC 1.21.11 EndDragonFight 的重生相关逻辑：
//   1. tryRespawn 检测出口传送门四周 4 个末影水晶
//   2. _respawnDragon 启动 START 阶段
//   3. 各阶段 tick 函数的状态机推进
//   4. onCrystalDestroyed 在重生序列中触发中止
//   5. resetSpikeCrystals 清除柱顶水晶无敌与光束
//   6. _updateCrystalCount 统计柱顶水晶数量
//   7. 存档序列化 / 反序列化 isRespawning 标志

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/EndDragonFightTestAccessor.hpp"
#include "common/world/dimension/end/DragonRespawnAnimation.hpp"
#include "common/world/dimension/end/EndDragonFight.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/gen/feature/end/EndSpikeFeature.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

using namespace mc;

namespace mc::test {

// ============================================================================
// 测试用世界 - 支持 setBlockState / getEntitiesInAABB / spawnEntity / createExplosion
// ============================================================================

class DragonRespawnTestWorld final : public BaseChunkBackedTestWorld {
public:
    DragonRespawnTestWorld()
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

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override { m_events.push_back({eventId, pos, data}); }

    // ========== 实体管理 ==========

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) {
            return EntityInstanceId(0);
        }
        const EntityInstanceId id = m_nextEntityId;
        m_nextEntityId = EntityInstanceId(static_cast<u64>(m_nextEntityId) + 1);
        entity->setId(id);
        entity->setWorld(this);
        m_entities.push_back(std::move(entity));
        return id;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesByType(const std::string& typeId) const override
    {
        std::vector<Entity*> result;
        for (const auto& entity : m_entities) {
            if (entity->getTypeId() == typeId && !entity->isRemoved()) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity*) const override
    {
        std::vector<Entity*> result;
        for (const auto& entity : m_entities) {
            if (entity->isRemoved()) {
                continue;
            }
            // 简化：使用实体位置点是否在 AABB 内
            const Vector3 pos(entity->x(), entity->y(), entity->z());
            if (pos.x >= box.minX && pos.x <= box.maxX && pos.y >= box.minY && pos.y <= box.maxY && pos.z >= box.minZ &&
                pos.z <= box.maxZ) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    Entity* getEntity(EntityInstanceId id) override
    {
        for (auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    void createExplosion(const Vector3& position,
        f32 /*radius*/,
        world::explosion::ExplosionMode /*mode*/,
        bool /*causesFire*/,
        Entity* /*source*/) override
    {
        m_explosions.push_back(position);
    }

    // ========== 进度触发回调 ==========

    void onSummonedEntity(PlayerId playerId, Entity* entity) override
    {
        // 记录调用以供测试断言 SUMMONED_ENTITY 进度触发逻辑
        m_summonedEntityCalls.push_back({playerId, entity});
    }

    // ========== dragonFight 注入 ==========

    [[nodiscard]] EndDragonFight* dragonFight() override { return m_fight; }
    [[nodiscard]] const EndDragonFight* dragonFight() const override { return m_fight; }

    void setDragonFight(EndDragonFight* fight) { m_fight = fight; }

    // ========== 测试辅助 ==========

    /// 创建并放入世界一个末影水晶实体，位于指定方块坐标
    entity::EnderCrystalEntity* spawnCrystalAt(i32 x, i32 y, i32 z)
    {
        auto& registry = entity::EntityRegistry::instance();
        const entity::EntityType* type = registry.getType(entity::EntityTypeKeys::END_CRYSTAL);
        if (type == nullptr) {
            return nullptr;
        }
        std::unique_ptr<Entity> ent = type->create(this);
        if (ent == nullptr) {
            return nullptr;
        }
        ent->setPosition(Vector3(static_cast<f32>(x) + 0.5f, static_cast<f32>(y), static_cast<f32>(z) + 0.5f));
        auto* crystal = dynamic_cast<entity::EnderCrystalEntity*>(ent.get());
        spawnEntity(std::move(ent));
        return crystal;
    }

    [[nodiscard]] const std::vector<Vector3>& explosions() const { return m_explosions; }

    [[nodiscard]] bool playedGrowlSound() const
    {
        return std::any_of(m_events.begin(), m_events.end(), [](const PlayEventCall& e) {
            return e.eventId == world::WorldEvents::ENDERMAN_GROWL_SOUND;
        });
    }

    [[nodiscard]] size_t explosionCount() const { return m_explosions.size(); }

    /// 获取 onSummonedEntity 调用记录（playerId, entity 指针）
    [[nodiscard]] const std::vector<std::pair<PlayerId, Entity*>>& summonedEntityCalls() const
    {
        return m_summonedEntityCalls;
    }

    void clearEvents()
    {
        m_events.clear();
        m_summonedEntityCalls.clear();
    }

private:
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
    EntityInstanceId m_nextEntityId = EntityInstanceId(1);
    EndDragonFight* m_fight = nullptr;

    struct PlayEventCall {
        i32 eventId;
        BlockPos pos;
        i32 data;
    };
    std::vector<PlayEventCall> m_events;
    std::vector<Vector3> m_explosions;
    std::vector<std::pair<PlayerId, Entity*>> m_summonedEntityCalls;
};

} // namespace mc::test

// ============================================================================
// 测试夹具
// ============================================================================

class EndDragonFightRespawnTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }

    test::DragonRespawnTestWorld m_world;
};

// ============================================================================
// tryRespawn 测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, TryRespawn_NoCrystals_DoesNothing)
{
    // 龙已死但未放置水晶：不应启动重生
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    data.dragonKilled = true;
    data.previouslyKilled = true;
    EndDragonFight fight(42, data);

    fight.tryRespawn(m_world);

    EXPECT_FALSE(fight.isRespawning());
    EXPECT_FALSE(fight.respawnStage().has_value());
}

TEST_F(EndDragonFightRespawnTest, TryRespawn_DragonAlive_DoesNothing)
{
    // 龙还活着：不应触发重生
    EndDragonFight fight(42, std::nullopt);
    // dragonKilled=false（默认）

    // 即使放了 4 个水晶也不应触发
    const BlockPos portalLoc(0, 0, 0);
    const BlockPos portalAbove = portalLoc.up(1);
    for (Direction dir : Directions::horizontal()) {
        const BlockPos pos = portalAbove.offset(dir, 2);
        m_world.spawnCrystalAt(pos.x, pos.y, pos.z);
    }

    fight.tryRespawn(m_world);
    EXPECT_FALSE(fight.isRespawning());
}

TEST_F(EndDragonFightRespawnTest, TryRespawn_FourCrystals_StartsRespawn)
{
    // 龙已死 + 4 个水晶就位：应启动重生序列
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    data.dragonKilled = true;
    data.previouslyKilled = true;
    EndDragonFight fight(42, data);
    m_world.setDragonFight(&fight);

    // 在出口传送门四周 2 格外放置 4 个末影水晶
    const BlockPos portalLoc(0, 0, 0);
    const BlockPos portalAbove = portalLoc.up(1);
    for (Direction dir : Directions::horizontal()) {
        const BlockPos pos = portalAbove.offset(dir, 2);
        m_world.spawnCrystalAt(pos.x, pos.y, pos.z);
    }

    fight.tryRespawn(m_world);

    EXPECT_TRUE(fight.isRespawning());
    ASSERT_TRUE(fight.respawnStage().has_value());
    EXPECT_EQ(*fight.respawnStage(), DragonRespawnAnimation::START);
}

TEST_F(EndDragonFightRespawnTest, TryRespawn_OnlyThreeCrystals_DoesNothing)
{
    // 龙已死 + 3 个水晶（缺少 1 个）：不应启动
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    data.dragonKilled = true;
    data.previouslyKilled = true;
    EndDragonFight fight(42, data);
    m_world.setDragonFight(&fight);

    const BlockPos portalLoc(0, 0, 0);
    const BlockPos portalAbove = portalLoc.up(1);
    const auto dirs = Directions::horizontal();
    // 仅放 3 个水晶
    for (size_t i = 0; i < 3; ++i) {
        const BlockPos pos = portalAbove.offset(dirs[i], 2);
        m_world.spawnCrystalAt(pos.x, pos.y, pos.z);
    }

    fight.tryRespawn(m_world);

    EXPECT_FALSE(fight.isRespawning());
}

TEST_F(EndDragonFightRespawnTest, TryRespawn_AlreadyRespawning_DoesNothing)
{
    // 已在重生中：即使再次调用 tryRespawn 也不应重置
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    data.dragonKilled = true;
    data.previouslyKilled = true;
    EndDragonFight fight(42, data);
    m_world.setDragonFight(&fight);

    // 手动设置重生阶段
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setRespawnStage(DragonRespawnAnimation::PREPARING_TO_SUMMON_PILLARS);
    accessor.setRespawnTime(50);

    // 放 4 个水晶
    const BlockPos portalLoc(0, 0, 0);
    const BlockPos portalAbove = portalLoc.up(1);
    for (Direction dir : Directions::horizontal()) {
        const BlockPos pos = portalAbove.offset(dir, 2);
        m_world.spawnCrystalAt(pos.x, pos.y, pos.z);
    }

    fight.tryRespawn(m_world);

    // 阶段不应被重置
    ASSERT_TRUE(fight.respawnStage().has_value());
    EXPECT_EQ(*fight.respawnStage(), DragonRespawnAnimation::PREPARING_TO_SUMMON_PILLARS);
    EXPECT_EQ(accessor.respawnTime(), 50);
}

// ============================================================================
// _respawnDragon 测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, RespawnDragon_SetsStartStage)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);

    std::vector<entity::EnderCrystalEntity*> crystals;
    for (i32 i = 0; i < 4; ++i) {
        crystals.push_back(m_world.spawnCrystalAt(i, 0, 0));
    }

    accessor.respawnDragon(m_world, crystals);

    ASSERT_TRUE(fight.respawnStage().has_value());
    EXPECT_EQ(*fight.respawnStage(), DragonRespawnAnimation::START);
    EXPECT_EQ(accessor.respawnTime(), 0);
    EXPECT_EQ(accessor.respawnCrystals().size(), 4u);
}

TEST_F(EndDragonFightRespawnTest, RespawnDragon_DragonAlive_DoesNothing)
{
    // 龙还活着时调用 _respawnDragon 应被拒绝
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    // dragonKilled=false（默认）

    std::vector<entity::EnderCrystalEntity*> crystals;
    crystals.push_back(m_world.spawnCrystalAt(0, 0, 0));

    accessor.respawnDragon(m_world, crystals);

    EXPECT_FALSE(fight.respawnStage().has_value());
    EXPECT_TRUE(accessor.respawnCrystals().empty());
}

// ============================================================================
// START 阶段 tick 测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, TickStart_SetsBeamTargetAndAdvancesToPreparing)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    // 准备 4 个水晶
    std::vector<entity::EnderCrystalEntity*> crystals;
    for (i32 i = 0; i < 4; ++i) {
        crystals.push_back(m_world.spawnCrystalAt(i, 0, 0));
    }
    accessor.respawnDragon(m_world, crystals);

    // 直接调用 tickStart（跳过 hasPlayers 早返回）
    dragon_respawn::tickStart(m_world, fight, crystals, 0, BlockPos(0, 0, 0));

    // START 应立即切换到 PREPARING_TO_SUMMON_PILLARS
    ASSERT_TRUE(fight.respawnStage().has_value());
    EXPECT_EQ(*fight.respawnStage(), DragonRespawnAnimation::PREPARING_TO_SUMMON_PILLARS);
}

// ============================================================================
// PREPARING_TO_SUMMON_PILLARS 阶段 tick 测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, TickPreparing_PlaysGrowlSoundAtTick0)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    std::vector<entity::EnderCrystalEntity*> crystals;
    accessor.setRespawnStage(DragonRespawnAnimation::PREPARING_TO_SUMMON_PILLARS);

    // time=0 时应播放音效 3001
    dragon_respawn::tickPreparingToSummonPillars(m_world, fight, crystals, 0, BlockPos(0, 0, 0));
    EXPECT_TRUE(m_world.playedGrowlSound());
}

TEST_F(EndDragonFightRespawnTest, TickPreparing_NoSoundAtTick10)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    std::vector<entity::EnderCrystalEntity*> crystals;
    accessor.setRespawnStage(DragonRespawnAnimation::PREPARING_TO_SUMMON_PILLARS);
    m_world.clearEvents();

    // time=10 不在音效触发点（0/50/51/52/95+）内
    dragon_respawn::tickPreparingToSummonPillars(m_world, fight, crystals, 10, BlockPos(0, 0, 0));
    EXPECT_FALSE(m_world.playedGrowlSound());
}

TEST_F(EndDragonFightRespawnTest, TickPreparing_AtTime100_AdvancesToSummoningPillars)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    std::vector<entity::EnderCrystalEntity*> crystals;
    accessor.setRespawnStage(DragonRespawnAnimation::PREPARING_TO_SUMMON_PILLARS);

    // time=100 应切换到 SUMMONING_PILLARS
    dragon_respawn::tickPreparingToSummonPillars(m_world, fight, crystals, 100, BlockPos(0, 0, 0));
    ASSERT_TRUE(fight.respawnStage().has_value());
    EXPECT_EQ(*fight.respawnStage(), DragonRespawnAnimation::SUMMONING_PILLARS);
}

// ============================================================================
// SUMMONING_DRAGON 阶段 tick 测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, TickSummoningDragon_AtTime100_AdvancesToEndAndCreatesDragon)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    // 放置 4 个重生水晶
    std::vector<entity::EnderCrystalEntity*> crystals;
    for (i32 i = 0; i < 4; ++i) {
        crystals.push_back(m_world.spawnCrystalAt(i, 0, 0));
    }
    accessor.respawnDragon(m_world, crystals);

    // 切换到 SUMMONING_DRAGON 阶段
    accessor.setRespawnStage(DragonRespawnAnimation::SUMMONING_DRAGON);

    // time=100 应触发 END 切换、爆炸水晶、创建新龙
    dragon_respawn::tickSummoningDragon(m_world, fight, crystals, 100, BlockPos(0, 0, 0));

    // 重生应结束
    EXPECT_FALSE(fight.isRespawning());
    EXPECT_FALSE(fight.respawnStage().has_value());
    // 应创建新龙
    auto dragons = m_world.getEntitiesByType(entity::EntityTypeKeys::ENDER_DRAGON);
    EXPECT_EQ(dragons.size(), 1u);
    // 应触发爆炸（4 个水晶各爆炸一次）
    EXPECT_GE(m_world.explosionCount(), 4u);
    // 水晶应被 discard
    for (auto* crystal : crystals) {
        EXPECT_TRUE(crystal->isRemoved());
    }
}

TEST_F(EndDragonFightRespawnTest, TickSummoningDragon_AtTime0_SetsBeamTarget)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    std::vector<entity::EnderCrystalEntity*> crystals;
    crystals.push_back(m_world.spawnCrystalAt(0, 0, 0));
    accessor.setRespawnStage(DragonRespawnAnimation::SUMMONING_DRAGON);

    dragon_respawn::tickSummoningDragon(m_world, fight, crystals, 0, BlockPos(0, 0, 0));

    // time=0 应设置光束目标到 (0, 128, 0)
    ASSERT_FALSE(crystals.empty());
    EXPECT_TRUE(crystals[0]->hasBeamTarget());
}

TEST_F(EndDragonFightRespawnTest, TickSummoningDragon_AtTime3_PlaysGrowlSound)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    std::vector<entity::EnderCrystalEntity*> crystals;
    accessor.setRespawnStage(DragonRespawnAnimation::SUMMONING_DRAGON);
    m_world.clearEvents();

    // time<5 应播放音效
    dragon_respawn::tickSummoningDragon(m_world, fight, crystals, 3, BlockPos(0, 0, 0));
    EXPECT_TRUE(m_world.playedGrowlSound());
}

// ============================================================================
// onCrystalDestroyed 测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, OnCrystalDestroyed_DuringRespawn_AbortsRespawn)
{
    // 重生序列进行中，某个重生水晶被破坏：应中止重生
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    std::vector<entity::EnderCrystalEntity*> crystals;
    for (i32 i = 0; i < 4; ++i) {
        crystals.push_back(m_world.spawnCrystalAt(i, 0, 0));
    }
    accessor.respawnDragon(m_world, crystals);

    ASSERT_TRUE(fight.isRespawning());

    // 模拟第一个水晶被破坏
    auto dmgSource = DamageSources::generic();
    fight.onCrystalDestroyed(m_world, crystals[0], dmgSource);

    // 重生应被中止
    EXPECT_FALSE(fight.isRespawning());
    EXPECT_TRUE(accessor.respawnCrystals().empty());
}

TEST_F(EndDragonFightRespawnTest, OnCrystalDestroyed_NotRespawnCrystal_UpdatesCount)
{
    // 非重生阶段：破坏柱顶水晶应更新 crystalsAlive 计数
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    // 放置一个柱顶水晶（使用第一根柱子的位置）
    const auto spikes = EndSpikeFeatureConfig::generateSpikes(42);
    ASSERT_FALSE(spikes.empty());
    const auto& spike = spikes[0];
    auto* crystal = m_world.spawnCrystalAt(spike.centerX, spike.height + 1, spike.centerZ);
    ASSERT_NE(crystal, nullptr);

    // 更新水晶计数 - 应为 1
    accessor.updateCrystalCount(m_world);
    EXPECT_EQ(accessor.crystalsAlive(), 1);

    // 先移除水晶（模拟被破坏），再调用 onCrystalDestroyed
    crystal->discard();
    auto dmgSource = DamageSources::generic();
    fight.onCrystalDestroyed(m_world, crystal, dmgSource);

    // 再次更新计数应为 0（水晶已被移除）
    accessor.updateCrystalCount(m_world);
    EXPECT_EQ(accessor.crystalsAlive(), 0);
}

// ============================================================================
// resetSpikeCrystals 测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, ResetSpikeCrystals_ClearsInvulnerableAndBeam)
{
    EndDragonFight fight(42, std::nullopt);
    m_world.setDragonFight(&fight);

    // 放置一个柱顶水晶并设置为无敌 + 光束
    const auto spikes = EndSpikeFeatureConfig::generateSpikes(42);
    ASSERT_FALSE(spikes.empty());
    const auto& spike = spikes[0];
    auto* crystal = m_world.spawnCrystalAt(spike.centerX, spike.height + 1, spike.centerZ);
    ASSERT_NE(crystal, nullptr);
    crystal->setInvulnerable(true);
    crystal->setBeamTarget(BlockPos(0, 128, 0));

    fight.resetSpikeCrystals(m_world);

    EXPECT_FALSE(crystal->isInvulnerable());
    EXPECT_FALSE(crystal->hasBeamTarget());
}

// ============================================================================
// _updateCrystalCount 测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, UpdateCrystalCount_CountsAllSpikeCrystals)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    m_world.setDragonFight(&fight);

    // 在每根柱顶放置水晶
    const auto spikes = EndSpikeFeatureConfig::generateSpikes(42);
    for (const auto& spike : spikes) {
        m_world.spawnCrystalAt(spike.centerX, spike.height + 1, spike.centerZ);
    }

    accessor.updateCrystalCount(m_world);
    EXPECT_EQ(accessor.crystalsAlive(), static_cast<i32>(spikes.size()));
}

TEST_F(EndDragonFightRespawnTest, UpdateCrystalCount_NoCrystals_ReturnsZero)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);

    accessor.updateCrystalCount(m_world);
    EXPECT_EQ(accessor.crystalsAlive(), 0);
}

// ============================================================================
// 序列化测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, Serialization_PreservesIsRespawningFlag)
{
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    data.dragonKilled = true;
    data.previouslyKilled = true;
    data.isRespawning = true;
    EndDragonFight fight(42, data);

    // 保存
    const auto saved = fight.saveData();
    EXPECT_TRUE(saved.isRespawning);

    // 从 JSON 反序列化
    const nlohmann::json json = saved.toJson();
    const auto restored = EndDragonFight::Data::fromJson(json);
    EXPECT_TRUE(restored.isRespawning);
}

TEST_F(EndDragonFightRespawnTest, Serialization_PreservesExitPortalLocation)
{
    EndDragonFight::Data data;
    data.needsStateScanning = false;
    data.dragonKilled = true;
    data.previouslyKilled = true;
    data.exitPortalLocation = BlockPos(10, 64, -20);
    EndDragonFight fight(42, data);

    const auto saved = fight.saveData();
    ASSERT_TRUE(saved.exitPortalLocation.has_value());
    EXPECT_EQ(saved.exitPortalLocation->x, 10);
    EXPECT_EQ(saved.exitPortalLocation->y, 64);
    EXPECT_EQ(saved.exitPortalLocation->z, -20);
}

TEST_F(EndDragonFightRespawnTest, Serialization_DefaultIsRespawningFalse)
{
    EndDragonFight fight(42, std::nullopt);
    const auto saved = fight.saveData();
    EXPECT_FALSE(saved.isRespawning);
    EXPECT_FALSE(saved.exitPortalLocation.has_value());
}

// ============================================================================
// isRespawning / respawnStage 查询测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, IsRespawning_InitiallyFalse)
{
    EndDragonFight fight(42, std::nullopt);
    EXPECT_FALSE(fight.isRespawning());
    EXPECT_FALSE(fight.respawnStage().has_value());
}

TEST_F(EndDragonFightRespawnTest, IsRespawning_TrueAfterRespawnDragon)
{
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);

    std::vector<entity::EnderCrystalEntity*> crystals;
    crystals.push_back(m_world.spawnCrystalAt(0, 0, 0));
    accessor.respawnDragon(m_world, crystals);

    EXPECT_TRUE(fight.isRespawning());
    ASSERT_TRUE(fight.respawnStage().has_value());
    EXPECT_EQ(*fight.respawnStage(), DragonRespawnAnimation::START);
}

// ============================================================================
// setRespawnStage 测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, SetRespawnStage_End_ThrowsOrCleared)
{
    // 调用 setRespawnStage(END) 应清除重生状态并创建新龙
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    accessor.setRespawnStage(DragonRespawnAnimation::START);
    accessor.setRespawnTime(100);

    fight.setRespawnStage(m_world, DragonRespawnAnimation::END);

    EXPECT_FALSE(fight.isRespawning());
    EXPECT_FALSE(fight.respawnStage().has_value());
    EXPECT_FALSE(fight.isDragonKilled());
    // 新龙应被创建
    auto dragons = m_world.getEntitiesByType(entity::EntityTypeKeys::ENDER_DRAGON);
    EXPECT_EQ(dragons.size(), 1u);
}

TEST_F(EndDragonFightRespawnTest, SetRespawnStage_End_FiresSummonedEntityForEachBossBarPlayer)
{
    // 验证 setRespawnStage(END) 创建新龙后，对 Boss 栏可见玩家列表中的每个玩家
    // 触发一次 onSummonedEntity 回调（对应 MC Java CriteriaTriggers.SUMMONED_ENTITY）。
    // 对应 MC Java: EndDragonFight.setRespawnStage(END) 中
    //   for (ServerPlayer serverplayer : this.dragonEvent.getPlayers()) {
    //       CriteriaTriggers.SUMMONED_ENTITY.trigger(serverplayer, enderdragon);
    //   }
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    // 注入一个可编程的 Boss 栏，预设可见玩家集合
    class TestBossBar final : public IDragonBossBar {
    public:
        explicit TestBossBar(std::set<PlayerId> players)
            : m_players(std::move(players))
        {}

        void setPercent(f32 /*percent*/) override {}
        void setName(std::unique_ptr<text::ITextComponent> /*name*/) override {}
        void setVisible(bool /*visible*/) override {}
        void addPlayer(PlayerId /*playerId*/) override {}
        void removePlayer(PlayerId /*playerId*/) override {}
        void removeAllPlayers() override {}
        void replacePlayers(const std::set<PlayerId>& /*playerIds*/) override {}
        [[nodiscard]] bool hasPlayers() const override { return !m_players.empty(); }
        [[nodiscard]] const std::set<PlayerId>& getPlayers() const override { return m_players; }
        [[nodiscard]] f32 percent() const override { return 0.0f; }
        [[nodiscard]] bool visible() const override { return true; }

    private:
        std::set<PlayerId> m_players;
    };

    const std::set<PlayerId> trackedPlayers{PlayerId{1001}, PlayerId{1002}, PlayerId{1003}};
    fight.setDragonBossBar(std::make_unique<TestBossBar>(trackedPlayers));

    accessor.setRespawnStage(DragonRespawnAnimation::START);

    fight.setRespawnStage(m_world, DragonRespawnAnimation::END);

    // 应对每个 Boss 栏可见玩家触发一次 onSummonedEntity
    const auto& calls = m_world.summonedEntityCalls();
    ASSERT_EQ(calls.size(), trackedPlayers.size());

    // 收集实际触发的 playerId 集合
    std::set<PlayerId> actualPlayers;
    for (const auto& [pid, entityPtr] : calls) {
        actualPlayers.insert(pid);
        // 实体指针应为新生成的末影龙（非空）
        EXPECT_NE(entityPtr, nullptr);
        EXPECT_EQ(entityPtr->getTypeId(), entity::EntityTypeKeys::ENDER_DRAGON);
    }
    EXPECT_EQ(actualPlayers, trackedPlayers);
}

TEST_F(EndDragonFightRespawnTest, SetRespawnStage_End_WithNoBossBarPlayers_DoesNotFireSummonedEntity)
{
    // Boss 栏无可见玩家时，setRespawnStage(END) 不应触发任何 onSummonedEntity 回调。
    // 默认 NullDragonBossBar.getPlayers() 返回空集合，满足此条件。
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    accessor.setRespawnStage(DragonRespawnAnimation::START);

    fight.setRespawnStage(m_world, DragonRespawnAnimation::END);

    // 无玩家时不应触发任何回调
    EXPECT_TRUE(m_world.summonedEntityCalls().empty());
    // 但新龙仍应被创建
    auto dragons = m_world.getEntitiesByType(entity::EntityTypeKeys::ENDER_DRAGON);
    EXPECT_EQ(dragons.size(), 1u);
}

TEST_F(EndDragonFightRespawnTest, SetRespawnStage_NotRespawning_DoesNothing)
{
    // 未在重生中调用 setRespawnStage 应被拒绝
    EndDragonFight fight(42, std::nullopt);

    fight.setRespawnStage(m_world, DragonRespawnAnimation::PREPARING_TO_SUMMON_PILLARS);

    EXPECT_FALSE(fight.isRespawning());
}

// ============================================================================
// EnderCrystalEntity::hurt 测试
// ============================================================================

TEST_F(EndDragonFightRespawnTest, CrystalHurt_TriggersExplosionAndNotifiesFight)
{
    // 末影水晶被非爆炸伤害击中时应爆炸并通知 EndDragonFight
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    // 放置一个柱顶水晶
    const auto spikes = EndSpikeFeatureConfig::generateSpikes(42);
    ASSERT_FALSE(spikes.empty());
    const auto& spike = spikes[0];
    auto* crystal = m_world.spawnCrystalAt(spike.centerX, spike.height + 1, spike.centerZ);
    ASSERT_NE(crystal, nullptr);

    accessor.updateCrystalCount(m_world);
    EXPECT_EQ(accessor.crystalsAlive(), 1);

    // 用通用伤害击中水晶
    auto dmgSource = DamageSources::generic();
    const bool result = crystal->hurt(dmgSource, 1.0f);

    EXPECT_TRUE(result);
    // 水晶应被移除
    EXPECT_TRUE(crystal->isRemoved());
    // 应触发了一次爆炸（非爆炸伤害来源）
    EXPECT_GE(m_world.explosionCount(), 1u);
}

TEST_F(EndDragonFightRespawnTest, CrystalHurt_ByExplosionDamage_DoesNotTriggerExplosion)
{
    // 被爆炸伤害击中时不应再次触发爆炸（避免递归）
    EndDragonFight fight(42, std::nullopt);
    test::EndDragonFightTestAccessor accessor(fight);
    accessor.setDragonKilledFlag(true);
    m_world.setDragonFight(&fight);

    auto* crystal = m_world.spawnCrystalAt(0, 0, 0);
    ASSERT_NE(crystal, nullptr);

    const size_t explosionsBefore = m_world.explosionCount();
    auto dmgSource = DamageSources::explosion();
    const bool result = crystal->hurt(dmgSource, 1.0f);

    EXPECT_TRUE(result);
    EXPECT_TRUE(crystal->isRemoved());
    // 不应触发新的爆炸
    EXPECT_EQ(m_world.explosionCount(), explosionsBefore);
}
