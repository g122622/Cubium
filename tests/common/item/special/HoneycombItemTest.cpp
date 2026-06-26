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
 * IMPLIED, INCLUDING BY NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT OF LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/HoneycombItem.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

class HoneycombItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

// ============================================================================
// 蜜脾物品注册测试
// ============================================================================

TEST_F(HoneycombItemTest, HoneycombIsRegistered)
{
    auto* honeycomb = Items::HONEYCOMB;
    ASSERT_NE(honeycomb, nullptr) << "HONEYCOMB should be registered";
    EXPECT_EQ(honeycomb->itemLocation(), ResourceLocation("minecraft:honeycomb"));
}

TEST_F(HoneycombItemTest, HoneycombIsStackable)
{
    auto* honeycomb = Items::HONEYCOMB;
    ASSERT_NE(honeycomb, nullptr);
    // 蜜脾可堆叠至64
    EXPECT_EQ(honeycomb->maxStackSize(), 64);
}

TEST_F(HoneycombItemTest, HoneycombIsNotDamageable)
{
    auto* honeycomb = Items::HONEYCOMB;
    ASSERT_NE(honeycomb, nullptr);
    EXPECT_FALSE(honeycomb->isDamageable());
}

// ============================================================================
// 涂蜡映射表测试
// ============================================================================

TEST_F(HoneycombItemTest, WaxablesMapContainsCopperBlockMappings)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 铜块基本映射
    EXPECT_NE(map.find(VanillaBlocks::COPPER_BLOCK), map.end()) << "COPPER_BLOCK should be in waxables map";
    EXPECT_NE(map.find(VanillaBlocks::EXPOSED_COPPER), map.end()) << "EXPOSED_COPPER should be in waxables map";
    EXPECT_NE(map.find(VanillaBlocks::WEATHERED_COPPER), map.end()) << "WEATHERED_COPPER should be in waxables map";
    EXPECT_NE(map.find(VanillaBlocks::OXIDIZED_COPPER), map.end()) << "OXIDIZED_COPPER should be in waxables map";
}

TEST_F(HoneycombItemTest, WaxablesMapContainsCutCopperMappings)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 切制铜映射
    EXPECT_NE(map.find(VanillaBlocks::CUT_COPPER), map.end());
    EXPECT_NE(map.find(VanillaBlocks::EXPOSED_CUT_COPPER), map.end());
    EXPECT_NE(map.find(VanillaBlocks::WEATHERED_CUT_COPPER), map.end());
    EXPECT_NE(map.find(VanillaBlocks::OXIDIZED_CUT_COPPER), map.end());
}

TEST_F(HoneycombItemTest, WaxablesMapContainsStairsMappings)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 切制铜楼梯映射
    EXPECT_NE(map.find(VanillaBlocks::CUT_COPPER_STAIRS), map.end());
    EXPECT_NE(map.find(VanillaBlocks::EXPOSED_CUT_COPPER_STAIRS), map.end());
}

TEST_F(HoneycombItemTest, WaxablesMapContainsSlabMappings)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 切制铜台阶映射
    EXPECT_NE(map.find(VanillaBlocks::CUT_COPPER_SLAB), map.end());
    EXPECT_NE(map.find(VanillaBlocks::EXPOSED_CUT_COPPER_SLAB), map.end());
}

TEST_F(HoneycombItemTest, WaxablesMapContainsDoorMappings)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 铜门映射
    EXPECT_NE(map.find(VanillaBlocks::COPPER_DOOR), map.end());
    EXPECT_NE(map.find(VanillaBlocks::EXPOSED_COPPER_DOOR), map.end());
}

TEST_F(HoneycombItemTest, WaxablesMapContainsTrapdoorMappings)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 铜活板门映射
    EXPECT_NE(map.find(VanillaBlocks::COPPER_TRAPDOOR), map.end());
    EXPECT_NE(map.find(VanillaBlocks::EXPOSED_COPPER_TRAPDOOR), map.end());
}

TEST_F(HoneycombItemTest, WaxablesMapContainsGrateMappings)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 铜格栅映射
    EXPECT_NE(map.find(VanillaBlocks::COPPER_GRATE), map.end());
    EXPECT_NE(map.find(VanillaBlocks::EXPOSED_COPPER_GRATE), map.end());
}

TEST_F(HoneycombItemTest, WaxablesMapContainsBulbMappings)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 铜灯映射
    EXPECT_NE(map.find(VanillaBlocks::COPPER_BULB), map.end());
    EXPECT_NE(map.find(VanillaBlocks::EXPOSED_COPPER_BULB), map.end());
}

TEST_F(HoneycombItemTest, WaxablesMapContainsChiseledCopperMappings)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 凿制铜映射
    EXPECT_NE(map.find(VanillaBlocks::CHISELED_COPPER), map.end());
    EXPECT_NE(map.find(VanillaBlocks::EXPOSED_CHISELED_COPPER), map.end());
}

TEST_F(HoneycombItemTest, WaxablesMapContainsChainMappings)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 铜链映射
    EXPECT_NE(map.find(VanillaBlocks::COPPER_CHAIN), map.end());
    EXPECT_NE(map.find(VanillaBlocks::EXPOSED_COPPER_CHAIN), map.end());
}

TEST_F(HoneycombItemTest, WaxablesMapContainsLanternMappings)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 铜灯笼映射
    EXPECT_NE(map.find(VanillaBlocks::COPPER_LANTERN), map.end());
    EXPECT_NE(map.find(VanillaBlocks::EXPOSED_COPPER_LANTERN), map.end());
}

TEST_F(HoneycombItemTest, WaxedCopperMapsCorrectly)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 验证铜块 -> 涂蜡铜块的映射
    ASSERT_NE(map.find(VanillaBlocks::COPPER_BLOCK), map.end());
    EXPECT_EQ(map[VanillaBlocks::COPPER_BLOCK], VanillaBlocks::WAXED_COPPER_BLOCK);

    ASSERT_NE(map.find(VanillaBlocks::EXPOSED_COPPER), map.end());
    EXPECT_EQ(map[VanillaBlocks::EXPOSED_COPPER], VanillaBlocks::WAXED_EXPOSED_COPPER);

    ASSERT_NE(map.find(VanillaBlocks::WEATHERED_COPPER), map.end());
    EXPECT_EQ(map[VanillaBlocks::WEATHERED_COPPER], VanillaBlocks::WAXED_WEATHERED_COPPER);

    ASSERT_NE(map.find(VanillaBlocks::OXIDIZED_COPPER), map.end());
    EXPECT_EQ(map[VanillaBlocks::OXIDIZED_COPPER], VanillaBlocks::WAXED_OXIDIZED_COPPER);
}

TEST_F(HoneycombItemTest, WaxablesMapDoesNotContainWaxedBlocks)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 涂蜡方块不应出现在涂蜡映射表的键中（已经涂蜡，不需要再涂）
    EXPECT_EQ(map.find(VanillaBlocks::WAXED_COPPER_BLOCK), map.end());
    EXPECT_EQ(map.find(VanillaBlocks::WAXED_EXPOSED_COPPER), map.end());
    EXPECT_EQ(map.find(VanillaBlocks::WAXED_WEATHERED_COPPER), map.end());
    EXPECT_EQ(map.find(VanillaBlocks::WAXED_OXIDIZED_COPPER), map.end());
}

TEST_F(HoneycombItemTest, WaxablesMapDoesNotContainNonCopperBlocks)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 非铜方块不应出现在涂蜡映射表中
    EXPECT_EQ(map.find(VanillaBlocks::STONE), map.end());
    EXPECT_EQ(map.find(VanillaBlocks::OAK_PLANKS), map.end());
}

// ============================================================================
// 除蜡映射表测试
// ============================================================================

TEST_F(HoneycombItemTest, WaxOffMapIsInverseOfWaxablesMap)
{
    auto& waxables = item::items::HoneycombItem::getWaxablesMap();
    auto& waxOff = item::items::HoneycombItem::getWaxOffMap();

    // 除蜡映射表应该与涂蜡映射表互为反向
    for (const auto& [unwaxed, waxed] : waxables) {
        auto it = waxOff.find(waxed);
        EXPECT_NE(it, waxOff.end()) << "Waxed block should be in wax-off map";
        if (it != waxOff.end()) {
            EXPECT_EQ(it->second, unwaxed) << "Wax-off map should reverse waxables map";
        }
    }
}

TEST_F(HoneycombItemTest, WaxOffMapContainsWaxedCopperBlocks)
{
    auto& map = item::items::HoneycombItem::getWaxOffMap();

    EXPECT_NE(map.find(VanillaBlocks::WAXED_COPPER_BLOCK), map.end());
    EXPECT_NE(map.find(VanillaBlocks::WAXED_EXPOSED_COPPER), map.end());
    EXPECT_NE(map.find(VanillaBlocks::WAXED_WEATHERED_COPPER), map.end());
    EXPECT_NE(map.find(VanillaBlocks::WAXED_OXIDIZED_COPPER), map.end());
}

TEST_F(HoneycombItemTest, WaxOffMapsToCorrectUnwaxedBlocks)
{
    auto& map = item::items::HoneycombItem::getWaxOffMap();

    EXPECT_EQ(map[VanillaBlocks::WAXED_COPPER_BLOCK], VanillaBlocks::COPPER_BLOCK);
    EXPECT_EQ(map[VanillaBlocks::WAXED_EXPOSED_COPPER], VanillaBlocks::EXPOSED_COPPER);
    EXPECT_EQ(map[VanillaBlocks::WAXED_WEATHERED_COPPER], VanillaBlocks::WEATHERED_COPPER);
    EXPECT_EQ(map[VanillaBlocks::WAXED_OXIDIZED_COPPER], VanillaBlocks::OXIDIZED_COPPER);
}

// ============================================================================
// getWaxed / getWaxedOff 测试
// ============================================================================

TEST_F(HoneycombItemTest, GetWaxedReturnsCorrectState)
{
    // 铜块默认状态 -> 涂蜡铜块默认状态
    auto result = item::items::HoneycombItem::getWaxed(VanillaBlocks::COPPER_BLOCK->defaultState());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(&result->owner(), VanillaBlocks::WAXED_COPPER_BLOCK);
}

TEST_F(HoneycombItemTest, GetWaxedReturnsNulloptForNonCopperBlock)
{
    // 石头不可涂蜡
    auto result = item::items::HoneycombItem::getWaxed(VanillaBlocks::STONE->defaultState());
    EXPECT_FALSE(result.has_value());
}

TEST_F(HoneycombItemTest, GetWaxedReturnsNulloptForAlreadyWaxedBlock)
{
    // 涂蜡铜块不可再次涂蜡
    auto result = item::items::HoneycombItem::getWaxed(VanillaBlocks::WAXED_COPPER_BLOCK->defaultState());
    EXPECT_FALSE(result.has_value());
}

TEST_F(HoneycombItemTest, GetWaxedOffReturnsCorrectState)
{
    // 涂蜡铜块 -> 铜块
    auto result = item::items::HoneycombItem::getWaxedOff(VanillaBlocks::WAXED_COPPER_BLOCK->defaultState());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(&result->owner(), VanillaBlocks::COPPER_BLOCK);
}

TEST_F(HoneycombItemTest, GetWaxedOffReturnsNulloptForNonWaxedBlock)
{
    // 铜块不是涂蜡方块，不能除蜡
    auto result = item::items::HoneycombItem::getWaxedOff(VanillaBlocks::COPPER_BLOCK->defaultState());
    EXPECT_FALSE(result.has_value());
}

TEST_F(HoneycombItemTest, GetWaxedOffReturnsNulloptForNonCopperBlock)
{
    // 石头不能除蜡
    auto result = item::items::HoneycombItem::getWaxedOff(VanillaBlocks::STONE->defaultState());
    EXPECT_FALSE(result.has_value());
}

TEST_F(HoneycombItemTest, GetWaxedPreservesCompatibleProperties)
{
    // 验证涂蜡时属性保留：切制铜楼梯有 FACING 等属性
    const BlockState& cutCopperStairsState = VanillaBlocks::CUT_COPPER_STAIRS->defaultState();
    auto result = item::items::HoneycombItem::getWaxed(cutCopperStairsState);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(&result->owner(), VanillaBlocks::WAXED_CUT_COPPER_STAIRS);
}

TEST_F(HoneycombItemTest, GetWaxedOffPreservesCompatibleProperties)
{
    // 验证除蜡时属性保留
    const BlockState& waxedCutCopperStairsState = VanillaBlocks::WAXED_CUT_COPPER_STAIRS->defaultState();
    auto result = item::items::HoneycombItem::getWaxedOff(waxedCutCopperStairsState);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(&result->owner(), VanillaBlocks::CUT_COPPER_STAIRS);
}

// ============================================================================
// 涂蜡/除蜡映射表大小测试
// ============================================================================

TEST_F(HoneycombItemTest, WaxablesMapHasExpectedSize)
{
    auto& map = item::items::HoneycombItem::getWaxablesMap();

    // 11种铜方块类型 × 4个氧化等级 = 44个映射
    // 铜块、切制铜、楼梯、台阶、门、活板门、格栅、灯、凿制铜、链、灯笼
    EXPECT_EQ(map.size(), 44u) << "Waxables map should contain 44 entries (11 types × 4 oxidation levels)";
}

TEST_F(HoneycombItemTest, WaxOffMapHasSameSizeAsWaxablesMap)
{
    auto& waxables = item::items::HoneycombItem::getWaxablesMap();
    auto& waxOff = item::items::HoneycombItem::getWaxOffMap();

    EXPECT_EQ(waxOff.size(), waxables.size());
}

} // namespace
} // namespace mc
