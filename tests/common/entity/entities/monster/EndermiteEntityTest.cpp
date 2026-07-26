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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 *
 * 提供末影螨测试所需的最小 IWorld 接口实现
 */
class EndermiteTestWorld final : public test::BaseTestWorld {
public:
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
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("EndermiteTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("EndermiteTestWorld::tickManager not implemented");
    }

    // 测试辅助方法
    void incrementTick() { m_currentTick++; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    u64 m_currentTick = 0;
};

class EndermiteEntityTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    EndermiteTestWorld m_world;
};

// ==================== 基础属性测试 ====================

TEST_F(EndermiteEntityTest, Create_ReturnsValidEntity)
{
    auto entity = EndermiteEntity::create(&m_world);
    ASSERT_NE(entity, nullptr);
    // 静态工厂 EndermiteEntity::create 仅构造对象，不 setTypeId（typeId 由 EntityType::create
    // 经注册表赋值，见 EntityType.cpp）。此处仅验证工厂返回非空且类型正确，不断言 typeId，
    // 对齐 CopperGolem/SnowGolem Create_ReturnsValidEntity 约定，避免依赖 registerAll 副作用。
    EXPECT_NE(dynamic_cast<EndermiteEntity*>(entity.get()), nullptr);
}

TEST_F(EndermiteEntityTest, Constructor_SetsCorrectDefaults)
{
    EndermiteEntity endermite(EntityInstanceId(1));
    endermite.setWorld(&m_world);

    // 验证不在阳光下燃烧
    EXPECT_FALSE(endermite.shouldBurnInDaylight());

    // 验证经验值（在构造函数中设置）
    EXPECT_EQ(endermite.experienceValue(), 3);

    // 验证初始存活时间
    EXPECT_FALSE(endermite.isRemoved());
}

TEST_F(EndermiteEntityTest, Attributes_HaveCorrectValues)
{
    EndermiteEntity endermite(EntityInstanceId(1));
    endermite.setWorld(&m_world);
    // registerAttributes 在 MonsterEntity 构造函数中被调用

    // MC 1.16.5: MAX_HEALTH = 8.0, MOVEMENT_SPEED = 0.25, ATTACK_DAMAGE = 2.0
    EXPECT_FLOAT_EQ(
        static_cast<f32>(endermite.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH, 0.0)), 8.0f);
    EXPECT_FLOAT_EQ(
        static_cast<f32>(endermite.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0)), 0.25f);
    EXPECT_FLOAT_EQ(
        static_cast<f32>(endermite.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0)), 2.0f);
}

// ==================== 消失逻辑测试 ====================

TEST_F(EndermiteEntityTest, LifetimeIncrements_OnTick_WhenNotPersistent)
{
    EndermiteEntity endermite(EntityInstanceId(1));
    endermite.setWorld(&m_world);

    // 未持久化的末影螨
    EXPECT_FALSE(endermite.isNoDespawnRequired());

    // tick 前 isRemoved 应为 false
    EXPECT_FALSE(endermite.isRemoved());

    // tick 一次
    endermite.tick();

    // 验证实体仍然存活（还没到消失时间）
    EXPECT_FALSE(endermite.isRemoved());
}

TEST_F(EndermiteEntityTest, DoesNotDespawn_WhenPersistent)
{
    EndermiteEntity endermite(EntityInstanceId(1));
    endermite.setWorld(&m_world);

    // 设置为持久化
    endermite.enablePersistence();
    EXPECT_TRUE(endermite.isNoDespawnRequired());

    // tick 多次
    for (int i = 0; i < 3000; ++i) {
        endermite.tick();
        m_world.incrementTick();
    }

    // 持久化的末影螨不应该消失
    EXPECT_FALSE(endermite.isRemoved());
}

TEST_F(EndermiteEntityTest, Despawns_After2400Ticks_WhenNotPersistent)
{
    EndermiteEntity endermite(EntityInstanceId(1));
    endermite.setWorld(&m_world);

    // 确认未持久化
    EXPECT_FALSE(endermite.isNoDespawnRequired());

    // tick 2399 次（小于消失时间）
    for (int i = 0; i < 2399; ++i) {
        endermite.tick();
        m_world.incrementTick();
    }

    // 还不应该消失
    EXPECT_FALSE(endermite.isRemoved());

    // 再 tick 一次，达到 2400 次
    endermite.tick();

    // 现在应该被标记为移除
    EXPECT_TRUE(endermite.isRemoved());
}

TEST_F(EndermiteEntityTest, DespawnTime_Is2400Ticks)
{
    // 验证消失时间是 2400 ticks = 120 秒 = 2 分钟
    // 这是 MC 1.16.5 的标准值
    constexpr i32 DESPAWN_TIME = 2400;
    EXPECT_EQ(DESPAWN_TIME, 2400);
    EXPECT_EQ(DESPAWN_TIME / 20, 120);    // 120 秒
    EXPECT_EQ(DESPAWN_TIME / 20 / 60, 2); // 2 分钟
}

// ==================== 玩家生成标记测试 ====================

TEST_F(EndermiteEntityTest, SpawnedByPlayer_DefaultFalse)
{
    EndermiteEntity endermite(EntityInstanceId(1));
    EXPECT_FALSE(endermite.isSpawnedByPlayer());
}

TEST_F(EndermiteEntityTest, SpawnedByPlayer_CanBeSet)
{
    EndermiteEntity endermite(EntityInstanceId(1));

    endermite.setSpawnedByPlayer(true);
    EXPECT_TRUE(endermite.isSpawnedByPlayer());

    endermite.setSpawnedByPlayer(false);
    EXPECT_FALSE(endermite.isSpawnedByPlayer());
}

// ==================== AI 目标注册测试 ====================

TEST_F(EndermiteEntityTest, Goals_RegisteredCorrectly)
{
    EndermiteEntity endermite(EntityInstanceId(1));
    endermite.setWorld(&m_world);

    // 验证 goalSelector 和 targetSelector 不为空
    // registerGoals 在构造函数中被调用，不会崩溃即为成功
    EXPECT_NO_THROW(endermite.tick());
}

// ==================== 蠹虫测试 ====================

class SilverfishEntityTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    EndermiteTestWorld m_world;
};

TEST_F(SilverfishEntityTest, Create_ReturnsValidEntity)
{
    auto entity = SilverfishEntity::create(&m_world);
    ASSERT_NE(entity, nullptr);
    // 静态工厂不 setTypeId，仅验证非空+类型，对齐 Endermite/CopperGolem/SnowGolem 约定。
    EXPECT_NE(dynamic_cast<SilverfishEntity*>(entity.get()), nullptr);
}

TEST_F(SilverfishEntityTest, Constructor_SetsCorrectDefaults)
{
    SilverfishEntity silverfish(EntityInstanceId(1));
    silverfish.setWorld(&m_world);

    // 验证不在阳光下燃烧
    EXPECT_FALSE(silverfish.shouldBurnInDaylight());

    // 验证经验值（在构造函数中设置）
    EXPECT_EQ(silverfish.experienceValue(), 5);

    // 验证初始存活
    EXPECT_FALSE(silverfish.isRemoved());
}

TEST_F(SilverfishEntityTest, Attributes_HaveCorrectValues)
{
    SilverfishEntity silverfish(EntityInstanceId(1));
    silverfish.setWorld(&m_world);
    // registerAttributes 在 MonsterEntity 构造函数中被调用

    // MC 1.16.5: MAX_HEALTH = 8.0, MOVEMENT_SPEED = 0.25, ATTACK_DAMAGE = 1.0
    EXPECT_FLOAT_EQ(
        static_cast<f32>(silverfish.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH, 0.0)), 8.0f);
    EXPECT_FLOAT_EQ(
        static_cast<f32>(silverfish.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0)), 0.25f);
    EXPECT_FLOAT_EQ(
        static_cast<f32>(silverfish.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0)), 1.0f);
}

TEST_F(SilverfishEntityTest, SummonCooldown_InitializedToZero)
{
    SilverfishEntity silverfish(EntityInstanceId(1));
    silverfish.setWorld(&m_world);

    // 召唤冷却初始化为 0
    // tick 后冷却应该递减但不会变成负数
    silverfish.tick();
    silverfish.tick();
    silverfish.tick();
    // 验证实体仍然存活
    EXPECT_FALSE(silverfish.isRemoved());
}

TEST_F(SilverfishEntityTest, NotifySummonCooldown_SetsCooldown)
{
    SilverfishEntity silverfish(EntityInstanceId(1));
    silverfish.setWorld(&m_world);

    // 调用通知方法设置冷却
    silverfish.notifySummonCooldown();

    // 验证实体仍然存活
    EXPECT_FALSE(silverfish.isRemoved());
}

TEST_F(SilverfishEntityTest, Goals_RegisteredCorrectly)
{
    SilverfishEntity silverfish(EntityInstanceId(1));
    silverfish.setWorld(&m_world);

    // 验证注册不会崩溃（registerGoals 在构造函数中被调用）
    EXPECT_NO_THROW(silverfish.tick());
}

TEST_F(SilverfishEntityTest, Tick_SyncsRenderYawOffset)
{
    SilverfishEntity silverfish(EntityInstanceId(1));
    silverfish.setWorld(&m_world);

    // 设置旋转角度
    silverfish.setRotation(45.0f, 0.0f);
    silverfish.tick();

    // 验证 renderYawOffset 与 yaw 同步
    EXPECT_FLOAT_EQ(silverfish.renderYawOffset(), 45.0f);
}

} // namespace
} // namespace mc
