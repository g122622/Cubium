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
 * animal_variant.ts — 动物变体能力的输入构造。
 *
 * 单元形如 `wolf_variant/minecraft:pale`，由 oracle 的 `list` 决定
 * （六类变体目录下有哪些 JSON 就是全集）。
 */

const VARIANT_KINDS = [
    "wolf_variant",
    "cat_variant",
    "frog_variant",
    "cow_variant",
    "chicken_variant",
    "pig_variant",
] as const;

/**
 * 单元 → 探针与 oracle 共用的一行输入。
 *
 * `kind` 决定查哪一类注册表；`variant` 是真正被查询的 ID。
 */
export function animalVariantInput(unit: string): Record<string, string> {
    const slash = unit.indexOf("/");
    if (slash < 0) {
        throw new Error(`animal_variant: 单元必须是 <kind>/<id> 形式，收到 ${unit}`);
    }
    const kind = unit.slice(0, slash);
    if (!(VARIANT_KINDS as readonly string[]).includes(kind)) {
        throw new Error(`animal_variant: 未知变体类别 ${kind}`);
    }
    return { unit, kind, variant: unit.slice(slash + 1) };
}