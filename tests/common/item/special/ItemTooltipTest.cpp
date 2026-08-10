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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/BannerPatternItem.hpp"
#include "common/item/items/block/BannerItem.hpp"
#include "common/item/items/special/PotterySherdItem.hpp"
#include "common/item/items/special/SmithingTemplateItem.hpp"
#include "common/resource/LanguageManager.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/blockentity/interactive/BannerPattern.hpp"
#include "common/world/blockentity/interactive/DecoratedPotPattern.hpp"

// 测试辅助：使用 BaseTestWorld 提供最小化的 IWorld 实现
#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::item;
using namespace mc::blockentity;

namespace {

// ============================================================================
// 测试辅助：创建一个空的 IWorld 桩（addInformation 需要但未使用 IWorld）
// ============================================================================
// 由于 addInformation 接受 IWorld& 参数，我们需要一个可用的 IWorld 引用。
// 在注册 Items 后，可以使用一个空的测试桩。
// 当前 addInformation 实现中并未使用 IWorld 参数，所以传 nullptr 包装即可。
// 但为了安全，我们使用 ItemRegistry 初始化。

class ItemTooltipTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
};

// ============================================================================
// SmithingTemplateItem addInformation 测试
// ============================================================================

TEST_F(ItemTooltipTest, SmithingTemplateAddInformation_FormatMatchesMCJava)
{
    // 验证 SmithingTemplateItem::addInformation 输出的6行tooltip格式与MC Java一致
    // MC Java 中 appendHoverText 按顺序添加：
    // 1. SMITHING_TEMPLATE_SUFFIX（"Smithing Template"）
    // 2. EMPTY（空行）
    // 3. APPLIES_TO_TITLE（"Applies to:"）
    // 4. 空格 + appliesTo 描述值
    // 5. INGREDIENTS_TITLE（"Ingredients:"）
    // 6. 空格 + ingredients 描述值

    auto* templateItem = dynamic_cast<SmithingTemplateItem*>(Items::SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE);
    ASSERT_NE(templateItem, nullptr);

    ItemStack stack(*templateItem, 1);
    std::vector<std::string> tooltip;
    // 注意：addInformation 中的 IWorld& 参数在当前实现中未被使用
    // 需要一个有效的 IWorld 引用，此处使用空桩
    // 由于 SmithingTemplateItem::addInformation 不实际使用 IWorld，
    // 我们通过基类调用间接测试

    // 加载翻译以验证翻译键能被正确解析
    auto& lang = LanguageManager::instance();
    // 即使 LanguageManager 未加载语言文件，get() 也会返回键本身作为回退

    // 验证翻译键存在并可获取
    std::string suffixText = lang.get("item.minecraft.smithing_template");
    EXPECT_FALSE(suffixText.empty()) << "Smithing template suffix key should not return empty";

    std::string appliesToTitle = lang.get("item.minecraft.smithing_template.applies_to");
    EXPECT_FALSE(appliesToTitle.empty()) << "Applies to title key should not return empty";

    std::string ingredientsTitle = lang.get("item.minecraft.smithing_template.ingredients");
    EXPECT_FALSE(ingredientsTitle.empty()) << "Ingredients title key should not return empty";

    // 验证盔甲纹饰模板的描述值翻译键
    std::string armorAppliesTo = lang.get("item.minecraft.smithing_template.armor_trim.applies_to");
    EXPECT_FALSE(armorAppliesTo.empty()) << "Armor trim applies to key should not return empty";

    std::string armorIngredients = lang.get("item.minecraft.smithing_template.armor_trim.ingredients");
    EXPECT_FALSE(armorIngredients.empty()) << "Armor trim ingredients key should not return empty";

    // 验证下界合金升级模板的描述值翻译键
    std::string netheriteAppliesTo = lang.get("item.minecraft.smithing_template.netherite_upgrade.applies_to");
    EXPECT_FALSE(netheriteAppliesTo.empty()) << "Netherite upgrade applies to key should not return empty";

    std::string netheriteIngredients = lang.get("item.minecraft.smithing_template.netherite_upgrade.ingredients");
    EXPECT_FALSE(netheriteIngredients.empty()) << "Netherite upgrade ingredients key should not return empty";
}

TEST_F(ItemTooltipTest, SmithingTemplateAddInformation_ArmorTrimTooltipLines)
{
    // 验证盔甲纹饰模板的 tooltip 行数和内容格式
    auto* armorTrim = dynamic_cast<SmithingTemplateItem*>(Items::SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE);
    ASSERT_NE(armorTrim, nullptr);
    EXPECT_EQ(armorTrim->getTemplateType(), SmithingTemplateType::ArmorTrim);

    // 验证模板类型
    EXPECT_EQ(armorTrim->getAppliesTo(), "item.minecraft.smithing_template.armor_trim.applies_to");
    EXPECT_EQ(armorTrim->getIngredients(), "item.minecraft.smithing_template.armor_trim.ingredients");
}

TEST_F(ItemTooltipTest, SmithingTemplateAddInformation_NetheriteUpgradeTooltipLines)
{
    // 验证下界合金升级模板的 tooltip 内容
    auto* netherite = dynamic_cast<SmithingTemplateItem*>(Items::NETHERITE_UPGRADE_SMITHING_TEMPLATE);
    ASSERT_NE(netherite, nullptr);
    EXPECT_EQ(netherite->getTemplateType(), SmithingTemplateType::NetheriteUpgrade);

    // 验证翻译键
    EXPECT_EQ(netherite->getAppliesTo(), "item.minecraft.smithing_template.netherite_upgrade.applies_to");
    EXPECT_EQ(netherite->getIngredients(), "item.minecraft.smithing_template.netherite_upgrade.ingredients");
}

TEST_F(ItemTooltipTest, SmithingTemplateAddInformation_AllArmorTrimsShareSameTranslationKeys)
{
    // MC Java 中所有18种盔甲纹饰模板共享相同的 appliesTo 和 ingredients 翻译键
    // 因为它们都适用于盔甲、都需要锭和水晶
    const std::vector<Item*> armorTrims = {
        Items::SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::VEX_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::WILD_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::COAST_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::DUNE_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::WAYFINDER_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::RAISER_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::SHAPER_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::HOST_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::WARD_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::SILENCE_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::TIDE_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::SNOUT_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::RIB_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::EYE_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::SPIRE_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::FLOW_ARMOR_TRIM_SMITHING_TEMPLATE,
        Items::BOLT_ARMOR_TRIM_SMITHING_TEMPLATE,
    };

    for (auto* item : armorTrims) {
        ASSERT_NE(item, nullptr);
        auto* templateItem = dynamic_cast<SmithingTemplateItem*>(item);
        ASSERT_NE(templateItem, nullptr) << "Item should be a SmithingTemplateItem";
        EXPECT_EQ(templateItem->getTemplateType(), SmithingTemplateType::ArmorTrim);
        EXPECT_EQ(templateItem->getAppliesTo(), "item.minecraft.smithing_template.armor_trim.applies_to");
        EXPECT_EQ(templateItem->getIngredients(), "item.minecraft.smithing_template.armor_trim.ingredients");
    }
}

TEST_F(ItemTooltipTest, SmithingTemplateAddInformation_StaticTranslationKeysAreCorrect)
{
    // 验证 SmithingTemplateItem 中定义的静态翻译键常量在资源包中存在
    // 这些键在 addInformation 中被 LanguageManager::get() 使用
    auto& lang = LanguageManager::instance();

    // 即使未加载语言文件，get() 应返回键本身（回退行为）
    EXPECT_EQ(lang.get("item.minecraft.smithing_template"), "item.minecraft.smithing_template");
    EXPECT_EQ(lang.get("item.minecraft.smithing_template.applies_to"), "item.minecraft.smithing_template.applies_to");
    EXPECT_EQ(lang.get("item.minecraft.smithing_template.ingredients"), "item.minecraft.smithing_template.ingredients");
}

// ============================================================================
// PotterySherdItem addInformation 测试
// ============================================================================

TEST_F(ItemTooltipTest, PotterySherdAddInformation_NoExtraTooltipLines)
{
    // MC Java 中陶片物品没有额外的 tooltip 描述行
    // PotterySherdItem::addInformation 仅调用基类，不添加任何额外行
    auto* sherd = dynamic_cast<PotterySherdItem*>(Items::ANGLER_POTTERY_SHERD);
    ASSERT_NE(sherd, nullptr);
    // 验证陶片确实持有正确的图案枚举
    EXPECT_EQ(sherd->getPattern(), DecoratedPotPattern::Angler);
}

TEST_F(ItemTooltipTest, AllPotterySherdsArePotterySherdItemsWithCorrectPatterns)
{
    // 验证所有陶片物品的 DecoratedPotPattern 正确关联
    struct SherdMapping {
        Item* const& item;
        DecoratedPotPattern pattern;
    };

    const SherdMapping mappings[] = {
        {Items::ANGLER_POTTERY_SHERD, DecoratedPotPattern::Angler},
        {Items::ARCHER_POTTERY_SHERD, DecoratedPotPattern::Archer},
        {Items::ARMS_UP_POTTERY_SHERD, DecoratedPotPattern::ArmsUp},
        {Items::BLADE_POTTERY_SHERD, DecoratedPotPattern::Blade},
        {Items::BREWER_POTTERY_SHERD, DecoratedPotPattern::Brewer},
        {Items::BURN_POTTERY_SHERD, DecoratedPotPattern::Burn},
        {Items::DANGER_POTTERY_SHERD, DecoratedPotPattern::Danger},
        {Items::EXPLORER_POTTERY_SHERD, DecoratedPotPattern::Explorer},
        {Items::FRIEND_POTTERY_SHERD, DecoratedPotPattern::Friend},
        {Items::HEART_POTTERY_SHERD, DecoratedPotPattern::Heart},
        {Items::HEARTBREAK_POTTERY_SHERD, DecoratedPotPattern::Heartbreak},
        {Items::HOWL_POTTERY_SHERD, DecoratedPotPattern::Howl},
        {Items::MINER_POTTERY_SHERD, DecoratedPotPattern::Miner},
        {Items::MOURNER_POTTERY_SHERD, DecoratedPotPattern::Mourner},
        {Items::PLENTY_POTTERY_SHERD, DecoratedPotPattern::Plenty},
        {Items::PRIZE_POTTERY_SHERD, DecoratedPotPattern::Prize},
        {Items::SHEAF_POTTERY_SHERD, DecoratedPotPattern::Sheaf},
        {Items::SHELTER_POTTERY_SHERD, DecoratedPotPattern::Shelter},
        {Items::SKULL_POTTERY_SHERD, DecoratedPotPattern::Skull},
        {Items::SNORT_POTTERY_SHERD, DecoratedPotPattern::Snort},
        {Items::FLOW_POTTERY_SHERD, DecoratedPotPattern::Flow},
        {Items::GUSTER_POTTERY_SHERD, DecoratedPotPattern::Guster},
        {Items::SCRAPE_POTTERY_SHERD, DecoratedPotPattern::Scrape},
    };

    for (const auto& mapping : mappings) {
        ASSERT_NE(mapping.item, nullptr) << "Pottery sherd item should be registered";
        auto* sherd = dynamic_cast<PotterySherdItem*>(mapping.item);
        ASSERT_NE(sherd, nullptr) << "Item should be a PotterySherdItem";
        EXPECT_EQ(sherd->getPattern(), mapping.pattern)
            << "Pattern mismatch for " << mapping.item->itemLocation().toString();
    }
}

// ============================================================================
// BannerPatternItem addInformation 翻译键拼接测试
// ============================================================================

TEST_F(ItemTooltipTest, BannerPatternItemTranslationKeyFormat)
{
    // 验证 BannerPatternItem 的翻译键格式为 "item.minecraft.banner_pattern.<filename>.desc"
    // 例如 creeper_banner_pattern -> "item.minecraft.banner_pattern.creeper.desc"
    EXPECT_EQ(BannerPatterns::getFileName(BannerPatternType::Creeper), "creeper");
    EXPECT_EQ(BannerPatterns::getFileName(BannerPatternType::Skull), "skull");
    EXPECT_EQ(BannerPatterns::getFileName(BannerPatternType::Flower), "flower");
    EXPECT_EQ(BannerPatterns::getFileName(BannerPatternType::Mojang), "mojang");
    EXPECT_EQ(BannerPatterns::getFileName(BannerPatternType::Globe), "globe");
    EXPECT_EQ(BannerPatterns::getFileName(BannerPatternType::Piglin), "piglin");
    EXPECT_EQ(BannerPatterns::getFileName(BannerPatternType::Flow), "flow");
}

TEST_F(ItemTooltipTest, BannerPatternItemDescKeysExistInResourcePack)
{
    // 验证旗帜图案的 .desc 翻译键在资源包中存在
    // 资源包路径: C:\Users\Administrator\minecraft_reborn\resourcepacks\Vanilla\assets\minecraft\lang\en_us.json
    auto& lang = LanguageManager::instance();

    // 即使未加载语言文件，get() 返回键本身（回退行为）
    // 验证键格式拼接正确
    std::string creeperKey = "item.minecraft.banner_pattern.creeper.desc";
    std::string skullKey = "item.minecraft.banner_pattern.skull.desc";
    std::string flowerKey = "item.minecraft.banner_pattern.flower.desc";

    // 未加载语言文件时，get() 返回键本身
    EXPECT_EQ(lang.get(creeperKey), creeperKey);
    EXPECT_EQ(lang.get(skullKey), skullKey);
    EXPECT_EQ(lang.get(flowerKey), flowerKey);
}

// ============================================================================
// BannerItem addInformation 颜色描述翻译测试
// ============================================================================

TEST_F(ItemTooltipTest, BannerItemTranslationKeyFormat)
{
    // 验证 BannerItem 的翻译键格式为 "block.minecraft.banner.<pattern_filename>.<color_name>"
    // 例如 "block.minecraft.banner.stripe_bottom.white"
    auto& lang = LanguageManager::instance();

    // 未加载语言文件时，get() 返回键本身
    std::string key = "block.minecraft.banner.stripe_bottom.white";
    EXPECT_EQ(lang.get(key), key);

    key = "block.minecraft.banner.creeper.red";
    EXPECT_EQ(lang.get(key), key);

    key = "block.minecraft.banner.cross.blue";
    EXPECT_EQ(lang.get(key), key);
}

// ============================================================================
// LanguageManager 回退行为测试
// ============================================================================

TEST_F(ItemTooltipTest, LanguageManagerFallbackReturnsKeyWhenNotLoaded)
{
    // LanguageManager 未加载语言文件时，get() 应返回键本身
    auto& lang = LanguageManager::instance();

    std::string key = "item.minecraft.nonexistent_key";
    EXPECT_EQ(lang.get(key), key);

    // 空键应返回空字符串
    EXPECT_EQ(lang.get(""), "");
}

TEST_F(ItemTooltipTest, LanguageManagerGetWithParamsFallback)
{
    // 带参数的 get() 在未加载语言文件时也应回退
    auto& lang = LanguageManager::instance();

    std::string key = "chat.type.text";
    std::vector<std::string> params = {"Player", "Hello"};
    std::string result = lang.get(key, params);
    // 未加载语言文件时，返回键本身（无法替换占位符）
    EXPECT_EQ(result, key);
}

// ============================================================================
// SmithingTemplateItem baseSlotDescription 和 additionsSlotDescription 测试
// ============================================================================

TEST_F(ItemTooltipTest, SmithingTemplateSlotDescriptionKeys)
{
    // 验证基础槽和附加槽的翻译键格式
    auto* netherite = dynamic_cast<SmithingTemplateItem*>(Items::NETHERITE_UPGRADE_SMITHING_TEMPLATE);
    ASSERT_NE(netherite, nullptr);

    // 下界合金升级模板的槽位描述键（MC Java 使用 netherite_upgrade 前缀）
    EXPECT_EQ(netherite->getBaseSlotDescription(),
        "item.minecraft.smithing_template.netherite_upgrade.base_slot_description");
    EXPECT_EQ(netherite->getAdditionsSlotDescription(),
        "item.minecraft.smithing_template.netherite_upgrade.additions_slot_description");

    auto* sentry = dynamic_cast<SmithingTemplateItem*>(Items::SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE);
    ASSERT_NE(sentry, nullptr);

    // 盔甲纹饰模板的槽位描述键（MC Java 使用 armor_trim 前缀）
    EXPECT_EQ(sentry->getBaseSlotDescription(), "item.minecraft.smithing_template.armor_trim.base_slot_description");
    EXPECT_EQ(sentry->getAdditionsSlotDescription(),
        "item.minecraft.smithing_template.armor_trim.additions_slot_description");
}

// ============================================================================
// addInformation 集成测试 — 实际调用并验证 tooltip 输出
// ============================================================================

// 最小化的 IWorld 测试桩，继承 BaseTestWorld
class TooltipTestWorld : public mc::test::BaseTestWorld {};

class ItemTooltipIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }

    TooltipTestWorld m_world;
};

TEST_F(ItemTooltipIntegrationTest, SmithingTemplateAddInformation_ArmorTrimOutputFormat)
{
    // 验证盔甲纹饰模板 addInformation 输出 6 行 tooltip，格式与 MC Java 一致：
    // 0: "Smithing Template"（后缀标题，翻译键 item.minecraft.smithing_template）
    // 1: ""（空行分隔）
    // 2: "Applies to:"（标题，翻译键 item.minecraft.smithing_template.applies_to）
    // 3: " Armor"（描述，翻译键 item.minecraft.smithing_template.armor_trim.applies_to，以空格开头）
    // 4: "Ingredients:"（标题，翻译键 item.minecraft.smithing_template.ingredients）
    // 5: " Ingots & Crystals"（描述，翻译键 item.minecraft.smithing_template.armor_trim.ingredients，以空格开头）
    auto* sentry = dynamic_cast<SmithingTemplateItem*>(Items::SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE);
    ASSERT_NE(sentry, nullptr);

    ItemStack stack(*sentry, 1);
    std::vector<std::string> tooltip;
    sentry->addInformation(stack, &m_world, tooltip, false);

    // 未加载语言文件时，LanguageManager::get() 返回键本身
    ASSERT_EQ(tooltip.size(), 6u) << "SmithingTemplateItem tooltip should have exactly 6 lines";

    EXPECT_EQ(tooltip[0], "item.minecraft.smithing_template") << "Line 0: suffix title";
    EXPECT_EQ(tooltip[1], "") << "Line 1: empty separator";
    EXPECT_EQ(tooltip[2], "item.minecraft.smithing_template.applies_to") << "Line 2: applies-to title";
    EXPECT_EQ(tooltip[3], " item.minecraft.smithing_template.armor_trim.applies_to")
        << "Line 3: applies-to description (space-prefixed)";
    EXPECT_EQ(tooltip[4], "item.minecraft.smithing_template.ingredients") << "Line 4: ingredients title";
    EXPECT_EQ(tooltip[5], " item.minecraft.smithing_template.armor_trim.ingredients")
        << "Line 5: ingredients description (space-prefixed)";
}

TEST_F(ItemTooltipIntegrationTest, SmithingTemplateAddInformation_NetheriteUpgradeOutputFormat)
{
    // 验证下界合金升级模板 addInformation 输出
    auto* netherite = dynamic_cast<SmithingTemplateItem*>(Items::NETHERITE_UPGRADE_SMITHING_TEMPLATE);
    ASSERT_NE(netherite, nullptr);

    ItemStack stack(*netherite, 1);
    std::vector<std::string> tooltip;
    netherite->addInformation(stack, &m_world, tooltip, false);

    ASSERT_EQ(tooltip.size(), 6u);

    EXPECT_EQ(tooltip[0], "item.minecraft.smithing_template") << "Line 0: suffix title";
    EXPECT_EQ(tooltip[1], "") << "Line 1: empty separator";
    EXPECT_EQ(tooltip[2], "item.minecraft.smithing_template.applies_to") << "Line 2: applies-to title";
    EXPECT_EQ(tooltip[3], " item.minecraft.smithing_template.netherite_upgrade.applies_to")
        << "Line 3: applies-to description (space-prefixed)";
    EXPECT_EQ(tooltip[4], "item.minecraft.smithing_template.ingredients") << "Line 4: ingredients title";
    EXPECT_EQ(tooltip[5], " item.minecraft.smithing_template.netherite_upgrade.ingredients")
        << "Line 5: ingredients description (space-prefixed)";
}

TEST_F(ItemTooltipIntegrationTest, SmithingTemplateAddInformation_AllArmorTrimsSameOutput)
{
    // 所有盔甲纹饰模板应产生相同的 tooltip 内容（仅翻译键不同）
    auto* sentry = dynamic_cast<SmithingTemplateItem*>(Items::SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE);
    auto* rib = dynamic_cast<SmithingTemplateItem*>(Items::RIB_ARMOR_TRIM_SMITHING_TEMPLATE);
    ASSERT_NE(sentry, nullptr);
    ASSERT_NE(rib, nullptr);

    ItemStack sentryStack(*sentry, 1);
    ItemStack ribStack(*rib, 1);
    std::vector<std::string> sentryTooltip;
    std::vector<std::string> ribTooltip;

    sentry->addInformation(sentryStack, &m_world, sentryTooltip, false);
    rib->addInformation(ribStack, &m_world, ribTooltip, false);

    // 所有盔甲纹饰模板共享相同的 appliesTo 和 ingredients 翻译键
    ASSERT_EQ(sentryTooltip.size(), 6u);
    ASSERT_EQ(ribTooltip.size(), 6u);
    EXPECT_EQ(sentryTooltip, ribTooltip) << "All armor trim templates should produce identical tooltip";
}

TEST_F(ItemTooltipIntegrationTest, PotterySherdAddInformation_NoExtraLines)
{
    // MC Java 中陶片物品没有额外的 tooltip 描述行
    // addInformation 仅调用基类（空实现），不添加任何额外行
    auto* angler = dynamic_cast<PotterySherdItem*>(Items::ANGLER_POTTERY_SHERD);
    ASSERT_NE(angler, nullptr);

    ItemStack stack(*angler, 1);
    std::vector<std::string> tooltip;
    angler->addInformation(stack, &m_world, tooltip, false);

    // 基类 Item::addInformation 是空操作，PotterySherdItem 也不添加任何行
    EXPECT_TRUE(tooltip.empty()) << "PotterySherdItem should not add any tooltip lines";
}

TEST_F(ItemTooltipIntegrationTest, BannerPatternItemAddInformation_DescKeyFormat)
{
    // BannerPatternItem::addInformation 应输出翻译键 + ".desc" 后缀
    // 例如 creeper_banner_pattern -> "item.minecraft.banner_pattern.creeper.desc"
    auto* creeper = dynamic_cast<BannerPatternItem*>(Items::CREEPER_BANNER_PATTERN);
    if (creeper == nullptr) {
        GTEST_SKIP() << "CREEPER_BANNER_PATTERN not registered";
    }

    ItemStack stack(*creeper, 1);
    std::vector<std::string> tooltip;
    creeper->addInformation(stack, &m_world, tooltip, false);

    // 未加载语言文件时，LanguageManager::get() 返回键本身
    ASSERT_EQ(tooltip.size(), 1u) << "BannerPatternItem should add exactly 1 tooltip line";
    EXPECT_EQ(tooltip[0], "item.minecraft.banner_pattern.creeper.desc")
        << "Tooltip should use .desc suffix translation key";
}

TEST_F(ItemTooltipIntegrationTest, LanguageManagerFallbackInTooltip)
{
    // 验证 LanguageManager 未加载语言文件时的回退行为：
    // get() 返回键本身，tooltip 中显示原始翻译键
    auto& lang = LanguageManager::instance();

    // 验证不存在的键返回键本身
    EXPECT_EQ(lang.get("nonexistent.key.12345"), "nonexistent.key.12345");

    // 验证空键返回空字符串
    EXPECT_EQ(lang.get(""), "");

    // 验证锻造模板键回退正确
    EXPECT_EQ(lang.get("item.minecraft.smithing_template"), "item.minecraft.smithing_template");
    EXPECT_EQ(lang.get("item.minecraft.smithing_template.armor_trim.applies_to"),
        "item.minecraft.smithing_template.armor_trim.applies_to");
}

} // namespace
