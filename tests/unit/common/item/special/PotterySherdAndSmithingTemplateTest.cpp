/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/PotterySherdItem.hpp"
#include "common/item/items/special/SmithingTemplateItem.hpp"
#include "common/world/blockentity/interactive/DecoratedPotPattern.hpp"

using namespace mc;
using namespace mc::blockentity;

namespace {

class DecoratedPotPatternTest : public ::testing::Test {
protected:
    // 无需初始化，DecoratedPotPatterns 是纯静态工具类
};

// ========== 枚举值测试 ==========

TEST_F(DecoratedPotPatternTest, EnumValuesAreCorrect)
{
    // 验证枚举值与 MC 原版一致
    EXPECT_EQ(static_cast<int>(DecoratedPotPattern::Blank), 0);
    EXPECT_EQ(static_cast<int>(DecoratedPotPattern::Angler), 1);
    EXPECT_EQ(static_cast<int>(DecoratedPotPattern::Snort), 20);
    EXPECT_EQ(static_cast<int>(DecoratedPotPattern::Flow), 21);
    EXPECT_EQ(static_cast<int>(DecoratedPotPattern::Guster), 22);
    EXPECT_EQ(static_cast<int>(DecoratedPotPattern::Scrape), 23);
    EXPECT_EQ(static_cast<int>(DecoratedPotPattern::Count), 24);
}

// ========== byName 测试 ==========

TEST_F(DecoratedPotPatternTest, ByNameReturnsCorrectPattern)
{
    // 验证名称查找返回正确的图案类型
    EXPECT_EQ(DecoratedPotPatterns::byName("angler"), DecoratedPotPattern::Angler);
    EXPECT_EQ(DecoratedPotPatterns::byName("flow"), DecoratedPotPattern::Flow);
    EXPECT_EQ(DecoratedPotPatterns::byName("guster"), DecoratedPotPattern::Guster);
    EXPECT_EQ(DecoratedPotPatterns::byName("scrape"), DecoratedPotPattern::Scrape);
    EXPECT_EQ(DecoratedPotPatterns::byName("blank"), DecoratedPotPattern::Blank);
    EXPECT_EQ(DecoratedPotPatterns::byName("heart"), DecoratedPotPattern::Heart);
    EXPECT_EQ(DecoratedPotPatterns::byName("skull"), DecoratedPotPattern::Skull);
}

TEST_F(DecoratedPotPatternTest, ByNameReturnsBlankForUnknown)
{
    // 不存在的名称应返回 Blank
    EXPECT_EQ(DecoratedPotPatterns::byName("nonexistent"), DecoratedPotPattern::Blank);
    EXPECT_EQ(DecoratedPotPatterns::byName(""), DecoratedPotPattern::Blank);
}

// ========== getAssetId 测试 ==========

TEST_F(DecoratedPotPatternTest, GetAssetIdReturnsCorrectPaths)
{
    // Blank 图案使用默认纹理
    EXPECT_EQ(DecoratedPotPatterns::getAssetId(DecoratedPotPattern::Blank), "decorated_pot_side");

    // 非空白图案格式为 {name}_pottery_pattern
    EXPECT_EQ(DecoratedPotPatterns::getAssetId(DecoratedPotPattern::Angler), "angler_pottery_pattern");
    EXPECT_EQ(DecoratedPotPatterns::getAssetId(DecoratedPotPattern::Flow), "flow_pottery_pattern");
    EXPECT_EQ(DecoratedPotPatterns::getAssetId(DecoratedPotPattern::Guster), "guster_pottery_pattern");
    EXPECT_EQ(DecoratedPotPatterns::getAssetId(DecoratedPotPattern::Scrape), "scrape_pottery_pattern");
    EXPECT_EQ(DecoratedPotPatterns::getAssetId(DecoratedPotPattern::Heart), "heart_pottery_pattern");
}

// ========== getTranslationKey 测试 ==========

TEST_F(DecoratedPotPatternTest, GetTranslationKeyReturnsCorrectKeys)
{
    // Blank 图案使用方块翻译键
    EXPECT_EQ(DecoratedPotPatterns::getTranslationKey(DecoratedPotPattern::Blank), "block.minecraft.decorated_pot");

    // 陶片使用物品翻译键
    EXPECT_EQ(
        DecoratedPotPatterns::getTranslationKey(DecoratedPotPattern::Angler), "item.minecraft.angler_pottery_sherd");
    EXPECT_EQ(DecoratedPotPatterns::getTranslationKey(DecoratedPotPattern::Flow), "item.minecraft.flow_pottery_sherd");
    EXPECT_EQ(
        DecoratedPotPatterns::getTranslationKey(DecoratedPotPattern::Guster), "item.minecraft.guster_pottery_sherd");
    EXPECT_EQ(
        DecoratedPotPatterns::getTranslationKey(DecoratedPotPattern::ArmsUp), "item.minecraft.arms_up_pottery_sherd");
}

// ========== getName 测试 ==========

TEST_F(DecoratedPotPatternTest, GetNameReturnsCorrectNames)
{
    EXPECT_EQ(DecoratedPotPatterns::getName(DecoratedPotPattern::Blank), "blank");
    EXPECT_EQ(DecoratedPotPatterns::getName(DecoratedPotPattern::Angler), "angler");
    EXPECT_EQ(DecoratedPotPatterns::getName(DecoratedPotPattern::Flow), "flow");
    EXPECT_EQ(DecoratedPotPatterns::getName(DecoratedPotPattern::ArmsUp), "arms_up");
    EXPECT_EQ(DecoratedPotPatterns::getName(DecoratedPotPattern::Heartbreak), "heartbreak");
}

// ========== isBlank 测试 ==========

TEST_F(DecoratedPotPatternTest, IsBlankReturnsCorrectValue)
{
    EXPECT_TRUE(DecoratedPotPatterns::isBlank(DecoratedPotPattern::Blank));
    EXPECT_FALSE(DecoratedPotPatterns::isBlank(DecoratedPotPattern::Angler));
    EXPECT_FALSE(DecoratedPotPatterns::isBlank(DecoratedPotPattern::Flow));
}

// ========== byName 与 getName 往返测试 ==========

TEST_F(DecoratedPotPatternTest, ByNameAndGetNameAreInverse)
{
    // 验证 byName(getName(pattern)) == pattern 对所有图案成立
    for (int i = 0; i < static_cast<int>(DecoratedPotPattern::Count); ++i) {
        auto pattern = static_cast<DecoratedPotPattern>(i);
        std::string name = DecoratedPotPatterns::getName(pattern);
        EXPECT_EQ(DecoratedPotPatterns::byName(name), pattern) << "Roundtrip failed for pattern index " << i;
    }
}

// ========== PotterySherdItem 注册测试 ==========

class PotterySherdItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
};

TEST_F(PotterySherdItemTest, AllSherdsAreRegistered)
{
    // 验证1.20考古学陶片全部注册
    ASSERT_NE(Items::ANGLER_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::ARCHER_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::ARMS_UP_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::BLADE_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::BREWER_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::BURN_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::DANGER_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::EXPLORER_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::FRIEND_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::HEART_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::HEARTBREAK_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::HOWL_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::MINER_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::MOURNER_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::PLENTY_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::PRIZE_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::SHEAF_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::SHELTER_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::SKULL_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::SNORT_POTTERY_SHERD, nullptr);

    // 验证1.21试炼密室陶片全部注册
    ASSERT_NE(Items::FLOW_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::GUSTER_POTTERY_SHERD, nullptr);
    ASSERT_NE(Items::SCRAPE_POTTERY_SHERD, nullptr);
}

TEST_F(PotterySherdItemTest, SherdsArePotterySherdItems)
{
    // 验证所有陶片都是 PotterySherdItem 类型
    auto* angler = dynamic_cast<item::PotterySherdItem*>(Items::ANGLER_POTTERY_SHERD);
    ASSERT_NE(angler, nullptr);
    EXPECT_EQ(angler->getPattern(), DecoratedPotPattern::Angler);

    auto* flow = dynamic_cast<item::PotterySherdItem*>(Items::FLOW_POTTERY_SHERD);
    ASSERT_NE(flow, nullptr);
    EXPECT_EQ(flow->getPattern(), DecoratedPotPattern::Flow);

    auto* guster = dynamic_cast<item::PotterySherdItem*>(Items::GUSTER_POTTERY_SHERD);
    ASSERT_NE(guster, nullptr);
    EXPECT_EQ(guster->getPattern(), DecoratedPotPattern::Guster);

    auto* scrape = dynamic_cast<item::PotterySherdItem*>(Items::SCRAPE_POTTERY_SHERD);
    ASSERT_NE(scrape, nullptr);
    EXPECT_EQ(scrape->getPattern(), DecoratedPotPattern::Scrape);
}

TEST_F(PotterySherdItemTest, SherdResourceLocations)
{
    // 验证陶片的资源位置与 MC 原版一致
    EXPECT_EQ(Items::ANGLER_POTTERY_SHERD->itemLocation().toString(), "minecraft:angler_pottery_sherd");
    EXPECT_EQ(Items::FLOW_POTTERY_SHERD->itemLocation().toString(), "minecraft:flow_pottery_sherd");
    EXPECT_EQ(Items::GUSTER_POTTERY_SHERD->itemLocation().toString(), "minecraft:guster_pottery_sherd");
    EXPECT_EQ(Items::SCRAPE_POTTERY_SHERD->itemLocation().toString(), "minecraft:scrape_pottery_sherd");
}

TEST_F(PotterySherdItemTest, SherdMaxStackSize)
{
    // MC 原版中陶片堆叠数为 64
    EXPECT_EQ(Items::ANGLER_POTTERY_SHERD->maxStackSize(), 64);
    EXPECT_EQ(Items::FLOW_POTTERY_SHERD->maxStackSize(), 64);
}

// ========== SmithingTemplateItem 注册测试 ==========

class SmithingTemplateItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
};

TEST_F(SmithingTemplateItemTest, AllTemplatesAreRegistered)
{
    // 下界合金升级模板
    ASSERT_NE(Items::NETHERITE_UPGRADE_SMITHING_TEMPLATE, nullptr);

    // 18种盔甲纹饰模板
    ASSERT_NE(Items::SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::VEX_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::WILD_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::COAST_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::DUNE_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::WAYFINDER_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::RAISER_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::SHAPER_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::HOST_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::WARD_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::SILENCE_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::TIDE_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::SNOUT_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::RIB_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::EYE_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::SPIRE_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::FLOW_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
    ASSERT_NE(Items::BOLT_ARMOR_TRIM_SMITHING_TEMPLATE, nullptr);
}

TEST_F(SmithingTemplateItemTest, TemplatesAreSmithingTemplateItems)
{
    // 验证下界合金升级模板是 SmithingTemplateItem 类型
    auto* netherite = dynamic_cast<item::SmithingTemplateItem*>(Items::NETHERITE_UPGRADE_SMITHING_TEMPLATE);
    ASSERT_NE(netherite, nullptr);
    EXPECT_EQ(netherite->getTemplateType(), item::SmithingTemplateType::NetheriteUpgrade);

    // 验证盔甲纹饰模板是 SmithingTemplateItem 类型且类型为 ArmorTrim
    auto* sentry = dynamic_cast<item::SmithingTemplateItem*>(Items::SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE);
    ASSERT_NE(sentry, nullptr);
    EXPECT_EQ(sentry->getTemplateType(), item::SmithingTemplateType::ArmorTrim);

    auto* rib = dynamic_cast<item::SmithingTemplateItem*>(Items::RIB_ARMOR_TRIM_SMITHING_TEMPLATE);
    ASSERT_NE(rib, nullptr);
    EXPECT_EQ(rib->getTemplateType(), item::SmithingTemplateType::ArmorTrim);

    auto* flow = dynamic_cast<item::SmithingTemplateItem*>(Items::FLOW_ARMOR_TRIM_SMITHING_TEMPLATE);
    ASSERT_NE(flow, nullptr);
    EXPECT_EQ(flow->getTemplateType(), item::SmithingTemplateType::ArmorTrim);
}

TEST_F(SmithingTemplateItemTest, TemplateResourceLocations)
{
    // 验证模板的资源位置与 MC 原版一致
    EXPECT_EQ(Items::NETHERITE_UPGRADE_SMITHING_TEMPLATE->itemLocation().toString(),
        "minecraft:netherite_upgrade_smithing_template");
    EXPECT_EQ(Items::SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE->itemLocation().toString(),
        "minecraft:sentry_armor_trim_smithing_template");
    EXPECT_EQ(Items::RIB_ARMOR_TRIM_SMITHING_TEMPLATE->itemLocation().toString(),
        "minecraft:rib_armor_trim_smithing_template");
    EXPECT_EQ(Items::FLOW_ARMOR_TRIM_SMITHING_TEMPLATE->itemLocation().toString(),
        "minecraft:flow_armor_trim_smithing_template");
    EXPECT_EQ(Items::BOLT_ARMOR_TRIM_SMITHING_TEMPLATE->itemLocation().toString(),
        "minecraft:bolt_armor_trim_smithing_template");
}

TEST_F(SmithingTemplateItemTest, TemplateMaxStackSize)
{
    // MC 原版中锻造模板堆叠数为 64
    EXPECT_EQ(Items::NETHERITE_UPGRADE_SMITHING_TEMPLATE->maxStackSize(), 64);
    EXPECT_EQ(Items::SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE->maxStackSize(), 64);
    EXPECT_EQ(Items::RIB_ARMOR_TRIM_SMITHING_TEMPLATE->maxStackSize(), 64);
}

} // namespace
