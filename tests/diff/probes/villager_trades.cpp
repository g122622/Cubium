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

/**
 * @file villager_trades.cpp
 * @brief 差分能力：村民交易（gen / price / restock）。
 *
 * 这个探针由三个共用一个可执行文件的能力组成，用环境变量 `MC_DIFF_VILLAGER_MODE`
 * 选择（`gen` / `price` / `restock`），因为 CMake 里每个 probe 目标只对应一个可执行文件。
 *
 * - `gen`：某职业某等级在某随机种子下抽取出的两条交易。输入是
 *   `{profession, level, num, seed, type}`，输出是交易列表（代价、成品、次数、经验、乘数）。
 *   与被测侧的生产路径一致：同样从该等级的条目列表里移除并生成，直到凑够 num 条。
 * - `price`：`{item, base, demand, multiplier, special, maxStack}` → `{count}`，即
 *   由真实的 `MerchantOffer` 算出的第一支付代价数量（`getAdjustedBuyPrice()`）。
 * - `restock`：一组交易 `{uses, demand, maxUses}` → 经过真实的 `VillagerEntity::restock()`
 *   之后的 `{uses, demand}`。
 *
 * 单元由共同语料决定（`harnesses/villager_trades.ts`），本探针不提供 `list`。
 */

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/WorldgenRandom.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"
#include "common/world/village/trade/TradeContext.hpp"
#include "common/world/village/trade/VillagerTrades.hpp"
#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using namespace mc::world::village::trade;

// ============================================================================
// 模式
// ============================================================================

/**
 * @brief 探针进程内的实体注册表
 *
 * 村民实体构造要求传入注册表；这里只需要一个存活的、进程内唯一的实例。
 */
ecs::EntityRegistry& registry()
{
    static ecs::EntityRegistry instance{"diff-villager"};
    return instance;
}

/**
 * @brief 交易生成上下文（注入附魔标签解析器）
 *
 * 附魔书条目从数据包标签取附魔池（`tradeable` / `trades_*_special`），双倍价格判定也用标签。
 * 标签的加载属于资源管理，交易层不持有全局表，所以这里从探针自己的数据包标签表桥接过去；
 * 生产环境由服务端在调用交易生成时注入同一份解析器。
 */
TradeContext& tradeContext()
{
    static TradeContext context = [] {
        TradeContext ctx;
        ctx.enchantmentTags = [](const ResourceLocation& tag) {
            return datapack::EnchantmentTags::members(tag.toString());
        };
        return ctx;
    }();
    return context;
}

// ============================================================================
// 名称 → 枚举
// ============================================================================

/// `minecraft:farmer` → VillagerProfession。未知职业返回 nullopt（单元键本身由语料决定）。
std::optional<entity::VillagerProfession> professionFromId(const std::string& id)
{
    using entity::VillagerProfession;
    static const std::pair<const char*, VillagerProfession> TABLE[] = {
        {"minecraft:none", VillagerProfession::None},
        {"minecraft:armorer", VillagerProfession::Armorer},
        {"minecraft:butcher", VillagerProfession::Butcher},
        {"minecraft:cartographer", VillagerProfession::Cartographer},
        {"minecraft:cleric", VillagerProfession::Cleric},
        {"minecraft:farmer", VillagerProfession::Farmer},
        {"minecraft:fisherman", VillagerProfession::Fisherman},
        {"minecraft:fletcher", VillagerProfession::Fletcher},
        {"minecraft:leatherworker", VillagerProfession::Leatherworker},
        {"minecraft:librarian", VillagerProfession::Librarian},
        {"minecraft:mason", VillagerProfession::Mason},
        {"minecraft:nitwit", VillagerProfession::Nitwit},
        {"minecraft:shepherd", VillagerProfession::Shepherd},
        {"minecraft:toolsmith", VillagerProfession::Toolsmith},
        {"minecraft:weaponsmith", VillagerProfession::Weaponsmith},
    };
    for (const auto& [name, profession] : TABLE) {
        if (id == name) {
            return profession;
        }
    }
    return std::nullopt;
}

/// `minecraft:plains` → VillagerType。
std::optional<entity::VillagerType> typeFromId(const std::string& id)
{
    using entity::VillagerType;
    static const std::pair<const char*, VillagerType> TABLE[] = {
        {"minecraft:desert", VillagerType::Desert},
        {"minecraft:jungle", VillagerType::Jungle},
        {"minecraft:plains", VillagerType::Plains},
        {"minecraft:savanna", VillagerType::Savanna},
        {"minecraft:snow", VillagerType::Snow},
        {"minecraft:swamp", VillagerType::Swamp},
        {"minecraft:taiga", VillagerType::Taiga},
    };
    for (const auto& [name, type] : TABLE) {
        if (id == name) {
            return type;
        }
    }
    return std::nullopt;
}

// ============================================================================
// 摘要
// ============================================================================

/**
 * @brief 物品堆摘要，与 oracle 的 `stackSummary` 保持同一字段集
 *
 * 只有原文明确"为空"的字段才输出：空堆输出 `item=""`、`count=0`；有耐久的物品才输出
 * `damage`（原版 `stackSummary` 的 `getMaxDamage() > 0` 判定）。附魔与自定义数据
 * （寻宝地图的目标结构）在原版 oracle 里也会输出，本轮尚未对齐，见文件末尾的 TODO。
 */
json stackSummary(const ItemStack& stack)
{
    json object;
    object["item"] = stack.isEmpty() || stack.getItem() == nullptr ? std::string() : stack.getItem()->toString();
    object["count"] = stack.getCount();
    if (!stack.isEmpty() && stack.getMaxDamage() > 0) {
        object["damage"] = stack.getDamage();
    }
    // 附魔书与附魔装备的条目差异主要落在附魔集合上：按附魔 id 建对象，与 oracle 的
    // ItemEnchantments 序列化口径一致（键为附魔资源位置，值为等级）。
    const auto& enchantments = stack.getEnchantments();
    if (!enchantments.isEmpty()) {
        json enchantmentObject = json::object();
        for (const auto& instance : enchantments.getAll()) {
            enchantmentObject[instance.enchantmentId] = instance.level;
        }
        object["enchantments"] = std::move(enchantmentObject);
    }
    return object;
}

/// 支付代价模板摘要：`ItemCost` 存的是模板堆 + 数量，数量取 `ItemCost::count()`。
json costSummary(const ItemCost& cost)
{
    json object = stackSummary(cost.templateStack());
    object["count"] = cost.count();
    return object;
}

json offerSummary(const MerchantOffer& offer)
{
    json object;
    object["buyA"] = costSummary(offer.getCostA());
    const auto buyB = offer.getBuyB();
    object["buyB"] = buyB.has_value() ? stackSummary(*buyB) : stackSummary(ItemStack());
    object["sell"] = stackSummary(offer.getSell());
    object["maxUses"] = offer.getMaxUses();
    object["xp"] = offer.getXp();
    object["priceMultiplier"] = offer.getPriceMultiplier();
    object["demand"] = offer.getDemand();
    return object;
}

// ============================================================================
// gen
// ============================================================================

json runGen(const json& input)
{

    const auto profession = professionFromId(input.at("profession").get<std::string>());
    const auto type = typeFromId(input.at("type").get<std::string>());
    const i32 level = input.at("level").get<i32>();
    const i32 num = input.at("num").get<i32>();
    const i64 seed = input.at("seed").get<i64>();

    // 走生产实现 `generateOffers`：它与原版一样，整批交易共用一条随机流（抽取与条目内部
    // 消耗交替进行）。离线随机源是 `JavaLegacyRandom`，对应原版 `RandomSource.create(seed)`。
    json offers = json::array();
    if (profession.has_value() && type.has_value()) {
        // 构造一个真正的村民作为卖家：条目只读它的 villagerType()。
        entity::VillagerEntity trader(1, registry());
        trader.setVillagerType(*type);

        TradeContext ctx;
        ctx.enchantmentTags = tradeContext().enchantmentTags;
        ctx.trader = &trader;
        ctx.traderSeed = static_cast<u64>(seed);
        ctx.sequence = 0;

        auto offersPtr = VillagerTrades::generateOffers(*profession, level, ctx);
        if (offersPtr != nullptr) {
            for (size_t i = 0; i < offersPtr->size(); ++i) {
                const auto* offer = offersPtr->getOffer(i);
                if (offer != nullptr) {
                    offers.push_back(offerSummary(*offer));
                }
            }
        }
    }
    (void)num;
    return json{{"unit", input.value("unit", "")}, {"offers", std::move(offers)}};
}

// ============================================================================
// price
// ============================================================================

json runPrice(const json& input)
{

    const i32 base = input.at("base").get<i32>();
    const i32 demand = input.at("demand").get<i32>();
    const f32 multiplier = input.at("multiplier").get<f32>();
    const i32 special = input.at("special").get<i32>();
    const i32 maxStack = input.at("maxStack").get<i32>();

    // 直接驱动生产的 MerchantOffer：价格公式只保留一处分发点，改错公式这里立刻可见。
    // 堆叠上限由"代价模板物品"本身决定，语料只用原版确实存在、上限为 1/16/64 的物品
    // （见 harnesses/villager_trades.ts 的 maxStack 说明）。
    ItemStack templateStack = makeStack(input.at("item").get<std::string>().c_str(), maxStack);
    ItemCost cost(templateStack, base);
    MerchantOffer offer(cost, makeStack("minecraft:emerald", 1), 12, 0, multiplier);
    offer.setDemand(demand);
    offer.addToSpecialPrice(special);
    return json{{"unit", input.value("unit", "")}, {"count", offer.getAdjustedBuyPrice()}};
}

// ============================================================================
// restock
// ============================================================================

json runRestock(const json& input)
{

    // 构造一个真正持有这些交易的村民，然后调用生产实现 VillagerEntity::restock()：
    // 补货语义（先 updateDemand 再 resetUses）只保留一处分发点。
    entity::VillagerEntity trader(1, registry());

    MerchantOffers offers;
    for (const auto& entry : input.at("offers")) {
        const i32 uses = entry.at("uses").get<i32>();
        const i32 demand = entry.at("demand").get<i32>();
        const i32 maxUses = entry.at("maxUses").get<i32>();
        MerchantOffer offer(
            ItemCost(makeStack("minecraft:emerald", 1), 1), makeStack("minecraft:emerald", 1), maxUses, 0, 1.0f);
        offer.setDemand(demand);
        for (i32 i = 0; i < uses; ++i) {
            offer.increaseUses();
        }
        offers.addOffer(std::make_unique<MerchantOffer>(std::move(offer)));
    }
    trader.setOffers(std::move(offers));
    trader.restock();

    json result = json::array();
    const auto& restocked = trader.getOffers();
    for (size_t i = 0; i < restocked.size(); ++i) {
        const auto* offer = restocked.getOffer(i);
        result.push_back(json{
            {"demand", offer != nullptr ? offer->getDemand() : 0}, {"uses", offer != nullptr ? offer->getUses() : 0}});
    }
    return json{{"unit", input.value("unit", "")}, {"offers", std::move(result)}};
}

// ============================================================================
// list：交易表本身的导出
// ============================================================================

/**
 * @brief 导出某职业的整张交易表
 *
 * 输入只有职业名，输出是 `{byLevel: {"1": [...], ..., "5": [...]}}`，条目由
 * `TradeListing::describe()` 给出。与 `gen` 的区别：这里**不经过抽签**，直接列出该等级
 * 全部条目，因此每一处差异都能定位到"哪一级、第几条、哪个字段"，而不会被随机抽取掩盖。
 */
json runList(const json& input)
{
    using P = entity::VillagerProfession;
    static const std::pair<const char*, P> PROFESSIONS[] = {
        {"minecraft:armorer", P::Armorer},
        {"minecraft:butcher", P::Butcher},
        {"minecraft:cartographer", P::Cartographer},
        {"minecraft:cleric", P::Cleric},
        {"minecraft:farmer", P::Farmer},
        {"minecraft:fisherman", P::Fisherman},
        {"minecraft:fletcher", P::Fletcher},
        {"minecraft:leatherworker", P::Leatherworker},
        {"minecraft:librarian", P::Librarian},
        {"minecraft:mason", P::Mason},
        {"minecraft:shepherd", P::Shepherd},
        {"minecraft:toolsmith", P::Toolsmith},
        {"minecraft:weaponsmith", P::Weaponsmith},
    };

    json byLevel = json::object();
    const std::string professionId = input.value("profession", "");
    for (const auto& [name, profession] : PROFESSIONS) {
        if (professionId != name) {
            continue;
        }
        const json table = VillagerTrades::describeTrades(profession, tradeContext());
        // 等级下标从 1 开始（下标 0 恒为空），与原版 TRADES 的键一致。
        for (i32 level = VillagerTrades::LEVEL_OFFSET; level <= VillagerTrades::MAX_LEVEL; ++level) {
            const std::string key = std::to_string(level);
            if (table.contains(key)) {
                byLevel[key] = table.at(key);
            }
        }
    }
    return json{{"unit", input.value("unit", "")}, {"byLevel", std::move(byLevel)}};
}

// ============================================================================
// 入口
// ============================================================================

std::string runLine(std::string_view inputLine)
{
    // 交易表是静态常量表，首次访问时构建；注册表（物品 / 方块）必须先就位，
    // 否则条目里引用的物品全部解析失败。
    static const bool initialized = [] {
        datapack::initializeRegistries();
        VillagerTrades::initialize();
        return true;
    }();
    (void)initialized;

    const json input = json::parse(inputLine.begin(), inputLine.end());
    // 四个能力共用一个可执行文件：模式随输入下发（同一进程内也不会串味）。
    const std::string mode = input.value("mode", "gen");
    if (mode == "price") {
        return runPrice(input).dump();
    }
    if (mode == "restock") {
        return runRestock(input).dump();
    }
    if (mode == "list") {
        return runList(input).dump();
    }
    return runGen(input).dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("villager_trades", nullptr, &runLine)

} // namespace mc::diff
