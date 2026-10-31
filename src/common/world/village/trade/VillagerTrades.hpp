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

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"

#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/item/core/Item.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/village/trade/ItemCost.hpp"
#include "common/world/village/trade/TradeContext.hpp"

#include <array>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace mc {
namespace world {
namespace village {
namespace trade {

class MerchantOffer;
class MerchantOffers;

/// 交易生成用随机源
using TradeRandomSource = mc::math::IRandom;

/**
 * @brief 单条交易条目（vanilla VillagerTrades.ItemListing）
 *
 * 交易表保存的是"条目"而非成品交易：每次为村民生成交易时（初次上任、升级追加、
 * 流浪商人刷新）重新调用 getOffer，按当前随机数、村民类型与卖家位置生成交易
 * （附魔等级、随机染料、寻宝地图目标、按生物群系分支等均在此时决定）。
 */
class TradeListing {
public:
    virtual ~TradeListing() = default;

    /**
     * @brief 生成一条交易
     * @param ctx 生成上下文（世界 / 卖家 / 离线种子）
     * @param random 本条交易专属的随机流
     * @return 交易；无法生成（如找不到目标结构、村民类型不匹配）返回 nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<MerchantOffer> getOffer(
        const TradeContext& ctx, TradeRandomSource& random) const = 0;

    /**
     * @brief 导出交易表条目的数据视图，供差分测试使用
     *
     * 条目生成的交易里，代价、成交物、次数、经验、价格乘数是静态的；只有附魔、染料颜色、
     * 寻宝地图目标等随随机数或世界变化。此方法把静态部分导出，随机部分则连同"它的输入参数"
     * （例如附魔书允许的附魔标签与等级区间）一并导出，使交易表数据本身能与原版逐条比对，
     * 而不必依赖随机数序列对齐。
     *
     * @param deterministicRandom 确定性随机源（恒返回 0）。条目如果需要"重放一次看结果"，
     *        必须只用它，保证导出值与随机数序列无关。
     * @param ctx 生成上下文；只提供与"表格数据"有关的部分（附魔标签解析器），
     *        世界与卖家为空
     * @return 条目描述；默认实现返回空对象，条目类按需覆写
     */
    [[nodiscard]] virtual nlohmann::json describe(TradeRandomSource& deterministicRandom, const TradeContext& ctx) const
    {
        static_cast<void>(deterministicRandom);
        static_cast<void>(ctx);
        return nlohmann::json::object();
    }

    /**
     * @brief 条目种类名，对应原版 `VillagerTrades.ItemListing` 的具体实现类名
     *
     * 差分测试用它区分条目种类：只比字段不分类名时，"用另一种条目凑出相同字段"会被漏掉。
     * 生产代码不调用。
     */
    [[nodiscard]] virtual const char* describeType() const
    {
        return "Unknown";
    }
};

/// 单个等级的交易条目列表
using TradeListingList = std::vector<std::unique_ptr<TradeListing>>;

// ========== 条目工厂（流浪商人交易表复用同一套条目类型） ==========

/**
 * @brief 以物品换绿宝石的条目
 */
[[nodiscard]] std::unique_ptr<TradeListing> makeEmeraldForItems(Item* item, i32 count, i32 maxUses, i32 xp);

/**
 * @brief 以第一支付代价换物品的条目
 */
[[nodiscard]] std::unique_ptr<TradeListing> makeItemsForEmeralds(
    ItemCost cost, ItemStack sell, i32 maxUses, i32 xp, f32 priceMultiplier);

/**
 * @brief 以绿宝石 + 另一物品换成品的条目
 */
[[nodiscard]] std::unique_ptr<TradeListing> makeItemsAndEmeraldsToItems(Item* fromItem,
    i32 fromCount,
    i32 emeraldCost,
    Item* toItem,
    i32 toCount,
    i32 maxUses,
    i32 xp,
    f32 priceMultiplier);

/**
 * @brief 出售随机附魔物品的条目
 */
[[nodiscard]] std::unique_ptr<TradeListing> makeEnchantedItem(
    Item* item, i32 baseEmeraldCost, i32 maxUses, i32 xp, f32 priceMultiplier);

/**
 * @brief 按资源位置构造一个物品堆
 */
[[nodiscard]] ItemStack makeStack(const char* itemId, i32 count);

/**
 * @brief 村民交易表（vanilla VillagerTrades.TRADES）
 *
 * 以「职业 → 等级(1-5) → 条目列表」组织。与原版一致，交易条目是静态构建的常量表，
 * 每次生成交易时按条目重新抽取随机结果。
 */
class VillagerTrades {
public:
    /// 等级 1 开始的下标偏移（等级 1 存于下标 1，下标 0 恒为空）
    static constexpr i32 LEVEL_OFFSET = 1;

    /// 最大职业等级
    static constexpr i32 MAX_LEVEL = 5;

    /**
     * @brief 初始化所有交易配方表
     *
     * 在游戏启动时调用；内部幂等。
     */
    static void initialize();

    /**
     * @brief 获取指定职业、指定等级的交易
     *
     * 只生成 level 这一个等级的条目（与 vanilla Villager.updateTrades 的逐级追加一致）。
     *
     * @param profession 职业
     * @param level 等级（1-5）
     * @param ctx 生成上下文（村民类型由 ctx.trader 提供）
     * @return 交易列表（找不到该等级时为空列表）
     */
    [[nodiscard]] static std::unique_ptr<MerchantOffers> generateOffers(
        entity::VillagerProfession profession, i32 level, const TradeContext& ctx);

    /**
     * @brief 检查职业是否有交易
     * @param profession 职业
     * @return 存在交易返回 true
     */
    [[nodiscard]] static bool hasTrades(entity::VillagerProfession profession);

    /**
     * @brief 注册附魔标签解析器（服务端启动时由资源层注入）
     *
     * 附魔书条目从数据包标签取附魔池，双倍价格判定也用标签。标签表属于资源管理，
     * 交易层不持有它，只保存一个解析回调：未注册时视为所有标签为空集合，
     * 附魔书条目退化为卖普通书（不会崩溃，但与原版不一致）。
     *
     * 该回调在调用 `generateOffers` 之前生效，且只被读取，可在启动期一次性设置。
     */
    static void setEnchantmentTagResolver(TradeContext::EnchantmentTagResolver resolver);

    /**
     * @brief 获取当前注册的附魔标签解析器（未注册时为空）
     */
    [[nodiscard]] static TradeContext::EnchantmentTagResolver enchantmentTagResolver();

    /**
     * @brief 获取职业的交易等级总数
     * @param profession 职业
     * @return 有交易的等级个数
     */
    [[nodiscard]] static i32 getTradeLevelCount(entity::VillagerProfession profession);

    /**
     * @brief 导出某职业完整交易表（等级 → 条目数据视图）
     *
     * 每项是 `TradeListing::describe()` 的结果：静态字段直接给出，随机字段给出"它的输入参数"。
     * 仅供差分测试，生产代码不调用。
     *
     * @param profession 职业
     * @param ctx 生成上下文；只用到其中的附魔标签解析器（附魔书条目要靠它取附魔池），
     *        世界与卖家可为空
     * @return `{"1": [条目, ...], "2": [...], ...}`；该等级没有条目时省略
     */
    [[nodiscard]] static nlohmann::json describeTrades(entity::VillagerProfession profession, const TradeContext& ctx);

private:
    /// 某职业的全部等级条目表（按等级下标索引）
    using LevelTrades = std::array<TradeListingList, MAX_LEVEL + LEVEL_OFFSET>;

    /// 附魔标签解析器（进程内唯一，启动期注册；见 setEnchantmentTagResolver）
    static TradeContext::EnchantmentTagResolver s_enchantmentTags;

    /**
     * @brief 获取（必要时创建）某职业的等级条目表
     * @param profession 职业
     * @return 条目表引用
     */
    static LevelTrades& _trades(entity::VillagerProfession profession);

    /**
     * @brief 向某职业某等级追加条目
     * @param profession 职业
     * @param level 等级（1-5）
     * @param listing 条目
     */
    static void _add(entity::VillagerProfession profession, i32 level, std::unique_ptr<TradeListing> listing);

    // ========== 各职业条目注册 ==========

    static void _registerFarmerTrades();
    static void _registerFishermanTrades();
    static void _registerShepherdTrades();
    static void _registerFletcherTrades();
    static void _registerLibrarianTrades();
    static void _registerCartographerTrades();
    static void _registerClericTrades();
    static void _registerArmorerTrades();
    static void _registerWeaponsmithTrades();
    static void _registerToolsmithTrades();
    static void _registerButcherTrades();
    static void _registerLeatherworkerTrades();
    static void _registerMasonTrades();

    // ========== 条目批量注册辅助 ==========

    /**
     * @brief 追加一条迷之炖菜交易
     * @param profession 职业
     * @param level 等级
     * @param effectType 效果类型
     * @param duration 持续时间（tick）
     */
    static void _addStew(
        entity::VillagerProfession profession, i32 level, entity::effect::EffectType effectType, i32 duration);

    /**
     * @brief 追加一条"仅对指定村民类型生效"的交易（原版 TypeSpecificTrade）
     * @param profession 职业
     * @param level 等级
     * @param listing 条目
     * @param types 生效的村民类型
     */
    static void _addTypeSpecific(entity::VillagerProfession profession,
        i32 level,
        std::unique_ptr<TradeListing> listing,
        std::initializer_list<entity::VillagerType> types);

    /**
     * @brief 追加一条寻宝地图交易
     * @param profession 职业
     * @param level 等级
     * @param emeraldCost 绿宝石价格
     * @param structureTag 目标结构标签
     * @param displayNameKey 地图名称翻译键
     * @param decorationId 地图装饰类型 id
     * @param xp 经验
     * @param types 生效的村民类型（为空表示所有类型）
     */
    static void _addTreasureMap(entity::VillagerProfession profession,
        i32 level,
        i32 emeraldCost,
        const char* structureTag,
        const char* displayNameKey,
        const char* decorationId,
        i32 xp,
        std::initializer_list<entity::VillagerType> types);

    /**
     * @brief 追加以绿宝石出售的 16 色羊毛交易
     */
    static void _addWools(
        entity::VillagerProfession profession, i32 level, i32 emeraldCost, i32 woolCount, i32 maxUses, i32 xp);

    /**
     * @brief 追加以绿宝石出售的 16 色地毯交易
     */
    static void _addCarpets(
        entity::VillagerProfession profession, i32 level, i32 emeraldCost, i32 carpetCount, i32 maxUses, i32 xp);

    /**
     * @brief 追加以绿宝石出售的 16 色床交易
     */
    static void _addBeds(entity::VillagerProfession profession, i32 level, i32 emeraldCost, i32 maxUses, i32 xp);

    /**
     * @brief 追加以绿宝石出售的 16 色旗帜交易
     */
    static void _addBanners(entity::VillagerProfession profession, i32 level, i32 emeraldCost, i32 maxUses, i32 xp);

    /**
     * @brief 追加以绿宝石出售的 16 色陶瓦交易（石匠 4 级）
     */
    static void _addTerracottaTrades(entity::VillagerProfession profession, i32 level);

    /**
     * @brief 追加以绿宝石出售的 16 色带釉陶瓦交易（石匠 4 级）
     */
    static void _addGlazedTerracottaTrades(entity::VillagerProfession profession, i32 level);

    /// 职业 → 等级条目表
    static std::unordered_map<entity::VillagerProfession, LevelTrades, std::hash<entity::VillagerProfession>> s_trades;

    /// 是否已初始化
    static bool s_initialized;
};

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
