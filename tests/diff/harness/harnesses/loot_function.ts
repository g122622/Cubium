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

/*
 * loot_function.ts — 战利品函数逐函数比对的场景表。
 *
 * 每个场景给出三样东西，两侧读同一份：
 *   - `item`：进入函数链的物品堆（`{id, count}`）；
 *   - `function`：**一个**函数的 JSON（数据包写法，`function` 字段带 `minecraft:` 前缀）；
 *   - 可选上下文：`looting`（击杀者手持物品上的掠夺等级）、`luck`（LUCK 参数）、
 *     `explosion_radius`（缺省不提供，与原版一致）。
 *
 * 探针与 oracle 各自现场造一张「1 个池 + 1 个必定产出一条物品」的表，只在表级挂这个
 * 函数再掷骰。差异因此只可能来自函数本身，而不是池、条目或条件。
 *
 * 为什么逐个函数测：函数是战利品体系里**唯一大量消费随机源**的一层。随机数的消耗顺序
 * 错一位（例如先判断再取数、或取值区间端点写反），整条链的结果都会偏移，而这类错误在
 * 手写期望值里几乎不可能被发现——只能在同一个种子下逐位比对。
 */

/** 一个场景：单元名 + 输入 + 单个函数 + 可选上下文。 */
export interface LootFunctionScenario {
    readonly unit: string;
    readonly item?: { readonly id: string; readonly count?: number };
    readonly function: Record<string, unknown>;
    readonly looting?: number;
    readonly luck?: number;
    readonly explosion_radius?: number;
}

/** 带 `minecraft:` 前缀的函数名（数据包写法，两侧解析器都认）。 */
const fn = (name: string, body: Record<string, unknown> = {}): Record<string, unknown> => ({
    function: `minecraft:${name}`,
    ...body,
});

export const LOOT_FUNCTIONS: readonly LootFunctionScenario[] = [
    // ---- 随机值域：NumberProvider 的四种形态 ----
    { unit: "set_count#constant", function: fn("set_count", { count: 5 }) },
    { unit: "set_count#uniform", function: fn("set_count", { count: { type: "minecraft:uniform", min: 1, max: 6 } }) },
    {
        unit: "set_count#binomial",
        function: fn("set_count", { count: { type: "minecraft:binomial", n: 5, p: 0.35 } }),
    },
    {
        // 小数端点：原版的 UniformGenerator.getInt 是
        // `Mth.nextInt(rand, min.getInt(), max.getInt())`——**两端各自取整**后再取闭区间整数，
        // 而不是「先取实数再截断」。把 1.5 / 4.5 写成 floor 或 ceil 会让整段偏一格。
        unit: "set_count#uniform_float_bounds",
        function: fn("set_count", { count: { type: "minecraft:uniform", min: 1.5, max: 4.5 } }),
    },
    {
        unit: "set_count#add",
        item: { id: "minecraft:stone", count: 4 },
        function: fn("set_count", { count: 3, add: true }),
    },

    // ---- 时运加成：四种公式 ----
    // 底数为 1 与 64 各测一次：底数 1 时加成公式只剩「加多少个」，底数 64 时才真正考验乘法。
    {
        unit: "apply_bonus#ore_drops",
        item: { id: "minecraft:stone", count: 64 },
        looting: 3,
        function: fn("apply_bonus", { enchantment: "minecraft:fortune", formula: "minecraft:ore_drops" }),
    },
    {
        unit: "apply_bonus#uniform_bonus",
        item: { id: "minecraft:stone", count: 1 },
        looting: 3,
        function: fn("apply_bonus", {
            enchantment: "minecraft:fortune",
            formula: "minecraft:uniform_bonus_count",
            parameters: { bonusMultiplier: 2 },
        }),
    },
    {
        unit: "apply_bonus#binomial_with_bonus",
        looting: 3,
        function: fn("apply_bonus", {
            enchantment: "minecraft:fortune",
            formula: "minecraft:binomial_with_bonus_count",
            parameters: { extra: 3, probability: 0.5 },
        }),
    },
    {
        // 没有「不加成」这一档：原版只注册了三个公式（ore_drops / uniform_bonus_count /
        // binomial_with_bonus_count）。这里补一个「掠夺等级为 0」的场景——公式存在但
        // 等级为 0 时应当原样返回，且**不消费随机数**。
        unit: "apply_bonus#zero_level",
        item: { id: "minecraft:stone", count: 7 },
        function: fn("apply_bonus", { enchantment: "minecraft:fortune", formula: "minecraft:ore_drops" }),
    },

    // ---- 上界裁剪与爆炸衰减 ----
    {
        unit: "limit_count#range",
        item: { id: "minecraft:stone", count: 9 },
        function: fn("limit_count", { limit: { min: 2, max: 5 } }),
    },
    {
        unit: "limit_count#only_max",
        item: { id: "minecraft:stone", count: 20 },
        function: fn("limit_count", { limit: { max: 7 } }),
    },
    {
        unit: "explosion_decay#plain",
        item: { id: "minecraft:stone", count: 32 },
        explosion_radius: 4,
        function: fn("explosion_decay"),
    },

    // ---- 伤害 ----
    {
        unit: "set_damage#fraction",
        item: { id: "minecraft:diamond_sword", count: 1 },
        function: fn("set_damage", { damage: 0.5, add: false }),
    },
    {
        // 反向映射：走 pickDamage 的 right - floor(rand * (right - left + 1)) 分支。
        unit: "set_damage#uniform",
        item: { id: "minecraft:diamond_sword", count: 1 },
        function: fn("set_damage", { damage: { type: "minecraft:uniform", min: 0.1, max: 0.9 }, add: true }),
    },

    // ---- 附魔 ----
    {
        unit: "enchant_with_levels#uniform",
        item: { id: "minecraft:book", count: 1 },
        function: fn("enchant_with_levels", {
            levels: { type: "minecraft:uniform", min: 10, max: 25 },
            treasure: false,
        }),
    },
    {
        unit: "enchant_with_levels#with_treasure",
        item: { id: "minecraft:book", count: 1 },
        function: fn("enchant_with_levels", {
            levels: { type: "minecraft:uniform", min: 20, max: 39 },
            treasure: true,
        }),
    },
    {
        unit: "enchant_randomly#plain",
        item: { id: "minecraft:book", count: 1 },
        function: fn("enchant_randomly"),
    },
    {
        unit: "enchant_randomly#options",
        item: { id: "minecraft:book", count: 1 },
        function: fn("enchant_randomly", { options: ["minecraft:sharpness", "minecraft:mending"] }),
    },
    {
        unit: "set_enchantments#levels",
        item: { id: "minecraft:book", count: 1 },
        function: fn("set_enchantments", { enchantments: { "minecraft:sharpness": 3 } }),
    },
    {
        unit: "enchanted_count_increase#limit",
        item: { id: "minecraft:stone", count: 1 },
        looting: 3,
        function: fn("enchanted_count_increase", {
            enchantment: "minecraft:looting",
            count: { type: "minecraft:uniform", min: 0, max: 1 },
        }),
    },

    // ---- 数据与组件 ----
    {
        unit: "set_custom_data#compound",
        // 原版这一项的 codec 是 `TagParser.LENIENT_CODEC`，字段形态是 **SNBT 文本**，
        // 不是 JSON 对象（对象形态是旧版写法）。
        function: fn("set_custom_data", { tag: "{DiffProbe:{level:3,tag:loot}}" }),
    },
    {
        unit: "set_components#stack_size",
        function: fn("set_components", { components: { "minecraft:max_stack_size": 16 } }),
    },
    {
        unit: "set_name#fixed",
        function: fn("set_name", { name: { text: "Diff Name", color: "gold" } }),
    },
    {
        unit: "set_lore#fixed",
        function: fn("set_lore", {
            lore: [{ text: "line one" }, { text: "line two", italic: false }],
            mode: "replace_all",
        }),
    },
    {
        unit: "set_potion#id",
        item: { id: "minecraft:potion", count: 1 },
        function: fn("set_potion", { id: "minecraft:strong_swiftness" }),
    },
    {
        unit: "set_instrument#tag",
        item: { id: "minecraft:goat_horn", count: 1 },
        // 原版 TagKey 形态：`"#命名空间:标签"`。取数据包里真实存在的山羊角标签；
        // 用内置的空标签或自造的标签名会变成「两侧都查不到」的伪差异。
        function: fn("set_instrument", { options: "#minecraft:goat_horns" }),
    },
    {
        unit: "set_stew_effect#single",
        item: { id: "minecraft:suspicious_stew", count: 1 },
        function: fn("set_stew_effect", { effects: [{ type: "minecraft:regeneration", duration: 100 }] }),
    },
    {
        unit: "set_ominous_bottle_amplifier#range",
        item: { id: "minecraft:ominous_bottle", count: 1 },
        function: fn("set_ominous_bottle_amplifier", { amplifier: { type: "minecraft:uniform", min: 0, max: 4 } }),
    },

    // ---- 物品替换与熔炼 ----
    {
        unit: "furnace_smelt#raw_beef",
        item: { id: "minecraft:beef", count: 3 },
        function: fn("furnace_smelt"),
    },
    {
        unit: "set_item#swap",
        item: { id: "minecraft:stone", count: 2 },
        function: fn("set_item", { item: "minecraft:diamond" }),
    },
    { unit: "discard#always", item: { id: "minecraft:stone", count: 1 }, function: fn("discard") },

    // ---- 容器与结构 ----
    {
        unit: "set_contents#single",
        item: { id: "minecraft:chest", count: 1 },
        function: fn("set_contents", {
            component: "minecraft:container",
            entries: [{ type: "minecraft:item", name: "minecraft:apple" }],
        }),
    },
    {
        unit: "set_loot_table#inline",
        item: { id: "minecraft:chest", count: 1 },

        function: fn("set_loot_table", {
            name: "minecraft:chests/simple_dungeon",
            seed: 42,
            type: "minecraft:chest",
        }),
    },
    {
        unit: "exploration_map#beach",
        item: { id: "minecraft:map", count: 1 },
        function: fn("exploration_map", {
            destination: "minecraft:ocean_ruin",
            decoration: "mansion",
            zoom: 2,
            search_radius: 50,
            skip_existing_chunks: true,
        }),
    },

    // ---- 组合与传递 ----
    {
        unit: "sequence#two",
        item: { id: "minecraft:stone", count: 1 },
        function: fn("sequence", {
            functions: [fn("set_count", { count: 4 }), fn("limit_count", { limit: { max: 2 } })],
        }),
    },
    {
        unit: "filtered#pass",
        item: { id: "minecraft:stone", count: 1 },
        function: fn("filtered", {
            on_pass: fn("set_count", { count: 6 }),
            on_fail: fn("set_count", { count: 2 }),
            item_filter: { items: "minecraft:stone" },
        }),
    },
    {
        unit: "filtered#block",
        item: { id: "minecraft:stone", count: 1 },
        function: fn("filtered", {
            on_pass: fn("set_count", { count: 6 }),
            on_fail: fn("set_count", { count: 2 }),
            item_filter: { items: "minecraft:dirt" },
        }),
    },
    {
        unit: "copy_name#this",
        item: { id: "minecraft:stone", count: 1 },
        function: fn("copy_name", { source: "this" }),
    },
    {
        unit: "copy_custom_data#this",
        item: { id: "minecraft:stone", count: 1 },
        function: fn("copy_custom_data", { source: "this", ops: [{ op: "merge", source: "this", target: "thing" }] }),
    },
    {
        unit: "copy_state#this",
        item: { id: "minecraft:stone", count: 1 },
        function: fn("copy_state", { block: "minecraft:oak_log", properties: ["axis"] }),
    },
    {
        unit: "fill_player_head#this",
        item: { id: "minecraft:player_head", count: 1 },
        function: fn("fill_player_head", { entity: "this" }),
    },
];

/** 场景单元名（与探针 / oracle 的 SCENARIO_UNITS 逐字一致，靠差分本身保真）。 */
export const lootFunctionUnitNames: readonly string[] = LOOT_FUNCTIONS.map((scenario) => scenario.unit);

/** 单元 → 探针 / oracle 的共同输入。 */
export function lootFunctionInput(unit: string): unknown {
    const scenario = LOOT_FUNCTIONS.find((candidate) => candidate.unit === unit);
    if (scenario === undefined) {
        return { unit };
    }
    const input: Record<string, unknown> = { unit };
    if (scenario.item !== undefined) {
        input["item"] = scenario.item;
    }
    input["function"] = scenario.function;
    if (scenario.looting !== undefined) {
        input["looting"] = scenario.looting;
    }
    if (scenario.luck !== undefined) {
        input["luck"] = scenario.luck;
    }
    if (scenario.explosion_radius !== undefined) {
        input["explosion_radius"] = scenario.explosion_radius;
    }
    return input;
}

/**
 * 比对前的规范化。
 *
 * 失败与成功是同一个字段的两个取值：解析失败时两侧都只给 `{unit, ok:false, error}`，
 * 而「原版解析失败、本侧成功」这种真正的差异两侧键集合不同——比较层只比参照侧声明过
 * 的字段，缺 `samples` 的一侧会被当成「没得比」而静默通过。因此把两段的键在**两侧**
 * 都补齐，缺失显式落成 null。
 */
export function normaliseLootFunction(_expected: unknown, actual: unknown): unknown {
    if (actual === null || typeof actual !== "object") {
        return actual;
    }
    const record = actual as Record<string, unknown>;
    return {
        unit: record["unit"] ?? null,
        ok: record["ok"] ?? null,
        error: record["error"] ?? null,
        samples: record["samples"] ?? null,
    };
}

normaliseLootFunction.canonicalising = true;