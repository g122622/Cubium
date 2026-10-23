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

#include "common/world/village/trade/WanderingTraderTrades.hpp"

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/potion/Potion.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"
#include "common/world/village/trade/TradeRandom.hpp"

#include <algorithm>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace village {
namespace trade {

namespace {

constexpr i32 DEFAULT_SUPPLY = 12;

/// 第一个池：常见商品的收购/出售
constexpr i32 FIRST_POOL_SELECTIONS = 2;
/// 第二个池：稀有商品
constexpr i32 SECOND_POOL_SELECTIONS = 2;

/**
 * @brief 以资源位置构造物品堆（流浪商人部分商品没有 Items 静态指针）
 * @param itemId 物品资源位置
 * @param count 数量
 * @return 物品堆
 */
ItemStack _stack(const char* itemId, i32 count)
{
    return makeStack(itemId, count);
}

/**
 * @brief 以资源位置构造绿宝石支付代价
 * @param count 数量
 * @return 支付代价
 */
ItemCost _emeralds(i32 count)
{
    return ItemCost(_stack("minecraft:emerald", count), count);
}

/**
 * @brief 出售指定物品的条目
 * @param itemId 物品资源位置
 * @param count 数量
 * @param emeraldCost 绿宝石价格
 * @param maxUses 最大使用次数
 * @param xp 经验
 * @return 条目
 */
std::unique_ptr<TradeListing> _sell(const char* itemId, i32 count, i32 emeraldCost, i32 maxUses, i32 xp)
{
    return makeItemsForEmeralds(_emeralds(emeraldCost), _stack(itemId, count), maxUses, xp, 0.05f);
}

/**
 * @brief 收购指定物品的条目（玩家 → 商人，换绿宝石）
 * @param itemId 物品资源位置
 * @param count 数量
 * @param emeraldCount 获得的绿宝石数量
 * @param maxUses 最大使用次数
 * @param xp 经验
 * @return 条目
 */
std::unique_ptr<TradeListing> _buy(const char* itemId, i32 count, i32 emeraldCount, i32 maxUses, i32 xp)
{
    return makeItemsForEmeralds(
        ItemCost(_stack(itemId, count), count), _stack("minecraft:emerald", emeraldCount), maxUses, xp, 0.05f);
}

} // namespace

// ============================================================================
// 静态成员
// ============================================================================

std::array<WanderingTraderTrades::Pool, WanderingTraderTrades::TRADE_POOL_COUNT> WanderingTraderTrades::s_pools;
bool WanderingTraderTrades::s_initialized = false;

// ============================================================================
// 初始化与生成
// ============================================================================

void WanderingTraderTrades::initialize()
{
    if (s_initialized) {
        return;
    }

    s_pools[0].selections = FIRST_POOL_SELECTIONS;
    s_pools[1].selections = SECOND_POOL_SELECTIONS;

    _registerFirstPool();
    _registerSecondPool();

    s_initialized = true;
}

size_t WanderingTraderTrades::getPoolSize(size_t pool)
{
    if (!s_initialized) {
        initialize();
    }
    if (pool >= TRADE_POOL_COUNT) {
        return 0;
    }
    return s_pools[pool].listings.size();
}

std::unique_ptr<MerchantOffers> WanderingTraderTrades::generateOffers(const TradeContext& ctx)
{
    auto offers = std::make_unique<MerchantOffers>();

    if (!s_initialized) {
        initialize();
    }

    // 每个池独立抽取：原版对每个池的条目列表随机取 selections 条，允许重复
    u64 sequence = 0;
    for (const auto& pool : s_pools) {
        if (pool.listings.empty() || pool.selections <= 0) {
            continue;
        }

        for (i32 i = 0; i < pool.selections; ++i) {
            TradeContext poolCtx = ctx;
            poolCtx.sequence = sequence++;
            TradeRandom offlineRandom(ctx.traderSeed + poolCtx.sequence * 0x9E3779B97F4A7C15ULL);

            // 抽取下标与生成条目使用同一条随机流（与原版先抽条目再生成一致）
            TradeRandomSource& random = (ctx.world != nullptr) ? static_cast<TradeRandomSource&>(ctx.world->getRandom())
                                                               : static_cast<TradeRandomSource&>(offlineRandom);
            const size_t index = static_cast<size_t>(random.nextInt(static_cast<i32>(pool.listings.size())));
            auto offer = pool.listings[index]->getOffer(poolCtx, random);
            if (offer != nullptr) {
                offers->addOffer(std::move(offer));
            }
        }
    }

    return offers;
}

// ============================================================================
// 第一个池：收购 + 常见商品
// ============================================================================

void WanderingTraderTrades::_registerFirstPool()
{
    auto& listings = s_pools[0].listings;

    listings.push_back(makeItemsForEmeralds(ItemCost(ItemStack(*Items::POTION, 1), 1).withPotion("minecraft:water"),
        _stack("minecraft:emerald", 1),
        1,
        1,
        0.05f));
    listings.push_back(_buy("minecraft:water_bucket", 1, 2, 1, 2));
    listings.push_back(_buy("minecraft:milk_bucket", 1, 2, 1, 2));
    listings.push_back(_buy("minecraft:fermented_spider_eye", 1, 2, 1, 3));
    listings.push_back(_buy("minecraft:baked_potato", 4, 2, 1, 1));
    listings.push_back(_buy("minecraft:hay_block", 1, 2, 1, 1));

    listings.push_back(_sell("minecraft:packed_ice", 1, 1, 6, 1));
    listings.push_back(_sell("minecraft:blue_ice", 1, 6, 6, 1));
    listings.push_back(_sell("minecraft:gunpowder", 4, 1, 2, 1));
    listings.push_back(_sell("minecraft:podzol", 3, 3, 6, 1));

    static const char* const LOGS[] = {"minecraft:acacia_log",
        "minecraft:birch_log",
        "minecraft:dark_oak_log",
        "minecraft:jungle_log",
        "minecraft:oak_log",
        "minecraft:spruce_log",
        "minecraft:cherry_log",
        "minecraft:mangrove_log",
        "minecraft:pale_oak_log"};
    for (const char* log : LOGS) {
        listings.push_back(_sell(log, 8, 1, 4, 1));
    }

    listings.push_back(makeEnchantedItem(Items::IRON_PICKAXE, 1, 1, 1, 0.2f));

    ItemStack invisibilityPotion = ItemStack(*Items::POTION, 1);
    potion::PotionUtils::setPotion(invisibilityPotion, potion::Potions::LONG_INVISIBILITY);
    listings.push_back(makeItemsForEmeralds(_emeralds(5), std::move(invisibilityPotion), 1, 1, 0.05f));
}

// ============================================================================
// 第二个池：稀有商品
// ============================================================================

void WanderingTraderTrades::_registerSecondPool()
{
    auto& listings = s_pools[1].listings;

    listings.push_back(_sell("minecraft:tropical_fish_bucket", 1, 3, 4, 1));
    listings.push_back(_sell("minecraft:pufferfish_bucket", 1, 3, 4, 1));
    listings.push_back(_sell("minecraft:sea_pickle", 1, 2, 5, 1));
    listings.push_back(_sell("minecraft:slime_ball", 1, 4, 5, 1));
    listings.push_back(_sell("minecraft:glowstone", 1, 2, 5, 1));
    listings.push_back(_sell("minecraft:nautilus_shell", 1, 5, 5, 1));
    listings.push_back(_sell("minecraft:fern", 1, 1, DEFAULT_SUPPLY, 1));
    listings.push_back(_sell("minecraft:sugar_cane", 1, 1, 8, 1));
    listings.push_back(_sell("minecraft:pumpkin", 1, 1, 4, 1));
    listings.push_back(_sell("minecraft:kelp", 1, 3, DEFAULT_SUPPLY, 1));
    listings.push_back(_sell("minecraft:cactus", 1, 3, 8, 1));

    static const char* const FLOWERS[] = {"minecraft:dandelion",
        "minecraft:poppy",
        "minecraft:blue_orchid",
        "minecraft:allium",
        "minecraft:azure_bluet",
        "minecraft:red_tulip",
        "minecraft:orange_tulip",
        "minecraft:white_tulip",
        "minecraft:pink_tulip",
        "minecraft:oxeye_daisy",
        "minecraft:cornflower",
        "minecraft:lily_of_the_valley",
        "minecraft:open_eyeblossom"};
    for (const char* flower : FLOWERS) {
        const bool rareFlower = (flower == std::string_view("minecraft:lily_of_the_valley") ||
            flower == std::string_view("minecraft:open_eyeblossom"));
        listings.push_back(_sell(flower, 1, 1, rareFlower ? 7 : DEFAULT_SUPPLY, 1));
    }

    static const char* const SEEDS[] = {
        "minecraft:wheat_seeds", "minecraft:beetroot_seeds", "minecraft:pumpkin_seeds", "minecraft:melon_seeds"};
    for (const char* seed : SEEDS) {
        listings.push_back(_sell(seed, 1, 1, DEFAULT_SUPPLY, 1));
    }

    static const char* const SAPLINGS[] = {"minecraft:acacia_sapling",
        "minecraft:birch_sapling",
        "minecraft:dark_oak_sapling",
        "minecraft:jungle_sapling",
        "minecraft:oak_sapling",
        "minecraft:spruce_sapling",
        "minecraft:cherry_sapling",
        "minecraft:pale_oak_sapling",
        "minecraft:mangrove_propagule"};
    for (const char* sapling : SAPLINGS) {
        listings.push_back(_sell(sapling, 1, 5, 8, 1));
    }

    static const char* const DYES[] = {"minecraft:red_dye",
        "minecraft:white_dye",
        "minecraft:blue_dye",
        "minecraft:pink_dye",
        "minecraft:black_dye",
        "minecraft:green_dye",
        "minecraft:light_gray_dye",
        "minecraft:magenta_dye",
        "minecraft:yellow_dye",
        "minecraft:gray_dye",
        "minecraft:purple_dye",
        "minecraft:light_blue_dye",
        "minecraft:lime_dye",
        "minecraft:orange_dye",
        "minecraft:brown_dye",
        "minecraft:cyan_dye"};
    for (const char* dye : DYES) {
        listings.push_back(_sell(dye, 3, 1, DEFAULT_SUPPLY, 1));
    }

    static const char* const CORAL_BLOCKS[] = {"minecraft:brain_coral_block",
        "minecraft:bubble_coral_block",
        "minecraft:fire_coral_block",
        "minecraft:horn_coral_block",
        "minecraft:tube_coral_block"};
    for (const char* block : CORAL_BLOCKS) {
        listings.push_back(_sell(block, 1, 3, 8, 1));
    }

    listings.push_back(_sell("minecraft:vine", 3, 1, 4, 1));
    listings.push_back(_sell("minecraft:pale_hanging_moss", 3, 1, 4, 1));
    listings.push_back(_sell("minecraft:brown_mushroom", 3, 1, 4, 1));
    listings.push_back(_sell("minecraft:red_mushroom", 3, 1, 4, 1));
    listings.push_back(_sell("minecraft:lily_pad", 5, 1, 2, 1));
    listings.push_back(_sell("minecraft:small_dripleaf", 2, 1, 5, 1));
    listings.push_back(_sell("minecraft:sand", 8, 1, 8, 1));
    listings.push_back(_sell("minecraft:red_sand", 4, 1, 6, 1));
    listings.push_back(_sell("minecraft:pointed_dripstone", 2, 1, 5, 1));
    listings.push_back(_sell("minecraft:rooted_dirt", 2, 1, 5, 1));
    listings.push_back(_sell("minecraft:moss_block", 2, 1, 5, 1));
    listings.push_back(_sell("minecraft:pale_moss_block", 2, 1, 5, 1));
    listings.push_back(_sell("minecraft:wildflowers", 1, 1, DEFAULT_SUPPLY, 1));
    listings.push_back(_sell("minecraft:tall_dry_grass", 1, 1, DEFAULT_SUPPLY, 1));
    listings.push_back(_sell("minecraft:firefly_bush", 1, 3, DEFAULT_SUPPLY, 1));
}

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc