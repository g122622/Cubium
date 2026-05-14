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
#include "common/entity/entities/passive/special/BeeEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/tag/ItemTags.hpp"
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
class BeeTestWorld final : public test::BaseTestWorld {
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
        throw std::runtime_error("BeeTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("BeeTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class BeeEntityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 -> 物品 -> 方块物品 -> 物品标签
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        item::tag::ItemTags::initialize();
    }

    BeeTestWorld m_world;
};

// ============================================================================
// 繁殖物品测试 - isBreedingItem
// ============================================================================

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsDandelion)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* dandelionItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DANDELION);
    ASSERT_NE(dandelionItem, nullptr);

    ItemStack dandelionStack(dandelionItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(dandelionStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsPoppy)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* poppyItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::POPPY);
    ASSERT_NE(poppyItem, nullptr);

    ItemStack poppyStack(poppyItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(poppyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsBlueOrchid)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* blueOrchidItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BLUE_ORCHID);
    ASSERT_NE(blueOrchidItem, nullptr);

    ItemStack blueOrchidStack(blueOrchidItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(blueOrchidStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsAllium)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* alliumItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::ALLIUM);
    ASSERT_NE(alliumItem, nullptr);

    ItemStack alliumStack(alliumItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(alliumStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsSunflower)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* sunflowerItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::SUNFLOWER);
    ASSERT_NE(sunflowerItem, nullptr);

    ItemStack sunflowerStack(sunflowerItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(sunflowerStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsLilac)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* lilacItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::LILAC);
    ASSERT_NE(lilacItem, nullptr);

    ItemStack lilacStack(lilacItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(lilacStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsRoseBush)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* roseBushItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::ROSE_BUSH);
    ASSERT_NE(roseBushItem, nullptr);

    ItemStack roseBushStack(roseBushItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(roseBushStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsPeony)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* peonyItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PEONY);
    ASSERT_NE(peonyItem, nullptr);

    ItemStack peonyStack(peonyItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(peonyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsCornflower)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* cornflowerItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CORNFLOWER);
    ASSERT_NE(cornflowerItem, nullptr);

    ItemStack cornflowerStack(cornflowerItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(cornflowerStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsLilyOfTheValley)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* lilyItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::LILY_OF_THE_VALLEY);
    ASSERT_NE(lilyItem, nullptr);

    ItemStack lilyStack(lilyItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(lilyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsTulips)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    // 测试所有颜色的郁金香
    const BlockItem* redTulipItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::RED_TULIP);
    const BlockItem* orangeTulipItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::ORANGE_TULIP);
    const BlockItem* whiteTulipItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WHITE_TULIP);
    const BlockItem* pinkTulipItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PINK_TULIP);

    ASSERT_NE(redTulipItem, nullptr);
    ASSERT_NE(orangeTulipItem, nullptr);
    ASSERT_NE(whiteTulipItem, nullptr);
    ASSERT_NE(pinkTulipItem, nullptr);

    EXPECT_TRUE(bee.isBreedingItem(ItemStack(redTulipItem, 1)));
    EXPECT_TRUE(bee.isBreedingItem(ItemStack(orangeTulipItem, 1)));
    EXPECT_TRUE(bee.isBreedingItem(ItemStack(whiteTulipItem, 1)));
    EXPECT_TRUE(bee.isBreedingItem(ItemStack(pinkTulipItem, 1)));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsWitherRose)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* witherRoseItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WITHER_ROSE);
    ASSERT_NE(witherRoseItem, nullptr);

    ItemStack witherRoseStack(witherRoseItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(witherRoseStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsWheat)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    if (Items::WHEAT != nullptr) {
        ItemStack wheatStack(Items::WHEAT, 1);
        EXPECT_FALSE(bee.isBreedingItem(wheatStack));
    }
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsCarrot)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    if (Items::CARROT != nullptr) {
        ItemStack carrotStack(Items::CARROT, 1);
        EXPECT_FALSE(bee.isBreedingItem(carrotStack));
    }
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsEmptyStack)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    ItemStack emptyStack;
    EXPECT_FALSE(bee.isBreedingItem(emptyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsStone)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    const BlockItem* stoneItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::STONE);
    ASSERT_NE(stoneItem, nullptr);

    ItemStack stoneStack(stoneItem, 1);
    EXPECT_FALSE(bee.isBreedingItem(stoneStack));
}

// ============================================================================
// spawnBaby 测试
// ============================================================================

TEST_F(BeeEntityTest, SpawnBaby_CreatesChildBee)
{
    BeeEntity parent1(LegacyEntityType::Unknown, 1);
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);

    BeeEntity parent2(LegacyEntityType::Unknown, 2);

    auto baby = parent1.spawnBaby(parent2);

    ASSERT_NE(baby, nullptr);
    EXPECT_TRUE(baby->isChild());

    // 检查是 BeeEntity 类型
    BeeEntity* babyBee = dynamic_cast<BeeEntity*>(baby.get());
    EXPECT_NE(babyBee, nullptr);
}

TEST_F(BeeEntityTest, SpawnBaby_PositionNearParent)
{
    BeeEntity parent(LegacyEntityType::Unknown, 1);
    parent.setWorld(&m_world);
    parent.setPosition(100.0f, 64.0f, 200.0f);

    BeeEntity partner(LegacyEntityType::Unknown, 2);

    auto baby = parent.spawnBaby(partner);

    ASSERT_NE(baby, nullptr);

    // 幼体应该在父体附近
    f32 dx = baby->x() - parent.x();
    f32 dy = baby->y() - parent.y();
    f32 dz = baby->z() - parent.z();
    f32 distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    // 位置应该非常接近（spawnBaby使用父体位置）
    EXPECT_LT(distance, 1.0f);
}

// ============================================================================
// 花粉状态测试
// ============================================================================

TEST_F(BeeEntityTest, NectarState_DefaultFalse)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);
    EXPECT_FALSE(bee.hasNectar());
}

TEST_F(BeeEntityTest, NectarState_CanSetAndGet)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    bee.setHasNectar(true);
    EXPECT_TRUE(bee.hasNectar());

    bee.setHasNectar(false);
    EXPECT_FALSE(bee.hasNectar());
}

// ============================================================================
// 螫刺状态测试
// ============================================================================

TEST_F(BeeEntityTest, StungState_DefaultFalse)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);
    EXPECT_FALSE(bee.hasStung());
}

TEST_F(BeeEntityTest, StungState_CanSetAndGet)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    bee.setHasStung(true);
    EXPECT_TRUE(bee.hasStung());

    bee.setHasStung(false);
    EXPECT_FALSE(bee.hasStung());
}

// ============================================================================
// 飞行状态测试
// ============================================================================

TEST_F(BeeEntityTest, FlyingState_DefaultFalse)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);
    EXPECT_FALSE(bee.isFlying());
}

TEST_F(BeeEntityTest, FlyingState_CanSetAndGet)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    bee.setFlying(true);
    EXPECT_TRUE(bee.isFlying());

    bee.setFlying(false);
    EXPECT_FALSE(bee.isFlying());
}

// ============================================================================
// 蜂巢系统测试
// ============================================================================

TEST_F(BeeEntityTest, HivePosition_DefaultNoHive)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);
    EXPECT_FALSE(bee.hasHive());
}

TEST_F(BeeEntityTest, HivePosition_CanSetAndGet)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    BlockPos hivePos(100, 64, 200);
    bee.setHivePos(hivePos);

    EXPECT_TRUE(bee.hasHive());
    EXPECT_EQ(bee.getHivePos(), hivePos);
}

// ============================================================================
// 花朵位置测试
// ============================================================================

TEST_F(BeeEntityTest, FlowerPosition_DefaultNoFlower)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);
    EXPECT_FALSE(bee.hasFlower());
}

TEST_F(BeeEntityTest, FlowerPosition_CanSetAndGet)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    BlockPos flowerPos(50, 64, 100);
    bee.setFlowerPos(flowerPos);

    EXPECT_TRUE(bee.hasFlower());
    EXPECT_EQ(bee.getFlowerPos(), flowerPos);
}

// ============================================================================
// 属性测试
// ============================================================================

TEST_F(BeeEntityTest, Attributes_HasCorrectBaseValues)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    // MC 1.16.5: 蜜蜂生命值为 10
    EXPECT_DOUBLE_EQ(bee.maxHealth(), 10.0);

    // MC 1.16.5: 蜜蜂移动速度为 0.3
    EXPECT_DOUBLE_EQ(bee.getAttributeValue("generic.movement_speed", 0.0), 0.3);
}

// ============================================================================
// 眼睛高度测试
// ============================================================================

TEST_F(BeeEntityTest, EyeHeight_IsCorrect)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);
    // MC 1.16.5: 蜜蜂眼睛高度 0.3
    EXPECT_FLOAT_EQ(bee.eyeHeight(), 0.3f);
}

// ============================================================================
// IAngerable 接口测试
// ============================================================================

TEST_F(BeeEntityTest, Anger_CanSetAngerTime)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    bee.setAngerTime(100);
    EXPECT_EQ(bee.getAngerTime(), 100);
    EXPECT_TRUE(bee.isAngry());
}

TEST_F(BeeEntityTest, Anger_CanSetAngry)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    bee.setAngry(true);
    EXPECT_TRUE(bee.isAngry());
    EXPECT_GT(bee.getAngerTime(), 0);
}

TEST_F(BeeEntityTest, Anger_CanClearAnger)
{
    BeeEntity bee(LegacyEntityType::Unknown, 1);

    // 先设置为愤怒状态
    bee.setAngerTime(100);
    EXPECT_TRUE(bee.isAngry());

    // 清除愤怒
    bee.setAngry(false);
    EXPECT_FALSE(bee.isAngry());
    EXPECT_EQ(bee.getAngerTime(), 0);
}

} // namespace
} // namespace mc
