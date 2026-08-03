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

#include "common/world/village/trade/VillagerTrades.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace village {
namespace trade {

// 静态成员初始化
std::unordered_map<entity::VillagerProfession, VillagerTrades::LevelTrades> VillagerTrades::s_trades;
bool VillagerTrades::s_initialized = false;

// ============================================================================
// 初始化
// ============================================================================

void VillagerTrades::initialize()
{
    if (s_initialized) {
        return;
    }

    spdlog::info("Initializing villager trades...");

    _registerArmorerTrades();
    _registerButcherTrades();
    _registerCartographerTrades();
    _registerClericTrades();
    _registerFarmerTrades();
    _registerFishermanTrades();
    _registerFletcherTrades();
    _registerLeatherworkerTrades();
    _registerLibrarianTrades();
    _registerMasonTrades();
    _registerShepherdTrades();
    _registerToolsmithTrades();
    _registerWeaponsmithTrades();
    _registerNitwitTrades();

    s_initialized = true;
    spdlog::info("Villager trades initialized");
}

std::unique_ptr<MerchantOffers> VillagerTrades::generateOffers(
    entity::VillagerProfession profession, entity::VillagerType type, i32 level, i32 demand, u64 seed)
{

    auto offers = std::make_unique<MerchantOffers>();

    // 傻子村民没有交易
    if (profession == entity::VillagerProfession::Nitwit) {
        return offers;
    }

    auto profIt = s_trades.find(profession);
    if (profIt == s_trades.end()) {
        return offers;
    }

    const LevelTrades& levelTrades = profIt->second;

    // 生成当前等级及以下等级的交易
    for (i32 lvl = 1; lvl <= level; ++lvl) {
        auto levelIt = levelTrades.find(lvl);
        if (levelIt == levelTrades.end()) {
            continue;
        }

        const std::vector<TradeFactory>& factories = levelIt->second;
        for (const auto& factory : factories) {
            auto offer = factory(demand, seed);
            if (offer) {
                offers->addOffer(std::move(offer));
            }
        }
    }

    // 村民类型不影响交易，但可以用于外观选择
    (void)type;

    return offers;
}

bool VillagerTrades::hasTrades(entity::VillagerProfession profession)
{
    return s_trades.find(profession) != s_trades.end();
}

i32 VillagerTrades::getTradeLevelCount(entity::VillagerProfession profession)
{
    auto it = s_trades.find(profession);
    if (it == s_trades.end()) {
        return 0;
    }
    return static_cast<i32>(it->second.size());
}

// ============================================================================
// 交易工厂辅助方法
// ============================================================================

TradeFactory VillagerTrades::_simpleTrade(
    const char* buyItem, i32 buyCount, const char* sellItem, i32 sellCount, i32 maxUses, i32 xp, f32 priceMultiplier)
{

    return [buyItem, buyCount, sellItem, sellCount, maxUses, xp, priceMultiplier](
               i32 demand, u64 seed) -> std::unique_ptr<MerchantOffer> {
        (void)seed;

        // 获取物品
        const Item* buy = ItemRegistry::instance().getItem(ResourceLocation(buyItem));
        const Item* sell = ItemRegistry::instance().getItem(ResourceLocation(sellItem));

        if (!buy || !sell) {
            return nullptr;
        }

        // 创建交易
        ItemStack buyStack(*buy, buyCount);
        ItemStack sellStack(*sell, sellCount);

        auto offer = std::make_unique<MerchantOffer>(buyStack, sellStack, maxUses, xp, priceMultiplier);

        // 应用需求调整
        if (demand != 0) {
            offer->applyDemand(demand);
        }

        return offer;
    };
}

TradeFactory VillagerTrades::_twoItemTrade(const char* buyItemA,
    i32 buyCountA,
    const char* buyItemB,
    i32 buyCountB,
    const char* sellItem,
    i32 sellCount,
    i32 maxUses,
    i32 xp,
    f32 priceMultiplier)
{

    return [buyItemA, buyCountA, buyItemB, buyCountB, sellItem, sellCount, maxUses, xp, priceMultiplier](
               i32 demand, u64 seed) -> std::unique_ptr<MerchantOffer> {
        (void)seed;

        const Item* buyA = ItemRegistry::instance().getItem(ResourceLocation(buyItemA));
        const Item* buyB = ItemRegistry::instance().getItem(ResourceLocation(buyItemB));
        const Item* sell = ItemRegistry::instance().getItem(ResourceLocation(sellItem));

        if (!buyA || !buyB || !sell) {
            return nullptr;
        }

        ItemStack buyStackA(*buyA, buyCountA);
        ItemStack buyStackB(*buyB, buyCountB);
        ItemStack sellStack(*sell, sellCount);

        auto offer = std::make_unique<MerchantOffer>(buyStackA, buyStackB, sellStack, maxUses, xp, priceMultiplier);

        if (demand != 0) {
            offer->applyDemand(demand);
        }

        return offer;
    };
}

TradeFactory VillagerTrades::_demandTrade(i32 baseBuyCount,
    const char* buyItem,
    const char* sellItem,
    i32 sellCount,
    i32 maxUses,
    i32 xp,
    f32 priceMultiplier)
{

    return [baseBuyCount, buyItem, sellItem, sellCount, maxUses, xp, priceMultiplier](
               i32 demand, u64 seed) -> std::unique_ptr<MerchantOffer> {
        (void)seed;

        const Item* buy = ItemRegistry::instance().getItem(ResourceLocation(buyItem));
        const Item* sell = ItemRegistry::instance().getItem(ResourceLocation(sellItem));

        if (!buy || !sell) {
            return nullptr;
        }

        // 需求调整买入数量
        i32 adjustedCount = baseBuyCount + demand;
        adjustedCount = std::max(1, adjustedCount);

        ItemStack buyStack(*buy, adjustedCount);
        ItemStack sellStack(*sell, sellCount);

        return std::make_unique<MerchantOffer>(buyStack, sellStack, maxUses, xp, priceMultiplier);
    };
}

void VillagerTrades::_registerTrade(entity::VillagerProfession profession, i32 level, TradeFactory factory)
{

    s_trades[profession][level].push_back(std::move(factory));
}

// ============================================================================
// 盔甲匠交易
// ============================================================================

void VillagerTrades::_registerArmorerTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Armorer, 1, _simpleTrade("coal", 15, "emerald", 1, 16, 2));
    _registerTrade(Profession::Armorer, 1, _demandTrade(7, "iron_ingot", "emerald", 1, 12, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Armorer, 2, _demandTrade(4, "iron_ingot", "iron_boots", 1, 12, 10));
    _registerTrade(Profession::Armorer, 2, _demandTrade(36, "emerald", "bell", 1, 12, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Armorer, 3, _demandTrade(4, "diamond", "emerald", 1, 12, 20));
    _registerTrade(Profession::Armorer, 3, _demandTrade(1, "emerald", "chainmail_leggings", 1, 12, 10));

    // 等级4 - 专家
    _registerTrade(Profession::Armorer, 4, _demandTrade(1, "emerald", "chainmail_boots", 1, 12, 15));
    _registerTrade(Profession::Armorer, 4, _demandTrade(1, "emerald", "chainmail_helmet", 1, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Armorer, 5, _demandTrade(7, "diamond", "diamond_leggings", 1, 12, 30));
    _registerTrade(Profession::Armorer, 5, _demandTrade(6, "diamond", "diamond_chestplate", 1, 12, 30));
}

// ============================================================================
// 屠夫交易
// ============================================================================

void VillagerTrades::_registerButcherTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Butcher, 1, _demandTrade(15, "coal", "emerald", 1, 16, 2));
    _registerTrade(Profession::Butcher, 1, _simpleTrade("porkchop", 7, "emerald", 1, 16, 2));
    _registerTrade(Profession::Butcher, 1, _simpleTrade("rabbit", 4, "emerald", 1, 16, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Butcher, 2, _simpleTrade("beef", 10, "emerald", 1, 16, 5));
    _registerTrade(Profession::Butcher, 2, _simpleTrade("mutton", 7, "emerald", 1, 16, 5));
    _registerTrade(Profession::Butcher, 2, _simpleTrade("chicken", 8, "emerald", 1, 16, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Butcher, 3, _demandTrade(1, "emerald", "cooked_porkchop", 5, 8, 10));
    _registerTrade(Profession::Butcher, 3, _demandTrade(1, "emerald", "cooked_chicken", 8, 8, 10));

    // 等级4 - 专家
    _registerTrade(Profession::Butcher, 4, _simpleTrade("dried_kelp_block", 10, "emerald", 1, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Butcher, 5, _demandTrade(1, "emerald", "rabbit_stew", 1, 12, 30));
}

// ============================================================================
// 制图师交易
// ============================================================================

void VillagerTrades::_registerCartographerTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Cartographer, 1, _demandTrade(23, "paper", "emerald", 1, 16, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Cartographer, 2, _demandTrade(11, "glass_pane", "emerald", 1, 16, 10));
    _registerTrade(Profession::Cartographer, 2, _demandTrade(1, "emerald", "map", 1, 12, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Cartographer, 3, _demandTrade(8, "compass", "emerald", 1, 12, 10));
    _registerTrade(Profession::Cartographer, 3, _demandTrade(13, "emerald", "item_frame", 1, 12, 10));

    // 等级4 - 专家
    _registerTrade(Profession::Cartographer, 4, _demandTrade(14, "emerald", "banner", 1, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Cartographer, 5, _demandTrade(7, "emerald", "banner_pattern", 1, 12, 30));
}

// ============================================================================
// 牧师交易
// ============================================================================

void VillagerTrades::_registerClericTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Cleric, 1, _demandTrade(32, "rotten_flesh", "emerald", 1, 16, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Cleric, 2, _demandTrade(1, "emerald", "redstone", 2, 12, 5));
    _registerTrade(Profession::Cleric, 2, _demandTrade(3, "gold_ingot", "emerald", 1, 12, 10));

    // 等级3 - 老手
    _registerTrade(Profession::Cleric, 3, _demandTrade(1, "emerald", "lapis_lazuli", 1, 12, 10));
    _registerTrade(Profession::Cleric, 3, _demandTrade(10, "rabbit_foot", "emerald", 1, 12, 20));

    // 等级4 - 专家
    _registerTrade(Profession::Cleric, 4, _demandTrade(1, "emerald", "glowstone", 4, 12, 15));
    _registerTrade(Profession::Cleric, 4, _demandTrade(3, "emerald", "glass_bottle", 1, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Cleric, 5, _demandTrade(5, "emerald", "ender_pearl", 1, 12, 30));
    _registerTrade(Profession::Cleric, 5, _demandTrade(4, "emerald", "experience_bottle", 1, 12, 30));
}

// ============================================================================
// 农民交易
// ============================================================================

void VillagerTrades::_registerFarmerTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Farmer, 1, _demandTrade(20, "wheat", "emerald", 1, 16, 2));
    _registerTrade(Profession::Farmer, 1, _demandTrade(15, "potato", "emerald", 1, 16, 2));
    _registerTrade(Profession::Farmer, 1, _demandTrade(15, "carrot", "emerald", 1, 16, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Farmer, 2, _demandTrade(22, "beetroot", "emerald", 1, 16, 5));
    _registerTrade(Profession::Farmer, 2, _demandTrade(15, "pumpkin", "emerald", 1, 12, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Farmer, 3, _demandTrade(6, "emerald", "apple", 4, 12, 10));
    _registerTrade(Profession::Farmer, 3, _demandTrade(1, "emerald", "cookie", 18, 12, 10));

    // 等级4 - 专家
    _registerTrade(Profession::Farmer, 4, _demandTrade(1, "emerald", "cake", 1, 12, 15));
    _registerTrade(Profession::Farmer, 4, _demandTrade(1, "emerald", "suspicious_stew", 1, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Farmer, 5, _demandTrade(3, "emerald", "golden_carrot", 3, 12, 30));
    _registerTrade(Profession::Farmer, 5, _demandTrade(1, "emerald", "glistering_melon_slice", 3, 12, 30));
}

// ============================================================================
// 渔夫交易
// ============================================================================

void VillagerTrades::_registerFishermanTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Fisherman, 1, _demandTrade(10, "coal", "emerald", 1, 16, 2));
    _registerTrade(Profession::Fisherman, 1, _simpleTrade("cod", 6, "emerald", 1, 16, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Fisherman, 2, _simpleTrade("salmon", 6, "emerald", 1, 16, 5));
    _registerTrade(Profession::Fisherman, 2, _demandTrade(2, "emerald", "campfire", 1, 12, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Fisherman, 3, _demandTrade(6, "emerald", "fishing_rod", 1, 12, 10));
    _registerTrade(Profession::Fisherman, 3, _simpleTrade("tropical_fish", 6, "emerald", 1, 12, 20));

    // 等级4 - 专家
    _registerTrade(Profession::Fisherman, 4, _demandTrade(1, "emerald", "cooked_cod", 6, 12, 15));
    _registerTrade(Profession::Fisherman, 4, _demandTrade(1, "emerald", "cooked_salmon", 6, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Fisherman, 5, _demandTrade(8, "emerald", "enchanted_fishing_rod", 1, 12, 30));
}

// ============================================================================
// 制箭师交易
// ============================================================================

void VillagerTrades::_registerFletcherTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Fletcher, 1, _demandTrade(32, "stick", "emerald", 1, 16, 2));
    _registerTrade(Profession::Fletcher, 1, _demandTrade(1, "emerald", "arrow", 16, 12, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Fletcher, 2, _demandTrade(16, "flint", "emerald", 1, 12, 10));
    _registerTrade(Profession::Fletcher, 2, _demandTrade(1, "emerald", "bow", 1, 12, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Fletcher, 3, _demandTrade(24, "feather", "emerald", 1, 16, 10));
    _registerTrade(Profession::Fletcher, 3, _demandTrade(2, "emerald", "crossbow", 1, 12, 10));

    // 等级4 - 专家
    _registerTrade(Profession::Fletcher, 4, _demandTrade(1, "emerald", "spectral_arrow", 8, 12, 15));
    _registerTrade(Profession::Fletcher, 4, _demandTrade(1, "emerald", "tipped_arrow", 8, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Fletcher, 5, _demandTrade(7, "emerald", "enchanted_bow", 1, 12, 30));
    _registerTrade(Profession::Fletcher, 5, _demandTrade(1, "emerald", "arrow_of_poison", 16, 12, 30));
}

// ============================================================================
// 皮革匠交易
// ============================================================================

void VillagerTrades::_registerLeatherworkerTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Leatherworker, 1, _demandTrade(6, "leather", "emerald", 1, 16, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Leatherworker, 2, _demandTrade(4, "rabbit_hide", "emerald", 1, 16, 5));
    _registerTrade(Profession::Leatherworker, 2, _demandTrade(1, "emerald", "leather_pants", 1, 12, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Leatherworker, 3, _demandTrade(3, "emerald", "leather_tunic", 1, 12, 10));
    _registerTrade(Profession::Leatherworker, 3, _demandTrade(2, "emerald", "leather_boots", 1, 12, 10));

    // 等级4 - 专家
    _registerTrade(Profession::Leatherworker, 4, _demandTrade(5, "emerald", "leather_helmet", 1, 12, 15));
    _registerTrade(Profession::Leatherworker, 4, _demandTrade(7, "emerald", "saddle", 1, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Leatherworker, 5, _demandTrade(6, "emerald", "leather_horse_armor", 1, 12, 30));
}

// ============================================================================
// 图书管理员交易
// ============================================================================

void VillagerTrades::_registerLibrarianTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Librarian, 1, _demandTrade(24, "paper", "emerald", 1, 16, 2));
    _registerTrade(Profession::Librarian, 1, _demandTrade(9, "emerald", "book", 1, 12, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Librarian, 2, _demandTrade(5, "book", "emerald", 1, 12, 10));
    _registerTrade(Profession::Librarian, 2, _demandTrade(1, "emerald", "bookshelf", 1, 12, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Librarian, 3, _demandTrade(4, "written_book", "emerald", 1, 12, 10));
    _registerTrade(Profession::Librarian, 3, _demandTrade(2, "emerald", "lantern", 1, 12, 10));

    // 等级4 - 专家
    _registerTrade(Profession::Librarian, 4, _demandTrade(5, "emerald", "enchanted_book", 1, 12, 15));
    _registerTrade(Profession::Librarian, 4, _demandTrade(5, "emerald", "glass", 4, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Librarian, 5, _demandTrade(20, "emerald", "name_tag", 1, 12, 30));
}

// ============================================================================
// 石匠交易
// ============================================================================

void VillagerTrades::_registerMasonTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Mason, 1, _demandTrade(10, "clay_ball", "emerald", 1, 16, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Mason, 2, _demandTrade(1, "emerald", "brick", 10, 16, 5));
    _registerTrade(Profession::Mason, 2, _simpleTrade("stone", 20, "emerald", 1, 16, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Mason, 3, _demandTrade(4, "emerald", "chiseled_stone_bricks", 4, 12, 10));
    _registerTrade(Profession::Mason, 3, _simpleTrade("gravel", 10, "emerald", 1, 12, 10));

    // 等级4 - 专家
    _registerTrade(Profession::Mason, 4, _demandTrade(1, "emerald", "polished_andesite", 4, 12, 15));
    _registerTrade(Profession::Mason, 4, _demandTrade(1, "emerald", "polished_granite", 4, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Mason, 5, _demandTrade(1, "emerald", "quartz_block", 1, 12, 30));
    _registerTrade(Profession::Mason, 5, _demandTrade(1, "emerald", "terracotta", 1, 12, 30));
}

// ============================================================================
// 牧羊人交易
// ============================================================================

void VillagerTrades::_registerShepherdTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Shepherd, 1, _demandTrade(18, "white_wool", "emerald", 1, 16, 2));
    _registerTrade(Profession::Shepherd, 1, _simpleTrade("black_wool", 18, "emerald", 1, 16, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Shepherd, 2, _simpleTrade("white_wool", 18, "emerald", 1, 16, 5));
    _registerTrade(Profession::Shepherd, 2, _demandTrade(1, "emerald", "shears", 1, 12, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Shepherd, 3, _demandTrade(15, "wheat", "emerald", 1, 16, 10));
    _registerTrade(Profession::Shepherd, 3, _demandTrade(2, "emerald", "white_bed", 1, 12, 10));

    // 等级4 - 专家
    _registerTrade(Profession::Shepherd, 4, _demandTrade(1, "emerald", "banner", 1, 12, 15));
    _registerTrade(Profession::Shepherd, 4, _demandTrade(1, "emerald", "painting", 1, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Shepherd, 5, _demandTrade(1, "emerald", "banner_pattern", 1, 12, 30));
}

// ============================================================================
// 工具匠交易
// ============================================================================

void VillagerTrades::_registerToolsmithTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Toolsmith, 1, _demandTrade(4, "cobblestone", "emerald", 1, 16, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Toolsmith, 2, _demandTrade(1, "emerald", "stone_axe", 1, 12, 5));
    _registerTrade(Profession::Toolsmith, 2, _demandTrade(1, "emerald", "stone_shovel", 1, 12, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Toolsmith, 3, _demandTrade(1, "emerald", "iron_pickaxe", 1, 12, 10));
    _registerTrade(Profession::Toolsmith, 3, _demandTrade(9, "flint", "emerald", 1, 12, 20));

    // 等级4 - 专家
    _registerTrade(Profession::Toolsmith, 4, _demandTrade(1, "emerald", "iron_axe", 1, 12, 15));
    _registerTrade(Profession::Toolsmith, 4, _demandTrade(1, "emerald", "iron_shovel", 1, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Toolsmith, 5, _demandTrade(12, "diamond", "diamond_pickaxe", 1, 12, 30));
    _registerTrade(Profession::Toolsmith, 5, _demandTrade(2, "emerald", "enchanted_iron_pickaxe", 1, 12, 30));
}

// ============================================================================
// 武器匠交易
// ============================================================================

void VillagerTrades::_registerWeaponsmithTrades()
{
    using Profession = entity::VillagerProfession;

    // 等级1 - 新手
    _registerTrade(Profession::Weaponsmith, 1, _demandTrade(15, "coal", "emerald", 1, 16, 2));

    // 等级2 - 学徒
    _registerTrade(Profession::Weaponsmith, 2, _demandTrade(4, "iron_ingot", "emerald", 1, 12, 10));
    _registerTrade(Profession::Weaponsmith, 2, _demandTrade(1, "emerald", "iron_sword", 1, 12, 5));

    // 等级3 - 老手
    _registerTrade(Profession::Weaponsmith, 3, _demandTrade(1, "emerald", "iron_axe", 1, 12, 10));
    _registerTrade(Profession::Weaponsmith, 3, _demandTrade(24, "flint", "emerald", 1, 16, 20));

    // 等级4 - 专家
    _registerTrade(Profession::Weaponsmith, 4, _demandTrade(1, "emerald", "diamond_sword", 1, 12, 15));
    _registerTrade(Profession::Weaponsmith, 4, _demandTrade(1, "emerald", "diamond_axe", 1, 12, 15));

    // 等级5 - 大师
    _registerTrade(Profession::Weaponsmith, 5, _demandTrade(2, "emerald", "enchanted_diamond_sword", 1, 12, 30));
    _registerTrade(Profession::Weaponsmith, 5, _demandTrade(12, "diamond", "enchanted_diamond_axe", 1, 12, 30));
}

// ============================================================================
// 傻子村民交易（无交易）
// ============================================================================

void VillagerTrades::_registerNitwitTrades()
{
    // 傻子村民没有任何交易
}

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
