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
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用 Mock World，支持 getBlockState、getBrightness 等方法
 */
class PathWeightTestWorld final : public mc::test::BaseTestWorld {
public:
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_blocks[BlockPos(x, y, z)] = state; }

    void setBrightness(f32 brightness) { m_brightness = brightness; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] f32 getBrightness(const BlockPos& pos) const override
    {
        (void)pos;
        return m_brightness;
    }

    // Stub implementations
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return 0; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("PathWeightTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("PathWeightTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    f32 m_brightness = 1.0f;
};

// 具体的 AnimalEntity 子类用于测试
class TestAnimalEntity : public AnimalEntity {
public:
    TestAnimalEntity(EntityInstanceId id)
        : AnimalEntity(id, mc::test::testEcsRegistry())
    {}

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& /*partner*/) override
    {
        return std::make_unique<TestAnimalEntity>(EntityInstanceId(0));
    }
};

// 具体的 MonsterEntity 子类用于测试
class TestMonsterEntity : public MonsterEntity {
public:
    TestMonsterEntity(EntityInstanceId id)
        : MonsterEntity(id, mc::test::testEcsRegistry())
    {}
};

// ==================== AnimalEntity::getPathWeight 测试 ====================

TEST(AnimalEntityGetPathWeightTest, ReturnsHighScoreOnGrassBlock)
{
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBlock(0, 63, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());
    world.setBrightness(1.0f);

    TestAnimalEntity animal(EntityInstanceId(1));
    animal.setWorld(&world);

    // 脚下是草方块，应该返回 10.0F
    f32 weight = animal.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 10.0f);
}

TEST(AnimalEntityGetPathWeightTest, ReturnsBrightnessMinusHalfOnNonGrassBlock)
{
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBlock(0, 63, 0, &VanillaBlocks::STONE->defaultState());
    world.setBrightness(1.0f);

    TestAnimalEntity animal(EntityInstanceId(1));
    animal.setWorld(&world);

    // 脚下是石头，亮度 1.0，应该返回 1.0 - 0.5 = 0.5F
    f32 weight = animal.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.5f);
}

TEST(AnimalEntityGetPathWeightTest, ReturnsNegativeScoreInDarkness)
{
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBlock(0, 63, 0, &VanillaBlocks::STONE->defaultState());
    world.setBrightness(0.0f);

    TestAnimalEntity animal(EntityInstanceId(1));
    animal.setWorld(&world);

    // 脚下是石头，亮度 0.0，应该返回 0.0 - 0.5 = -0.5F
    f32 weight = animal.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, -0.5f);
}

TEST(AnimalEntityGetPathWeightTest, ReturnsZeroWhenNoWorld)
{
    VanillaBlocks::initialize();

    TestAnimalEntity animal(EntityInstanceId(1));
    // 没有 world，应该返回 0.0f
    f32 weight = animal.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST(AnimalEntityGetPathWeightTest, PrefersGrassOverHighBrightness)
{
    VanillaBlocks::initialize();

    PathWeightTestWorld world;

    // 位置1: 草方块，低亮度
    world.setBlock(0, 63, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());
    world.setBrightness(0.0f);

    TestAnimalEntity animal(EntityInstanceId(1));
    animal.setWorld(&world);

    f32 grassWeight = animal.getPathWeight(0.0f, 64.0f, 0.0f);

    // 位置2: 石头，高亮度
    world.setBlock(10, 63, 10, &VanillaBlocks::STONE->defaultState());
    world.setBrightness(1.0f);

    f32 stoneWeight = animal.getPathWeight(10.0f, 64.0f, 10.0f);

    // 草方块权重应该高于石头（即使石头在明亮处）
    EXPECT_GT(grassWeight, stoneWeight);
    EXPECT_FLOAT_EQ(grassWeight, 10.0f);
    EXPECT_FLOAT_EQ(stoneWeight, 0.5f);
}

// ==================== MonsterEntity::getPathWeight 测试 ====================

TEST(MonsterEntityGetPathWeightTest, PrefersDarkness)
{
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBrightness(0.0f); // 完全黑暗

    TestMonsterEntity monster(EntityInstanceId(1));
    monster.setWorld(&world);

    // 亮度 0.0，应该返回 0.5 - 0.0 = 0.5F
    f32 weight = monster.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.5f);
}

TEST(MonsterEntityGetPathWeightTest, DislikesBrightness)
{
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBrightness(1.0f); // 完全明亮

    TestMonsterEntity monster(EntityInstanceId(1));
    monster.setWorld(&world);

    // 亮度 1.0，应该返回 0.5 - 1.0 = -0.5F
    f32 weight = monster.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, -0.5f);
}

TEST(MonsterEntityGetPathWeightTest, ReturnsZeroWhenNoWorld)
{
    VanillaBlocks::initialize();

    TestMonsterEntity monster(EntityInstanceId(1));
    // 没有 world，应该返回 0.0f
    f32 weight = monster.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST(MonsterEntityGetPathWeightTest, MediumBrightness)
{
    VanillaBlocks::initialize();

    PathWeightTestWorld world;
    world.setBrightness(0.5f);

    TestMonsterEntity monster(EntityInstanceId(1));
    monster.setWorld(&world);

    // 亮度 0.5，应该返回 0.5 - 0.5 = 0.0F
    f32 weight = monster.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST(MonsterEntityGetPathWeightTest, SlightlyDarkPreferredOverBright)
{
    VanillaBlocks::initialize();

    PathWeightTestWorld world;

    TestMonsterEntity monster(EntityInstanceId(1));
    monster.setWorld(&world);

    // 较暗位置
    world.setBrightness(0.2f);
    f32 darkWeight = monster.getPathWeight(0.0f, 64.0f, 0.0f);

    // 较亮位置
    world.setBrightness(0.8f);
    f32 brightWeight = monster.getPathWeight(10.0f, 64.0f, 10.0f);

    // 怪物应该偏好较暗的位置
    EXPECT_GT(darkWeight, brightWeight);
    EXPECT_FLOAT_EQ(darkWeight, 0.3f);    // 0.5 - 0.2
    EXPECT_FLOAT_EQ(brightWeight, -0.3f); // 0.5 - 0.8
}

// ============================================================================
// AnimalEntity::setInLove 广播 LoveHeart 状态测试
// ============================================================================

/**
 * @brief 支持实体状态广播追踪的测试用世界
 */
class LoveHeartTestWorld final : public mc::test::BaseTestWorld {
public:
    void broadcastEntityStatus(EntityInstanceId entityId, u8 status) override
    {
        m_lastBroadcastEntityId = entityId;
        m_lastBroadcastStatus = status;
        m_broadcastCount++;
    }

    [[nodiscard]] EntityInstanceId getLastBroadcastEntityId() const { return m_lastBroadcastEntityId; }
    [[nodiscard]] u8 getLastBroadcastStatus() const { return m_lastBroadcastStatus; }
    [[nodiscard]] i32 getBroadcastCount() const { return m_broadcastCount; }
    void resetBroadcastTracking()
    {
        m_lastBroadcastEntityId = EntityInstanceId(0);
        m_lastBroadcastStatus = 0;
        m_broadcastCount = 0;
    }

private:
    EntityInstanceId m_lastBroadcastEntityId{0};
    u8 m_lastBroadcastStatus = 0;
    i32 m_broadcastCount = 0;
};

TEST(AnimalEntitySetInLoveTest, BroadcastsLoveHeartStatus)
{
    LoveHeartTestWorld world;
    TestAnimalEntity animal(EntityInstanceId(42));
    animal.setWorld(&world);

    world.resetBroadcastTracking();

    // 调用 setInLove 应该广播 LoveHeart(18) 状态
    animal.setInLove(12345);

    // 验证广播了正确状态
    EXPECT_EQ(world.getBroadcastCount(), 1);
    EXPECT_EQ(world.getLastBroadcastEntityId(), EntityInstanceId(42));
    EXPECT_EQ(world.getLastBroadcastStatus(), static_cast<u8>(network::EntityStatus::LoveHeart));
}

TEST(AnimalEntitySetInLoveTest, SetInLoveWithoutWorldDoesNotCrash)
{
    // 没有 world 的实体调用 setInLove 不应崩溃
    TestAnimalEntity animal(EntityInstanceId(1));
    EXPECT_NO_THROW(animal.setInLove(999));

    // 验证 love 状态确实被设置了
    EXPECT_TRUE(animal.isInLove());
}

TEST(AnimalEntitySetInLoveTest, SetInLoveSetsTimerAndLoveCause)
{
    LoveHeartTestWorld world;
    TestAnimalEntity animal(EntityInstanceId(1));
    animal.setWorld(&world);

    animal.setInLove(12345);

    EXPECT_TRUE(animal.isInLove());
    // 600 ticks = 30 seconds
    EXPECT_EQ(animal.getLoveCause(), 12345);
}

TEST(AnimalEntitySetInLoveTest, ResetInLoveClearsState)
{
    LoveHeartTestWorld world;
    TestAnimalEntity animal(EntityInstanceId(1));
    animal.setWorld(&world);

    animal.setInLove(0);
    EXPECT_TRUE(animal.isInLove());

    animal.resetInLove();
    EXPECT_FALSE(animal.isInLove());
}

} // anonymous namespace
} // namespace mc
