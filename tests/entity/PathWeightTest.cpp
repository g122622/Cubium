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
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

using namespace mc;

namespace {

/**
 * @brief 测试用 Mock World，支持亮度、方块状态等
 */
class PathWeightTestWorld final : public test::BaseTestWorld {
public:
    void setBrightness(f32 brightness) { m_brightness = brightness; }
    void setBlockStateAt(i32 x, i32 y, i32 z, const BlockState* state) { m_blockStates[BlockPos(x, y, z)] = state; }
    void setGrassBlockAt(i32 x, i32 y, i32 z)
    {
        m_blockStates[BlockPos(x, y, z)] = &VanillaBlocks::GRASS_BLOCK->defaultState();
    }

    // ========== IWorld 接口实现 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockStates.find(BlockPos(x, y, z));
        if (it != m_blockStates.end()) {
            return it->second;
        }
        return nullptr;
    }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return static_cast<u8>(m_brightness * 15.0f); }

    [[nodiscard]] f32 getBrightness(const BlockPos& pos) const override
    {
        (void)pos;
        return m_brightness;
    }

    // TickManager interface
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("PathWeightTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("PathWeightTestWorld::tickManager not implemented");
    }

private:
    f32 m_brightness = 0.5f;
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
};

/**
 * @brief 测试用具体动物实体（猪）
 */
class TestAnimalEntity final : public AnimalEntity {
public:
    TestAnimalEntity()
        : AnimalEntity(EntityInstanceId(1))
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
 * @brief 测试用具体怪物实体（僵尸）
 */
class TestMonsterEntity final : public MonsterEntity {
public:
    TestMonsterEntity()
        : MonsterEntity(EntityInstanceId(2))
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

/**
 * @brief 测试用基础生物实体
 */
class TestCreatureEntity final : public CreatureEntity {
public:
    TestCreatureEntity()
        : CreatureEntity(EntityInstanceId(3))
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// ============================================================================
// CreatureEntity::getPathWeight 测试
// ============================================================================

class CreatureEntityPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    PathWeightTestWorld world;
};

TEST_F(CreatureEntityPathWeightTest, DefaultPathWeightIsZero)
{
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    // CreatureEntity 默认返回 0.0F
    EXPECT_FLOAT_EQ(creature.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(creature.getPathWeight(10.0f, 64.0f, 10.0f), 0.0f);
}

TEST_F(CreatureEntityPathWeightTest, BlockPosOverloadWorks)
{
    TestCreatureEntity creature;
    creature.setWorld(&world);

    // BlockPos 版本应该委托给 float 版本
    EXPECT_FLOAT_EQ(creature.getPathWeight(BlockPos(5, 64, 10)), 0.0f);
}

// ============================================================================
// AnimalEntity::getPathWeight 测试
// ============================================================================

class AnimalEntityPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    PathWeightTestWorld world;
};

TEST_F(AnimalEntityPathWeightTest, ReturnsHighWeightOnGrassBlock)
{
    TestAnimalEntity animal;
    animal.setWorld(&world);
    animal.setPosition(0.0f, 65.0f, 0.0f);

    // 在脚下方块位置 (y-1) 设置草方块
    world.setGrassBlockAt(0, 64, 0);

    // 动物在草方块上应该返回 10.0F
    EXPECT_FLOAT_EQ(animal.getPathWeight(0.0f, 65.0f, 0.0f), 10.0f);
    // 测试 BlockPos 版本（继承自 CreatureEntity）
    EXPECT_FLOAT_EQ(static_cast<const CreatureEntity&>(animal).getPathWeight(BlockPos(0, 65, 0)), 10.0f);
}

TEST_F(AnimalEntityPathWeightTest, ReturnsBrightnessWeightOnNonGrassBlock)
{
    TestAnimalEntity animal;
    animal.setWorld(&world);
    animal.setPosition(0.0f, 65.0f, 0.0f);

    // 不设置草方块，测试亮度权重
    world.setBrightness(0.5f);

    // 动物在非草方块上应该返回 亮度 - 0.5F
    // 0.5 - 0.5 = 0.0
    EXPECT_FLOAT_EQ(animal.getPathWeight(0.0f, 65.0f, 0.0f), 0.0f);

    world.setBrightness(1.0f);
    // 1.0 - 0.5 = 0.5
    EXPECT_FLOAT_EQ(animal.getPathWeight(0.0f, 65.0f, 0.0f), 0.5f);

    world.setBrightness(0.0f);
    // 0.0 - 0.5 = -0.5
    EXPECT_FLOAT_EQ(animal.getPathWeight(0.0f, 65.0f, 0.0f), -0.5f);
}

TEST_F(AnimalEntityPathWeightTest, ReturnsZeroWhenNoWorld)
{
    TestAnimalEntity animal;
    // 不设置 world

    // 没有世界时应该返回 0.0F
    EXPECT_FLOAT_EQ(animal.getPathWeight(0.0f, 65.0f, 0.0f), 0.0f);
}

TEST_F(AnimalEntityPathWeightTest, GrassBlockTakesPriorityOverBrightness)
{
    TestAnimalEntity animal;
    animal.setWorld(&world);
    animal.setPosition(0.0f, 65.0f, 0.0f);

    // 设置草方块和高亮度
    world.setGrassBlockAt(0, 64, 0);
    world.setBrightness(0.0f); // 低亮度

    // 草方块权重应该优先于亮度权重
    // 草方块返回 10.0F，不是 0.0 - 0.5 = -0.5
    EXPECT_FLOAT_EQ(animal.getPathWeight(0.0f, 65.0f, 0.0f), 10.0f);
}

TEST_F(AnimalEntityPathWeightTest, WorksAtDifferentPositions)
{
    TestAnimalEntity animal;
    animal.setWorld(&world);
    animal.setPosition(100.0f, 64.0f, 200.0f);

    // 在不同位置设置草方块
    world.setGrassBlockAt(100, 63, 200);
    EXPECT_FLOAT_EQ(animal.getPathWeight(100.0f, 64.0f, 200.0f), 10.0f);

    // 位置没有草方块
    world.setBrightness(0.3f);
    EXPECT_FLOAT_EQ(animal.getPathWeight(50.0f, 64.0f, 50.0f), -0.2f); // 0.3 - 0.5
}

// ============================================================================
// MonsterEntity::getPathWeight 测试
// ============================================================================

class MonsterEntityPathWeightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    PathWeightTestWorld world;
};

TEST_F(MonsterEntityPathWeightTest, PrefersDarkness)
{
    TestMonsterEntity monster;
    monster.setWorld(&world);
    monster.setPosition(0.0f, 64.0f, 0.0f);

    // 黑暗环境（亮度 0）
    world.setBrightness(0.0f);
    // 0.5 - 0.0 = 0.5（高权重，偏好）
    EXPECT_FLOAT_EQ(monster.getPathWeight(0.0f, 64.0f, 0.0f), 0.5f);
    // 测试 BlockPos 版本（继承自 CreatureEntity）
    EXPECT_FLOAT_EQ(static_cast<const CreatureEntity&>(monster).getPathWeight(BlockPos(0, 64, 0)), 0.5f);
}

TEST_F(MonsterEntityPathWeightTest, DislikesBrightLight)
{
    TestMonsterEntity monster;
    monster.setWorld(&world);
    monster.setPosition(0.0f, 64.0f, 0.0f);

    // 明亮环境（亮度 1.0）
    world.setBrightness(1.0f);
    // 0.5 - 1.0 = -0.5（负权重，避开）
    EXPECT_FLOAT_EQ(monster.getPathWeight(0.0f, 64.0f, 0.0f), -0.5f);
}

TEST_F(MonsterEntityPathWeightTest, NeutralAtMidBrightness)
{
    TestMonsterEntity monster;
    monster.setWorld(&world);
    monster.setPosition(0.0f, 64.0f, 0.0f);

    // 中等亮度（亮度 0.5）
    world.setBrightness(0.5f);
    // 0.5 - 0.5 = 0.0（中性）
    EXPECT_FLOAT_EQ(monster.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

TEST_F(MonsterEntityPathWeightTest, ReturnsZeroWhenNoWorld)
{
    TestMonsterEntity monster;
    // 不设置 world

    // 没有世界时应该返回 0.0F
    EXPECT_FLOAT_EQ(monster.getPathWeight(0.0f, 64.0f, 0.0f), 0.0f);
}

TEST_F(MonsterEntityPathWeightTest, WeightRangeIsNegativeOneHalfToPositiveOneHalf)
{
    TestMonsterEntity monster;
    monster.setWorld(&world);

    // 亮度范围 0.0 ~ 1.0，权重范围应该是 0.5 ~ -0.5
    world.setBrightness(0.0f);
    EXPECT_FLOAT_EQ(monster.getPathWeight(0.0f, 64.0f, 0.0f), 0.5f); // 最大权重

    world.setBrightness(0.25f);
    EXPECT_FLOAT_EQ(monster.getPathWeight(0.0f, 64.0f, 0.0f), 0.25f);

    world.setBrightness(0.75f);
    EXPECT_FLOAT_EQ(monster.getPathWeight(0.0f, 64.0f, 0.0f), -0.25f);

    world.setBrightness(1.0f);
    EXPECT_FLOAT_EQ(monster.getPathWeight(0.0f, 64.0f, 0.0f), -0.5f); // 最小权重
}

// ============================================================================
// 积分测试：验证不同实体类型的寻路偏好差异
// ============================================================================

class PathWeightComparisonTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    PathWeightTestWorld world;
};

TEST_F(PathWeightComparisonTest, AnimalPrefersGrassOverDarkness)
{
    TestAnimalEntity animal;
    animal.setWorld(&world);
    animal.setPosition(0.0f, 65.0f, 0.0f);

    // 草方块位置（高权重 10.0）
    world.setGrassBlockAt(0, 64, 0);
    f32 grassWeight = animal.getPathWeight(0.0f, 65.0f, 0.0f);

    // 非草方块黑暗位置
    world.setBrightness(0.0f);
    BlockPos darkPos(10, 65, 10);
    f32 darkWeight =
        animal.getPathWeight(static_cast<f32>(darkPos.x), static_cast<f32>(darkPos.y), static_cast<f32>(darkPos.z));

    // 动物应该偏好草方块位置
    EXPECT_GT(grassWeight, darkWeight);
    EXPECT_FLOAT_EQ(grassWeight, 10.0f);
    EXPECT_FLOAT_EQ(darkWeight, -0.5f); // 0.0 - 0.5
}

TEST_F(PathWeightComparisonTest, MonsterPrefersDarknessOverLight)
{
    TestMonsterEntity monster;
    monster.setWorld(&world);
    monster.setPosition(0.0f, 64.0f, 0.0f);

    // 黑暗位置
    world.setBrightness(0.0f);
    f32 darkWeight = monster.getPathWeight(0.0f, 64.0f, 0.0f);

    // 明亮位置
    world.setBrightness(1.0f);
    BlockPos brightPos(10, 64, 10);
    f32 brightWeight = monster.getPathWeight(
        static_cast<f32>(brightPos.x), static_cast<f32>(brightPos.y), static_cast<f32>(brightPos.z));

    // 怪物应该偏好黑暗位置
    EXPECT_GT(darkWeight, brightWeight);
    EXPECT_FLOAT_EQ(darkWeight, 0.5f);
    EXPECT_FLOAT_EQ(brightWeight, -0.5f);
}

TEST_F(PathWeightComparisonTest, AnimalAndMonsterHaveOppositePreferences)
{
    TestAnimalEntity animal;
    animal.setWorld(&world);

    TestMonsterEntity monster;
    monster.setWorld(&world);

    // 相同亮度下的权重计算
    world.setBrightness(0.3f);

    f32 animalWeight = animal.getPathWeight(0.0f, 64.0f, 0.0f);
    f32 monsterWeight = monster.getPathWeight(0.0f, 64.0f, 0.0f);

    // Animal: 亮度 - 0.5 = 0.3 - 0.5 = -0.2
    // Monster: 0.5 - 亮度 = 0.5 - 0.3 = 0.2
    EXPECT_FLOAT_EQ(animalWeight, -0.2f);
    EXPECT_FLOAT_EQ(monsterWeight, 0.2f);

    // 两者符号相反
    EXPECT_LT(animalWeight, 0.0f);
    EXPECT_GT(monsterWeight, 0.0f);
}

} // namespace
