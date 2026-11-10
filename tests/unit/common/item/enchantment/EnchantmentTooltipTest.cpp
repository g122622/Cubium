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

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "common/TempDirHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/enchantment/EnchantmentTag.hpp"
#include "common/item/enchantment/EnchantmentTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

using namespace mc;
using namespace mc::item;
using namespace mc::item::enchant;

namespace {

/// 往数据包目录里写一个文本文件（自动建父目录）。
void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
}

/// tooltip_order 标签的声明顺序：仅用于在本测试里验证「按声明顺序」而非字母顺序。
constexpr const char* TOOLTIP_ORDER_JSON = R"({"values": [
    "minecraft:mending",
    "minecraft:sharpness",
    "minecraft:protection",
    "minecraft:fortune"
]})";

/// 被引用的子标签，验证 `#` 引用在声明位置就地展开。
constexpr const char* SUB_TAG_JSON = R"({"values": [
    "minecraft:unbreaking",
    "minecraft:looting"
]})";

/**
 * @brief 构造一个只含附魔标签的数据包目录（真实文件系统，走 DataPackRepository 的完整路径）。
 */
std::filesystem::path createEnchantmentTagPack()
{
    const auto root = mc::test::makeUniqueTestDir("mc_enchantment_tags_test_");
    const auto packDir = root / "tag_pack";
    writeTextFile(packDir / "pack.mcmeta", R"({"pack":{"pack_format":71,"description":"enchantment tags"}})");
    writeTextFile(packDir / "data" / "minecraft" / "tags" / "enchantment" / "tooltip_order.json", TOOLTIP_ORDER_JSON);
    writeTextFile(packDir / "data" / "minecraft" / "tags" / "enchantment" / "sub_tag.json", SUB_TAG_JSON);
    writeTextFile(packDir / "data" / "minecraft" / "tags" / "enchantment" / "complex_tag.json",
        R"({"values": ["minecraft:sharpness", "#minecraft:sub_tag", "minecraft:looting"]})");
    writeTextFile(packDir / "data" / "minecraft" / "tags" / "enchantment" / "unknown_member.json",
        R"({"values": ["minecraft:no_such_enchantment", "minecraft:fortune"]})");
    return root;
}

std::vector<std::string> ids(const std::vector<std::pair<const Enchantment*, i32>>& list)
{
    std::vector<std::string> result;
    result.reserve(list.size());
    for (const auto& [enchantment, level] : list) {
        (void)level;
        result.push_back(enchantment->id());
    }
    return result;
}

} // namespace

/**
 * @brief 附魔提示框排序 / 全名 / 附魔标签顺序的单元测试
 *
 * 对应原版 `ItemEnchantments#addToTooltip`（按 `EnchantmentTags.TOOLTIP_ORDER` 排序）、
 * `Enchantment#getFullname`（颜色 + 罗马数字等级）与 `ItemEnchantments` 在物品提示框里
 * 的附魔行产出。
 */
class EnchantmentTooltipTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        Items::ensureInitialized();
    }

    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
        EnchantmentTags::clearAll();
        EnchantmentTags::initialize();
    }

    void TearDown() override
    {
        EnchantmentTags::clearAll();
        EnchantmentRegistry::clear();
    }

    /// 构造一把带若干附魔的剑（存储顺序与 tooltip_order 的声明顺序刻意不同）。
    [[nodiscard]] static ItemStack makeEnchantedSword()
    {
        ItemStack stack(*Items::DIAMOND_SWORD, 1);
        stack.addEnchantment("minecraft:fortune", 3);
        stack.addEnchantment("minecraft:sharpness", 2);
        stack.addEnchantment("minecraft:mending", 1);
        stack.addEnchantment("minecraft:protection", 4);
        return stack;
    }
};

// ============================================================================
// 附魔标签：声明顺序保留
// ============================================================================

TEST_F(EnchantmentTooltipTest, EnchantmentTagKeepsDeclarationOrder)
{
    EnchantmentTag tag(ResourceLocation("minecraft:test_order"));
    tag.add(ResourceLocation("minecraft:zebra"));
    tag.add(ResourceLocation("minecraft:apple"));
    tag.add(ResourceLocation("minecraft:zebra")); // 重复加入不应改变首次位置

    const auto& ordered = tag.getOrderedEnchantmentIds();
    ASSERT_EQ(ordered.size(), 2u);
    EXPECT_EQ(ordered[0].toString(), "minecraft:zebra");
    EXPECT_EQ(ordered[1].toString(), "minecraft:apple");
    EXPECT_EQ(tag.getEnchantmentIds().size(), 2u);

    tag.clear();
    EXPECT_TRUE(tag.getOrderedEnchantmentIds().empty());
    EXPECT_TRUE(tag.getEnchantmentIds().empty());
}

TEST_F(EnchantmentTooltipTest, TooltipOrderLoadedFromDataPackKeepsOrder)
{
    const auto root = createEnchantmentTagPack();
    resource::DataPackRepository repository;
    (void)repository.addPack(root / "tag_pack", true, 0);

    const auto loadResult = EnchantmentTags::loadFromDataPackRepository(repository);
    ASSERT_TRUE(loadResult.success()) << loadResult.error().toString();

    const auto* tag = EnchantmentTags::getTag(ResourceLocation("minecraft:tooltip_order"));
    ASSERT_NE(tag, nullptr);
    const std::vector<std::string> expected{
        "minecraft:mending", "minecraft:sharpness", "minecraft:protection", "minecraft:fortune"};
    std::vector<std::string> actual;
    for (const auto& id : tag->getOrderedEnchantmentIds()) {
        actual.push_back(id.toString());
    }
    EXPECT_EQ(actual, expected);

    mc::test::removeTestDir(root);
}

TEST_F(EnchantmentTooltipTest, TagReferenceExpandsInPlace)
{
    const auto root = createEnchantmentTagPack();
    resource::DataPackRepository repository;
    (void)repository.addPack(root / "tag_pack", true, 0);

    const auto loadResult = EnchantmentTags::loadFromDataPackRepository(repository);
    ASSERT_TRUE(loadResult.success()) << loadResult.error().toString();

    const auto* tag = EnchantmentTags::getTag(ResourceLocation("minecraft:complex_tag"));
    ASSERT_NE(tag, nullptr);
    // 声明顺序：sharpness, #sub_tag(unbreaking, looting), looting（重复不追加）
    const std::vector<std::string> expected{"minecraft:sharpness", "minecraft:unbreaking", "minecraft:looting"};
    std::vector<std::string> actual;
    for (const auto& id : tag->getOrderedEnchantmentIds()) {
        actual.push_back(id.toString());
    }
    EXPECT_EQ(actual, expected);

    mc::test::removeTestDir(root);
}

TEST_F(EnchantmentTooltipTest, UnknownTagMemberIsSkipped)
{
    const auto root = createEnchantmentTagPack();
    resource::DataPackRepository repository;
    (void)repository.addPack(root / "tag_pack", true, 0);

    const auto loadResult = EnchantmentTags::loadFromDataPackRepository(repository);
    ASSERT_TRUE(loadResult.success()) << loadResult.error().toString();

    const auto* tag = EnchantmentTags::getTag(ResourceLocation("minecraft:unknown_member"));
    ASSERT_NE(tag, nullptr);
    EXPECT_FALSE(tag->contains(ResourceLocation("minecraft:no_such_enchantment")));
    EXPECT_TRUE(tag->contains(ResourceLocation("minecraft:fortune")));

    mc::test::removeTestDir(root);
}

// ============================================================================
// 提示框排序
// ============================================================================

TEST_F(EnchantmentTooltipTest, SortsEnchantmentsByTagDeclarationOrder)
{
    const auto root = createEnchantmentTagPack();
    resource::DataPackRepository repository;
    (void)repository.addPack(root / "tag_pack", true, 0);
    ASSERT_TRUE(EnchantmentTags::loadFromDataPackRepository(repository).success());

    const ItemStack sword = makeEnchantedSword();
    const auto sorted = EnchantmentHelper::getSortedEnchantmentsForTooltip(sword);

    // 物品存储顺序是 fortune, sharpness, mending, protection；排序后必须按标签声明顺序
    EXPECT_EQ(ids(sorted),
        (std::vector<std::string>{
            "minecraft:mending", "minecraft:sharpness", "minecraft:protection", "minecraft:fortune"}));

    // 等级必须随附魔一起搬移，不能只换顺序丢等级
    ASSERT_EQ(sorted.size(), 4u);
    EXPECT_EQ(sorted[0].second, 1); // mending
    EXPECT_EQ(sorted[1].second, 2); // sharpness
    EXPECT_EQ(sorted[2].second, 4); // protection
    EXPECT_EQ(sorted[3].second, 3); // fortune

    mc::test::removeTestDir(root);
}

TEST_F(EnchantmentTooltipTest, EnchantmentsOutsideTagKeepStorageOrderAtEnd)
{
    const auto root = createEnchantmentTagPack();
    resource::DataPackRepository repository;
    (void)repository.addPack(root / "tag_pack", true, 0);
    ASSERT_TRUE(EnchantmentTags::loadFromDataPackRepository(repository).success());

    ItemStack sword(*Items::DIAMOND_SWORD, 1);
    sword.addEnchantment("minecraft:unbreaking", 3); // 不在 tooltip_order 里
    sword.addEnchantment("minecraft:fortune", 3);
    sword.addEnchantment("minecraft:looting", 3); // 不在 tooltip_order 里

    const auto sorted = EnchantmentHelper::getSortedEnchantmentsForTooltip(sword);
    EXPECT_EQ(
        ids(sorted), (std::vector<std::string>{"minecraft:fortune", "minecraft:unbreaking", "minecraft:looting"}));

    mc::test::removeTestDir(root);
}

TEST_F(EnchantmentTooltipTest, FallsBackToStorageOrderWhenTagNotLoaded)
{
    // 没有加载任何数据包：标签为空，排序退化为存储顺序，但不会丢附魔。
    const ItemStack sword = makeEnchantedSword();
    const auto sorted = EnchantmentHelper::getSortedEnchantmentsForTooltip(sword);
    EXPECT_EQ(ids(sorted),
        (std::vector<std::string>{
            "minecraft:fortune", "minecraft:sharpness", "minecraft:mending", "minecraft:protection"}));
}

TEST_F(EnchantmentTooltipTest, EmptyStackHasNoSortedEnchantments)
{
    const ItemStack empty;
    EXPECT_TRUE(EnchantmentHelper::getSortedEnchantmentsForTooltip(empty).empty());
}

// ============================================================================
// 附魔全名
// ============================================================================

TEST_F(EnchantmentTooltipTest, NameKeyUsesDotSeparatedNamespace)
{
    const Enchantment* fortune = EnchantmentRegistry::get("minecraft:fortune");
    ASSERT_NE(fortune, nullptr);
    // 翻译键必须是 enchantment.minecraft.fortune（数据包 description.translate 同款），
    // 不能是带冒号的 enchantment.minecraft:fortune。
    EXPECT_EQ(fortune->getNameKey(), "enchantment.minecraft.fortune");
}

TEST_F(EnchantmentTooltipTest, FullNameAddsRomanLevelWhenNeeded)
{
    const Enchantment* fortune = EnchantmentRegistry::get("minecraft:fortune");
    const Enchantment* silkTouch = EnchantmentRegistry::get("minecraft:silk_touch");
    ASSERT_NE(fortune, nullptr);
    ASSERT_NE(silkTouch, nullptr);

    // 多级附魔恒带等级后缀（与是否 1 级无关）
    EXPECT_NE(EnchantmentHelper::getFullName(*fortune, 1, false).find(' '), std::string::npos);
    EXPECT_NE(EnchantmentHelper::getFullName(*fortune, 3, false).find(' '), std::string::npos);
    // 单级附魔在 1 级时没有后缀
    EXPECT_EQ(EnchantmentHelper::getFullName(*silkTouch, 1, false).find(' '), std::string::npos);
}

TEST_F(EnchantmentTooltipTest, FullNameColorDistinguishesCurse)
{
    const Enchantment* fortune = EnchantmentRegistry::get("minecraft:fortune");
    ASSERT_NE(fortune, nullptr);

    const std::string normal = EnchantmentHelper::getFullName(*fortune, 3, false);
    const std::string cursed = EnchantmentHelper::getFullName(*fortune, 3, true);
    // 颜色码不同（灰 vs 红），且都带 § 前缀
    EXPECT_NE(normal.rfind("\xC2\xA7", 0), std::string::npos);
    EXPECT_NE(cursed.rfind("\xC2\xA7", 0), std::string::npos);
    EXPECT_NE(normal, cursed);
    EXPECT_NE(EnchantmentHelper::getColoredName(*fortune, false), EnchantmentHelper::getColoredName(*fortune, true));
}

// ============================================================================
// 物品提示框集成（Item::addInformation）
// ============================================================================

TEST_F(EnchantmentTooltipTest, ItemTooltipContainsSortedEnchantmentLines)
{
    const auto root = createEnchantmentTagPack();
    resource::DataPackRepository repository;
    (void)repository.addPack(root / "tag_pack", true, 0);
    ASSERT_TRUE(EnchantmentTags::loadFromDataPackRepository(repository).success());

    const ItemStack sword = makeEnchantedSword();
    std::vector<std::string> tooltip;
    sword.getItem()->addInformation(sword, nullptr, tooltip, false);

    // 普通剑的 appendHoverText 默认实现为空，于是提示行恰好全是附魔行，顺序即标签顺序。
    // 文本是经 LanguageManager 查到的翻译（未加载语言文件时退回翻译键），因此这里断言
    // 翻译键而非英文显示名，同时验证「翻译键带点分隔的命名空间」这一关键约束。
    ASSERT_EQ(tooltip.size(), 4u);
    EXPECT_NE(tooltip[0].find("enchantment.minecraft.mending"), std::string::npos) << tooltip[0];
    EXPECT_NE(tooltip[1].find("enchantment.minecraft.sharpness"), std::string::npos) << tooltip[1];
    EXPECT_NE(tooltip[2].find("enchantment.minecraft.protection"), std::string::npos) << tooltip[2];
    EXPECT_NE(tooltip[3].find("enchantment.minecraft.fortune"), std::string::npos) << tooltip[3];
    // 等级后缀存在（罗马数字的翻译键或已本地化的罗马数字）
    EXPECT_NE(tooltip[3].find("enchantment.level.3"), std::string::npos) << tooltip[3];

    mc::test::removeTestDir(root);
}

TEST_F(EnchantmentTooltipTest, ItemTooltipHasNoEnchantmentLineForPlainItem)
{
    Items::ensureInitialized();
    ItemStack sword(*Items::DIAMOND_SWORD, 1);
    std::vector<std::string> tooltip;
    sword.getItem()->addInformation(sword, nullptr, tooltip, false);
    EXPECT_TRUE(tooltip.empty());
}
