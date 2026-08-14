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

#include <cmath>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "common/entity/entities/monster/basic/GiantEntity.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/entity/entities/monster/ocean/GuardianEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/entity/entities/passive/basic/MooshroomEntity.hpp"
#include "common/entity/entities/passive/special/StriderEntity.hpp"
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/entity/entities/passive/water/WaterMobEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/decorative/CampfireBlock.hpp"
#include "common/world/block/blocks/mob/InfestedBlock.hpp"
#include "common/world/block/registry/BaseBlocks.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

using namespace mc;

namespace {

/**
 * @brief 测试用 Mock World，支持亮度、方块状态、流体状态
 */
class PathWeightOverrideTestWorld final : public mc::test::BaseTestWorld {
public:
    void setBrightness(f32 brightness) { m_brightness = brightness; }
    void setBlockStateAt(i32 x, i32 y, i32 z, const BlockState* state) { m_blockStates[BlockPos(x, y, z)] = state; }
    void setGrassBlockAt(i32 x, i32 y, i32 z)
    {
        m_blockStates[BlockPos(x, y, z)] = &VanillaBlocks::GRASS_BLOCK->defaultState();
    }
    void setMyceliumAt(i32 x, i32 y, i32 z)
    {
        m_blockStates[BlockPos(x, y, z)] = &block_registry::NaturalBlocks::MYCELIUM->defaultState();
    }
    void setWaterAt(i32 x, i32 y, i32 z) { m_blockStates[BlockPos(x, y, z)] = &VanillaBlocks::WATER->defaultState(); }
    void setLavaAt(i32 x, i32 y, i32 z) { m_blockStates[BlockPos(x, y, z)] = &VanillaBlocks::LAVA->defaultState(); }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockStates.find(BlockPos(x, y, z));
        if (it != m_blockStates.end()) {
            return it->second;
        }
        return nullptr;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fs = state->getFluidState();
            if (fs != nullptr && !fs->isEmpty()) {
                return fs;
            }
        }
        return nullptr; // 没有方块或空流体时返回 nullptr
    }

    [[nodiscard]] f32 getBrightness(const BlockPos& pos) const override
    {
        (void)pos;
        return m_brightness;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blockStates[BlockPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return 0; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("PathWeightOverrideTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("PathWeightOverrideTestWorld::tickManager not implemented");
    }

private:
    f32 m_brightness = 0.5f;
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
};

/**
 * @brief 测试用基础生物实体
 */
class TestCreatureEntity final : public CreatureEntity {
public:
    TestCreatureEntity()
        : CreatureEntity(EntityInstanceId(0), mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

/**
 * @brief 测试用具体动物实体
 */
class TestAnimalEntity final : public AnimalEntity {
public:
    TestAnimalEntity()
        : AnimalEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override { return std::nullopt; }
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource&) const override { return std::nullopt; }
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override { return std::nullopt; }
    [[nodiscard]] bool isBreedingItem(const ItemStack&) const override { return false; }
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity&) override { return nullptr; }

protected:
    [[nodiscard]] f32 getBaseWidth() const override { return 0.9f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.9f; }
    [[nodiscard]] f32 eyeHeight() const override { return 0.4f * height(); }
};

/**
 * @brief 测试用具体怪物实体
 */
class TestMonsterEntity final : public MonsterEntity {
public:
    TestMonsterEntity()
        : MonsterEntity(EntityInstanceId(2), mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override { return std::nullopt; }
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource&) const override { return std::nullopt; }
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override { return std::nullopt; }

protected:
    void registerGoals() override { MonsterEntity::registerGoals(); }
};

// ============================================================================
// CreatureEntity::canSpawnAt 测试
// ============================================================================

class CreatureEntityCanSpawnAtTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
    PathWeightOverrideTestWorld world;
};

TEST_F(CreatureEntityCanSpawnAtTest, CreatureCanSpawnAtNeutralWeight)
{
    // CreatureEntity 默认 getPathWeight 返回 0.0f，canSpawnAt 应返回 true
    TestCreatureEntity creature;
    creature.setWorld(&world);
    EXPECT_TRUE(creature.canSpawnAt(0.0f, 64.0f, 0.0f));
}

TEST_F(CreatureEntityCanSpawnAtTest, AnimalCanSpawnAtOnGrass)
{
    // AnimalEntity 在草方块上 getPathWeight 返回 10.0f，canSpawnAt 应返回 true
    world.setGrassBlockAt(0, 63, 0);
    TestAnimalEntity animal;
    animal.setWorld(&world);
    EXPECT_TRUE(animal.canSpawnAt(0.0f, 64.0f, 0.0f));
}

TEST_F(CreatureEntityCanSpawnAtTest, AnimalCannotSpawnInDarkness)
{
    // AnimalEntity 在黑暗中 getPathWeight 返回 -0.5f，canSpawnAt 应返回 false
    world.setBrightness(0.0f);
    TestAnimalEntity animal;
    animal.setWorld(&world);
    EXPECT_FALSE(animal.canSpawnAt(0.0f, 64.0f, 0.0f));
}

TEST_F(CreatureEntityCanSpawnAtTest, MonsterCanSpawnInDarkness)
{
    // MonsterEntity 在黑暗中 getPathWeight 返回 0.5f，canSpawnAt 应返回 true
    world.setBrightness(0.0f);
    TestMonsterEntity monster;
    monster.setWorld(&world);
    EXPECT_TRUE(monster.canSpawnAt(0.0f, 64.0f, 0.0f));
}

TEST_F(CreatureEntityCanSpawnAtTest, MonsterCannotSpawnInBrightLight)
{
    // MonsterEntity 在明亮中 getPathWeight 返回 -0.5f，canSpawnAt 应返回 false
    world.setBrightness(1.0f);
    TestMonsterEntity monster;
    monster.setWorld(&world);
    EXPECT_FALSE(monster.canSpawnAt(0.0f, 64.0f, 0.0f));
}

TEST_F(CreatureEntityCanSpawnAtTest, MonsterAtBoundaryBrightness)
{
    // MonsterEntity 在亮度 0.5 时 getPathWeight 返回 0.0f，canSpawnAt 应返回 true
    world.setBrightness(0.5f);
    TestMonsterEntity monster;
    monster.setWorld(&world);
    EXPECT_TRUE(monster.canSpawnAt(0.0f, 64.0f, 0.0f));
}

// ============================================================================
// WaterMobEntity::getPathWeight 测试
// ============================================================================

class WaterMobPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
    PathWeightOverrideTestWorld world;
};

TEST_F(WaterMobPathWeightTest, ReturnsHighWeightInWater)
{
    // 水生生物在水中应返回 10.0f
    world.setWaterAt(0, 64, 0);

    WaterMobEntity waterMob(EntityInstanceId(10), mc::test::testEcsRegistry());
    waterMob.setWorld(&world);
    EXPECT_FLOAT_EQ(waterMob.getPathWeight(0.0f, 64.0f, 0.0f), 10.0f);
}

TEST_F(WaterMobPathWeightTest, ReturnsZeroOnLand)
{
    // 水生生物在陆地上应返回 0.0f
    WaterMobEntity waterMob(EntityInstanceId(10), mc::test::testEcsRegistry());
    waterMob.setWorld(&world);
    EXPECT_FLOAT_EQ(waterMob.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

TEST_F(WaterMobPathWeightTest, ReturnsZeroWhenNoWorld)
{
    // 没有世界时返回 0.0f
    WaterMobEntity waterMob(EntityInstanceId(10), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(waterMob.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

// ============================================================================
// GuardianEntity::getPathWeight 测试
// ============================================================================

class GuardianPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
    PathWeightOverrideTestWorld world;
};

TEST_F(GuardianPathWeightTest, PrefersWaterOverLand)
{
    // 守卫者在水中权重应高于陆地
    world.setWaterAt(0, 64, 0);
    world.setBrightness(0.5f);

    GuardianEntity guardian(EntityInstanceId(20), mc::test::testEcsRegistry());
    guardian.setWorld(&world);
    f32 waterWeight = guardian.getPathWeight(0.0f, 64.0f, 0.0f);

    // 水中: 10.0 + (0.5 - 0.5) = 10.0
    EXPECT_FLOAT_EQ(waterWeight, 10.0f);
}

TEST_F(GuardianPathWeightTest, ReturnsMonsterWeightOnLand)
{
    // 守卫者在陆地上应使用 MonsterEntity 的权重逻辑
    world.setBrightness(0.0f); // 黑暗中怪物偏好高
    GuardianEntity guardian(EntityInstanceId(20), mc::test::testEcsRegistry());
    guardian.setWorld(&world);
    f32 weight = guardian.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.5f); // MonsterEntity: 0.5 - 0.0 = 0.5
}

TEST_F(GuardianPathWeightTest, ReturnsZeroWhenNoWorld)
{
    GuardianEntity guardian(EntityInstanceId(20), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(guardian.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

// ============================================================================
// StriderEntity::getPathWeight 测试
// ============================================================================

class StriderPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
    PathWeightOverrideTestWorld world;
};

TEST_F(StriderPathWeightTest, PrefersLava)
{
    // 炽足兽在岩浆中应返回 10.0f
    world.setLavaAt(0, 64, 0);

    StriderEntity strider(EntityInstanceId(30), mc::test::testEcsRegistry());
    strider.setWorld(&world);
    f32 lavaWeight = strider.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(lavaWeight, 10.0f);
}

TEST_F(StriderPathWeightTest, ReturnsZeroOnLandWhenNotInLava)
{
    // 炽足兽在陆地上且自身不在岩浆中——返回 0.0f
    StriderEntity strider(EntityInstanceId(30), mc::test::testEcsRegistry());
    strider.setWorld(&world);
    EXPECT_FLOAT_EQ(strider.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

TEST_F(StriderPathWeightTest, ReturnsNegInfOnLandWhenInLava)
{
    // 炽足兽自身在岩浆中，但目标位置不是岩浆——返回 -∞（强烈避免离开岩浆）
    // 对应 MC: isInLava() ? Float.NEGATIVE_INFINITY
    StriderEntity strider(EntityInstanceId(30), mc::test::testEcsRegistry());
    strider.setWorld(&world);
    strider.setInLava(true); // 模拟炽足兽当前站在岩浆中

    // 目标位置没有岩浆
    EXPECT_TRUE(std::isinf(strider.getPathWeight(10.0f, 64.0f, 10.0f)));
    EXPECT_LT(strider.getPathWeight(10.0f, 64.0f, 10.0f), 0.0f);
}

TEST_F(StriderPathWeightTest, ReturnsZeroWhenNoWorld)
{
    StriderEntity strider(EntityInstanceId(30), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(strider.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

// ============================================================================
// MooshroomEntity::getPathWeight 测试
// ============================================================================

class MooshroomPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
    PathWeightOverrideTestWorld world;
};

TEST_F(MooshroomPathWeightTest, PrefersMycelium)
{
    // 哞菇在菌丝上应返回 10.0f
    world.setMyceliumAt(0, 63, 0);

    MooshroomEntity mooshroom(EntityInstanceId(40), mc::test::testEcsRegistry());
    mooshroom.setWorld(&world);
    f32 myceliumWeight = mooshroom.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(myceliumWeight, 10.0f);
}

TEST_F(MooshroomPathWeightTest, FallsBackToAnimalWeightOnNonMycelium)
{
    // 哞菇在非菌丝上应委托 AnimalEntity 的逻辑
    world.setBrightness(1.0f);
    MooshroomEntity mooshroom(EntityInstanceId(40), mc::test::testEcsRegistry());
    mooshroom.setWorld(&world);

    // 不在菌丝上，也不在草方块上，应返回 brightness - 0.5 = 0.5
    f32 weight = mooshroom.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.5f);
}

TEST_F(MooshroomPathWeightTest, PrefersMyceliumOverDarkness)
{
    // 哞菇在菌丝上应优于黑暗位置
    world.setMyceliumAt(0, 63, 0);
    MooshroomEntity mooshroom(EntityInstanceId(40), mc::test::testEcsRegistry());
    mooshroom.setWorld(&world);
    f32 myceliumWeight = mooshroom.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(myceliumWeight, 10.0f);

    // 菌丝位置权重应远高于黑暗非菌丝位置
    world.setBrightness(0.0f);
    f32 darkWeight = mooshroom.getPathWeight(10.0f, 64.0f, 10.0f);
    EXPECT_GT(myceliumWeight, darkWeight);
}

TEST_F(MooshroomPathWeightTest, ReturnsZeroWhenNoWorld)
{
    MooshroomEntity mooshroom(EntityInstanceId(40), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(mooshroom.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

// ============================================================================
// EndermanEntity::getPathWeight 测试
// ============================================================================

class EndermanPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
    PathWeightOverrideTestWorld world;
};

TEST_F(EndermanPathWeightTest, ReturnsZeroInDarkness)
{
    // 末影人不依赖光照，在黑暗中返回 0.0f
    world.setBrightness(0.0f);
    EndermanEntity enderman(EntityInstanceId(50), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    EXPECT_FLOAT_EQ(enderman.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

TEST_F(EndermanPathWeightTest, ReturnsZeroInBrightLight)
{
    // 末影人在明亮中也返回 0.0f
    world.setBrightness(1.0f);
    EndermanEntity enderman(EntityInstanceId(50), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    EXPECT_FLOAT_EQ(enderman.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

TEST_F(EndermanPathWeightTest, AlwaysReturnsZero)
{
    // 末影人始终返回 0.0f
    EndermanEntity enderman(EntityInstanceId(50), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    EXPECT_FLOAT_EQ(enderman.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(enderman.getPathWeight(100.0f, -10.0f, 200.0f), 0.0f);
}

TEST_F(EndermanPathWeightTest, CanSpawnAnywhere)
{
    // 由于 getPathWeight 始终返回 0.0f，canSpawnAt 始终返回 true
    EndermanEntity enderman(EntityInstanceId(50), mc::test::testEcsRegistry());
    enderman.setWorld(&world);
    EXPECT_TRUE(enderman.canSpawnAt(0.0f, 64.0f, 0.0f));
}

// ============================================================================
// GiantEntity::getPathWeight 测试
// ============================================================================

class GiantPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
    PathWeightOverrideTestWorld world;
};

TEST_F(GiantPathWeightTest, PrefersBrightLight)
{
    // MC Giant.getWalkTargetValue: 返回 brightness - 0.5f（不取反）
    // 巨人是唯一偏好明亮区域的 Monster 子类
    world.setBrightness(1.0f);
    GiantEntity giant(EntityInstanceId(60), mc::test::testEcsRegistry());
    giant.setWorld(&world);

    // 亮度 1.0 → 1.0 - 0.5 = 0.5（正值，偏好）
    f32 weight = giant.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.5f);
}

TEST_F(GiantPathWeightTest, AvoidsDarkness)
{
    // 亮度 0.0 → 0.0 - 0.5 = -0.5（负值，避免）
    world.setBrightness(0.0f);
    GiantEntity giant(EntityInstanceId(60), mc::test::testEcsRegistry());
    giant.setWorld(&world);

    f32 weight = giant.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, -0.5f);
}

TEST_F(GiantPathWeightTest, NeutralAtHalfBrightness)
{
    // 亮度 0.5 → 0.5 - 0.5 = 0.0（中性）
    world.setBrightness(0.5f);
    GiantEntity giant(EntityInstanceId(60), mc::test::testEcsRegistry());
    giant.setWorld(&world);

    f32 weight = giant.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST_F(GiantPathWeightTest, ReturnsZeroWhenNoWorld)
{
    // 没有世界时返回 0.0f
    GiantEntity giant(EntityInstanceId(60), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(giant.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

TEST_F(GiantPathWeightTest, OppositeOfNormalMonster)
{
    // 巨人的光照权重与普通怪物相反
    // 普通怪物：0.5 - brightness（偏好黑暗）
    // 巨人：brightness - 0.5（偏好明亮）
    world.setBrightness(1.0f);
    GiantEntity giant(EntityInstanceId(60), mc::test::testEcsRegistry());
    giant.setWorld(&world);
    TestMonsterEntity monster;
    monster.setWorld(&world);

    f32 giantWeight = giant.getPathWeight(0.0f, 64.0f, 0.0f);
    f32 monsterWeight = monster.getPathWeight(0.0f, 64.0f, 0.0f);

    // 在明亮环境中，巨人权重为正，怪物权重为负
    EXPECT_GT(giantWeight, 0.0f);
    EXPECT_LT(monsterWeight, 0.0f);
}

// ============================================================================
// SilverfishEntity::getPathWeight 测试
// ============================================================================

class SilverfishPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
    PathWeightOverrideTestWorld world;
};

TEST_F(SilverfishPathWeightTest, PrefersInfestedBlocks)
{
    // MC Silverfish.getWalkTargetValue: 脚下是可被虫蚀的方块（宿主方块）返回 10.0f
    // canContainSilverfish 检查的是宿主方块（如普通石头），而非虫蚀方块本身
    const BlockState* hostState = &VanillaBlocks::STONE->defaultState();
    world.setBlockStateAt(0, 63, 0, hostState);

    SilverfishEntity silverfish(EntityInstanceId(70), mc::test::testEcsRegistry());
    silverfish.setWorld(&world);

    // getPathWeight 检查脚下方块（y-1 = 63），可被虫蚀的宿主方块返回 10.0f
    f32 weight = silverfish.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 10.0f);
}

TEST_F(SilverfishPathWeightTest, FallsBackToMonsterWeightOnNonInfested)
{
    // 非虫蚀方块：委托给 MonsterEntity 的默认实现
    // MonsterEntity 返回 0.5 - brightness
    world.setBrightness(0.0f); // 黑暗中，MonsterEntity 返回 0.5
    SilverfishEntity silverfish(EntityInstanceId(70), mc::test::testEcsRegistry());
    silverfish.setWorld(&world);

    f32 weight = silverfish.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.5f); // MonsterEntity: 0.5 - 0.0 = 0.5
}

TEST_F(SilverfishPathWeightTest, FallsBackToMonsterWeightInBrightLight)
{
    // 明亮环境且非虫蚀方块：MonsterEntity 返回负值
    world.setBrightness(1.0f);
    SilverfishEntity silverfish(EntityInstanceId(70), mc::test::testEcsRegistry());
    silverfish.setWorld(&world);

    f32 weight = silverfish.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, -0.5f); // MonsterEntity: 0.5 - 1.0 = -0.5
}

TEST_F(SilverfishPathWeightTest, InfestedBlockOverridesDarknessPenalty)
{
    // 即使在黑暗中，可被虫蚀的宿主方块位置也返回 10.0f
    world.setBrightness(0.0f);
    const BlockState* hostState = &VanillaBlocks::STONE->defaultState();
    world.setBlockStateAt(0, 63, 0, hostState);

    SilverfishEntity silverfish(EntityInstanceId(70), mc::test::testEcsRegistry());
    silverfish.setWorld(&world);

    f32 weight = silverfish.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 10.0f);
}

TEST_F(SilverfishPathWeightTest, ReturnsZeroWhenNoWorld)
{
    // 没有世界时返回 0.0f
    SilverfishEntity silverfish(EntityInstanceId(70), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(silverfish.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

// ============================================================================
// HoglinEntity::getPathWeight 测试
// ============================================================================

class HoglinPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
    PathWeightOverrideTestWorld world;
};

TEST_F(HoglinPathWeightTest, PrefersCrimsonNylium)
{
    // MC Hoglin.getWalkTargetValue: 站在绯红菌岩上返回 10.0f
    world.setBlockStateAt(0, 63, 0, &block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState());

    HoglinEntity hoglin(EntityInstanceId(80), mc::test::testEcsRegistry());
    hoglin.setWorld(&world);
    f32 weight = hoglin.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 10.0f);
}

TEST_F(HoglinPathWeightTest, ReturnsZeroOnNonCrimsonNylium)
{
    // 非绯红菌岩方块返回 0.0f
    HoglinEntity hoglin(EntityInstanceId(80), mc::test::testEcsRegistry());
    hoglin.setWorld(&world);
    f32 weight = hoglin.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST_F(HoglinPathWeightTest, ReturnsZeroWhenNoWorld)
{
    // 没有世界时返回 0.0f
    HoglinEntity hoglin(EntityInstanceId(80), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(hoglin.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

TEST_F(HoglinPathWeightTest, WarpedNyliumNotPreferred)
{
    // 诡异菌岩不是疣猪兽偏好方块，应返回 0.0f
    world.setBlockStateAt(0, 63, 0, &block_registry::NetherBlocks::WARPED_NYLIUM->defaultState());

    HoglinEntity hoglin(EntityInstanceId(80), mc::test::testEcsRegistry());
    hoglin.setWorld(&world);
    f32 weight = hoglin.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST_F(HoglinPathWeightTest, CrimsonNyliumOverridesDarknessPreference)
{
    // 绯红菌岩权重 (10.0f) 应远高于普通怪物的黑暗偏好
    world.setBlockStateAt(0, 63, 0, &block_registry::NetherBlocks::CRIMSON_NYLIUM->defaultState());

    HoglinEntity hoglin(EntityInstanceId(80), mc::test::testEcsRegistry());
    hoglin.setWorld(&world);
    f32 weight = hoglin.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 10.0f);

    // 没有绯红菌岩的位置权重应为 0.0f
    f32 nonNyliumWeight = hoglin.getPathWeight(10.0f, 64.0f, 10.0f);
    EXPECT_FLOAT_EQ(nonNyliumWeight, 0.0f);

    // 绯红菌岩权重应高于非绯红菌岩权重
    EXPECT_GT(weight, nonNyliumWeight);
}

// ============================================================================
// PiglinEntity::getPathWeight 测试
// ============================================================================

class PiglinPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
    PathWeightOverrideTestWorld world;
};

TEST_F(PiglinPathWeightTest, RepelledByLitSoulCampfire)
{
    // MC 1.21.11 PiglinSpecificSensor.isValidRepellent:
    // 点燃的灵魂营火属于 PIGLIN_REPELLENTS 标签，应排斥猪灵
    const BlockState& litSoulCampfire = block_registry::NetherBlocks::SOUL_CAMPFIRE->defaultState();
    ASSERT_TRUE(blocks::CampfireBlock::isLitCampfire(litSoulCampfire));
    world.setBlockStateAt(0, 64, 0, &litSoulCampfire);

    PiglinEntity piglin(EntityInstanceId(90), mc::test::testEcsRegistry());
    piglin.setWorld(&world);
    f32 weight = piglin.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, -1.0f);
}

TEST_F(PiglinPathWeightTest, NotRepelledByUnlitSoulCampfire)
{
    // MC 1.21.11 PiglinSpecificSensor.isValidRepellent:
    // 未点燃的灵魂营火不应排斥猪灵
    const BlockState& unlitSoulCampfire =
        block_registry::NetherBlocks::SOUL_CAMPFIRE->defaultState().with(BlockStateProperties::LIT(), false);
    ASSERT_FALSE(blocks::CampfireBlock::isLitCampfire(unlitSoulCampfire));
    world.setBlockStateAt(0, 64, 0, &unlitSoulCampfire);

    PiglinEntity piglin(EntityInstanceId(90), mc::test::testEcsRegistry());
    piglin.setWorld(&world);
    f32 weight = piglin.getPathWeight(0.0f, 64.0f, 0.0f);
    // 未点燃的灵魂营火不排斥，应返回 CreatureEntity 默认值 0.0f
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST_F(PiglinPathWeightTest, RepelledBySoulFire)
{
    // 灵魂火属于 PIGLIN_REPELLENTS 标签，应排斥猪灵
    world.setBlockStateAt(0, 64, 0, &block_registry::NetherBlocks::SOUL_FIRE->defaultState());

    PiglinEntity piglin(EntityInstanceId(90), mc::test::testEcsRegistry());
    piglin.setWorld(&world);
    f32 weight = piglin.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, -1.0f);
}

TEST_F(PiglinPathWeightTest, NotRepelledByWarpedFungus)
{
    // MC 1.21.11 VanillaBlockTagsProvider:
    //   PIGLIN_REPELLENTS = {soul_fire, soul_torch, soul_wall_torch, soul_lantern, soul_campfire}
    //   HOGLIN_REPELLENTS = {warped_fungus, potted_warped_fungus, nether_portal, respawn_anchor}
    // 诡异菌（warped_fungus）不在 PIGLIN_REPELLENTS 中，仅排斥疣猪兽，不排斥猪灵。
    // 因此猪灵在诡异菌附近应返回 CreatureEntity 默认值 0.0f。
    world.setBlockStateAt(0, 64, 0, &block_registry::NetherBlocks::WARPED_FUNGUS->defaultState());

    PiglinEntity piglin(EntityInstanceId(90), mc::test::testEcsRegistry());
    piglin.setWorld(&world);
    f32 weight = piglin.getPathWeight(0.0f, 64.0f, 0.0f);
    // 诡异菌不排斥猪灵，应返回 CreatureEntity 默认值 0.0f
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST_F(PiglinPathWeightTest, NotRepelledByRegularCampfire)
{
    // 普通营火不属于 PIGLIN_REPELLENTS 标签，不应排斥猪灵
    world.setBlockStateAt(0, 64, 0, &block_registry::NetherBlocks::CAMPFIRE->defaultState());

    PiglinEntity piglin(EntityInstanceId(90), mc::test::testEcsRegistry());
    piglin.setWorld(&world);
    f32 weight = piglin.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST_F(PiglinPathWeightTest, NotRepelledWhenNoRepellentsNearby)
{
    // 附近没有排斥物时返回默认值 0.0f
    PiglinEntity piglin(EntityInstanceId(90), mc::test::testEcsRegistry());
    piglin.setWorld(&world);
    f32 weight = piglin.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

TEST_F(PiglinPathWeightTest, ReturnsZeroWhenNoWorld)
{
    // 没有世界时返回 0.0f
    PiglinEntity piglin(EntityInstanceId(90), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(piglin.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

TEST_F(PiglinPathWeightTest, RepelledBySoulCampfireAtDetectionRange)
{
    // 检测范围为水平 8 格、垂直 4 格
    // 在水平范围边缘（8格）放置点燃的灵魂营火，应排斥猪灵
    const BlockState& litSoulCampfire = block_registry::NetherBlocks::SOUL_CAMPFIRE->defaultState();
    world.setBlockStateAt(8, 64, 0, &litSoulCampfire);

    PiglinEntity piglin(EntityInstanceId(90), mc::test::testEcsRegistry());
    piglin.setWorld(&world);
    f32 weight = piglin.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, -1.0f);
}

TEST_F(PiglinPathWeightTest, NotRepelledBySoulCampfireBeyondDetectionRange)
{
    // 在水平范围外（9格）放置点燃的灵魂营火，不应排斥猪灵
    const BlockState& litSoulCampfire = block_registry::NetherBlocks::SOUL_CAMPFIRE->defaultState();
    world.setBlockStateAt(9, 64, 0, &litSoulCampfire);

    PiglinEntity piglin(EntityInstanceId(90), mc::test::testEcsRegistry());
    piglin.setWorld(&world);
    f32 weight = piglin.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.0f);
}

class TurtlePathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
    PathWeightOverrideTestWorld world;
};

TEST_F(TurtlePathWeightTest, PrefersWaterWhenNotGoingHome)
{
    // 非回家状态 + 水中：返回 10.0f
    world.setWaterAt(0, 64, 0);

    TurtleEntity turtle(EntityInstanceId(90), mc::test::testEcsRegistry());
    turtle.setWorld(&world);
    turtle.setGoingHome(false);

    f32 weight = turtle.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 10.0f);
}

TEST_F(TurtlePathWeightTest, DoesNotPreferWaterWhenGoingHome)
{
    // 回家状态 + 水中：水中不再是高权重，回到亮度逻辑
    world.setWaterAt(0, 64, 0);
    world.setBrightness(0.8f);

    TurtleEntity turtle(EntityInstanceId(90), mc::test::testEcsRegistry());
    turtle.setWorld(&world);
    turtle.setGoingHome(true);

    f32 weight = turtle.getPathWeight(0.0f, 64.0f, 0.0f);
    // 回家时水中不返回 10.0f，而是 brightness - 0.5
    EXPECT_FLOAT_EQ(weight, 0.8f - 0.5f); // 0.3f
}

TEST_F(TurtlePathWeightTest, PrefersSandWhenGoingHome)
{
    // 回家状态 + 沙滩上：仍然返回 10.0f（偏好沙滩产卵）
    world.setBlockStateAt(0, 63, 0, &block_registry::BaseBlocks::SAND->defaultState());
    world.setBrightness(0.0f); // 即使在黑暗中

    TurtleEntity turtle(EntityInstanceId(90), mc::test::testEcsRegistry());
    turtle.setWorld(&world);
    turtle.setGoingHome(true);

    f32 weight = turtle.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 10.0f);
}

TEST_F(TurtlePathWeightTest, PrefersSandWhenNotGoingHome)
{
    // 非回家状态 + 沙滩上：返回 10.0f
    world.setBlockStateAt(0, 63, 0, &block_registry::BaseBlocks::SAND->defaultState());

    TurtleEntity turtle(EntityInstanceId(90), mc::test::testEcsRegistry());
    turtle.setWorld(&world);
    turtle.setGoingHome(false);

    f32 weight = turtle.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 10.0f);
}

TEST_F(TurtlePathWeightTest, ReturnsBrightnessWeightOnNonWaterNonSand)
{
    // 非回家状态 + 非水非沙滩：返回 brightness - 0.5f
    world.setBrightness(0.8f);

    TurtleEntity turtle(EntityInstanceId(90), mc::test::testEcsRegistry());
    turtle.setWorld(&world);
    turtle.setGoingHome(false);

    f32 weight = turtle.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 0.8f - 0.5f); // 0.3f
}

TEST_F(TurtlePathWeightTest, ReturnsNegativeWeightInDarkness)
{
    // 黑暗环境 + 非水非沙滩：返回负值
    world.setBrightness(0.0f);

    TurtleEntity turtle(EntityInstanceId(90), mc::test::testEcsRegistry());
    turtle.setWorld(&world);
    turtle.setGoingHome(false);

    f32 weight = turtle.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, -0.5f);
}

TEST_F(TurtlePathWeightTest, ReturnsZeroWhenNoWorld)
{
    // 没有世界时返回 0.0f
    TurtleEntity turtle(EntityInstanceId(90), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(turtle.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

TEST_F(TurtlePathWeightTest, WaterOverridesSandWhenNotGoingHome)
{
    // 非回家状态 + 水中（脚下虽有沙子但检测的是位置流体）：水中优先返回 10.0f
    world.setWaterAt(0, 64, 0);
    world.setBlockStateAt(0, 63, 0, &block_registry::BaseBlocks::SAND->defaultState());

    TurtleEntity turtle(EntityInstanceId(90), mc::test::testEcsRegistry());
    turtle.setWorld(&world);
    turtle.setGoingHome(false);

    f32 weight = turtle.getPathWeight(0.0f, 64.0f, 0.0f);
    // 非回家时水中直接返回 10.0f，不会到沙子检查
    EXPECT_FLOAT_EQ(weight, 10.0f);
}

TEST_F(TurtlePathWeightTest, GoingHomeSandOverridesDarkness)
{
    // 回家状态 + 沙滩 + 黑暗：沙子偏好优先于亮度
    world.setBlockStateAt(0, 63, 0, &block_registry::BaseBlocks::SAND->defaultState());
    world.setBrightness(0.0f);

    TurtleEntity turtle(EntityInstanceId(90), mc::test::testEcsRegistry());
    turtle.setWorld(&world);
    turtle.setGoingHome(true);

    f32 weight = turtle.getPathWeight(0.0f, 64.0f, 0.0f);
    EXPECT_FLOAT_EQ(weight, 10.0f);
}

} // namespace
