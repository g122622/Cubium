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

#include "client/ui/screen/tooltip/BundleTooltipRenderer.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/bundle/BundleContents.hpp"
#include "common/item/items/special/bundle/BundleItem.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::item::items;
using namespace mc::client::ui::screen::tooltip;

// ============================================================================
// BundleTooltipRenderer 布局算法测试
//
// 验证布局计算（gridSizeY、itemGridHeight、slotCount、progressBarFill、
// amountOfHiddenItems、tooltipHeight、positionTooltip）与 MC 1.21.11
// ClientBundleTooltip 的算法一致。
//
// 不依赖 GuiRenderer / ItemRenderer，仅测试纯算法部分。
// ============================================================================

class BundleTooltipRendererTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 → 物品（BundleContents::getWeight 需要 Items::STONE 等）
        VanillaBlocks::initialize();
        Items::initialize();
    }

    /// 获取可用于放入收纳袋的物品列表（避免 BundleContents 自动合并同类堆叠）
    /// 每次调用返回不同物品，确保每次插入都创建独立 ItemStack
    static Item* getDistinctItem(i32 index)
    {
        // 使用 100+ 种不同物品，覆盖所有大数量测试场景
        static Item* items[] = {
            Items::STONE,
            Items::DIRT,
            Items::COBBLESTONE,
            Items::OAK_LOG,
            Items::SAND,
            Items::GRAVEL,
            Items::GLOWSTONE,
            Items::END_STONE,
            Items::RED_SAND,
            Items::COARSE_DIRT,
            Items::SANDSTONE,
            Items::CHISELED_SANDSTONE,
            Items::CUT_SANDSTONE,
            Items::SMOOTH_SANDSTONE,
            Items::RED_SANDSTONE,
            Items::CHISELED_RED_SANDSTONE,
            Items::CUT_RED_SANDSTONE,
            Items::SMOOTH_RED_SANDSTONE,
            Items::SOUL_SAND,
            Items::BLACKSTONE,
            Items::POLISHED_BLACKSTONE,
            Items::GILDED_BLACKSTONE,
            Items::STRIPPED_OAK_LOG,
            Items::STRIPPED_DARK_OAK_LOG,
            Items::BRICK,
            Items::RESIN_BRICK,
            Items::AMETHYST_SHARD,
            Items::RAW_IRON,
            Items::RAW_COPPER,
            Items::RAW_GOLD,
            Items::DIAMOND,
            Items::EMERALD,
            Items::RAW_IRON_BLOCK,
            Items::RAW_COPPER_BLOCK,
            Items::RAW_GOLD_BLOCK,
            Items::BAMBOO_BLOCK,
            Items::STRIPPED_BAMBOO_BLOCK,
            Items::BAMBOO_PLANKS,
            Items::BAMBOO_MOSAIC,
            Items::CRIMSON_PLANKS,
            Items::WARPED_PLANKS,
            Items::MANGROVE_PLANKS,
            Items::CHERRY_PLANKS,
            Items::PALE_OAK_PLANKS,
            Items::HONEYCOMB,
            Items::BELL,
            Items::SUSPICIOUS_SAND,
            Items::SUSPICIOUS_GRAVEL,
            Items::NAME_TAG,
            Items::SADDLE,
            Items::RABBIT_HIDE,
            Items::BLUE_EGG,
            Items::BROWN_EGG,
            Items::SNOWBALL,
            Items::RECOVERY_COMPASS,
            Items::MAP,
            Items::FILLED_MAP,
            Items::PAPER,
            Items::EXPERIENCE_BOTTLE,
            Items::TORCHFLOWER_SEEDS,
            Items::PITCHER_POD,
            Items::HAY_BLOCK,
            Items::CACTUS,
            Items::LILY_PAD,
            Items::VINE,
            Items::BAMBOO,
            Items::CRIMSON_FUNGUS,
            Items::WARPED_FUNGUS,
            Items::TURTLE_SCUTE,
            Items::ARMADILLO_SCUTE,
            Items::HEART_OF_THE_SEA,
            Items::NAUTILUS_SHELL,
            Items::PHANTOM_MEMBRANE,
            Items::DRIED_KELP_BLOCK,
            Items::SEA_PICKLE,
            Items::KELP,
            Items::SEAGRASS,
            Items::NETHER_WART,
            Items::GHAST_TEAR,
            Items::RABBIT_FOOT,
            Items::MAGMA_CREAM,
            Items::DRAGON_BREATH,
            Items::GLISTERING_MELON_SLICE,
            Items::GLASS_BOTTLE,
            Items::POTION,
            Items::SPLASH_POTION,
            Items::LINGERING_POTION,
            Items::BOW,
            Items::ARROW,
            Items::SPECTRAL_ARROW,
            Items::TIPPED_ARROW,
            Items::CROSSBOW,
            Items::TRIDENT,
            Items::SHIELD,
            Items::FISHING_ROD,
            Items::WOODEN_SPEAR,
            Items::STONE_SPEAR,
            Items::COPPER_SPEAR,
            Items::IRON_SPEAR,
            Items::GOLDEN_SPEAR,
            Items::DIAMOND_SPEAR,
            Items::NETHERITE_SPEAR,
            Items::CARROT_ON_A_STICK,
            Items::WARPED_FUNGUS_ON_A_STICK,
            Items::BUCKET,
            Items::BOOK,
            Items::ENCHANTED_BOOK,
            Items::WRITABLE_BOOK,
            Items::WRITTEN_BOOK,
            Items::KNOWLEDGE_BOOK,
            Items::SPONGE,
            Items::WET_SPONGE,
            Items::MINECART,
            Items::CHEST_MINECART,
            Items::FURNACE_MINECART,
            Items::TNT_MINECART,
            Items::HOPPER_MINECART,
            Items::COMMAND_BLOCK_MINECART,
            Items::OAK_BOAT,
            Items::SPRUCE_BOAT,
            Items::BIRCH_BOAT,
            Items::JUNGLE_BOAT,
            Items::ACACIA_BOAT,
            Items::DARK_OAK_BOAT,
            Items::MANGROVE_BOAT,
            Items::CHERRY_BOAT,
            Items::PALE_OAK_BOAT,
            Items::BAMBOO_RAFT,
            Items::OAK_CHEST_BOAT,
            Items::SPRUCE_CHEST_BOAT,
            Items::BIRCH_CHEST_BOAT,
            Items::JUNGLE_CHEST_BOAT,
        };
        constexpr i32 N = static_cast<i32>(sizeof(items) / sizeof(items[0]));
        return items[index % N];
    }

    /// 构造一个包含指定数量物品堆（每堆 1 个）的 BundleContents
    /// 使用不同物品类型，避免 BundleContents 自动合并同类堆叠
    static BundleContents makeBundleWithItemCount(i32 itemCount)
    {
        BundleContents::Mutable mutableContents(BundleContents::EMPTY);
        for (i32 i = 0; i < itemCount; ++i) {
            ItemStack item(*getDistinctItem(i), 1);
            mutableContents.tryInsert(item);
        }
        return mutableContents.toImmutable();
    }

    /// 构造一个包含指定数量物品堆的 BundleContents
    /// 前 itemCount-1 堆每堆 1 个，最后一堆为 lastCount 个（仍使用不同物品类型）
    static BundleContents makeBundleWithItemCountAndLastCount(i32 itemCount, i32 lastCount)
    {
        BundleContents::Mutable mutableContents(BundleContents::EMPTY);
        for (i32 i = 0; i < itemCount - 1; ++i) {
            ItemStack item(*getDistinctItem(i), 1);
            mutableContents.tryInsert(item);
        }
        ItemStack lastItem(*getDistinctItem(itemCount - 1), lastCount);
        mutableContents.tryInsert(lastItem);
        return mutableContents.toImmutable();
    }
};

// ============================================================================
// slotCount 测试（对应 MC ClientBundleTooltip#slotCount：min(12, size)）
// ============================================================================

TEST_F(BundleTooltipRendererTest, SlotCount_EmptyBundle_ReturnsZero)
{
    EXPECT_EQ(BundleTooltipRenderer::slotCount(BundleContents::EMPTY), 0);
}

TEST_F(BundleTooltipRendererTest, SlotCount_SingleItem_ReturnsOne)
{
    EXPECT_EQ(BundleTooltipRenderer::slotCount(makeBundleWithItemCount(1)), 1);
}

TEST_F(BundleTooltipRendererTest, SlotCount_FourItems_ReturnsFour)
{
    EXPECT_EQ(BundleTooltipRenderer::slotCount(makeBundleWithItemCount(4)), 4);
}

TEST_F(BundleTooltipRendererTest, SlotCount_TwelveItems_ReturnsTwelve)
{
    EXPECT_EQ(BundleTooltipRenderer::slotCount(makeBundleWithItemCount(12)), 12);
}

TEST_F(BundleTooltipRendererTest, SlotCount_ThirteenItems_CappedAtTwelve)
{
    // 对应 MC: Math.min(12, contents.size())
    EXPECT_EQ(BundleTooltipRenderer::slotCount(makeBundleWithItemCount(13)), 12);
}

TEST_F(BundleTooltipRendererTest, SlotCount_TwentyItems_CappedAtTwelve)
{
    EXPECT_EQ(BundleTooltipRenderer::slotCount(makeBundleWithItemCount(20)), 12);
}

// ============================================================================
// gridSizeY 测试（对应 MC ClientBundleTooltip#gridSizeY：positiveCeilDiv(slotCount, 4)）
// ============================================================================

TEST_F(BundleTooltipRendererTest, GridSizeY_EmptyBundle_ReturnsZero)
{
    EXPECT_EQ(BundleTooltipRenderer::gridSizeY(BundleContents::EMPTY), 0);
}

TEST_F(BundleTooltipRendererTest, GridSizeY_OneItem_ReturnsOne)
{
    // positiveCeilDiv(1, 4) = 1
    EXPECT_EQ(BundleTooltipRenderer::gridSizeY(makeBundleWithItemCount(1)), 1);
}

TEST_F(BundleTooltipRendererTest, GridSizeY_FourItems_ReturnsOne)
{
    // positiveCeilDiv(4, 4) = 1
    EXPECT_EQ(BundleTooltipRenderer::gridSizeY(makeBundleWithItemCount(4)), 1);
}

TEST_F(BundleTooltipRendererTest, GridSizeY_FiveItems_ReturnsTwo)
{
    // positiveCeilDiv(5, 4) = 2
    EXPECT_EQ(BundleTooltipRenderer::gridSizeY(makeBundleWithItemCount(5)), 2);
}

TEST_F(BundleTooltipRendererTest, GridSizeY_EightItems_ReturnsTwo)
{
    // positiveCeilDiv(8, 4) = 2
    EXPECT_EQ(BundleTooltipRenderer::gridSizeY(makeBundleWithItemCount(8)), 2);
}

TEST_F(BundleTooltipRendererTest, GridSizeY_NineItems_ReturnsThree)
{
    // positiveCeilDiv(9, 4) = 3
    EXPECT_EQ(BundleTooltipRenderer::gridSizeY(makeBundleWithItemCount(9)), 3);
}

TEST_F(BundleTooltipRendererTest, GridSizeY_TwelveItems_ReturnsThree)
{
    // positiveCeilDiv(12, 4) = 3
    EXPECT_EQ(BundleTooltipRenderer::gridSizeY(makeBundleWithItemCount(12)), 3);
}

TEST_F(BundleTooltipRendererTest, GridSizeY_ThirteenItems_CappedAtThree)
{
    // slotCount(13) = 12, positiveCeilDiv(12, 4) = 3
    EXPECT_EQ(BundleTooltipRenderer::gridSizeY(makeBundleWithItemCount(13)), 3);
}

// ============================================================================
// itemGridHeight 测试（对应 MC ClientBundleTooltip#itemGridHeight：gridSizeY * 24）
// ============================================================================

TEST_F(BundleTooltipRendererTest, ItemGridHeight_EmptyBundle_ReturnsZero)
{
    EXPECT_EQ(BundleTooltipRenderer::itemGridHeight(BundleContents::EMPTY), 0);
}

TEST_F(BundleTooltipRendererTest, ItemGridHeight_OneItem_Returns24)
{
    EXPECT_EQ(BundleTooltipRenderer::itemGridHeight(makeBundleWithItemCount(1)), 24);
}

TEST_F(BundleTooltipRendererTest, ItemGridHeight_FourItems_Returns24)
{
    EXPECT_EQ(BundleTooltipRenderer::itemGridHeight(makeBundleWithItemCount(4)), 24);
}

TEST_F(BundleTooltipRendererTest, ItemGridHeight_FiveItems_Returns48)
{
    EXPECT_EQ(BundleTooltipRenderer::itemGridHeight(makeBundleWithItemCount(5)), 48);
}

TEST_F(BundleTooltipRendererTest, ItemGridHeight_TwelveItems_Returns72)
{
    EXPECT_EQ(BundleTooltipRenderer::itemGridHeight(makeBundleWithItemCount(12)), 72);
}

// ============================================================================
// progressBarFill 测试（对应 MC getProgressBarFill：clamp(weight * 94 / 64, 0, 94)）
// ============================================================================

TEST_F(BundleTooltipRendererTest, ProgressBarFill_EmptyBundle_ReturnsZero)
{
    EXPECT_EQ(BundleTooltipRenderer::progressBarFill(BundleContents::EMPTY), 0);
}

TEST_F(BundleTooltipRendererTest, ProgressBarFill_QuarterFull_Returns23Or24)
{
    // 16 个石头，权重 16，填充 = 16 * 94 / 64 = 23.5 -> 23
    const auto contents = makeBundleWithItemCount(16);
    EXPECT_EQ(BundleTooltipRenderer::progressBarFill(contents), 23);
}

TEST_F(BundleTooltipRendererTest, ProgressBarFill_HalfFull_Returns47)
{
    // 32 个石头，权重 32，填充 = 32 * 94 / 64 = 47
    const auto contents = makeBundleWithItemCount(32);
    EXPECT_EQ(BundleTooltipRenderer::progressBarFill(contents), 47);
}

TEST_F(BundleTooltipRendererTest, ProgressBarFill_FullBundle_Returns94)
{
    // 64 个石头，权重 64，填充 = 64 * 94 / 64 = 94
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack stone(*Items::STONE, 64);
    mutableContents.tryInsert(stone);
    const auto contents = mutableContents.toImmutable();

    EXPECT_EQ(BundleTooltipRenderer::progressBarFill(contents), 94);
}

TEST_F(BundleTooltipRendererTest, ProgressBarFill_OverFull_ClampedTo94)
{
    // 收纳袋嵌套：内袋权重 + 4，总权重可能超过 64
    // 但 BundleContents::tryInsert 会拒绝超限插入，所以无法构造 > 64 的情况
    // 此测试验证 clamp 上界：用一个 64 权重的物品（如剑，maxStackSize=1, weight=64）
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack sword(*Items::DIAMOND_SWORD, 1); // 权重 64
    mutableContents.tryInsert(sword);
    const auto contents = mutableContents.toImmutable();

    EXPECT_EQ(BundleTooltipRenderer::progressBarFill(contents), 94);
}

// ============================================================================
// amountOfHiddenItems 测试（对应 MC getAmountOfHiddenItems）
// ============================================================================

TEST_F(BundleTooltipRendererTest, AmountOfHiddenItems_EmptyBundle_ReturnsZero)
{
    EXPECT_EQ(BundleTooltipRenderer::amountOfHiddenItems(BundleContents::EMPTY), 0);
}

TEST_F(BundleTooltipRendererTest, AmountOfHiddenItems_TwelveItems_ReturnsZero)
{
    // size = 12, numberOfItemsToShow = 12, no hidden
    const auto contents = makeBundleWithItemCount(12);
    EXPECT_EQ(BundleTooltipRenderer::amountOfHiddenItems(contents), 0);
}

TEST_F(BundleTooltipRendererTest, AmountOfHiddenItems_ThirteenItems_ReturnsLastItemCount)
{
    // size = 13, numberOfItemsToShow = 8 (j=11, k=1, l=3, min(13, 11-3) = 8)
    // 隐藏 items[8..12]，共 5 个物品（每堆 1 个）
    const auto contents = makeBundleWithItemCount(13);
    EXPECT_EQ(BundleTooltipRenderer::amountOfHiddenItems(contents), 5);
}

TEST_F(BundleTooltipRendererTest, AmountOfHiddenItems_ThirteenItemsWithLastCountSumsCounts)
{
    // size = 13, numberOfItemsToShow = 8
    // BundleContents 将最新插入的物品放在 items[0]，所以最后插入的 lastCount 物品位于 items[0]
    // 隐藏 items[8..12] = 5 个堆，全部为 count=1 的物品（最先插入的 5 个）
    // 注意：lastCount 物品位于 items[0]，在 shownItemsCount=8 内，不会被隐藏
    // 因此隐藏物品数量 = 5 × 1 = 5
    const auto contents = makeBundleWithItemCountAndLastCount(13, 7);
    EXPECT_EQ(BundleTooltipRenderer::amountOfHiddenItems(contents), 5);
}

// ============================================================================
// tooltipHeight 测试（对应 MC ClientBundleTooltip#getHeight）
// ============================================================================

TEST_F(BundleTooltipRendererTest, TooltipHeight_EmptyBundle_ReturnsEmptyDescPlus21)
{
    // 空：emptyDescHeight + 13 + 8 = emptyDescHeight + 21
    EXPECT_EQ(BundleTooltipRenderer::tooltipHeight(BundleContents::EMPTY, 9), 9 + 13 + 8);
    EXPECT_EQ(BundleTooltipRenderer::tooltipHeight(BundleContents::EMPTY, 0), 0 + 13 + 8);
}

TEST_F(BundleTooltipRendererTest, TooltipHeight_NonEmptyBundle_ReturnsGridHeightPlus21)
{
    // 非空：itemGridHeight + 13 + 8 = gridHeight + 21
    EXPECT_EQ(BundleTooltipRenderer::tooltipHeight(makeBundleWithItemCount(1)), 24 + 13 + 8);
    EXPECT_EQ(BundleTooltipRenderer::tooltipHeight(makeBundleWithItemCount(4)), 24 + 13 + 8);
    EXPECT_EQ(BundleTooltipRenderer::tooltipHeight(makeBundleWithItemCount(5)), 48 + 13 + 8);
    EXPECT_EQ(BundleTooltipRenderer::tooltipHeight(makeBundleWithItemCount(12)), 72 + 13 + 8);
}

// ============================================================================
// positionTooltip 测试
// ============================================================================

TEST_F(BundleTooltipRendererTest, PositionTooltip_DefaultPlacesRightBelowMouse)
{
    // 鼠标 (100, 100)，宽 96，高 45，屏幕 1920×1080
    // 默认：x = 100 + 12 = 112, y = 100 + 12 = 112
    const auto [x, y] = BundleTooltipRenderer::positionTooltip(100, 100, 96, 45, 1920, 1080);
    EXPECT_EQ(x, 112);
    EXPECT_EQ(y, 112);
}

TEST_F(BundleTooltipRendererTest, PositionTooltip_ExceedsRightFlipsToLeft)
{
    // 鼠标 (1900, 100)，宽 96，高 45，屏幕 1920×1080
    // 默认 x = 1912，超出右边界 1912 + 96 = 2008 > 1920
    // 翻转：x = 1900 - 12 - 96 = 1792
    const auto [x, y] = BundleTooltipRenderer::positionTooltip(1900, 100, 96, 45, 1920, 1080);
    EXPECT_EQ(x, 1792);
}

TEST_F(BundleTooltipRendererTest, PositionTooltip_ExceedsBottomFlipsToTop)
{
    // 鼠标 (100, 1050)，宽 96，高 45，屏幕 1920×1080
    // 默认 y = 1062，超出下边界 1062 + 45 = 1107 > 1080
    // 翻转：y = 1050 - 12 - 45 = 993
    const auto [x, y] = BundleTooltipRenderer::positionTooltip(100, 1050, 96, 45, 1920, 1080);
    EXPECT_EQ(x, 112);
    EXPECT_EQ(y, 993);
}

TEST_F(BundleTooltipRendererTest, PositionTooltip_NearOriginDoesNotFlipOrClamp)
{
    // 鼠标 (0, 0)，宽 96，高 45，屏幕 1920×1080
    // 默认 x = 12, y = 12，未超出右/下边界，无需翻转
    // max(4, 12) = 12，无需钳制
    const auto [x, y] = BundleTooltipRenderer::positionTooltip(0, 0, 96, 45, 1920, 1080);
    EXPECT_EQ(x, 12);
    EXPECT_EQ(y, 12);
}

TEST_F(BundleTooltipRendererTest, PositionTooltip_NegativeFlipClampsToMinPosition)
{
    // 鼠标 (0, 0)，宽 96，高 45，屏幕 100×100（极小屏幕）
    // 默认 x = 12, y = 12
    // 水平：12+96=108 > 100，翻转为 x = 0 - 12 - 96 = -108，钳制到 4
    // 垂直：12+45=57 ≤ 100，不翻转，y = max(4, 12) = 12
    const auto [x, y] = BundleTooltipRenderer::positionTooltip(0, 0, 96, 45, 100, 100);
    EXPECT_EQ(x, 4);
    EXPECT_EQ(y, 12);
}

TEST_F(BundleTooltipRendererTest, PositionTooltip_BothAxesFlipClampsToMinPosition)
{
    // 鼠标 (0, 0)，宽 96，高 96，屏幕 100×100（极小屏幕）
    // 默认 x = 12, y = 12
    // 水平：12+96=108 > 100，翻转 x = -108，钳制到 4
    // 垂直：12+96=108 > 100，翻转 y = -108，钳制到 4
    const auto [x, y] = BundleTooltipRenderer::positionTooltip(0, 0, 96, 96, 100, 100);
    EXPECT_EQ(x, 4);
    EXPECT_EQ(y, 4);
}

// ============================================================================
// 布局常量测试（与 MC 1.21.11 ClientBundleTooltip 一致）
// ============================================================================

TEST_F(BundleTooltipRendererTest, Constants_MatchMC1111Values)
{
    // 与 MC 1.21.11 ClientBundleTooltip 静态常量一致
    EXPECT_EQ(BundleTooltipRenderer::SLOT_SIZE, 24);
    EXPECT_EQ(BundleTooltipRenderer::GRID_COLUMNS, 4);
    EXPECT_EQ(BundleTooltipRenderer::GRID_WIDTH, 96);
    EXPECT_EQ(BundleTooltipRenderer::PROGRESSBAR_HEIGHT, 13);
    EXPECT_EQ(BundleTooltipRenderer::PROGRESSBAR_WIDTH, 96);
    EXPECT_EQ(BundleTooltipRenderer::PROGRESSBAR_FILL_MAX, 94);
    EXPECT_EQ(BundleTooltipRenderer::MAX_VISIBLE_SLOTS, 12);
    EXPECT_EQ(BundleTooltipRenderer::TOOLTIP_WIDTH, 104);  // 96 + 4*2
    EXPECT_EQ(BundleTooltipRenderer::SLOT_ICON_OFFSET, 4); // (24-16)/2
}

// ============================================================================
// 集成测试：BundleItem::isBundleItem + BundleTooltipRenderer 布局
// ============================================================================

TEST_F(BundleTooltipRendererTest, Integration_BundleItemIsBundleItemDetectsAllVariants)
{
    // 验证所有 17 个收纳袋变体都被 isBundleItem 识别
    // 这保证 renderItemTooltip 会正确委托给 BundleTooltipRenderer
    std::vector<Item*> bundles = {
        Items::BUNDLE,
        Items::WHITE_BUNDLE,
        Items::ORANGE_BUNDLE,
        Items::MAGENTA_BUNDLE,
        Items::LIGHT_BLUE_BUNDLE,
        Items::YELLOW_BUNDLE,
        Items::LIME_BUNDLE,
        Items::PINK_BUNDLE,
        Items::GRAY_BUNDLE,
        Items::LIGHT_GRAY_BUNDLE,
        Items::CYAN_BUNDLE,
        Items::PURPLE_BUNDLE,
        Items::BLUE_BUNDLE,
        Items::BROWN_BUNDLE,
        Items::GREEN_BUNDLE,
        Items::RED_BUNDLE,
        Items::BLACK_BUNDLE,
    };

    for (auto* item : bundles) {
        ASSERT_NE(item, nullptr);
        ItemStack stack(*item, 1);
        EXPECT_TRUE(BundleItem::isBundleItem(stack))
            << "BundleItem::isBundleItem should return true for " << item->itemLocation().toString();
    }
}

TEST_F(BundleTooltipRendererTest, Integration_NonBundleItemNotDetected)
{
    // 验证非收纳袋物品不被识别
    ItemStack stone(*Items::STONE, 1);
    EXPECT_FALSE(BundleItem::isBundleItem(stone));

    ItemStack sword(*Items::DIAMOND_SWORD, 1);
    EXPECT_FALSE(BundleItem::isBundleItem(sword));
}

TEST_F(BundleTooltipRendererTest, Integration_EmptyStackNotDetected)
{
    ItemStack empty;
    EXPECT_FALSE(BundleItem::isBundleItem(empty));
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(BundleTooltipRendererTest, SlotCount_LargeBundleStillCappedAt12)
{
    // 验证即使有 100 个物品，slotCount 仍为 12
    EXPECT_EQ(BundleTooltipRenderer::slotCount(makeBundleWithItemCount(100)), 12);
}

TEST_F(BundleTooltipRendererTest, GridSizeY_LargeBundleStillCappedAt3)
{
    // 验证即使有 100 个物品，gridSizeY 仍为 3（3 行 × 4 列 = 12 格）
    EXPECT_EQ(BundleTooltipRenderer::gridSizeY(makeBundleWithItemCount(100)), 3);
}

TEST_F(BundleTooltipRendererTest, ItemGridHeight_LargeBundleCappedAt72)
{
    // 3 行 × 24 = 72
    EXPECT_EQ(BundleTooltipRenderer::itemGridHeight(makeBundleWithItemCount(100)), 72);
}
