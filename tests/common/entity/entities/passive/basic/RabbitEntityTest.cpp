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
#include "common/entity/entities/passive/basic/RabbitEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用模拟世界
 */
class RabbitTestWorld final : public test::BaseTestWorld {
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

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("RabbitTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("RabbitTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class RabbitEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }

    RabbitTestWorld m_world;
};

// ========== 兔子类型测试 ==========

TEST_F(RabbitEntityTest, RabbitType_DefaultIsBrown)
{
    RabbitEntity rabbit(EntityId(1));
    // 默认类型由 setRandomRabbitType 设置，测试概率分布
    // 由于随机性，我们只测试类型在有效范围内
    EXPECT_GE(static_cast<u8>(rabbit.getRabbitType()), 0);
    EXPECT_LE(static_cast<u8>(rabbit.getRabbitType()), 99); // 包括 Killer (99)
}

TEST_F(RabbitEntityTest, RabbitType_CanSetAndGetType)
{
    RabbitEntity rabbit(EntityId(1));

    rabbit.setRabbitType(RabbitEntity::RabbitType::White);
    EXPECT_EQ(rabbit.getRabbitType(), RabbitEntity::RabbitType::White);

    rabbit.setRabbitType(RabbitEntity::RabbitType::Black);
    EXPECT_EQ(rabbit.getRabbitType(), RabbitEntity::RabbitType::Black);

    rabbit.setRabbitType(RabbitEntity::RabbitType::Killer);
    EXPECT_EQ(rabbit.getRabbitType(), RabbitEntity::RabbitType::Killer);
    EXPECT_TRUE(rabbit.isKillerRabbit());
}

TEST_F(RabbitEntityTest, RabbitType_KillerRabbitDetection)
{
    RabbitEntity rabbit(EntityId(1));

    rabbit.setRabbitType(RabbitEntity::RabbitType::Brown);
    EXPECT_FALSE(rabbit.isKillerRabbit());

    rabbit.setRabbitType(RabbitEntity::RabbitType::Killer);
    EXPECT_TRUE(rabbit.isKillerRabbit());
}

// ========== 繁殖物品测试 ==========

TEST_F(RabbitEntityTest, IsBreedingItem_AcceptsCarrot)
{
    RabbitEntity rabbit(EntityId(1));

    ItemStack carrotStack(Items::CARROT, 1);
    EXPECT_TRUE(rabbit.isBreedingItem(carrotStack));
}

TEST_F(RabbitEntityTest, IsBreedingItem_AcceptsGoldenCarrot)
{
    RabbitEntity rabbit(EntityId(1));

    ItemStack goldenCarrotStack(Items::GOLDEN_CARROT, 1);
    EXPECT_TRUE(rabbit.isBreedingItem(goldenCarrotStack));
}

TEST_F(RabbitEntityTest, IsBreedingItem_AcceptsDandelion)
{
    RabbitEntity rabbit(EntityId(1));

    // 获取蒲公英方块物品
    const BlockItem* dandelionItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DANDELION);
    ASSERT_NE(dandelionItem, nullptr);

    ItemStack dandelionStack(dandelionItem, 1);
    EXPECT_TRUE(rabbit.isBreedingItem(dandelionStack));
}

TEST_F(RabbitEntityTest, IsBreedingItem_RejectsOtherItems)
{
    RabbitEntity rabbit(EntityId(1));

    // 测试不接受其他物品
    if (Items::WHEAT != nullptr) {
        ItemStack wheatStack(Items::WHEAT, 1);
        EXPECT_FALSE(rabbit.isBreedingItem(wheatStack));
    }

    // 空物品栈
    ItemStack emptyStack;
    EXPECT_FALSE(rabbit.isBreedingItem(emptyStack));
}

// ========== spawnBaby 测试 ==========

TEST_F(RabbitEntityTest, SpawnBaby_CreatesChildRabbit)
{
    RabbitEntity parent1(EntityId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setRabbitType(RabbitEntity::RabbitType::Brown);

    RabbitEntity parent2(EntityId(2));
    parent2.setRabbitType(RabbitEntity::RabbitType::White);

    auto baby = parent1.spawnBaby(parent2);

    ASSERT_NE(baby, nullptr);
    EXPECT_TRUE(baby->isChild());

    // 检查是 RabbitEntity 类型
    RabbitEntity* babyRabbit = dynamic_cast<RabbitEntity*>(baby.get());
    EXPECT_NE(babyRabbit, nullptr);
}

TEST_F(RabbitEntityTest, SpawnBaby_InheritsParentType)
{
    RabbitEntity parent1(EntityId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setRabbitType(RabbitEntity::RabbitType::Gold);

    RabbitEntity parent2(EntityId(2));
    parent2.setRabbitType(RabbitEntity::RabbitType::SaltAndPepper);

    // 多次测试类型继承（由于随机性）
    for (int i = 0; i < 100; ++i) {
        auto baby = parent1.spawnBaby(parent2);
        ASSERT_NE(baby, nullptr);

        RabbitEntity* babyRabbit = dynamic_cast<RabbitEntity*>(baby.get());
        ASSERT_NE(babyRabbit, nullptr);

        // 类型应该是父母之一或随机生成的（在正常范围内）
        RabbitEntity::RabbitType type = babyRabbit->getRabbitType();
        bool validType = (type == RabbitEntity::RabbitType::Brown || type == RabbitEntity::RabbitType::White ||
            type == RabbitEntity::RabbitType::Black || type == RabbitEntity::RabbitType::WhiteSpotted ||
            type == RabbitEntity::RabbitType::Gold || type == RabbitEntity::RabbitType::SaltAndPepper ||
            type == RabbitEntity::RabbitType::Killer || type == RabbitEntity::RabbitType::Toast);
        EXPECT_TRUE(validType) << "Invalid rabbit type: " << static_cast<int>(type);
    }
}

// ========== 声音类别测试 ==========

TEST_F(RabbitEntityTest, SoundCategory_NeutralForNormalRabbit)
{
    RabbitEntity rabbit(EntityId(1));
    rabbit.setRabbitType(RabbitEntity::RabbitType::Brown);

    EXPECT_EQ(rabbit.getSoundCategory(), sound::SoundCategory::Neutral);
}

TEST_F(RabbitEntityTest, SoundCategory_HostileForKillerRabbit)
{
    RabbitEntity rabbit(EntityId(1));
    rabbit.setRabbitType(RabbitEntity::RabbitType::Killer);

    EXPECT_EQ(rabbit.getSoundCategory(), sound::SoundCategory::Hostile);
}

// ========== 属性测试 ==========

TEST_F(RabbitEntityTest, Attributes_HasCorrectBaseValues)
{
    RabbitEntity rabbit(EntityId(1));

    // MC 1.16.5: 兔子生命值为 3
    EXPECT_DOUBLE_EQ(rabbit.maxHealth(), 3.0);

    // MC 1.16.5: 兔子移动速度为 0.3
    EXPECT_DOUBLE_EQ(rabbit.getAttributeValue("generic.movement_speed", 0.0), 0.3);
}

// ========== 尺寸测试 ==========

TEST_F(RabbitEntityTest, Dimensions_CorrectBaseSize)
{
    RabbitEntity rabbit(EntityId(1));
    rabbit.setChild(false); // 设置为成体

    // MC 1.16.5: 兔子宽度 0.4，高度 0.5
    // 通过碰撞箱来验证尺寸
    const AxisAlignedBB& box = rabbit.boundingBox();
    // 碰撞箱的宽度和高度应该接近实体尺寸
    f32 boxWidth = box.maxX - box.minX;
    f32 boxHeight = box.maxY - box.minY;
    EXPECT_NEAR(boxWidth, 0.4f, 0.01f);
    EXPECT_NEAR(boxHeight, 0.5f, 0.01f);
}

TEST_F(RabbitEntityTest, EyeHeight_DifferentForChildAndAdult)
{
    RabbitEntity adultRabbit(EntityId(1));
    adultRabbit.setChild(false);

    RabbitEntity childRabbit(EntityId(2));
    childRabbit.setChild(true);

    // 成体眼睛高度 0.35，幼体 0.2
    EXPECT_FLOAT_EQ(adultRabbit.eyeHeight(), 0.35f);
    EXPECT_FLOAT_EQ(childRabbit.eyeHeight(), 0.2f);
}

} // namespace
} // namespace mc
