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

#include "world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/blocks/special/BarrierBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/Fluids.hpp"
#include "world/spawn/MobSpawnInfo.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>

namespace mc {
namespace test {

namespace {

class SpawnPlacementTestWorld final : public world::spawn::ISpawnWorldReader, public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return getAirState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = state != nullptr ? state : getAirState();
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }

    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override
    {
        i32 highest = 0;
        for (const auto& entry : m_blocks) {
            if (entry.first.x == x && entry.first.z == z && entry.second != nullptr && !entry.second->isAir()) {
                const i32 candidateHeight = entry.first.y + 1;
                if (candidateHeight > highest) {
                    highest = candidateHeight;
                }
            }
        }
        return highest;
    }

    [[nodiscard]] i32 getHeight(HeightmapType type, i32 x, i32 z) const override
    {
        (void)type;
        return getHeight(x, z);
    }

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override
    {
        (void)x;
        (void)y;
        (void)z;
        return static_cast<BiomeId>(0);
    }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] bool isInWorldBounds(i32 x, i32 y, i32 z) const override { return isWithinWorldBounds(x, y, z); }

    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }

    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }

    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] DimensionId dimension() const override { return static_cast<DimensionId>(0); }

    [[nodiscard]] u64 seed() const override { return 0; }

    [[nodiscard]] u64 currentTick() const override { return 0; }

    [[nodiscard]] i64 dayTime() const override { return 0; }

    [[nodiscard]] bool isHardcore() const override { return false; }

    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }

    [[nodiscard]] i32 getMaxLocalRawBrightness(i32, i32, i32) const override { return 15; }

    [[nodiscard]] bool isClientSide() const override { return false; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SpawnPlacementTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SpawnPlacementTestWorld::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override
    {
        throw std::runtime_error("SpawnPlacementTestWorld::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override
    {
        throw std::runtime_error("SpawnPlacementTestWorld::getRandom not implemented");
    }

    // WorldBorder interface
    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    [[nodiscard]] const BlockState* getAirState() const { return &VanillaBlocks::AIR->defaultState(); }

    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    world::border::WorldBorder m_worldBorder;
};

class SupportBlock final : public Block {
public:
    explicit SupportBlock(const BlockProperties& properties)
        : Block(properties)
    {}

    [[nodiscard]] bool isSolid(const BlockState& state) const override
    {
        (void)state;
        return false;
    }

    [[nodiscard]] bool isSolidSide(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const override
    {
        (void)state;
        (void)world;
        (void)pos;
        return side == Direction::Up;
    }
};

} // namespace

/**
 * @brief EntitySpawnPlacementRegistry 测试套件
 *
 * 测试实体生成位置规则的注册和查询。
 */
class EntitySpawnPlacementRegistryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化注册表
        VanillaBlocks::initialize();
        world::spawn::EntitySpawnPlacementRegistry::initializeDefaults();
    }
};

// ========== 基本功能测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, IsInitialized)
{
    EXPECT_TRUE(world::spawn::EntitySpawnPlacementRegistry::isInitialized());
}

TEST_F(EntitySpawnPlacementRegistryTest, GetPlacementTypeForKnownEntity)
{
    // 测试已知实体的放置类型
    auto pigType = world::spawn::EntitySpawnPlacementRegistry::getPlacementType("minecraft:pig");
    EXPECT_EQ(pigType, world::spawn::PlacementType::OnGround);

    auto codType = world::spawn::EntitySpawnPlacementRegistry::getPlacementType("minecraft:cod");
    EXPECT_EQ(codType, world::spawn::PlacementType::InWater);

    auto striderType = world::spawn::EntitySpawnPlacementRegistry::getPlacementType("minecraft:strider");
    EXPECT_EQ(striderType, world::spawn::PlacementType::InLava);

    auto phantomType = world::spawn::EntitySpawnPlacementRegistry::getPlacementType("minecraft:phantom");
    EXPECT_EQ(phantomType, world::spawn::PlacementType::NoRestrictions);
}

TEST_F(EntitySpawnPlacementRegistryTest, GetPlacementTypeForUnknownEntity)
{
    // 未知实体应该返回 NoRestrictions
    auto unknownType = world::spawn::EntitySpawnPlacementRegistry::getPlacementType("minecraft:unknown_entity");
    EXPECT_EQ(unknownType, world::spawn::PlacementType::NoRestrictions);
}

TEST_F(EntitySpawnPlacementRegistryTest, GetHeightmapTypeForKnownEntity)
{
    // 测试已知实体的高度图类型
    auto pigHeightmap = world::spawn::EntitySpawnPlacementRegistry::getHeightmapType("minecraft:pig");
    EXPECT_EQ(pigHeightmap, HeightmapType::MotionBlockingNoLeaves);

    auto ocelotHeightmap = world::spawn::EntitySpawnPlacementRegistry::getHeightmapType("minecraft:ocelot");
    EXPECT_EQ(ocelotHeightmap, HeightmapType::MotionBlocking);
}

TEST_F(EntitySpawnPlacementRegistryTest, GetHeightmapTypeForUnknownEntity)
{
    // 未知实体应该返回默认高度图类型
    auto unknownHeightmap = world::spawn::EntitySpawnPlacementRegistry::getHeightmapType("minecraft:unknown_entity");
    EXPECT_EQ(unknownHeightmap, HeightmapType::MotionBlockingNoLeaves);
}

// ========== SpawnReason 枚举测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, SpawnReasonValues)
{
    // 验证 SpawnReason 枚举值（与 MC 1.16.5 对齐）
    // 参考: net.minecraft.entity.SpawnReason
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Natural), 0);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::ChunkGeneration), 1);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Spawner), 2);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Structure), 3);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Breeding), 4);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::MobSummons), 5);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Jockey), 6);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Event), 7);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Conversion), 8);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Reinforcement), 9);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Trigger), 10);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Bucket), 11);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::SpawnEgg), 12);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Command), 13);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Dispenser), 14);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Patrol), 15);
}

TEST_F(EntitySpawnPlacementRegistryTest, SpawnReasonNameConversion)
{
    // 测试 SpawnReason 名称转换函数
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Natural), "natural");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::ChunkGeneration), "chunk_generation");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Spawner), "spawner");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Structure), "structure");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Breeding), "breeding");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::MobSummons), "mob_summons");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Jockey), "jockey");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Event), "event");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Conversion), "conversion");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Reinforcement), "reinforcement");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Trigger), "trigger");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Bucket), "bucket");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::SpawnEgg), "spawn_egg");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Command), "command");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Dispenser), "dispenser");
    EXPECT_STREQ(world::spawn::getSpawnReasonName(world::spawn::SpawnReason::Patrol), "patrol");
}

TEST_F(EntitySpawnPlacementRegistryTest, SpawnReasonFromName)
{
    // 测试从名称获取 SpawnReason
    EXPECT_EQ(world::spawn::getSpawnReasonByName("natural"), world::spawn::SpawnReason::Natural);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("chunk_generation"), world::spawn::SpawnReason::ChunkGeneration);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("spawner"), world::spawn::SpawnReason::Spawner);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("structure"), world::spawn::SpawnReason::Structure);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("breeding"), world::spawn::SpawnReason::Breeding);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("mob_summons"), world::spawn::SpawnReason::MobSummons);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("jockey"), world::spawn::SpawnReason::Jockey);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("event"), world::spawn::SpawnReason::Event);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("conversion"), world::spawn::SpawnReason::Conversion);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("reinforcement"), world::spawn::SpawnReason::Reinforcement);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("trigger"), world::spawn::SpawnReason::Trigger);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("bucket"), world::spawn::SpawnReason::Bucket);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("spawn_egg"), world::spawn::SpawnReason::SpawnEgg);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("command"), world::spawn::SpawnReason::Command);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("dispenser"), world::spawn::SpawnReason::Dispenser);
    EXPECT_EQ(world::spawn::getSpawnReasonByName("patrol"), world::spawn::SpawnReason::Patrol);

    // 测试无效名称返回默认值 Natural
    EXPECT_EQ(world::spawn::getSpawnReasonByName("invalid"), world::spawn::SpawnReason::Natural);
    EXPECT_EQ(world::spawn::getSpawnReasonByName(""), world::spawn::SpawnReason::Natural);
}

TEST_F(EntitySpawnPlacementRegistryTest, SpawnReasonRoundTrip)
{
    // 测试名称转换的往返一致性
    for (int i = 0; i <= 15; ++i) {
        auto reason = static_cast<world::spawn::SpawnReason>(i);
        const char* name = world::spawn::getSpawnReasonName(reason);
        world::spawn::SpawnReason converted = world::spawn::getSpawnReasonByName(name);
        EXPECT_EQ(converted, reason) << "Failed for reason index " << i;
    }
}

TEST_F(EntitySpawnPlacementRegistryTest, IsSpawnerReason)
{
    // Spawner 应该返回 true
    EXPECT_TRUE(world::spawn::isSpawnerReason(world::spawn::SpawnReason::Spawner));

    // 其他生成原因应该返回 false
    EXPECT_FALSE(world::spawn::isSpawnerReason(world::spawn::SpawnReason::Natural));
    EXPECT_FALSE(world::spawn::isSpawnerReason(world::spawn::SpawnReason::ChunkGeneration));
    EXPECT_FALSE(world::spawn::isSpawnerReason(world::spawn::SpawnReason::Structure));
    EXPECT_FALSE(world::spawn::isSpawnerReason(world::spawn::SpawnReason::Event));
    EXPECT_FALSE(world::spawn::isSpawnerReason(world::spawn::SpawnReason::SpawnEgg));
    EXPECT_FALSE(world::spawn::isSpawnerReason(world::spawn::SpawnReason::Command));
}

// ========== PlacementType 枚举测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, PlacementTypeValues)
{
    // 验证 PlacementType 枚举值
    EXPECT_EQ(static_cast<int>(world::spawn::PlacementType::OnGround), 0);
    EXPECT_EQ(static_cast<int>(world::spawn::PlacementType::InWater), 1);
    EXPECT_EQ(static_cast<int>(world::spawn::PlacementType::InLava), 2);
    EXPECT_EQ(static_cast<int>(world::spawn::PlacementType::NoRestrictions), 3);
}

// ========== 注册陆生动物测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, LandAnimalsRegistered)
{
    // 验证陆生动物已注册
    const auto* pigEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:pig");
    ASSERT_NE(pigEntry, nullptr);
    EXPECT_EQ(pigEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* cowEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:cow");
    ASSERT_NE(cowEntry, nullptr);
    EXPECT_EQ(cowEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* sheepEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:sheep");
    ASSERT_NE(sheepEntry, nullptr);
    EXPECT_EQ(sheepEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* chickenEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:chicken");
    ASSERT_NE(chickenEntry, nullptr);
    EXPECT_EQ(chickenEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* horseEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:horse");
    ASSERT_NE(horseEntry, nullptr);
    EXPECT_EQ(horseEntry->placementType, world::spawn::PlacementType::OnGround);
}

// ========== 注册水生生物测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, WaterCreaturesRegistered)
{
    // 验证水生生物已注册
    const auto* codEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:cod");
    ASSERT_NE(codEntry, nullptr);
    EXPECT_EQ(codEntry->placementType, world::spawn::PlacementType::InWater);

    const auto* salmonEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:salmon");
    ASSERT_NE(salmonEntry, nullptr);
    EXPECT_EQ(salmonEntry->placementType, world::spawn::PlacementType::InWater);

    const auto* squidEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:squid");
    ASSERT_NE(squidEntry, nullptr);
    EXPECT_EQ(squidEntry->placementType, world::spawn::PlacementType::InWater);

    const auto* dolphinEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:dolphin");
    ASSERT_NE(dolphinEntry, nullptr);
    EXPECT_EQ(dolphinEntry->placementType, world::spawn::PlacementType::InWater);
}

// ========== 注册怪物测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, MonstersRegistered)
{
    // 验证怪物已注册
    const auto* zombieEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:zombie");
    ASSERT_NE(zombieEntry, nullptr);
    EXPECT_EQ(zombieEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* skeletonEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:skeleton");
    ASSERT_NE(skeletonEntry, nullptr);
    EXPECT_EQ(skeletonEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* creeperEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:creeper");
    ASSERT_NE(creeperEntry, nullptr);
    EXPECT_EQ(creeperEntry->placementType, world::spawn::PlacementType::OnGround);
}

// ========== 注册岩浆生物测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, LavaCreaturesRegistered)
{
    // 验证岩浆生物已注册
    const auto* striderEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:strider");
    ASSERT_NE(striderEntry, nullptr);
    EXPECT_EQ(striderEntry->placementType, world::spawn::PlacementType::InLava);
}

// ========== SpawnCosts 测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, SpawnCostsDefaultValues)
{
    world::spawn::SpawnCosts costs;
    EXPECT_DOUBLE_EQ(costs.energyBudget, 0.0);
    EXPECT_DOUBLE_EQ(costs.charge, 0.0);
    EXPECT_FALSE(costs.isValid());
}

TEST_F(EntitySpawnPlacementRegistryTest, SpawnCostsValidValues)
{
    world::spawn::SpawnCosts costs(1.0, 0.5);
    EXPECT_DOUBLE_EQ(costs.energyBudget, 1.0);
    EXPECT_DOUBLE_EQ(costs.charge, 0.5);
    EXPECT_TRUE(costs.isValid());
}

TEST_F(EntitySpawnPlacementRegistryTest, SpawnCostsInvalidValues)
{
    world::spawn::SpawnCosts costs1(0.0, 0.5);
    EXPECT_FALSE(costs1.isValid());

    world::spawn::SpawnCosts costs2(1.0, 0.0);
    EXPECT_FALSE(costs2.isValid());

    world::spawn::SpawnCosts costs3(0.0, 0.0);
    EXPECT_FALSE(costs3.isValid());
}

TEST_F(EntitySpawnPlacementRegistryTest, OnGroundSpawnUsesSurfaceSupport)
{
    SpawnPlacementTestWorld world;
    SupportBlock supportBlock(BlockProperties(Material::DECORATION).noCollision().notSolid());

    world.setBlockState(0, 63, 0, &supportBlock.defaultState());

    EXPECT_EQ(world.getHeight(0, 0), 64);

    EXPECT_TRUE(world::spawn::EntitySpawnPlacementRegistry::canSpawnAtLocation(
        world::spawn::PlacementType::OnGround, world, Vector3i(0, 64, 0), "minecraft:pig"));

    EXPECT_FALSE(world::spawn::EntitySpawnPlacementRegistry::canSpawnAtLocation(
        world::spawn::PlacementType::OnGround, world, Vector3i(0, 63, 0), "minecraft:pig"));
}

TEST_F(EntitySpawnPlacementRegistryTest, OnGroundSpawnRejectsBarrierLikeBlocks)
{
    SpawnPlacementTestWorld world;
    SupportBlock supportBlock(BlockProperties(Material::DECORATION).noCollision().notSolid());
    blocks::BarrierBlock barrierBlock(BlockProperties(Material::DECORATION).noCollision().notSolid());

    world.setBlockState(0, 63, 0, &supportBlock.defaultState());
    world.setBlockState(0, 64, 0, &barrierBlock.defaultState());

    EXPECT_FALSE(world::spawn::EntitySpawnPlacementRegistry::canSpawnAtLocation(
        world::spawn::PlacementType::OnGround, world, Vector3i(0, 64, 0), "minecraft:pig"));
}

} // namespace test
} // namespace mc
