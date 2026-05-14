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
#include "common/entity/entities/passive/tamable/ParrotEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 鹦鹉实体测试用世界
 *
 * 提供最小化测试环境用于鹦鹉实体功能测试
 */
class ParrotTestWorld final : public test::BaseTestWorld {
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
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ParrotTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ParrotTestWorld::tickManager not implemented");
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class ParrotEntityTestFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    ParrotTestWorld m_world;
};

// ============================================================================
// 驯服物品测试
// 参考 MC 1.16.5: ParrotEntity.TAME_ITEMS = {WHEAT_SEEDS, MELON_SEEDS, PUMPKIN_SEEDS, BEETROOT_SEEDS}
// ============================================================================

TEST_F(ParrotEntityTestFixture, IsTameItem_WheatSeeds_ReturnsTrue)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 小麦种子可以驯服鹦鹉
    ItemStack stack(Items::WHEAT_SEEDS, 1);
    EXPECT_TRUE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_PumpkinSeeds_ReturnsTrue)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 南瓜种子可以驯服鹦鹉
    ItemStack stack(Items::PUMPKIN_SEEDS, 1);
    EXPECT_TRUE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_MelonSeeds_ReturnsTrue)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 西瓜种子可以驯服鹦鹉
    ItemStack stack(Items::MELON_SEEDS, 1);
    EXPECT_TRUE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_BeetrootSeeds_ReturnsTrue)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 甜菜种子可以驯服鹦鹉
    ItemStack stack(Items::BEETROOT_SEEDS, 1);
    EXPECT_TRUE(parrot.isTameItem(stack));
}

// ============================================================================
// 非驯服物品测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, IsTameItem_Wheat_ReturnsFalse)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 小麦不能驯服鹦鹉
    ItemStack stack(Items::WHEAT, 1);
    EXPECT_FALSE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_Bone_ReturnsFalse)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 骨头不能驯服鹦鹉（骨头用于驯服狼）
    ItemStack stack(Items::BONE, 1);
    EXPECT_FALSE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_Cod_ReturnsFalse)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 生鳕鱼不能驯服鹦鹉（生鳕鱼用于驯服猫）
    ItemStack stack(Items::COD, 1);
    EXPECT_FALSE(parrot.isTameItem(stack));
}

TEST_F(ParrotEntityTestFixture, IsTameItem_Apple_ReturnsFalse)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 苹果不能驯服鹦鹉
    ItemStack stack(Items::APPLE, 1);
    EXPECT_FALSE(parrot.isTameItem(stack));
}

// ============================================================================
// 空物品测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, IsTameItem_EmptyStack_ReturnsFalse)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 空物品堆不能驯服
    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(parrot.isTameItem(emptyStack));
}

// ============================================================================
// 繁殖测试 - 鹦鹉不能繁殖
// 参考 MC 1.16.5: 鹦鹉是唯一不能繁殖的可驯服动物
// ============================================================================

TEST_F(ParrotEntityTestFixture, IsBreedingItem_AnyItem_ReturnsFalse)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 鹦鹉不能繁殖，任何物品都返回 false
    ItemStack wheatSeeds(Items::WHEAT_SEEDS, 1);
    EXPECT_FALSE(parrot.isBreedingItem(wheatSeeds));

    ItemStack pumpkinSeeds(Items::PUMPKIN_SEEDS, 1);
    EXPECT_FALSE(parrot.isBreedingItem(pumpkinSeeds));

    ItemStack wheat(Items::WHEAT, 1);
    EXPECT_FALSE(parrot.isBreedingItem(wheat));
}

TEST_F(ParrotEntityTestFixture, SpawnBaby_ReturnsNullptr)
{
    ParrotEntity parent1(LegacyEntityType::Unknown, 0);
    ParrotEntity parent2(LegacyEntityType::Unknown, 0);

    // 鹦鹉不能生成幼体
    auto baby = parent1.spawnBaby(parent2);
    EXPECT_EQ(baby, nullptr);
}

// ============================================================================
// 变种测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, Variant_CanBeSetAndGet)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    parrot.setVariant(ParrotEntity::ParrotVariant::Blue);
    EXPECT_EQ(parrot.getVariant(), ParrotEntity::ParrotVariant::Blue);

    parrot.setVariant(ParrotEntity::ParrotVariant::Green);
    EXPECT_EQ(parrot.getVariant(), ParrotEntity::ParrotVariant::Green);

    parrot.setVariant(ParrotEntity::ParrotVariant::YellowBlue);
    EXPECT_EQ(parrot.getVariant(), ParrotEntity::ParrotVariant::YellowBlue);

    parrot.setVariant(ParrotEntity::ParrotVariant::Gray);
    EXPECT_EQ(parrot.getVariant(), ParrotEntity::ParrotVariant::Gray);
}

TEST_F(ParrotEntityTestFixture, RandomizeVariant_SetsValidVariant)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    parrot.randomizeVariant();

    // 变种应该在 0-4 范围内
    auto variant = parrot.getVariant();
    EXPECT_GE(static_cast<u8>(variant), 0u);
    EXPECT_LE(static_cast<u8>(variant), 4u);
}

// ============================================================================
// 飞行测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, Flying_CanBeSetAndCleared)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 初始不飞行
    EXPECT_FALSE(parrot.isFlying());

    // 设置飞行
    parrot.setFlying(true);
    EXPECT_TRUE(parrot.isFlying());

    // 清除飞行
    parrot.setFlying(false);
    EXPECT_FALSE(parrot.isFlying());
}

TEST_F(ParrotEntityTestFixture, CanFly_AlwaysReturnsTrue)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 鹦鹉始终可以飞
    EXPECT_TRUE(parrot.canFly());
}

// ============================================================================
// 模仿测试
// ============================================================================

TEST_F(ParrotEntityTestFixture, Imitation_CanBeSetAndQueried)
{
    ParrotEntity parrot(LegacyEntityType::Unknown, 0);

    // 初始不模仿
    EXPECT_FALSE(parrot.isImitating());

    // 设置模仿目标
    parrot.setImitatingTarget(100);
    EXPECT_TRUE(parrot.isImitating());
    EXPECT_EQ(parrot.getImitatingTarget(), 100u);

    // 设置模仿状态
    parrot.setImitating(false);
    EXPECT_FALSE(parrot.isImitating());
}

} // namespace
} // namespace mc
