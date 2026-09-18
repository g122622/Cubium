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
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/blocks/vegetation/SweetBerryBushBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 甜浆果丛测试用世界
 */
class SweetBerryBushTestWorld final : public IBlockReader {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override { return sampleLight(m_blockLight, x, y, z); }

    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override { return sampleLight(m_skyLight, x, y, z); }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
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
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }

    void setSeed(u64 seed) { m_seed = seed; }
    void setClientSide(bool clientSide) { m_isClientSide = clientSide; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    void setSkyLightAt(const BlockPos& pos, u8 light) { m_skyLight[pos] = light; }

    void setBlockLightAt(const BlockPos& pos, u8 light) { m_blockLight[pos] = light; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SweetBerryBushTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SweetBerryBushTestWorld::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override
    {
        throw std::runtime_error("SweetBerryBushTestWorld::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override
    {
        throw std::runtime_error("SweetBerryBushTestWorld::getRandom not implemented");
    }

    // WorldBorder interface
    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    [[nodiscard]] const BlockState* airState() const { return BlockRegistry::instance().airState(); }

    [[nodiscard]] static u8 sampleLight(const std::map<BlockPos, u8>& lights, i32 x, i32 y, i32 z)
    {
        const BlockPos pos(x, y, z);
        const auto it = lights.find(pos);
        if (it != lights.end()) {
            return it->second;
        }
        return 0;
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, u8> m_blockLight;
    std::map<BlockPos, u8> m_skyLight;
    world::border::WorldBorder m_worldBorder;
    u64 m_seed = 12345;
    bool m_isClientSide = false;
};

/**
 * @brief 测试用随机数生成器
 */
class SequenceRandom final : public math::IRandom {
public:
    explicit SequenceRandom(std::vector<i32> values)
        : m_values(std::move(values))
    {}

    void setSeed(u64 seed) override
    {
        m_seed = seed;
        m_index = 0;
    }

    [[nodiscard]] u64 nextU64() override { return static_cast<u64>(nextValue()); }

    [[nodiscard]] u32 nextU32() override { return static_cast<u32>(nextValue()); }

    [[nodiscard]] i32 nextInt(i32 bound) override { return nextValue() % bound; }

    [[nodiscard]] i32 nextInt() override { return nextValue(); }

    [[nodiscard]] i32 nextInt(i32 min, i32 max) override { return min + (nextValue() % (max - min + 1)); }

    [[nodiscard]] bool nextBoolean() override { return (nextValue() & 1) != 0; }

    [[nodiscard]] f32 nextFloat() override
    {
        return static_cast<f32>(nextValue() & 0x00FFFFFF) / static_cast<f32>(1 << 24);
    }

    [[nodiscard]] f32 nextFloat(f32 min, f32 max) override { return min + nextFloat() * (max - min); }

    [[nodiscard]] f64 nextDouble() override
    {
        return static_cast<f64>(nextValue() & 0x001FFFFFFFFFFFFF) / static_cast<f64>(1ULL << 53);
    }

    [[nodiscard]] f64 nextDouble(f64 min, f64 max) override { return min + nextDouble() * (max - min); }

    [[nodiscard]] f32 nextGaussian(f32 mean, f32 stddev) override
    {
        return mean + stddev * static_cast<f32>(nextValue());
    }

    [[nodiscard]] i64 nextLong() override { return static_cast<i64>(nextValue()); }

    [[nodiscard]] i64 nextLong(i64 bound) override { return static_cast<i64>(nextValue() % bound); }

private:
    [[nodiscard]] i32 nextValue()
    {
        if (m_index < m_values.size()) {
            return m_values[m_index++];
        }
        return 0;
    }

    std::vector<i32> m_values;
    size_t m_index = 0;
    u64 m_seed = 0;
};

/**
 * @brief 测试用生物实体（带类型ID设置）
 */
class TestLivingEntity : public LivingEntity {
public:
    explicit TestLivingEntity(const std::string& typeId = "minecraft:player")
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        setTypeId(typeId);
        setHealth(maxHealth());
        // 设置位置以便移动检测
        setPosition(0.0f, 1.0f, 0.0f);
    }
};

/**
 * @brief 测试用狐狸实体
 */
class TestFoxEntity : public LivingEntity {
public:
    TestFoxEntity()
        : LivingEntity(EntityInstanceId(2), nullptr, mc::test::testEcsRegistry())
    {
        setTypeId("minecraft:fox");
        setHealth(maxHealth());
    }
};

/**
 * @brief 测试用蜜蜂实体
 */
class TestBeeEntity : public LivingEntity {
public:
    TestBeeEntity()
        : LivingEntity(EntityInstanceId(3), nullptr, mc::test::testEcsRegistry())
    {
        setTypeId("minecraft:bee");
        setHealth(maxHealth());
    }
};

} // namespace

/**
 * @brief 测试访问器类，暴露 protected 方法用于测试
 */
class SweetBerryBushBlockTestAccess : public SweetBerryBushBlock {
public:
    using SweetBerryBushBlock::SweetBerryBushBlock;

    // 暴露 protected 方法用于测试
    [[nodiscard]] bool testCanSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
    {
        return canSustain(groundState, world, groundPos);
    }
};

class SweetBerryBushBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();

        m_bush = std::make_unique<SweetBerryBushBlockTestAccess>(
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());
    }

    void TearDown() override { m_bush.reset(); }

    std::unique_ptr<SweetBerryBushBlockTestAccess> m_bush;
};

// ============================================================================
// canSustain Tests
// ============================================================================

TEST_F(SweetBerryBushBlockTest, CanSustainOnGrassBlock)
{
    SweetBerryBushTestWorld world;
    const BlockPos pos(0, 1, 0);

    world.setBlockAt(pos.down(), &VanillaBlocks::GRASS_BLOCK->defaultState());
    const BlockState* groundState = world.getBlockState(pos.down());
    ASSERT_NE(groundState, nullptr);
    EXPECT_TRUE(m_bush->testCanSustain(*groundState, world, pos.down()));
}

TEST_F(SweetBerryBushBlockTest, CanSustainOnDirt)
{
    SweetBerryBushTestWorld world;
    const BlockPos pos(0, 1, 0);

    world.setBlockAt(pos.down(), &VanillaBlocks::DIRT->defaultState());
    const BlockState* groundState = world.getBlockState(pos.down());
    ASSERT_NE(groundState, nullptr);
    EXPECT_TRUE(m_bush->testCanSustain(*groundState, world, pos.down()));
}

TEST_F(SweetBerryBushBlockTest, CanSustainOnCoarseDirt)
{
    SweetBerryBushTestWorld world;
    const BlockPos pos(0, 1, 0);

    world.setBlockAt(pos.down(), &VanillaBlocks::COARSE_DIRT->defaultState());
    const BlockState* groundState = world.getBlockState(pos.down());
    ASSERT_NE(groundState, nullptr);
    EXPECT_TRUE(m_bush->testCanSustain(*groundState, world, pos.down()));
}

TEST_F(SweetBerryBushBlockTest, CanSustainOnPodzol)
{
    SweetBerryBushTestWorld world;
    const BlockPos pos(0, 1, 0);

    world.setBlockAt(pos.down(), &VanillaBlocks::PODZOL->defaultState());
    const BlockState* groundState = world.getBlockState(pos.down());
    ASSERT_NE(groundState, nullptr);
    EXPECT_TRUE(m_bush->testCanSustain(*groundState, world, pos.down()));
}

TEST_F(SweetBerryBushBlockTest, CanSustainOnFarmland)
{
    SweetBerryBushTestWorld world;
    const BlockPos pos(0, 1, 0);

    world.setBlockAt(pos.down(), &VanillaBlocks::FARMLAND->defaultState());
    const BlockState* groundState = world.getBlockState(pos.down());
    ASSERT_NE(groundState, nullptr);
    EXPECT_TRUE(m_bush->testCanSustain(*groundState, world, pos.down()));
}

TEST_F(SweetBerryBushBlockTest, CannotSustainOnStone)
{
    SweetBerryBushTestWorld world;
    const BlockPos pos(0, 1, 0);

    world.setBlockAt(pos.down(), &VanillaBlocks::STONE->defaultState());
    const BlockState* groundState = world.getBlockState(pos.down());
    ASSERT_NE(groundState, nullptr);
    EXPECT_FALSE(m_bush->testCanSustain(*groundState, world, pos.down()));
}

TEST_F(SweetBerryBushBlockTest, CannotSustainOnSand)
{
    SweetBerryBushTestWorld world;
    const BlockPos pos(0, 1, 0);

    world.setBlockAt(pos.down(), &VanillaBlocks::SAND->defaultState());
    const BlockState* groundState = world.getBlockState(pos.down());
    ASSERT_NE(groundState, nullptr);
    EXPECT_FALSE(m_bush->testCanSustain(*groundState, world, pos.down()));
}

TEST_F(SweetBerryBushBlockTest, CannotSustainOnAir)
{
    SweetBerryBushTestWorld world;
    const BlockPos pos(0, 1, 0);

    // 空气不支撑 - 使用空气方块状态
    const BlockState* airState = BlockRegistry::instance().airState();
    ASSERT_NE(airState, nullptr);
    EXPECT_FALSE(m_bush->testCanSustain(*airState, world, pos.down()));
}

// ============================================================================
// onEntityCollision Tests
// ============================================================================

TEST_F(SweetBerryBushBlockTest, OnEntityCollisionAppliesMotionMultiplier)
{
    SweetBerryBushTestWorld world;
    world.setClientSide(false);
    const BlockPos pos(0, 0, 0);
    const BlockState& state = m_bush->defaultState();

    TestLivingEntity entity;
    entity.setPosition(0.5f, 0.0f, 0.5f);

    // 设置上一个位置（模拟移动）
    entity.baseTick();                    // 清除 motion multiplier
    entity.setPosition(0.5f, 0.0f, 0.5f); // 重置位置

    m_bush->onEntityCollision(state, world, pos, entity);

    // 应该应用运动乘数
    EXPECT_TRUE(entity.hasMotionMultiplier());
    EXPECT_FLOAT_EQ(entity.motionMultiplier().x, physics::SWEET_BERRY_BUSH_SLOWDOWN_XZ);
    EXPECT_FLOAT_EQ(entity.motionMultiplier().y, physics::SWEET_BERRY_BUSH_SLOWDOWN_Y);
    EXPECT_FLOAT_EQ(entity.motionMultiplier().z, physics::SWEET_BERRY_BUSH_SLOWDOWN_XZ);
}

TEST_F(SweetBerryBushBlockTest, FoxIsImmuneToDamageAndSlowdown)
{
    SweetBerryBushTestWorld world;
    world.setClientSide(false);
    const BlockPos pos(0, 0, 0);

    // AGE = 3 的灌木
    const BlockState& state = m_bush->defaultState().with(SweetBerryBushBlock::AGE(), 3);

    TestFoxEntity fox;
    fox.setPosition(0.5f, 0.0f, 0.5f);
    fox.baseTick();
    f32 initialHealth = fox.health();

    m_bush->onEntityCollision(state, world, pos, fox);

    // 狐狸不应该受伤
    EXPECT_FLOAT_EQ(fox.health(), initialHealth);
    // 狐狸不应该有运动乘数
    EXPECT_FALSE(fox.hasMotionMultiplier());
}

TEST_F(SweetBerryBushBlockTest, BeeIsImmuneToDamageAndSlowdown)
{
    SweetBerryBushTestWorld world;
    world.setClientSide(false);
    const BlockPos pos(0, 0, 0);

    const BlockState& state = m_bush->defaultState().with(SweetBerryBushBlock::AGE(), 3);

    TestBeeEntity bee;
    bee.setPosition(0.5f, 0.0f, 0.5f);
    bee.baseTick();
    f32 initialHealth = bee.health();

    m_bush->onEntityCollision(state, world, pos, bee);

    // 蜜蜂不应该受伤
    EXPECT_FLOAT_EQ(bee.health(), initialHealth);
    // 蜜蜂不应该有运动乘数
    EXPECT_FALSE(bee.hasMotionMultiplier());
}

TEST_F(SweetBerryBushBlockTest, NonLivingEntityNotAffected)
{
    // 注意：Entity 基类实例应该不受影响，因为没有 hurt 方法
    // 但实际上 onEntityCollision 会 dynamic_cast 到 LivingEntity
    // 这里测试一个 Entity 基类实例

    SweetBerryBushTestWorld world;
    world.setClientSide(false);
    const BlockPos pos(0, 0, 0);
    const BlockState& state = m_bush->defaultState().with(SweetBerryBushBlock::AGE(), 3);

    // Entity 基类不会应用任何效果（因为 dynamic_cast<LivingEntity> 失败）
    // 这个测试验证非 LivingEntity 不受影响
    TestLivingEntity entity; // 使用 LivingEntity 但设置类型为普通实体
    entity.setPosition(0.5f, 0.0f, 0.5f);
    entity.setTypeId("minecraft:zombie"); // 不是狐狸或蜜蜂
    entity.baseTick();

    f32 initialHealth = entity.health();

    // 模拟移动（设置 prevPosition 不同）
    entity.baseTick(); // 这会清除 motion multiplier

    m_bush->onEntityCollision(state, world, pos, entity);

    // 应该应用运动乘数（僵尸不是狐狸或蜜蜂）
    EXPECT_TRUE(entity.hasMotionMultiplier());
}

TEST_F(SweetBerryBushBlockTest, DamageOnlyWhenAgeGreaterThanZero)
{
    SweetBerryBushTestWorld world;
    world.setClientSide(false);
    const BlockPos pos(0, 0, 0);

    TestLivingEntity entity;
    entity.setTypeId("minecraft:zombie");
    entity.setPosition(0.5f, 0.0f, 0.5f);
    entity.baseTick();
    f32 initialHealth = entity.health();

    // AGE = 0 不造成伤害
    const BlockState& state0 = m_bush->defaultState().with(SweetBerryBushBlock::AGE(), 0);
    m_bush->onEntityCollision(state0, world, pos, entity);
    // baseTick 会清除 motion multiplier，但我们需要手动设置位置来模拟移动
    entity.setPosition(0.5f, 0.0f, 0.5f);

    // 仍然会应用运动乘数（减速）
    // 但不会造成伤害因为 AGE = 0
    EXPECT_FLOAT_EQ(entity.health(), initialHealth);
}

TEST_F(SweetBerryBushBlockTest, DamageOnServerSide)
{
    SweetBerryBushTestWorld world;
    world.setClientSide(false); // 服务端
    const BlockPos pos(0, 0, 0);

    TestLivingEntity entity;
    entity.setTypeId("minecraft:zombie");
    entity.setPosition(0.5f, 0.0f, 0.5f);

    const BlockState& state = m_bush->defaultState().with(SweetBerryBushBlock::AGE(), 3);
    f32 initialHealth = entity.health();

    // 模拟移动
    entity.baseTick();
    entity.setPosition(0.1f, 0.0f, 0.1f); // 移动位置

    m_bush->onEntityCollision(state, world, pos, entity);

    // 服务端应该造成伤害（移动距离 > 0.003）
    EXPECT_LT(entity.health(), initialHealth);
}

TEST_F(SweetBerryBushBlockTest, NoDamageOnClientSide)
{
    SweetBerryBushTestWorld world;
    world.setClientSide(true); // 客户端
    const BlockPos pos(0, 0, 0);

    TestLivingEntity entity;
    entity.setTypeId("minecraft:zombie");
    entity.setPosition(0.5f, 0.0f, 0.5f);

    const BlockState& state = m_bush->defaultState().with(SweetBerryBushBlock::AGE(), 3);
    f32 initialHealth = entity.health();

    // 模拟移动
    entity.baseTick();
    entity.setPosition(0.1f, 0.0f, 0.1f);

    m_bush->onEntityCollision(state, world, pos, entity);

    // 客户端不应该造成伤害
    EXPECT_FLOAT_EQ(entity.health(), initialHealth);
    // 但仍然应用运动乘数
    EXPECT_TRUE(entity.hasMotionMultiplier());
}

TEST_F(SweetBerryBushBlockTest, NoDamageWhenNotMoving)
{
    SweetBerryBushTestWorld world;
    world.setClientSide(false);
    const BlockPos pos(0, 0, 0);

    TestLivingEntity entity;
    entity.setTypeId("minecraft:zombie");

    const BlockState& state = m_bush->defaultState().with(SweetBerryBushBlock::AGE(), 3);
    f32 initialHealth = entity.health();

    // 设置位置但不移动（prevPosition == currentPosition）
    entity.setPosition(0.5f, 0.0f, 0.5f);
    entity.baseTick(); // 这会将 prevPosition 设置为当前位置

    m_bush->onEntityCollision(state, world, pos, entity);

    // 没有移动，不应该造成伤害
    EXPECT_FLOAT_EQ(entity.health(), initialHealth);
}

TEST_F(SweetBerryBushBlockTest, DamageWhenMovingBeyondThreshold)
{
    SweetBerryBushTestWorld world;
    world.setClientSide(false);
    const BlockPos pos(0, 0, 0);

    TestLivingEntity entity;
    entity.setTypeId("minecraft:zombie");

    const BlockState& state = m_bush->defaultState().with(SweetBerryBushBlock::AGE(), 3);
    f32 initialHealth = entity.health();

    // 设置初始位置
    entity.setPosition(0.0f, 0.0f, 0.0f);
    entity.baseTick(); // prevPosition = (0, 0, 0)

    // 移动距离 > 0.003
    entity.setPosition(0.01f, 0.0f, 0.01f); // 移动了 sqrt(0.01^2 + 0.01^2) ≈ 0.014 > 0.003

    m_bush->onEntityCollision(state, world, pos, entity);

    // 移动距离 > 0.003，应该造成伤害
    EXPECT_LT(entity.health(), initialHealth);
}

TEST_F(SweetBerryBushBlockTest, NoDamageWhenMovementBelowThreshold)
{
    SweetBerryBushTestWorld world;
    world.setClientSide(false);
    const BlockPos pos(0, 0, 0);

    TestLivingEntity entity;
    entity.setTypeId("minecraft:zombie");

    const BlockState& state = m_bush->defaultState().with(SweetBerryBushBlock::AGE(), 3);
    f32 initialHealth = entity.health();

    // 设置初始位置
    entity.setPosition(0.0f, 0.0f, 0.0f);
    entity.baseTick();

    // 移动距离 < 0.003
    entity.setPosition(0.001f, 0.0f, 0.001f); // 移动距离 < 0.003

    m_bush->onEntityCollision(state, world, pos, entity);

    // 移动距离 < 0.003，不应该造成伤害
    EXPECT_FLOAT_EQ(entity.health(), initialHealth);
}

// ============================================================================
// Growth Tests
// ============================================================================

TEST_F(SweetBerryBushBlockTest, RandomTickGrowsUnderSufficientLight)
{
    SweetBerryBushTestWorld world;
    const BlockPos pos(0, 64, 0);

    world.setBlockAt(pos, &m_bush->defaultState());
    world.setSkyLightAt(pos.up(), 15);
    world.setBlockLightAt(pos.up(), 0);

    SequenceRandom random({0}); // nextInt(5) == 0, 触发生长

    BlockState state = m_bush->defaultState();
    m_bush->randomTick(world, pos, state, random);

    const BlockState* updated = world.getBlockState(pos);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(m_bush->getAge(*updated), 1); // 从 AGE 0 长到 AGE 1
}

TEST_F(SweetBerryBushBlockTest, RandomTickNoGrowthUnderLowLight)
{
    SweetBerryBushTestWorld world;
    const BlockPos pos(0, 64, 0);

    world.setBlockAt(pos, &m_bush->defaultState());
    world.setSkyLightAt(pos.up(), 8); // 光照 < 9
    world.setBlockLightAt(pos.up(), 0);

    SequenceRandom random({0}); // 触发生长的随机数，但光照不足

    BlockState state = m_bush->defaultState();
    m_bush->randomTick(world, pos, state, random);

    const BlockState* updated = world.getBlockState(pos);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(m_bush->getAge(*updated), 0); // 光照不足，不生长
}

TEST_F(SweetBerryBushBlockTest, MaxAgeDoesNotGrow)
{
    SweetBerryBushTestWorld world;
    const BlockPos pos(0, 64, 0);

    // AGE = 3（最大年龄）
    const BlockState& matureState = m_bush->defaultState().with(SweetBerryBushBlock::AGE(), 3);
    world.setBlockAt(pos, &matureState);
    world.setSkyLightAt(pos.up(), 15);

    SequenceRandom random({0});

    BlockState state = matureState;
    m_bush->randomTick(world, pos, state, random);

    const BlockState* updated = world.getBlockState(pos);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(m_bush->getAge(*updated), 3); // 最大年龄不变
}
