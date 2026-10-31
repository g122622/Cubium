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
 * armor_trim.ts — 盔甲纹饰能力的输入构造与无序字段声明。
 *
 * 单元是 `pattern/<id>` 与 `material/<id>` 两种前缀的合集，由 oracle 的 `list` 决定
 * （数据包 `trim_pattern/` 与 `trim_material/` 下有哪些 JSON 就是全集）。
 *
 * 每个图案单元回答「贴图 ID、描述翻译键、是否贴花」；每个材料单元回答
 * 「贴图后缀、描述翻译键、描述颜色、覆盖的装备材质表」。
 */

/**
 * 单元 → 探针与 oracle 共用的一行输入。
 *
 * 前缀决定探针走图案分支还是材料分支；`pattern` / `material` 字段是真正被查询的 ID。
 */
export function armorTrimInput(unit: string): Record<string, string> {
    if (unit.startsWith("pattern/")) {
        return { unit, pattern: unit.slice("pattern/".length) };
    }
    if (unit.startsWith("material/")) {
        return { unit, material: unit.slice("material/".length) };
    }
    throw new Error(`armor_trim: 单元必须以 pattern/ 或 material/ 开头，收到 ${unit}`);
}

/**
 * 材料的 `overrides` 是无序表（原版是 `Map<ResourceKey<EquipmentAsset>, AssetInfo>`）。
 *
 * 两侧都按装备材质键排序后输出，这里再声明一次，让比较器不依赖顺序。
 */
export const ARMOR_TRIM_UNORDERED = [
    { key: "*", field: "overrides", reason: "原版 overrides 是 Map，遍历顺序不参与语义" },
] as const;