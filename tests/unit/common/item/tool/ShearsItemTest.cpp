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
#include "common/entity/entities/passive/basic/SheepEntity.hpp"
#include "common/entity/interfaces/IShearable.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/items/tool/ShearsItem.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

class ShearsEntityInteractionTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ============================================================================
// 剪刀物品基本属性测试
// ============================================================================

TEST_F(ShearsEntityInteractionTest, ShearsIsRegistered)
{
    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr) << "Shears should be registered";
    EXPECT_EQ(shears->itemLocation(), ResourceLocation("minecraft:shears"));
}

TEST_F(ShearsEntityInteractionTest, ShearsHasCorrectDurability)
{
    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr);
    // MC 1.16.5: 剪刀耐久度为 238
    EXPECT_EQ(shears->maxDamage(), 238);
}

TEST_F(ShearsEntityInteractionTest, ShearsIsDamageable)
{
    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr);
    EXPECT_TRUE(shears->isDamageable());
}

TEST_F(ShearsEntityInteractionTest, ShearsIsNotStackable)
{
    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr);
    // 有耐久度的物品堆叠数为 1
    EXPECT_EQ(shears->maxStackSize(), 1);
}

// ============================================================================
// 剪刀挖掘速度测试
// ============================================================================

TEST_F(ShearsEntityInteractionTest, ShearsEffectiveOnCobweb)
{
    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr);

    auto* cobweb = VanillaBlocks::COBWEB;
    if (cobweb == nullptr) {
        GTEST_SKIP() << "COBWEB not registered yet";
    }

    ItemStack stack(*shears, 1);
    const BlockState& state = cobweb->defaultState();

    // MC 1.16.5: 剪刀对蜘蛛网挖掘速度为 15.0
    f32 speed = shears->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 15.0f);
}

TEST_F(ShearsEntityInteractionTest, ShearsEffectiveOnLeaves)
{
    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr);

    auto* oakLeaves = VanillaBlocks::OAK_LEAVES;
    if (oakLeaves == nullptr) {
        GTEST_SKIP() << "OAK_LEAVES not registered yet";
    }

    ItemStack stack(*shears, 1);
    const BlockState& state = oakLeaves->defaultState();

    // MC 1.16.5: 剪刀对树叶挖掘速度为 15.0
    f32 speed = shears->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 15.0f);
}

TEST_F(ShearsEntityInteractionTest, ShearsEffectiveOnWool)
{
    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr);

    auto* whiteWool = VanillaBlocks::WHITE_WOOL;
    if (whiteWool == nullptr) {
        GTEST_SKIP() << "WHITE_WOOL not registered yet";
    }

    ItemStack stack(*shears, 1);
    const BlockState& state = whiteWool->defaultState();

    // MC 1.16.5: 剪刀对羊毛挖掘速度为 5.0
    f32 speed = shears->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 5.0f);
}

TEST_F(ShearsEntityInteractionTest, ShearsNotEffectiveOnStone)
{
    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr);

    auto* stone = VanillaBlocks::STONE;
    ASSERT_NE(stone, nullptr);

    ItemStack stack(*shears, 1);
    const BlockState& state = stone->defaultState();

    // 剪刀对石头没有加成
    f32 speed = shears->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 1.0f);
}

// ============================================================================
// 剪刀采集测试
// ============================================================================

TEST_F(ShearsEntityInteractionTest, ShearsCanHarvestCobweb)
{
    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr);

    auto* cobweb = VanillaBlocks::COBWEB;
    if (cobweb == nullptr) {
        GTEST_SKIP() << "COBWEB not registered yet";
    }

    const BlockState& state = cobweb->defaultState();
    EXPECT_TRUE(shears->canHarvestBlock(state));
}

TEST_F(ShearsEntityInteractionTest, ShearsCannotHarvestStone)
{
    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr);

    auto* stone = VanillaBlocks::STONE;
    ASSERT_NE(stone, nullptr);

    const BlockState& state = stone->defaultState();
    // 石头需要镐子，剪刀不能采集
    EXPECT_FALSE(shears->canHarvestBlock(state));
}

// ============================================================================
// 羊毛颜色映射测试
// ============================================================================

TEST_F(ShearsEntityInteractionTest, SheepWoolColorMapping)
{
    // 测试羊毛颜色到方块的映射
    // DyeColor 枚举有 16 种颜色（White 到 Black + Count）
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::White), VanillaBlocks::WHITE_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Orange), VanillaBlocks::ORANGE_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Magenta), VanillaBlocks::MAGENTA_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::LightBlue), VanillaBlocks::LIGHT_BLUE_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Yellow), VanillaBlocks::YELLOW_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Lime), VanillaBlocks::LIME_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Pink), VanillaBlocks::PINK_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Gray), VanillaBlocks::GRAY_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::LightGray), VanillaBlocks::LIGHT_GRAY_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Cyan), VanillaBlocks::CYAN_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Purple), VanillaBlocks::PURPLE_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Blue), VanillaBlocks::BLUE_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Brown), VanillaBlocks::BROWN_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Green), VanillaBlocks::GREEN_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Red), VanillaBlocks::RED_WOOL);
    EXPECT_EQ(SheepEntity::getWoolBlockByColor(DyeColor::Black), VanillaBlocks::BLACK_WOOL);
}

TEST_F(ShearsEntityInteractionTest, SheepWoolColorMappingReturnsWhiteForInvalid)
{
    // 无效颜色（Count 枚举值）应返回白色羊毛
    const Block* wool = SheepEntity::getWoolBlockByColor(DyeColor::Count);
    EXPECT_EQ(wool, VanillaBlocks::WHITE_WOOL);
}

// ============================================================================
// IShearable 接口测试
// ============================================================================

TEST_F(ShearsEntityInteractionTest, SheepImplementsIShearable)
{
    SheepEntity sheep(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证羊实现了 IShearable 接口
    auto* shearable = dynamic_cast<entity::IShearable*>(&sheep);
    EXPECT_NE(shearable, nullptr) << "SheepEntity should implement IShearable";
}

TEST_F(ShearsEntityInteractionTest, SheepIsShearableWhenAdultAndNotSheared)
{
    SheepEntity sheep(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 成年且未剪毛的羊可以被剪
    sheep.setChild(false);
    sheep.setSheared(false);

    EXPECT_TRUE(sheep.isShearable()) << "Adult sheep with wool should be shearable";
}

TEST_F(ShearsEntityInteractionTest, SheepNotShearableWhenSheared)
{
    SheepEntity sheep(EntityInstanceId(1), mc::test::testEcsRegistry());

    sheep.setChild(false);
    sheep.setSheared(true);

    EXPECT_FALSE(sheep.isShearable()) << "Sheared sheep should not be shearable";
}

TEST_F(ShearsEntityInteractionTest, SheepNotShearableWhenChild)
{
    SheepEntity sheep(EntityInstanceId(1), mc::test::testEcsRegistry());

    sheep.setChild(true);
    sheep.setSheared(false);

    EXPECT_FALSE(sheep.isShearable()) << "Baby sheep should not be shearable";
}

TEST_F(ShearsEntityInteractionTest, SheepShearReturnsWoolItems)
{
    SheepEntity sheep(EntityInstanceId(1), mc::test::testEcsRegistry());
    sheep.setChild(false);
    sheep.setSheared(false);
    sheep.setFleeceColor(DyeColor::White);

    // 剪羊毛
    std::vector<ItemStack> drops = sheep.shear(nullptr);

    // MC 1.16.5: 羊会掉落 1-3 个羊毛
    // 实际数量取决于随机数，但应该在 1-3 范围内
    EXPECT_GE(drops.size(), 1u) << "Sheep should drop at least 1 wool";
    EXPECT_LE(drops.size(), 3u) << "Sheep should drop at most 3 wool";

    // 验证掉落物是羊毛
    for (const auto& drop : drops) {
        EXPECT_FALSE(drop.isEmpty()) << "Drops should not be empty";
        EXPECT_NE(drop.getItem(), nullptr) << "Drops should have an item";
    }

    // 验证羊被标记为已剪毛
    EXPECT_TRUE(sheep.isSheared()) << "Sheep should be sheared after shearing";
}

TEST_F(ShearsEntityInteractionTest, SheepShearSetsShearedFlag)
{
    SheepEntity sheep(EntityInstanceId(1), mc::test::testEcsRegistry());
    sheep.setChild(false);
    sheep.setSheared(false);

    EXPECT_FALSE(sheep.isSheared());

    sheep.shear(nullptr);

    EXPECT_TRUE(sheep.isSheared()) << "Shear should set sheared flag";
}

TEST_F(ShearsEntityInteractionTest, SheepCannotBeShearedTwice)
{
    SheepEntity sheep(EntityInstanceId(1), mc::test::testEcsRegistry());
    sheep.setChild(false);
    sheep.setSheared(false);

    // 第一次剪毛
    std::vector<ItemStack> drops1 = sheep.shear(nullptr);
    EXPECT_GE(drops1.size(), 1u);

    // 第二次剪毛应该失败
    EXPECT_FALSE(sheep.isShearable());
    std::vector<ItemStack> drops2 = sheep.shear(nullptr);
    EXPECT_EQ(drops2.size(), 0u) << "Second shear should return empty drops";
}

TEST_F(ShearsEntityInteractionTest, SheepColoredWoolDrops)
{
    SheepEntity sheep(EntityInstanceId(1), mc::test::testEcsRegistry());
    sheep.setChild(false);
    sheep.setSheared(false);
    sheep.setFleeceColor(DyeColor::Red);

    // 红羊应该掉落红羊毛
    std::vector<ItemStack> drops = sheep.shear(nullptr);

    EXPECT_GE(drops.size(), 1u);

    // 验证掉落的是红羊毛
    const Item* woolItem = drops[0].getItem();
    ASSERT_NE(woolItem, nullptr);

    // 红羊毛物品应该对应 RED_WOOL 方块
    const Block* expectedBlock = VanillaBlocks::RED_WOOL;
    if (expectedBlock != nullptr) {
        // 物品应该是红羊毛
        EXPECT_EQ(woolItem->itemLocation(), ResourceLocation("minecraft:red_wool"));
    }
}

TEST_F(ShearsEntityInteractionTest, SheepFleeceColorDefaultIsWhite)
{
    SheepEntity sheep(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 默认羊毛颜色应该是白色
    EXPECT_EQ(sheep.getFleeceColor(), DyeColor::White);
}

TEST_F(ShearsEntityInteractionTest, SheepFleeceColorCanBeSet)
{
    SheepEntity sheep(EntityInstanceId(1), mc::test::testEcsRegistry());

    sheep.setFleeceColor(DyeColor::Pink);
    EXPECT_EQ(sheep.getFleeceColor(), DyeColor::Pink);

    sheep.setFleeceColor(DyeColor::Black);
    EXPECT_EQ(sheep.getFleeceColor(), DyeColor::Black);
}

// ============================================================================
// ItemStack 耐久度测试
// ============================================================================

TEST_F(ShearsEntityInteractionTest, ShearsStackDamage)
{
    auto* shears = Items::SHEARS;
    ASSERT_NE(shears, nullptr);

    ItemStack stack(*shears, 1);
    EXPECT_EQ(stack.getDamage(), 0);
    EXPECT_FALSE(stack.isDamaged());

    // 造成伤害
    stack.attemptDamageItem(10);
    EXPECT_TRUE(stack.isDamaged());
    EXPECT_EQ(stack.getDamage(), 10);
}

} // namespace
} // namespace mc
