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
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"
#include "common/world/village/trade/TradeRandom.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
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
/// 每个等级最多生成几条交易（原版 Villager.updateTrades 传 2）
constexpr i32 MAX_OFFERS_PER_LEVEL = 2;

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

// ========== 条目数据导出（供 TradeListing::describe 使用） ==========
//
// 这些函数只在差分测试读取交易表时被调用，生产路径不经过它们：把「静态字段」的
// 拼装集中在一处，条目类只需回答"我的静态字段有哪些"。

/**
 * @brief 物品 id（空堆返回空串）
 * @param stack 物品堆
 * @return 资源位置字符串
 */
std::string _itemId(const ItemStack& stack)
{
    if (stack.isEmpty() || stack.getItem() == nullptr) {
        return {};
    }
    return stack.getItem()->toString();
}

/**
 * @brief 支付代价摘要
 * @param cost 支付代价
 * @return `{item, count}`
 */
nlohmann::json _summariseCost(const ItemCost& cost)
{
    return nlohmann::json{{"item", _itemId(cost.templateStack())}, {"count", cost.count()}};
}

/**
 * @brief 成交物摘要的公共部分
 * @param stack 成交物模板
 * @return `{item, count}`
 */
nlohmann::json _summariseStack(const ItemStack& stack)
{
    return nlohmann::json{{"item", _itemId(stack)}, {"count", stack.getCount()}};
}

/// 全部村民类型的枚举顺序（导出 TypeSpecificListing 时按此顺序枚举，与原版一致）。
constexpr entity::VillagerType ALL_VILLAGER_TYPES[] = {entity::VillagerType::Desert,
    entity::VillagerType::Jungle,
    entity::VillagerType::Plains,
    entity::VillagerType::Savanna,
    entity::VillagerType::Snow,
    entity::VillagerType::Swamp,
    entity::VillagerType::Taiga};

/**
 * @brief 村民类型的资源位置
 * @param type 村民类型
 * @return `minecraft:xxx`
 */
std::string villagerTypeId(entity::VillagerType type)
{
    switch (type) {
        case entity::VillagerType::Desert:
            return "minecraft:desert";
        case entity::VillagerType::Jungle:
            return "minecraft:jungle";
        case entity::VillagerType::Plains:
            return "minecraft:plains";
        case entity::VillagerType::Savanna:
            return "minecraft:savanna";
        case entity::VillagerType::Snow:
            return "minecraft:snow";
        case entity::VillagerType::Swamp:
            return "minecraft:swamp";
        case entity::VillagerType::Taiga:
            return "minecraft:taiga";
    }
    return {};
}

/**
 * @brief 固定字段条目的公共描述
 * @param buyA 第一支付的摘要
 * @param sell 成交物摘要（至少含 `item`）
 * @param maxUses 最大使用次数
 * @param xp 经验
 * @param priceMultiplier 价格乘数
 * @return 条目描述对象
 */
nlohmann::json describeFixed(nlohmann::json buyA, nlohmann::json sell, i32 maxUses, i32 xp, f32 priceMultiplier)
{
    nlohmann::json result = nlohmann::json::object();
    result["buyA"] = std::move(buyA);
    result["sell"] = std::move(sell);
    result["maxUses"] = maxUses;
    result["xp"] = xp;
    // 价格乘数按 f32 提升到 f64 输出：原版 oracle 侧读到的就是这个值，
    // 两侧用同一表示，避免 0.05f 的最短十进制表示（0.05）与提升后的值
    // （0.05000000074505806）被当成差异。
    result["priceMultiplier"] = static_cast<double>(priceMultiplier);
    return result;
}

/**
 * @brief 确定性随机源：所有取数方法都返回 0
 *
 * 差分测试导出交易表时用它在条目上"重放一次"，使随机量（附魔等级、染料颜色）固定为
 * 下界。原版 oracle 用 `RandomSource.create(0L)` 做同样的事，两侧结果直接可比；
 * 条目里的价格公式若写错，重放结果会立刻偏离。
 */
class ZeroRandom final : public TradeRandomSource {
public:
    void setSeed(u64) override {}

    [[nodiscard]] u64 nextU64() override
    {
        return 0;
    }

    /// 恒返回区间下界：标准库重载把 `nextInt(bound)` 实现为 `nextU32() % bound`，
    /// 而 `nextU32()` 默认取 `nextU64()` 的高 32 位（同样为 0），结果一致。
    [[nodiscard]] i32 nextInt(i32) override
    {
        return 0;
    }

    [[nodiscard]] i32 nextInt(i32 min, i32) override
    {
        return min;
    }

    [[nodiscard]] f32 nextFloat() override
    {
        return 0.0f;
    }

    [[nodiscard]] math::PositionalRandomFactory forkPositional() override
    {
        return math::PositionalRandomFactory(u64{0});
    }
};

/**
 * @brief 由一次重放结果拼装条目描述（供可变条目使用）
 * @param offer 重放得到的交易
 * @param sell 成交物摘要（已按需要剥掉随机部分）
 * @return 条目描述对象
 */
nlohmann::json describeOffer(const MerchantOffer& offer, nlohmann::json sell)
{
    return describeFixed(_summariseCost(offer.getCostA()),
        std::move(sell),
        offer.getMaxUses(),
        offer.getXp(),
        offer.getPriceMultiplier());
}

/**
 * @brief 交易上下文中的随机源访问
 *
 * 有世界时使用世界随机流（与村民 tick 共享，重放一致）；离线场景回退到由种子构造的
 * 独立随机树。
 *
 * 【一条流贯穿整批】原版 `AbstractVillager.addOffersFromItemListings` 把**同一个**
 * `RandomSource` 既用于"抽哪一条"、又传给条目自身的 `getOffer`，所以"抽中哪条"与
 * "条目内部抽到什么"共享同一条序列。这里保持同样的语义：整批交易只用一条流，
 * 不再为每条派生独立子流（派生会让两段随机互相独立，与原版错位）。
 *
 * 【离线算法】离线流用 `JavaLegacyRandom`（Java 的 48 位 LCG），对应原版
 * `RandomSource.create(seed)` → `LegacyRandomSource`。项目内另有 Xoroshiro 实现，
 * 但原版只有 `createThreadLocalInstance` 等路径才用它，与"以种子建交易流"无关。
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
     * @brief 获取整批交易共用的随机流
     * @return 随机流
     */
    [[nodiscard]] TradeRandomSource& next()
    {
        if (m_ctx.world != nullptr) {
            return m_ctx.world->getRandom();
        }
        // 离线：对应原版 `RandomSource.create(seed)`，它构造的是 **LegacyRandomSource**
        // （Java 的 48 位 LCG），不是 Xoroshiro。用 Xoroshiro 会让整条随机序列与原版不同，
        // 同一 seed 下抽到的条目、条目内部抽到的附魔等级都会整体错位。
        //
        // 也不再叠加序号：原版就是"以某个种子建一条流"，需要区分批次时由调用方给不同的
        // traderSeed（服务端用「世界种子 + 实体 id」派生，与这里无关）。
        if (!m_offline.has_value()) {
            m_offline.emplace(m_ctx.traderSeed);
        }
        return *m_offline;
    }

private:
    const TradeContext& m_ctx;
    std::optional<math::JavaLegacyRandom> m_offline;
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

    [[nodiscard]] nlohmann::json describe(
        TradeRandomSource& deterministicRandom, const TradeContext& ctx) const override
    {
        return describeFixed(_summariseCost(m_costA), _summariseStack(m_sell), m_maxUses, m_xp, m_priceMultiplier);
    }

private:
    ItemCost m_costA;
    ItemStack m_sell;
    i32 m_maxUses;
    i32 m_xp;
    f32 m_priceMultiplier;

public:
    /**
     * @brief 条目种类名（原版把这两种形态分成 `EmeraldForItems` 与 `ItemsForEmeralds`）
     *
     * 项目用同一个类承载两者，按「成交物是否为绿宝石」区分：卖绿宝石的是收购型条目，
     * 其余是出售型条目。
     */
    [[nodiscard]] const char* describeType() const override
    {
        return _itemId(m_sell) == "minecraft:emerald" ? "EmeraldForItems" : "ItemsForEmeralds";
    }
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

    [[nodiscard]] nlohmann::json describe(
        TradeRandomSource& deterministicRandom, const TradeContext& ctx) const override
    {
        nlohmann::json result = describeFixed(
            _summariseCost(_emeralds(m_emeraldCost)), _summariseStack(m_toItem), m_maxUses, m_xp, m_priceMultiplier);
        if (m_fromItem.has_value()) {
            result["buyB"] = _summariseCost(*m_fromItem);
        }
        return result;
    }

private:
    i32 m_emeraldCost;
    std::optional<ItemCost> m_fromItem;
    ItemStack m_toItem;
    i32 m_maxUses;
    i32 m_xp;
    f32 m_priceMultiplier;

public:
    [[nodiscard]] const char* describeType() const override
    {
        return "ItemsAndEmeraldsToItems";
    }
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
     * @param kind 条目种类名（差分测试用）：原版把「按类型给不同物品」拆成
     *        `TypeSpecificTrade`（给整条交易）与 `EmeraldsForVillagerTypeItem`（只给物品）两族
     */
    explicit TypeSpecificListing(std::unordered_map<entity::VillagerType, std::shared_ptr<TradeListing>> listings,
        const char* kind = "TypeSpecificTrade")
        : m_listings(std::move(listings))
        , m_kind(kind)
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

    [[nodiscard]] nlohmann::json describe(
        TradeRandomSource& deterministicRandom, const TradeContext& ctx) const override
    {
        // 原版 TypeSpecificTrade 记录的是一张「村民类型 → 条目」表；导出为有序数组
        // （按 `VillagerType` 枚举顺序），与 oracle 侧枚举 7 种类型的顺序一致。
        nlohmann::json byType = nlohmann::json::array();
        for (entity::VillagerType type : ALL_VILLAGER_TYPES) {
            auto found = m_listings.find(type);
            if (found == m_listings.end() || found->second == nullptr) {
                continue;
            }
            nlohmann::json entry = found->second->describe(deterministicRandom, ctx);
            entry["type"] = found->second->describeType();
            byType.push_back(nlohmann::json{{"type", villagerTypeId(type)}, {"listing", std::move(entry)}});
        }
        return nlohmann::json{{"byType", std::move(byType)}};
    }

private:
    std::unordered_map<entity::VillagerType, std::shared_ptr<TradeListing>> m_listings;
    const char* m_kind;

public:
    [[nodiscard]] const char* describeType() const override
    {
        return m_kind;
    }
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

    [[nodiscard]] std::unique_ptr<MerchantOffer> getOffer(
        const TradeContext& ctx, TradeRandomSource& random) const override
    {
        const i32 level = 5 + random.nextInt(15);
        ItemStack stack = _stackOf(m_item, 1);
        // 原版只从 `ON_TRADED_EQUIPMENT` 标签的附魔里挑（如锋利、保护），不会出现
        // "钻石护腿附锋利"这种物品本身不支持的附魔。标签由调用方注入。
        std::vector<std::string> allowed;
        if (ctx.enchantmentTags) {
            // 保留标签自身的声明顺序：候选列表按下标抽取（与原版 `getRandomElementOf` 一致），
            // 排序会让同一个随机数抽到别的附魔。
            allowed = ctx.enchantmentTags(ResourceLocation("minecraft:on_traded_equipment"));
        }
        stack = item::enchant::EnchantmentHelper::addRandomEnchantment(random, stack, level, false, &allowed);
        const i32 cost = std::min(m_baseEmeraldCost + level, 64);
        return std::make_unique<MerchantOffer>(_emeralds(cost), stack, m_maxUses, m_xp, m_priceMultiplier);
    }

    [[nodiscard]] nlohmann::json describe(
        TradeRandomSource& deterministicRandom, const TradeContext& ctx) const override
    {
        // 价格与附魔随随机数变化：用确定性随机源重放一次 getOffer。
        //
        // 原版 `EnchantedItemForEmeralds.getOffer` 以 `5 + random.nextInt(15)` 作为附魔等级，
        // 价格 = min(基础价格 + 该等级, 64)。随机数恒为 0 时等级恰为 5、价格 = 基础 + 5；
        // oracle 侧用 `RandomSource.create(0L)` 做同样的事，两侧结果可直接比对，
        // 基础价格写错会立刻显现。附魔内容本身随机，只导出"附魔池"标签。
        ZeroRandom random;
        auto offer = getOffer(ctx, random);
        nlohmann::json sell = _summariseStack(_stackOf(m_item, 1));
        sell["enchantmentTag"] = std::string("minecraft:on_traded_equipment");
        return describeOffer(*offer, std::move(sell));
    }

private:
    Item* m_item;
    i32 m_baseEmeraldCost;
    i32 m_maxUses;
    i32 m_xp;
    f32 m_priceMultiplier;

public:
    [[nodiscard]] const char* describeType() const override
    {
        return "EnchantedItemForEmeralds";
    }
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
     * @param enchantmentTag 附魔标签（不含 `#` 前缀与命名空间，例如 `tradeable`）
     * @param minLevel 附魔等级下界（0 表示不限）
     * @param maxLevel 附魔等级上界（0 表示不限）
     */
    EnchantBookListing(i32 xp, std::string enchantmentTag = "tradeable", i32 minLevel = 0, i32 maxLevel = 0)
        : m_xp(xp)
        , m_enchantmentTag(std::move(enchantmentTag))
        , m_minLevel(minLevel)
        , m_maxLevel(maxLevel)
    {}

    [[nodiscard]] std::unique_ptr<MerchantOffer> getOffer(
        const TradeContext& ctx, TradeRandomSource& random) const override
    {
        // 附魔池来自数据包标签（普通村民 `tradeable`，沼泽/针叶林专用村民各有一个专用标签），
        // 与原版 `getRandomElementOf(TagKey)` 一致：只在该标签的成员里均匀抽取。
        std::vector<const item::enchant::Enchantment*> candidates;
        if (ctx.enchantmentTags) {
            // 原版 `getRandomElementOf(TagKey)` 的下标落在**标签内容列表**上，该列表按附魔注册表
            // 的 holder 顺序生成（实测：protection、fire_protection……mending，与项目注册顺序一致）。
            // 因此候选顺序必须是注册顺序——不能按 id 排序，也不能用标签解析器返回的任意顺序
            // （哈希表遍历顺序不稳定）。排序会让 nextInt(size) 抽到别的附魔。
            std::vector<std::string> members = ctx.enchantmentTags(ResourceLocation("minecraft:" + m_enchantmentTag));
            const auto contains = [&members](const std::string& id) {
                return std::find(members.begin(), members.end(), id) != members.end();
            };
            for (const auto* enchantment : item::enchant::EnchantmentRegistry::allInRegistrationOrder()) {
                if (enchantment != nullptr && contains(enchantment->id())) {
                    candidates.push_back(enchantment);
                }
            }
        }

        const Item* book = _item("minecraft:book");
        MC_ASSERT_RELEASE(book != nullptr);

        ItemStack itemStack;
        i32 price = 1;
        if (!candidates.empty()) {
            // 原版 `getRandomElementOf` 均匀抽取（不是按权重），与这里一致。
            const auto& enchantment =
                *candidates[static_cast<size_t>(random.nextInt(static_cast<i32>(candidates.size())))];
            const i32 minLevel = std::max(enchantment.minLevel(), m_minLevel);
            const i32 maxLevel =
                m_maxLevel <= 0 ? enchantment.maxLevel() : std::min(enchantment.maxLevel(), m_maxLevel);
            const i32 level = minLevel >= maxLevel ? minLevel : random.nextInt(minLevel, maxLevel);

            itemStack = ItemStack(*_item("minecraft:enchanted_book"), 1);
            item::items::EnchantedBookItem::addEnchantment(itemStack, enchantment, level);

            price = 2 + random.nextInt(5 + level * 10) + 3 * level;
            // 双倍价格：原版按 `DOUBLE_TRADE_PRICE` 标签判定（例如经验修补）
            if (ctx.enchantmentTags) {
                const auto doublePrice = ctx.enchantmentTags(ResourceLocation("minecraft:double_trade_price"));
                if (std::find(doublePrice.begin(), doublePrice.end(), enchantment.id()) != doublePrice.end()) {
                    price *= 2;
                }
            }
            price = std::min(price, 64);
        } else {
            itemStack = ItemStack(*book, 1);
        }

        return std::make_unique<MerchantOffer>(
            _emeralds(price), ItemCost(ItemStack(*book, 1)), itemStack, 12, m_xp, HIGH_TIER_PRICE_MULTIPLIER);
    }

    [[nodiscard]] nlohmann::json describe(
        TradeRandomSource& deterministicRandom, const TradeContext& ctx) const override
    {
        // 附魔内容与价格都随随机数变化：用确定性随机源重放一次 getOffer。
        // TODO: 这里不带 TradeContext，附魔池解析器为空 → 只能导出"卖普通书"的价格。
        //   待 TradeListing::describe 增加上下文参数后再导出真实附魔池与价格，见
        //   VillagerTrades::describeTrades 的说明。
        ZeroRandom random;
        auto offer = getOffer(ctx, random);
        nlohmann::json sell = nlohmann::json{{"item", std::string("minecraft:enchanted_book")},
            {"count", 1},
            {"enchantmentTag", "minecraft:" + m_enchantmentTag},
            {"minLevel", m_minLevel},
            {"maxLevel", m_maxLevel}};
        nlohmann::json result = describeOffer(*offer, std::move(sell));
        // 第一支付（成交价）完全取决于随机抽中的附魔与等级，不承载表数据；
        // 静态部分是「第二支付恒为 1 本书」与上面的附魔池 + 等级区间。
        // 成交价的正确性由 gen 层验证。
        result.erase("buyA");
        result["buyB"] = _summariseCost(ItemCost(_stackOf(_item("minecraft:book"), 1)));
        return result;
    }

private:
    i32 m_xp;
    std::string m_enchantmentTag;
    i32 m_minLevel;
    i32 m_maxLevel;

public:
    [[nodiscard]] const char* describeType() const override
    {
        return "EnchantBookForEmeralds";
    }
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

    [[nodiscard]] nlohmann::json describe(
        TradeRandomSource& deterministicRandom, const TradeContext& ctx) const override
    {
        // 颜色随机（1~3 种染料混合），静态部分只有物品与价格。
        nlohmann::json sell = _summariseStack(_stackOf(m_item, 1));
        sell["dyed"] = true;
        return describeFixed(
            _summariseCost(_emeralds(m_emeraldCost)), std::move(sell), m_maxUses, m_xp, HIGH_TIER_PRICE_MULTIPLIER);
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

public:
    [[nodiscard]] const char* describeType() const override
    {
        return "DyedArmorForEmeralds";
    }
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

    [[nodiscard]] nlohmann::json describe(
        TradeRandomSource& deterministicRandom, const TradeContext& ctx) const override
    {
        nlohmann::json sell = nlohmann::json{{"item", std::string("minecraft:suspicious_stew")},
            {"count", 1},
            {"effect", entity::effect::getEffectResourceLocation(m_effectType).toString()}};
        return describeFixed(_summariseCost(_emeralds(1)), std::move(sell), 12, m_xp, LOW_TIER_PRICE_MULTIPLIER);
    }

private:
    entity::effect::EffectType m_effectType;
    i32 m_duration;
    i32 m_xp;

public:
    [[nodiscard]] const char* describeType() const override
    {
        return "SuspiciousStewForEmerald";
    }
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

    [[nodiscard]] nlohmann::json describe(
        TradeRandomSource& deterministicRandom, const TradeContext& ctx) const override
    {
        // TODO: 地图创建落地后，成交物应导出带 `treasureMap` 目标结构标签与装饰类型的
        //   filled_map；当前生产路径恒返回空交易（见 getOffer 的 TODO），导出值只用于
        //   比对"这张表里登记了哪些地图条目"，不能让地图类条目通过一致性判定。
        nlohmann::json sell = nlohmann::json{{"item", std::string("minecraft:filled_map")},
            {"count", 1},
            {"targetTag", m_structureTag.toString()},
            {"displayName", std::string(m_displayNameKey != nullptr ? m_displayNameKey : "")},
            {"decorationType", std::string(m_decoration.id != nullptr ? m_decoration.id : "")}};
        return describeFixed(
            _summariseCost(_emeralds(m_emeraldCost)), std::move(sell), m_maxUses, m_xp, LOW_TIER_PRICE_MULTIPLIER);
    }

private:
    i32 m_emeraldCost;
    ResourceLocation m_structureTag;
    const char* m_displayNameKey;
    MapDecoration m_decoration;
    i32 m_maxUses;
    i32 m_xp;

public:
    [[nodiscard]] const char* describeType() const override
    {
        return "TreasureMapForEmeralds";
    }
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

    [[nodiscard]] nlohmann::json describe(
        TradeRandomSource& deterministicRandom, const TradeContext& ctx) const override
    {
        // 药水随机，静态部分是两侧物品与数量；成交物标记为"带药水的药箭"。
        nlohmann::json result = describeFixed(_summariseCost(_emeralds(m_emeraldCost)),
            _summariseStack(_stackOf(m_toItem, m_toCount)),
            m_maxUses,
            m_xp,
            LOW_TIER_PRICE_MULTIPLIER);
        result["buyB"] = _summariseCost(ItemCost(_stackOf(m_fromItem, m_fromCount)));
        return result;
    }

private:
    Item* m_fromItem;
    i32 m_fromCount;
    Item* m_toItem;
    i32 m_toCount;
    i32 m_emeraldCost;
    i32 m_maxUses;
    i32 m_xp;

public:
    [[nodiscard]] const char* describeType() const override
    {
        return "TippedArrowForItemsAndEmeralds";
    }
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
     * @brief 以绿宝石买物品（原版 `new ItemsForEmeralds(item, emeraldCost, count, maxUses, xp)`）
     *
     * 参数顺序与原版源码**逐位对应**。项目里另有一个把物品数量放在第二位、
     * 绿宝石价格放在第三位的变体，两者顺序相反，抄原版表时极易写反，
     * 因此这里用独立名字单独固化原版顺序：绿宝石价格在前、成交数量在后，
     * 价格乘数固定 0.05（原版该构造器的默认值）。
     */
    static std::unique_ptr<TradeListing> itemsForEmeraldsByPrice(
        Item* item, i32 emeraldCost, i32 itemCount, i32 maxUses, i32 xp)
    {
        return std::make_unique<SimpleListing>(
            _emeralds(emeraldCost), _stackOf(item, itemCount), maxUses, xp, LOW_TIER_PRICE_MULTIPLIER);
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
TradeContext::EnchantmentTagResolver VillagerTrades::s_enchantmentTags;

// ============================================================================
// 初始化与查询
// ============================================================================

void VillagerTrades::setEnchantmentTagResolver(TradeContext::EnchantmentTagResolver resolver)
{
    s_enchantmentTags = std::move(resolver);
}

TradeContext::EnchantmentTagResolver VillagerTrades::enchantmentTagResolver()
{
    return s_enchantmentTags;
}

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
    entity::VillagerProfession profession, i32 level, const TradeContext& incomingCtx)
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

    // 附魔标签由调用方（服务端资源层 / 测试探针）注入；调用方没有显式给出时，
    // 退回进程内注册的那份（见 setEnchantmentTagResolver）。
    TradeContext ctx = incomingCtx;
    if (!ctx.enchantmentTags) {
        ctx.enchantmentTags = s_enchantmentTags;
    }

    // 对应原版 AbstractVillager.addOffersFromItemListings(..., offeredCount = 2)：
    // 从该等级的条目里**随机抽取**若干条，而不是把整级条目全部转成交易。原版复制一份列表，
    // 每次 `remove(random.nextInt(size))` 取出一条；抽中的条目 getOffer 返回空时不计数、
    // 继续抽其余条目，直到凑够 offeredCount 或列表用尽。
    //
    // 【随机数】条目选择与条目内部的随机抽取共用同一条随机流（原版传的就是 this.random），
    // 因此这里把同一条流交给 getOffer，而不是给每条派生独立子流——派生会让"抽中哪条"与
    // "条目内部抽到什么"互相独立，随机数消耗顺序与原版完全不同。
    std::vector<TradeListing*> remaining;
    for (const auto& listing : found->second[static_cast<size_t>(level)]) {
        remaining.push_back(listing.get());
    }

    RandomSourceView randomView(ctx);
    i32 drawn = 0;
    while (drawn < MAX_OFFERS_PER_LEVEL && !remaining.empty()) {
        TradeRandomSource& random = randomView.next();
        const size_t index = static_cast<size_t>(random.nextInt(static_cast<i32>(remaining.size())));
        TradeListing* listing = remaining[index];
        remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(index));

        auto offer = listing->getOffer(ctx, random);
        if (offer != nullptr) {
            offers->addOffer(std::move(offer));
            ++drawn;
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

nlohmann::json VillagerTrades::describeTrades(entity::VillagerProfession profession, const TradeContext& ctx)
{
    if (!s_initialized) {
        initialize();
    }
    nlohmann::json result = nlohmann::json::object();
    auto found = s_trades.find(profession);
    if (found == s_trades.end()) {
        return result;
    }
    // 每条条目都从同一个零值随机源开始重放：随机量固定取下界，导出值与序列无关。
    ZeroRandom deterministicRandom;
    for (size_t index = 0; index < found->second.size(); ++index) {
        const auto& levelList = found->second[index];
        if (levelList.empty()) {
            continue;
        }
        nlohmann::json listings = nlohmann::json::array();
        for (const auto& listing : levelList) {
            nlohmann::json entry = listing->describe(deterministicRandom, ctx);
            // `type` 由具体条目类声明（对应原版 ItemListing 的类名），是区分条目种类的主键：
            // 只比字段不分类名，"用另一种条目凑出同样的字段"会被漏过去。
            entry["type"] = listing->describeType();
            listings.push_back(std::move(entry));
        }
        result[std::to_string(index)] = std::move(listings);
    }
    return result;
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
        _add(P::Fisherman, 5, std::make_unique<TypeSpecificListing>(std::move(boats), "EmeraldsForVillagerTypeItem"));
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
    _add(P::Shepherd, 1, Entry::itemsForEmeralds(Items::SHEARS, 1, 2, DEFAULT_SUPPLY, XP_LEVEL_1_SELL));

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
        std::make_unique<EnchantedItemListing>(Items::CROSSBOW, 3, 3, XP_LEVEL_4_SELL, LOW_TIER_PRICE_MULTIPLIER));
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
        "minecraft:village_taiga",
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
        "minecraft:village_snowy",
        XP_LEVEL_2_SELL,
        {entity::VillagerType::Taiga, entity::VillagerType::Swamp});
    _addTreasureMap(P::Cartographer,
        2,
        8,
        "minecraft:on_savanna_village_maps",
        "filled_map.village_savanna",
        "minecraft:village_savanna",
        XP_LEVEL_2_SELL,
        {entity::VillagerType::Plains, entity::VillagerType::Jungle, entity::VillagerType::Desert});
    _addTreasureMap(P::Cartographer,
        2,
        8,
        "minecraft:on_plains_village_maps",
        "filled_map.village_plains",
        "minecraft:village_plains",
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
        "minecraft:village_desert",
        XP_LEVEL_2_SELL,
        {entity::VillagerType::Savanna, entity::VillagerType::Jungle});

    _add(P::Cartographer, 3, Entry::emeraldForItems(Items::COMPASS, 1, DEFAULT_SUPPLY, XP_LEVEL_3_BUY));
    _addTreasureMap(P::Cartographer,
        3,
        13,
        "minecraft:on_ocean_explorer_maps",
        "filled_map.monument",
        "minecraft:monument",
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
        "minecraft:mansion",
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
    _add(P::Cleric, 1, Entry::itemsForEmeralds(Items::REDSTONE, 2, 1, DEFAULT_SUPPLY, XP_LEVEL_1_SELL));

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
    _add(P::Butcher, 1, Entry::itemsForEmeralds(Items::RABBIT_STEW, 1, 1, DEFAULT_SUPPLY, XP_LEVEL_1_SELL));

    _add(P::Butcher, 2, Entry::emeraldForItems(Items::COAL, 15, COMMON_ITEMS_SUPPLY, XP_LEVEL_1_BUY));
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
