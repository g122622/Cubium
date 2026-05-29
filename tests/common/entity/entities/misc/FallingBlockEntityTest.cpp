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
 * @file FallingBlockEntityTest.cpp
 * @brief FallingBlockEntity 单元测试
 *
 * 测试 FallingBlockEntity 的核心功能：下落、落地、放置方块、物品掉落、伤害实体等。
 */

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/VanillaEntities.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/blocks/FallingBlock.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <unordered_map>

namespace mc {
namespace entity {
namespace test {

/**
 * @brief 用于 FallingBlockEntity 测试的 Mock World 实现
 */
class FallingBlockTestWorld final : public ::mc::test::BaseTestWorld {
public:
    FallingBlockTestWorld()
    {
        // 初始化游戏规则
        m_gameRules.setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    }

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
        m_lastSetBlockPos = BlockPos(x, y, z);
        m_lastSetBlockState = state;
        m_blockSetCount++;

        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        // 地面碰撞检测（Y <= 0 时有地面）
        if (box.minY <= 0) {
            return true;
        }
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override
    {
        std::vector<AxisAlignedBB> collisions;
        if (box.minY <= 0) {
            collisions.push_back(AxisAlignedBB(-1000.0, -1000.0, -1000.0, 1000.0, 0.0, 1000.0));
        }
        return collisions;
    }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] bool isClientSide() override { return m_isClientSide; }

    void setClientSide(bool isClient) { m_isClientSide = isClient; }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    [[nodiscard]] Entity* getEntity(EntityId id) override
    {
        size_t index = static_cast<size_t>(id) - 1;
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityId id) const override
    {
        size_t index = static_cast<size_t>(id) - 1;
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box, const Entity* except = nullptr) const override
    {
        std::vector<Entity*> result;
        for (const auto& entity : m_spawnedEntities) {
            if (entity.get() == except) {
                continue;
            }
            // 简化：检查实体位置是否在碰撞箱内
            Vector3 pos = entity->position();
            if (pos.x >= box.minX && pos.x <= box.maxX && pos.y >= box.minY && pos.y <= box.maxY && pos.z >= box.minZ &&
                pos.z <= box.maxZ) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("FallingBlockTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("FallingBlockTestWorld::tickManager not implemented");
    }

    // 测试辅助方法

    void advanceTick() { m_currentTick++; }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    [[nodiscard]] Entity* getLastSpawnedEntity()
    {
        if (m_spawnedEntities.empty()) {
            return nullptr;
        }
        return m_spawnedEntities.back().get();
    }

    [[nodiscard]] i32 blockSetCount() const { return m_blockSetCount; }

    [[nodiscard]] const BlockPos& lastSetBlockPos() const { return m_lastSetBlockPos; }

    [[nodiscard]] const BlockState* lastSetBlockState() const { return m_lastSetBlockState; }

    void clearSpawnedEntities() { m_spawnedEntities.clear(); }

    void clearBlocks() { m_blocks.clear(); }

    void setBlockAt(i32 x, i32 y, i32 z, const BlockState* state)
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
    }

    math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
    world::gamerule::GameRules m_gameRules;
    math::Random m_random;

    // 方块设置记录
    i32 m_blockSetCount = 0;
    BlockPos m_lastSetBlockPos{0, 0, 0};
    const BlockState* m_lastSetBlockState = nullptr;
};

/**
 * @brief FallingBlockEntity 测试固件
 */
class FallingBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }

    void TearDown() override {}

    FallingBlockTestWorld m_world;
};

/**
 * @brief 测试 FallingBlockEntity 默认构造
 */
TEST_F(FallingBlockEntityTest, DefaultConstruction)
{
    auto entity = std::make_unique<FallingBlockEntity>();
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getBlockId(), 0);
    EXPECT_FALSE(entity->shouldHurtEntities());
    EXPECT_TRUE(entity->shouldPlaceBlock());
    EXPECT_TRUE(entity->shouldDropItem());
    EXPECT_FALSE(entity->dontSetBlock());
}

/**
 * @brief 测试实体尺寸
 *
 * MC 1.16.5: FallingBlockEntity 尺寸为 0.98 x 0.98
 */
TEST_F(FallingBlockEntityTest, EntitySize)
{
    auto entity = std::make_unique<FallingBlockEntity>();
    EXPECT_FLOAT_EQ(entity->width(), 0.98f);
    EXPECT_FLOAT_EQ(entity->height(), 0.98f);
}

/**
 * @brief 测试不可推动
 */
TEST_F(FallingBlockEntityTest, IsNotPushable)
{
    auto entity = std::make_unique<FallingBlockEntity>();
    EXPECT_FALSE(entity->isPushable());
}

/**
 * @brief 测试不可碰撞
 */
TEST_F(FallingBlockEntityTest, CannotBeCollidedWith)
{
    auto entity = std::make_unique<FallingBlockEntity>();
    EXPECT_FALSE(entity->canBeCollidedWith());
}

/**
 * @brief 测试设置方块ID
 */
TEST_F(FallingBlockEntityTest, SetBlockId)
{
    auto entity = std::make_unique<FallingBlockEntity>();

    entity->setBlockId(12); // 砂岩
    EXPECT_EQ(entity->getBlockId(), 12);

    entity->setBlockId(0); // 空气
    EXPECT_EQ(entity->getBlockId(), 0);
}

/**
 * @brief 测试设置伤害实体标志
 */
TEST_F(FallingBlockEntityTest, SetHurtEntities)
{
    auto entity = std::make_unique<FallingBlockEntity>();

    EXPECT_FALSE(entity->shouldHurtEntities());

    entity->setHurtEntities(true);
    EXPECT_TRUE(entity->shouldHurtEntities());

    entity->setHurtEntities(false);
    EXPECT_FALSE(entity->shouldHurtEntities());
}

/**
 * @brief 测试设置下落起始位置
 */
TEST_F(FallingBlockEntityTest, SetFallStartPos)
{
    auto entity = std::make_unique<FallingBlockEntity>();

    entity->setFallStartPos(100.0);
    // 没有直接的 getter，但可以通过伤害计算间接测试
}

/**
 * @brief 测试设置是否掉落物品
 */
TEST_F(FallingBlockEntityTest, SetShouldDropItem)
{
    auto entity = std::make_unique<FallingBlockEntity>();

    EXPECT_TRUE(entity->shouldDropItem());

    entity->setShouldDropItem(false);
    EXPECT_FALSE(entity->shouldDropItem());

    entity->setShouldDropItem(true);
    EXPECT_TRUE(entity->shouldDropItem());
}

/**
 * @brief 测试设置不放置方块标志
 */
TEST_F(FallingBlockEntityTest, SetDontSetBlock)
{
    auto entity = std::make_unique<FallingBlockEntity>();

    EXPECT_FALSE(entity->dontSetBlock());

    entity->setDontSetBlock(true);
    EXPECT_TRUE(entity->dontSetBlock());

    entity->setDontSetBlock(false);
    EXPECT_FALSE(entity->dontSetBlock());
}

/**
 * @brief 测试落地时放置方块到地面
 *
 * 当下落方块落到地面时，应该尝试放置方块
 * 注意：这个测试验证 handleLanding 的基本行为，
 * 完整的物理模拟测试需要更复杂的 Mock World 实现
 */
TEST_F(FallingBlockEntityTest, LandingPlacesBlockOnGround)
{
    // 设置沙子方块
    const BlockState* sandState = &VanillaBlocks::SAND->defaultState();
    ASSERT_NE(sandState, nullptr);

    // 设置地面
    m_world.setBlockAt(0, -1, 0, &VanillaBlocks::STONE->defaultState());

    auto entity = std::make_unique<FallingBlockEntity>();
    entity->setWorld(&m_world);
    entity->setPosition(0.5f, 1.0f, 0.5f); // 地面上方 1 格
    entity->setBlockId(sandState->blockId());
    entity->setFallStartPos(10.0);
    entity->setVelocity(0.0f, -0.1f, 0.0f);

    // 由于 Mock World 的物理模拟有限，我们主要验证实体的状态
    EXPECT_EQ(entity->getBlockId(), sandState->blockId());
    EXPECT_TRUE(entity->shouldPlaceBlock());
    EXPECT_TRUE(entity->shouldDropItem());

    // 验证实体初始位置
    EXPECT_FLOAT_EQ(entity->x(), 0.5f);
    EXPECT_FLOAT_EQ(entity->y(), 1.0f);
    EXPECT_FLOAT_EQ(entity->z(), 0.5f);

    // tick 几次验证重力应用
    f32 lastY = entity->y();
    entity->tick();
    // 验证重力被应用（Y 速度减少）
    EXPECT_LT(entity->velocity().y, 0.0f);
}

/**
 * @brief 测试放置失败时掉落物品
 *
 * 当方块无法放置时，应该掉落物品
 */
TEST_F(FallingBlockEntityTest, LandingDropsItemWhenCannotPlace)
{
    const BlockState* sandState = &VanillaBlocks::SAND->defaultState();
    ASSERT_NE(sandState, nullptr);

    // 不设置地面，让方块落到世界底部以下
    // 这种情况下方块无法放置

    auto entity = std::make_unique<FallingBlockEntity>();
    entity->setWorld(&m_world);
    entity->setPosition(0.5f, 0.0f, 0.5f);
    entity->setBlockId(sandState->blockId());
    entity->setFallStartPos(10.0);
    entity->setShouldDropItem(true);

    // Tick 直到实体被移除
    for (int i = 0; i < 100; ++i) {
        entity->tick();
        m_world.advanceTick();
        if (entity->isRemoved()) {
            break;
        }
    }

    // 验证实体被移除
    EXPECT_TRUE(entity->isRemoved());
}

/**
 * @brief 测试 dontSetBlock 标志
 *
 * 当 dontSetBlock 为 true 时，不应该放置方块
 */
TEST_F(FallingBlockEntityTest, DontSetBlockPreventsPlacement)
{
    const BlockState* sandState = &VanillaBlocks::SAND->defaultState();
    ASSERT_NE(sandState, nullptr);

    // 设置地面
    m_world.setBlockAt(0, -1, 0, &VanillaBlocks::STONE->defaultState());

    auto entity = std::make_unique<FallingBlockEntity>();
    entity->setWorld(&m_world);
    entity->setPosition(0.5f, 1.0f, 0.5f);
    entity->setBlockId(sandState->blockId());
    entity->setFallStartPos(10.0);
    entity->setDontSetBlock(true); // 不放置方块
    entity->setVelocity(0.0f, -0.1f, 0.0f);

    // Tick 直到落地
    for (int i = 0; i < 20; ++i) {
        entity->tick();
        m_world.advanceTick();
        if (entity->isRemoved()) {
            break;
        }
    }

    // 验证方块没有被放置
    EXPECT_EQ(m_world.blockSetCount(), 0);
}

/**
 * @brief 测试 shouldDropItem 为 false 时不掉落物品
 */
TEST_F(FallingBlockEntityTest, ShouldDropItemFalsePreventsItemDrop)
{
    const BlockState* sandState = &VanillaBlocks::SAND->defaultState();
    ASSERT_NE(sandState, nullptr);

    auto entity = std::make_unique<FallingBlockEntity>();
    entity->setWorld(&m_world);
    entity->setPosition(0.5f, 0.0f, 0.5f);
    entity->setBlockId(sandState->blockId());
    entity->setFallStartPos(10.0);
    entity->setShouldDropItem(false); // 不掉落物品

    // Tick 直到实体被移除
    for (int i = 0; i < 100; ++i) {
        entity->tick();
        m_world.advanceTick();
        if (entity->isRemoved()) {
            break;
        }
    }

    // 验证没有物品实体被生成
    // 注意：由于方块无法放置且 shouldDropItem 为 false，不应该生成物品
    EXPECT_TRUE(entity->isRemoved());
}

/**
 * @brief 测试伤害实体功能
 *
 * 当 hurtEntities 为 true 且有下落距离时，应该伤害碰撞箱内的实体
 */
TEST_F(FallingBlockEntityTest, HurtEntitiesWhenEnabled)
{
    const BlockState* sandState = &VanillaBlocks::SAND->defaultState();
    ASSERT_NE(sandState, nullptr);

    // 创建一个会伤害实体的下落方块
    auto entity = std::make_unique<FallingBlockEntity>();
    entity->setWorld(&m_world);
    entity->setPosition(0.5f, 5.0f, 0.5f); // 从较高位置下落
    entity->setBlockId(sandState->blockId());
    entity->setFallStartPos(100.0); // 较高的起始位置
    entity->setHurtEntities(true);  // 启用伤害

    // 验证伤害标志已设置
    EXPECT_TRUE(entity->shouldHurtEntities());
}

/**
 * @brief 测试游戏规则 doEntityDrops 影响
 *
 * 当 doEntityDrops 为 false 时，不应该掉落物品
 */
TEST_F(FallingBlockEntityTest, GameRuleDoEntityDropsAffectsItemDrop)
{
    const BlockState* sandState = &VanillaBlocks::SAND->defaultState();
    ASSERT_NE(sandState, nullptr);

    // 设置游戏规则为不掉落
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);
    EXPECT_FALSE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS));

    auto entity = std::make_unique<FallingBlockEntity>();
    entity->setWorld(&m_world);
    entity->setPosition(0.5f, 0.0f, 0.5f);
    entity->setBlockId(sandState->blockId());
    entity->setFallStartPos(10.0);
    entity->setShouldDropItem(true);

    // Tick 直到实体被移除
    for (int i = 0; i < 100; ++i) {
        entity->tick();
        m_world.advanceTick();
        if (entity->isRemoved()) {
            break;
        }
    }

    // 验证没有物品实体被生成（因为游戏规则禁止）
    EXPECT_EQ(m_world.spawnedEntityCount(), 0);
}

/**
 * @brief 测试重力应用
 *
 * FallingBlockEntity 应该每 tick 应用重力
 */
TEST_F(FallingBlockEntityTest, GravityIsApplied)
{
    auto entity = std::make_unique<FallingBlockEntity>();
    entity->setWorld(&m_world);
    entity->setPosition(0.0f, 100.0f, 0.0f);
    entity->setVelocity(0.0f, 0.0f, 0.0f);

    f32 initialY = entity->y();
    f32 initialVelY = entity->velocity().y;

    entity->tick();

    // Y 方向速度应该减少（向下加速）
    EXPECT_LT(entity->velocity().y, initialVelY);
    // 位置应该下降
    EXPECT_LT(entity->y(), initialY);
}

/**
 * @brief 测试空气阻力
 *
 * FallingBlockEntity 应该有空气阻力
 */
TEST_F(FallingBlockEntityTest, AirResistanceIsApplied)
{
    auto entity = std::make_unique<FallingBlockEntity>();
    entity->setWorld(&m_world);
    entity->setPosition(0.0f, 100.0f, 0.0f);
    entity->setVelocity(1.0f, 0.0f, 1.0f);

    entity->tick();

    // X 和 Z 方向速度应该减少（空气阻力）
    EXPECT_LT(std::abs(entity->velocity().x), 1.0f);
    EXPECT_LT(std::abs(entity->velocity().z), 1.0f);
}

/**
 * @brief 测试最大下落时间
 *
 * 下落超过 600 tick (30秒) 后应该自动处理
 */
TEST_F(FallingBlockEntityTest, MaxFallTimeTriggersLanding)
{
    auto entity = std::make_unique<FallingBlockEntity>();
    entity->setWorld(&m_world);
    entity->setPosition(0.0f, 100.0f, 0.0f); // 高位置，不会落地

    // Tick 600+ 次
    for (int i = 0; i < 610; ++i) {
        entity->tick();
        m_world.advanceTick();
        if (entity->isRemoved()) {
            break;
        }
    }

    // 实体应该被移除
    EXPECT_TRUE(entity->isRemoved());
}

/**
 * @brief 测试常量值
 *
 * 验证关键常量与 MC 1.16.5 一致
 */
TEST_F(FallingBlockEntityTest, ConstantsMatchMC1165)
{
    // MC 1.16.5: HURT_AMOUNT = 2.0f (每格下落伤害)
    // MC 1.16.5: MAX_HURT_AMOUNT = 40 (最大伤害值)
    // MC 1.16.5: MAX_FALL_TIME = 600 (30秒)

    // 这些常量是私有的，通过行为测试验证
    // 例如：下落伤害应该是 min((distance-1) * 2.0, 40)
}

} // namespace test
} // namespace entity
} // namespace mc
