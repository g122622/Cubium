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
 * item_component_nbt.ts — 组件补丁 NBT 表示的共同输入。
 *
 * 单元是「物品#变体」。变体决定往物品堆上施加哪一组组件，**取值只写在这里**：
 *   - 探针按 components 逐组件调用 ItemStack::setComponent（oracle 侧则是把它们喂给
 *     ItemStack.CODEC 解析）；
 *   - 两侧读同一条 inputs.jsonl，因此不可能出现「一边放 A、一边放 B」这种伪差异。
 *
 * 三个变体的分工：
 *   - plain  ：不施加任何组件。校验「没有组件时不写出 components 段」；
 *   - suite  ：施加一组固定的非默认值。校验组件名、各组件自己的 NBT 结构；
 *   - removed：在 suite 之上再主动移除原型的组件。校验 `!name` 形式的移除键。
 *
 * 为什么取值必须非默认：值等于原型的默认值时，原版补丁里根本不会出现这个键
 * （`getComponentsPatch()` 只报相对原型的差异），那就测不到组件本身了。
 */

import { canonicalSnbt } from "../core/snbt.ts";
import { KNOWN_MISSING as STATIC_MISSING_ITEMS } from "./known_missing_items.ts";

/** 全体变体名（与 ItemComponentNbtOracle / item_component_nbt.cpp 的常量一致）。 */
export const VARIANTS: readonly string[] = ["plain", "suite", "removed"];

/**
 * suite 施加的组件。
 *
 * 只放 Cubium 已经结构化落地的组件：未落地的组件在探针侧会直接报错（而不是静默跳过），
 * 那会变成一整片「探针多出错误」的噪声。新增组件时先确认 `DataComponentPatchNbt.cpp` 的
 * `payloadToNbt` 有对应分支，再往这里加。
 *
 * 字符串里的 JSON 是数据包写法；探针与 oracle 各自按自己的 codec 解析同一份结构。
 */
export const SUITE_COMPONENTS: Readonly<Record<string, unknown>> = {
    "minecraft:damage": 7,
    "minecraft:repair_cost": 3,
    "minecraft:max_stack_size": 42,
    "minecraft:max_damage": 321,
    "minecraft:unbreakable": {},
    "minecraft:custom_name": "{\"text\":\"Diff Name\",\"color\":\"gold\"}",
    "minecraft:lore": [
        "{\"text\":\"line one\"}",
        "{\"text\":\"line two\",\"italic\":false}",
    ],
    // 附魔表的持久形态是平铺的 {附魔: 等级}（ItemEnchantments.CODEC），与数据包里
// `set_enchantments` 函数用的 {levels: {...}} 不是同一种写法。
    "minecraft:enchantments": { "minecraft:sharpness": 4, "minecraft:unbreaking": 3 },
    "minecraft:stored_enchantments": { "minecraft:mending": 1 },
    "minecraft:rarity": "epic",
    "minecraft:item_model": "minecraft:diff/probe",
    // Enchantable 是 {value: int} 记录，不是裸整数。
    "minecraft:enchantable": { value: 17 },
    "minecraft:enchantment_glint_override": true,
    "minecraft:custom_data": { DiffProbe: { level: 3, tag: "suite" } },
    // can_place_on / can_break 是 BlockPredicate 列表。
    "minecraft:can_place_on": [{ blocks: ["minecraft:stone", "minecraft:dirt"] }],
    "minecraft:can_break": [{ blocks: "#minecraft:logs" }],
    // 持久形态是裸列表（ItemAttributeModifiers.CODEC = Entry.CODEC.listOf()）。
    // TODO: attribute_modifiers 在 Cubium 侧尚未落地组件编解码（DataComponentPayloadCodec
    //       的 payloadToNbt/nbtToPayload 没有该分支，载荷会落成 monostate），因此暂时不放进
    //       固定组件表；补上编解码后把下面这段放回去，并删掉这条 TODO。
    // "minecraft:attribute_modifiers": [
    //     {
    //         type: "minecraft:max_health",
    //         id: "minecraft:diff/probe",
    //         amount: 3.5,
    //         operation: "add_value",
    //         slot: "mainhand",
    //     },
    // ],
    "minecraft:custom_model_data": { floats: [1.5, 2.25], flags: [true, false], strings: ["a", "b"] },
    "minecraft:potion_contents": { potion: "minecraft:strong_swiftness", custom_color: 16711935 },
    "minecraft:ominous_bottle_amplifier": 3,
    "minecraft:firework_explosion": {
        shape: "large_ball",
        colors: [16711680, 65280],
        fade_colors: [255],
        has_trail: true,
        has_twinkle: true,
    },
    "minecraft:fireworks": {
        flight_duration: 2,
        explosions: [{ shape: "star", colors: [16777215] }],
    },
    "minecraft:instrument": "minecraft:ponder_goat_horn",
    "minecraft:attack_range": {
        min_reach: 1.5,
        max_reach: 4.0,
        min_creative_reach: 2.0,
        max_creative_reach: 5.0,
        hitbox_margin: 0.25,
        mob_factor: 0.75,
    },
    // Tool.Rule 的持久形态：blocks 直接是标签/方块 ID（撑开成同一列表，**不带** "#"，
// 读写两侧都按「是不是已注册的方块标签」区分），这里只放方块 ID 以免产生歧义。
    "minecraft:tool": {
        rules: [{ blocks: ["minecraft:stone", "minecraft:dirt"], speed: 6.5, correct_for_drops: true }],
        default_mining_speed: 1.25,
        damage_per_block: 2,
        can_destroy_blocks_in_creative: false,
    },
};

/**
 * 「壳」用的最小组件集：栈编解码（stack_codec）只关心 `components` 段在不在、
 * 长什么样，不需要整张组件表。取三项最容易解出文本、且所有物品能承载的组件。
 */
export const SHELL_COMPONENTS: Readonly<Record<string, unknown>> = {
    "minecraft:damage": 7,
    "minecraft:rarity": "epic",
    "minecraft:custom_data": { DiffProbe: { level: 3, tag: "suite" } },
};

/**
 * removed 变体在 suite 之上额外移除的组件。
 *
 * 取所有物品原型都会带的几项：组件的「移除」在存档里写作 `"!name"`，与「没有这个组件」
 * 是两种状态（前者的语义是「覆盖掉原型自带的默认值并置空」）。
 */
export const REMOVED_COMPONENTS: readonly string[] = [
    "minecraft:max_stack_size",
    "minecraft:max_damage",
    "minecraft:rarity",
];

/**
 * 单元级的已知缺口：原版有、Cubium 没注册的物品（静态清单 + 运行时多出来的）。
 *
 * `oracleMissing` 按**完整单元键**匹配，而这里的单元是「物品#变体」，所以要把每个
 * 缺失物品展开成它的三个变体。静态清单来自 known_missing_items.ts；运行时多出来的
 * 部分由探针自己的 `list` 与 oracle 的 `list` 求差得到——两处都不能靠猜。
 */
export function missingLabelComponents(
    oracleUnits: readonly string[],
    probeListing: readonly string[],
): string[] {
    const known = new Set(STATIC_MISSING_ITEMS);
    const available = new Set(probeListing.map((unit) => splitUnit(unit).item));
    const result: string[] = [];
    for (const unit of oracleUnits) {
        const { item, variant } = splitUnit(unit);
        if (!known.has(item)) {
            continue;
        }
        // 探针其实建得出来的物品不算缺口（清单可能滞后于注册表），交给比对去报。
        if (available.has(item)) {
            continue;
        }
        result.push(`${item}#${variant}`);
    }
    return result;
}

/**
 * 比对前的规范化：把每个组件的 SNBT 重写成唯一规范形。
 *
 * 两侧的 SNBT 打印器不同（原版 `StringTagVisitor` 用单引号、Cubium 用双引号转义、
 * 浮点后缀写法也不同），不规范化的话每一行都会因为「同一个标签印法不同」而报差异。
 * 规范化对原版是等价变换（`normalisationIsFaithful` 会校验这一点）。
 */
export function normaliseComponentSnbt(_expected: unknown, actual: unknown): unknown {
    return canonicaliseOne(actual);
}

/** 对单个结果行做规范形变换（不修改入参）。 */
function canonicaliseOne(value: unknown): unknown {
    if (value === null || typeof value !== "object") {
        return value;
    }
    const record = value as Record<string, unknown>;
    if (record["components"] === undefined || record["components"] === null) {
        return value;
    }
    const components = record["components"] as Record<string, unknown>;
    const canonical: Record<string, unknown> = {};
    for (const [key, raw] of Object.entries(components)) {
        canonical[key] = typeof raw === "string" ? (canonicalSnbt(raw) ?? raw) : raw;
    }
    return { ...record, components: canonical };
}

normaliseComponentSnbt.canonicalising = true;

/** 单元键拆分。 */
function splitUnit(unit: string): { item: string; variant: string } {
    const hash = unit.lastIndexOf("#");
    return { item: unit.slice(0, hash), variant: unit.slice(hash + 1) };
}

/** 单元 → 探针 / oracle 的共同 stack 描述。 */
export function itemComponentStackInput(unit: string): unknown {
    const { item, variant } = splitUnit(unit);
    const stack: Record<string, unknown> = { id: item };

    if (variant === "suite" || variant === "removed") {
        const components: Record<string, unknown> = { ...SUITE_COMPONENTS };
        if (variant === "removed") {
            // 移除只落在 suite 没设的那几项上：同一个组件不能既 added 又 removed（原版
            // patch 的 map 一项只有一个值）。探针与 oracle 因此都只需要「先批量 set」，
            // 再批量 remove，不存在顺序歧义。
            for (const name of REMOVED_COMPONENTS) {
                delete components[name];
                components[`!${name}`] = {};
            }
        }
        stack["components"] = components;
    }
    return { unit, stack };
}