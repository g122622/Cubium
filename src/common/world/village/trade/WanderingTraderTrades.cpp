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

#include "WanderingTraderTrades.hpp"

#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"

#include <algorithm>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace village {
namespace trade {

// 静态成员初始化
std::vector<WanderingTraderTrades::TradeFactory> WanderingTraderTrades::s_normalTrades;
std::vector<WanderingTraderTrades::TradeFactory> WanderingTraderTrades::s_rareTrades;
bool WanderingTraderTrades::s_initialized = false;

// ============================================================================
// 初始化
// ============================================================================

void WanderingTraderTrades::initialize()
{
    if (s_initialized) {
        return;
    }

    spdlog::info("Initializing wandering trader trades...");

    _registerOceanTrades();
    _registerPlantTrades();
    _registerFlowerTrades();
    _registerSeedTrades();
    _registerSaplingTrades();
    _registerDyeTrades();
    _registerCoralTrades();
    _registerRareItemTrades();

    s_initialized = true;
    spdlog::info("Wandering trader trades initialized: {} normal, {} rare", s_normalTrades.size(), s_rareTrades.size());
}

// ============================================================================
// 交易生成
// ============================================================================

std::unique_ptr<MerchantOffers> WanderingTraderTrades::generateNormalOffers(u64 seed, i32 count)
{
    auto offers = std::make_unique<MerchantOffers>();

    if (s_normalTrades.empty()) {
        return offers;
    }

    // 确定要选取的交易数量
    size_t selectCount = (count < 0) ? s_normalTrades.size() : static_cast<size_t>(count);
    selectCount = std::min(selectCount, s_normalTrades.size());

    // 使用种子创建随机数生成器
    math::Random rng(seed);

    // 创建索引并打乱
    std::vector<size_t> indices(s_normalTrades.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }

    // Fisher-Yates 洗牌
    for (size_t i = indices.size() - 1; i > 0; --i) {
        size_t j = rng.nextInt(static_cast<i32>(i + 1));
        std::swap(indices[i], indices[j]);
    }

    // 选取指定数量的交易
    for (size_t i = 0; i < selectCount; ++i) {
        const auto& factory = s_normalTrades[indices[i]];
        auto offer = factory(rng.nextLong());
        if (offer) {
            offers->addOffer(std::move(offer));
        }
    }

    return offers;
}

std::unique_ptr<MerchantOffers> WanderingTraderTrades::generateRareOffers(u64 seed, i32 count)
{
    auto offers = std::make_unique<MerchantOffers>();

    if (s_rareTrades.empty()) {
        return offers;
    }

    // 确定要选取的交易数量
    size_t selectCount = (count < 0) ? s_rareTrades.size() : static_cast<size_t>(count);
    selectCount = std::min(selectCount, s_rareTrades.size());

    // 使用种子创建随机数生成器
    math::Random rng(seed * 31 + 17); // 使用不同的种子

    // 创建索引并打乱
    std::vector<size_t> indices(s_rareTrades.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }

    // Fisher-Yates 洗牌
    for (size_t i = indices.size() - 1; i > 0; --i) {
        size_t j = rng.nextInt(static_cast<i32>(i + 1));
        std::swap(indices[i], indices[j]);
    }

    // 选取指定数量的交易
    for (size_t i = 0; i < selectCount; ++i) {
        const auto& factory = s_rareTrades[indices[i]];
        auto offer = factory(rng.nextLong());
        if (offer) {
            offers->addOffer(std::move(offer));
        }
    }

    return offers;
}

std::unique_ptr<MerchantOffers> WanderingTraderTrades::generateOffers(u64 seed)
{
    auto offers = std::make_unique<MerchantOffers>();

    // 流浪商人交易生成规则：
    // - 从普通池中随机选取多个交易
    // - 从稀有池中随机选取少量交易

    // 添加普通交易（随机选取）
    auto normalOffers = generateNormalOffers(seed);
    for (size_t i = 0; i < normalOffers->size(); ++i) {
        auto* offer = normalOffers->getOffer(i);
        if (offer) {
            offers->addOffer(std::make_unique<MerchantOffer>(*offer));
        }
    }

    // 添加稀有交易（随机选取）
    auto rareOffers = generateRareOffers(seed);
    for (size_t i = 0; i < rareOffers->size(); ++i) {
        auto* offer = rareOffers->getOffer(i);
        if (offer) {
            offers->addOffer(std::make_unique<MerchantOffer>(*offer));
        }
    }

    return offers;
}

// ============================================================================
// 交易工厂辅助方法
// ============================================================================

WanderingTraderTrades::TradeFactory WanderingTraderTrades::_sellForEmeralds(
    const char* sellItem, i32 sellCount, i32 emeraldCount, i32 maxUses, i32 xp)
{

    return [sellItem, sellCount, emeraldCount, maxUses, xp](u64 seed) -> std::unique_ptr<MerchantOffer> {
        (void)seed;

        const Item* sell = ItemRegistry::instance().getItem(ResourceLocation(sellItem));
        const Item* emerald = ItemRegistry::instance().getItem(ResourceLocation("emerald"));

        if (!sell || !emerald) {
            return nullptr;
        }

        // 玩家出售物品，获得绿宝石
        ItemStack sellStack(*sell, sellCount);
        ItemStack emeraldStack(*emerald, emeraldCount);

        return std::make_unique<MerchantOffer>(emeraldStack, sellStack, maxUses, xp, 0.05f);
    };
}

WanderingTraderTrades::TradeFactory WanderingTraderTrades::_buyForEmeralds(
    i32 emeraldCount, const char* buyItem, i32 buyCount, i32 maxUses, i32 xp)
{

    return [emeraldCount, buyItem, buyCount, maxUses, xp](u64 seed) -> std::unique_ptr<MerchantOffer> {
        (void)seed;

        const Item* buy = ItemRegistry::instance().getItem(ResourceLocation(buyItem));
        const Item* emerald = ItemRegistry::instance().getItem(ResourceLocation("emerald"));

        if (!buy || !emerald) {
            return nullptr;
        }

        // 玩家用绿宝石购买物品
        ItemStack emeraldStack(*emerald, emeraldCount);
        ItemStack buyStack(*buy, buyCount);

        return std::make_unique<MerchantOffer>(emeraldStack, buyStack, maxUses, xp, 0.05f);
    };
}

void WanderingTraderTrades::_registerNormalTrade(TradeFactory factory)
{
    s_normalTrades.push_back(std::move(factory));
}

void WanderingTraderTrades::_registerRareTrade(TradeFactory factory)
{
    s_rareTrades.push_back(std::move(factory));
}

// ============================================================================
// 海洋物品交易
// ============================================================================

void WanderingTraderTrades::_registerOceanTrades()
{
    // 海泡菜：2 绿宝石 -> 1 海泡菜
    _registerNormalTrade(_buyForEmeralds(2, "sea_pickle", 1, 5, 1));

    // 粘液球：4 绿宝石 -> 1 粘液球
    _registerNormalTrade(_buyForEmeralds(4, "slime_ball", 1, 5, 1));

    // 荧石：2 绿宝石 -> 1 荧石
    _registerNormalTrade(_buyForEmeralds(2, "glowstone", 1, 5, 1));

    // 鹦鹉螺壳：5 绿宝石 -> 1 鹦鹉螺壳
    _registerNormalTrade(_buyForEmeralds(5, "nautilus_shell", 1, 5, 1));

    // 稀有：热带鱼桶
    _registerRareTrade(_buyForEmeralds(5, "tropical_fish_bucket", 1, 4, 1));

    // 稀有：河豚桶
    _registerRareTrade(_buyForEmeralds(5, "pufferfish_bucket", 1, 4, 1));
}

// ============================================================================
// 植物交易
// ============================================================================

void WanderingTraderTrades::_registerPlantTrades()
{
    // 蕨：1 绿宝石 -> 1 蕨
    _registerNormalTrade(_buyForEmeralds(1, "fern", 1, 12, 1));

    // 甘蔗：1 绿宝石 -> 1 甘蔗
    _registerNormalTrade(_buyForEmeralds(1, "sugar_cane", 1, 8, 1));

    // 南瓜：1 绿宝石 -> 1 南瓜
    _registerNormalTrade(_buyForEmeralds(1, "pumpkin", 1, 4, 1));

    // 海带：3 绿宝石 -> 1 海带
    _registerNormalTrade(_buyForEmeralds(3, "kelp", 1, 12, 1));

    // 仙人掌：3 绿宝石 -> 1 仙人掌
    _registerNormalTrade(_buyForEmeralds(3, "cactus", 1, 8, 1));

    // 藤蔓：1 绿宝石 -> 1 藤蔓
    _registerNormalTrade(_buyForEmeralds(1, "vine", 1, 12, 1));

    // 棕色蘑菇：1 绿宝石 -> 1 棕色蘑菇
    _registerNormalTrade(_buyForEmeralds(1, "brown_mushroom", 1, 12, 1));

    // 红色蘑菇：1 绿宝石 -> 1 红色蘑菇
    _registerNormalTrade(_buyForEmeralds(1, "red_mushroom", 1, 12, 1));

    // 睡莲：1 绿宝石 -> 2 睡莲
    _registerNormalTrade(_buyForEmeralds(1, "lily_pad", 2, 5, 1));

    // 沙子：1 绿宝石 -> 8 沙子
    _registerNormalTrade(_buyForEmeralds(1, "sand", 8, 8, 1));

    // 红沙：1 绿宝石 -> 4 红沙
    _registerNormalTrade(_buyForEmeralds(1, "red_sand", 4, 6, 1));
}

// ============================================================================
// 花朵交易
// ============================================================================

void WanderingTraderTrades::_registerFlowerTrades()
{
    // 蒲公英：1 绿宝石 -> 1 蒲公英
    _registerNormalTrade(_buyForEmeralds(1, "dandelion", 1, 12, 1));

    // 罂粟：1 绿宝石 -> 1 罂粟
    _registerNormalTrade(_buyForEmeralds(1, "poppy", 1, 12, 1));

    // 兰花：1 绿宝石 -> 1 兰花
    _registerNormalTrade(_buyForEmeralds(1, "blue_orchid", 1, 8, 1));

    // 绒球葱：1 绿宝石 -> 1 绒球葱
    _registerNormalTrade(_buyForEmeralds(1, "allium", 1, 12, 1));

    // 茜草花：1 绿宝石 -> 1 茜草花
    _registerNormalTrade(_buyForEmeralds(1, "azure_bluet", 1, 12, 1));

    // 红色郁金香：1 绿宝石 -> 1 红色郁金香
    _registerNormalTrade(_buyForEmeralds(1, "red_tulip", 1, 12, 1));

    // 橙色郁金香：1 绿宝石 -> 1 橙色郁金香
    _registerNormalTrade(_buyForEmeralds(1, "orange_tulip", 1, 12, 1));

    // 白色郁金香：1 绿宝石 -> 1 白色郁金香
    _registerNormalTrade(_buyForEmeralds(1, "white_tulip", 1, 12, 1));

    // 粉色郁金香：1 绿宝石 -> 1 粉色郁金香
    _registerNormalTrade(_buyForEmeralds(1, "pink_tulip", 1, 12, 1));

    // 雏菊：1 绿宝石 -> 1 雏菊
    _registerNormalTrade(_buyForEmeralds(1, "oxeye_daisy", 1, 12, 1));

    // 矢车菊：1 绿宝石 -> 1 矢车菊
    _registerNormalTrade(_buyForEmeralds(1, "cornflower", 1, 12, 1));

    // 铃兰：1 绿宝石 -> 1 铃兰
    _registerNormalTrade(_buyForEmeralds(1, "lily_of_the_valley", 1, 7, 1));
}

// ============================================================================
// 种子交易
// ============================================================================

void WanderingTraderTrades::_registerSeedTrades()
{
    // 小麦种子：1 绿宝石 -> 1 小麦种子
    _registerNormalTrade(_buyForEmeralds(1, "wheat_seeds", 1, 12, 1));

    // 甜菜种子：1 绿宝石 -> 1 甜菜种子
    _registerNormalTrade(_buyForEmeralds(1, "beetroot_seeds", 1, 12, 1));

    // 南瓜种子：1 绿宝石 -> 1 南瓜种子
    _registerNormalTrade(_buyForEmeralds(1, "pumpkin_seeds", 1, 12, 1));

    // 西瓜种子：1 绿宝石 -> 1 西瓜种子
    _registerNormalTrade(_buyForEmeralds(1, "melon_seeds", 1, 12, 1));
}

// ============================================================================
// 树苗交易
// ============================================================================

void WanderingTraderTrades::_registerSaplingTrades()
{
    // 金合欢树苗：5 绿宝石 -> 1 金合欢树苗
    _registerNormalTrade(_buyForEmeralds(5, "acacia_sapling", 1, 8, 1));

    // 白桦树苗：5 绿宝石 -> 1 白桦树苗
    _registerNormalTrade(_buyForEmeralds(5, "birch_sapling", 1, 8, 1));

    // 深色橡树树苗：5 绿宝石 -> 1 深色橡树树苗
    _registerNormalTrade(_buyForEmeralds(5, "dark_oak_sapling", 1, 8, 1));

    // 丛林树苗：5 绿宝石 -> 1 丛林树苗
    _registerNormalTrade(_buyForEmeralds(5, "jungle_sapling", 1, 8, 1));

    // 橡树树苗：5 绿宝石 -> 1 橡树树苗
    _registerNormalTrade(_buyForEmeralds(5, "oak_sapling", 1, 8, 1));

    // 云杉树苗：5 绿宝石 -> 1 云杉树苗
    _registerNormalTrade(_buyForEmeralds(5, "spruce_sapling", 1, 8, 1));
}

// ============================================================================
// 染料交易
// ============================================================================

void WanderingTraderTrades::_registerDyeTrades()
{
    // 所有染料：1 绿宝石 -> 3 染料
    _registerNormalTrade(_buyForEmeralds(1, "red_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "white_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "blue_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "pink_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "black_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "green_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "light_gray_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "magenta_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "yellow_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "gray_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "purple_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "light_blue_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "lime_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "orange_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "brown_dye", 3, 12, 1));
    _registerNormalTrade(_buyForEmeralds(1, "cyan_dye", 3, 12, 1));
}

// ============================================================================
// 珊瑚方块交易
// ============================================================================

void WanderingTraderTrades::_registerCoralTrades()
{
    // 脑纹珊瑚方块：3 绿宝石 -> 1 脑纹珊瑚方块
    _registerNormalTrade(_buyForEmeralds(3, "brain_coral_block", 1, 8, 1));

    // 气泡珊瑚方块：3 绿宝石 -> 1 气泡珊瑚方块
    _registerNormalTrade(_buyForEmeralds(3, "bubble_coral_block", 1, 8, 1));

    // 火珊瑚方块：3 绿宝石 -> 1 火珊瑚方块
    _registerNormalTrade(_buyForEmeralds(3, "fire_coral_block", 1, 8, 1));

    // 鹿角珊瑚方块：3 绿宝石 -> 1 鹿角珊瑚方块
    _registerNormalTrade(_buyForEmeralds(3, "horn_coral_block", 1, 8, 1));

    // 管状珊瑚方块：3 绿宝石 -> 1 管状珊瑚方块
    _registerNormalTrade(_buyForEmeralds(3, "tube_coral_block", 1, 8, 1));
}

// ============================================================================
// 稀有物品交易
// ============================================================================

void WanderingTraderTrades::_registerRareItemTrades()
{
    // 浮冰：3 绿宝石 -> 1 浮冰
    _registerRareTrade(_buyForEmeralds(3, "packed_ice", 1, 6, 1));

    // 蓝冰：6 绿宝石 -> 1 蓝冰
    _registerRareTrade(_buyForEmeralds(6, "blue_ice", 1, 6, 1));

    // 火药：1 绿宝石 -> 1 火药
    _registerRareTrade(_buyForEmeralds(1, "gunpowder", 1, 8, 1));

    // 灰化土：3 绿宝石 -> 3 灰化土
    _registerRareTrade(_buyForEmeralds(3, "podzol", 3, 6, 1));
}

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
