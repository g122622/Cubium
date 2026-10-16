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

// DyeItemHelper 的染料判定与颜色查询测试。
//
// 覆盖核心契约：判定集合必须与 vanilla 1.21.11 的 DyeItem 实例集合**完全一致**——
// 仅 16 种 *_dye。ink_sac / cocoa_beans / bone_meal 分别是 InkSacItem / BlockItem /
// BoneMealItem，不是 DyeItem，不得被判为染料（否则它们能染项圈、进织布机染料槽、
// 参与盔甲与烟花之星配方，均与 vanilla 不符）。

#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/items/DyeItemHelper.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyeColor.hpp"

#include <array>
#include <vector>

using namespace mc;
using mc::item::DyeItemHelper;

namespace {

/// 16 种标准染料物品与其颜色的期望映射（vanilla DyeItem 实例全集）
struct ExpectedDye {
    Item* item;
    DyeColor color;
};

const std::vector<ExpectedDye>& _allVanillaDyes()
{
    static const std::vector<ExpectedDye> dyes = {
        {Items::WHITE_DYE, DyeColor::White},
        {Items::ORANGE_DYE, DyeColor::Orange},
        {Items::MAGENTA_DYE, DyeColor::Magenta},
        {Items::LIGHT_BLUE_DYE, DyeColor::LightBlue},
        {Items::YELLOW_DYE, DyeColor::Yellow},
        {Items::LIME_DYE, DyeColor::Lime},
        {Items::PINK_DYE, DyeColor::Pink},
        {Items::GRAY_DYE, DyeColor::Gray},
        {Items::LIGHT_GRAY_DYE, DyeColor::LightGray},
        {Items::CYAN_DYE, DyeColor::Cyan},
        {Items::PURPLE_DYE, DyeColor::Purple},
        {Items::BLUE_DYE, DyeColor::Blue},
        {Items::BROWN_DYE, DyeColor::Brown},
        {Items::GREEN_DYE, DyeColor::Green},
        {Items::RED_DYE, DyeColor::Red},
        {Items::BLACK_DYE, DyeColor::Black},
    };
    return dyes;
}

} // namespace

class DyeItemHelperTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

// ============================================================================
// 标准染料：16 种 *_dye 全部被识别，且颜色映射正确
// ============================================================================

TEST_F(DyeItemHelperTest, AllSixteenDyeItemsAreRecognized)
{
    for (const auto& [item, color] : _allVanillaDyes()) {
        ASSERT_NE(item, nullptr) << item->itemLocation().toString() << " should be registered";
        EXPECT_TRUE(DyeItemHelper::isDyeItem(item)) << item->itemLocation().toString() << " should be a dye";
        const auto resolved = DyeItemHelper::dyeColorOfItem(item);
        ASSERT_TRUE(resolved.has_value()) << item->itemLocation().toString() << " should resolve a color";
        EXPECT_EQ(*resolved, color) << item->itemLocation().toString() << " should map to the expected color";
    }
}

TEST_F(DyeItemHelperTest, EveryColorHasExactlyOneDyeItem)
{
    // 16 种颜色各恰好对应一个染料物品（不得有颜色缺失或重复占用）
    std::array<i32, static_cast<usize>(DyeColor::Count)> counts{};
    for (const auto& [item, color] : _allVanillaDyes()) {
        ++counts[static_cast<usize>(color)];
    }
    for (usize i = 0; i < counts.size(); ++i) {
        EXPECT_EQ(counts[i], 1) << "DyeColor index " << i << " should have exactly one dye item";
    }
}

// ============================================================================
// 非染料物品：vanilla 中不是 DyeItem 的"类染料"物品必须被拒绝
//
// 这是本测试文件的核心契约。这 3 个物品各自对应一种染料色，但 vanilla 中它们是
// InkSacItem / BlockItem / BoneMealItem，只在合成配方里作为对应染料的原料
// （black_dye ← ink_sac、brown_dye ← cocoa_beans、white_dye ← bone_meal），
// 不能用于任何 `instanceof DyeItem` 判定点。
// ============================================================================

TEST_F(DyeItemHelperTest, InkSacIsNotADyeItem)
{
    ASSERT_NE(Items::INK_SAC, nullptr);
    EXPECT_FALSE(DyeItemHelper::isDyeItem(Items::INK_SAC));
    EXPECT_FALSE(DyeItemHelper::dyeColorOfItem(Items::INK_SAC).has_value());
}

TEST_F(DyeItemHelperTest, CocoaBeansIsNotADyeItem)
{
    ASSERT_NE(Items::COCOA_BEANS, nullptr);
    EXPECT_FALSE(DyeItemHelper::isDyeItem(Items::COCOA_BEANS));
    EXPECT_FALSE(DyeItemHelper::dyeColorOfItem(Items::COCOA_BEANS).has_value());
}

TEST_F(DyeItemHelperTest, BoneMealIsNotADyeItem)
{
    ASSERT_NE(Items::BONE_MEAL, nullptr);
    EXPECT_FALSE(DyeItemHelper::isDyeItem(Items::BONE_MEAL));
    EXPECT_FALSE(DyeItemHelper::dyeColorOfItem(Items::BONE_MEAL).has_value());
}

// ============================================================================
// 非物品与空指针：不得崩溃，一律返回"非染料"
// ============================================================================

TEST_F(DyeItemHelperTest, NullItemIsNotADyeItem)
{
    EXPECT_FALSE(DyeItemHelper::isDyeItem(nullptr));
    EXPECT_FALSE(DyeItemHelper::dyeColorOfItem(nullptr).has_value());
}

TEST_F(DyeItemHelperTest, NonDyeItemIsRejected)
{
    ASSERT_NE(Items::STICK, nullptr);
    EXPECT_FALSE(DyeItemHelper::isDyeItem(Items::STICK));
    EXPECT_FALSE(DyeItemHelper::dyeColorOfItem(Items::STICK).has_value());
}

// ============================================================================
// 已删除的旧名 lapis_lazuli_dye 不得再被解析为蓝色染料
// ============================================================================

TEST_F(DyeItemHelperTest, RemovedLegacyLapisLazuliDyeIdResolvesToNoItem)
{
    // 项目曾注册旧名 minecraft:lapis_lazuli_dye（与 1.21.11 blue_dye 重复），现已删除。
    // 该旧 id 不得再解析出任何物品，蓝色染料只能经 minecraft:blue_dye 获得。
    Item* legacy = ItemRegistry::instance().getItem(ResourceLocation("minecraft:lapis_lazuli_dye"));
    EXPECT_EQ(legacy, nullptr);
    EXPECT_FALSE(DyeItemHelper::isDyeItem(legacy));
}
