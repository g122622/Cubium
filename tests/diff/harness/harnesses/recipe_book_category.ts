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
 * recipe_book_category.ts — 每条配方落在配方书的哪个页签。
 *
 * 单元是配方 ID。两侧各输出两项：`type`（序列化器 ID）与 `page`（页签归属）。
 *
 * `page` 是原版 `Recipe#recipeBookCategory()` 的结果——它已经把「配方类型」与
 * 数据包 JSON 里的 `category` 字段压成一个注册表键。因此只比这一个字段，就能同时
 * 发现两类错误：分类字段解析错了（例如把 `equipment` 当成缺省落进 misc），以及
 * 类型映射错了（例如给高炉造出一个原版不存在的「食物页」）。
 */

import type { HarnessDeclaration } from "../core/types.ts";

/**
 * 比对前的规范化。
 *
 * 成功侧有 `type` 与 `page`，失败侧只有 `{recipe, ok:false, error}`。比较层只比
 * 参照侧声明过的字段——缺键的一侧会被当成「没得比」静默通过，而「原版认识这条配方、
 * 本侧解析失败」正是最该被看见的情况。
 */
export function normaliseRecipeBookCategory(_expected: unknown, actual: unknown): unknown {
    if (actual === null || typeof actual !== "object") {
        return actual;
    }
    const record = actual as Record<string, unknown>;
    return {
        ok: record["ok"] ?? null,
        type: record["type"] ?? null,
        page: record["page"] ?? null,
    };
}

normaliseRecipeBookCategory.canonicalising = true;

/** 能力声明。 */
export function recipeBookCategoryDeclaration(): HarnessDeclaration {
    return {
        name: "recipe_book_category",
        probe: "recipe_book_category",
        oracle: "RecipeBookCategoryOracle",
        key: "recipe",
        units: { kind: "listing" },
        input: (recipe: string): { recipe: string } => ({ recipe }),
        normalise: normaliseRecipeBookCategory,
        // oracle 要在 java.io.tmpdir 里建原版服务端世界并持有区域锁：同组必须串行。
        exclusiveGroup: "loot-world",
    };
}