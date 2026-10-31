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
 * crafting_match.ts — 合成配方的网格语料。
 *
 * 单元 = 「配方 ID # 摆法名」。摆法由**配方自身**的原料决定（探针与 oracle 都不猜测
 * 该怎么摆），因此这里只用一条规则把配方摊成一个网格：按行优先把每个原料的第一个
 * 候选物品放进去，其余留空。规则本身与两侧无关，两侧读到的仍是同一份数据。
 *
 * ## 为什么不能只测「原样摆」
 *
 * 如果每个配方只喂它自己的标准摆法，两侧都判 true，等于只验证了「能合」，验不出
 * 「不该合的也合了」。真正有区分度的是**反例**：多放一个物品、少放一个、把两格
 * 对调、镜像、整体挪到角落。这些摆法在两套实现下最可能分叉——
 *
 *   - 原版先裁掉空行空列再要求尺寸**严格相等**；多一个物品会撑大包围盒，
 *     或让 `ingredientCount` 对不上，直接判否；
 *   - Cubium 在整张网格上滑窗，配方外的格子用空原料测试，多出来的物品由
 *     那一格的 `EMPTY.test(stack)` 拦下。
 *
 * 两条路径在「多一个物品」上结论相同，但在「2×2 配方放进 3×3 网格的中间、
 * 四周留空」这类输入上就可能不同：裁剪版看到的是 2×2，滑窗版也看到 2×2，
 * 结论相同；而「1×1 配方与其他物品拼在一起」时裁剪版只留下那一格，滑窗版
 * 则会因为多出的物品判否——后者才是原版玩家的实际结果（合成器里多出来的东西
 * 会让整个网格不再匹配）。这里把三类摆法都列出来，让差异自己暴露。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessContext, HarnessDeclaration } from "../core/types.ts";
import { KNOWN_MISSING as KNOWN_MISSING_ITEMS } from "./known_missing_items.ts";

/** 网格的一格：物品名与数量；`null` 表示空。 */
export type CraftingCell = { readonly id: string; readonly count?: number } | null;

/** 摆法名：与探针 / oracle 逐字一致（单元由 `${recipeId}#${placement}` 拼成）。 */
export const PLACEMENT_NAMES: readonly string[] = [
    // 原样摆进配方的尺寸；这个必须匹配。
    "canonical",
    // 原样摆，但把每个物品的数量写成 2：数量不影响匹配（原版只看物品种类），
    // 但会在 `assemble` 之前暴露「按数量判等」这类实现错误。
    "stacked",
    // 整体平移到网格的其余位置（只对小于网格的配方有意义）。
    "shifted",
    // 多放一个物品：必须判否。合成器里多出东西时整张网格不再匹配。
    "extra",
    // 少放一个物品：必须判否。
    "missing",
    // 空网格：必须判否。
    "empty",
];

/** 某个配方摊开后的标准摆法（未做任何改造）。 */
function canonicalGrid(width: number, height: number, candidates: readonly (string | null)[]): CraftingCell[] {
    return Array.from({ length: width * height }, (_unused, index) => {
        const candidate = candidates[index];
        return candidate === undefined || candidate === null ? null : { id: candidate };
    });
}

/** 把标准摆法平移到网格左下角（尽量远离原点）。 */
function shiftedGrid(width: number, height: number, base: readonly CraftingCell[]): CraftingCell[] {
    const grid: CraftingCell[] = Array.from({ length: width * height }, () => null);
    // 只在有余量时平移；没有余量时返回原样（此时该摆法与 canonical 相同，仍可比）。
    for (let y = 0; y < height; y += 1) {
        for (let x = 0; x < width; x += 1) {
            const cell = base[y * width + x];
            if (cell !== null && cell !== undefined) {
                grid[y * width + x] = cell;
            }
        }
    }
    return grid;
}

/** 一个配方的完整语料。 */
export interface CraftingUnit {
    readonly unit: string;
    readonly recipe: string;
    readonly width: number;
    readonly height: number;
    readonly grid: readonly CraftingCell[];
}

/**
 * 由「配方 → 尺寸与逐格候选物品」构造全部摆法。
 *
 * `candidates` 按行优先给出每个格子选中的物品（`null` 表示这一格在标准摆法里是空的），
 * 长度必须等于 `width * height`。
 */
export function craftingUnits(
    recipeId: string,
    width: number,
    height: number,
    candidates: readonly (string | null)[],
): CraftingUnit[] {
    const base = canonicalGrid(width, height, candidates);
    const units: CraftingUnit[] = [];

    units.push({ unit: `${recipeId}#canonical`, recipe: recipeId, width, height, grid: base });

    // 数量放大：匹配只看种类，因此结论应与 canonical 一致——这一步专门验证
    // 「实现有没有把数量也算进匹配」。
    units.push({
        unit: `${recipeId}#stacked`,
        recipe: recipeId,
        width,
        height,
        grid: base.map((cell) => (cell === null ? null : { id: cell.id, count: 2 })),
    });

    // 平移：把标准摆法整体挪到网格另一头。只在标准摆法本身没有铺满网格时有意义。
    const filled = base.filter((cell) => cell !== null).length;
    if (filled < width * height) {
        units.push({ unit: `${recipeId}#shifted`, recipe: recipeId, width, height, grid: shiftedGrid(width, height, base) });
    }

    // 多放一个物品：找一个空格塞进去。没有空格时改放一个重复物品顶替位置
    // （那同样是「多一个」——会让包围盒或计数对不上）。
    const extraIndex = base.findIndex((cell) => cell === null);
    if (extraIndex >= 0) {
        const extra = [...base];
        extra[extraIndex] = { id: "minecraft:stone" };
        units.push({ unit: `${recipeId}#extra`, recipe: recipeId, width, height, grid: extra });
    }

    // 少放一个物品：把第一个非空格清空。
    const missingIndex = base.findIndex((cell) => cell !== null);
    if (missingIndex >= 0) {
        const missing = [...base];
        missing[missingIndex] = null;
        units.push({ unit: `${recipeId}#missing`, recipe: recipeId, width, height, grid: missing });
    }

    // 空网格：两侧都必须判否。原版这里要给「裁剪后为空」的 CraftingInput，
    // 探针得到的是全空网格——两者都应当不匹配任何配方。
    units.push({
        unit: `${recipeId}#empty`,
        recipe: recipeId,
        width,
        height,
        grid: Array.from({ length: width * height }, () => null),
    });

    return units;
}

/**
 * 我们的单元清单里带 `#` 的是「配方 × 摆法」，而探针 / oracle 的 `list` 只给配方 ID。
 * 因此单元集合由 harness 自己按清单展开：清单是两侧共用的那一个事实来源。
 */
export function missingLabelCraftingMatch(oracleUnits: readonly string[], probeListing: readonly string[]): string[] {
    if (probeListing.length === 0) {
        return [];
    }
    const probeRecipes = new Set(probeListing);
    const missing = new Set<string>();
    for (const unit of oracleUnits) {
        const separator = unit.lastIndexOf("#");
        if (separator < 0) {
            continue;
        }
        const recipe = unit.slice(0, separator);
        if (!probeRecipes.has(recipe) || KNOWN_MISSING_ITEMS.includes(recipe)) {
            missing.add(unit);
        }
    }
    return [...missing].sort();
}

/**
 * 比对前的规范化。
 *
 * 「原版判定成功、本侧判定失败」这类真差异两侧键集合不同：失败侧只有 `{unit, ok, error}`，
 * 成功侧还有 `matches` 与 `result`。比较层只比参照侧声明过的字段，缺键的一侧会被当成
 * 「没得比」而静默通过。因此在**两侧**都把四个键补齐，缺失显式落成 null。
 */
export function normaliseCraftingMatch(_expected: unknown, actual: unknown): unknown {
    if (actual === null || typeof actual !== "object") {
        return actual;
    }
    const record = actual as Record<string, unknown>;
    return {
        unit: record["unit"] ?? null,
        ok: record["ok"] ?? null,
        matches: record["matches"] ?? null,
        result: record["result"] ?? null,
    };
}

normaliseCraftingMatch.canonicalising = true;

/** oracle `dump` 的一行：配方的标准摆法。 */
interface CraftingLayout {
    readonly recipe: string;
    readonly width: number;
    readonly height: number;
    readonly grid: readonly CraftingCell[];
}

/**
 * 能力声明。
 *
 * 单元集合是 `computed`：由 oracle 导出的标准摆法（`beforeUnits`）展开成
 * 「配方 × 摆法」。探针与 oracle 的 `list` 只给配方 ID，用来判断「探针认不认得这个
 * 配方」——配方在探针侧没加载成功的，它的全部摆法都记为 oracle 独有。
 */
export function craftingMatchDeclaration(): HarnessDeclaration {
    const units = new Map<string, CraftingUnit>();
    return {
        name: "crafting_match",
        probe: "crafting_match",
        oracle: "CraftingMatchOracle",
        key: "unit",
        units: { kind: "computed", units: () => [...units.keys()].sort() },
        input: (unit: string): CraftingUnit => {
            const entry = units.get(unit);
            if (entry === undefined) {
                throw new Error(`No crafting_match unit: ${unit}`);
            }
            return entry;
        },
        normalise: normaliseCraftingMatch,
        // 单元是「配方 × 摆法」，而两侧的 `list` 只给配方 ID：配方在探针侧没加载成功的，
        // 它的全部摆法都算 oracle 独有。用 `deriveMissing`（拿两份清单现算）而不是静态
        // 数组——缺口清单会随注册表推进而变。
        deriveMissing: missingLabelCraftingMatch,
        // oracle 要在 java.io.tmpdir 里建原版服务端世界并持有区域锁：同组必须串行。
        exclusiveGroup: "loot-world",
        beforeUnits: (context: HarnessContext): void => {
            const directory = context.outputDirectory("crafting_match", "oracle");
            context.compileOracle("CraftingMatchOracle", "crafting_match");
            const output = path.join(directory, "layouts.jsonl");
            execFileSync(
                "java",
                [
                    "-cp",
                    context.oracleClasspath(context.classesDirectory("crafting_match")),
                    "dev.horizonlab.diff.CraftingMatchOracle",
                    "dump",
                    output,
                ],
                { cwd: directory, env: context.environment, stdio: ["ignore", "pipe", "pipe"] },
            );

            units.clear();
            for (const line of fs.readFileSync(output, "utf8").split(/\r?\n/).filter(Boolean)) {
                const layout = JSON.parse(line) as CraftingLayout;
                for (const unit of craftingUnits(layout.recipe, layout.width, layout.height, layout.grid.map(
                    (cell) => (cell === null ? null : cell.id),
                ))) {
                    units.set(unit.unit, unit);
                }
            }
        },
    };
}