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
 * single_item_recipe.ts — 单物品配方（熔炼 / 高炉 / 烟熏 / 营火 / 切石）的查询语义。
 *
 * 单元是**输入物品**，不是配方 ID。两侧各自回答「拿着这个物品，炉子会烧出什么、
 * 切石机上有哪些选项」。
 *
 * ## 与 recipe 能力的分工
 *
 * `recipe` 按配方 ID 逐个导出「这条配方长什么样」。它能发现「某条配方的烹饪时间写错了」，
 * 但发现不了：
 *
 *   - **同一个输入命中多条配方时，实际返回哪一条**。查询走 findFirst，结果取决于注册
 *     顺序；顺序一变，玩家烧出来的就是别的东西，而两条配方本身都没错。
 *   - **配方有没有被装进正确的类型桶**。熔炼与烟熏的原料完全相同（都是生肉），只在
 *     cookTime 上不同——桶装错的话，高炉里能烧出生肉，而每条配方单看都正常。
 *   - **切石的可选产物漏没漏**。切石机界面列出该输入的全部产物，少一条就少一个选项。
 *
 * 因此这里比的是「以物品为入口的查询结果」，与按 ID 的静态导出互补。
 */

import type { HarnessDeclaration } from "../core/types.ts";

/** 输出里除 `unit` / `ok` / `error` 之外的字段；失败侧要补齐这些键。 */
const RESULT_FIELDS = ["furnace", "blast", "smoke", "campfire", "stonecutting"] as const;

/**
 * 比对前的规范化。
 *
 * 失败侧只给 `{unit, ok:false, error}`，成功侧还有五个查询字段。比较层只比参照侧
 * 声明过的字段，缺键的一侧会被当成「没得比」而静默通过——「原版能烧、本侧物品没注册」
 * 正是最该被看见的差异。因此在**两侧**都把这五个键补齐，缺失显式落成 null。
 */
export function normaliseSingleItemRecipe(_expected: unknown, actual: unknown): unknown {
    if (actual === null || typeof actual !== "object") {
        return actual;
    }
    const record = actual as Record<string, unknown>;
    const result: Record<string, unknown> = {
        unit: record["unit"] ?? null,
        ok: record["ok"] ?? null,
    };
    for (const field of RESULT_FIELDS) {
        result[field] = record[field] ?? null;
    }
    return result;
}

normaliseSingleItemRecipe.canonicalising = true;

/** 能力声明。 */
export function singleItemRecipeDeclaration(): HarnessDeclaration {
    return {
        name: "single_item_recipe",
        probe: "single_item_recipe",
        oracle: "SingleItemRecipeOracle",
        key: "unit",
        units: { kind: "listing" },
        input: (unit: string): { unit: string } => ({ unit }),
        normalise: normaliseSingleItemRecipe,
        // oracle 要在 java.io.tmpdir 里建原版服务端世界并持有区域锁：同组必须串行。
        exclusiveGroup: "loot-world",
    };
}