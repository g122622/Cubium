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
 * @file loot_function.cpp
 * @brief 差分能力：战利品函数**逐函数**比对（原版 `LootItemFunction`）。
 *
 * `list`：固定场景单元名（与 LootFunctionOracle.SCENARIO_UNITS 逐字一致）
 * `run` ：{ "unit": "set_count#flat" }
 *         → { "ok": true, "samples": [[{id, count}...] × 4] } 或 { "ok": false, "error": "..." }
 *
 * 现有的三个战利品能力（`loot_table` / `loot_table_block` / `loot_table_entity`）比的是
 * 「整张表掷出来什么」；一张表的差异可能来自池、条目、条件或函数中的任何一层。这里把
 * 范围收窄到**单个函数**：现场造一张「1 个池 + 1 个必定产出一条物品」的表，只在表级
 * 挂一个函数，然后掷骰。差异因此只可能来自这个函数本身。
 *
 * 逐函数覆盖的东西是战利品体系里最难靠肉眼保证的部分：随机值域（`uniform` / `binomial`
 * / `constant` / `score` 的边界）、`apply_bonus` 的四种公式、`set_damage` 的耐久缩放、
 * 附魔与药水的随机抽取顺序……它们都消费随机源，**消耗次数的顺序**错一个就整条链偏移，
 * 而这类错误在手写的期望值里几乎不可能被发现。
 *
 * 掷骰约定与原版侧逐位一致（种子 / 样本推进次数 / 每次重放）：见
 * `loot_probe_common.hpp` 的 ROLL_SEED / ROLL_SAMPLES / SAMPLE_STRIDE。
 */
#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/entity/core/Entity.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootPool.hpp"
#include "common/item/loot/LootSerializers.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/entries/ItemLootEntry.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "loot_probe_common.hpp"
#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using namespace lootprobe;

/**
 * 场景单元名，与 LootFunctionOracle.SCENARIO_UNITS 逐字一致。
 *
 * 两份清单靠差分本身保真：任何一边新增场景而另一边没有，单元集合立刻对不上，报告里会
 * 点名多出来的那个。场景的**取值**（输入物品、函数 JSON）只在 harness 的
 * `loot_function.ts` 里有一份。
 */
constexpr const char* SCENARIO_UNITS[] = {
    // 随机值域：四种 NumberProvider 的边界与分布
    "set_count#constant",
    "set_count#uniform",
    "set_count#binomial",
    "set_count#uniform_float_bounds",
    "set_count#add",
    // 时运加成：四种公式各自对「以 1 为底」与「以 64 为底」的影响
    "apply_bonus#ore_drops",
    "apply_bonus#uniform_bonus",
    "apply_bonus#binomial_with_bonus",
    "apply_bonus#zero_level",
    // 上界裁剪与爆炸衰减
    "limit_count#range",
    "limit_count#only_max",
    "explosion_decay#plain",
    // 伤害：比例 / 区间 / 反向映射
    "set_damage#fraction",
    "set_damage#uniform",
    // 附魔：等级区间、随机附魔、显式附魔表
    "enchant_with_levels#uniform",
    "enchant_with_levels#with_treasure",
    "enchant_randomly#plain",
    "enchant_randomly#options",
    "set_enchantments#levels",
    "enchanted_count_increase#limit",
    // 数据与组件
    "set_custom_data#compound",
    "set_components#stack_size",
    "set_name#fixed",
    "set_lore#fixed",
    "set_potion#id",
    "set_instrument#tag",
    "set_stew_effect#single",
    "set_ominous_bottle_amplifier#range",
    // 物品替换与熔炼
    "furnace_smelt#raw_beef",
    "set_item#swap",
    "discard#always",
    // 容器与结构
    "set_contents#single",
    "set_loot_table#inline",
    "exploration_map#beach",
    // 组合
    "sequence#two",
    "filtered#pass",
    "filtered#block",
    "copy_name#this",
    "copy_custom_data#this",
    "copy_state#this",
    "fill_player_head#this",
};

/// 单元 → 场景参数。取值表在 harness 侧，这里只按 `input` 里的字段驱动。
struct Scenario {
    std::string unit;
    json inputItem;              ///< 函数链的输入物品：`{id, count}`
    json function;               ///< 单个函数的 JSON（数据包写法）
    i32 looting = 0;             ///< 击杀者装备上的 looting 等级（apply_bonus / enchanted_count_increase 用）
    f32 luck = 0.0f;             ///< `LUCK` 参数（原版 SomeFunction 的幸运加成）
    f32 explosionRadius = -1.0f; ///< 负数表示不提供 `explosion_radius`（explosion_decay 用）
};

Scenario parseScenario(const json& input)
{
    Scenario scenario;
    scenario.unit = input.at("unit").get<std::string>();
    scenario.inputItem = input.value("item", json{{"id", "minecraft:stone"}, {"count", 1}});
    scenario.function = input.at("function");
    scenario.looting = input.value("looting", 0);
    scenario.luck = input.value("luck", 0.0f);
    if (input.contains("explosion_radius")) {
        scenario.explosionRadius = input.at("explosion_radius").get<f32>();
    }
    return scenario;
}

/// 由 `{id, count}` 构造输入物品堆；物品不认识时返回 nullptr。
std::unique_ptr<ItemStack> makeInputStack(const json& spec)
{
    const ResourceLocation id(spec.at("id").get<std::string>());
    const Item* item = ItemRegistry::instance().getItem(id);
    if (item == nullptr) {
        return nullptr;
    }
    return std::make_unique<ItemStack>(item, spec.value("count", 1));
}

std::string runLine(std::string_view inputLine)
{
    auto& env = environment();
    const json input = json::parse(inputLine);
    const Scenario scenario = parseScenario(input);

    auto baseItem = makeInputStack(scenario.inputItem);
    if (baseItem == nullptr) {
        // 输入物品不存在：场景本身在当前注册表下不可构造。
        return json{{"unit", scenario.unit}, {"ok", false}, {"error", "unknown input item"}}.dump();
    }

    auto parsed = loot::LootSerializers::parseFunction(scenario.function);
    if (!parsed.success()) {
        return json{{"unit", scenario.unit}, {"ok", false}, {"error", parsed.error().toString()}}.dump();
    }
    auto function = parsed.value();

    // 单池单条目的表：条目必定产出，掷骰次数固定为 1。表级只挂这一个函数——
    // 于是「掷出来什么」完全由函数决定。
    loot::LootTable table;
    auto pool = std::make_unique<loot::LootPool>(math::RandomValueRange(1.0f, 1.0f));
    // 条目按物品**名**构造（与原版 ItemLootEntry 一致），数量取输入给定值，权重 1。
    // 不额外叠 set_count：条目自己的数量区间就够，多一层会让每个场景多消费一次随机数。
    const f32 stackCount = static_cast<f32>(baseItem->getCount());
    pool->addEntry(std::make_unique<loot::ItemLootEntry>(
        baseItem->getItem()->itemLocation().toString(), math::RandomValueRange(stackCount, stackCount), 1, 0));
    table.addPool(std::move(pool));
    table.addFunction(std::move(function));

    // 参数集取 block：它与原版 `LootContextParamSets.BLOCK` 逐项一致——
    //   required {BLOCK_STATE, ORIGIN, TOOL}，optional {THIS_ENTITY, BLOCK_ENTITY, EXPLOSION_RADIUS}。
    // 只有它同时允许 tool 与 explosion_radius：`apply_bonus` 要从工具读附魔等级、
    // `explosion_decay` 要读半径，两者都是「键在就执行、不在就原样返回」。
    const loot::LootParameterSet paramSet = loot::LootParameterSets::block();
    // 工具：原版 `ApplyBonusCount` 从 `TOOL` 键读附魔等级，因此「掠夺等级」落在工具上。
    // 每个场景一份，不跨场景复用，避免上一轮的附魔影响下一轮。
    ItemStack tool(ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_sword")), 1);
    if (scenario.looting > 0) {
        tool.addEnchantment("minecraft:looting", scenario.looting);
    }

    json samples = json::array();
    for (int sample = 0; sample < ROLL_SAMPLES; ++sample) {
        math::Xoroshiro128ppRandom random(ROLL_SEED);
        for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(sample) * SAMPLE_STRIDE; ++i) {
            static_cast<void>(random.nextU64());
        }
        loot::LootContextBuilder builder(*env.world);
        builder.withRandom(random).withLootTableResolver([&env](const std::string& id) -> const loot::LootTable* {
            return env.tables.getTable(id);
        });
        builder.withOwnedValue(loot::LootParams::ORIGIN, Vector3(0.5f, 64.0f, 0.5f));
        builder.withParameter(loot::LootParams::THIS_ENTITY, env.thisEntity.get());
        builder.withOwnedValue(loot::LootParams::LUCK, scenario.luck);
        // `block_state` 是 block 参数集的必需键：取空气，与原版 oracle（世界原点的方块）一致。
        // 本能力的函数都不读它，取值本身不参与比对。
        builder.withParameter(
            loot::LootParams::BLOCK_STATE, const_cast<BlockState*>(&VanillaBlocks::AIR->defaultState()));
        builder.withParameter(loot::LootParams::TOOL, &tool);
        if (scenario.explosionRadius >= 0.0f) {
            builder.withOwnedValue(loot::LootParams::EXPLOSION_RADIUS, scenario.explosionRadius);
        }
        auto context = builder.build(paramSet);
        if (context == nullptr) {
            samples.push_back(json::array());
            continue;
        }
        std::vector<ItemStack> stacks;
        env.world->rollLoot(table, *context, stacks);
        samples.push_back(summariseStacks(stacks));
    }
    return json{{"unit", scenario.unit}, {"ok", true}, {"samples", std::move(samples)}}.dump();
}

void writeList(FILE* out)
{
    for (const char* unit : SCENARIO_UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

} // namespace

MC_DIFF_DEFINE_PROBE("loot_function", &writeList, &runLine)

} // namespace mc::diff
