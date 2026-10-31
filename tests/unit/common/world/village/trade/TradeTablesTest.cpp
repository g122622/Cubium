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

#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/items/armor/DyeableArmorItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/items/special/EnchantedBookItem.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/village/trade/ItemCost.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"
#include "common/world/village/trade/TradeContext.hpp"
#include "common/world/village/trade/VillagerTrades.hpp"
#include "common/world/village/trade/WanderingTraderTrades.hpp"
#include <algorithm>
#include <cstdio>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace world::village::trade;
using entity::VillagerProfession;
using entity::VillagerType;

/**
 * @brief 交易表测试夹具
 *
 * 交易条目在构建时解析物品与附魔注册表，因此须先完成对应初始化；每个用例都
 * 用离线上下文（无世界、无村民实体）生成交易，验证条目数据与生成逻辑。
 */
class TradeTablesTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 方块类物品（玻璃、带釉陶瓦、滴水石等）由 BlockItemRegistry 在方块注册之后统一登记
        VanillaBlocks::initialize();
        Items::ensureInitialized();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        item::tag::ItemTags::initialize();
        item::enchant::EnchantmentRegistry::initialize();
        VillagerTrades::initialize();
        WanderingTraderTrades::initialize();
    }

    void TearDown() override
    {
        item::enchant::EnchantmentRegistry::clear();
    }

    /**
     * @brief 构造离线交易上下文
     * @param seed 种子
     * @param sequence 生成序号
     */
    static TradeContext offlineContext(u64 seed, std::size_t sequence = 1)
    {
        TradeContext ctx;
        ctx.traderSeed = seed;
        ctx.sequence = sequence;
        // 附魔池来自数据包标签：这些用例断言的是附魔书 / 附魔装备的内容，必须给出解析器，
        // 否则候选池为空、条目退化成"卖普通书"（生产环境由服务端启动时注入同一份标签）。
        // 这里用注册顺序近似 `#minecraft:tradeable` 与 `#minecraft:on_traded_equipment`。
        ctx.enchantmentTags = [](const ResourceLocation& tag) {
            const std::string name = tag.toString();
            std::vector<std::string> members;
            if (name == "minecraft:tradeable" || name == "minecraft:on_traded_equipment") {
                for (const auto* enchantment : item::enchant::EnchantmentRegistry::allInRegistrationOrder()) {
                    if (enchantment != nullptr) {
                        members.push_back(enchantment->id());
                    }
                }
            }
            return members;
        };
        return ctx;
    }

    /**
     * @brief 把同一等级在多个种子下生成的交易拼在一起
     *
     * 原版每次升级只从该等级**抽**两条交易（`addOffersFromItemListings(..., 2)`），所以想覆盖
     * 一个等级的全部条目，只能换种子多抽几轮。这里按种子顺序拼接，作为后续断言的语料。
     */
    static MerchantOffers collectLevelEntries(VillagerProfession profession, i32 level, u64 firstSeed, u64 seedCount)
    {
        MerchantOffers collected;
        for (u64 seed = firstSeed; seed < firstSeed + seedCount; ++seed) {
            auto offers = VillagerTrades::generateOffers(profession, level, offlineContext(seed));
            for (std::size_t i = 0; i < offers->size(); ++i) {
                const MerchantOffer* offer = offers->getOffer(i);
                if (offer != nullptr) {
                    collected.addOffer(std::make_unique<MerchantOffer>(*offer));
                }
            }
        }
        return collected;
    }

    /**
     * @brief 统计某物品在某等级交易中的出现次数
     */
    static i32 countSellItem(const MerchantOffers& offers, const Item* item)
    {
        i32 count = 0;
        for (std::size_t i = 0; i < offers.size(); ++i) {
            const MerchantOffer* offer = offers.getOffer(i);
            if (offer != nullptr && offer->getSell().getItem() == item) {
                ++count;
            }
        }
        return count;
    }
};

// ============================================================================
// ItemCost
// ============================================================================

TEST_F(TradeTablesTest, ItemCostMatchesItemAndCount)
{
    const Item* emerald = ItemRegistry::instance().getItem(ResourceLocation("minecraft:emerald"));
    ASSERT_NE(emerald, nullptr);

    ItemCost cost(ItemStack(*emerald, 5), 5);
    EXPECT_EQ(cost.count(), 5);
    EXPECT_TRUE(cost.test(ItemStack(*emerald, 5)));
    EXPECT_FALSE(cost.test(ItemStack(*emerald, 4)));
    EXPECT_FALSE(cost.test(ItemStack()));
}

TEST_F(TradeTablesTest, ItemCostPotionPredicate)
{
    ItemCost cost(ItemStack(*Items::POTION, 1), 1);
    cost.withPotion("minecraft:water");

    ItemStack waterPotion(*Items::POTION, 1);
    potion::PotionUtils::setPotion(waterPotion, potion::Potions::WATER);
    EXPECT_TRUE(cost.test(waterPotion));

    ItemStack nightVisionPotion(*Items::POTION, 1);
    potion::PotionUtils::setPotion(nightVisionPotion, potion::Potions::LONG_NIGHT_VISION);
    EXPECT_FALSE(cost.test(nightVisionPotion));
}

// ============================================================================
// 村民交易表
// ============================================================================

TEST_F(TradeTablesTest, InitializationRegistersAllProfessions)
{
    for (auto profession : {VillagerProfession::Farmer,
             VillagerProfession::Fisherman,
             VillagerProfession::Shepherd,
             VillagerProfession::Fletcher,
             VillagerProfession::Librarian,
             VillagerProfession::Cartographer,
             VillagerProfession::Cleric,
             VillagerProfession::Armorer,
             VillagerProfession::Weaponsmith,
             VillagerProfession::Toolsmith,
             VillagerProfession::Butcher,
             VillagerProfession::Leatherworker,
             VillagerProfession::Mason}) {
        EXPECT_TRUE(VillagerTrades::hasTrades(profession));
        EXPECT_EQ(VillagerTrades::getTradeLevelCount(profession), 5);
    }
}

TEST_F(TradeTablesTest, FarmerLevelOneMatchesVanillaEntries)
{
    const auto offers = collectLevelEntries(VillagerProfession::Farmer, 1, 1, 12);

    // 原版农民 1 级：小麦 / 马铃薯 / 胡萝卜 / 甜菜根各换绿宝石，外加"6 面包换 1 绿宝石"。
    i32 cropCount = 0;
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const MerchantOffer* offer = offers.getOffer(i);
        const Item* cost = offer->getCostA().templateStack().getItem();
        if (cost == Items::WHEAT || cost == Items::POTATO || cost == Items::CARROT || cost == Items::BEETROOT) {
            ++cropCount;
            EXPECT_EQ(offer->getSell().getItem(), Items::EMERALD);
            EXPECT_EQ(offer->getSell().getCount(), 1);
        }
    }
    EXPECT_GT(cropCount, 0);
    EXPECT_GT(countSellItem(offers, Items::BREAD), 0);

    // 小麦交易：20 个小麦换 1 个绿宝石，供应 16 次，经验 2
    const MerchantOffer* wheatOffer = nullptr;
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const MerchantOffer* offer = offers.getOffer(i);
        if (offer->getCostA().templateStack().getItem() == Items::WHEAT) {
            wheatOffer = offer;
        }
    }
    ASSERT_NE(wheatOffer, nullptr);
    EXPECT_EQ(wheatOffer->getCostA().count(), 20);
    EXPECT_EQ(wheatOffer->getMaxUses(), 16);
    EXPECT_EQ(wheatOffer->getXp(), 2);
    EXPECT_EQ(wheatOffer->getSell().getItem(), Items::EMERALD);
    EXPECT_EQ(wheatOffer->getSell().getCount(), 1);
}

TEST_F(TradeTablesTest, FishermanRepairsTradesUseTwoCosts)
{
    const auto offers = collectLevelEntries(VillagerProfession::Fisherman, 1, 1, 12);

    const MerchantOffer* cookedCodOffer = nullptr;
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const MerchantOffer* offer = offers.getOffer(i);
        if (offer->getSell().getItem() == Items::COOKED_COD) {
            cookedCodOffer = offer;
        }
    }
    ASSERT_NE(cookedCodOffer, nullptr);
    ASSERT_TRUE(cookedCodOffer->getCostB().has_value());
    EXPECT_EQ(cookedCodOffer->getCostA().count(), 1);
    EXPECT_EQ(cookedCodOffer->getCostB()->templateStack().getItem(), Items::COD);
    EXPECT_EQ(cookedCodOffer->getCostB()->count(), 6);
    EXPECT_EQ(cookedCodOffer->getSell().getCount(), 6);
}

TEST_F(TradeTablesTest, LibrarianSellsEnchantedBooks)
{
    const auto offers = collectLevelEntries(VillagerProfession::Librarian, 1, 1, 12);

    const MerchantOffer* bookOffer = nullptr;
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const MerchantOffer* offer = offers.getOffer(i);
        if (offer->getSell().getItem() == Items::ENCHANTED_BOOK) {
            bookOffer = offer;
        }
    }
    ASSERT_NE(bookOffer, nullptr);
    // 附魔书交易需要书作为第二支付
    ASSERT_TRUE(bookOffer->getCostB().has_value());
    EXPECT_EQ(bookOffer->getCostB()->templateStack().getItem(), Items::BOOK);
    EXPECT_EQ(bookOffer->getMaxUses(), 12);
    EXPECT_EQ(bookOffer->getXp(), 1);
    EXPECT_TRUE(item::items::EnchantedBookItem::hasEnchantments(bookOffer->getSell()));
    EXPECT_GE(bookOffer->getCostA().count(), 1);
    EXPECT_LE(bookOffer->getCostA().count(), 64);
}

TEST_F(TradeTablesTest, EnchantedItemTradesScalePriceWithLevel)
{
    // 盔甲匠 4 级：两条附魔钻石甲交易（每轮换种子重抽）
    for (u64 seed = 1; seed <= 8; ++seed) {
        auto offers = VillagerTrades::generateOffers(VillagerProfession::Armorer, 4, offlineContext(seed));
        ASSERT_EQ(offers->size(), 2u);
        for (std::size_t i = 0; i < offers->size(); ++i) {
            const MerchantOffer* offer = offers->getOffer(i);
            const ItemStack& sell = offer->getSell();
            EXPECT_TRUE(sell.hasEnchantments());
            // 价格 = 基础价格 + 附魔等级（5~19），且不超过 64
            const i32 base = (sell.getItem() == Items::DIAMOND_LEGGINGS) ? 14 : 8;
            EXPECT_GE(offer->getCostA().count(), base + 5);
            EXPECT_LE(offer->getCostA().count(), std::min(base + 19, 64));
            EXPECT_FLOAT_EQ(offer->getPriceMultiplier(), 0.2f);
        }
    }
}

TEST_F(TradeTablesTest, LeatherworkerTradesAreDyed)
{
    const auto offers = collectLevelEntries(VillagerProfession::Leatherworker, 1, 1, 12);

    i32 dyedCount = 0;
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const MerchantOffer* offer = offers.getOffer(i);
        if (offer->getSell().getItem() == Items::LEATHER_LEGGINGS ||
            offer->getSell().getItem() == Items::LEATHER_CHESTPLATE) {
            ++dyedCount;
            EXPECT_TRUE(item::items::DyeableArmorItem::hasColor(offer->getSell()));
        }
    }
    EXPECT_GT(dyedCount, 0);
}

TEST_F(TradeTablesTest, FarmerStewTradesCarryEffects)
{
    const auto offers = collectLevelEntries(VillagerProfession::Farmer, 4, 1, 24);

    i32 stewCount = 0;
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const MerchantOffer* offer = offers.getOffer(i);
        if (offer->getSell().getItem() == Items::SUSPICIOUS_STEW) {
            ++stewCount;
            const auto* effects = offer->getSell().getStewEffects();
            ASSERT_NE(effects, nullptr);
            ASSERT_EQ(effects->size(), 1u);
            EXPECT_GT((*effects)[0].duration(), 0);
        }
    }
    EXPECT_GT(stewCount, 0);
}

TEST_F(TradeTablesTest, GenerationIsDeterministicForSameSeed)
{
    auto first = VillagerTrades::generateOffers(VillagerProfession::Librarian, 3, offlineContext(2024, 3));
    auto second = VillagerTrades::generateOffers(VillagerProfession::Librarian, 3, offlineContext(2024, 3));
    ASSERT_EQ(first->size(), second->size());
    for (std::size_t i = 0; i < first->size(); ++i) {
        const MerchantOffer* a = first->getOffer(i);
        const MerchantOffer* b = second->getOffer(i);
        EXPECT_EQ(a->getCostA().count(), b->getCostA().count());
        EXPECT_EQ(a->getSell().getItem(), b->getSell().getItem());
        EXPECT_EQ(a->getSell().getCount(), b->getSell().getCount());
    }
}

TEST_F(TradeTablesTest, NitwitAndNoneHaveNoTrades)
{
    auto nitwit = VillagerTrades::generateOffers(VillagerProfession::Nitwit, 1, offlineContext(1));
    EXPECT_TRUE(nitwit->empty());
    auto none = VillagerTrades::generateOffers(VillagerProfession::None, 1, offlineContext(1));
    EXPECT_TRUE(none->empty());
    EXPECT_FALSE(VillagerTrades::hasTrades(VillagerProfession::Nitwit));
}

TEST_F(TradeTablesTest, TypeSpecificTradeReturnsNothingWithoutVillager)
{
    // 渔夫 5 级的船只交易按村民类型分支；离线上下文没有村民实体，条目应被跳过
    auto offers = VillagerTrades::generateOffers(VillagerProfession::Fisherman, 5, offlineContext(5));
    ASSERT_EQ(offers->size(), 1u); // 仅河豚收购；船只条目在没有村民类型时返回空交易
    EXPECT_EQ(countSellItem(*offers, Items::OAK_BOAT), 0);
}

// ============================================================================
// 流浪商人交易表
// ============================================================================

TEST_F(TradeTablesTest, WanderingTraderHasTwoPools)
{
    EXPECT_GT(WanderingTraderTrades::getPoolSize(0), 0u);
    EXPECT_GT(WanderingTraderTrades::getPoolSize(1), 0u);
    EXPECT_EQ(WanderingTraderTrades::getPoolSize(2), 0u);
}

TEST_F(TradeTablesTest, WanderingTraderGeneratesFourOffers)
{
    // 两个池各抽取 2 次，共 4 条交易
    auto offers = WanderingTraderTrades::generateOffers(offlineContext(1234));
    EXPECT_EQ(offers->size(), 4u);
    for (std::size_t i = 0; i < offers->size(); ++i) {
        const MerchantOffer* offer = offers->getOffer(i);
        EXPECT_EQ(offer->getMaxUses() >= 1, true);
        EXPECT_FALSE(offer->getSell().isEmpty());
    }
}

TEST_F(TradeTablesTest, WanderingTraderIsDeterministicForSameSeed)
{
    auto first = WanderingTraderTrades::generateOffers(offlineContext(777));
    auto second = WanderingTraderTrades::generateOffers(offlineContext(777));
    ASSERT_EQ(first->size(), second->size());
    for (std::size_t i = 0; i < first->size(); ++i) {
        EXPECT_EQ(first->getOffer(i)->getSell().getItem(), second->getOffer(i)->getSell().getItem());
        EXPECT_EQ(first->getOffer(i)->getSell().getCount(), second->getOffer(i)->getSell().getCount());
    }
}

} // namespace
} // namespace mc
