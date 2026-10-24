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
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/items/DyeItemHelper.hpp"
#include "common/item/items/armor/DyeableArmorItem.hpp"
#include "common/item/items/special/EnchantedBookItem.hpp"
#include "common/item/potion/Potion.hpp"
#include "common/item/potion/PotionBrewing.hpp"
#include "common/item/potion/PotionRegistry.hpp"
#include "common/item/potion/PotionType.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyedItemColor.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"
#include "common/world/village/trade/TradeRandom.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace village {
namespace trade {

namespace {

/**
 * @brief 交易表共用的供应次数与经验常量
 *
 * 原版交易条目几乎都以这几个字面量构造，集中命名避免同类数字散落。
 */
constexpr i32 DEFAULT_SUPPLY = 12;
constexpr i32 COMMON_ITEMS_SUPPLY = 16;
constexpr i32 XP_LEVEL_1_SELL = 1;
constexpr i32 XP_LEVEL_1_BUY = 2;
constexpr i32 XP_LEVEL_2_SELL = 5;
constexpr i32 XP_LEVEL_2_BUY = 10;
constexpr i32 XP_LEVEL_3_SELL = 10;
constexpr i32 XP_LEVEL_3_BUY = 20;
constexpr i32 XP_LEVEL_4_SELL = 15;
constexpr i32 XP_LEVEL_4_BUY = 30;
constexpr i32 XP_LEVEL_5_TRADE = 30;
constexpr f32 LOW_TIER_PRICE_MULTIPLIER = 0.05f;
constexpr f32 HIGH_TIER_PRICE_MULTIPLIER = 0.2f;

/**
 * @brief 按资源位置取物品
 * @param id 物品资源位置
 * @return 物品指针；不存在返回 nullptr
 */
Item* _item(const char* id)
{
    return ItemRegistry::instance().getItem(ResourceLocation(id));
}

/**
 * @brief 以固定的已注册物品构造物品堆
 * @param item 物品静态指针（Items::XXX）
 * @param count 数量
 * @return 物品堆
 */
ItemStack _stackOf(Item* item, i32 count)
{
    MC_ASSERT_RELEASE(item != nullptr);
    return ItemStack(*item, count);
}

/**
 * @brief 构造"以物品换绿宝石"的支付代价（不含组件谓词）
 * @param item 物品（Items::XXX）
 * @param count 数量
 * @return 支付代价
 */
ItemCost _costOf(Item* item, i32 count)
{
    return ItemCost(_stackOf(item, count), count);
}

/**
 * @brief 构造绿宝石支付代价
 * @param count 数量
 * @return 支付代价
 */
ItemCost _emeralds(i32 count)
{
    const Item* emerald = _item("minecraft:emerald");
    MC_ASSERT_RELEASE(emerald != nullptr);
    return ItemCost(ItemStack(*emerald, count), count);
}

/**
 * @brief 交易上下文中的随机源访问
 *
 * 有世界时使用世界随机流（与村民 tick 共享，重放一致）；离线场景回退到由种子
 * 构造的独立随机树。每条交易条目生成前都会 fork 一次，互不干扰。
 */
class RandomSourceView {
public:
    /**
     * @brief 构造随机源视图
     * @param ctx 交易上下文
     */
    explicit RandomSourceView(const TradeContext& ctx)
        : m_ctx(ctx)
    {}

    /**
     * @brief 获取本次交易条目专属的随机流
     * @return 随机流（生命周期覆盖单次 getOffer 调用）
     */
    [[nodiscard]] TradeRandomSource& next()
    {
        if (m_ctx.world != nullptr) {
            return m_ctx.world->getRandom();
        }
        // 离线：以 (种子, 序号) 构造独立随机树，保证同一上下文的多次生成可复现且互不相同
        m_offline.emplace(m_ctx.traderSeed + m_ctx.sequence * 0x9E3779B97F4A7C15ULL + m_index++);
        return *m_offline;
    }

private:
    const TradeContext& m_ctx;
    std::optional<TradeRandom> m_offline;
    u64 m_index = 0;
};

/**
 * @brief 基础交易条目：单一支付 + 固定物品（vanilla EmeraldForItems / ItemsForEmeralds）
 */
class SimpleListing : public TradeListing {
public:
    /**
     * @brief 构造
     * @param costA 第一支付
     * @param sell 卖出物品模板
     * @param maxUses 最大使用次数
     * @param xp 经验
     * @param priceMultiplier 价格乘数
     */
    SimpleListing(ItemCost costA, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier)
        : m_costA(std::move(costA))
        , m_sell(std::move(sell))
        , m_maxUses(maxUses)
        , m_xp(xp)
        , m_priceMultiplier(priceMultiplier)
    {}

    [[nodiscard]] std::unique_ptr<MerchantOffer> getOffer(const TradeContext&, TradeRandomSource&) const override
    {
        return std::make_unique<MerchantOffer>(m_costA, m_sell.copy(), m_maxUses, m_xp, m_priceMultiplier);
    }

private:
    ItemCost m_costA;
    ItemStack m_sell;
    i32 m_maxUses;
    i32 m_xp;
    f32 m_priceMultiplier;
};

/**
 * @brief 双支付条目：绿宝石 + 另一种物品换成品（vanilla ItemsAndEmeraldsToItems）
 */
class ItemsAndEmeraldsListing : public TradeListing {
public:
    /**
     * @brief 构造
     * @param emeraldCost 绿宝石数量
     * @param fromItem 第二支付物品（可选）
     * @param toItem 卖出物品模板
     * @param maxUses 最大使用次数
     * @param xp 经验
     * @param priceMultiplier 价格乘数
     */
    ItemsAndEmeraldsListing(
        i32 emeraldCost, std::optional<ItemCost> fromItem, ItemStack toItem, i32 maxUses, i32 xp, f32 priceMultiplier)
        : m_emeraldCost(emeraldCost)
        , m_fromItem(std::move(fromItem))
        , m_toItem(std::move(toItem))
        , m_maxUses(maxUses)
        , m_xp(xp)
        , m_priceMultiplier(priceMultiplier)
    {}

    [[nodiscard]] std::unique_ptr<MerchantOffer> getOffer(const TradeContext&, TradeRandomSource&) const override
    {
        return std::make_unique<MerchantOffer>(
            _emeralds(m_emeraldCost), m_fromItem, m_toItem.copy(), m_maxUses, m_xp, m_priceMultiplier);
    }

private:
    i32 m_emeraldCost;
    std::optional<ItemCost> m_fromItem;
    ItemStack m_toItem;
    i32 m_maxUses;
    i32 m_xp;
    f32 m_priceMultiplier;
};

/**
 * @brief 按村民类型分支的条目（vanilla TypeSpecificTrade）
 *
 * 用于"同一等级在不同生物群系卖不同物品"的场景（职业为制图师的地图、渔夫的船等）。
 * 村民类型不匹配或无类型时返回空交易。
 */
class TypeSpecificListing : public TradeListing {
public:
    /**
     * @brief 构造
     * @param listings 村民类型 → 条目
     */
    explicit TypeSpecificListing(std::unordered_map<entity::VillagerType, std::shared_ptr<TradeListing>> listings)
        : m_listings(std::move(listings))
    {}

    [[nodiscard]] std::unique_ptr<MerchantOffer> getOffer(
        const TradeContext& ctx, TradeRandomSource& random) const override
    {
        const auto type = tradeVillagerType(ctx);
        if (!type.has_value()) {
            return nullptr;
        }
        auto found = m_listings.find(*type);
        if (found == m_listings.end()) {
            return nullptr;
        }
        return found->second->getOffer(ctx, random);
    }

private:
    std::unordered_map<entity::VillagerType, std::shared_ptr<TradeListing>> m_listings;
};

/**
 * @brief 出售附魔物品的条目（vanilla EnchantedItemForEmeralds）
 *
 * 附魔等级 = 5 + random(15)；绿宝石价格 = min(基础价格 + 附魔等级, 64)。
 */
class EnchantedItemListing : public TradeListing {
public:
    /**
     * @brief 构造
     * @param item 物品
     * @param baseEmeraldCost 基础绿宝石价格
     * @param maxUses 最大使用次数
     * @param xp 经验
     * @param priceMultiplier 价格乘数
     */
    EnchantedItemListing(Item* item, i32 baseEmeraldCost, i32 maxUses, i32 xp, f32 priceMultiplier)
        : m_item(item)
        , m_baseEmeraldCost(baseEmeraldCost)
        , m_maxUses(maxUses)
        , m_xp(xp)
        , m_priceMultiplier(priceMultiplier)
    {}

    [[nodiscard]] std::unique_ptr<MerchantOffer> getOffer(const TradeContext&, TradeRandomSource& random) const override
    {
        const i32 level = 5 + random.nextInt(15);
        ItemStack stack = _stackOf(m_item, 1);
        stack = item::enchant::EnchantmentHelper::addRandomEnchantment(random, stack, level, false);
        const i32 cost = std::min(m_baseEmeraldCost + level, 64);
        return std::make_unique<MerchantOffer>(_emeralds(cost), stack, m_maxUses, m_xp, m_priceMultiplier);
    }

private:
    Item* m_item;
    i32 m_baseEmeraldCost;
    i32 m_maxUses;
    i32 m_xp;
    f32 m_priceMultiplier;
};

/**
 * @brief 出售附魔书（绿宝石 + 书）的条目（vanilla EnchantBookForEmeralds）
 *
 * 从可交易附魔中随机抽取一本，等级落在 [max(minLevel, 附魔最低级), min(maxLevel, 附魔最高级)]；
 * 价格 = 2 + random(5 + 10 * 等级) + 3 * 等级，双倍价格附魔再翻倍，上限 64。
 */
class EnchantBookListing : public TradeListing {
public:
    /**
     * @brief 构造
     * @param xp 经验
     */
    explicit EnchantBookListing(i32 xp)
        : m_xp(xp)
    {}

    [[nodiscard]] std::unique_ptr<MerchantOffer> getOffer(const TradeContext&, TradeRandomSource& random) const override
    {
        // 收集所有允许出现在交易中的附魔（对齐 TRADEABLE 标签：宝藏附魔不出现）
        std::vector<const item::enchant::Enchantment*> candidates;
        for (const auto& [id, enchantment] : item::enchant::EnchantmentRegistry::all()) {
            if (enchantment != nullptr && enchantment->canVillagerTrade()) {
                candidates.push_back(enchantment.get());
            }
        }

        const Item* book = _item("minecraft:book");
        const Item* emerald = _item("minecraft:emerald");
        MC_ASSERT_RELEASE(book != nullptr && emerald != nullptr);

        ItemStack itemStack;
        i32 price = 1;
        if (!candidates.empty()) {
            const auto& enchantment =
                *candidates[static_cast<size_t>(random.nextInt(static_cast<i32>(candidates.size())))];
            const i32 minLevel = enchantment.minLevel();
            const i32 maxLevel = enchantment.maxLevel();
            const i32 level = minLevel >= maxLevel ? minLevel : random.nextInt(minLevel, maxLevel);

            itemStack = ItemStack(*_item("minecraft:enchanted_book"), 1);
            item::items::EnchantedBookItem::addEnchantment(itemStack, enchantment, level);

            price = 2 + random.nextInt(5 + level * 10) + 3 * level;
            // 双倍价格：交易表中的该类附魔（如经验修补）翻倍
            if (enchantment.canVillagerTrade() && enchantment.isTreasure()) {
                price *= 2;
            }
            price = std::min(price, 64);
        } else {
            itemStack = ItemStack(*book, 1);
        }

        return std::make_unique<MerchantOffer>(
            _emeralds(price), ItemCost(ItemStack(*book, 1)), itemStack, 12, m_xp, HIGH_TIER_PRICE_MULTIPLIER);
    }

private:
    i32 m_xp;
};

/**
 * @brief 出售随机染色护甲的条目（vanilla DyedArmorForEmeralds）
 *
 * 颜色由 1~3 种随机染料的纹理色混合，概率分别为 1、0.3、0.2。
 */
class DyedArmorListing : public TradeListing {
public:
    /**
     * @brief 构造
     * @param item 护甲物品
     * @param emeraldCost 绿宝石价格
     * @param maxUses 最大使用次数
     * @param xp 经验
     */
    DyedArmorListing(Item* item, i32 emeraldCost, i32 maxUses, i32 xp)
        : m_item(item)
        , m_emeraldCost(emeraldCost)
        , m_maxUses(maxUses)
        , m_xp(xp)
    {}

    [[nodiscard]] std::unique_ptr<MerchantOffer> getOffer(const TradeContext&, TradeRandomSource& random) const override
    {
        ItemStack stack = _stackOf(m_item, 1);
        const Item* item = stack.getItem();
        if (item != nullptr && item::tag::ItemTags::DYEABLE().contains(item)) {
            std::vector<u32> colors;
            colors.push_back(_randomDyeColor(random));
            if (random.nextFloat() > 0.7f) {
                colors.push_back(_randomDyeColor(random));
            }
            if (random.nextFloat() > 0.8f) {
                colors.push_back(_randomDyeColor(random));
            }
            const u32 mixed = DyedItemColor::applyDyes(std::nullopt, colors);
            item::items::DyeableArmorItem::setColor(stack, mixed);
        }

        return std::make_unique<MerchantOffer>(
            _emeralds(m_emeraldCost), stack, m_maxUses, m_xp, HIGH_TIER_PRICE_MULTIPLIER);
    }

private:
    /**
     * @brief 随机取一种染料的纹理漫反射色
     * @param random 随机源
     * @return 0xRRGGBB 颜色
     */
    static u32 _randomDyeColor(TradeRandomSource& random)
    {
        const auto color = static_cast<DyeColor>(random.nextInt(static_cast<i32>(DyeColor::Count)));
        return dyeTextureDiffuseColor(color);
    }

    Item* m_item;
    i32 m_emeraldCost;
    i32 m_maxUses;
    i32 m_xp;
};

/**
 * @brief 迷之炖菜条目（vanilla SuspiciousStewForEmerald）
 *
 * 写 suspicious_stew_effects 组件，饮用时生效。
 */
class SuspiciousStewListing : public TradeListing {
public:
    /**
     * @brief 构造
     * @param effectType 效果类型
     * @param duration 持续时间（tick）
     * @param xp 经验
     */
    SuspiciousStewListing(entity::effect::EffectType effectType, i32 duration, i32 xp)
        : m_effectType(effectType)
        , m_duration(duration)
        , m_xp(xp)
    {}

    [[nodiscard]] std::unique_ptr<MerchantOffer> getOffer(const TradeContext&, TradeRandomSource&) const override
    {
        ItemStack stack(*_item("minecraft:suspicious_stew"), 1);
        item::component::SuspiciousStewEffectsPayload effects;
        effects.emplace_back(m_effectType, m_duration);
        stack.setStewEffects(std::move(effects));
        return std::make_unique<MerchantOffer>(_emeralds(1), stack, 12, m_xp, LOW_TIER_PRICE_MULTIPLIER);
    }

private:
    entity::effect::EffectType m_effectType;
    i32 m_duration;
    i32 m_xp;
};

/**
 * @brief 地图装饰类型（vanilla MapDecorationTypes）
 */
struct MapDecoration {
    const char* id;
    /// 地图名称翻译键（写入 item_name）
    const char* nameKey;
};

/**
 * @brief 寻宝地图条目（vanilla TreasureMapForEmeralds）
 *
 * 在世界中搜索指定结构标签的最近结构；找不到时返回空交易。
 */
class TreasureMapListing : public TradeListing {
public:
    /**
     * @brief 构造
     * @param emeraldCost 绿宝石价格
     * @param structureTag 目标结构标签
     * @param displayNameKey 地图名称翻译键
     * @param decoration 地图装饰类型
     * @param maxUses 最大使用次数
     * @param xp 经验
     */
    TreasureMapListing(i32 emeraldCost,
        ResourceLocation structureTag,
        const char* displayNameKey,
        MapDecoration decoration,
        i32 maxUses,
        i32 xp)
        : m_emeraldCost(emeraldCost)
        , m_structureTag(std::move(structureTag))
        , m_displayNameKey(displayNameKey)
        , m_decoration(decoration)
        , m_maxUses(maxUses)
        , m_xp(xp)
    {}

    [[nodiscard]] std::unique_ptr<MerchantOffer> getOffer(const TradeContext& ctx, TradeRandomSource&) const override
    {
        if (ctx.world == nullptr || ctx.trader == nullptr) {
            return nullptr;
        }

        const auto traderPos = BlockPos(static_cast<i32>(std::floor(ctx.trader->x())),
            static_cast<i32>(std::floor(ctx.trader->y())),
            static_cast<i32>(std::floor(ctx.trader->z())));
        const auto target = ctx.world->findNearestMapStructure(traderPos, m_structureTag, 100, true);
        if (!target.has_value()) {
            return nullptr;
        }

        // 原版用 MapItem.create(level, x, z, scale=2, trackingPosition=true, unlimitedTracking=true)，
        // 本项目的填图物品接口在套件重建过程中，TODO: 接入地图创建与装饰标记后返回真正的探险家地图，
        // 在此之前返回空交易，避免卖给玩家一张普通空白地图。
        (void)m_decoration;
        spdlog::warn("Treasure map trade for structure tag {} skipped: map creation is not available yet",
            m_structureTag.toString());
        return nullptr;
    }

private:
    i32 m_emeraldCost;
    ResourceLocation m_structureTag;
    const char* m_displayNameKey;
    MapDecoration m_decoration;
    i32 m_maxUses;
    i32 m_xp;
};

/**
 * @brief 出售药箭的条目（vanilla TippedArrowForItemsAndEmeralds）
 *
 * 从"有药水效果且可通过酿造获得"的药水中随机挑选一种，写入 potion_contents 组件；
 * 价格 = 绿宝石 + 原版箭矢数量。
 */
class TippedArrowListing : public TradeListing {
public:
    /**
     * @brief 构造
     * @param fromItem 第一支付物品（普通箭）
     * @param fromCount 第一支付数量
     * @param toItem 卖出物品（药箭）
     * @param toCount 卖出数量
     * @param emeraldCost 绿宝石价格
     * @param maxUses 最大使用次数
     * @param xp 经验
     */
    TippedArrowListing(Item* fromItem, i32 fromCount, Item* toItem, i32 toCount, i32 emeraldCost, i32 maxUses, i32 xp)
        : m_fromItem(fromItem)
        , m_fromCount(fromCount)
        , m_toItem(toItem)
        , m_toCount(toCount)
        , m_emeraldCost(emeraldCost)
        , m_maxUses(maxUses)
        , m_xp(xp)
    {}

    [[nodiscard]] std::unique_ptr<MerchantOffer> getOffer(const TradeContext&, TradeRandomSource& random) const override
    {
        // 收集"有药水效果且可酿造"的药水（原版 potionBrewing().isBrewablePotion 过滤）
        std::vector<const potion::Potion*> candidates;
        for (const auto& [id, candidate] : potion::PotionRegistry::instance().getAllPotions()) {
            if (candidate != nullptr && candidate->hasEffects() && potion::PotionBrewing::isBrewablePotion(candidate)) {
                candidates.push_back(candidate);
            }
        }
        if (candidates.empty()) {
            return nullptr;
        }

        const auto* potion = candidates[static_cast<size_t>(random.nextInt(static_cast<i32>(candidates.size())))];
        ItemStack stack = _stackOf(m_toItem, m_toCount);
        potion::PotionUtils::setPotion(stack, potion);

        return std::make_unique<MerchantOffer>(_emeralds(m_emeraldCost),
            ItemCost(_stackOf(m_fromItem, m_fromCount)),
            stack,
            m_maxUses,
            m_xp,
            LOW_TIER_PRICE_MULTIPLIER);
    }

private:
    Item* m_fromItem;
    i32 m_fromCount;
    Item* m_toItem;
    i32 m_toCount;
    i32 m_emeraldCost;
    i32 m_maxUses;
    i32 m_xp;
};

/**
 * @brief 商店与旅行商人共用的条目构造辅助
 */
class Entry {
public:
    /**
     * @brief 以物品换绿宝石（player → villager）
     * @param item 物品
     * @param count 数量
     * @param maxUses 最大使用次数
     * @param xp 经验
     * @return 条目
     */
    static std::unique_ptr<TradeListing> emeraldForItems(Item* item, i32 count, i32 maxUses, i32 xp)
    {
        return std::make_unique<SimpleListing>(
            _costOf(item, count), _stackOf(_item("minecraft:emerald"), 1), maxUses, xp, LOW_TIER_PRICE_MULTIPLIER);
    }

    /**
     * @brief 以绿宝石买物品（villager → player）
     * @param sellItem 卖出物品
     * @param sellCount 卖出数量
     * @param emeraldCost 绿宝石价格
     * @param maxUses 最大使用次数
     * @param xp 经验
     * @return 条目
     */
    static std::unique_ptr<TradeListing> itemsForEmeralds(
        Item* sellItem, i32 sellCount, i32 emeraldCost, i32 maxUses, i32 xp)
    {
        return std::make_unique<SimpleListing>(
            _emeralds(emeraldCost), _stackOf(sellItem, sellCount), maxUses, xp, LOW_TIER_PRICE_MULTIPLIER);
    }

    /**
     * @brief 以绿宝石买物品（带自定义价格乘数）
     */
    static std::unique_ptr<TradeListing> itemsForEmeralds(
        Item* sellItem, i32 sellCount, i32 emeraldCost, i32 maxUses, i32 xp, f32 priceMultiplier)
    {
        return std::make_unique<SimpleListing>(
            _emeralds(emeraldCost), _stackOf(sellItem, sellCount), maxUses, xp, priceMultiplier);
    }

    /**
     * @brief 以绿宝石 + 另一物品换成品
     */
    static std::unique_ptr<TradeListing> itemsAndEmeraldsToItems(Item* fromItem,
        i32 fromCount,
        i32 emeraldCost,
        Item* toItem,
        i32 toCount,
        i32 maxUses,
        i32 xp,
        f32 priceMultiplier)
    {
        return std::make_unique<ItemsAndEmeraldsListing>(
            emeraldCost, _costOf(fromItem, fromCount), _stackOf(toItem, toCount), maxUses, xp, priceMultiplier);
    }

    /**
     * @brief 以物品换多个绿宝石（原版 EmeraldForItems 的自定义绿宝石数量重载）
     * @param item 支付物品
     * @param count 支付数量
     * @param emeraldCount 获得绿宝石数量
     * @param maxUses 最大使用次数
     * @param xp 经验
     * @return 条目
     */
    static std::unique_ptr<TradeListing> emeraldsForItems(Item* item, i32 count, i32 emeraldCount, i32 maxUses, i32 xp)
    {
        return std::make_unique<SimpleListing>(_costOf(item, count),
            _stackOf(_item("minecraft:emerald"), emeraldCount),
            maxUses,
            xp,
            LOW_TIER_PRICE_MULTIPLIER);
    }

    /**
     * @brief 以绿宝石换指定物品（制图师地图等需要组件版本）
     * @param cost 第一支付代价
     * @param sell 卖出物品
     * @param maxUses 最大使用次数
     * @param xp 经验
     * @param priceMultiplier 价格乘数
     * @return 条目
     */
    static std::unique_ptr<TradeListing> simple(ItemCost cost, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier)
    {
        return std::make_unique<SimpleListing>(std::move(cost), std::move(sell), maxUses, xp, priceMultiplier);
    }

    /**
     * @brief 以绿宝石 + 另一物品换成品（无第二支付的物品出售）
     * @param cost 第一支付代价
     * @param sell 卖出物品
     * @param maxUses 最大使用次数
     * @param xp 经验
     * @param priceMultiplier 价格乘数
     * @return 条目
     */
    static std::unique_ptr<TradeListing> pricedEmeralds(
        i32 emeraldCost, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier)
    {
        return std::make_unique<SimpleListing>(_emeralds(emeraldCost), std::move(sell), maxUses, xp, priceMultiplier);
    }
};

} // namespace

/**
 * @brief 物品换绿宝石条目（供流浪商人交易表复用）
 */
std::unique_ptr<TradeListing> makeEmeraldForItems(Item* item, i32 count, i32 maxUses, i32 xp)
{
    return Entry::emeraldForItems(item, count, maxUses, xp);
}

/**
 * @brief 绿宝石换物品条目（供流浪商人交易表复用）
 */
std::unique_ptr<TradeListing> makeItemsForEmeralds(
    ItemCost cost, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier)
{
    return Entry::simple(std::move(cost), std::move(sell), maxUses, xp, priceMultiplier);
}

/**
 * @brief 绿宝石 + 物品换成品条目（供流浪商人交易表复用）
 */
std::unique_ptr<TradeListing> makeItemsAndEmeraldsToItems(
    Item* fromItem, i32 fromCount, i32 emeraldCost, Item* toItem, i32 toCount, i32 maxUses, i32 xp, f32 priceMultiplier)
{
    return Entry::itemsAndEmeraldsToItems(
        fromItem, fromCount, emeraldCost, toItem, toCount, maxUses, xp, priceMultiplier);
}

/**
 * @brief 出售附魔物品条目（供流浪商人交易表复用）
 */
std::unique_ptr<TradeListing> makeEnchantedItem(
    Item* item, i32 baseEmeraldCost, i32 maxUses, i32 xp, f32 priceMultiplier)
{
    return std::make_unique<EnchantedItemListing>(item, baseEmeraldCost, maxUses, xp, priceMultiplier);
}

/**
 * @brief 以资源位置构造物品堆（供流浪商人交易表使用不存在静态指针的物品）
 */
ItemStack makeStack(const char* itemId, i32 count)
{
    return _stackOf(const_cast<Item*>(_item(itemId)), count);
}

// ============================================================================
// 静态成员
// ============================================================================

std::unordered_map<entity::VillagerProfession, VillagerTrades::LevelTrades, std::hash<entity::VillagerProfession>>
    VillagerTrades::s_trades;
bool VillagerTrades::s_initialized = false;

// ============================================================================
// 初始化与查询
// ============================================================================

void VillagerTrades::initialize()
{
    if (s_initialized) {
        return;
    }

    _registerFarmerTrades();
    _registerFishermanTrades();
    _registerShepherdTrades();
    _registerFletcherTrades();
    _registerLibrarianTrades();
    _registerCartographerTrades();
    _registerClericTrades();
    _registerArmorerTrades();
    _registerWeaponsmithTrades();
    _registerToolsmithTrades();
    _registerButcherTrades();
    _registerLeatherworkerTrades();
    _registerMasonTrades();

    s_initialized = true;
}

VillagerTrades::LevelTrades& VillagerTrades::_trades(entity::VillagerProfession profession)
{
    return s_trades[profession];
}

void VillagerTrades::_add(entity::VillagerProfession profession, i32 level, std::unique_ptr<TradeListing> listing)
{
    MC_ASSERT_RELEASE(level >= LEVEL_OFFSET && level < MAX_LEVEL + LEVEL_OFFSET);
    _trades(profession)[static_cast<size_t>(level)].push_back(std::move(listing));
}

std::unique_ptr<MerchantOffers> VillagerTrades::generateOffers(
    entity::VillagerProfession profession, i32 level, const TradeContext& ctx)
{
    auto offers = std::make_unique<MerchantOffers>();

    if (!s_initialized) {
        initialize();
    }

    auto found = s_trades.find(profession);
    if (found == s_trades.end()) {
        return offers;
    }
    if (level < LEVEL_OFFSET || level >= MAX_LEVEL + LEVEL_OFFSET) {
        return offers;
    }

    RandomSourceView randomView(ctx);
    for (const auto& listing : found->second[static_cast<size_t>(level)]) {
        auto offer = listing->getOffer(ctx, randomView.next());
        if (offer != nullptr) {
            offers->addOffer(std::move(offer));
        }
    }

    return offers;
}

bool VillagerTrades::hasTrades(entity::VillagerProfession profession)
{
    if (!s_initialized) {
        initialize();
    }
    auto found = s_trades.find(profession);
    if (found == s_trades.end()) {
        return false;
    }
    for (const auto& levelList : found->second) {
        if (!levelList.empty()) {
            return true;
        }
    }
    return false;
}

i32 VillagerTrades::getTradeLevelCount(entity::VillagerProfession profession)
{
    if (!s_initialized) {
        initialize();
    }
    auto found = s_trades.find(profession);
    if (found == s_trades.end()) {
        return 0;
    }
    i32 count = 0;
    for (const auto& levelList : found->second) {
        if (!levelList.empty()) {
            ++count;
        }
    }
    return count;
}

// ============================================================================
// 农民（vanilla VillagerProfession.FARMER）
// ============================================================================

void VillagerTrades::_registerFarmerTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Farmer, 1, Entry::emeraldForItems(Items::WHEAT, 20, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Farmer, 1, Entry::emeraldForItems(Items::POTATO, 26, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Farmer, 1, Entry::emeraldForItems(Items::CARROT, 22, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Farmer, 1, Entry::emeraldForItems(Items::BEETROOT, 15, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Farmer, 1, Entry::itemsForEmeralds(Items::BREAD, 6, 1, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_SELL));

    _add(P::Farmer, 2, Entry::emeraldForItems(Items::PUMPKIN, 6, DEFAULT_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Farmer, 2, Entry::itemsForEmeralds(Items::PUMPKIN_PIE, 4, 1, DEFAULT_SUPPLY, XP_LEVEL_2_SELL));
    _add(P::Farmer, 2, Entry::itemsForEmeralds(Items::APPLE, 4, 1, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_SELL));

    _add(P::Farmer, 3, Entry::itemsForEmeralds(Items::COOKIE, 18, 3, DEFAULT_SUPPLY, XP_LEVEL_3_SELL));
    _add(P::Farmer, 3, Entry::emeraldForItems(Items::MELON, 4, DEFAULT_SUPPLY, XP_LEVEL_3_BUY));

    _add(P::Farmer, 4, Entry::itemsForEmeralds(Items::CAKE, 1, 1, DEFAULT_SUPPLY, XP_LEVEL_4_SELL));
    _addStew(P::Farmer, 4, entity::effect::EffectType::NightVision, 100);
    _addStew(P::Farmer, 4, entity::effect::EffectType::JumpBoost, 160);
    _addStew(P::Farmer, 4, entity::effect::EffectType::Weakness, 140);
    _addStew(P::Farmer, 4, entity::effect::EffectType::Blindness, 120);
    _addStew(P::Farmer, 4, entity::effect::EffectType::Poison, 280);
    _addStew(P::Farmer, 4, entity::effect::EffectType::Saturation, 7);

    _add(P::Farmer, 5, Entry::itemsForEmeralds(Items::GOLDEN_CARROT, 3, 3, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
    _add(P::Farmer, 5, Entry::itemsForEmeralds(Items::GLISTERING_MELON_SLICE, 3, 4, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
}

// ============================================================================
// 渔夫（vanilla VillagerProfession.FISHERMAN）
// ============================================================================

void VillagerTrades::_registerFishermanTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Fisherman, 1, Entry::emeraldForItems(Items::STRING, 20, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Fisherman, 1, Entry::emeraldForItems(Items::COAL, 10, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Fisherman,
        1,
        std::make_unique<ItemsAndEmeraldsListing>(1,
            _costOf(Items::COD, 6),
            _stackOf(Items::COOKED_COD, 6),
            COMMON_ITEMS_SUPPLY,
            XP_LEVEL_1_SELL,
            LOW_TIER_PRICE_MULTIPLIER));
    _add(P::Fisherman, 1, Entry::itemsForEmeralds(Items::COD_BUCKET, 1, 3, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_SELL));

    _add(P::Fisherman, 2, Entry::emeraldForItems(Items::COD, 15, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Fisherman,
        2,
        std::make_unique<ItemsAndEmeraldsListing>(1,
            _costOf(Items::SALMON, 6),
            _stackOf(Items::COOKED_SALMON, 6),
            COMMON_ITEMS_SUPPLY,
            XP_LEVEL_2_SELL,
            LOW_TIER_PRICE_MULTIPLIER));
    _add(P::Fisherman, 2, Entry::itemsForEmeralds(Items::CAMPFIRE, 1, 2, DEFAULT_SUPPLY, XP_LEVEL_2_SELL));

    _add(P::Fisherman, 3, Entry::emeraldForItems(Items::SALMON, 13, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Fisherman,
        3,
        std::make_unique<EnchantedItemListing>(Items::FISHING_ROD, 3, 3, XP_LEVEL_3_SELL, HIGH_TIER_PRICE_MULTIPLIER));

    _add(P::Fisherman, 4, Entry::emeraldForItems(Items::TROPICAL_FISH, 6, DEFAULT_SUPPLY, XP_LEVEL_4_BUY));

    _add(P::Fisherman, 5, Entry::emeraldForItems(Items::PUFFERFISH, 4, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
    {
        // 船只按村民类型分支（原版 EmeraldsForVillagerTypeItem）
        std::unordered_map<entity::VillagerType, std::shared_ptr<TradeListing>> boats;
        boats.emplace(
            entity::VillagerType::Plains, Entry::emeraldForItems(Items::OAK_BOAT, 1, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
        boats.emplace(entity::VillagerType::Taiga,
            Entry::emeraldForItems(Items::SPRUCE_BOAT, 1, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
        boats.emplace(entity::VillagerType::Snow,
            Entry::emeraldForItems(Items::SPRUCE_BOAT, 1, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
        boats.emplace(entity::VillagerType::Desert,
            Entry::emeraldForItems(Items::JUNGLE_BOAT, 1, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
        boats.emplace(entity::VillagerType::Jungle,
            Entry::emeraldForItems(Items::JUNGLE_BOAT, 1, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
        boats.emplace(entity::VillagerType::Savanna,
            Entry::emeraldForItems(Items::ACACIA_BOAT, 1, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
        boats.emplace(entity::VillagerType::Swamp,
            Entry::emeraldForItems(Items::DARK_OAK_BOAT, 1, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
        _add(P::Fisherman, 5, std::make_unique<TypeSpecificListing>(std::move(boats)));
    }
}

// ============================================================================
// 牧羊人（vanilla VillagerProfession.SHEPHERD）
// ============================================================================

void VillagerTrades::_registerShepherdTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Shepherd, 1, Entry::emeraldForItems(Items::WHITE_WOOL, 18, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Shepherd, 1, Entry::emeraldForItems(Items::BROWN_WOOL, 18, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Shepherd, 1, Entry::emeraldForItems(Items::BLACK_WOOL, 18, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Shepherd, 1, Entry::emeraldForItems(Items::GRAY_WOOL, 18, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Shepherd, 1, Entry::itemsForEmeralds(Items::SHEARS, 1, 2, 1, XP_LEVEL_1_SELL));

    _add(P::Shepherd, 2, Entry::emeraldForItems(Items::WHITE_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Shepherd, 2, Entry::emeraldForItems(Items::GRAY_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Shepherd, 2, Entry::emeraldForItems(Items::BLACK_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Shepherd, 2, Entry::emeraldForItems(Items::LIGHT_BLUE_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Shepherd, 2, Entry::emeraldForItems(Items::LIME_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_BUY));
    _addWools(P::Shepherd, 2, 1, 1, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_SELL);
    _addCarpets(P::Shepherd, 2, 1, 4, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_SELL);

    _add(P::Shepherd, 3, Entry::emeraldForItems(Items::YELLOW_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Shepherd, 3, Entry::emeraldForItems(Items::LIGHT_GRAY_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Shepherd, 3, Entry::emeraldForItems(Items::ORANGE_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Shepherd, 3, Entry::emeraldForItems(Items::RED_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Shepherd, 3, Entry::emeraldForItems(Items::PINK_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));
    _addBeds(P::Shepherd, 3, 3, DEFAULT_SUPPLY, XP_LEVEL_3_SELL);

    _add(P::Shepherd, 4, Entry::emeraldForItems(Items::BROWN_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Shepherd, 4, Entry::emeraldForItems(Items::PURPLE_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Shepherd, 4, Entry::emeraldForItems(Items::BLUE_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Shepherd, 4, Entry::emeraldForItems(Items::GREEN_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Shepherd, 4, Entry::emeraldForItems(Items::MAGENTA_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Shepherd, 4, Entry::emeraldForItems(Items::CYAN_DYE, 12, COMMON_ITEMS_SUPPLY, XP_LEVEL_4_BUY));
    _addBanners(P::Shepherd, 4, 3, DEFAULT_SUPPLY, XP_LEVEL_4_SELL);

    _add(P::Shepherd, 5, Entry::itemsForEmeralds(Items::PAINTING, 3, 2, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
}

// ============================================================================
// 制箭师（vanilla VillagerProfession.FLETCHER）
// ============================================================================

void VillagerTrades::_registerFletcherTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Fletcher, 1, Entry::emeraldForItems(Items::STICK, 32, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Fletcher, 1, Entry::itemsForEmeralds(Items::ARROW, 16, 1, DEFAULT_SUPPLY, XP_LEVEL_1_SELL));
    _add(P::Fletcher,
        1,
        std::make_unique<ItemsAndEmeraldsListing>(1,
            _costOf(Items::GRAVEL, 10),
            _stackOf(Items::FLINT, 10),
            DEFAULT_SUPPLY,
            XP_LEVEL_1_SELL,
            LOW_TIER_PRICE_MULTIPLIER));

    _add(P::Fletcher, 2, Entry::emeraldForItems(Items::FLINT, 26, DEFAULT_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Fletcher, 2, Entry::itemsForEmeralds(Items::BOW, 1, 2, DEFAULT_SUPPLY, XP_LEVEL_2_SELL));

    _add(P::Fletcher, 3, Entry::emeraldForItems(Items::STRING, 14, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Fletcher, 3, Entry::itemsForEmeralds(Items::CROSSBOW, 1, 3, DEFAULT_SUPPLY, XP_LEVEL_3_SELL));

    _add(P::Fletcher, 4, Entry::emeraldForItems(Items::FEATHER, 24, COMMON_ITEMS_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Fletcher,
        4,
        std::make_unique<EnchantedItemListing>(Items::BOW, 2, 3, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER));

    _add(P::Fletcher, 5, Entry::emeraldForItems(Items::TRIPWIRE_HOOK, 8, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
    _add(P::Fletcher,
        5,
        std::make_unique<EnchantedItemListing>(Items::CROSSBOW, 3, 3, XP_LEVEL_5_TRADE, LOW_TIER_PRICE_MULTIPLIER));
    _add(P::Fletcher,
        5,
        std::make_unique<TippedArrowListing>(
            Items::ARROW, 5, Items::TIPPED_ARROW, 5, 2, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
}

// ============================================================================
// 图书管理员（vanilla VillagerProfession.LIBRARIAN）
// ============================================================================

void VillagerTrades::_registerLibrarianTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Librarian, 1, Entry::emeraldForItems(Items::PAPER, 24, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Librarian, 1, std::make_unique<EnchantBookListing>(XP_LEVEL_1_SELL));
    _add(P::Librarian, 1, Entry::itemsForEmeralds(Items::BOOKSHELF, 1, 9, DEFAULT_SUPPLY, XP_LEVEL_1_SELL));

    _add(P::Librarian, 2, Entry::emeraldForItems(Items::BOOK, 4, DEFAULT_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Librarian, 2, std::make_unique<EnchantBookListing>(XP_LEVEL_2_SELL));
    _add(P::Librarian, 2, Entry::itemsForEmeralds(Items::LANTERN, 1, 1, DEFAULT_SUPPLY, XP_LEVEL_2_SELL));

    _add(P::Librarian, 3, Entry::emeraldForItems(Items::INK_SAC, 5, DEFAULT_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Librarian, 3, std::make_unique<EnchantBookListing>(XP_LEVEL_3_SELL));
    _add(P::Librarian, 3, Entry::itemsForEmeralds(Items::GLASS, 4, 1, DEFAULT_SUPPLY, XP_LEVEL_3_SELL));

    _add(P::Librarian, 4, Entry::emeraldForItems(Items::WRITABLE_BOOK, 2, DEFAULT_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Librarian, 4, std::make_unique<EnchantBookListing>(XP_LEVEL_4_SELL));
    _add(P::Librarian, 4, Entry::itemsForEmeralds(Items::CLOCK, 1, 5, DEFAULT_SUPPLY, XP_LEVEL_4_SELL));
    _add(P::Librarian, 4, Entry::itemsForEmeralds(Items::COMPASS, 1, 4, DEFAULT_SUPPLY, XP_LEVEL_4_SELL));

    _add(P::Librarian, 5, Entry::itemsForEmeralds(Items::NAME_TAG, 1, 20, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
}

// ============================================================================
// 制图师（vanilla VillagerProfession.CARTOGRAPHER）
// ============================================================================

void VillagerTrades::_registerCartographerTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Cartographer, 1, Entry::emeraldForItems(Items::PAPER, 24, DEFAULT_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Cartographer,
        1,
        Entry::pricedEmeralds(7, _stackOf(Items::MAP, 1), DEFAULT_SUPPLY, XP_LEVEL_1_SELL, LOW_TIER_PRICE_MULTIPLIER));

    _add(P::Cartographer, 2, Entry::emeraldForItems(Items::GLASS_PANE, 11, DEFAULT_SUPPLY, XP_LEVEL_2_BUY));
    _addTreasureMap(P::Cartographer,
        2,
        8,
        "minecraft:on_taiga_village_maps",
        "filled_map.village_taiga",
        "minecraft:taiga_village",
        XP_LEVEL_2_SELL,
        {entity::VillagerType::Swamp, entity::VillagerType::Snow, entity::VillagerType::Plains});
    _addTreasureMap(P::Cartographer,
        2,
        8,
        "minecraft:on_swamp_explorer_maps",
        "filled_map.explorer_swamp",
        "minecraft:swamp_hut",
        XP_LEVEL_2_SELL,
        {entity::VillagerType::Taiga, entity::VillagerType::Snow, entity::VillagerType::Jungle});
    _addTreasureMap(P::Cartographer,
        2,
        8,
        "minecraft:on_snowy_village_maps",
        "filled_map.village_snowy",
        "minecraft:snowy_village",
        XP_LEVEL_2_SELL,
        {entity::VillagerType::Taiga, entity::VillagerType::Swamp});
    _addTreasureMap(P::Cartographer,
        2,
        8,
        "minecraft:on_savanna_village_maps",
        "filled_map.village_savanna",
        "minecraft:savanna_village",
        XP_LEVEL_2_SELL,
        {entity::VillagerType::Plains, entity::VillagerType::Jungle, entity::VillagerType::Desert});
    _addTreasureMap(P::Cartographer,
        2,
        8,
        "minecraft:on_plains_village_maps",
        "filled_map.village_plains",
        "minecraft:plains_village",
        XP_LEVEL_2_SELL,
        {entity::VillagerType::Taiga,
            entity::VillagerType::Snow,
            entity::VillagerType::Savanna,
            entity::VillagerType::Desert});
    _addTreasureMap(P::Cartographer,
        2,
        8,
        "minecraft:on_jungle_explorer_maps",
        "filled_map.explorer_jungle",
        "minecraft:jungle_temple",
        XP_LEVEL_2_SELL,
        {entity::VillagerType::Swamp, entity::VillagerType::Savanna, entity::VillagerType::Desert});
    _addTreasureMap(P::Cartographer,
        2,
        8,
        "minecraft:on_desert_village_maps",
        "filled_map.village_desert",
        "minecraft:desert_village",
        XP_LEVEL_2_SELL,
        {entity::VillagerType::Savanna, entity::VillagerType::Jungle});

    _add(P::Cartographer, 3, Entry::emeraldForItems(Items::COMPASS, 1, DEFAULT_SUPPLY, XP_LEVEL_3_BUY));
    _addTreasureMap(P::Cartographer,
        3,
        13,
        "minecraft:on_ocean_explorer_maps",
        "filled_map.monument",
        "minecraft:ocean_monument",
        XP_LEVEL_3_SELL,
        {});
    _addTreasureMap(P::Cartographer,
        3,
        12,
        "minecraft:on_trial_chambers_maps",
        "filled_map.trial_chambers",
        "minecraft:trial_chambers",
        XP_LEVEL_3_SELL,
        {});

    _add(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            7, _stackOf(Items::ITEM_FRAME, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER));
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::BLUE_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Snow, entity::VillagerType::Taiga});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::WHITE_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Snow, entity::VillagerType::Plains});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::RED_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Snow, entity::VillagerType::Savanna});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::GREEN_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Desert, entity::VillagerType::Savanna, entity::VillagerType::Jungle});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::LIME_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Desert, entity::VillagerType::Taiga});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::PURPLE_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Taiga, entity::VillagerType::Swamp});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::CYAN_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Desert, entity::VillagerType::Snow});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::YELLOW_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Plains, entity::VillagerType::Jungle});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::ORANGE_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Savanna, entity::VillagerType::Desert});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::BROWN_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Plains, entity::VillagerType::Jungle});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::MAGENTA_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Savanna});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::LIGHT_BLUE_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Snow, entity::VillagerType::Swamp});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::PINK_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Taiga, entity::VillagerType::Plains});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::GRAY_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Desert});
    _addTypeSpecific(P::Cartographer,
        4,
        Entry::pricedEmeralds(
            2, _stackOf(Items::BLACK_BANNER, 1), DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER),
        {entity::VillagerType::Swamp});

    _add(P::Cartographer,
        5,
        Entry::pricedEmeralds(
            8, _stackOf(Items::GLOBE_BANNER_PATTERN, 1), DEFAULT_SUPPLY, XP_LEVEL_5_TRADE, LOW_TIER_PRICE_MULTIPLIER));
    _addTreasureMap(P::Cartographer,
        5,
        14,
        "minecraft:on_woodland_explorer_maps",
        "filled_map.mansion",
        "minecraft:woodland_mansion",
        XP_LEVEL_5_TRADE,
        {});
}

// ============================================================================
// 牧师（vanilla VillagerProfession.CLERIC）
// ============================================================================

void VillagerTrades::_registerClericTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Cleric, 1, Entry::emeraldForItems(Items::ROTTEN_FLESH, 32, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Cleric, 1, Entry::itemsForEmeralds(Items::REDSTONE, 2, 1, 1, XP_LEVEL_1_SELL));

    _add(P::Cleric, 2, Entry::emeraldForItems(Items::GOLD_INGOT, 3, DEFAULT_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Cleric, 2, Entry::itemsForEmeralds(Items::LAPIS_LAZULI, 1, 1, DEFAULT_SUPPLY, XP_LEVEL_2_SELL));

    _add(P::Cleric, 3, Entry::emeraldForItems(Items::RABBIT_FOOT, 2, DEFAULT_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Cleric, 3, Entry::itemsForEmeralds(Items::GLOWSTONE, 1, 4, DEFAULT_SUPPLY, XP_LEVEL_3_SELL));

    _add(P::Cleric, 4, Entry::emeraldForItems(Items::TURTLE_SCUTE, 4, DEFAULT_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Cleric, 4, Entry::emeraldForItems(Items::GLASS_BOTTLE, 9, DEFAULT_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Cleric, 4, Entry::itemsForEmeralds(Items::ENDER_PEARL, 1, 5, DEFAULT_SUPPLY, XP_LEVEL_4_SELL));

    _add(P::Cleric, 5, Entry::emeraldForItems(Items::NETHER_WART, 22, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
    _add(P::Cleric, 5, Entry::itemsForEmeralds(Items::EXPERIENCE_BOTTLE, 1, 3, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
}

// ============================================================================
// 盔甲匠（vanilla VillagerProfession.ARMORER）
// ============================================================================

void VillagerTrades::_registerArmorerTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Armorer, 1, Entry::emeraldForItems(Items::COAL, 15, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Armorer,
        1,
        Entry::pricedEmeralds(
            7, _stackOf(Items::IRON_LEGGINGS, 1), DEFAULT_SUPPLY, XP_LEVEL_1_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Armorer,
        1,
        Entry::pricedEmeralds(
            4, _stackOf(Items::IRON_BOOTS, 1), DEFAULT_SUPPLY, XP_LEVEL_1_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Armorer,
        1,
        Entry::pricedEmeralds(
            5, _stackOf(Items::IRON_HELMET, 1), DEFAULT_SUPPLY, XP_LEVEL_1_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Armorer,
        1,
        Entry::pricedEmeralds(
            9, _stackOf(Items::IRON_CHESTPLATE, 1), DEFAULT_SUPPLY, XP_LEVEL_1_SELL, HIGH_TIER_PRICE_MULTIPLIER));

    _add(P::Armorer, 2, Entry::emeraldForItems(Items::IRON_INGOT, 4, DEFAULT_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Armorer,
        2,
        Entry::pricedEmeralds(
            36, _stackOf(Items::BELL, 1), DEFAULT_SUPPLY, XP_LEVEL_2_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Armorer,
        2,
        Entry::pricedEmeralds(
            1, _stackOf(Items::CHAINMAIL_BOOTS, 1), DEFAULT_SUPPLY, XP_LEVEL_2_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Armorer,
        2,
        Entry::pricedEmeralds(
            3, _stackOf(Items::CHAINMAIL_LEGGINGS, 1), DEFAULT_SUPPLY, XP_LEVEL_2_SELL, HIGH_TIER_PRICE_MULTIPLIER));

    _add(P::Armorer, 3, Entry::emeraldForItems(Items::LAVA_BUCKET, 1, DEFAULT_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Armorer, 3, Entry::emeraldForItems(Items::DIAMOND, 1, DEFAULT_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Armorer,
        3,
        Entry::pricedEmeralds(
            1, _stackOf(Items::CHAINMAIL_HELMET, 1), DEFAULT_SUPPLY, XP_LEVEL_3_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Armorer,
        3,
        Entry::pricedEmeralds(
            4, _stackOf(Items::CHAINMAIL_CHESTPLATE, 1), DEFAULT_SUPPLY, XP_LEVEL_3_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Armorer,
        3,
        Entry::pricedEmeralds(
            5, _stackOf(Items::SHIELD, 1), DEFAULT_SUPPLY, XP_LEVEL_3_SELL, HIGH_TIER_PRICE_MULTIPLIER));

    _add(P::Armorer,
        4,
        std::make_unique<EnchantedItemListing>(
            Items::DIAMOND_LEGGINGS, 14, 3, XP_LEVEL_4_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Armorer,
        4,
        std::make_unique<EnchantedItemListing>(
            Items::DIAMOND_BOOTS, 8, 3, XP_LEVEL_4_SELL, HIGH_TIER_PRICE_MULTIPLIER));

    _add(P::Armorer,
        5,
        std::make_unique<EnchantedItemListing>(
            Items::DIAMOND_HELMET, 8, 3, XP_LEVEL_5_TRADE, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Armorer,
        5,
        std::make_unique<EnchantedItemListing>(
            Items::DIAMOND_CHESTPLATE, 16, 3, XP_LEVEL_5_TRADE, HIGH_TIER_PRICE_MULTIPLIER));
}

// ============================================================================
// 武器匠（vanilla VillagerProfession.WEAPONSMITH）
// ============================================================================

void VillagerTrades::_registerWeaponsmithTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Weaponsmith, 1, Entry::emeraldForItems(Items::COAL, 15, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Weaponsmith,
        1,
        Entry::pricedEmeralds(
            3, _stackOf(Items::IRON_AXE, 1), DEFAULT_SUPPLY, XP_LEVEL_1_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Weaponsmith,
        1,
        std::make_unique<EnchantedItemListing>(Items::IRON_SWORD, 2, 3, XP_LEVEL_1_SELL, LOW_TIER_PRICE_MULTIPLIER));

    _add(P::Weaponsmith, 2, Entry::emeraldForItems(Items::IRON_INGOT, 4, DEFAULT_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Weaponsmith,
        2,
        Entry::pricedEmeralds(
            36, _stackOf(Items::BELL, 1), DEFAULT_SUPPLY, XP_LEVEL_2_SELL, HIGH_TIER_PRICE_MULTIPLIER));

    _add(P::Weaponsmith, 3, Entry::emeraldForItems(Items::FLINT, 24, DEFAULT_SUPPLY, XP_LEVEL_3_BUY));

    _add(P::Weaponsmith, 4, Entry::emeraldForItems(Items::DIAMOND, 1, DEFAULT_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Weaponsmith,
        4,
        std::make_unique<EnchantedItemListing>(Items::DIAMOND_AXE, 12, 3, XP_LEVEL_4_SELL, HIGH_TIER_PRICE_MULTIPLIER));

    _add(P::Weaponsmith,
        5,
        std::make_unique<EnchantedItemListing>(
            Items::DIAMOND_SWORD, 8, 3, XP_LEVEL_5_TRADE, HIGH_TIER_PRICE_MULTIPLIER));
}

// ============================================================================
// 工具匠（vanilla VillagerProfession.TOOLSMITH）
// ============================================================================

void VillagerTrades::_registerToolsmithTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Toolsmith, 1, Entry::emeraldForItems(Items::COAL, 15, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Toolsmith,
        1,
        Entry::pricedEmeralds(
            1, _stackOf(Items::STONE_AXE, 1), DEFAULT_SUPPLY, XP_LEVEL_1_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Toolsmith,
        1,
        Entry::pricedEmeralds(
            1, _stackOf(Items::STONE_SHOVEL, 1), DEFAULT_SUPPLY, XP_LEVEL_1_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Toolsmith,
        1,
        Entry::pricedEmeralds(
            1, _stackOf(Items::STONE_PICKAXE, 1), DEFAULT_SUPPLY, XP_LEVEL_1_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Toolsmith,
        1,
        Entry::pricedEmeralds(
            1, _stackOf(Items::STONE_HOE, 1), DEFAULT_SUPPLY, XP_LEVEL_1_SELL, HIGH_TIER_PRICE_MULTIPLIER));

    _add(P::Toolsmith, 2, Entry::emeraldForItems(Items::IRON_INGOT, 4, DEFAULT_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Toolsmith,
        2,
        Entry::pricedEmeralds(
            36, _stackOf(Items::BELL, 1), DEFAULT_SUPPLY, XP_LEVEL_2_SELL, HIGH_TIER_PRICE_MULTIPLIER));

    _add(P::Toolsmith, 3, Entry::emeraldForItems(Items::FLINT, 30, DEFAULT_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Toolsmith,
        3,
        std::make_unique<EnchantedItemListing>(Items::IRON_AXE, 1, 3, XP_LEVEL_3_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Toolsmith,
        3,
        std::make_unique<EnchantedItemListing>(Items::IRON_SHOVEL, 2, 3, XP_LEVEL_3_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Toolsmith,
        3,
        std::make_unique<EnchantedItemListing>(Items::IRON_PICKAXE, 3, 3, XP_LEVEL_3_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Toolsmith,
        3,
        Entry::pricedEmeralds(4, _stackOf(Items::DIAMOND_HOE, 1), 3, XP_LEVEL_3_SELL, HIGH_TIER_PRICE_MULTIPLIER));

    _add(P::Toolsmith, 4, Entry::emeraldForItems(Items::DIAMOND, 1, DEFAULT_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Toolsmith,
        4,
        std::make_unique<EnchantedItemListing>(Items::DIAMOND_AXE, 12, 3, XP_LEVEL_4_SELL, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Toolsmith,
        4,
        std::make_unique<EnchantedItemListing>(
            Items::DIAMOND_SHOVEL, 5, 3, XP_LEVEL_4_SELL, HIGH_TIER_PRICE_MULTIPLIER));

    _add(P::Toolsmith,
        5,
        std::make_unique<EnchantedItemListing>(
            Items::DIAMOND_PICKAXE, 13, 3, XP_LEVEL_5_TRADE, HIGH_TIER_PRICE_MULTIPLIER));
}

// ============================================================================
// 屠夫（vanilla VillagerProfession.BUTCHER）
// ============================================================================

void VillagerTrades::_registerButcherTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Butcher, 1, Entry::emeraldForItems(Items::CHICKEN, 14, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Butcher, 1, Entry::emeraldForItems(Items::PORKCHOP, 7, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Butcher, 1, Entry::emeraldForItems(Items::RABBIT, 4, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Butcher, 1, Entry::itemsForEmeralds(Items::RABBIT_STEW, 1, 1, 1, XP_LEVEL_1_SELL));

    _add(P::Butcher, 2, Entry::emeraldForItems(Items::COAL, 15, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Butcher, 2, Entry::itemsForEmeralds(Items::COOKED_PORKCHOP, 5, 1, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_SELL));
    _add(P::Butcher, 2, Entry::itemsForEmeralds(Items::COOKED_CHICKEN, 8, 1, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_SELL));

    _add(P::Butcher, 3, Entry::emeraldForItems(Items::MUTTON, 7, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Butcher, 3, Entry::emeraldForItems(Items::BEEF, 10, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));

    _add(P::Butcher, 4, Entry::emeraldForItems(Items::DRIED_KELP_BLOCK, 10, DEFAULT_SUPPLY, XP_LEVEL_4_BUY));

    _add(P::Butcher, 5, Entry::emeraldForItems(Items::SWEET_BERRIES, 10, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
}

// ============================================================================
// 皮革匠（vanilla VillagerProfession.LEATHERWORKER）
// ============================================================================

void VillagerTrades::_registerLeatherworkerTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Leatherworker, 1, Entry::emeraldForItems(Items::LEATHER, 6, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Leatherworker, 1, std::make_unique<DyedArmorListing>(Items::LEATHER_LEGGINGS, 3, DEFAULT_SUPPLY, 1));
    _add(P::Leatherworker, 1, std::make_unique<DyedArmorListing>(Items::LEATHER_CHESTPLATE, 7, DEFAULT_SUPPLY, 1));

    _add(P::Leatherworker, 2, Entry::emeraldForItems(Items::FLINT, 26, DEFAULT_SUPPLY, XP_LEVEL_2_BUY));
    _add(P::Leatherworker,
        2,
        std::make_unique<DyedArmorListing>(Items::LEATHER_HELMET, 5, DEFAULT_SUPPLY, XP_LEVEL_2_SELL));
    _add(P::Leatherworker,
        2,
        std::make_unique<DyedArmorListing>(Items::LEATHER_BOOTS, 4, DEFAULT_SUPPLY, XP_LEVEL_2_SELL));

    _add(P::Leatherworker, 3, Entry::emeraldForItems(Items::RABBIT_HIDE, 9, DEFAULT_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Leatherworker, 3, std::make_unique<DyedArmorListing>(Items::LEATHER_CHESTPLATE, 7, DEFAULT_SUPPLY, 1));

    _add(P::Leatherworker, 4, Entry::emeraldForItems(Items::TURTLE_SCUTE, 4, DEFAULT_SUPPLY, XP_LEVEL_4_BUY));
    _add(P::Leatherworker,
        4,
        std::make_unique<DyedArmorListing>(Items::LEATHER_HORSE_ARMOR, 6, DEFAULT_SUPPLY, XP_LEVEL_4_SELL));

    _add(P::Leatherworker,
        5,
        Entry::pricedEmeralds(
            6, _stackOf(Items::SADDLE, 1), DEFAULT_SUPPLY, XP_LEVEL_5_TRADE, HIGH_TIER_PRICE_MULTIPLIER));
    _add(P::Leatherworker,
        5,
        std::make_unique<DyedArmorListing>(Items::LEATHER_HELMET, 5, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
}

// ============================================================================
// 石匠（vanilla VillagerProfession.MASON）
// ============================================================================

void VillagerTrades::_registerMasonTrades()
{
    using P = entity::VillagerProfession;

    _add(P::Mason, 1, Entry::emeraldForItems(Items::CLAY_BALL, 10, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
    _add(P::Mason, 1, Entry::itemsForEmeralds(Items::BRICK, 10, 1, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_SELL));

    _add(P::Mason, 2, Entry::emeraldForItems(Items::STONE, 20, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_BUY));
    _add(
        P::Mason, 2, Entry::itemsForEmeralds(Items::CHISELED_STONE_BRICKS, 4, 1, COMMON_ITEMS_SUPPLY, XP_LEVEL_2_SELL));

    _add(P::Mason, 3, Entry::emeraldForItems(Items::GRANITE, 16, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Mason, 3, Entry::emeraldForItems(Items::ANDESITE, 16, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Mason, 3, Entry::emeraldForItems(Items::DIORITE, 16, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_BUY));
    _add(P::Mason,
        3,
        Entry::itemsForEmeralds(_item("minecraft:dripstone_block"), 4, 1, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_SELL));
    _add(P::Mason, 3, Entry::itemsForEmeralds(Items::POLISHED_ANDESITE, 4, 1, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_SELL));
    _add(P::Mason, 3, Entry::itemsForEmeralds(Items::POLISHED_DIORITE, 4, 1, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_SELL));
    _add(P::Mason, 3, Entry::itemsForEmeralds(Items::POLISHED_GRANITE, 4, 1, COMMON_ITEMS_SUPPLY, XP_LEVEL_3_SELL));

    _add(P::Mason, 4, Entry::emeraldForItems(Items::QUARTZ, 12, DEFAULT_SUPPLY, XP_LEVEL_4_BUY));
    _addTerracottaTrades(P::Mason, 4);
    _addGlazedTerracottaTrades(P::Mason, 4);

    _add(P::Mason, 5, Entry::itemsForEmeralds(Items::QUARTZ_PILLAR, 1, 1, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
    _add(P::Mason, 5, Entry::itemsForEmeralds(Items::QUARTZ_BLOCK, 1, 1, DEFAULT_SUPPLY, XP_LEVEL_5_TRADE));
}

// ============================================================================
// 条目批量注册辅助
// ============================================================================

void VillagerTrades::_addStew(
    entity::VillagerProfession profession, i32 level, entity::effect::EffectType effectType, i32 duration)
{
    _add(profession, level, std::make_unique<SuspiciousStewListing>(effectType, duration, XP_LEVEL_4_SELL));
}

void VillagerTrades::_addTypeSpecific(entity::VillagerProfession profession,
    i32 level,
    std::unique_ptr<TradeListing> listing,
    std::initializer_list<entity::VillagerType> types)
{
    std::unordered_map<entity::VillagerType, std::shared_ptr<TradeListing>> shared;
    std::shared_ptr<TradeListing> value(std::move(listing));
    for (auto type : types) {
        shared.emplace(type, value);
    }
    _add(profession, level, std::make_unique<TypeSpecificListing>(std::move(shared)));
}

void VillagerTrades::_addTreasureMap(entity::VillagerProfession profession,
    i32 level,
    i32 emeraldCost,
    const char* structureTag,
    const char* displayNameKey,
    const char* decorationId,
    i32 xp,
    std::initializer_list<entity::VillagerType> types)
{
    auto map = std::make_unique<TreasureMapListing>(emeraldCost,
        ResourceLocation(structureTag),
        displayNameKey,
        MapDecoration{decorationId, displayNameKey},
        DEFAULT_SUPPLY,
        xp);
    if (types.size() == 0) {
        _add(profession, level, std::move(map));
        return;
    }
    _addTypeSpecific(profession, level, std::move(map), types);
}

void VillagerTrades::_addWools(
    entity::VillagerProfession profession, i32 level, i32 emeraldCost, i32 woolCount, i32 maxUses, i32 xp)
{
    // 原版按颜色枚举顺序逐个登记，行会按此顺序展示
    static const std::array<Item**, 16> WOOLS = {&Items::WHITE_WOOL,
        &Items::ORANGE_WOOL,
        &Items::MAGENTA_WOOL,
        &Items::LIGHT_BLUE_WOOL,
        &Items::YELLOW_WOOL,
        &Items::LIME_WOOL,
        &Items::PINK_WOOL,
        &Items::GRAY_WOOL,
        &Items::LIGHT_GRAY_WOOL,
        &Items::CYAN_WOOL,
        &Items::PURPLE_WOOL,
        &Items::BLUE_WOOL,
        &Items::BROWN_WOOL,
        &Items::GREEN_WOOL,
        &Items::RED_WOOL,
        &Items::BLACK_WOOL};
    for (Item** wool : WOOLS) {
        _add(profession,
            level,
            Entry::itemsForEmeralds(*wool, woolCount, emeraldCost, maxUses, xp, LOW_TIER_PRICE_MULTIPLIER));
    }
}

void VillagerTrades::_addCarpets(
    entity::VillagerProfession profession, i32 level, i32 emeraldCost, i32 carpetCount, i32 maxUses, i32 xp)
{
    static const std::array<Item**, 16> CARPETS = {&Items::WHITE_CARPET,
        &Items::ORANGE_CARPET,
        &Items::MAGENTA_CARPET,
        &Items::LIGHT_BLUE_CARPET,
        &Items::YELLOW_CARPET,
        &Items::LIME_CARPET,
        &Items::PINK_CARPET,
        &Items::GRAY_CARPET,
        &Items::LIGHT_GRAY_CARPET,
        &Items::CYAN_CARPET,
        &Items::PURPLE_CARPET,
        &Items::BLUE_CARPET,
        &Items::BROWN_CARPET,
        &Items::GREEN_CARPET,
        &Items::RED_CARPET,
        &Items::BLACK_CARPET};
    for (Item** carpet : CARPETS) {
        _add(profession,
            level,
            Entry::itemsForEmeralds(*carpet, carpetCount, emeraldCost, maxUses, xp, LOW_TIER_PRICE_MULTIPLIER));
    }
}

void VillagerTrades::_addBeds(entity::VillagerProfession profession, i32 level, i32 emeraldCost, i32 maxUses, i32 xp)
{
    static const std::array<Item**, 16> BEDS = {&Items::WHITE_BED,
        &Items::YELLOW_BED,
        &Items::RED_BED,
        &Items::BLACK_BED,
        &Items::BLUE_BED,
        &Items::BROWN_BED,
        &Items::CYAN_BED,
        &Items::GRAY_BED,
        &Items::GREEN_BED,
        &Items::LIGHT_BLUE_BED,
        &Items::LIGHT_GRAY_BED,
        &Items::LIME_BED,
        &Items::MAGENTA_BED,
        &Items::ORANGE_BED,
        &Items::PINK_BED,
        &Items::PURPLE_BED};
    for (Item** bed : BEDS) {
        _add(profession, level, Entry::itemsForEmeralds(*bed, 1, emeraldCost, maxUses, xp, LOW_TIER_PRICE_MULTIPLIER));
    }
}

void VillagerTrades::_addBanners(entity::VillagerProfession profession, i32 level, i32 emeraldCost, i32 maxUses, i32 xp)
{
    static const std::array<Item**, 16> BANNERS = {&Items::WHITE_BANNER,
        &Items::BLUE_BANNER,
        &Items::LIGHT_BLUE_BANNER,
        &Items::RED_BANNER,
        &Items::PINK_BANNER,
        &Items::GREEN_BANNER,
        &Items::LIME_BANNER,
        &Items::GRAY_BANNER,
        &Items::BLACK_BANNER,
        &Items::PURPLE_BANNER,
        &Items::MAGENTA_BANNER,
        &Items::CYAN_BANNER,
        &Items::BROWN_BANNER,
        &Items::YELLOW_BANNER,
        &Items::ORANGE_BANNER,
        &Items::LIGHT_GRAY_BANNER};
    for (Item** banner : BANNERS) {
        _add(profession,
            level,
            Entry::itemsForEmeralds(*banner, 1, emeraldCost, maxUses, xp, LOW_TIER_PRICE_MULTIPLIER));
    }
}

void VillagerTrades::_addTerracottaTrades(entity::VillagerProfession profession, i32 level)
{
    static const std::array<Item**, 16> TERRACOTTA = {&Items::ORANGE_TERRACOTTA,
        &Items::WHITE_TERRACOTTA,
        &Items::BLUE_TERRACOTTA,
        &Items::LIGHT_BLUE_TERRACOTTA,
        &Items::GRAY_TERRACOTTA,
        &Items::LIGHT_GRAY_TERRACOTTA,
        &Items::BLACK_TERRACOTTA,
        &Items::RED_TERRACOTTA,
        &Items::PINK_TERRACOTTA,
        &Items::MAGENTA_TERRACOTTA,
        &Items::LIME_TERRACOTTA,
        &Items::GREEN_TERRACOTTA,
        &Items::CYAN_TERRACOTTA,
        &Items::PURPLE_TERRACOTTA,
        &Items::YELLOW_TERRACOTTA,
        &Items::BROWN_TERRACOTTA};
    for (Item** block : TERRACOTTA) {
        _add(profession,
            level,
            Entry::itemsForEmeralds(*block, 1, 1, DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER));
    }
}

void VillagerTrades::_addGlazedTerracottaTrades(entity::VillagerProfession profession, i32 level)
{
    // 带釉陶瓦没有 Items 静态指针，按资源位置逐色登记
    static const char* const GLAZED[] = {"minecraft:orange_glazed_terracotta",
        "minecraft:white_glazed_terracotta",
        "minecraft:blue_glazed_terracotta",
        "minecraft:light_blue_glazed_terracotta",
        "minecraft:gray_glazed_terracotta",
        "minecraft:light_gray_glazed_terracotta",
        "minecraft:black_glazed_terracotta",
        "minecraft:red_glazed_terracotta",
        "minecraft:pink_glazed_terracotta",
        "minecraft:magenta_glazed_terracotta",
        "minecraft:lime_glazed_terracotta",
        "minecraft:green_glazed_terracotta",
        "minecraft:cyan_glazed_terracotta",
        "minecraft:purple_glazed_terracotta",
        "minecraft:yellow_glazed_terracotta",
        "minecraft:brown_glazed_terracotta"};
    for (const char* block : GLAZED) {
        _add(profession,
            level,
            Entry::itemsForEmeralds(_item(block), 1, 1, DEFAULT_SUPPLY, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER));
    }
}
} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
