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
 * @file TNTBlockTest.cpp
 * @brief TNTBlock 单元测试
 *
 * 测试 TNT 方块的 prime、ignite、爆炸功能及交互回调。
 * 覆盖：
 * - prime() 仅生成实体和音效，不移除方块
 * - ignite() = prime() + 移除方块
 * - onBlockActivated（打火石/火焰弹点燃）
 * - playerWillDestroy（不稳定 TNT 自动点燃，无双重移除）
 * - onProjectileHit（燃烧投掷物点燃）
 * - canDropFromExplosion
 * - onBlockExploded（连锁爆炸短引信）
 * - tntExplodes 游戏规则控制
 */

#include "common/world/block/blocks/redstone/TNTBlock.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace mc {
namespace blocks {
namespace test {

// ============================================================================
// 测试用 Mock World
// ============================================================================

/**
 * @brief 用于 TNTBlock 测试的 Mock World 实现
 */
class TNTBlockTestWorld final : public ::mc::test::BaseTestWorld {
public:
    TNTBlockTestWorld() = default;

    // ========== 方块访问 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        m_setBlockCallCount++;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        m_lastSetBlockFlags = flags;
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }

    void setClientSide(bool isClient) { m_isClientSide = isClient; }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entityLookup.find(id);
        return it != m_entityLookup.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_entityLookup.find(id);
        return it != m_entityLookup.end() ? it->second : nullptr;
    }

    void registerEntity(Entity* entity)
    {
        if (entity != nullptr) {
            m_entityLookup[entity->id()] = entity;
        }
    }

    void unregisterEntity(Entity* entity)
    {
        if (entity != nullptr) {
            m_entityLookup.erase(entity->id());
        }
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        // 记录生成的 TNT 实体
        if (auto* tnt = dynamic_cast<entity::TNTEntity*>(entity.get())) {
            m_spawnedTNTCount++;
            m_lastTNTPosition = tnt->position();
            m_lastTNTFuse = tnt->getFuse();
            m_lastTNTVelocity = tnt->velocity();
            m_lastTNTOwner = tnt->getOwner();
        }

        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_lastSoundEvent = soundEventId;
        m_lastSoundCategory = category;
        m_lastSoundPosition = position;
        m_lastSoundVolume = volume;
        m_lastSoundPitch = pitch;
        m_soundPlayed = true;
    }

    void createExplosion(const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode,
        bool causesFire,
        Entity* source) override
    {
        m_lastExplosionPos = position;
        m_lastExplosionRadius = radius;
        m_lastExplosionMode = mode;
        m_explosionCausesFire = causesFire;
        m_lastExplosionSource = source;
        m_explosionCount++;
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_lastGameEventId = event.id();
        m_lastGameEventPos = pos;
        m_lastGameEventSourceEntity = context.sourceEntity();
        m_gameEventCount++;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("TNTBlockTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("TNTBlockTestWorld::tickManager not implemented");
    }

    // ========== 测试辅助方法 ==========

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = std::make_unique<BlockState>(*state);
        }
    }

    void advanceTick() { m_currentTick++; }

    [[nodiscard]] i32 spawnedTNTCount() const { return m_spawnedTNTCount; }
    [[nodiscard]] const Vector3& lastTNTPosition() const { return m_lastTNTPosition; }
    [[nodiscard]] i32 lastTNTFuse() const { return m_lastTNTFuse; }
    [[nodiscard]] const Vector3& lastTNTVelocity() const { return m_lastTNTVelocity; }
    [[nodiscard]] Entity* lastTNTOwner() const { return m_lastTNTOwner; }

    [[nodiscard]] bool soundPlayed() const { return m_soundPlayed; }
    [[nodiscard]] const ResourceLocation& lastSoundEvent() const { return m_lastSoundEvent; }

    [[nodiscard]] i32 explosionCount() const { return m_explosionCount; }
    [[nodiscard]] const Vector3& lastExplosionPos() const { return m_lastExplosionPos; }
    [[nodiscard]] f32 lastExplosionRadius() const { return m_lastExplosionRadius; }
    [[nodiscard]] world::explosion::ExplosionMode lastExplosionMode() const { return m_lastExplosionMode; }
    [[nodiscard]] bool explosionCausesFire() const { return m_explosionCausesFire; }

    [[nodiscard]] i32 setBlockCallCount() const { return m_setBlockCallCount; }
    [[nodiscard]] i32 lastSetBlockFlags() const { return m_lastSetBlockFlags; }

    [[nodiscard]] i32 gameEventCount() const { return m_gameEventCount; }
    [[nodiscard]] const std::string& lastGameEventId() const { return m_lastGameEventId; }
    [[nodiscard]] const Entity* lastGameEventSourceEntity() const { return m_lastGameEventSourceEntity; }

    void clearState()
    {
        m_blocks.clear();
        m_spawnedEntities.clear();
        m_spawnedTNTCount = 0;
        m_explosionCount = 0;
        m_soundPlayed = false;
        m_setBlockCallCount = 0;
        m_lastSetBlockFlags = 0;
        m_gameEventCount = 0;
        m_lastTNTOwner = nullptr;
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::unordered_map<EntityInstanceId, Entity*> m_entityLookup;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
    world::gamerule::GameRules m_gameRules;

    // TNT 生成记录
    i32 m_spawnedTNTCount = 0;
    Vector3 m_lastTNTPosition{0, 0, 0};
    i32 m_lastTNTFuse = 0;
    Vector3 m_lastTNTVelocity{0, 0, 0};
    Entity* m_lastTNTOwner = nullptr;

    // 声音记录
    bool m_soundPlayed = false;
    ResourceLocation m_lastSoundEvent;
    sound::SoundCategory m_lastSoundCategory = sound::SoundCategory::Master;
    Vector3 m_lastSoundPosition{0, 0, 0};
    f32 m_lastSoundVolume = 1.0f;
    f32 m_lastSoundPitch = 1.0f;

    // 爆炸记录
    i32 m_explosionCount = 0;
    Vector3 m_lastExplosionPos{0, 0, 0};
    f32 m_lastExplosionRadius = 0.0f;
    world::explosion::ExplosionMode m_lastExplosionMode = world::explosion::ExplosionMode::None;
    bool m_explosionCausesFire = false;
    Entity* m_lastExplosionSource = nullptr;

    // 方块设置记录
    i32 m_setBlockCallCount = 0;
    i32 m_lastSetBlockFlags = 0;

    // 游戏事件记录
    i32 m_gameEventCount = 0;
    std::string m_lastGameEventId;
    BlockPos m_lastGameEventPos{0, 0, 0};
    const Entity* m_lastGameEventSourceEntity = nullptr;
};

// ============================================================================
// 测试固件
// ============================================================================

class TNTBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        Items::initialize();
    }

    void TearDown() override { m_world.clearState(); }

    TNTBlockTestWorld m_world;
};

// ============================================================================
// 基本构造与状态测试
// ============================================================================

TEST_F(TNTBlockTest, Construction)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);
    ASSERT_NE(tntBlock, nullptr);
    const BlockState& defaultState = tntBlock->defaultState();
    EXPECT_EQ(&defaultState.getBlock(), static_cast<const Block*>(tntBlock.get()));
}

TEST_F(TNTBlockTest, IsUnstable_DefaultIsFalse)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);
    const BlockState& defaultState = tntBlock->defaultState();
    EXPECT_FALSE(TNTBlock::isUnstable(defaultState));
}

TEST_F(TNTBlockTest, IsUnstable_CanSetToTrue)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);
    BlockState unstableState = tntBlock->defaultState().with(BlockStateProperties::UNSTABLE(), true);
    EXPECT_TRUE(TNTBlock::isUnstable(unstableState));
}

TEST_F(TNTBlockTest, CanDropFromExplosionReturnsFalse)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);
    const BlockState& defaultState = tntBlock->defaultState();
    EXPECT_FALSE(tntBlock->canDropFromExplosion(defaultState));
}

// ============================================================================
// prime() 测试 — 仅生成实体和音效，不移除方块
// ============================================================================

TEST_F(TNTBlockTest, Prime_SpawnsEntityAndPlaysSound)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    bool result = TNTBlock::prime(m_world, tntPos, nullptr);

    // prime 应该成功
    EXPECT_TRUE(result);
    // 应该生成 TNT 实体
    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    // 应该播放音效
    EXPECT_TRUE(m_world.soundPlayed());
    EXPECT_EQ(m_world.lastSoundEvent(), SoundEvents::ENTITY_TNT_PRIMED);
    // 应该发出 PRIME_FUSE 游戏事件
    EXPECT_EQ(m_world.gameEventCount(), 1);
    EXPECT_EQ(m_world.lastGameEventId(), "prime_fuse");
}

TEST_F(TNTBlockTest, Prime_DoesNotRemoveBlock)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    // prime 前记下 setBlock 调用次数
    i32 setBlockCallsBefore = m_world.setBlockCallCount();

    TNTBlock::prime(m_world, tntPos, nullptr);

    // prime 不应该调用 setBlockState（不移除方块）
    // 注意：TNTEntity 生成可能会调用一些 setBlock，但 prime 本身不调用
    // 验证方块仍然存在
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state != nullptr && !state->is(VanillaBlocks::AIR));
}

TEST_F(TNTBlockTest, Prime_ClientSideReturnsFalse)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(true);

    bool result = TNTBlock::prime(m_world, tntPos, nullptr);

    EXPECT_FALSE(result);
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
    EXPECT_FALSE(m_world.soundPlayed());
}

TEST_F(TNTBlockTest, Prime_TntExplodesFalseReturnsFalse)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES, false, nullptr);

    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    bool result = TNTBlock::prime(m_world, tntPos, nullptr);

    EXPECT_FALSE(result);
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
    EXPECT_FALSE(m_world.soundPlayed());
}

TEST_F(TNTBlockTest, Prime_SetsIgniterAsTNTOwner)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    // 创建一个玩家作为点燃者
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    TNTBlock::prime(m_world, tntPos, player.get());

    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    // TNT 实体的 owner 应该被设置为玩家
    EXPECT_EQ(m_world.lastTNTOwner(), player.get());
}

TEST_F(TNTBlockTest, Prime_NullIgniter_TNTOwnerIsNull)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    TNTBlock::prime(m_world, tntPos, nullptr);

    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    EXPECT_EQ(m_world.lastTNTOwner(), nullptr);
}

TEST_F(TNTBlockTest, Prime_SetsCorrectPosition)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    TNTBlock::prime(m_world, tntPos, nullptr);

    // TNT 位置应该是方块中心
    EXPECT_FLOAT_EQ(m_world.lastTNTPosition().x, 10.5f);
    EXPECT_FLOAT_EQ(m_world.lastTNTPosition().y, 64.0f);
    EXPECT_FLOAT_EQ(m_world.lastTNTPosition().z, 20.5f);
}

TEST_F(TNTBlockTest, Prime_SetsDefaultFuse)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    TNTBlock::prime(m_world, tntPos, nullptr);

    // 默认引信时间 80 ticks
    EXPECT_EQ(m_world.lastTNTFuse(), 80);
}

TEST_F(TNTBlockTest, Prime_SetsYVelocity)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(0, 64, 0);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    TNTBlock::prime(m_world, tntPos, nullptr);

    // Y 速度应该是固定的 0.2
    EXPECT_FLOAT_EQ(m_world.lastTNTVelocity().y, 0.2f);
}

// ============================================================================
// ignite() 测试 — prime() + 移除方块
// ============================================================================

TEST_F(TNTBlockTest, Ignite_PrimesAndRemovesBlock)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    bool result = tntBlock->ignite(m_world, tntPos, tntBlock->defaultState());

    EXPECT_TRUE(result);
    // 应该生成 TNT 实体
    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    // 应该播放音效
    EXPECT_TRUE(m_world.soundPlayed());
    // 方块应该被移除
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state == nullptr || state->is(VanillaBlocks::AIR));
}

TEST_F(TNTBlockTest, Ignite_WithIgniter_SetsTNTOwner)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    bool result = tntBlock->ignite(m_world, tntPos, tntBlock->defaultState(), player.get());

    EXPECT_TRUE(result);
    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    EXPECT_EQ(m_world.lastTNTOwner(), player.get());
}

TEST_F(TNTBlockTest, Ignite_ClientSideReturnsFalse)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(true);

    bool result = tntBlock->ignite(m_world, tntPos, tntBlock->defaultState());

    EXPECT_FALSE(result);
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
    // 方块不应该被移除
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state != nullptr && !state->is(VanillaBlocks::AIR));
}

TEST_F(TNTBlockTest, Ignite_TntExplodesFalse_ReturnsFalseAndBlockStays)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES, false, nullptr);

    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    bool result = tntBlock->ignite(m_world, tntPos, tntBlock->defaultState());

    EXPECT_FALSE(result);
    // 方块应该仍然存在
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state != nullptr && !state->is(VanillaBlocks::AIR));
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
    EXPECT_FALSE(m_world.soundPlayed());
}

// ============================================================================
// onBlockAdded 测试
// ============================================================================

TEST_F(TNTBlockTest, OnBlockAdded_WithFire_PrimesAndRemovesBlock)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(0, 64, 0);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());

    // 在旁边放置火焰
    BlockPos firePos(1, 64, 0);
    m_world.setBlockAt(firePos, &VanillaBlocks::FIRE->defaultState());
    m_world.setClientSide(false);

    tntBlock->onBlockAdded(m_world, tntPos, tntBlock->defaultState());

    // 应该生成 TNT 实体
    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    // 方块应该被移除（onBlockAdded 调用 prime() + setBlockState）
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state == nullptr || state->is(VanillaBlocks::AIR));
}

// ============================================================================
// onBlockActivated 测试 — 玩家交互
// ============================================================================

TEST_F(TNTBlockTest, OnBlockActivated_EmptyHand_ReturnsPass)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

    auto result = tntBlock->onBlockActivated(tntBlock->defaultState(), m_world, tntPos, *player, Hand::MainHand, hit);

    // 空手应该返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);
    // 不应该生成 TNT 实体
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
}

TEST_F(TNTBlockTest, OnBlockActivated_FlintAndSteel_PrimesAndRemovesBlock)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    // 给玩家手持打火石
    if (Items::FLINT_AND_STEEL != nullptr) {
        ItemStack flintAndSteel(Items::FLINT_AND_STEEL, 1);
        player->getHeldItem(Hand::MainHand) = flintAndSteel;

        BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

        auto result =
            tntBlock->onBlockActivated(tntBlock->defaultState(), m_world, tntPos, *player, Hand::MainHand, hit);

        // 应该返回 Success
        EXPECT_EQ(result, ActionResultType::Success);
        // 应该生成 TNT 实体
        EXPECT_EQ(m_world.spawnedTNTCount(), 1);
        // 方块应该被移除
        const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
        EXPECT_TRUE(state == nullptr || state->is(VanillaBlocks::AIR));
    }
}

TEST_F(TNTBlockTest, OnBlockActivated_FlintAndSteel_SetsPlayerAsIgniter)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    if (Items::FLINT_AND_STEEL != nullptr) {
        ItemStack flintAndSteel(Items::FLINT_AND_STEEL, 1);
        player->getHeldItem(Hand::MainHand) = flintAndSteel;

        BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

        tntBlock->onBlockActivated(tntBlock->defaultState(), m_world, tntPos, *player, Hand::MainHand, hit);

        // 玩家应该是点燃者
        EXPECT_EQ(m_world.spawnedTNTCount(), 1);
        EXPECT_EQ(m_world.lastTNTOwner(), player.get());
    }
}

TEST_F(TNTBlockTest, OnBlockActivated_FireCharge_PrimesAndRemovesBlock)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    // 给玩家手持火焰弹
    if (Items::FIRE_CHARGE != nullptr) {
        ItemStack fireCharge(Items::FIRE_CHARGE, 1);
        player->getHeldItem(Hand::MainHand) = fireCharge;

        BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

        auto result =
            tntBlock->onBlockActivated(tntBlock->defaultState(), m_world, tntPos, *player, Hand::MainHand, hit);

        EXPECT_EQ(result, ActionResultType::Success);
        EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    }
}

TEST_F(TNTBlockTest, OnBlockActivated_NonIgnitionItem_ReturnsPass)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    // Player 的 getHeldItem 默认为空物品堆，空手应返回 Pass
    BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

    auto result = tntBlock->onBlockActivated(tntBlock->defaultState(), m_world, tntPos, *player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
}

TEST_F(TNTBlockTest, OnBlockActivated_TntExplodesFalse_ShowsMessageAndReturnsPass)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES, false, nullptr);

    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    if (Items::FLINT_AND_STEEL != nullptr) {
        ItemStack flintAndSteel(Items::FLINT_AND_STEEL, 1);
        player->getHeldItem(Hand::MainHand) = flintAndSteel;

        BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

        auto result =
            tntBlock->onBlockActivated(tntBlock->defaultState(), m_world, tntPos, *player, Hand::MainHand, hit);

        // tntExplodes=false 时应该返回 Pass（不消耗物品）
        EXPECT_EQ(result, ActionResultType::Pass);
        // 不应该生成 TNT 实体
        EXPECT_EQ(m_world.spawnedTNTCount(), 0);
        // 方块应该仍然存在
        const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
        EXPECT_TRUE(state != nullptr && !state->is(VanillaBlocks::AIR));
    }
}

// ============================================================================
// playerWillDestroy 测试
// ============================================================================

TEST_F(TNTBlockTest, PlayerWillDestroy_UnstableTNT_SurvivalMode_PrimesWithoutRemovingBlock)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);
    BlockState unstableState = tntBlock->defaultState().with(BlockStateProperties::UNSTABLE(), true);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &unstableState);
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    // Player 默认是生存模式

    // 记录调用前的 setBlock 调用次数
    i32 setBlockCallsBefore = m_world.setBlockCallCount();

    tntBlock->playerWillDestroy(m_world, tntPos, unstableState, *player);

    // 应该生成 TNT 实体（prime 成功）
    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    // 播放音效
    EXPECT_TRUE(m_world.soundPlayed());

    // 关键：playerWillDestroy 只调用 prime()，不移除方块
    // 方块移除由破坏流程处理，所以方块应该仍然存在
    // （在实际游戏中，破坏流程会在 playerWillDestroy 返回后移除方块）
    // 这里我们只能验证 playerWillDestroy 自身没有移除方块
    // 由于 mock world 的 setBlockState 不做实际移除（只记录），
    // 我们通过检查 setBlock 调用次数来验证
    // playerWillDestroy 不应额外调用 setBlockState
    EXPECT_EQ(m_world.setBlockCallCount(), setBlockCallsBefore);
}

TEST_F(TNTBlockTest, PlayerWillDestroy_StableTNT_DoesNotPrime)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);
    const BlockState& stableState = tntBlock->defaultState(); // UNSTABLE=false

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &stableState);
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    tntBlock->playerWillDestroy(m_world, tntPos, stableState, *player);

    // 稳定 TNT 不应点燃
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
    EXPECT_FALSE(m_world.soundPlayed());
}

TEST_F(TNTBlockTest, PlayerWillDestroy_UnstableTNT_CreativeMode_DoesNotPrime)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);
    BlockState unstableState = tntBlock->defaultState().with(BlockStateProperties::UNSTABLE(), true);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &unstableState);
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Creative);

    tntBlock->playerWillDestroy(m_world, tntPos, unstableState, *player);

    // 创造模式下不应点燃
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
}

TEST_F(TNTBlockTest, PlayerWillDestroy_ClientSide_DoesNotPrime)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);
    BlockState unstableState = tntBlock->defaultState().with(BlockStateProperties::UNSTABLE(), true);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &unstableState);
    m_world.setClientSide(true); // 客户端

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    tntBlock->playerWillDestroy(m_world, tntPos, unstableState, *player);

    // 客户端不应点燃
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
}

// ============================================================================
// onProjectileHit 测试
// ============================================================================

TEST_F(TNTBlockTest, OnProjectileHit_BurningEntity_PrimesAndRemovesBlock)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    // 创建一个燃烧的实体来模拟投掷物
    Entity projectile(EntityInstanceId(200), nullptr, mc::test::testEcsRegistry());
    projectile.setWorld(&m_world);
    projectile.setFire(100); // 设置着火

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

    tntBlock->onProjectileHit(m_world, tntBlock->defaultState(), hitResult, projectile);

    // 燃烧投掷物应该点燃 TNT
    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    // 方块应该被移除
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state == nullptr || state->is(VanillaBlocks::AIR));
}

TEST_F(TNTBlockTest, OnProjectileHit_NonBurningEntity_DoesNotPrime)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    // 创建一个未燃烧的实体
    Entity projectile(EntityInstanceId(200), nullptr, mc::test::testEcsRegistry());
    projectile.setWorld(&m_world);
    // 不设置着火

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

    tntBlock->onProjectileHit(m_world, tntBlock->defaultState(), hitResult, projectile);

    // 非燃烧投掷物不应该点燃 TNT
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
    // 方块应该仍然存在
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state != nullptr && !state->is(VanillaBlocks::AIR));
}

TEST_F(TNTBlockTest, OnProjectileHit_ClientSide_DoesNotPrime)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(true); // 客户端

    Entity projectile(EntityInstanceId(200), nullptr, mc::test::testEcsRegistry());
    projectile.setWorld(&m_world);
    projectile.setFire(100);

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

    tntBlock->onProjectileHit(m_world, tntBlock->defaultState(), hitResult, projectile);

    // 客户端不应该点燃
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
}

TEST_F(TNTBlockTest, OnProjectileHit_TntExplodesFalse_DoesNotPrime)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES, false, nullptr);

    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    Entity projectile(EntityInstanceId(200), nullptr, mc::test::testEcsRegistry());
    projectile.setWorld(&m_world);
    projectile.setFire(100);

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

    tntBlock->onProjectileHit(m_world, tntBlock->defaultState(), hitResult, projectile);

    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state != nullptr && !state->is(VanillaBlocks::AIR));
}

// ============================================================================
// explode() 测试
// ============================================================================

TEST_F(TNTBlockTest, Explode)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());

    tntBlock->explode(m_world, tntPos, 4.0f);

    // 方块应该被移除
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state == nullptr || state->is(VanillaBlocks::AIR));

    // 应该创建爆炸
    EXPECT_EQ(m_world.explosionCount(), 1);
    EXPECT_FLOAT_EQ(m_world.lastExplosionRadius(), 4.0f);
    EXPECT_EQ(m_world.lastExplosionMode(), world::explosion::ExplosionMode::Break);
    EXPECT_FALSE(m_world.explosionCausesFire());

    // 爆炸位置（方块中心，Y 偏移 0.0625）
    EXPECT_FLOAT_EQ(m_world.lastExplosionPos().x, 10.5f);
    EXPECT_FLOAT_EQ(m_world.lastExplosionPos().y, 64.0f + 0.0625f);
    EXPECT_FLOAT_EQ(m_world.lastExplosionPos().z, 20.5f);
}

TEST_F(TNTBlockTest, Explode_RemovesBlockEvenWhenRuleDisabled)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES, false, nullptr);

    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());

    tntBlock->explode(m_world, tntPos, 4.0f);

    // 即使 tntExplodes=false，方块仍应被移除
    const BlockState* state = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(state == nullptr || state->is(VanillaBlocks::AIR));

    // 不应该创建爆炸
    EXPECT_EQ(m_world.explosionCount(), 0);
}

// ============================================================================
// onBlockExploded 测试 — 连锁爆炸
// ============================================================================

TEST_F(TNTBlockTest, OnBlockExploded_SpawnsPrimedTNTWithShortFuse)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    tntBlock->onBlockExploded(m_world, tntPos, tntBlock->defaultState(), nullptr);

    // 应该生成 TNT 实体
    EXPECT_EQ(m_world.spawnedTNTCount(), 1);

    // 短引信：DEFAULT_FUSE / 4 = 20，DEFAULT_FUSE / 8 = 10
    // 随机范围：[10, 29] ticks
    i32 fuse = m_world.lastTNTFuse();
    EXPECT_GE(fuse, 10);
    EXPECT_LE(fuse, 29);
}

TEST_F(TNTBlockTest, OnBlockExploded_ClientSide_DoesNothing)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(true);

    tntBlock->onBlockExploded(m_world, tntPos, tntBlock->defaultState(), nullptr);

    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
}

TEST_F(TNTBlockTest, OnBlockExploded_TntExplodesFalse_DoesNothing)
{
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES, false, nullptr);

    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    tntBlock->onBlockExploded(m_world, tntPos, tntBlock->defaultState(), nullptr);

    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
}

// ============================================================================
// TNTEntity 注册测试
// ============================================================================

TEST_F(TNTBlockTest, TNTEntityIsRegistered)
{
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* tntType = registry.getType(entity::EntityTypeKeys::TNT);

    ASSERT_NE(tntType, nullptr);
    EXPECT_TRUE(tntType->isValid());
}

// ============================================================================
// prime() 与 ignite() 的关键区别测试
// ============================================================================

TEST_F(TNTBlockTest, PrimeVersusIgnite_PrimeDoesNotCallSetBlock_IgniteDoes)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);

    // ---- 测试 prime()：不应移除方块 ----
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);
    i32 setBlockCallsBefore = m_world.setBlockCallCount();

    TNTBlock::prime(m_world, tntPos, nullptr);

    i32 setBlockCallsAfterPrime = m_world.setBlockCallCount();
    // prime 不应调用 setBlockState（方块仍存在）
    EXPECT_EQ(setBlockCallsAfterPrime, setBlockCallsBefore);
    // 方块应仍然存在
    const BlockState* stateAfterPrime = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(stateAfterPrime != nullptr && !stateAfterPrime->is(VanillaBlocks::AIR));

    // ---- 测试 ignite()：应移除方块 ----
    m_world.clearState();
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);
    setBlockCallsBefore = m_world.setBlockCallCount();

    tntBlock->ignite(m_world, tntPos, tntBlock->defaultState());

    i32 setBlockCallsAfterIgnite = m_world.setBlockCallCount();
    // ignite 应调用 setBlockState（移除方块）
    EXPECT_GT(setBlockCallsAfterIgnite, setBlockCallsBefore);
    // 方块应被移除
    const BlockState* stateAfterIgnite = m_world.getBlockState(tntPos.x, tntPos.y, tntPos.z);
    EXPECT_TRUE(stateAfterIgnite == nullptr || stateAfterIgnite->is(VanillaBlocks::AIR));
}

// ============================================================================
// PRIME_FUSE 游戏事件测试
// ============================================================================

TEST_F(TNTBlockTest, Prime_EmitsPrimeFuseGameEvent)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    TNTBlock::prime(m_world, tntPos, nullptr);

    // 应该发出 PRIME_FUSE 游戏事件
    EXPECT_EQ(m_world.gameEventCount(), 1);
    EXPECT_EQ(m_world.lastGameEventId(), "prime_fuse");
}

TEST_F(TNTBlockTest, Prime_WithIgniter_EmitsGameEventWithSourceEntity)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    TNTBlock::prime(m_world, tntPos, player.get());

    // 游戏事件应该记录玩家作为源实体
    EXPECT_EQ(m_world.gameEventCount(), 1);
    EXPECT_EQ(m_world.lastGameEventSourceEntity(), player.get());
}

// ============================================================================
// awardUsedStat 测试 — 打火石/火焰弹点燃时更新物品使用统计
// ============================================================================

TEST_F(TNTBlockTest, OnBlockActivated_FlintAndSteel_AwardUsedStat)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    if (Items::FLINT_AND_STEEL != nullptr) {
        ItemStack flintAndSteel(Items::FLINT_AND_STEEL, 1);
        player->getHeldItem(Hand::MainHand) = flintAndSteel;

        BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

        tntBlock->onBlockActivated(tntBlock->defaultState(), m_world, tntPos, *player, Hand::MainHand, hit);

        // Player 基类 awardUsedStat 默认空实现，不会崩溃即为通过
        // 验证 TNT 被成功点燃（说明 awardUsedStat 调用不会阻断执行流程）
        EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    }
}

TEST_F(TNTBlockTest, OnBlockActivated_FireCharge_AwardUsedStat)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    if (Items::FIRE_CHARGE != nullptr) {
        ItemStack fireCharge(Items::FIRE_CHARGE, 1);
        player->getHeldItem(Hand::MainHand) = fireCharge;

        BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

        tntBlock->onBlockActivated(tntBlock->defaultState(), m_world, tntPos, *player, Hand::MainHand, hit);

        // Player 基类 awardUsedStat 默认空实现，不会崩溃即为通过
        // 验证 TNT 被成功点燃
        EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    }
}

// ============================================================================
// mayInteract 测试 — 投掷物交互权限
// ============================================================================

TEST_F(TNTBlockTest, OnProjectileHit_MayInteractFalse_DoesNotPrime)
{
    // 验证：当 mayInteract 返回 false 时，燃烧投掷物不点燃 TNT
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    // 创建一个燃烧的 Player（冒险模式），mayInteract 返回 false
    auto player = std::make_unique<Player>(EntityInstanceId(200), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);
    player->setFire(100); // 设置着火

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

    // 冒险模式下 Player::mayInteract 返回 false
    // 直接使用 Player 作为 projectile 参数（Entity& 类型）
    tntBlock->onProjectileHit(m_world, tntBlock->defaultState(), hitResult, *player);

    // 冒险模式下 mayInteract 返回 false，不应点燃 TNT
    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
}

TEST_F(TNTBlockTest, OnProjectileHit_SpectatorMayInteractFalse_DoesNotPrime)
{
    // 验证：旁观者模式下 mayInteract 返回 false
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(200), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Spectator);
    player->setFire(100);

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

    tntBlock->onProjectileHit(m_world, tntBlock->defaultState(), hitResult, *player);

    EXPECT_EQ(m_world.spawnedTNTCount(), 0);
}

TEST_F(TNTBlockTest, OnProjectileHit_SurvivalMayInteractTrue_Primes)
{
    // 验证：生存模式下 mayInteract 返回 true，燃烧投掷物点燃 TNT
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(200), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Survival);
    player->setFire(100);

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

    tntBlock->onProjectileHit(m_world, tntBlock->defaultState(), hitResult, *player);

    // 生存模式下 mayInteract 返回 true，应该点燃 TNT
    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
}

TEST_F(TNTBlockTest, OnProjectileHit_CreativeMayInteractTrue_Primes)
{
    // 验证：创造模式下 mayInteract 返回 true
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(200), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Creative);
    player->setFire(100);

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), tntPos, Direction::Up, 0.0f);

    tntBlock->onProjectileHit(m_world, tntBlock->defaultState(), hitResult, *player);

    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
}

// ============================================================================
// Entity::mayInteract 默认实现测试
// ============================================================================

TEST_F(TNTBlockTest, EntityMayInteract_DefaultReturnsTrue)
{
    // 验证：Entity 基类的 mayInteract 默认返回 true
    Entity entity(EntityInstanceId(300), nullptr, mc::test::testEcsRegistry());
    BlockPos pos(10, 64, 20);
    EXPECT_TRUE(entity.mayInteract(m_world, pos));
}

// ============================================================================
// Player::mayInteract 测试
// ============================================================================

TEST_F(TNTBlockTest, PlayerMayInteract_Survival_ReturnsTrue)
{
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Survival);
    BlockPos pos(10, 64, 20);
    EXPECT_TRUE(player->mayInteract(m_world, pos));
}

TEST_F(TNTBlockTest, PlayerMayInteract_Creative_ReturnsTrue)
{
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Creative);
    BlockPos pos(10, 64, 20);
    EXPECT_TRUE(player->mayInteract(m_world, pos));
}

TEST_F(TNTBlockTest, PlayerMayInteract_Adventure_ReturnsFalse)
{
    // 冒险模式下没有 CanPlaceOn 标签的物品不能与方块交互
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);
    BlockPos pos(10, 64, 20);
    EXPECT_FALSE(player->mayInteract(m_world, pos));
}

TEST_F(TNTBlockTest, PlayerMayInteract_Spectator_ReturnsFalse)
{
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Spectator);
    BlockPos pos(10, 64, 20);
    EXPECT_FALSE(player->mayInteract(m_world, pos));
}

// ============================================================================
// onBlockExploded 带 Explosion 参数测试 — 连锁爆炸间接源实体
// ============================================================================

TEST_F(TNTBlockTest, OnBlockExploded_WithExplosion_SetsIndirectSourceAsOwner)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    // 创建一个玩家作为爆炸的间接源
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    // 创建爆炸对象，以玩家作为源实体
    world::explosion::Explosion explosion(
        m_world, Vector3(10.5f, 64.0f, 20.5f), 4.0f, world::explosion::ExplosionMode::Break, false, player.get());

    tntBlock->onBlockExploded(m_world, tntPos, tntBlock->defaultState(), &explosion);

    // 连锁 TNT 应该以玩家作为 owner（因为玩家是 LivingEntity，getIndirectSourceEntity 返回它）
    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    EXPECT_EQ(m_world.lastTNTOwner(), player.get());
}

TEST_F(TNTBlockTest, OnBlockExploded_WithNullExplosion_NoOwner)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    // nullptr 表示没有爆炸信息（向后兼容）
    tntBlock->onBlockExploded(m_world, tntPos, tntBlock->defaultState(), nullptr);

    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    // 没有 owner
    EXPECT_EQ(m_world.lastTNTOwner(), nullptr);
}

TEST_F(TNTBlockTest, OnBlockExploded_WithExplosion_NoSource_NoOwner)
{
    BlockProperties props(Material::TNT);
    auto tntBlock = std::make_unique<TNTBlock>(props);

    BlockPos tntPos(10, 64, 20);
    m_world.setBlockAt(tntPos, &tntBlock->defaultState());
    m_world.setClientSide(false);

    // 创建无源爆炸
    world::explosion::Explosion explosion(
        m_world, Vector3(10.5f, 64.0f, 20.5f), 4.0f, world::explosion::ExplosionMode::Break, false, nullptr);

    tntBlock->onBlockExploded(m_world, tntPos, tntBlock->defaultState(), &explosion);

    EXPECT_EQ(m_world.spawnedTNTCount(), 1);
    // 无源爆炸的 getIndirectSourceEntity 返回 nullptr
    EXPECT_EQ(m_world.lastTNTOwner(), nullptr);
}

// ============================================================================
// ProjectileEntity::mayInteract 测试
// ============================================================================

TEST_F(TNTBlockTest, ProjectileMayInteract_NullShooter_Allowed)
{
    // 无主投掷物允许交互
    entity::ArrowEntity projectile(EntityInstanceId(400), mc::test::testEcsRegistry());
    projectile.setWorld(&m_world);
    BlockPos pos(10, 64, 20);

    EXPECT_TRUE(projectile.mayInteract(m_world, pos));
}

TEST_F(TNTBlockTest, ProjectileMayInteract_SurvivalPlayerShooter_Allowed)
{
    // 发射者是生存模式玩家，mayInteract 返回 true
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Survival);
    m_world.registerEntity(player.get());

    entity::ArrowEntity projectile(EntityInstanceId(401), mc::test::testEcsRegistry());
    projectile.setWorld(&m_world);
    projectile.setShooter(player.get());

    BlockPos pos(10, 64, 20);
    EXPECT_TRUE(projectile.mayInteract(m_world, pos));
}

TEST_F(TNTBlockTest, ProjectileMayInteract_AdventurePlayerShooter_Denied)
{
    // 发射者是冒险模式玩家，mayInteract 返回 false
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);
    m_world.registerEntity(player.get());

    entity::ArrowEntity projectile(EntityInstanceId(402), mc::test::testEcsRegistry());
    projectile.setWorld(&m_world);
    projectile.setShooter(player.get());

    BlockPos pos(10, 64, 20);
    EXPECT_FALSE(projectile.mayInteract(m_world, pos));
}

TEST_F(TNTBlockTest, ProjectileMayInteract_MobGriefingTrue_Allowed)
{
    // 非玩家发射者 + MOB_GRIEFING=true 允许交互
    // 默认 MOB_GRIEFING 为 true
    Entity mob(EntityInstanceId(300), nullptr, mc::test::testEcsRegistry());
    mob.setWorld(&m_world);
    m_world.registerEntity(&mob);

    entity::ArrowEntity projectile(EntityInstanceId(403), mc::test::testEcsRegistry());
    projectile.setWorld(&m_world);
    projectile.setShooter(&mob);

    BlockPos pos(10, 64, 20);
    EXPECT_TRUE(projectile.mayInteract(m_world, pos));
}

TEST_F(TNTBlockTest, ProjectileMayInteract_MobGriefingFalse_Denied)
{
    // 非玩家发射者 + MOB_GRIEFING=false 禁止交互
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false, nullptr);

    Entity mob(EntityInstanceId(301), nullptr, mc::test::testEcsRegistry());
    mob.setWorld(&m_world);
    m_world.registerEntity(&mob);

    entity::ArrowEntity projectile(EntityInstanceId(404), mc::test::testEcsRegistry());
    projectile.setWorld(&m_world);
    projectile.setShooter(&mob);

    BlockPos pos(10, 64, 20);
    EXPECT_FALSE(projectile.mayInteract(m_world, pos));
}

// ============================================================================
// Explosion::getIndirectSourceEntity 测试
// ============================================================================

TEST_F(TNTBlockTest, ExplosionGetIndirectSourceEntity_NullSource)
{
    world::explosion::Explosion explosion(
        m_world, Vector3(0, 0, 0), 4.0f, world::explosion::ExplosionMode::Break, false, nullptr);
    EXPECT_EQ(explosion.getIndirectSourceEntity(), nullptr);
}

TEST_F(TNTBlockTest, ExplosionGetIndirectSourceEntity_PlayerSource)
{
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    world::explosion::Explosion explosion(
        m_world, Vector3(0, 0, 0), 4.0f, world::explosion::ExplosionMode::Break, false, player.get());

    // 玩家是 LivingEntity，getIndirectSourceEntity 应该返回它
    LivingEntity* indirect = explosion.getIndirectSourceEntity();
    EXPECT_EQ(indirect, player.get());
}

} // namespace test
} // namespace blocks
} // namespace mc
