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
 * stack_codec.ts — 物品堆整体编解码（原版 `ItemStack.CODEC`）的共同输入。
 *
 * 单元分两类：
 *   1. **按物品枚举**（list 给出）：`<物品>#base` 与 `<物品>#suite`，覆盖全部物品；
 *   2. **固定场景**（computed 单元）：数量区间边界、坏数据、未知键——与具体物品无关，
 *      单个物品重复跑一遍即可。
 *
 * 变体决定喂给 `ItemStack.CODEC` 的 NBT：
 *   - base  ：只给 `id`。校验 count 缺省为 1、components 缺省为空；
 *   - suite ：`id` + `count` + 组件表全给。校验三个字段都在场时的编码；
 *   - 场景  ：见 `SCENARIOS`，每个场景自带完整的 stack 描述。
 *
 * 与 item_component_nbt 的分工：那边比组件载荷本身，这边比**包着组件的那层壳**——
 * 数量、空堆、`components` 段的省略、解码失败的边界。shell 写错在运行时不会报错，
 * 只会让存档里的数量被截断或被静默接受。
 */

import { canonicalSnbt } from "../core/snbt.ts";
import { KNOWN_MISSING as STATIC_MISSING_ITEMS } from "./known_missing_items.ts";
import { SHELL_COMPONENTS } from "./item_component_nbt.ts";

/** 变体名（与 StackCodecOracle / stack_codec.cpp 的常量一致）。 */
export const ITEM_VARIANTS: readonly string[] = ["base", "suite"];

/** suite 变体的数量。取一个既不等于 1、也不超过 99 的值，让 `count` 确实需要写出。 */
const SUITE_COUNT = 5;

/**
 * 固定场景：数量的区间边界与坏数据。
 *
 * 原版 `ItemStack.CODEC` 的 count 是 `ExtraCodecs.intRange(1, 99)`，且在
 * `STRICT_CODEC` 里还会校验「不可既毁损又可堆叠」。这些边界只在少数取值上暴露，
 * 逐个物品重复没有意义，因此单独列出。
 */
export const SCENARIOS: ReadonlyArray<{ readonly unit: string; readonly stack: unknown }> = [
    // count 的区间：1 与 99 是端点。原版 codec 是 `intRange(1, 99).orElse(1)`，
    // 而 `orElse` 对越界是**宽容**的——0 / 100 / -1 都退回 1，不回退也不报错。
    { unit: "count#min", stack: { id: "minecraft:stone", count: 1 } },
    { unit: "count#max", stack: { id: "minecraft:stone", count: 99 } },
    { unit: "count#zero", stack: { id: "minecraft:stone", count: 0 } },
    { unit: "count#over", stack: { id: "minecraft:stone", count: 100 } },
    { unit: "count#negative", stack: { id: "minecraft:stone", count: -1 } },
    // count 缺省：应当按 1 处理。
    { unit: "count#absent", stack: { id: "minecraft:stone" } },
    // 顶层多余键：原版 CODEC 忽略未声明的字段，不应因此失败。
    { unit: "shell#extra-key", stack: { id: "minecraft:stone", count: 3, Slots: [], foo: "bar" } },
    // 空堆：air 在 CODEC 层是「合法的物品」，但 `ItemStack.CODEC` 显式拒绝它。
    { unit: "shell#air", stack: { id: "minecraft:air", count: 1 } },
    // 未知物品：原版按注册表查不到即失败。
    { unit: "shell#unknown-item", stack: { id: "minecraft:not_an_item", count: 1 } },
    // 组件段的形态：空对象应当与缺省等价。
    { unit: "shell#empty-components", stack: { id: "minecraft:stone", count: 2, components: {} } },
    // 不可堆叠（有耐久度）的物品给 count > 1：CODEC 本身不管，STRICT_CODEC 才管。
    { unit: "shell#damageable-stacked", stack: { id: "minecraft:diamond_sword", count: 5 } },
];

/**
 * 单元级缺口：原版有、Cubium 没注册的物品。
 *
 * 探针在这些物品的 `base` / `suite` 上输出 `missing`；固定场景不与物品绑定，不参与。
 */
export function missingLabelStackCodec(
    oracleUnits: readonly string[],
    probeListing: readonly string[],
): string[] {
    // 「探针认不认得这个物品」看它的 `#base` 单元在不在探针的清单里；`#suite` 只在这之后追加。
    const listed = new Set(probeListing.filter((unit) => unit.endsWith("#base")).map((unit) => unit.slice(0, -5)));
    const result: string[] = [];
    for (const unit of oracleUnits) {
        if (!unit.endsWith("#base") && !unit.endsWith("#suite")) {
            continue;
        }
        const item = unit.slice(0, unit.lastIndexOf("#"));
        const registered = listed.has(item);
        // 静态清单会滞后于注册表（补了物品没删清单是常态），因此两条判据取或：
        // 清单里点名了，或探针的 list 里根本没有它。
        if (!registered || STATIC_MISSING_ITEMS.includes(item)) {
            result.push(unit);
        }
    }
    return result;
}

/**
 * 比对前的规范化：把结果里的每一段 NBT / SNBT 文本重写成唯一规范形。
 *
 * 三处需要规范化，都是「同一个值、两种印法」而不是真差异：
 *   - `decoded.components[*]`：组件载荷 SNBT（引号风格、浮点后缀）；
 *   - `reencode.nbt`：整段物品堆 NBT 文本。
 *
 * `reencode.nbt` 的顶层键序（`id` / `count` / `components`）不参与比较：两侧的 map
 * 容器不同，键的输出顺序没有语义。这里把顶层拆成按键排序的对象再逐个规范化其值。
 */
export function normaliseStackCodec(_expected: unknown, actual: unknown): unknown {
    return canonicaliseStack(actual);
}

function canonicaliseStack(value: unknown): unknown {
    if (value === null || typeof value !== "object") {
        return value;
    }
    const record = value as Record<string, unknown>;
    // 比较层只比参照侧声明过的字段：参照侧失败时只有 `{unit, ok:false}`，被测侧成功时
    // 有 `decoded` / `reencode`——三段的键必须在**两侧**都补齐，否则「原版失败、本侧成功」
    // 会被当成「原版没说这几段，没得比」而静默通过。补齐后的 null 与缺失是两种状态。
    const out: Record<string, unknown> = {
        unit: record["unit"] ?? null,
        ok: record["ok"] ?? null,
        decoded: record["decoded"] ?? null,
        reencode: record["reencode"] ?? null,
    };

    const decoded = record["decoded"];
    if (decoded !== null && typeof decoded === "object") {
        const inner = decoded as Record<string, unknown>;
        const components = inner["components"];
        if (components !== null && typeof components === "object") {
            const canonical: Record<string, unknown> = {};
            for (const [key, raw] of Object.entries(components as Record<string, unknown>)) {
                canonical[key] = typeof raw === "string" ? (canonicalSnbt(raw) ?? raw) : raw;
            }
            out["decoded"] = { ...inner, components: canonical };
        }
    }

    const reencode = record["reencode"];
    if (reencode !== null && typeof reencode === "object") {
        const inner = reencode as Record<string, unknown>;
        const text = inner["nbt"];
        if (typeof text === "string") {
            out["reencode"] = { ...inner, nbt: canonicalSnbt(text) ?? text };
        }
    }
    return out;
}

normaliseStackCodec.canonicalising = true;

/** 按物品枚举的单元 → 探针 / oracle 的共同 stack 描述。 */
export function stackCodecInput(unit: string): unknown {
    const hash = unit.lastIndexOf("#");
    const item = hash < 0 ? unit : unit.slice(0, hash);
    const variant = hash < 0 ? "" : unit.slice(hash + 1);

    const fixed = SCENARIOS.find((scenario) => scenario.unit === unit);
    if (fixed !== undefined) {
        return { unit, stack: fixed.stack };
    }

    const stack: Record<string, unknown> = { id: item };
    if (variant === "suite") {
        // 只放「物品一切原型都带」的那几个组件：它们的值已被组件层（item_component_nbt）逐字
        // 校验过，这里要验证的是「壳」——count 与非空 components 段同时在场的编码。
        // 不用 SUITE_COMPONENTS 的整张表：那份表里有几项在原版侧解不出文本（需要注册表或
        // 世界上下文），会把壳的比对淹没在组件层的差异里。
        stack["count"] = SUITE_COUNT;
        stack["components"] = { ...SHELL_COMPONENTS };
    }
    return { unit, stack };
}



